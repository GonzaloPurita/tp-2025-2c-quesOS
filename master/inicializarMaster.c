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
            if (configMaster->tiempo_aging_ms > 0) {
                pthread_t th_aging;
                pthread_create(&th_aging, NULL, hilo_aging, NULL);
                pthread_detach(th_aging);
            }

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
            // Recibo la query: [ int prioridad ][ int len ][ char[len] path ]
            t_list* datos = recibir_paquete(fd);
            if (!datos || list_size(datos) < 3) {
                log_error(loggerMaster, "fd=%d: SUBMIT_QUERY mal formado (items=%d)", fd, datos ? list_size(datos) : -1);
                if (datos) list_destroy_and_destroy_elements(datos, free);
                close(fd);
                return NULL;
            }
            void* hilo_aging(void* arg) {
    const int ms = configMaster->tiempo_aging_ms;
    if (ms <= 0) return NULL;

    for (;;) {
        usleep(ms * 1000);
        AGING_TICK_GLOBAL++;
        aplicar_aging_ready();
        // Puede haber cambiado quién es la mejor; despertá al planificador
        sem_post(&hay_query_ready);
    }
    return NULL;
}
            
            char* path_dup = strndup(path_in, len);
            list_destroy_and_destroy_elements(datos, free);

            // Genero QID
            int qid = 0;
            pthread_mutex_lock(&mutex_qid);
            qid = NEXT_QID++;
            pthread_mutex_unlock(&mutex_qid);

            // Creo la query
            t_query* q = crearQuery(path_dup, prioridad);
            if (!q) {
                log_error(loggerMaster, "No se pudo crear la query (prio=%d)", prioridad);
                close(fd);
                return NULL;
            }

            q->QCB->qid = qid;
            q->QCB->pc  = 0;
            q->fd_qc = fd;

            if(configMaster -> algoritmo_planificacion == "FIFO"){
                mutex_lock(&mutex_cola_ready);
                list_add(cola_ready, q);
                mutex_unlock(&mutex_cola_ready);
                sem_post(&hay_query_ready)
            }
            else{
                agregarAReadyPorPrioridad(q);  // esta función hacee el sem_post(&hay_query_ready) para avisar que hay una query a planificar
            }
            

            log_info(loggerMaster, "QID=%d recibida (prio=%d, path=%s)", qid, q->prioridad /* o q->prioridad_actual */, q->path);

            // ACK al Query_Control con el QID asignado
            t_paquete* r = crear_paquete();
            r->codigo_operacion = RTA_SUBMIT_QUERY;
            agregar_a_paquete(r, &qid, sizeof(int));
            enviar_paquete(r, fd);
            eliminar_paquete(r);

            // NO cerrar fd. Queda asociado a esta query para reenviar las operaciones de worker
            // Este hilo termina; otros hilos usarán q->QCB->fd_qc para escribirle al QC.
            return NULL;
        }

        default:
            log_warning(loggerMaster, "fd=%d: op_code inicial desconocido %d", fd, cod);
            close(fd);
            break;
    }
    return NULL;
}
