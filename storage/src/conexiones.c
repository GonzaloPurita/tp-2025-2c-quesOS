#include "conexiones.h"

// Funciones privadas --> No están en el .h porque no se usa afuera
void recibirCliente(void* cliente);
t_paquete* crearNuevoPaquete(op_code codigo);

void iniciarServidor() {
    char* puertoEscucha = string_itoa(configStorage->puerto_escucha);
    int socket_servidor = crearConexionServidor(puertoEscucha);
    free(puertoEscucha);

    if (socket_servidor == -1) {
        log_error(loggerStorage, "No se pudo crear la conexion del servidor");
        return;
    }

    while(1) {
        int socket_cliente = esperarCliente(socket_servidor);
        if (socket_cliente == -1) {
            log_error(loggerStorage, "No se pudo aceptar la conexion del cliente");
            continue;
        }
        
        pthread_t hiloCliente;
        int* pSocket = malloc(sizeof(int));
        *pSocket = socket_cliente;
        pthread_create(&hiloCliente, NULL, (void*) recibirCliente, pSocket);
        pthread_detach(hiloCliente); // El hilo se libera solo al terminar
    }
}

void recibirCliente(void* cliente) {
    int socket_cliente = *((int*) cliente);
    free(cliente);

    t_list* datosRecibidos = NULL;
    t_paquete* datosEnviados = NULL;
    log_info(loggerStorage, "Entre a recibirCliente()");
    while(1) {
        log_info(loggerStorage, "Esperando peticiones del Worker...");
        op_code cod = recibir_operacion(socket_cliente);
        switch(cod) {
            case TAMANIO_BLOQUE: {
                log_debug(loggerStorage, "Recibida operacion TAMANIO_BLOQUE del Worker");
                log_info(loggerStorage, "Handshake correcto, respondiendo con blocksize=%d", superblock->blocksize);
                datosRecibidos = recibir_paquete(socket_cliente);
                list_destroy_and_destroy_elements(datosRecibidos, free);
                log_info(loggerStorage, "tamanio de bloque: %d", superblock->blocksize);
                datosEnviados = crearNuevoPaquete(TAMANIO_BLOQUE);
                agregar_a_paquete(datosEnviados, &(superblock->blocksize), sizeof(int));
                enviar_paquete(datosEnviados, socket_cliente);
                break;
            }
            case OP_CREATE: {
                log_debug(loggerStorage, "Recibida operacion OP_CREATE del Worker");
                datosRecibidos = recibir_paquete(socket_cliente);
                list_destroy_and_destroy_elements(datosRecibidos, free);
                datosEnviados = crearNuevoPaquete(OP_SUCCESS);
                enviar_paquete(datosEnviados, socket_cliente);
                // TODO: Implementar comportamiento para OP_CREATE
                break;
            }
            case OP_TRUNCATE: {
                log_debug(loggerStorage, "Recibida operacion OP_TRUNCATE del Worker");
                datosRecibidos = recibir_paquete(socket_cliente);
                list_destroy_and_destroy_elements(datosRecibidos, free);
                datosEnviados = crearNuevoPaquete(OP_SUCCESS);
                enviar_paquete(datosEnviados, socket_cliente);
                // TODO: Implementar comportamiento para OP_TRUNCATE_ADD
                break;
            }
            case OP_DELETE: {
                log_debug(loggerStorage, "Recibida operacion OP_DELETE del Worker");
                datosRecibidos = recibir_paquete(socket_cliente);
                list_destroy_and_destroy_elements(datosRecibidos, free);
                datosEnviados = crearNuevoPaquete(OP_SUCCESS);
                enviar_paquete(datosEnviados, socket_cliente);
                // TODO: Implementar comportamiento para OP_DELETE
                break;
            }
            case OP_TAG: {
                log_debug(loggerStorage, "Recibida operacion OP_TAG del Worker");
                datosRecibidos = recibir_paquete(socket_cliente);
                list_destroy_and_destroy_elements(datosRecibidos, free);
                datosEnviados = crearNuevoPaquete(OP_SUCCESS);
                enviar_paquete(datosEnviados, socket_cliente);
                // TODO: Implementar comportamiento para OP_TAG
                break;
            }
            case OP_COMMIT: {
                log_debug(loggerStorage, "Recibida operacion OP_COMMIT del Worker");
                datosRecibidos = recibir_paquete(socket_cliente);
                list_destroy_and_destroy_elements(datosRecibidos, free);
                datosEnviados = crearNuevoPaquete(OP_SUCCESS);
                enviar_paquete(datosEnviados, socket_cliente);
                // TODO: Implementar comportamiento para OP_COMMIT
                break;
            }
            case OP_WRITE: {
                log_debug(loggerStorage, "Recibida operacion OP_WRITE del Worker");
                datosRecibidos = recibir_paquete(socket_cliente);
                list_destroy_and_destroy_elements(datosRecibidos, free);
                datosEnviados = crearNuevoPaquete(OP_SUCCESS);
                agregar_a_paquete(datosEnviados, "Lei algo", 9);
                enviar_paquete(datosEnviados, socket_cliente);
                // TODO: Implementar comportamiento para OP_WRITE
                break;
            }
            case OP_READ: {
                log_debug(loggerStorage, "Recibida operacion OP_READ del Worker");
                datosRecibidos = recibir_paquete(socket_cliente);
                list_destroy_and_destroy_elements(datosRecibidos, free);
                datosEnviados = crearNuevoPaquete(OP_SUCCESS);
                enviar_paquete(datosEnviados, socket_cliente);
                // TODO: Implementar comportamiento para OP_READ
                break;
            }
            default:
                log_error(loggerStorage, "Operacion desconocida: %d.", cod);
                return;
        }

        if (datosEnviados != NULL) {
            eliminar_paquete(datosEnviados);
        }
    }
}

t_paquete* crearNuevoPaquete(op_code codigo) {
    t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion = codigo;
    return paquete;
}