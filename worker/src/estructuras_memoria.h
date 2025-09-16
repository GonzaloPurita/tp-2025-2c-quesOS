#ifndef ESTRUCTURAS_MEMORIA_H
#define ESTRUCTURAS_MEMORIA_H

#include <stdbool.h>
#include <commons/collections/dictionary.h>

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

#endif