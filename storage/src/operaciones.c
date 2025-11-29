#include "operaciones.h"

// Privados
void enviarRespuesta(op_code codigo, int socket_cliente);
void agrandarFileTag(char* nombreFile, char* nombreTag, int nuevoTamanio, int bloquesActuales, t_config* metadata, char** blocks, int bytes, int socket_cliente, int query_id);
void reducirFileTag(char* nombreFile, char* nombreTag, int bloquesActuales, int nuevoTamanio, t_config* metadata, char** blocks, int bytes, int socket_cliente, int query_id);
int contarElementos(char** array);
void actualizarBloques(t_config* metadata, int bloquesActuales, int nuevoTamanio, int bytes, int* bloquesFisicos);
op_code borrarTag(char* nombreFile, char* nombreTag, int query_id);
op_code commitTag(char* nombreFile, char* nombreTag, int query_id);
op_code escribirBloqueLogico(char* nombreFile, char* nombreTag, int numeroBloqueLogico, char* contenido, size_t sizeContenido, int query_id);
void logearResultadoOP(op_code resultado, char* operacion);
op_code opTag(char* fOriginal, char* tOriginal, char* f, char* t, int query_id);

void crearFile(t_list* data, int socket_cliente) {
    int query_id = *((int*) list_get(data, 0));
    char* nombreFile = list_get(data, 1);
    char* nombreTag = list_get(data, 2);
    op_code resultado = crearFileTag(nombreFile, nombreTag);
    
    if (resultado == OP_SUCCESS) {
        log_info(loggerStorage, "##%d - File Creado %s:%s", query_id, nombreFile, nombreTag);
    }
    
    enviarRespuesta(resultado, socket_cliente);
    logearResultadoOP(resultado, "Crear FILE");
}

void truncar(t_list* data, int socket_cliente) {
    int query_id = *((int*) list_get(data, 0));
    char* nombreFile = list_get(data, 1);
    char* nombreTag = list_get(data, 2);
    int bytes = *((int*) list_get(data, 3));
    int nuevoTamanio = pasarABloques(bytes);

    t_config* metadata = getMetaData(nombreFile, nombreTag);

    if (metadata == NULL) {
        enviarRespuesta(ERROR_FILE_NOT_FOUND, socket_cliente);
        logearResultadoOP(ERROR_FILE_NOT_FOUND, "TRUNCAR");
        return;
    }

    if (strcmp("COMMITED", config_get_string_value(metadata, "ESTADO")) == 0) {
        enviarRespuesta(ERROR_WRITE_NOT_ALLOWED, socket_cliente);
        logearResultadoOP(ERROR_WRITE_NOT_ALLOWED, "TRUNCAR");
        config_destroy(metadata);
        return;
    }

    char** blocks = config_get_array_value(metadata, "BLOCKS");
    int bloquesActuales = contarElementos(blocks);

    if(bloquesActuales == nuevoTamanio){
        log_info(loggerStorage, "##%d - File Truncado %s:%s - Tamaño: %d", query_id, nombreFile, nombreTag, bytes);
        enviarRespuesta(OP_SUCCESS, socket_cliente);
        logearResultadoOP(OP_SUCCESS, "TRUNCAR");
    }
    else if(bloquesActuales < nuevoTamanio){
        agrandarFileTag(nombreFile, nombreTag, nuevoTamanio, bloquesActuales, metadata, blocks, bytes, socket_cliente, query_id);
    }
    else{
        reducirFileTag(nombreFile, nombreTag, bloquesActuales, nuevoTamanio, metadata, blocks, bytes, socket_cliente, query_id);   
    }
    string_array_destroy(blocks);
    config_destroy(metadata);
}

void tag(t_list* data, int socket_cliente) {
    if(list_size(data) != 5) {
        log_error(loggerStorage, "Me llegaron mas/menos datos de los que esperaba (5).");
        return;
    }
    int query_id = *((int*) list_get(data, 0));
    char* nombreFileOriginal = list_get(data, 1);
    char* nombreTagOriginal = list_get(data, 2);
    char* nombreFile = list_get(data, 3);
    char* nombreTag = list_get(data, 4);
    op_code resultado = opTag(nombreFileOriginal, nombreTagOriginal, nombreFile, nombreTag, query_id);
    enviarRespuesta(resultado, socket_cliente);

    logearResultadoOP(resultado, "Crear TAG");
}

void eliminarTag(t_list* data, int socket_cliente) {
    int query_id = *((int*) list_get(data, 0));
    char* nombreFile = list_get(data, 1);
    char* nombreTag = list_get(data, 2);
    op_code resultado = borrarTag(nombreFile, nombreTag, query_id);
    enviarRespuesta(resultado, socket_cliente);

    logearResultadoOP(resultado, "Eliminar TAG");
}

void commit(t_list* data, int socket_cliente) {
    int query_id = *((int*) list_get(data, 0));
    char* nombreFile = list_get(data, 1);
    char* nombreTag = list_get(data, 2);
    op_code resultado = commitTag(nombreFile, nombreTag, query_id);
    enviarRespuesta(resultado, socket_cliente);
    logearResultadoOP(resultado, "COMMIT");
}

void writeFileTag(t_list* data, int socket_cliente) {
    int query_id = *((int*) list_get(data, 0));
    char* nombreFile = list_get(data, 1);
    char* nombreTag = list_get(data, 2);
    int nroBloqueLogico = *((int*) list_get(data, 3));

    char* contenido = list_get(data, 4);
    int sizeContenido = *((int*) list_get(data, 5));
    op_code resultado = escribirBloqueLogico(nombreFile, nombreTag, nroBloqueLogico, contenido, sizeContenido, query_id);
    enviarRespuesta(resultado, socket_cliente);

    logearResultadoOP(resultado, "WRITE");
}

void readBloqueLogico(t_list* data, int socket_cliente) {
    int query_id = *((int*) list_get(data, 0));
    char* nombreFile = list_get(data, 1);
    char* nombreTag = list_get(data, 2);
    int nroBloqueLogico = *((int*) list_get(data, 3));

    t_config *metadata = getMetaData(nombreFile, nombreTag);
    if (metadata == NULL) {
        logearResultadoOP(ERROR_FILE_NOT_FOUND, "READ");
        enviarRespuesta(ERROR_FILE_NOT_FOUND, socket_cliente);
        return;
    }
    
    int tamanioBloques = config_get_int_value(metadata, "TAMAÑO")/superblock->blocksize;
    if (nroBloqueLogico < 0 || nroBloqueLogico >= tamanioBloques) {
        logearResultadoOP(ERROR_OUT_OF_BOUNDS, "READ");
        enviarRespuesta(ERROR_OUT_OF_BOUNDS, socket_cliente);
        config_destroy(metadata);
        return;
    }

    // Obtener la lista de bloques físicos directamente desde el metadata que ya tenemos
    char** bloques = config_get_array_value(metadata, "BLOCKS");
    if (bloques == NULL || bloques[nroBloqueLogico] == NULL) {
        logearResultadoOP(ERROR_OUT_OF_BOUNDS, "READ");
        enviarRespuesta(ERROR_OUT_OF_BOUNDS, socket_cliente);
        if (bloques != NULL) {
            string_array_destroy(bloques);
        }
        config_destroy(metadata);
        return;
    }
    
    int nroBloqueFisico = atoi(bloques[nroBloqueLogico]);
    string_array_destroy(bloques);
    config_destroy(metadata);
    
    if (nroBloqueFisico == -1) {
        logearResultadoOP(ERROR_OUT_OF_BOUNDS, "READ");
        enviarRespuesta(ERROR_OUT_OF_BOUNDS, socket_cliente);
        return;
    }

    char* datos = leerBloqueFisico(nroBloqueFisico);
    if (datos == NULL) {
        logearResultadoOP(OP_FAILED, "READ");
        enviarRespuesta(OP_FAILED, socket_cliente);
        return;
    }

    log_info(loggerStorage, "##%d - Bloque Lógico Leído %s:%s - Número de Bloque: %d", query_id, nombreFile, nombreTag, nroBloqueLogico);
    log_debug(loggerStorage, "Datos leídos: %s", datos);
    t_paquete* paqueteRespuesta = crear_paquete();
    paqueteRespuesta->codigo_operacion = OP_SUCCESS;
    agregar_a_paquete(paqueteRespuesta, datos, superblock->blocksize);
    enviar_paquete(paqueteRespuesta, socket_cliente);
    eliminar_paquete(paqueteRespuesta);
    free(datos);
}

// Privados - Implementaciones
void enviarRespuesta(op_code codigo, int socket_cliente) {
    t_paquete* paqueteRespuesta = crear_paquete();
    paqueteRespuesta->codigo_operacion = codigo;
    enviar_paquete(paqueteRespuesta, socket_cliente);
    eliminar_paquete(paqueteRespuesta);
}

void agrandarFileTag(char* nombreFile, char* nombreTag, int nuevoTamanio, int bloquesActuales, t_config* metadata, char** blocks, int bytes, int socket_cliente, int query_id) {
    int i = bloquesActuales;
    bool error = false;

    for (; i < nuevoTamanio; i++) {
        if (!agregarBloqueLogicoHL(nombreFile, nombreTag, i, 0, query_id)) {
            error = true;
            break;
        }
    }

    if (error) {
        for (int j = bloquesActuales; j < i; j++) {
            eliminarBloqueLogicoHL(nombreFile, nombreTag, j, query_id);
        }
        enviarRespuesta(OP_FAILED, socket_cliente);
    } else {
        int* bloquesFisicos = malloc(sizeof(int) * nuevoTamanio);
        if (bloquesFisicos == NULL) {
            log_error(loggerStorage, "Error al asignar memoria para bloquesFisicos");
            enviarRespuesta(OP_FAILED, socket_cliente);
            return;
        }
        for (int k = 0; k < bloquesActuales; k++) {
            bloquesFisicos[k] = atoi(blocks[k]);
        }
        actualizarBloques(metadata, bloquesActuales, nuevoTamanio, bytes, bloquesFisicos);
        free(bloquesFisicos);
        log_info(loggerStorage, "##%d - File Truncado %s:%s - Tamaño: %d", query_id, nombreFile, nombreTag, bytes);
        enviarRespuesta(OP_SUCCESS, socket_cliente);
    }
}

void reducirFileTag(char* nombreFile, char* nombreTag, int bloquesActuales, int nuevoTamanio, t_config* metadata, char** blocks, int bytes, int socket_cliente, int query_id) {
    int i;
    bool error = false;

    for (i = bloquesActuales - 1; i >= nuevoTamanio; i--) {
        if (!eliminarBloqueLogicoHL(nombreFile, nombreTag, i, query_id)) {
            error = true;
            break;
        }
    }

    if (error) {
        // Restaurar bloques eliminados previamente
        // TODO: Restauración?S
        log_error(loggerStorage, "Error al reducir el FileTag %s:%s", nombreFile, nombreTag);
        enviarRespuesta(OP_FAILED, socket_cliente);
    } else {
        int* bloquesFisicos = NULL;
        if (nuevoTamanio > 0) {
            bloquesFisicos = malloc(sizeof(int) * nuevoTamanio);
        }
        for (int k = 0; k < nuevoTamanio; k++) {
            bloquesFisicos[k] = atoi(blocks[k]);
        }
        actualizarBloques(metadata, bloquesActuales, nuevoTamanio, bytes, bloquesFisicos);
        free(bloquesFisicos);
        log_info(loggerStorage, "##%d - File Truncado %s:%s - Tamaño: %d", query_id, nombreFile, nombreTag, bytes);
        enviarRespuesta(OP_SUCCESS, socket_cliente);
    }
}

int contarElementos(char** array) {
    int count = 0;
    while (array[count] != NULL) {
        count++;
    }
    return count;
}

void actualizarBloques(t_config* metadata, int bloquesActuales, int nuevoTamanio, int bytes, int* bloquesFisicos) {
    // Crear un string para almacenar la lista de bloques
    char* bloquesActualizados = string_new();
    string_append(&bloquesActualizados, "[");

    for (int i = 0; i < nuevoTamanio; i++) {
        if (i < bloquesActuales) {
            // Agregar los bloques físicos existentes
            char* bloque = string_itoa(bloquesFisicos[i]);
            string_append(&bloquesActualizados, bloque);
            free(bloque);
        } else {
            // Agregar ceros para los nuevos bloques
            string_append(&bloquesActualizados, "0");
        }

        // Agregar coma si no es el último elemento
        if (i < nuevoTamanio - 1) {
            string_append(&bloquesActualizados, ",");
        }
    }

    string_append(&bloquesActualizados, "]");
    char* bytesStr = string_itoa(bytes);
    char* tamanio = string_itoa(nuevoTamanio);
    config_set_value(metadata, "TAMAÑO", bytesStr);
    config_set_value(metadata, "BLOCKS", bloquesActualizados);
    config_save(metadata);
    free(bloquesActualizados);
    free(tamanio);
    free(bytesStr);
}

op_code borrarTag(char* nombreFile, char* nombreTag, int query_id) {
    t_config* metadata = getMetaData(nombreFile, nombreTag);
    if (metadata == NULL) {
        return ERROR_FILE_NOT_FOUND;
    }

    char** blocks = config_get_array_value(metadata, "BLOCKS");
    int bloquesActuales = contarElementos(blocks);

    // Eliminar todos los bloques lógicos asociados
    for (int i = 0; i < bloquesActuales; i++) {
        eliminarBloqueLogicoHL(nombreFile, nombreTag, i, query_id);
    }

    // Eliminar el FileTag
    char* pathTag = rutaFileTag(nombreFile, nombreTag);
    config_destroy(metadata);
    borrar(pathTag);
    free(pathTag);
    string_array_destroy(blocks);

    log_info(loggerStorage, "##%d - Tag Eliminado %s:%s", query_id, nombreFile, nombreTag);

    return OP_SUCCESS;
}

op_code commitTag(char* nombreFile, char* nombreTag, int query_id) {
    t_config* metadata = getMetaData(nombreFile, nombreTag);
    if (metadata == NULL) {
        return ERROR_FILE_NOT_FOUND;
    }
    
    char** blocks = config_get_array_value(metadata, "BLOCKS");
    int bloquesActuales = contarElementos(blocks);

    for (int i = 0; i < bloquesActuales; i++) {
        op_code resultado = validarBloqueLogico(nombreFile, nombreTag, i, query_id);
        if (resultado != OP_SUCCESS) {
            string_array_destroy(blocks);
            config_destroy(metadata);
            return resultado;
        }
    }
    
    string_array_destroy(blocks);
    config_destroy(metadata);
    
    metadata = getMetaData(nombreFile, nombreTag);
    config_set_value(metadata, "ESTADO", "COMMITED");
    config_save(metadata);
    config_destroy(metadata);

    log_info(loggerStorage, "##%d - Commit de File:Tag %s:%s", query_id, nombreFile, nombreTag);

    return OP_SUCCESS;
}

op_code escribirBloqueLogico(char* nombreFile, char* nombreTag, int numeroBloqueLogico, char* datos, size_t sizeDatos, int query_id) {
    // Validación de datos
    if (nombreFile == NULL || nombreTag == NULL) {
        log_error(loggerStorage, "Error escribiendo bloque lógico, el nombre del file o del tag es NULL");
        return OP_FAILED;
    }
    if (string_is_empty(nombreFile) || string_is_empty(nombreTag)) {
        log_error(loggerStorage, "Error escribiendo bloque lógico, el nombre del file o del tag está vacío");
        return OP_FAILED;
    }
    if (numeroBloqueLogico < 0) {
        log_error(loggerStorage, "Error escribiendo bloque lógico, el número de bloque lógico es negativo");
        return OP_FAILED;
    }
    if (datos == NULL) {
        log_error(loggerStorage, "Error escribiendo bloque lógico, los datos a escribir son NULL");
        return OP_FAILED;
    }
    if (sizeDatos == 0 || sizeDatos > superblock->blocksize) {
        log_error(loggerStorage, "Error escribiendo bloque lógico, el tamaño de los datos es inválido");
        return ERROR_OUT_OF_BOUNDS;
    }

    // Obtener metadata
    t_config* metadata = getMetaData(nombreFile, nombreTag);
    if (metadata == NULL) {
        log_error(loggerStorage, "Error escribiendo bloque lógico, no se encontró el FileTag %s:%s", nombreFile, nombreTag);
        return ERROR_FILE_NOT_FOUND;
    }

    // Verificar estado
    if (strcmp("COMMITED", config_get_string_value(metadata, "ESTADO")) == 0) {
        log_error(loggerStorage, "Error escribiendo bloque lógico, el FileTag %s:%s está COMMITED", nombreFile, nombreTag);
        config_destroy(metadata);
        return ERROR_WRITE_NOT_ALLOWED;
    }

    // Ruta al bloque lógico
    char* pathFolder = rutaFileTag(nombreFile, nombreTag);
    string_append(&pathFolder, "/logical_blocks/");

    char* pathBloqueLogico = string_duplicate(pathFolder);
    char* nombreBloqueLogico = crearNombreBloque(numeroBloqueLogico);
    string_append(&pathBloqueLogico, nombreBloqueLogico);
    free(nombreBloqueLogico);

    int bloqueFisico;
    if(esHardlinkUnico(pathBloqueLogico)) { // Como es HL unico, puedo escribir directamente en el bloque fisico
        log_debug(loggerStorage, "Hardlink único para %s", pathBloqueLogico);
        bloqueFisico = obtenerBloqueFisico(nombreFile, nombreTag, numeroBloqueLogico);
    } else {
        // El bloque tiene mas de un Hardlink -> Le asigno uno nuevo
        log_debug(loggerStorage, "Hardlink múltiple para %s", pathBloqueLogico);
        bloqueFisico = obtenerBloqueLibre(query_id);
        if (bloqueFisico == -1) {
            free(pathBloqueLogico);
            free(pathFolder);
            config_destroy(metadata);
            return ERROR_NO_SPACE;
        }
        if (!eliminarBloqueLogicoHL(nombreFile, nombreTag, numeroBloqueLogico, query_id)) {
            free(pathBloqueLogico);
            free(pathFolder);
            config_destroy(metadata);
            return OP_FAILED;
        }
        if (!crearHardlink(pathFolder, numeroBloqueLogico, bloqueFisico, query_id)) {
            free(pathBloqueLogico);
            free(pathFolder);
            config_destroy(metadata);
            return OP_FAILED;
        }
        if(!actualizarBloqueFileTag(metadata, numeroBloqueLogico, bloqueFisico)) {
            free(pathBloqueLogico);
            free(pathFolder);
            config_destroy(metadata);
            return OP_FAILED;
        }
    }
    if(escribirBloqueFisico(bloqueFisico, datos, sizeDatos, false, query_id)) {
        log_info(loggerStorage, "##%d - Bloque Lógico Escrito %s:%s - Número de Bloque: %d", query_id, nombreFile, nombreTag, numeroBloqueLogico);
        free(pathBloqueLogico);
        free(pathFolder);
        config_destroy(metadata);
        return OP_SUCCESS;
    } else { // TODO: Quizas estaría bueno que devuelva mas info, pero mucho refactor zzz
        free(pathBloqueLogico);
        free(pathFolder);
        config_destroy(metadata);
        return OP_FAILED;
    }
}

void logearResultadoOP(op_code resultado, char* operacion) {
    if(resultado == OP_SUCCESS) {
        log_debug(loggerStorage, "Operacion: %s exitosa!", operacion);
    } else {
        log_warning(loggerStorage, "Operacion: %s finalizada con error: %d", operacion, resultado);
    }
}

op_code opTag(char* fOriginal, char* tOriginal, char* f, char* t, int query_id) {
    // Validar que el tag destino no exista
    t_config* metaDataDestino = getMetaData(f, t);
    if (metaDataDestino != NULL) {
        log_error(loggerStorage, "El tag destino %s:%s ya existe", f, t);
        config_destroy(metaDataDestino);
        return ERROR_FILE_TAG_EXISTS;
    }

    // Crear el tag destino
    op_code rtaCrear = crearTag(f, t);
    if (rtaCrear != OP_SUCCESS) {
        return rtaCrear;
    }

    log_info(loggerStorage, "##%d - Tag creado %s:%s", query_id, f, t);

    // Duplicar el contenido del tag origen al tag destino
    op_code rtaDuplicar = duplicarFileTag(fOriginal, tOriginal, f, t, query_id);
    if (rtaDuplicar != OP_SUCCESS) {
        // Rollback: eliminar el tag destino que se creó
        borrarTag(f, t, query_id);
    }
    return rtaDuplicar;
}