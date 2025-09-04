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
        if(linea_actual < PC_ACTUAL){
            linea_actual++;
            continue;
        }
        log_info(loggerWorker, "## Query %d: FETCH - Program Counter: %d - %s", query_actual->query_id, PC_ACTUAL, linea);

        // RETARDO -> siempre uno por instrucción
        usleep(configWorker->retardo_memoria * 1000);


    // DECODE
        t_instruccion* inst = decode(linea);
        execute(inst);
        destruir_instruccion(inst);

        PC_ACTUAL++;
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

void ejecutar_tag(t_instruccion* inst){ //  WRITE <NOMBRE_FILE>:<TAG> <DIRECCION_BASE> <CONTENIDO>
    char* recurso = inst->parametros[0];            

    t_formato* formato = mapear_formato(recurso);
    char* valor = inst->parametros[1];

    t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion = OP_TAG;
    agregar_a_paquete(paquete, formato->file_name, strlen(formato->file_name) + 1);
    agregar_a_paquete(paquete, formato->tag, strlen(formato->tag) + 1);
    agregar_a_paquete(paquete, valor, strlen(valor) + 1);

    enviar_paquete(paquete, conexionStorage);
    eliminar_paquete(paquete);

    log_info(loggerWorker, "## Query %d: - Instrucción realizada: TAG %s = %s", QUERY_ACTUAL->query_id, recurso, valor);

    destruir_formato(formato);
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

void ejecutar_read(t_instruccion* inst){ // READ <NOMBRE_FILE>:<TAG> <DIRECCION_BASE> <TAMAÑO>
    char* recurso = inst->parametros[0];           // ej: READ MATERIAS:BASE 0 8
    //TODO
}

void ejecutar_write(t_instruccion* inst){ // FLUSH <NOMBRE_FILE>:<TAG> ej: FLUSH <NOMBRE_FILE>:<TAG>
    char* recurso = inst->parametros[0];            //ej: WRITE MATERIAS:V2 0 SISTEMAS_OPERATIVOS_2
    //TODO
}

void ejecutar_flush(t_instruccion* inst){ // FLUSH <NOMBRE_FILE>:<TAG> ej: FLUSH <NOMBRE_FILE>:<TAG>
    char* recurso = inst->parametros[0];
    //TODO
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