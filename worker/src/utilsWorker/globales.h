#ifndef GLOBALES_H_
#define GLOBALES_H_

#include "../main.h"
#include "../memoria_interna.h"

// Conexiones globales
extern int conexionMaster;
extern int conexionStorage;

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
