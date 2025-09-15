#ifndef MEM_INT_H_
#define MEM_INT_H_

#include "memoria_interna.h"
#include <utilsWorker/api_memoria.h>
#include <utilsWorker/globales.h>
#include <commons/string.h>

// tengo que poner este para no poner el include y tener dependencia circular
typedef struct t_formato t_formato;

typedef struct {
    bool ocupado; // indica si está asignado a una pagina
    bool modificado;
    bool uso;
    char* file;
    char* tag;
    int page_num;
} frame;

// Entrada en la tabla de páginas
typedef struct {
    int indice_frame;
    bool presente; // indica si esta en memoria o en storage
    bool modificado;
    bool uso;
} entrada_pag;

typedef struct {
    char* file;
    char* tag;
    t_dictionary* paginas; // clave = nro_pagina (string), valor = entrada_pag*
} tabla_pag;

int obtener_marco_libre_o_victima();
int elegir_victima();

#endif 