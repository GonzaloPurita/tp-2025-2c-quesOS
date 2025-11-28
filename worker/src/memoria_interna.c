#include "memoria_interna.h"
#include "query_interpreter.h"
#include "utilsWorker/globales.h"
#include <commons/string.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

t_list* paginas_necesarias(int direccion_base, int tamanio) {
    t_list* paginas = list_create();

    int primera = direccion_base / TAM_PAGINA;
    int ultima = (direccion_base + tamanio - 1) / TAM_PAGINA;

    for (int p = primera; p <= ultima; p++) {
        int* nro = malloc(sizeof(int)); // guardo punteros porque t_list maneja punteros
        *nro = p;
        list_add(paginas, nro);
    }
    return paginas;
}

bool esta_en_memoria(t_formato* formato, int nro_pagina) {
    char* clave_tabla = string_from_format("%s:%s", formato->file_name, formato->tag);
    tabla_pag* tabla = dictionary_get(diccionario_tablas, clave_tabla);
    free(clave_tabla);

    if (tabla == NULL) {
        log_info(loggerWorker, "No existe tabla de páginas para %s:%s", formato->file_name, formato->tag);
        return false;
    }

    char* clave_pagina = string_from_format("%d", nro_pagina);
    entrada_pag* entrada = dictionary_get(tabla->paginas, clave_pagina);
    free(clave_pagina);

    if (entrada == NULL) {
        log_error(loggerWorker, "No existe entrada para la página %d en %s:%s", nro_pagina, formato->file_name, formato->tag);
        return false;
    } 

    return entrada->presente;
}

char* leer_desde_memoria(t_formato* formato, int direccion_base, int tamanio) {
    char* buffer = calloc(1, tamanio + 1); // el buffer arranca en 0, con un solo bloque de memoria, y con un tamaño

    int pagina_inicio = direccion_base / TAM_PAGINA;
    int pagina_fin = (direccion_base + tamanio - 1) / TAM_PAGINA;
    int offset = direccion_base % TAM_PAGINA;
    int bytes_restantes = tamanio;
    int posicion_buffer = 0;

    for (int p = pagina_inicio; p <= pagina_fin; p++) {
        char* clave = string_itoa(p);
        tabla_pag* tabla = dictionary_get(diccionario_tablas, string_from_format("%s:%s", formato->file_name, formato->tag));
        entrada_pag* entrada = dictionary_get(tabla->paginas, clave);
        free(clave);

        int frame = entrada->indice_frame;
        int dir_fisica = frame * TAM_PAGINA + offset;

        int bytes_a_usar = (bytes_restantes < TAM_PAGINA - offset) ? bytes_restantes : TAM_PAGINA - offset; // los bytes a usar van ser los que quedan o los que entran en la página

        memcpy(buffer + posicion_buffer, MEMORIA + dir_fisica, bytes_a_usar);

        bytes_restantes -= bytes_a_usar;
        posicion_buffer += bytes_a_usar;
        offset = 0; // a partir de la segunda página empieza desde 0, porque antes tal vez arrancaba con la primer pagina empezada, pero ya la segunda si o si arranc de cero
    }

    return buffer;
}

void escribir_en_memoria(t_formato* formato, int direccion_base, char* valor) {
    int tamanio_valor = strlen(valor);
    int pagina_inicio = direccion_base / TAM_PAGINA;
    int pagina_fin = (direccion_base + tamanio_valor - 1) / TAM_PAGINA;
    int offset = direccion_base % TAM_PAGINA;
    int bytes_restantes = tamanio_valor;
    int posicion_valor = 0;

    for (int p = pagina_inicio; p <= pagina_fin; p++) {
        char* clave = string_itoa(p);
        tabla_pag* tabla = dictionary_get(diccionario_tablas, string_from_format("%s:%s", formato->file_name, formato->tag));
        entrada_pag* entrada = dictionary_get(tabla->paginas, clave);
        free(clave);

        int frame = entrada->indice_frame;
        int dir_fisica = frame * TAM_PAGINA + offset;

        int bytes_a_usar = (bytes_restantes < TAM_PAGINA - offset) ? bytes_restantes : TAM_PAGINA - offset;

        memcpy(MEMORIA + dir_fisica, valor + posicion_valor, bytes_a_usar); // estoy guardando la posicion en donde se queda el string valor porque ese string puede quedar repartido en varias paginas

        entrada->modificado = true; // marco como la entrada como modificada
        frames[frame].modificado = true; // marco el frame como modificado

        bytes_restantes -= bytes_a_usar;
        posicion_valor += bytes_a_usar;
        offset = 0;
    }
}

int obtener_timestamp() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int)(tv.tv_sec) * 1000 + (tv.tv_usec / 1000); // tiempo en milisegundos
}

int obtener_marco_libre_o_victima() {
    //Busco un marco libre
    for (int i = 0; i < CANTIDAD_MARCOS; i++) {
        if (!frames[i].ocupado) {
            if(strcmp(configWorker->algoritmo_reemplazo, "LRU") == 0) {
                frames[i].timestamp = obtener_timestamp(); // actualizo el timestamp si es LRU
            }
            return i;
        }
    }
    return elegir_victima();
}

int elegir_victima() {
    if (strcmp(configWorker->algoritmo_reemplazo, "LRU") == 0) {
        return elegir_victima_LRU();
    } 
    else if (strcmp(configWorker->algoritmo_reemplazo, "CLOCK-M") == 0) {
        return elegir_victima_CLOCKM();
    }

    log_error(loggerWorker, "Algoritmo de reemplazo desconocido: %s", configWorker->algoritmo_reemplazo);
    return 0;
}

int elegir_victima_LRU() {
    int victima = 0;

    for (int i = 1; i < CANTIDAD_MARCOS; i++) { // no se si tiene que ser < o <=
        if (frames[i].timestamp < frames[victima].timestamp) {
            frames[i].timestamp = obtener_timestamp();
            victima = i;
        }
    }

    log_debug(loggerWorker, "Victima LRU seleccionada: marco %d", victima);
    return victima;
}

int elegir_victima_CLOCKM() {
    int vueltas = 0;
    int inicio = puntero_clock;

    while (1) {
        frame* frame = &frames[puntero_clock];

        // Caso 1: uso=0, modificado=0
        if (frame->ocupado && !frame->uso && !frame->modificado) {
            int victima = puntero_clock;
            puntero_clock = (puntero_clock + 1) % CANTIDAD_MARCOS;
            log_debug(loggerWorker, "Victima CLOCK-M seleccionada (uso=0,mod=0): marco %d", victima);
            return victima;
        }

        // Caso 2: uso=0, modificado=1
        if (vueltas > 0 && frame->ocupado && !frame->uso && frame->modificado) {
            int victima = puntero_clock;
            puntero_clock = (puntero_clock + 1) % CANTIDAD_MARCOS;
            log_debug(loggerWorker, "Victima CLOCK-M seleccionada (uso=0,mod=1): marco %d", victima);
            return victima;
        }

        // Si uso=1 → lo pongo en 0
        if (frame->uso) {
            frame->uso = false;
        }

        // avanzo el puntero
        puntero_clock = (puntero_clock + 1) % CANTIDAD_MARCOS;

        // ya di una vuelta completa
        if (puntero_clock == inicio) {
            vueltas++;
        }
    }
}

void pedir_pagina_a_storage(t_formato* formato, int nro_pagina){

    int marco = obtener_marco_libre_o_victima();

    int paginasXbloque = TAM_MEMORIA / TAM_BLOQUE;
    int nro_bloque = nro_pagina / paginasXbloque;

    // paquete para Storage (enviamos nro_pagina)
    t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion = PED_PAG;
    agregar_a_paquete(paquete, &query_actual->query_id, sizeof(int));
    agregar_a_paquete(paquete, formato->file_name, strlen(formato->file_name) + 1);
    agregar_a_paquete(paquete, formato->tag, strlen(formato->tag) + 1);
    agregar_a_paquete(paquete, &nro_bloque, sizeof(int));

    enviar_paquete(paquete, conexionStorage);
    eliminar_paquete(paquete);

    op_code cod_op = recibir_operacion(conexionStorage);

    if (cod_op == OP_SUCCESS) {
        t_list* lista = recibir_paquete(conexionStorage);
        char* contenido = list_get(lista, 0);

        int offset_en_bloque = (nro_pagina % paginasXbloque) * TAM_PAGINA;

        memcpy(MEMORIA + marco * TAM_PAGINA, contenido + offset_en_bloque, TAM_PAGINA); // copio el contenido recibido en el marco asignado

        // seteo el frame
        frames[marco].ocupado = true;
        frames[marco].modificado = false;
        frames[marco].uso = true;

        frames[marco].file = strdup(formato->file_name);
        frames[marco].tag = strdup(formato->tag);
        frames[marco].page_num = nro_pagina;
        frames[marco].timestamp = obtener_timestamp();

        // actualizo la tabla de páginas
        char* clave_file_tag = string_from_format("%s:%s", formato->file_name, formato->tag);
        ////char* clave = string_itoa(nro_pagina);
        // busco en la tabla
        tabla_pag* tabla = dictionary_get(diccionario_tablas, clave_file_tag);

        //si la tabla no existe (si es la primera escritura) ser crea!!!!
        if(tabla == NULL){
            log_info(loggerWorker, "Creando tabla de páginas nueva para %s:%s", formato->file_name, formato->tag);
            tabla = malloc(sizeof(tabla_pag));
            tabla->file = strdup(formato->file_name);
            tabla->tag = strdup(formato->tag);
            tabla->paginas = dictionary_create();
            dictionary_put(diccionario_tablas, clave_file_tag, tabla);
        }
        free(clave_file_tag);

        // ahora q nos aseguramos q la tabla existe..
        char* clave_pagina = string_itoa(nro_pagina);
        entrada_pag* entrada = malloc(sizeof(entrada_pag));
        entrada->indice_frame = marco;
        entrada->presente = true;
        entrada->modificado = false;
        entrada->uso = true;
        
        dictionary_put(tabla->paginas, clave_pagina, entrada);
        free(clave_pagina);

        list_destroy_and_destroy_elements(lista, free);
        log_debug(loggerWorker, "Contenido de página %d recibido desde Storage en marco %d", nro_pagina, marco);
    } else {
        // si Storage no responde SUCCESS
        log_error(loggerWorker, "Fallo al pedir página al Storage. Código recibido: %d", cod_op);
        t_list* basura = recibir_paquete(conexionStorage);
        list_destroy_and_destroy_elements(basura, free);
    }
}