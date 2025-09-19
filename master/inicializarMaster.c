#include <inicializarMaster.h>

static int server_fd_master = -1;
static int NEXT_QID = 0;

void* recibirConexiones(void* arg);
void* atenderCliente(void* arg);

void iniciarConexionesMaster() {
    char* puerto = string_itoa(configMaster->puerto_escucha);
    server_fd_master = crearConexionServidor(puerto);
    free(puerto);
    inicializarSemaforos();
    workers_iniciar();

    log_info(loggerMaster, "Master escuchando en puerto %d", configMaster->puerto_escucha);

    pthread_t hilo;
    pthread_create(&hilo, NULL, (void*) recibirConexiones, NULL);
    pthread_detach(hilo);
}

void cerrarConexionesMaster() {
    if (server_fd_master != -1) close(server_fd_master);
}

void* recibirConexiones(void* arg) {
    while (1) {
        int cliente_fd = esperarCliente(server_fd_master);
        int* fd_ptr = malloc(sizeof(int));
        *fd_ptr = cliente_fd;

        pthread_t hilo;
        pthread_create(&hilo, NULL, (void*) atenderCliente, fd_ptr);
        pthread_detach(hilo);
    }
    return NULL;
}

void* atenderCliente(void* arg) {
    int fd = *((int*) arg);
    free(arg);

    op_code cod = recibir_operacion(fd);

    switch (cod) {
        case ID_WORKER: {
            t_list* datos = recibir_paquete(fd);
            if (list_size(datos) != 1) {
                log_error(loggerMaster, "fd=%d: ID_WORKER mal formado (items=%d)", fd, list_size(datos));
                list_destroy_and_destroy_elements(datos, free);
                close(fd);
                return NULL;
            }

            int worker_id = 0;
            memcpy(&worker_id, list_get(datos, 0), sizeof(int));
            list_destroy_and_destroy_elements(datos, free);

            worker_registrar(worker_id, fd);

            // ACK y a otra cosa
            int ok = 1;
            t_paquete* r = crear_paquete();
            r->codigo_operacion = RTA_ID_WORKER;
            agregar_a_paquete(r, &ok, sizeof(int));
            enviar_paquete(r, fd);
            eliminar_paquete(r);

            log_info(loggerMaster, "Workers conectados=%d, disponibles=%d", workers_conectados(), workers_disponibles());

            int* fd_ptr = malloc(sizeof(int));
            *fd_ptr = fd;
            pthread_t th;
            pthread_create(&th, NULL, atender_worker, fd_ptr);
            pthread_detach(th);

            return NULL; // Lanzo un NULL porque el hilo de atender_worker se encarga de cerrar el fd


        }

        case SUBMIT_QUERY: {
            t_list* datos = recibir_paquete(fd);
            if (list_size(datos) < 3) { /* log + close + return */ }

            int prioridad = *(int*) list_get(datos, 0);
            int len       = *(int*) list_get(datos, 1);
            char* path    = (char*) list_get(datos, 2);
            char* path_dup = strndup(path, len);
            list_destroy_and_destroy_elements(datos, free);

            int qid;
            pthread_mutex_lock(&mutex_qid);
            qid = NEXT_QID++;
            pthread_mutex_unlock(&mutex_qid);

            // TODO: crear t_query y encolarla a READY

            log_info(loggerMaster, "Query QID=%d recibida (prio=%d, path=%s)",
                    qid, prioridad, path_dup);

            t_paquete* r = crear_paquete();
            r->codigo_operacion = RTA_SUBMIT_QUERY;
            agregar_a_paquete(r, &qid, sizeof(int));
            enviar_paquete(r, fd);
            eliminar_paquete(r);

            free(path_dup);
            break;
        }

        default:
            log_warning(loggerMaster, "fd=%d: op_code inicial desconocido %d", fd, cod);
            close(fd);
            break;
    }
    return NULL;
}
