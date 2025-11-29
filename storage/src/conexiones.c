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
    char* idWorker = NULL;


    while(1) {
        op_code cod = recibir_operacion(socket_cliente);
        usleep(configStorage->retardo_operacion * 1000); // Simulo retardo de operacion
        log_debug(loggerStorage, "Operacion recibida: %d", cod);
        datosRecibidos = recibir_paquete(socket_cliente);
        switch(cod) {
            case TAMANIO_BLOQUE: {
                datosEnviados = crearNuevoPaquete(TAMANIO_BLOQUE);
                agregar_a_paquete(datosEnviados, &(superblock->blocksize), sizeof(int));
                enviar_paquete(datosEnviados, socket_cliente);
                break;
            }
            case OP_CREATE: {
                crearFile(datosRecibidos, socket_cliente);
                break;
            }
            case OP_TRUNCATE: {
                truncar(datosRecibidos, socket_cliente);
                break;
            }
            case OP_TAG: {
                tag(datosRecibidos, socket_cliente);
                break;
            }
            case OP_DELETE: {
                eliminarTag(datosRecibidos, socket_cliente);
                break;
            }
            case OP_COMMIT: {
                commit(datosRecibidos, socket_cliente);
                break;
            }
            case OP_WRITE: {
                usleep(configStorage->retardo_acceso_bloque * 1000); // Simulo retardo de acceso a bloque
                writeFileTag(datosRecibidos, socket_cliente);
                break;
            }
            case PED_PAG: { // era OP_READ pero la cambie xq ya tengo op_read en worker
                usleep(configStorage->retardo_acceso_bloque * 1000); // Simulo retardo de acceso a bloque
                readBloqueLogico(datosRecibidos, socket_cliente);
                break;
            }
            case ID_WORKER: {
                agregarWorker();
                if(list_size(datosRecibidos) > 0) {
                    idWorker = strdup(list_get(datosRecibidos, 0));
                    log_info(loggerStorage, "Worker con ID %s conectado. Total Workers: %d", idWorker, numeroWorkers);
                } else {
                    log_info(loggerStorage, "Worker conectado. Total Workers: %d", numeroWorkers);
                }
                break;
            }
            case -1: {
                // “##Se desconecta el Worker <WORKER_ID> - Cantidad de Workers: <CANTIDAD>”
                eliminarWorker();
                if (idWorker != NULL) {
                    log_info(loggerStorage, "##Se desconecta el Worker %s - Cantidad de Workers: %d", idWorker, numeroWorkers);
                } else {
                    log_info(loggerStorage, "##Se desconecta un Worker - Cantidad de Workers: %d", numeroWorkers);
                }
                free(idWorker);
                return;
            }
            default:
                log_error(loggerStorage, "Operacion desconocida. No se puede atender al cliente.");
                return;
        }

        if (datosEnviados != NULL) {
            eliminar_paquete(datosEnviados);
            datosEnviados = NULL;
        }
        if (datosRecibidos != NULL) {
            list_destroy_and_destroy_elements(datosRecibidos, free);
            datosRecibidos = NULL;
        }
    }
}

t_paquete* crearNuevoPaquete(op_code codigo) {
    t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion = codigo;
    return paquete;
}