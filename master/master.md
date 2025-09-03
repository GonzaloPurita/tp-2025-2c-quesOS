# Master

Es el Kernel, es decir: El encargado de la gestión de Query (vendrían a ser como los procesos).

- [Master](#master)
  - [Query Control](#query-control)
    - [Desconexión de Query Control](#desconexión-de-query-control)
      - [Query en READY](#query-en-ready)
      - [Query en EXEC](#query-en-exec)
  - [Planificación](#planificación)
    - [Planificador a largo plazo](#planificador-a-largo-plazo)
    - [PLanificador a corto plazo](#planificador-a-corto-plazo)
      - [FIFO](#fifo)
      - [Prioridades con desalojo y Aging](#prioridades-con-desalojo-y-aging)
        - [Criterio para desalojar](#criterio-para-desalojar)
        - [Aging](#aging)
          - [Ejemplos](#ejemplos)
  - [Query](#query)
    - [Estructura de Query](#estructura-de-query)
  - [Workers](#workers)
    - [Desconexión de Workers](#desconexión-de-workers)


## Query Control

Disponemos de un puerto para escuchar las conexiones de las Query Control. Estas conexiones **se deben mantener** hasta que termine la Query.

- Las lecturas que se realicen deben reenviarse al Query Control
- Si la Query termina se le informa.
- Se pueden desconectar

### Desconexión de Query Control

Pueden darse dos situciones:

#### Query en READY

Significa que la Query no se esta ejecutando.

1) Debemos enviar la Query a EXIT


#### Query en EXEC

Significa que la Query se esta ejecutando

1) Debemos informar al Worker correspondiente que debe desalojar dicha Query.
2) Enviarla a EXIT.

## Planificación

Las Query pueden estar en alguno de los siguientes estados:

- READY
- EXEC
- EXIT

### Planificador a largo plazo

Todas las Query comienzan en el estado **READY**, por lo tanto no hay planificación a largo plazo. :c

### PLanificador a corto plazo

Va encargarse de pasar los procesos de READY a EXEC y desalajar Querys que esten en EXEC pasandolas a READY.

#### FIFO

FIFO --> Orden de llegada.

#### Prioridades con desalojo y Aging

Se envian las Querys según su prioridad. 

> A mayor número --> Menor prioridad.

Sería bastante bueno contar con algo como:

```c
t_query* obtenerQueryConMenorPrioridad(t_list* cola) {
    t_query* query = list_get_minimum(cola, minPrioridad);
    if (query == NULL) {
        log_error(...)
    }
    return query;
}

void* minPrioridad(void* a, void* b) {
    t_query* queryA = (t_query*) a;
    t_query* queryB = (t_query*) b;

    return queryA->prioridad <= queryB->prioridad ? queryA : queryB;
    // Revisar como desempata, ya desaprobamos un TP por este igual.
}
```

ya que se utiliza frecuentemente.

##### Criterio para desalojar

Se va a desalojar la Query con menor prioridad.

1) Se busca la Query con la menor prioridad.
2) Comparamos esa Query con el candidato (prioridades). 
3) Solicitar al Worker su desalojo.
4) Nos devuelve el PC y actualizamos el valor en el `t_query`.

Dice que cuando volvamos a planificar una Query le tenemos que mandar el PC para que retome desde el lugar correcto, pero si siempre lo mandamos y lo tenemos actualizado no debería haber problema.


##### Aging

Se aumenta la prioridad (reducimos el número) de **los procesos en READY**.

Se aumenta la prioridad (reduce el número) de 1 en 1 cada un tiempo definido en la config.

Para esto vamos a tener que hacer como el temporizador que pasaba a SUSP.BLOCKED en el TP anterior. Es decir:

- Se lanza un hilo cuando entra en READY.
- Se le asigna el ID de ese hilo a la Query. Esto es para: (esta es la solución que encontre yo en el TP anterior, quizas haya al mejor)
  - Una Query solo tenga un hilo que le cambie la prioridad. Ejemplo: 1.
  - Una Query cambie de prioridad correctamente. Ejemplo 2.
- EL hilo hace el usleep por la cantidad de tiempo definida en la config.
- Comprueba que este en READY
- Comprueba que la Query tenga el mismo ID que el hilo.
- Reduce la prioridad
  - Si la prioridad es 0 no se reduce.
- Hay que ver como se maneja la cola de READY
  - Si esta ordenada por prioridad tener en cuenta que cuando le cambie el lugar respete FIFO.
- Seguro que haya que avisar que tiene que replanificar.
- Lanzo otro hilo igual a este.
  - Si la prioridad es 0 no se lanza otro hilo.

El ID puede ser algo simple como el QID.

###### Ejemplos

Ejemplo 1:

```
Llega proceso a READY inicio un contador de 5seg, por ejemplo.
A los 2seg paso a EXEC. Esa Query tiene ahora un hilo con 3seg restantes.
A los 2seg paso a READY. Esa Query tiene ahora dos hilos:
    El primero con 1seg restante
    El segundo con 5seg restantes (Nuevo)
```

Ejemplo 2:
```
Siguiendo con el ejemplo anterior:

Pasan 7seg en READY y esa Query bajo dos niveles de prioridad cuando no debería.
    Tendría que bajar solo uno, ya que solo estuvo 5seg en READY.
```

## Query

Al recibir una Query lo primero que debemos hacer es asignarle un ID.

### Estructura de Query

Por ahora una Query cuenta con:

``` c
typedef {
    t_qcb* QCB; // PCB pero de Query
    char* path; // La ruta al archivo
    int prioridad; // La prioridad que tiene
    int IDAging; // Se explica en la parte de AGING
    t_estado estado; // OPCIONAL: Podemos guardar el estado en el que esta.
}t_query;


typedef {
    int QID; // Por Query ID
    int PC: // Program Counter
}t_qcb;
```

## Workers

Vendrían a representar las CPU, donde se ejecutan las Query.

- Tenemos varios Workers.
- La cantidad de Workers nos indica el grado de multiprogramación.

Cuando se conecta un nuevo Worker se tiene que planificar según el algoritmo.

### Desconexión de Workers

Se puede desconectar en tiempo de ejecución. Si estaba ejecutando una Query:

1) Pasar la Query a EXIT.
2) Notificar al Query control correspondiente que finalizo con ERROR.