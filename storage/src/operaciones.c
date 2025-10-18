#include "operaciones.h"

// Privados
void enviarRespueta(op_code codigo, int socket_cliente);
void agrandarFileTag(char* nombreFile, char* nombreTag, int nuevoTamanio, int bloquesActuales, t_config* metadata, int socket_cliente);
void reducirFileTag(char* nombreFile, char* nombreTag, int bloquesActuales, int nuevoTamanio, t_config* metadata, int socket_cliente);

void crearFile(t_list* data, int socket_cliente) {
    char* nombreFile = list_get(data, 0);
    char* nombreTag = list_get(data, 1);
    op_code resultado = crearFileTag(nombreFile, nombreTag);
    enviarRespueta(resultado, socket_cliente);
}

void truncar(t_list* data, int socket_cliente) {
    char* nombreFile = list_get(data, 0);
    char* nombreTag = list_get(data, 1);
    int nuevoTamanio = *((int*) list_get(data, 2));

    t_config* metadata = getMetaData(nombreFile, nombreTag);

    if (metadata == NULL) {
        enviarRespueta(ERROR_FILE_NOT_FOUND, socket_cliente);
        return;
    }

    int bloquesActuales = config_get_int_value(metadata, "BLOCKS");

    if(bloquesActuales == nuevoTamanio){
        enviarRespueta(OP_SUCCESS, socket_cliente);
    }
    else if(bloquesActuales < nuevoTamanio){
        agrandarFileTag(nombreFile, nombreTag, nuevoTamanio, bloquesActuales, metadata, socket_cliente);
    }
    else{
        reducirFileTag(nombreFile, nombreTag, bloquesActuales, nuevoTamanio, metadata, socket_cliente);   
    }
}

// Privados - Implementaciones
void enviarRespueta(op_code codigo, int socket_cliente) {
    t_paquete* paqueteRespuesta = crear_paquete();
    paqueteRespuesta->codigo_operacion = codigo;
    enviar_paquete(paqueteRespuesta, socket_cliente);
    eliminar_paquete(paqueteRespuesta);
}

void agrandarFileTag(char* nombreFile, char* nombreTag, int nuevoTamanio, int bloquesActuales, t_config* metadata, int socket_cliente) {
    int i = bloquesActuales;
    for(i; i < nuevoTamanio; i++) {
        if(!agregarBloqueLogicoHL(nombreFile, nombreTag, i, 0)) {
            enviarRespueta(OP_FAILED, socket_cliente);
            goto error;
        }
    }

    error:
        for (int j = bloquesActuales; j < i; j++) {
            // TODO:
            // Aquí debería eliminar los bloques lógicos que se hayan creado hasta el error
            // Pero como no tenemos una función para eso, lo dejamos así por ahora
        }
        return;
}

// TODO: Implementar la función reducirFileTag
void reducirFileTag(char* nombreFile, char* nombreTag, int bloquesActuales, int nuevoTamanio, t_config* metadata, int socket_cliente) {
    for(int i = bloquesActuales - 1; i >= nuevoTamanio; i--) {
        // if(!eliminarBloqueLogicoHL(nombreFile, nombreTag, i)) {
        //     enviarRespueta(OP_FAILED, socket_cliente);
        // }
    }
}