#ifndef MEM_INT_H_
#define MEM_INT_H_

#include <stdbool.h>
#include <commons/collections/dictionary.h>
#include <commons/collections/list.h>
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

t_list* paginas_necesarias(int direccion_base, int tamanio);
bool esta_en_memoria(t_formato* formato, int nro_pagina);
char* leer_desde_memoria(t_formato* formato, int direccion_base, int tamanio);
void escribir_en_memoria(t_formato* formato, int direccion_base, char* valor);
int obtener_marco_libre_o_victima();
int elegir_victima();
void pedir_pagina_a_storage(t_formato* formato, int nro_pagina);

#endif 