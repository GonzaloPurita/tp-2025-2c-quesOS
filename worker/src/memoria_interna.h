#ifndef MEM_INT_H_
#define MEM_INT_H_

#include <utilsWorker/api_memoria.h>
#include <utilsWorker/globales.h>
#include <commons/string.h>
#include <utilsWorker/formato.h>

struct frame {
    bool ocupado; // indica si está asignado a una pagina
    bool modificado;
    bool uso;
    char* file;
    char* tag;
    int page_num;
};

// Entrada en la tabla de páginas
struct entrada_pag {
    int indice_frame;
    bool presente; // indica si esta en memoria o en storage
    bool modificado;
    bool uso;
};

struct tabla_pag {
    char* file;
    char* tag;
    t_dictionary* paginas; // clave = nro_pagina (string), valor = entrada_pag*
};


int obtener_marco_libre_o_victima();
int elegir_victima();

#endif 