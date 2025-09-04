#ifndef GLOBALES_H_
#define GLOBALES_H_

#include <commons/config.h>
#include "main.h"
#include <commons/log.h>

// Conexiones globales
extern int conexionMaster;
extern int conexionStorage;
extern char* puertoMaster;
extern char* puertoStorage;

// Datos globales
extern int TAM_BLOQUE;

extern int TAM_PAGINA;       // lo manda Storage
extern int TAM_MEMORIA;      // viene del config
extern int CANTIDAD_MARCOS;  // calculado
extern void* MEMORIA;        // malloc general


#endif /* GLOBALES_H_ */
