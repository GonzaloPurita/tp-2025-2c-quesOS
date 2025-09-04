#ifndef GLOBALES_H_
#define GLOBALES_H_

#include <commons/config.h>
#include "main.h"
#include <commons/log.h>

typedef struct {
    int query_id;      
    char* nombre_query;
    int pc_inicial;         
} t_query_context;

// Conexiones globales
extern int conexionMaster;
extern int conexionStorage;

// Memoria
extern int TAM_PAGINA;       // lo manda Storage
extern int TAM_MEMORIA;      // viene del config
extern int CANTIDAD_MARCOS;  // calculado
extern void* MEMORIA;        // malloc general

// Datos
extern int TAM_BLOQUE;
extern int PC_ACTUAL;

// QUERY
extern t_query_context* QUERY_ACTUAL;

// Funciones

void iniciar_memoria();
void ejecutar_query(t_query_context* ctx);


#endif /* GLOBALES_H_ */
