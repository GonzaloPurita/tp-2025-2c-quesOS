#ifndef GLOBALES_H_
#define GLOBALES_H_

#include <commons/config.h>
#include <commons/log.h>

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

// Funciones

void iniciar_memoria();


#endif /* GLOBALES_H_ */
