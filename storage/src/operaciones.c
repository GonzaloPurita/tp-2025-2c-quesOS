#include "operaciones.h"

// Privados
void enviarRespuesta(op_code codigo, int socket_cliente);
void agrandarFileTag(char* nombreFile, char* nombreTag, int nuevoTamanio, int bloquesActuales, t_config* metadata, char** blocks, int socket_cliente);
void reducirFileTag(char* nombreFile, char* nombreTag, int bloquesActuales, int nuevoTamanio, t_config* metadata, char** blocks, int socket_cliente);
int contarElementos(char** array);
void actualizarBloques(t_config* metadata, int bloquesActuales, int nuevoTamanio, int* bloquesFisicos);
op_code borrarTag(char* nombreFile, char* nombreTag);
op_code commitTag(char* nombreFile, char* nombreTag);
op_code escribirBloqueLogico(char* nombreFile, char* nombreTag, int numeroBloqueLogico, void* contenido, size_t sizeContenido);
void logearResultadoOP(op_code resultado, char* operacion);

void crearFile(t_list* data, int socket_cliente) {
    char* nombreFile = list_get(data, 0);
    char* nombreTag = list_get(data, 1);
    op_code resultado = crearFileTag(nombreFile, nombreTag);
    enviarRespuesta(resultado, socket_cliente);
    logearResultadoOP(resultado, "Crear FILE");
}

void truncar(t_list* data, int socket_cliente) {
    char* nombreFile = list_get(data, 0);
    char* nombreTag = list_get(data, 1);
    int bytes = *((int*) list_get(data, 2));
    int nuevoTamanio = pasarABloques(bytes);
    log_debug(loggerStorage, "Truncar file tag %s:%s a tamaño %d", nombreFile, nombreTag, nuevoTamanio);

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
        enviarRespuesta(OP_SUCCESS, socket_cliente);
        logearResultadoOP(OP_SUCCESS, "TRUNCAR");
    }
    else if(bloquesActuales < nuevoTamanio){
        agrandarFileTag(nombreFile, nombreTag, nuevoTamanio, bloquesActuales, metadata, blocks, socket_cliente);
    }
    else{
        reducirFileTag(nombreFile, nombreTag, bloquesActuales, nuevoTamanio, metadata, blocks, socket_cliente);   
    }
    string_array_destroy(blocks);
    config_destroy(metadata);
}

void tag(t_list* data, int socket_cliente) { // TODO: MAL HECHO, VOLVER A HACER :-(
    char* nombreFile = list_get(data, 0);
    char* nombreTag = list_get(data, 1);
    op_code resultado = crearTag(nombreFile, nombreTag);
    enviarRespuesta(resultado, socket_cliente);

    logearResultadoOP(resultado, "Crear TAG");
}

void eliminarTag(t_list* data, int socket_cliente) {
    char* nombreFile = list_get(data, 0);
    char* nombreTag = list_get(data, 1);
    op_code resultado = borrarTag(nombreFile, nombreTag);
    enviarRespuesta(resultado, socket_cliente);

    logearResultadoOP(resultado, "Eliminar TAG");
}

void commit(t_list* data, int socket_cliente) {
    char* nombreFile = list_get(data, 0);
    char* nombreTag = list_get(data, 1);
    op_code resultado = commitTag(nombreFile, nombreTag);
    enviarRespuesta(resultado, socket_cliente);
    logearResultadoOP(resultado, "COMMIT");
}

void writeFileTag(t_list* data, int socket_cliente) {
    char* nombreFile = list_get(data, 0);
    char* nombreTag = list_get(data, 1);
    int nroBloqueLogico = *((int*) list_get(data, 2));
    void* contenido = list_get(data, 3);
    size_t sizeContenido = *((size_t*) list_get(data, 4));
    op_code resultado = escribirBloqueLogico(nombreFile, nombreTag, nroBloqueLogico, contenido, sizeContenido);
    enviarRespuesta(resultado, socket_cliente);

    logearResultadoOP(resultado, "WRITE");
}

void readBloqueLogico(t_list* data, int socket_cliente) {
    char* nombreFile = list_get(data, 0);
    char* nombreTag = list_get(data, 1);
    int nroBloqueLogico = *((int*) list_get(data, 2));

    t_config *metadata = getMetaData(nombreFile, nombreTag);
    if (metadata == NULL) {
        enviarRespuesta(ERROR_FILE_NOT_FOUND, socket_cliente);
        return;
    }

    int nroBloqueFisico = obtenerBloqueFisico(nombreFile, nombreTag, nroBloqueLogico);
    if (nroBloqueFisico == -1) {
        enviarRespuesta(ERROR_OUT_OF_BOUNDS, socket_cliente);
        return;
    }

    char* datos = leerBloqueFisico(nroBloqueFisico);
    if (datos == NULL) {
        enviarRespuesta(OP_FAILED, socket_cliente);
        return;
    }

    t_paquete* paqueteRespuesta = crear_paquete();
    paqueteRespuesta->codigo_operacion = OP_SUCCESS;
    agregar_a_paquete(paqueteRespuesta, datos, superblock->blocksize);
    log_debug(loggerStorage, "Enviando paquete EXITOSO - READ");
    enviar_paquete(paqueteRespuesta, socket_cliente);
    eliminar_paquete(paqueteRespuesta);
    free(datos);
}

// Privados - Implementaciones
void enviarRespuesta(op_code codigo, int socket_cliente) {
    t_paquete* paqueteRespuesta = crear_paquete();
    log_debug(loggerStorage, "Enviando un paquete con codigo %d", codigo);
    paqueteRespuesta->codigo_operacion = codigo;
    enviar_paquete(paqueteRespuesta, socket_cliente);
    eliminar_paquete(paqueteRespuesta);
}

void agrandarFileTag(char* nombreFile, char* nombreTag, int nuevoTamanio, int bloquesActuales, t_config* metadata, char** blocks, int socket_cliente) {
    int i = bloquesActuales;
    bool error = false;

    log_debug(loggerStorage, "bloquesActuales: %d, nuevoTamanio: %d", bloquesActuales, nuevoTamanio);
    for (; i < nuevoTamanio; i++) {
        if (!agregarBloqueLogicoHL(nombreFile, nombreTag, i, 0)) {
            error = true;
            break;
        }
    }

    if (error) {
        for (int j = bloquesActuales; j < i; j++) {
            eliminarBloqueLogicoHL(nombreFile, nombreTag, j);
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
        actualizarBloques(metadata, bloquesActuales, nuevoTamanio, bloquesFisicos);
        free(bloquesFisicos);
        enviarRespuesta(OP_SUCCESS, socket_cliente);
    }
}

void reducirFileTag(char* nombreFile, char* nombreTag, int bloquesActuales, int nuevoTamanio, t_config* metadata, char** blocks, int socket_cliente) {
    int i;
    bool error = false;

    for (i = bloquesActuales - 1; i >= nuevoTamanio; i--) {
        if (!eliminarBloqueLogicoHL(nombreFile, nombreTag, i)) {
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
        actualizarBloques(metadata, bloquesActuales, nuevoTamanio, bloquesFisicos);
        free(bloquesFisicos);
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

void actualizarBloques(t_config* metadata, int bloquesActuales, int nuevoTamanio, int* bloquesFisicos) {
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

    // Actualizar la metadata (clave BLOCKS)
    config_set_value(metadata, "BLOCKS", bloquesActualizados);
    config_save(metadata);
    free(bloquesActualizados);
}

op_code borrarTag(char* nombreFile, char* nombreTag) {
    t_config* metadata = getMetaData(nombreFile, nombreTag);
    if (metadata == NULL) {
        return ERROR_FILE_NOT_FOUND;
    }

    char** blocks = config_get_array_value(metadata, "BLOCKS");
    int bloquesActuales = contarElementos(blocks);

    // Eliminar todos los bloques lógicos asociados
    for (int i = 0; i < bloquesActuales; i++) {
        eliminarBloqueLogicoHL(nombreFile, nombreTag, i);
    }

    // Eliminar el FileTag
    char* pathTag = rutaFileTag(nombreFile, nombreTag);
    config_destroy(metadata);
    borrar(pathTag);
    free(pathTag);
    string_array_destroy(blocks);

    return OP_SUCCESS;
}

op_code commitTag(char* nombreFile, char* nombreTag) {
    t_config* metadata = getMetaData(nombreFile, nombreTag);
    if (metadata == NULL) {
        return ERROR_FILE_NOT_FOUND;
    }

    char** blocks = config_get_array_value(metadata, "BLOCKS");
    int bloquesActuales = contarElementos(blocks);

    for (int i = 0; i < bloquesActuales; i++) {
        op_code resultado = validarBloqueLogico(nombreFile, nombreTag, i);
        if (resultado != OP_SUCCESS) {
            string_array_destroy(blocks);
            config_destroy(metadata);
            return resultado;
        }
    }

    config_set_value(metadata, "ESTADO", "COMMITED");
    config_save(metadata);
    config_destroy(metadata);

    return OP_SUCCESS;
}

op_code escribirBloqueLogico(char* nombreFile, char* nombreTag, int numeroBloqueLogico, void* datos, size_t sizeDatos) {
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
    char* pathBloqueLogico = rutaFileTag(nombreFile, nombreTag);
    string_append(&pathBloqueLogico, "/logical_blocks/");
    char* nombreBloqueLogico = crearNombreBloque(numeroBloqueLogico);
    string_append(&pathBloqueLogico, nombreBloqueLogico);
    free(nombreBloqueLogico);

    int bloqueFisico;
    if(esHardlinkUnico(pathBloqueLogico)) {
        bloqueFisico = obtenerBloqueFisico(nombreFile, nombreTag, numeroBloqueLogico);
    } else {
        // El bloque tiene mas de un Hardlink -> Le asigno uno nuevo
        bloqueFisico = obtenerBloqueLibre();
        if (bloqueFisico == -1) {
            free(pathBloqueLogico);
            config_destroy(metadata);
            return ERROR_NO_SPACE;
        }
        if (!eliminarBloqueLogicoHL(nombreFile, nombreTag, numeroBloqueLogico)) {
            free(pathBloqueLogico);
            config_destroy(metadata);
            return OP_FAILED;
        }
        if (!crearHardlink(pathBloqueLogico, numeroBloqueLogico, bloqueFisico)) {
            free(pathBloqueLogico);
            config_destroy(metadata);
            return OP_FAILED;
        }
    }
    if(escribirBloqueFisico(bloqueFisico, datos, sizeDatos, false)) {
        free(pathBloqueLogico);
        config_destroy(metadata);
        return OP_SUCCESS;
    } else { // TODO: Quizas estaría bueno que devuelva mas info, pero mucho refactor zzz
        free(pathBloqueLogico);
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