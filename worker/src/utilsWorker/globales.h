#ifndef GLOBALES_H_
#define GLOBALES_H_

#include <stdatomic.h>
#include <commons/collections/dictionary.h>
#include <commons/log.h>
#include "estructuras_memoria.h"

typedef struct t_config_worker t_config_worker;
// typedef struct frame frame;
// typedef struct tabla_pag tabla_pag;
typedef struct t_query_context t_query_context;

/* conexiones */
extern int conexionMaster;
extern int conexionStorage;

/* logger global */
extern t_log* loggerWorker;

/* config global */
extern t_config_worker* configWorker;

/* flags */
extern atomic_int interrupt_flag;
extern atomic_int query_error_flag;

/* otros externos ... */
extern t_query_context* query_actual;

/* memoria */
extern int TAM_PAGINA;
extern int TAM_MEMORIA;
extern int CANTIDAD_MARCOS;
extern void* MEMORIA;
extern frame* frames;

/* diccionarios */
extern t_dictionary* diccionario_tablas;

/* querys */
extern int PC_ACTUAL;
extern int TAM_BLOQUE;

extern int puntero_clock;

void iniciar_memoria();

#endif
