#include "operaciones.h"

// Privados
void enviarRespuesta(op_code codigo, int socket_cliente);
void agrandarFileTag(char* nombreFile, char* nombreTag, int nuevoTamanio, int bloquesActuales, t_config* metadata, char** blocks, int socket_cliente);
void reducirFileTag(char* nombreFile, char* nombreTag, int bloquesActuales, int nuevoTamanio, t_config* metadata, char** blocks, int socket_cliente);
int contarElementos(char** array);
void actualizarBloques(t_config* metadata, int bloquesActuales, int nuevoTamanio, int* bloquesFisicos);
op_code borrarTag(char* nombreFile, char* nombreTag);

void crearFile(t_list* data, int socket_cliente) {
    char* nombreFile = list_get(data, 0);
    char* nombreTag = list_get(data, 1);
    op_code resultado = crearFileTag(nombreFile, nombreTag);
    enviarRespuesta(resultado, socket_cliente);
}

void truncar(t_list* data, int socket_cliente) {
    char* nombreFile = list_get(data, 0);
    char* nombreTag = list_get(data, 1);
    int nuevoTamanio = *((int*) list_get(data, 2));

    t_config* metadata = getMetaData(nombreFile, nombreTag);

    if (metadata == NULL) {
        enviarRespuesta(ERROR_FILE_NOT_FOUND, socket_cliente);
        return;
    }

    if (strcmp("COMMITED", config_get_string_value(metadata, "ESTADO")) == 0) {
        enviarRespuesta(ERROR_WRITE_NOT_ALLOWED, socket_cliente);
        config_destroy(metadata);
        return;
    }

    char** blocks = config_get_array_value(metadata, "BLOCKS");
    int bloquesActuales = contarElementos(blocks);

    if(bloquesActuales == nuevoTamanio){
        enviarRespuesta(OP_SUCCESS, socket_cliente);
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

void tag(t_list* data, int socket_cliente) {
    char* nombreFile = list_get(data, 0);
    char* nombreTag = list_get(data, 1);
    op_code resultado = crearTag(nombreFile, nombreTag);
    enviarRespuesta(resultado, socket_cliente);
}

void eliminarTag(t_list* data, int socket_cliente) {
    char* nombreFile = list_get(data, 0);
    char* nombreTag = list_get(data, 1);
    op_code resultado = borrarTag(nombreFile, nombreTag);
    enviarRespuesta(resultado, socket_cliente);
}

// Privados - Implementaciones
void enviarRespuesta(op_code codigo, int socket_cliente) {
    t_paquete* paqueteRespuesta = crear_paquete();
    paqueteRespuesta->codigo_operacion = codigo;
    enviar_paquete(paqueteRespuesta, socket_cliente);
    eliminar_paquete(paqueteRespuesta);
}

void agrandarFileTag(char* nombreFile, char* nombreTag, int nuevoTamanio, int bloquesActuales, t_config* metadata, char** blocks, int socket_cliente) {
    int i = bloquesActuales;
    bool error = false;

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
        // TODO: Restauración?
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

    // Actualizar la metadata
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