#include "query_interpreter.h"

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
            QUERY_ACTUAL = malloc(sizeof(t_query_context));
            QUERY_ACTUAL->query_id = *query_id;
            QUERY_ACTUAL->nombre_query = strdup(nombre_query);
            QUERY_ACTUAL->pc_inicial = *pc_inicial; 

            PC_ACTUAL = *pc_inicial;

            // Armo el path completo
            char *path_query = string_new();
            string_append(&path_query, configWorker->path_scripts);
            string_append(&path_query, nombre_query);

            log_info(loggerWorker, "## Query %d: Se recibe la Query. El path de operaciones es: %d", QUERY_ACTUAL->query_id, path_query);

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

        log_info(loggerWorker, "## Query %d: FETCH - Program Counter: %d - %s", QUERY_ACTUAL->query_id, PC_ACTUAL, linea);

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
    if (strcmp(nombre, "CREATE") == 0) return OP_CREATE;
    if (strcmp(nombre, "TRUNCATE") == 0) return OP_TRUNCATE;
    if (strcmp(nombre, "WRITE") == 0) return OP_WRITE;
    if (strcmp(nombre, "READ") == 0) return OP_READ;
    if (strcmp(nombre, "FLUSH") == 0) return OP_FLUSH;
    if (strcmp(nombre, "COMMIT") == 0) return OP_COMMIT;
    if (strcmp(nombre, "DELETE") == 0) return OP_DELETE;
    if (strcmp(nombre, "TAG") == 0) return OP_TAG;
    if (strcmp(nombre, "END") == 0) return OP_END;
    return OP_UNKNOWN;
}

//TODO EXECUTE
// ejecuta según el opcode 
void execute(t_instruccion* inst) {
    switch (inst->opcode) {
        case OP_CREATE:
            printf("Ejecutando CREATE sobre %s\n", inst->parametros[0]);
            // mandar paquete a storage
            break;

        case OP_TRUNCATE:
            printf("Ejecutando TRUNCATE %s a tamaño %s\n",
                   inst->parametros[0], inst->parametros[1]);
            break;

        case OP_WRITE:
            printf("Ejecutando WRITE en %s offset %s valor %s\n",
                   inst->parametros[0], inst->parametros[1], inst->parametros[2]);
            break;

        case OP_FLUSH:
            printf("Ejecutando FLUSH sobre %s\n", inst->parametros[0]);
            break;

        case OP_END:
            printf("Ejecutando END\n");
            break;

        default:
            printf("Instrucción desconocida\n");
            break;
    }
}

// Liberar memoria
void destruir_instruccion(t_instruccion* inst) {
    for (int i = 0; i < inst->num_parametros; i++) {
        free(inst->parametros[i]);
    }
    free(inst->parametros);
    free(inst);
}