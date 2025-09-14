# Storage 

Este módulo representa un File System (FS) en el cual los workers pueden operar de forma **concurrente**.

- [Storage](#storage)
  - [Inicialización](#inicialización)
    - [Estructura de directorios y archivos](#estructura-de-directorios-y-archivos)
      - [Archivo `superblock.config`](#archivo-superblockconfig)
      - [Archivo `bitmap.bin`](#archivo-bitmapbin)
        - [`mmap()`](#mmap)
        - [Archivos en C](#archivos-en-c)
      - [Archivo `blocks_hash_index.config`](#archivo-blocks_hash_indexconfig)
        - [Hasheo](#hasheo)
      - [Directorio `physical_blocks`](#directorio-physical_blocks)
      - [Directorio `files`](#directorio-files)
        - [Hardlink](#hardlink)
      - [Ejemplo de estructura](#ejemplo-de-estructura)
  - [Operaciones](#operaciones)
    - [Creación de File](#creación-de-file)
    - [Truncado de archivo](#truncado-de-archivo)
      - [Incrementar el tamaño](#incrementar-el-tamaño)
      - [Reducir el tamaño](#reducir-el-tamaño)
      - [`stat`](#stat)
    - [Tag de File](#tag-de-file)
    - [Commit de un Tag](#commit-de-un-tag)
    - [Escritura de Bloque](#escritura-de-bloque)
    - [Lectura de Bloque](#lectura-de-bloque)
    - [Eliminar un Tag](#eliminar-un-tag)
  - [Errores](#errores)
  - [Retardo](#retardo)
  - [Structs y variables importantes](#structs-y-variables-importantes)
  - [Funciones importantes](#funciones-importantes)
  - [Preguntas:](#preguntas)


## Inicialización

1) Debe leer la configuración.
   1) Verificar el valor de `FRESH_START`
      1) De ser necesario creará los archivos y estructuras necesarias para inicializar el FS.
         1) El único archivo que necesitamos tener obligatoriamente es `superblock.config`.
         2) Nos da los datos necesarios para poder formatear el FS.
         3) Debemos crear el primer File `initial_file`.
         4) Tenemos que darle un único TAG `BASE`.
         5) Su contenido es 1 bloque lógico con el bloque físico 0 asignado.
         6) Completar el bloque con el caracter 0. ??????????
         7) Este File/Tag no se podrá borrar.
      2) Puede ser que use los archivos ya existentes.
2) Crear un servidor multihilo para esperar la conexión de los Workers

> Un hilo por cada Worker que este atendiendo.

### Estructura de directorios y archivos

El FS se montra sobre un path definido por el archivo de configuración.

**Raiz**: Es la ruta sobre la cual se monta el FS.

Cuenta con 3 archivos inicialmente

#### Archivo `superblock.config`

Contiene la información administrativa del FS y debe ser compatible con las configs de la bibloteca commons. Contiene mínimamente:

* Tamaño del FS en Bytes
* Tamaño de bloque en Bytes

Ejemplo: 

```
FS_SIZE=4096
BLOCK_SIZE=128
```

#### Archivo `bitmap.bin`

Es un `bitmap` de los bloques del FS, sirve para identificar los bloques que están reservados.

**Bitmap**: Consiste en un `array` de bits que indican el estado (libre/ocupado) de un elemento (en nuestro caso bloques).

* Debe ser compatible con el formate de `bitarray` de las `commons`.
* Se recomienda usar la función `mmap()`.

```
Ejemplo:

1 --> Ocupado
0 --> Libre

bitmap[3] = {1,0,0}

Se que el bloque 0 está ocupado y el resto libres.
```

> Si o Si hay que usar un bitmap, es decir un array de bits. Otra implementación es motivo de desaprobación. 💀

##### `mmap()`

Es una función de C que sirve para "mappear" (crear un vínculo) un espacio de memoria y un archivo. [Explicación completa](https://docs.utnso.com.ar/guias/programacion/mmap)

> Se encuentra incluida en la librería `sys/mman.h`.

`mmap()`: Es la que genera el vínculo. Tiene 6 parametros 🫠

1) Dirección: En nuestro caso podemos dejarlo en `NULL`.
2) Tamaño: Es el tamaño que va a tener el archivo
   1) Lo sabemos en general porque son archivos que representan bloques.
3) Permisos que se le dan al mappeo:
   1) `PROT_READ`
   2) `PROT_WRITE`
4) Banderas: En nuestro caso solo vamos a utilizar `MAP_FIXED`.
5) ***Fildes*** (File descriptor): Es una manera interna del sistema UNIX de describir a los archivos.
   1) Podemos usar una función: `fileno(FILE* stream)` que nos da el File descriptor para un FILE dado.
6) Off: Es un OFFSET que no vamos a usar.

`msync`: Actualiza el vinculo que creo el `mmap`. Se encarga de guardar el tamaño especificado del archivo de forma asincrónica o sincrónica. Tiene 3 parametros:

1) Dirección: Donde se tiene el mmap preparado.
2) Tamaño.
3) Banderas - Flags: La única que vamos a usar es `MS_SYNC`, para actualizar sincrónicamente.

##### Archivos en C

| Modo | Descripción | Archivo existe | Archivo no existe | Posición inicial |
|------|-------------|----------------|-------------------|------------------|
| **"r"** | Solo lectura (read) | ✅ Abre para lectura | ❌ Error (NULL) | Inicio del archivo |
| **"w"** | Solo escritura (write) | ❌ Borra contenido | ✅ Crea nuevo | Inicio del archivo |
| **"a"** | Append (añadir) | ✅ Abre para escribir al final | ✅ Crea nuevo | Final del archivo |
| **"r+"** | Lectura y escritura | ✅ Abre para leer/escribir | ❌ Error (NULL) | Inicio del archivo |
| **"w+"** | Lectura y escritura | ❌ Borra contenido | ✅ Crea nuevo | Inicio del archivo |
| **"a+"** | Lectura y append | ✅ Abre para leer/append | ✅ Crea nuevo | Lectura: inicio<br>Escritura: final |
| **"rb"** | Lectura binaria | ✅ Abre binario lectura | ❌ Error (NULL) | Inicio del archivo |
| **"wb"** | Escritura binaria | ❌ Borra contenido | ✅ Crea binario | Inicio del archivo |
| **"ab"** | Append binario | ✅ Abre binario append | ✅ Crea binario | Final del archivo |
| **"r+b"** | Lectura/escritura binaria | ✅ Abre binario r/w | ❌ Error (NULL) | Inicio del archivo |
| **"w+b"** | Lectura/escritura binaria | ❌ Borra contenido | ✅ Crea binario | Inicio del archivo |
| **"a+b"** | Lectura/append binario | ✅ Abre binario r/append | ✅ Crea binario | Lectura: inicio<br>Escritura: final |

#### Archivo `blocks_hash_index.config`

El objetivo es asociar cada **bloque físico ocupado** con un identificador único (hash) generado a partir del mismo. 

> Dos bloques lógicos con el mismo contenido generan el mismo hash por lo que se pueden asociar al mismo bloque físico.

```
Ejemplo:

4d186321c1a7f0f354b297e8914ab240 = block0000
ea5cb8ff0abf6b4ca0080069daaeada0 = block0001
67d6c28fac7541d9ce1f46ba4f84e149 = block0002
749dfe7c0cd3b291ec96d0bb8924cb46 = block0003
5058f1af8388633f609cadb75a75dc9d = block0004
```

##### Hasheo

Se debe utilizar la función `crypto_md5()` de las `commons`.

```c
char* crypto_md5(void* source, size_t lenght);
```

Dada una porción de memoria, calcula su hash MD5 y lo devuelve como una cadena de caracteres ASCII.

#### Directorio `physical_blocks`

Este directorio representa cada bloque físico del FS como un archivo de tamaño fijo definido por archivo de superbloque.

> Bloque físico ==> archivo.dat

#### Directorio `files`

> Archivo = File

* Representa cada File como una entrada de directorio
  * El nombre de la entrada de directorio es el nombre del File.
  * No pueden haber multiples Files con el mismo nombre.
* Cada directorio del File, trendrá dentro otra entrada al directorio por cada TAG 
  * El nombre de la entrada, es el nombre del TAG.
  * Un mismo File no puede tener dos TAGS con el mismo nombre.
  * Cada entrada TAG debe contar con un archivo `metadata.config`. El mismo cuenta con:
    * Tamaño del File
    * Estado del File `WORK_IN_PROGRESS` o `COMMITED`.
    * Lista ordenada de los números de bloques bloques **físicos** que pertenecen al File.
* Cada entrada TAG debe contar con otra entrada de directorio llamada `logical_blocks`.
  * Tendra un archivo por cada bloque físco que corresponde del FS
  * Cada archivo es un **hardlink**


**Ejemplo de metadata.config:**
```
Ejemplo de metadata.config

TAMAÑO = 160
BLOCKS = [17,2,5,10,8]
ESTADO = WORK_IN_PROGRESS
```

##### Hardlink

Un hardlink es una referencia al inodo de un archivo. Para esto usamos la función `link()`.

```c
#include <unistd.h>

int link(const char *oldpath, const char *newpath);
```

* `oldpath`: ruta al archivo original (tu bloque físico en physical_blocks/).
* `newpath`: ruta al nuevo archivo (el bloque lógico en logical_blocks/).
* Retorna 0 si fue exitoso, -1 si falla (y setea errno).

#### Ejemplo de estructura

```
/home/utnso/storage/
           ⤷ ./superblock.config
           ⤷ ./bitmap.bin
           ⤷ ./blocks_hash_index.config
           ⤷ ./physical_blocks/
              ⤷ ./block0000.dat
              ⤷ ./block0001.dat
              ⤷ ./block0002.dat
              ⤷ ./block0003.dat
              ⤷ ./block0004.dat
           ⤷ ./files/
              ⤷ ./initial_file/
                 ⤷ ./BASE/
                    ⤷ ./metadata.config
                    ⤷ ./logical_blocks/
                       ⤷ ./000000.dat # hard link a ‘block0000.dat’
              ⤷ ./arch1/
                 ⤷ ./tag_1_0_0/
                    ⤷ ./metadata.config
                    ⤷ ./logical_blocks/
                       ⤷ ./000000.dat # hard link a ‘block0003.dat’
                       ⤷ ./000001.dat # hard link a …
                       ⤷ ./000002.dat
                 ⤷ ./tag_2_0_0/
                    ⤷ ./metadata.config
                    ⤷ ./logical_blocks/
                       ⤷ ./000000.dat
                       ⤷ ./000001.dat
                       ⤷ ./000002.dat
                 ⤷ ./tag_3_0_0/
                    ⤷ ./metadata.config
                    ⤷ ./logical_blocks/
                       ⤷ ./000000.dat
                       ⤷ ./000001.dat
                       ⤷ ./000002.dat
                       ⤷ ./000003.dat
```
## Operaciones

Ofrece una serie de operaciones para que soliciten los Workers.

### Creación de File

Recibe:

* `OP_CODE`: `CREATE_FILE`
* Nombre del File 
* Nombre del TAG inicial 

Lo que debemos hacer:

1) Recibir los parametros.
   1) Nombre del File 
   2) Tag inicial
2) Debemos crear:
   1) Entrada del archivo
      1) Entrada del TAG
         1) Archivo metadata.config:
            1) Tamaño: 0 (Arranca sin bloques).
            2) Bloques: `[]`.
            3) Estado: `WORK_IN_PROGRESS`.
         2) Entrada de `logical_blocks`.
3) Deberiamos responderle con algo al worker.

### Truncado de archivo

Es la que modifica el tamaño del archivo. Recibe:

* `OP_CODE`: `TRUNCATE_FILE`
  * Propuesta:
    * `TRUNCATE_FILE_ADD`
    * `TRUNCATE_FILE_REDUCE`
* Nombre del Archivo
* Nombre del Tag
* El tamaño
  * El enunciado no es claro en esta parte tenemos que buscar una forma de identificar que ese Tamaño es para **disminuir** o **incrementar**.


#### Incrementar el tamaño

1) Recibimos el tamaño que queremos incrementar.
2) Determinamos cuantos bloques **nuevos** debemos asignar.
   1) Nuestro archivo tiene un tamaño `x` y nos piden incrementarlo en `y` lo que nos da el tamaño `total`. Cuando tenemos el `total` tenemos que volver a calcular la cantidad de bloques que necesita y en base a la diferencia con los que tiene sabemos cuantos nuevos hay que asignar.
   2) Con el `bitmap` sabemos si hay bloques libres
3) Le asignamos los bloques lógicos que necesite (hard links), todos apuntan al bloque físico 0.

#### Reducir el tamaño

1) Recibimos el tamaño que debemos reducir.
2) Le restamos ese tamaño al tamño del archivo.
3) Volvemos a recalcular los bloques que necesita.
4) Si "pierde" bloques debemos desasignarlos.
   1) Los que desasignamos son los bloque lógicos del final.
   2) Revisamos si ese bloque físico asociado a ese bloque lógico es referenciado por otro File:Tag. Usar `stat`.
   3) En caso de que no sea referenciado lo marcamos como libre en el `bitmap`.

#### `stat`



### Tag de File

Recibimos:

* `OP_CODE`: `FILE_TAG`.
* El nombre del Tag, para crearlo
* El nombre del archivo, para saber a cual se refiere ??

Debemos:

1) Crear una copia del directorio nativo (Tag nativo Origen)
2) La copia tiene el nombre de Tag que recibimos.
3) Debemos modicar el archivo `metadata.config`:
   1) Ponemos el estado en `WORK_IN_PROGRESS`.

### Commit de un Tag

Recibimos:

* `OP_CODE`: `COMMIT`.
* El nombre del File.
* El nombre del Tag.

> Si el Tag ya se encuentra confirmado no hacemos nada.

Debemos:

1) Actualizar la información del archivo `metada.config`.
   1) Ponemos el estado en `COMMITED`.
   2) Si ya estaba en `COMMITED` no hacemos nada.
2) Debemos analizar cada bloque lógico con su `hash` y comparar en el archivo `blocks_hash_index.config`
   1) Puede haber un bloque físico con ese hash:
      1) Debemos liberar el bloque físico actual.
      2) Re-apuntar el bloque lógico al bloque físico que tenía el mismo contenido.
   2) Puede no haber un bloque con ese hash:
      1) Agregamos el hash al archivo

### Escritura de Bloque

Recibimos:

* `OP_CODE`: `WRITE`.
* El nombre del File.
* El nombre del Tag.
* El contenido que se desea escribir
* El bloque lógico sobre el cual se escribe.

Debemos:

1) Revisar que el File:Tag no este en `COMMITED`.
2) Revisar que el bloque lógico se encuentre asignado.
   1) Entiendo que se refiere a que si mi File:Tag tiene 2 bloque lógicos no puedo escribir en el bloque lógico 5.
3) Debemos chequear las referencias entre el bloque lógico y el bloque físico:
   1) Si el bloque físico es referenciado únicamente por el bloque lógico:
      1) Escrbimos los datos directamente sobre el bloque físico. Entiendo que se sobreescribe todo el contenido del bloque por lo que nos llega.
   2) Si el bloque es referenciado por varios bloques lógicos:
      1) Buscamos un nuevo bloque físico libre.
      2) Escribimos la información en ese bloque.
      3) Nuestro bloque lógico ahora apunta a ese nuevo bloque.

### Lectura de Bloque

Recibimos:

* `OP_CODE`: `READ`
* Nombre del File.
* Nombre del Tag.
* Número de bloque lógico.

Debemos:

1) Buscamos el File
2) Buscamos el Tag
3) Obtenemos el bloque lógico
4) Con el bloque lógico sabemos a que bloque físico se refiere.
5) Leemos el bloque físico
6) Devolvemos lo que leimos

### Eliminar un Tag

Recibimos:

* `OP_CODE`: `DELETE`.
* Nombre del File.
* Nombre del Tag.

Debemos:

1) Buscar el File.
2) Buscar el Tag.
3) Tenemos que revisar cada bloque lógico:
   1) Revisamos si el bloque físico al que apunta es solo referenciado por este bloque lógico.
   2) En caso de ser el único, debemos marcar ese bloque físico como libre.
4) Borramos la entrada de directorio con sus archivos correspondientes. (la del Tag).

## Errores

Detallan el problema que ocurrio en la ejecución de una operación. Como si fueran excepciones.

Cada error se debe devolver al Worker, el cuál finalizará la Query.

* File inexistente
  * No aplica a cuando creamos el File.
* Tag Inexistente
  * No aplica a cuando creamos el Tag.
* Espacio insuficiente: Al buscar un bloque libre, no hay.
* Escritura no permitida: Trata de escribir o truncar un File:Tag que se encuentra en estado `COMMITED`.
* Lectura o escritura fuera de limite: Una lectura o escritura por fuera del tamaño File:Tag.

## Retardo

Cada operación tiene un retardo fijo definido en la config como `RETARDO_OPERACION`.

Cada lectura de un bloque tiene un retardo definido en la config como `RETARDO_ACCESO_BLOQUE`, que es por bloque.

## Structs y variables importantes

```c
t_dictionary* diccionarioFiles; // Guardo todos los File acá.

typedef struct {
   t_dictionary* diccionarioTags; // Asi se los Tags que tiene
   char* path; // OPCIONAL: Path de la entrada directorio
}t_file;

typedef struct {
   t_metadata data;
   char* path; // OPCIONAL: Path de la entrada de directorio
}

typedef struct {
   int tamanio; // Tamaño del File
   t_list* bloques; // Lista de los números de los bloques físicos asignados.
   t_estadoTag estado; // Estado del Tag
}t_metadata;

typedef enum {
   WORK_IN_PROGRESS;
   COMMITED;
}t_estadoTag;
```

## Funciones importantes

```c
t_list* buscarReferencias(FILE* bloqueFisico) {
   // Encuentra los bloques lógicos que referencian a un bloque físico
}
// Hay que definir que tipo de dato es un bloque físico. 

```

## Preguntas:

1) Cita: 

```
Tag de File

Esta operación creará una copia completa del directorio nativo correspondiente al Tag de origen...
```

¿El Tag de origen se refiere al primer Tag que tiene? ¿Qué sería copiar el directorio nativo? Entiendo que deberíamos hacer una copia del último tag, el directorio de `logical_blocks`
