#include "query_interpreter.h"
t_query_context* query_actual;

void recibir_queries(){

    while(1){
        int cod_op = recibir_operacion(conexionMaster);
        
        if(cod_op == NUEVA_QUERY){
                
            t_list* paquete = recibir_paquete(conexionMaster);

            if(list_size(paquete) < 3) { // Son tres datos necesarios: tamaño, PID y la ruta al archivo
                log_error(loggerMemoria, "Error: No se recibieron suficientes datos para la query.");
                list_destroy(paquete);
                return;
            }

            // El Master envía: [query_id, nombre_query, pc_inicial]
            int* query_id = list_get(paquete, 0);
            char* nombre_query = list_get(paquete, 1);
            int* pc_inicial = list_get(paquete, 2);

            // Crear contexto global
            query_actual = malloc(sizeof(t_query_context));
            query_actual->query_id = *query_id;
            query_actual->nombre_query = strdup(nombre_query);
            query_actual->pc_inicial = *pc_inicial; 
            PC_ACTUAL = *pc_inicial;

            // Armo el path completo
            char *path_query = string_new();
            string_append(&path_query, configWorker->path_scripts);
            string_append(&path_query, nombre_query);

            log_info(loggerWorker, "## Query %d: Se recibe la Query. El path de operaciones es: %d", query_actual->query_id, path_query);

            // ESTO SERIA COMO UN CICLO DE INSTRUCCION
            ejercutar_query(path_query);

            list_destroy_and_destroy_elements(paquete, free);
        }
    }
}

void ejercutar_query(char* path_query){
    FILE* file = fopen(path_query, "r");
    if(file == NULL){
        log_error(loggerWorker, "No se pudo abrir el archivo: %d", path_query);
        free(path_query);
        return NULL;
    }

    char* linea = NULL;
    size_t len = 0;
    int linea_actual = 0;

    // FETCH
    while(getline(&linea, &len, file) != -1){
        // Solo ejecutamos si ya llegamos al PC_ACTUAL
        if (linea_actual >= PC_ACTUAL) {
            log_info(loggerWorker, "## Query %d: FETCH - Program Counter: %d - %s", query_actual->query_id, PC_ACTUAL, linea);

            // Simula retardo de memoria
            usleep(configWorker->retardo_memoria * 1000);

            t_instruccion* inst = decode(linea);
            execute(inst);
            destruir_instruccion(inst);

            PC_ACTUAL++;
        }
        linea_actual++;
    }

    free(linea);
    fclose(file);
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
            ejecutar_delete(nst);
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
   
    // Mapear <NOMBRE_FILE>:<TAG> → t_formato
    t_formato* formato = mapear_formato(recurso);

    t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion = OP_CREATE;
    agregar_a_paquete(paquete, formato->file_name, strlen(formato->file_name) + 1);
    agregar_a_paquete(paquete, formato->tag, strlen(formato->tag) + 1);
    enviar_paquete(paquete, conexionStorage);
    eliminar_paquete(paquete);

    log_info(loggerWorker,  "## Query %d: - Instrucción realizada: CREATE %s", query_actual->query_id, recurso);

    destruir_formato(formato);
}

void ejecutar_truncate(t_instruccion* inst){ // TRUNCATE <NOMBRE_FILE>:<TAG> <TAMAÑO> ej TRUNCATE MATERIAS:BASE 1024
    char* recurso = inst->parametros[0];
    char* tamanio = inst->parametros[1]; // "1024"
    
    // Mapear <NOMBRE_FILE>:<TAG> → t_formato
    t_formato* formato = mapear_formato(recurso);

    t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion = OP_TRUNCATE;
    agregar_a_paquete(paquete, formato->file_name, strlen(formato->file_name) + 1);
    agregar_a_paquete(paquete, formato->tag, strlen(formato->tag) + 1);
    agregar_a_paquete(paquete, tamanio, strlen(tamanio) + 1);
    enviar_paquete(paquete, conexionStorage);
    eliminar_paquete(paquete);

    log_info(loggerWorker,  "## Query %d: - Instrucción realizada: CREATE %d %s", query_actual->query_id, recurso);

    destruir_formato(formato);
}

void ejecutar_tag(t_instruccion* inst){ //  TAG <FILE_ORIGEN>:<TAG_ORIGEN> <FILE_DEST>:<TAG_DESTINO>
    char* recurso_origen = inst->parametros[0];            
    char* recurso_destino = inst->parametros[1];

    t_formato* formato_origen = mapear_formato(recurso_origen);
    t_formato* formato_destino = mapear_formato(recurso_destino);

    t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion = OP_TAG;
    agregar_a_paquete(paquete, formato_origen->file_name, strlen(formato_origen->file_name) + 1);
    agregar_a_paquete(paquete, formato_origen->tag, strlen(formato_origen->tag) + 1);
    agregar_a_paquete(paquete, formato_destino->file_name, strlen(formato_destino->file_name) + 1);
    agregar_a_paquete(paquete, formato_destino->tag, strlen(formato_destino->tag) + 1);

    enviar_paquete(paquete, conexionStorage);
    eliminar_paquete(paquete);

    log_info(loggerWorker, "## Query %d: - Instrucción realizada: TAG %s = %s", QUERY_ACTUAL->query_id, recurso, valor);

    destruir_formato(formato_origen);
    destruir_formato(formato_destino);
}

void ejecutar_delete(t_instruccion* inst){ // DELETE <NOMBRE_FILE>:<TAG>
    char* recurso = inst->parametros[0];
    t_formato* formato = mapear_formato(recurso);

    t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion = OP_DELETE;
    agregar_a_paquete(paquete, formato->file_name, strlen(formato->file_name) + 1);
    agregar_a_paquete(paquete, formato->tag, strlen(formato->tag) + 1);

    enviar_paquete(paquete, conexionStorage);
    eliminar_paquete(paquete);

    log_info(loggerWorker, "## Query %d: - Instrucción realizada: DELETE %s", query_actual->query_id, recurso);

    destruir_formato(formato);
}

void ejecutar_commit(t_instruccion* inst){ // COMMIT <NOMBRE_FILE>:<TAG> ej: COMMIT MATERIAS:BASE
    char* recurso = inst->parametros[0];

    t_formato* formato = mapear_formato(recurso);

    t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion = OP_COMMIT;
    agregar_a_paquete(paquete, formato->file_name, strlen(formato->file_name) + 1);
    agregar_a_paquete(paquete, formato->tag, strlen(formato->tag) + 1);

    enviar_paquete(paquete, conexionStorage);
    eliminar_paquete(paquete);

    log_info(loggerWorker, "## Query %d: - Instrucción realizada: COMMIT %s", query_actual->query_id, recurso);

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

    // libero contexto
    destruir_query_context(query_actual);
}


void ejecutar_read(t_instruccion* inst){ // READ <NOMBRE_FILE>:<TAG> <DIRECCION_BASE> <TAMAÑO>  ej: READ MATERIAS:BASE 0 8
    char* recurso = inst->parametros[0];           
    int direccion_base = atoi(inst->parametros[1]);
    int tamanio = atoi(inst->parametros[2]);

    t_formato* formato = mapear_formato(recurso);

    //Consigna: La instrucción READ leerá de la Memoria Interna los bytes correspondientes a partir de la dirección 
    //base del File y Tag pasados por parámetro, y deberá enviar dicha información al módulo Master. 
    
    t_list* paginas = paginas_necesarias(direccion_base, tamanio); //TODO calcular las paginas necesarias para leer el contenido

    for (int i = 0; i < list_size(paginas); i++) { // para cada página, verifica si está en memoria; si no, pedirla
        int* nro_pagina = list_get(paginas, i);
        bool en_memoria = esta_en_memoria(nro_pagina); //TODO consultar si la pagina esta en la memoria

        if (!en_memoria) {
            pedir_pagina_a_storage(formato, *nro_pagina); //TODO: abajo plantee la funcion pero hay que revisarla
        }
    }

    char* contenido = leer_desde_memoria(formato, direccion_base, tamanio); //TODO

    // enviar el resultado al Master
    t_paquete* respuesta = crear_paquete();
    respuesta->codigo_operacion = OP_READ;
    agregar_a_paquete(respuesta, contenido, strlen(contenido) + 1);

    enviar_paquete(respuesta, conexionMaster);
    eliminar_paquete(respuesta);

    log_info(loggerWorker, "## Query %d: - Instrucción realizada: READ %s dir_base=%d tam=%d", query_actual->query_id, recurso, direccion_base, tamanio);
    
    free(contenido);
    list_destroy_and_destroy_elements(paginas, free);
    destruir_formato(formato);
}

void ejecutar_write(t_instruccion* inst){   //ej: WRITE MATERIAS:V2 0 SISTEMAS_OPERATIVOS_2
    char* recurso = inst->parametros[0];           
    
    t_formato* formato = mapear_formato(inst);
    int direccion_base = atoi(inst->parametros[1]);
    char* valor  = inst->parametros[2];
    int tamanio_valor = strlen(valor); //lo que ocupa el string

    // calculo las paginas necesarias
    t_list* paginas = paginas_necesarias(direccion_base, tamanio);

    // para cada página, verifica si está en memoria; si no, pedirla
    for (int i = 0; i < list_size(paginas); i++) { 
        int* nro_pagina = list_get(paginas, i);
        bool en_memoria = esta_en_memoria(nro_pagina); //TODO consultar si la pagina esta en la memoria

        if (!en_memoria) {
            pedir_pagina_a_storage(formato, *nro_pagina); //TODO: abajo plantee la funcion pero hay que revisarla
        }
    }

    // escribir valor en memoria
    escribir_en_memoria(formato, direccion_base, valor);

    // marcar página como dirty 
    // TODO: implementar en tabla de páginas real
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
    //TODO
}

void pedir_pagina_a_storage(t_formato* formato, int nro_pagina){
    int base_pagina = nro_pagina * TAM_PAGINA;

    t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion = PED_PAG;
    agregar_a_paquete(paquete, formato->file_name, strlen(formato->file_name) + 1);
    agregar_a_paquete(paquete, formato->tag, strlen(formato->tag) + 1);
    agregar_a_paquete(paquete, &base_pagina, sizeof(int));
    
    enviar_paquete(paquete, conexionStorage);
    eliminar_paquete(paquete);

    // recibo la pagina

    op_code cod_op = recibir_operacion(conexionStorage);
    if (cod_op == PED_PAG) {
        t_list* lista = recibir_paquete(conexionStorage);
        char* contenido = list_get(lista, 0);

        escribir_en_memoria(formato, base_pagina, contenido); // TODO: guardar 'contenido' en MEMORIA en el marco asignado a la página nro_pagina, void escribir_en_memoria(t_formato* formato, int direccion_base, char* valor)
        
        log_debug(loggerWorker, "Contenido de página %d recibido desde Storage: %s", nro_pagina, contenido);

        list_destroy_and_destroy_elements(lista, free);
    }
}

// --- Para liberar memoria --- //


void destruir_instruccion(t_instruccion* inst) {
    for (int i = 0; i < inst->num_parametros; i++) {
        free(inst->parametros[i]);
    }
    free(inst->parametros);
    free(inst);
}

void destruir_query_context(t_query_context* ctx) {
    if (ctx == NULL) return;

    free(ctx->nombre_query);  // strdup en la creación
    free(ctx);
}