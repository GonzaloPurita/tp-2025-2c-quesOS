#ifndef GLOBALES_H_
#define GLOBALES_H_

#include <commons/collections/dictionary.h>
#include <stdlib.h>
#include <stdatomic.h>

// tengo que poner este para no poner el include y tener dependencia circular
typedef struct frame frame;
typedef struct tabla_pag tabla_pag;
typedef struct t_config_worker t_config_worker;

// Conexiones globales
extern int conexionMaster;
extern int conexionStorage;

extern t_log* loggerWorker;
extern t_config_worker* configWorker;

extern atomic_bool interrupcion;

// Memoria
extern int TAM_PAGINA;       // lo manda Storage
extern int TAM_MEMORIA;      // viene del config
extern int CANTIDAD_MARCOS;  // calculado
extern void* MEMORIA;        // malloc general

extern frame* frames;

// Datos
extern int TAM_BLOQUE;
extern int PC_ACTUAL;

// Diccionarios
extern t_dictionary* diccionario_tablas; // clave = "file:tag", valor = tabla_pag*

// Funciones

void iniciar_memoria();


#endif
