# Worker

El **Worker** representa la CPU: es el encargado de **ejecutar una Query instrucción por instrucción**.  

- [Worker](#worker)
  - [Inicialización](#inicialización)
  - [Query Interpreter](#query-interpreter)
    - [Instrucciones](#instrucciones)
      - [CREATE](#create)
      - [TRUNCATE](#truncate)
      - [WRITE](#write)
      - [READ](#read)
      - [TAG](#tag)
      - [COMMIT](#commit)
      - [FLUSH](#flush)
      - [DELETE](#delete)
      - [END](#end)
  - [Memoria Interna](#memoria-interna)
    - [Esquema de Paginación](#esquema-de-paginación)
    - [Algoritmos de Reemplazo](#algoritmos-de-reemplazo)
      - [LRU](#lru)
      - [CLOCK-M](#clock-m)
  - [Logs](#logs)

---

## Inicialización

Al iniciar un Worker:

1. Recibe parámetros: `./bin/worker [archivo_config] [ID Worker]`.
2. Se conecta primero a **Storage** (para conocer tamaño de bloque).
3. Luego se conecta al **Master**, y envia su ID.
4. Espera a que el Master le asigne Queries. (igual q a cpu con kernel)

---

## Query Interpreter

Es el encargado de **leer y ejecutar** las instrucciones de la Query.  

- Recibe del Master: `path_query` + `program counter (PC)`.
- Busca la Query en el path indicado en config.
- Empieza a ejecutar instrucción por instrucción respetando el retardo configurado.
- Se comunica con:
  - **Memoria Interna** (lectura/escritura de páginas).
  - **Storage** (cuando falta una página o hay operaciones de persistencia).
  - **Master** (para enviar resultados de lecturas o avisar fin).

## Instrucciones

### CREATE

    CREATE <NOMBRE_FILE>:<TAG>

Solicita al Storage la creación de un File con tamaño 0.  

#### ejemplo 

En la Query:

CREATE MATERIAS:BASE

El Worker parsea → File = MATERIAS, Tag = BASE.

Arma un paquete tipo CREATE_REQUEST:

```c
typedef struct {
    char* nombre_file;
    char* tag;
} t_create;
```

Lo envía a Storage.

---

### TRUNCATE

    TRUNCATE <NOMBRE_FILE>:<TAG> <TAMAÑO>

Modifica el tamaño de un File:Tag. El tamaño debe ser múltiplo del tamaño de bloque.  

En la Query:

TRUNCATE MATERIAS:BASE 1024

El Worker parsea → File = MATERIAS, Tag = BASE, Tamaño = 1024.

Arma un paquete tipo TRUNCATE_REQUEST

```c
typedef struct {
    char* nombre_file;
    char* tag;
    int tamanio;
} t_truncate;
```

Lo envía a Storage.

---

### TAG

    TAG <FILE_ORIGEN>:<TAG_ORIGEN> <FILE_DEST>:<TAG_DESTINO>

Pide a Storage crear un nuevo File:Tag a partir de otro.  

```c
typedef struct {
    char* file_origen;   
    char* tag_origen;    
    char* file_destino;  
    char* tag_destino;   
} t_tag_request;
```

Parsear origen y destino.
Armar un paquete TAG_REQUEST.
Enviarlo a Storage.

---

### COMMIT
parecido a los dos primeros

    COMMIT <NOMBRE_FILE>:<TAG>

```c
typedef struct {
    char* nombre_file;
    char* tag;
} t_commit_request;
```

Indica a Storage que el File:Tag queda confirmado y no se modifica más.  

---

### DELETE
parecido a los dos primeros

    DELETE <NOMBRE_FILE>:<TAG>

```c
typedef struct {
    char* nombre_file;
    char* tag;
} t_delete_request;
```

Pide a Storage eliminar el File:Tag.  

---

### END
parecido a los dos primeros

    END

Finaliza la Query y avisa al Master.  

--- 


### FLUSH

    FLUSH <NOMBRE_FILE>:<TAG>


Persiste en Storage todo lo modificado en Memoria Interna.

Se ejecuta también **implícitamente**:
- Antes de un COMMIT.
- Antes de un desalojo de la Query.  

---
### WRITE

    WRITE <NOMBRE_FILE>:<TAG> <DIRECCION_BASE> <CONTENIDO>


Escribe en **Memoria Interna** a partir de la dirección base.  
Si no tiene las páginas necesarias → las pide a Storage.  

---

#### READ

    READ <NOMBRE_FILE>:<TAG> <DIRECCION_BASE> <TAMAÑO>


Lee de **Memoria Interna** y envía el contenido al Master.  
Si no tiene las páginas necesarias → las pide a Storage.  


(algo que tmb puede ser)
```c
typedef enum {
    CREATE_REQUEST,
    TRUNCATE_REQUEST,
    WRITE_REQUEST,
    READ_REQUEST,
    TAG_REQUEST,
    FILE_TAG_REQUEST // <-  COMMIT, FLUSH, DELETE
} t_opcode;
```


## Memoria Interna

- Un único `malloc()` de tamaño definido en config (`TAM_MEMORIA`).
- Paginación simple a demanda.  
- El tamaño de página = tamaño de bloque del Storage (obtenido en handshake).
- Cada acceso de lectura/escritura tiene un retardo (`RETARDO_MEMORIA`).
- Mantiene una tabla de páginas por cada File:Tag con páginas cargadas.

### Esquema de Paginación

1. Si la página solicitada **está en memoria** → se usa.
2. Si no está (page fault) → se pide a Storage y se carga en memoria.
3. Si no hay marcos libres → se reemplaza una página usando el algoritmo configurado.

### Algoritmos de Reemplazo

#### LRU

- Se reemplaza la página menos recientemente usada.

#### CLOCK-M

- Variante del algoritmo del reloj, con bit de uso y bit de modificado.  
- Si la víctima está modificada → primero se persiste en Storage.

---

## Logs

Ejemplos de logs obligatorios:

- **Recepción de Query:**  
  `## Query <QID>: Se recibe la Query. Path: <PATH_QUERY>`

- **FETCH instrucción:**  
  `## Query <QID>: FETCH - PC: <PC> - <INSTRUCCIÓN>`

- **Instrucción realizada:**  
  `## Query <QID>: - Instrucción realizada: <INSTRUCCIÓN>`

- **Desalojo:**  
  `## Query <QID>: Desalojada por pedido del Master`

- **Lectura/escritura en memoria:**  
  `Query <QID>: Acción: <LEER/ESCRIBIR> - Dirección Física: <DIR> - Valor: <VALOR>`

- **Asignación de marco:**  
  `Query <QID>: Se asigna el Marco <M> a Página <P> - File:Tag <F:T>`

- **Liberación de marco:**  
  `Query <QID>: Se libera el Marco <M> - File:Tag <F:T>`

- **Reemplazo:**  
  `## Query <QID>: Se reemplaza la página <File1:Tag1>/<Pag1> por <File2:Tag2>/<Pag2>`

- **Memoria Miss / Add:**  
  `Query <QID>: - Memoria Miss - File: <F> - Tag: <T> - Pagina: <P>`  
  `Query <QID>: - Memoria Add - File: <F> - Tag: <T> - Pagina: <P> - Marco: <M>`CREATE <NOMBRE_FILE>:<TAG>

