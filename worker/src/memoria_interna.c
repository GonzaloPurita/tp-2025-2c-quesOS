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
    char clave_tabla[256];
    snprintf(clave_tabla, sizeof(clave_tabla), "%s:%s", formato->file_name, formato->tag);

    tabla_pag* tabla = dictionary_get(diccionario_tablas, clave_tabla);
    if (!tabla) {
        log_info(loggerWorker, "No existe tabla de páginas para %s:%s", formato->file_name, formato->tag);
        return false;
    }

    char clave_pagina[16];
    snprintf(clave_pagina, sizeof(clave_pagina), "%d", nro_pagina);

    entrada_pag* entrada = dictionary_get(tabla->paginas, clave_pagina);

    if (!entrada) {
        log_warning(loggerWorker, "No existe entrada para la página %d en %s:%s", nro_pagina, formato->file_name, formato->tag);
        return false;
    }

    return entrada->presente;
}

char* leer_desde_memoria(t_formato* formato, int direccion_base, int tamanio) {
    char* buffer = calloc(1, tamanio + 1);

    int pagina_inicio = direccion_base / TAM_PAGINA;
    int pagina_fin = (direccion_base + tamanio - 1) / TAM_PAGINA;
    int offset = direccion_base % TAM_PAGINA;
    int bytes_restantes = tamanio;
    int posicion_buffer = 0;

    // Obtener tabla (ya sabemos que existe porque ejecutar_read lo validó, pero por seguridad...)
    char* clave_tabla = string_from_format("%s:%s", formato->file_name, formato->tag);
    tabla_pag* tabla  = dictionary_get(diccionario_tablas, clave_tabla);
    free(clave_tabla);

    if (!tabla) {
        free(buffer);
        return NULL;
    }

    for (int p = pagina_inicio; p <= pagina_fin; p++) {
        char* clave_pag = string_itoa(p);
        entrada_pag* entrada = dictionary_get(tabla->paginas, clave_pag);
        
        // --- FIX: AUTO-RECUPERACIÓN DE PÁGINA ---
        // Si la página fue desalojada por una posterior en la misma instrucción, la traemos de vuelta.
        if (entrada == NULL || !entrada->presente || entrada->indice_frame == -1) {
            log_warning(loggerWorker, "Page Fault dentro de READ (Thrashing): Recuperando Página %d", p);
            
            // Llamamos a la función de carga (que ya tiene mutex y todo)
            if (!pedir_pagina_a_storage(formato, p)) {
                log_error(loggerWorker, "Error fatal recuperando página %d en lectura.", p);
                free(clave_pag);
                free(buffer);
                return NULL; // O manejar error
            }
            
            // Refrescamos la entrada porque el puntero viejo pudo cambiar o invalidarse
            entrada = dictionary_get(tabla->paginas, clave_pag);
        }
        // ----------------------------------------

        free(clave_pag);

        int frame = entrada->indice_frame;
        int dir_fisica = frame * TAM_PAGINA + offset;

        // Actualizamos política
        entrada->uso = true;
        frames[frame].uso = true;
        frames[frame].timestamp = obtener_timestamp(); 

        int bytes_a_usar = (bytes_restantes < TAM_PAGINA - offset) ? bytes_restantes : TAM_PAGINA - offset;

        // Simulamos acceso a RAM
        usleep(configWorker->retardo_memoria * 1000);
        
        memcpy(buffer + posicion_buffer, MEMORIA + dir_fisica, bytes_a_usar);

        bytes_restantes -= bytes_a_usar;
        posicion_buffer += bytes_a_usar;
        offset = 0;
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

    char* clave_tabla = string_from_format("%s:%s", formato->file_name, formato->tag);
    tabla_pag* tabla  = dictionary_get(diccionario_tablas, clave_tabla);
    free(clave_tabla);

    for (int p = pagina_inicio; p <= pagina_fin; p++) { 
        char* clave_pag  = string_itoa(p);
        entrada_pag* entrada = dictionary_get(tabla->paginas, clave_pag);
        free(clave_pag);

        if (entrada == NULL) {
            log_warning(loggerWorker, "Intento de escritura en pagina %d no cargada", p);
            break;
        }

        int frame = entrada->indice_frame;
        int dir_fisica = frame * TAM_PAGINA + offset;

        // Calculamos cuánto escribir en esta página
        int bytes_a_usar = (bytes_restantes < TAM_PAGINA - offset) ? bytes_restantes : TAM_PAGINA - offset;

        usleep(configWorker->retardo_memoria * 1000);
        // Si la página es nueva, pedir_pagina_a_storage ya debió traerla limpia o con datos.
        memcpy(MEMORIA + dir_fisica, valor + posicion_valor, bytes_a_usar);

        log_info(loggerWorker, "Query %d: Acción: ESCRIBIR - Dirección Física: %d - Valor: %s", 
                 query_actual->query_id, dir_fisica, valor + posicion_valor);

        entrada->modificado = true; 
        frames[frame].modificado = true; 
        entrada->uso = true; 
        frames[frame].uso = true;

        bytes_restantes -= bytes_a_usar;
        posicion_valor += bytes_a_usar;
        offset = 0; // Para las siguientes páginas, el offset siempre arranca en 0
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
    int inicio = puntero_clock; // arranca en cero

    while (1) {
        frame* frame = &frames[puntero_clock];

        // Caso 1: uso=0, modificado=0
        if (frame->ocupado && !frame->uso && !frame->modificado) { // no hace falta chequear ocupado
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

bool pedir_pagina_a_storage(t_formato* formato, int nro_pagina) {
    pthread_mutex_lock(&mutex_memoria);

    log_info(loggerWorker, "Query %d: - Memoria Miss - File: %s - Tag: %s - Pagina: %d", 
             query_actual->query_id, formato->file_name, formato->tag, nro_pagina);

    // 1. Obtener/Crear Tabla
    char clave_tabla[128];
    snprintf(clave_tabla, sizeof(clave_tabla), "%s:%s", formato->file_name, formato->tag);

    tabla_pag* tabla = dictionary_get(diccionario_tablas, clave_tabla);

    if (tabla == NULL) {
        char* clave_heap = strdup(clave_tabla);
        tabla = malloc(sizeof(tabla_pag));
        tabla->file = strdup(formato->file_name);
        tabla->tag  = strdup(formato->tag);
        tabla->paginas = dictionary_create();
        dictionary_put(diccionario_tablas, clave_heap, tabla);
    }

    // 2. Elegir Marco
    int marco = obtener_marco_libre_o_victima();

    // 3. Desalojo (Swap Out)
    if (frames[marco].ocupado) {
        
        log_info(loggerWorker, "Query %d: Se libera el Marco: %d perteneciente al - File: %s - Tag: %s",
                 query_actual->query_id, marco, frames[marco].file, frames[marco].tag);

        // Write-Back
        if (frames[marco].modificado) {
            log_info(loggerWorker, "SWAP OUT: La página %d de %s:%s está sucia. Guardando en Storage...", 
                     frames[marco].page_num, frames[marco].file, frames[marco].tag);

            t_paquete* paquete = crear_paquete();
            paquete->codigo_operacion = OP_WRITE; 

            agregar_a_paquete(paquete, &query_actual->query_id, sizeof(int));
            agregar_a_paquete(paquete, frames[marco].file, strlen(frames[marco].file) + 1);
            agregar_a_paquete(paquete, frames[marco].tag, strlen(frames[marco].tag) + 1);
            agregar_a_paquete(paquete, &frames[marco].page_num, sizeof(int)); 

            void* contenido_victima = MEMORIA + (marco * TAM_PAGINA);
            agregar_a_paquete(paquete, contenido_victima, TAM_PAGINA);
            
            int tam_pag = TAM_PAGINA; 
            agregar_a_paquete(paquete, &tam_pag, sizeof(int));

            enviar_paquete(paquete, conexionStorage);
            eliminar_paquete(paquete);

            op_code rta = recibir_operacion(conexionStorage);
            t_list* rtaList = recibir_paquete(conexionStorage);
            if(rtaList) list_destroy_and_destroy_elements(rtaList, free);

            if (rta != OP_SUCCESS) {
                log_error(loggerWorker, "CRITICAL: Falló el guardado de página víctima.");
            }
        }

        // Invalidar tabla anterior
        char* clave_ant = string_from_format("%s:%s", frames[marco].file, frames[marco].tag);
        tabla_pag* tabla_ant = dictionary_get(diccionario_tablas, clave_ant);

        if (tabla_ant != NULL) {
            char* clave_pag_ant = string_itoa(frames[marco].page_num);
            entrada_pag* entrada_ant = dictionary_get(tabla_ant->paginas, clave_pag_ant);

            if (entrada_ant != NULL) {
                entrada_ant->presente = false;
                entrada_ant->modificado = false;
                entrada_ant->indice_frame = -1;
            }
            free(clave_pag_ant);
        }
        free(clave_ant);
    }

    // 4. Pedir página nueva (Swap In)
    int nro_bloque = nro_pagina;
    t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion = PED_PAG;

    agregar_a_paquete(paquete, &query_actual->query_id, sizeof(int));
    agregar_a_paquete(paquete, formato->file_name, strlen(formato->file_name) + 1);
    agregar_a_paquete(paquete, formato->tag, strlen(formato->tag) + 1);
    agregar_a_paquete(paquete, &nro_bloque, sizeof(int));

    enviar_paquete(paquete, conexionStorage);
    eliminar_paquete(paquete);

    op_code op = recibir_operacion(conexionStorage);

    // --- MANEJO DE ERROR DEL STORAGE ---
    if (op != OP_SUCCESS) {
        log_warning(loggerWorker, "Storage devolvió error al pedir bloque %d (Posible Out of Bounds)", nro_bloque);
        t_list* basura = recibir_paquete(conexionStorage);
        list_destroy_and_destroy_elements(basura, free);
        pthread_mutex_unlock(&mutex_memoria);
        return false; // RETORNAMOS FALLO
    }

    t_list* lista = recibir_paquete(conexionStorage);
    char* contenido_bloque = list_get(lista, 0);
    
    // --- RETARDO SIMULADO ---
    usleep(configWorker->retardo_memoria * 1000);

    // 5. Actualizar Memoria
    memcpy(MEMORIA + marco * TAM_PAGINA, contenido_bloque, TAM_PAGINA);

    // --- FIX MEMORY LEAK: Liberamos lo viejo AQUÍ ---
    if (frames[marco].file) free(frames[marco].file);
    if (frames[marco].tag) free(frames[marco].tag);
    // ------------------------------------------------

    // Actualizar Frame
    frames[marco].ocupado = true;
    frames[marco].modificado = false; 
    frames[marco].uso = true;
    frames[marco].page_num = nro_pagina;
    frames[marco].timestamp = obtener_timestamp();
    frames[marco].file = strdup(formato->file_name);
    frames[marco].tag  = strdup(formato->tag);

    // Actualizar Tabla de Páginas
    char* clave_pag = string_itoa(nro_pagina);
    entrada_pag* entrada = dictionary_get(tabla->paginas, clave_pag);

    if (entrada == NULL) {
        entrada = malloc(sizeof(entrada_pag));
        entrada->modificado = false; 
        dictionary_put(tabla->paginas, clave_pag, entrada);
    } 
    
    entrada->presente = true;
    entrada->uso = true;
    entrada->indice_frame = marco;

    free(clave_pag);
    list_destroy_and_destroy_elements(lista, free);

    // --- LOG OBLIGATORIO: ADD / ASIGNACIÓN ---
    log_info(loggerWorker, "Query %d: - Memoria Add - File: %s - Tag: %s - Pagina: %d - Marco: %d",
             query_actual->query_id, formato->file_name, formato->tag, nro_pagina, marco);
             
    log_info(loggerWorker, "Query %d: Se asigna el Marco: %d a la Página: %d perteneciente al - File: %s - Tag: %s",
             query_actual->query_id, marco, nro_pagina, formato->file_name, formato->tag);

    pthread_mutex_unlock(&mutex_memoria);
    return true; // ÉXITO
}