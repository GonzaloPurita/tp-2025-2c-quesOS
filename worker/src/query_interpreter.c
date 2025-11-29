#include "query_interpreter.h"
#include <utilsWorker/configWorker.h>
#include <commons/log.h> 

t_query_context* query_actual;
pthread_mutex_t mutex_error = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_interrupt = PTHREAD_MUTEX_INITIALIZER;

void recibir_queries() {
    while (1) {
        int cod_op = recibir_operacion(conexionMaster);
        
        if (cod_op <= 0) {
            log_error(loggerWorker, "Master desconectado o error en recibir_operacion (op=%d)", cod_op);
            break;
        }
        log_info(loggerWorker, "Recibida operacion %d del Master", cod_op);

        t_list* paquete = recibir_paquete(conexionMaster);

        if (cod_op == QCB) {

            if (!paquete || list_size(paquete) < 3) {
                log_error(loggerWorker, "Error: Datos insuficientes en QCB.");
                if (paquete) list_destroy_and_destroy_elements(paquete, free);
                continue;
            }

            int* query_id = list_get(paquete, 0);
            int* pc_inicial = list_get(paquete, 1);
            char* nombre_query = list_get(paquete, 2);

            if (query_actual) {
                if (query_actual->nombre_query) free(query_actual->nombre_query);
                free(query_actual);
            }
            
            query_actual = malloc(sizeof(t_query_context));
            query_actual->query_id = *query_id;
            query_actual->pc_inicial = *pc_inicial;
            query_actual->nombre_query = strdup(nombre_query);
            PC_ACTUAL = *pc_inicial;

            t_hilo_args* args = malloc(sizeof(t_hilo_args));
            args->query_id = *query_id;

            args->path_script = string_from_format("%s%s", configWorker->path_scripts, nombre_query);

            pthread_mutex_lock(&mutex_error);
            query_error_flag = false;
            pthread_mutex_unlock(&mutex_error);
            pthread_mutex_lock(&mutex_interrupt);
            interrupt_flag = false;
            pthread_mutex_unlock(&mutex_interrupt);

            pthread_t hilo_cpu;
            if (pthread_create(&hilo_cpu, NULL, (void*)correr_query_en_hilo, (void*)args) != 0) {
                log_error(loggerWorker, "Error al crear el hilo de ejecución");
                free(args->path_script);
                free(args);
            } else {
                pthread_detach(hilo_cpu);
                log_info(loggerWorker, "## Query %d: Hilo de CPU iniciado.", args->query_id);
            }
        }
        else if (cod_op == DESALOJO) {
            pthread_mutex_lock(&mutex_interrupt);
            interrupt_flag = true;
            pthread_mutex_unlock(&mutex_interrupt);
            log_warning(loggerWorker, "Master pidió desalojo (Hilo Principal)");
        }
        else {
            log_error(loggerWorker, "Operación inesperada del Master: %d", cod_op);
        }
        list_destroy_and_destroy_elements(paquete, free);
    }
}

void* correr_query_en_hilo(void* arg) {
    // char *path_query = string_new();
    // string_append(&path_query, configWorker->path_scripts);
    // string_append(&path_query, query_actual->nombre_query);
    t_hilo_args* args = (t_hilo_args*) arg;
    char* path_query = args->path_script;
    int q_id = args->query_id;

    log_info(loggerWorker, "## Query %d: Ejecutando script: %s", q_id, path_query);
    
    t_estado_query estado = ejecutar_query(path_query); 
    log_debug(loggerWorker, "## Query %d: Ejecución finalizada con estado %d", q_id, estado);

    switch (estado) {
        case QUERY_DESALOJADA:
            log_info(loggerWorker, "## Query %d: Desalojada. Guardando contexto...", q_id);
            // TODO: Usar instrucción FLUSH
            guardar_paginas_modificadas();
            notificar_master_desalojo(PC_ACTUAL);
            break;

        case QUERY_EXITO:
            log_info(loggerWorker, "## Query %d: Finalizada exitosamente (EXIT)", q_id);
            // TODO: Hay que notificar al Master que terminó la query?
            break;

        case QUERY_ERROR:
            log_error(loggerWorker, "## Query %d: Finalizada por error", q_id);
            notificar_error_a_master("Error de ejecución");
            break;
    }

    // free(query_actual->nombre_query);
    // free(query_actual);
    // query_actual = NULL;

    free(path_query); 
    free(args);

    pthread_mutex_lock(&mutex_interrupt);
    interrupt_flag = false;
    pthread_mutex_unlock(&mutex_interrupt);
    
    return NULL;
}

t_estado_query ejecutar_query(char* path_query) {
    FILE* file = fopen(path_query, "r");
    if (file == NULL) {
        log_error(loggerWorker, "No se pudo abrir el archivo: %s", path_query);
        //free(path_query);
        return QUERY_ERROR;
    }

    char* linea = NULL;
    size_t len = 0;
    int linea_actual = 0;
    
    t_estado_query estado_salida = QUERY_EXITO; 

    while (getline(&linea, &len, file) != -1) {
        // Eliminar \n manualmente SIN modificar el puntero
        size_t largo = strlen(linea);
        if (largo > 0 && linea[largo - 1] == '\n') {
        linea[largo - 1] = '\0';
        }
        // También eliminar \r si existe (archivos de Windows)
        largo = strlen(linea);
        if (largo > 0 && linea[largo - 1] == '\r') {
        linea[largo - 1] = '\0';
        }

        if (linea_actual >= PC_ACTUAL) {
            pthread_mutex_lock(&mutex_error);
            bool error_flag = query_error_flag;
            pthread_mutex_unlock(&mutex_error);
            if (error_flag) {
                log_warning(loggerWorker, "## Query %d abortada por error externo", query_actual->query_id);
                estado_salida = QUERY_ERROR;
                break;
            }

            log_info(loggerWorker, "## Query %d: FETCH - PC: %d - %s", query_actual->query_id, PC_ACTUAL, linea);

            usleep(configWorker->retardo_memoria * 1000);

            t_instruccion* inst = decode(linea);
            execute(inst);
            destruir_instruccion(inst);
            
            PC_ACTUAL++;

            pthread_mutex_lock(&mutex_interrupt);
            bool interrupt = interrupt_flag;
            pthread_mutex_unlock(&mutex_interrupt);
            if (interrupt) {
                log_warning(loggerWorker, "## Query %d: Interrupción detectada en PC: %d", query_actual->query_id, PC_ACTUAL);
                estado_salida = QUERY_DESALOJADA;
                break;
            }
        }
        linea_actual++;
    }

    free(linea);
    fclose(file);
    //free(path_query);

    return estado_salida;
}

//convierto la instruccion cruda (osea la linea) en un enum
t_instruccion* decode(char* instruccion_raw) {
    char** partes = string_split(instruccion_raw, " "); // separo por espacios ej: ["WRITE","MATERIAS:BASE","0","SISTEMAS_OPERATIVOS",NULL]

    t_instruccion* inst = malloc(sizeof(t_instruccion));
    inst->opcode = obtener_opcode(partes[0]);

    // contar parámetros
    int i = 0;
    while (partes[i+1] != NULL) i++;

    inst->parametros = malloc(sizeof(char*) * (i + 1));
    for (int j = 0; j < i; j++) {
        inst->parametros[j] = strdup(partes[j+1]);
    }
    inst->parametros[i] = NULL;  // null-terminated
    inst->num_parametros = i;

    string_array_destroy(partes);
    return inst;
}

// Mapea string -> opcode
t_opcode obtener_opcode(char* nombre) {
    if (strcmp(nombre, "CREATE") == 0) return CREATE;
    if (strcmp(nombre, "TRUNCATE") == 0) return TRUNCATE;
    if (strcmp(nombre, "WRITE") == 0) return WRITE;
    if (strcmp(nombre, "READ") == 0) return READ;
    if (strcmp(nombre, "FLUSH") == 0) return FLUSH;
    if (strcmp(nombre, "COMMIT") == 0) return COMMIT;
    if (strcmp(nombre, "DELETE") == 0) return DELETE;
    if (strcmp(nombre, "TAG") == 0) return TAG;
    if (strcmp(nombre, "END") == 0) return END;
    return UNKNOWN;
}

// ejecuta según el opcode 
void execute(t_instruccion* inst) {
    switch (inst->opcode) {
        case CREATE:
            ejecutar_create(inst);
            break;
        case TRUNCATE:
            ejecutar_truncate(inst);
            break;
        case WRITE:
            ejecutar_write(inst);
            break;
        case READ:
            ejecutar_read(inst);
            break;
        case TAG:
            ejecutar_tag(inst);
            break;
        case COMMIT:
            ejecutar_commit(inst);
            break;
        case FLUSH:
            ejecutar_flush(inst);
            break;
        case DELETE:
            ejecutar_delete(inst);
            break;
        case END:
            ejecutar_end(inst);
            break;
        default:
            log_info(loggerWorker, "Instrucción desconocida");
            break;
    }
}

//mapea <NOMBRE_FILE>:<TAG> a t_formato
t_formato* mapear_formato(char* recurso){

    // separo en base al ":"
    char** partes = string_split(recurso, ":");
    if (partes == NULL || partes[0] == NULL || partes[1] == NULL) {
        log_error(loggerWorker, "parametros invalidos");
        string_array_destroy(partes);
        return NULL; // formato inválido
    }

    // armo la estructura
    t_formato* formato = malloc(sizeof(t_formato));
    formato->file_name = strdup(partes[0]); // "MATERIAS"
    formato->tag = strdup(partes[1]); // "BASE"

    string_array_destroy(partes);
    return formato;
}

void destruir_formato(t_formato* formato) {
    if (formato == NULL) return;
    free(formato->file_name);
    free(formato->tag);
    free(formato);
}

// --- INSTRUCCIONES --- //

// le pide al storage que cree un archivo con un tag vacio
void ejecutar_create(t_instruccion* inst){ //CREATE <NOMBRE_FILE>:<TAG> ej: CREATE MATERIAS:BASE
    char* recurso = inst->parametros[0]; //MATERIAS:BASE
    //string_trim_right(&recurso);
   
    // Mapear <NOMBRE_FILE>:<TAG> → t_formato
    t_formato* formato = mapear_formato(recurso);

    if (formato == NULL) {
        log_error(loggerWorker, "Error al mapear formato para CREATE");
        return;
    }

    t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion = OP_CREATE;
    agregar_a_paquete(paquete, &query_actual->query_id, sizeof(int));
    agregar_a_paquete(paquete, formato->file_name, strlen(formato->file_name) + 1);
    agregar_a_paquete(paquete, formato->tag, strlen(formato->tag) + 1);
    enviar_paquete(paquete, conexionStorage);
    eliminar_paquete(paquete);

    op_code rta = recibir_operacion(conexionStorage);
    t_list* rtaP = recibir_paquete(conexionStorage);
    manejar_respuesta_storage(rta, "CREATE");

    log_info(loggerWorker, "## Query %d: - Instrucción realizada: CREATE %s:%s", query_actual->query_id, formato->file_name, formato->tag);
    list_destroy_and_destroy_elements(rtaP, free);
    destruir_formato(formato);
}

void ejecutar_truncate(t_instruccion* inst){ // TRUNCATE <NOMBRE_FILE>:<TAG> <TAMAÑO> ej TRUNCATE MATERIAS:BASE 1024
    char* recurso = inst->parametros[0];
    char* tamanio_str = inst->parametros[1]; // "1024"
    log_debug(loggerWorker, "Ejecutando TRUNCATE %s %s", recurso, tamanio_str);
    int tamanio = atoi(tamanio_str);
    
    // Mapear <NOMBRE_FILE>:<TAG> → t_formato
    t_formato* formato = mapear_formato(recurso);

    t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion = OP_TRUNCATE;
    agregar_a_paquete(paquete, &query_actual->query_id, sizeof(int));
    agregar_a_paquete(paquete, formato->file_name, strlen(formato->file_name) + 1);
    agregar_a_paquete(paquete, formato->tag, strlen(formato->tag) + 1);
    agregar_a_paquete(paquete, &tamanio, sizeof(int));
    enviar_paquete(paquete, conexionStorage);
    eliminar_paquete(paquete);

    op_code rta1 = recibir_operacion(conexionStorage);
    t_list* rtaP = recibir_paquete(conexionStorage);
    log_debug(loggerWorker, "Respuesta recibida de Storage para TRUNCATE 1: %d", rta1);

    manejar_respuesta_storage(rta1, "TRUNCATE");

    log_info(loggerWorker, "## Query %d: - Instrucción realizada: TRUNCATE %s", query_actual->query_id, recurso);

    list_destroy_and_destroy_elements(rtaP, free);
    destruir_formato(formato);
}

void ejecutar_tag(t_instruccion* inst){ //  TAG <FILE_ORIGEN>:<TAG_ORIGEN> <FILE_DEST>:<TAG_DESTINO>
    char* recurso_origen = inst->parametros[0];            
    char* recurso_destino = inst->parametros[1];

    t_formato* formato_origen = mapear_formato(recurso_origen);
    t_formato* formato_destino = mapear_formato(recurso_destino);

    t_paquete* paquete = crear_paquete(); 
    paquete->codigo_operacion = OP_TAG;
    agregar_a_paquete(paquete, &query_actual->query_id, sizeof(int));
    agregar_a_paquete(paquete, formato_origen->file_name, strlen(formato_origen->file_name) + 1);
    agregar_a_paquete(paquete, formato_origen->tag, strlen(formato_origen->tag) + 1);
    agregar_a_paquete(paquete, formato_destino->file_name, strlen(formato_destino->file_name) + 1);
    agregar_a_paquete(paquete, formato_destino->tag, strlen(formato_destino->tag) + 1);

    enviar_paquete(paquete, conexionStorage);
    eliminar_paquete(paquete);

    op_code rta = recibir_operacion(conexionStorage);
    t_list* rtaP = recibir_paquete(conexionStorage);
    manejar_respuesta_storage(rta, "TAG");

    log_info(loggerWorker, "## Query %d: - Instrucción realizada: TAG %s = %s", query_actual->query_id, recurso_origen, recurso_destino);

    list_destroy_and_destroy_elements(rtaP, free);
    destruir_formato(formato_origen);
    destruir_formato(formato_destino);
}

void ejecutar_delete(t_instruccion* inst){ // DELETE <NOMBRE_FILE>:<TAG>
    char* recurso = inst->parametros[0];
    t_formato* formato = mapear_formato(recurso);

    t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion = OP_DELETE;
    agregar_a_paquete(paquete, &query_actual->query_id, sizeof(int));
    agregar_a_paquete(paquete, formato->file_name, strlen(formato->file_name) + 1);
    agregar_a_paquete(paquete, formato->tag, strlen(formato->tag) + 1);

    enviar_paquete(paquete, conexionStorage);
    eliminar_paquete(paquete);

    op_code rta = recibir_operacion(conexionStorage);
    t_list* rtaP = recibir_paquete(conexionStorage);
    manejar_respuesta_storage(rta, "DELETE");


    log_info(loggerWorker, "## Query %d: - Instrucción realizada: DELETE %s", query_actual->query_id, recurso);

    list_destroy_and_destroy_elements(rtaP, free);
    destruir_formato(formato);
}

void ejecutar_commit(t_instruccion* inst){ // COMMIT <NOMBRE_FILE>:<TAG> ej: COMMIT MATERIAS:BASE
    char* recurso = inst->parametros[0];
    t_formato* formato = mapear_formato(recurso);

    // antes del COMMIT, flush de las páginas modificadas !!
    char* clave_tabla = string_from_format("%s:%s", formato->file_name, formato->tag);
    tabla_pag* tabla = dictionary_get(diccionario_tablas, clave_tabla);

    if (tabla != NULL) {
        flush_paginas_modificadas_de_tabla(tabla, formato);
    } else {
        log_warning(loggerWorker, "No existe tabla de páginas para %s:%s — no hay nada que commitear", formato->file_name, formato->tag);
    }

    free(clave_tabla);

    //notifico al storage que se realizo el commit 
    t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion = OP_COMMIT;
    agregar_a_paquete(paquete, &query_actual->query_id, sizeof(int));
    agregar_a_paquete(paquete, formato->file_name, strlen(formato->file_name) + 1);
    agregar_a_paquete(paquete, formato->tag, strlen(formato->tag) + 1);

    enviar_paquete(paquete, conexionStorage);
    eliminar_paquete(paquete);

    op_code rta = recibir_operacion(conexionStorage);
    t_list* rtaP = recibir_paquete(conexionStorage);
    
    manejar_respuesta_storage(rta, "COMMIT");

    log_info(loggerWorker, "## Query %d: - Instrucción realizada: COMMIT %s", query_actual->query_id, recurso);

    list_destroy_and_destroy_elements(rtaP, free);
    destruir_formato(formato);
}

void ejecutar_end(t_instruccion* inst){ 

    log_info(loggerWorker, "## Query %d: Instrucción realizada: END", query_actual->query_id);

    // aviso al master que la Query terminó
    t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion = OP_END;
    agregar_a_paquete(paquete, &query_actual->query_id, sizeof(int));
    enviar_paquete(paquete, conexionMaster);
    eliminar_paquete(paquete);

    // op_code rta = recibir_operacion(conexionStorage);
    // manejar_respuesta_storage(rta, "END");

    // libero contexto
    //destruir_query_context(query_actual);
}


void ejecutar_read(t_instruccion* inst){ // READ <NOMBRE_FILE>:<TAG> <DIRECCION_BASE> <TAMAÑO>  ej: READ MATERIAS:BASE 0 8
    char* recurso = inst->parametros[0];
    int direccion_base = atoi(inst->parametros[1]);
    int tamanio = atoi(inst->parametros[2]);

    t_formato* formato = mapear_formato(recurso);

    //Consigna: La instrucción READ leerá de la Memoria Interna los bytes correspondientes a partir de la dirección 
    //base del File y Tag pasados por parámetro, y deberá enviar dicha información al módulo Master. 
    
    t_list* paginas = paginas_necesarias(direccion_base, tamanio);

    for (int i = 0; i < list_size(paginas); i++) { // para cada página, verifica si está en memoria; si no, pedirla
        int* nro_pagina = list_get(paginas, i);
        bool en_memoria = esta_en_memoria(formato, *nro_pagina);

        if (!en_memoria) {
            pedir_pagina_a_storage(formato, *nro_pagina);
        }
    }

    char* contenido = leer_desde_memoria(formato, direccion_base, tamanio);
    log_info(loggerWorker, "Contenido leído de memoria para READ %s dir_base=%d tam=%d: %s", recurso, direccion_base, tamanio, contenido);

    // enviar el resultado al Master
    t_paquete* respuesta = crear_paquete();
    respuesta->codigo_operacion = OP_READ;
    // agregar_a_paquete(respuesta, contenido, strlen(contenido) + 1);

    agregar_a_paquete(respuesta, &query_actual->query_id, sizeof(int));
    agregar_a_paquete(respuesta, &tamanio, sizeof(int));
    agregar_a_paquete(respuesta, contenido, tamanio + 1);
    agregar_a_paquete(respuesta, formato->tag, strlen(formato->tag) + 1);
    agregar_a_paquete(respuesta, formato->file_name, strlen(formato->file_name) + 1);

    enviar_paquete(respuesta, conexionMaster);
    eliminar_paquete(respuesta);

    log_info(loggerWorker, "## Query %d: - Instrucción realizada: READ %s dir_base=%d tam=%d", query_actual->query_id, recurso, direccion_base, tamanio);
    
    free(contenido);
    list_destroy_and_destroy_elements(paginas, free);
    destruir_formato(formato);
}

void ejecutar_write(t_instruccion* inst){   //ej: WRITE MATERIAS:V2 0 SISTEMAS_OPERATIVOS_2
    char* recurso = inst->parametros[0];       
    
    t_formato* formato = mapear_formato(recurso);
    int direccion_base = atoi(inst->parametros[1]);
    char* valor  = inst->parametros[2];
    int tamanio_valor = strlen(valor); //lo que ocupa el string

    // calculo las paginas necesarias
    t_list* paginas = paginas_necesarias(direccion_base, tamanio_valor);

    log_debug(loggerWorker, "WRITE necesita %d páginas para %s:%s desde base %d tamaño %d", list_size(paginas), formato->file_name, formato->tag, direccion_base, tamanio_valor);

    // para cada página, verificamos si esta en memoria
    for (int i = 0; i < list_size(paginas); i++) { 
        int* nro_pagina = list_get(paginas, i);
        bool en_memoria = esta_en_memoria(formato, *nro_pagina);

        if (!en_memoria) {
            pedir_pagina_a_storage(formato, *nro_pagina);
        }
    }

    escribir_en_memoria(formato, direccion_base, valor);

    log_debug(loggerWorker, "Página marcada como dirty para %s:%s en base %d", formato->file_name, formato->tag, direccion_base);

    // informo al Master que se hizo el WRITE
    t_paquete* respuesta = crear_paquete();
    respuesta->codigo_operacion = OP_WRITE;
    agregar_a_paquete(respuesta, valor, strlen(valor) + 1);
    enviar_paquete(respuesta, conexionMaster);
    eliminar_paquete(respuesta);

    log_info(loggerWorker, "## Query %d: - Instrucción realizada: WRITE %s:%s base=%d valor=%s", query_actual->query_id, formato->file_name, formato->tag, direccion_base, valor);

    list_destroy_and_destroy_elements(paginas, free);
    destruir_formato(formato);
}

void ejecutar_flush(t_instruccion* inst){ // FLUSH <NOMBRE_FILE>:<TAG> ej: FLUSH <NOMBRE_FILE>:<TAG>
    char* recurso = inst->parametros[0];
    t_formato* formato = mapear_formato(recurso);

    //busco en la tabla de paginas del file:tag
    char* clave_tabla = string_from_format("%s:%s", formato->file_name, formato->tag);
    tabla_pag* tabla = dictionary_get(diccionario_tablas, clave_tabla);

    if (tabla == NULL) {
        log_warning(loggerWorker, "No existe tabla de páginas para %s:%s — no hay nada que flushear", formato->file_name, formato->tag);
        free(clave_tabla);
        destruir_formato(formato);
        return;
    }else {
        flush_paginas_modificadas_de_tabla(tabla, formato);
        log_info(loggerWorker, "## Query %d: - Instrucción realizada: FLUSH %s:%s", query_actual->query_id, formato->file_name, formato->tag);
    }
    
    free(clave_tabla);
    destruir_formato(formato);
}

// envia todas las pags modificadas de la tabla 'tabla' para el file:tag 'formato'.
void flush_paginas_modificadas_de_tabla(tabla_pag* tabla, t_formato* formato) {

    if (!tabla || !formato) return;

    t_list* keys = dictionary_keys(tabla->paginas);

    for (int i = 0; i < list_size(keys); i++) {

        char* key = list_get(keys, i);
        entrada_pag* entrada = dictionary_get(tabla->paginas, key);

        if (!entrada) continue;
        if (!entrada->presente || !entrada->modificado) continue;

        int nro_pagina = atoi(key);
        int frame_index = entrada->indice_frame;

        // BLOQUE COMPLETO A ENVIAR
        char* contenido_bloque = malloc(TAM_PAGINA);
        memcpy(contenido_bloque, MEMORIA + frame_index * TAM_PAGINA, TAM_PAGINA);

        t_paquete* paquete = crear_paquete();
        paquete->codigo_operacion = OP_WRITE;

        agregar_a_paquete(paquete, &query_actual->query_id, sizeof(int));
        agregar_a_paquete(paquete, formato->file_name, strlen(formato->file_name) + 1);
        agregar_a_paquete(paquete, formato->tag, strlen(formato->tag) + 1);

        // nro_pag == nro_bloque
        agregar_a_paquete(paquete, &nro_pagina, sizeof(int));

        agregar_a_paquete(paquete, contenido_bloque, TAM_PAGINA);
        agregar_a_paquete(paquete, &TAM_PAGINA, sizeof(int));

        enviar_paquete(paquete, conexionStorage);
        eliminar_paquete(paquete);
        free(contenido_bloque);

        entrada->modificado = false;
        frames[frame_index].modificado = false;

        op_code rta = recibir_operacion(conexionStorage);
        t_list* rtaList = recibir_paquete(conexionStorage);
        if (rtaList) list_destroy_and_destroy_elements(rtaList, free);

        if (rta == OP_SUCCESS)
            log_debug(loggerWorker,"Flush OK página/bloque %d", nro_pagina);
        else
            log_error(loggerWorker,"Error flush en página/bloque %d", nro_pagina);
    }

    list_destroy(keys);
}

void guardar_paginas_modificadas() {
    t_list* paginas = list_create();

    // Obtengo las páginas modificadas
    pthread_mutex_lock(&mutex_memoria);
    for (int i = 0; i < CANTIDAD_MARCOS; i++) {
        frame* f = &frames[i];
        if (f->ocupado && f->modificado) {
            t_pagina_a_guardar* p = malloc(sizeof(t_pagina_a_guardar));
            p->index_frame = i;
            p->file = strdup(f->file);
            p->tag = strdup(f->tag);
            p->page_num = f->page_num;

            int base_memoria = i * TAM_PAGINA;
            p->contenido = malloc(TAM_PAGINA);
            memcpy(p->contenido, MEMORIA + base_memoria, TAM_PAGINA);

            list_add(paginas, p);
        }
    }
    pthread_mutex_unlock(&mutex_memoria);

    // Enviar a Storage
    for (int i = 0; i < list_size(paginas); i++) {
        t_pagina_a_guardar* p = list_get(paginas, i);

        t_paquete* paquete = crear_paquete();
        agregar_a_paquete(paquete, p->file, strlen(p->file) + 1);
        agregar_a_paquete(paquete, p->tag, strlen(p->tag) + 1);
        agregar_a_paquete(paquete, &p->page_num, sizeof(int));
        agregar_a_paquete(paquete, p->contenido, TAM_PAGINA);

        enviar_paquete(paquete, conexionStorage);
        eliminar_paquete(paquete);

        pthread_mutex_lock(&mutex_memoria);
        frames[p->index_frame].modificado = false;
        pthread_mutex_unlock(&mutex_memoria);

        free(p->file);
        free(p->tag);
        free(p->contenido);
        free(p);
    }

    list_destroy(paginas);
    log_debug(loggerWorker, "Finalizado guardado de páginas modificadas.");
}

void manejar_respuesta_storage(op_code respuesta, char* operacion) {
    char* motivo_error = NULL;

    switch (respuesta) {
        case OP_SUCCESS:
            log_debug(loggerWorker, "[Storage] %s -> SUCCESS", operacion);
            return;
        case ERROR_FILE_NOT_FOUND:
            motivo_error = "File no encontrado";
            break;
        case ERROR_TAG_NOT_FOUND:
            motivo_error = "Tag no encontrado";
            break;
        case ERROR_NO_SPACE:
            motivo_error = "Espacio insuficiente";
            break;
        case ERROR_WRITE_NOT_ALLOWED:
            motivo_error = "Escritura no permitida";
            break;
        case ERROR_OUT_OF_BOUNDS:
            motivo_error = "Lectura fuera de límites";
            break;
        default:
            motivo_error = "Error desconocido";
            break;
    }

    //le aviso al master que la query fallo
    if (query_actual) notificar_error_a_master((char*)motivo_error);

    // marco error global para frenar ejecución
    pthread_mutex_lock(&mutex_error);
    query_error_flag = true;
    pthread_mutex_unlock(&mutex_error);

    // liberar contexto y marcar error
    // if (query_actual) {
    //     destruir_query_context(query_actual);
    //     query_actual = NULL;
    // }
}

void notificar_error_a_master(char* motivo) {
    t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion = OP_ERROR;

    // envio id query
    agregar_a_paquete(paquete, &query_actual->query_id, sizeof(int));
    // el motivo seria como un string
    agregar_a_paquete(paquete, motivo, strlen(motivo) + 1);

    enviar_paquete(paquete, conexionMaster);
    eliminar_paquete(paquete);

    log_error(loggerWorker, "## Query %d: finaliza con error -> %s", query_actual->query_id, motivo);
}

// --- Para liberar memoria --- //

void destruir_instruccion(t_instruccion* inst) {
    if (!inst) return;
    if (inst->parametros) {
        for (int i = 0; i < inst->num_parametros; i++) {
            if (inst->parametros[i]) free(inst->parametros[i]);
        }
        free(inst->parametros);
    }
    free(inst);
}

void destruir_query_context(t_query_context* ctx) {
    if (ctx == NULL) return;

    free(ctx->nombre_query);  // strdup en la creación
    free(ctx);
}

char* generar_clave(t_formato* f) {
    return string_from_format("%s:%s", f->file_name, f->tag);
}