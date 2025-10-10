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

    t_list* datosRecibidos;
    t_paquete* datosEnviados;

    while(1) {
        op_code cod = recibir_operacion(socket_cliente);
        switch(cod) {
            case TAMANIO_BLOQUE: {
                datosRecibidos = recibir_paquete(socket_cliente);
                list_destroy_and_destroy_elements(datosRecibidos, free);
                datosEnviados = crearNuevoPaquete(TAMANIO_BLOQUE);
                agregar_a_paquete(datosEnviados, &(superblock->blocksize), sizeof(int));
                enviar_paquete(datosEnviados, socket_cliente);
                break;
            }
            case OP_CREATE: {
                // TODO: Implementar comportamiento para OP_CREATE
                break;
            }
            case OP_TRUNCATE: {
                // TODO: Implementar comportamiento para OP_TRUNCATE
                break;
            }
            case OP_DELETE: {
                // TODO: Implementar comportamiento para OP_DELETE
                break;
            }
            case OP_TAG: {
                // TODO: Implementar comportamiento para OP_TAG
                break;
            }
            case OP_COMMIT: {
                // TODO: Implementar comportamiento para OP_COMMIT
                break;
            }
            case PED_PAG: {
                // TODO: Implementar comportamiento para PED_PAG
                break;
            }
            case GUARDAR_MODIFICADAS: {
                // TODO: Implementar comportamiento para GUARDAR_MODIFICADAS
                break;
            }
            default:
                log_error(loggerStorage, "Operacion desconocida. No se puede atender al cliente.");
                return;
        }
        eliminar_paquete(datosEnviados);
    }
}

t_paquete* crearNuevoPaquete(op_code codigo) {
    t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion = codigo;
    return paquete;
}