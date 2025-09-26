#include <t_conexionWorker.h>

void workers_iniciar(void) {
    LISTA_WORKERS = list_create();
}

void workers_destruir(void) {
    if (!LISTA_WORKERS) return;
    pthread_mutex_lock(&mutex_workers);
    for (int i = 0; i < list_size(LISTA_WORKERS); i++) {
        t_conexionWorker* w = list_get(LISTA_WORKERS, i);
        if (w->conectado && w->fd >= 0) close(w->fd);
        free(w);
    }
    list_destroy(LISTA_WORKERS);
    LISTA_WORKERS = NULL;
    pthread_mutex_unlock(&mutex_workers);
}

void worker_registrar(int id, int fd) {
    t_conexionWorker* w = malloc(sizeof(*w));
    w->id = id;
    w->fd = fd;
    w->conectado = true;
    w->qid_actual = -1;

    pthread_mutex_lock(&mutex_workers);
    if(!LISTA_WORKERS){
        workers_iniciar();
    }
    list_add(LISTA_WORKERS, w);
    pthread_mutex_unlock(&mutex_workers);

    if (loggerMaster) log_info(loggerMaster, "Worker %d conectado (fd=%d)", id, fd);
}

void worker_eliminar_por_fd(int fd) {
    pthread_mutex_lock(&mutex_workers);
    for (int i = 0; i < list_size(LISTA_WORKERS); i++) {
        t_conexionWorker* w = list_get(LISTA_WORKERS, i);
        if (w->fd == fd) {
            if (loggerMaster) log_warning(loggerMaster, "Worker %d desconectado (fd=%d)", w->id, fd);
            if (w->conectado && w->fd >= 0) close(w->fd);
            list_remove(LISTA_WORKERS, i);
            free(w);
            break;
        }
    }
    pthread_mutex_unlock(&mutex_workers);
}


void worker_marcar_libre_por_fd(int fd) {
    pthread_mutex_lock(&mutex_workers);
    for (int i = 0; i < list_size(LISTA_WORKERS); i++) {
        t_conexionWorker* w = list_get(LISTA_WORKERS, i);
        if (w->fd == fd) { w->qid_actual = -1; break; }
    }
    pthread_mutex_unlock(&mutex_workers);
}

int workers_conectados(void) {
    pthread_mutex_lock(&mutex_workers);
    int n = LISTA_WORKERS ? list_size(LISTA_WORKERS) : 0;
    pthread_mutex_unlock(&mutex_workers);
    return n;
}

int workers_disponibles(void) {
    int libres = 0;
    pthread_mutex_lock(&mutex_workers);
    for (int i = 0; i < list_size(LISTA_WORKERS); i++) {
        t_conexionWorker* w = list_get(LISTA_WORKERS, i);
        if (w->conectado && w->qid_actual == -1) libres++;
    }
    pthread_mutex_unlock(&mutex_workers);
    return libres;
}


int worker_id_por_fd(int fd) {
    int id = -1;
    pthread_mutex_lock(&mutex_workers);
    for (int i = 0; i < list_size(LISTA_WORKERS); i++) {
        t_conexionWorker* w = list_get(LISTA_WORKERS, i);
        if (w->fd == fd) { id = w->id; break; }
    }
    pthread_mutex_unlock(&mutex_workers);
    return id;
}

void worker_eliminar_por_id(int id) {
    pthread_mutex_lock(&mutex_workers);
    for (int i = 0; i < list_size(LISTA_WORKERS); i++) {
        t_conexionWorker* w = list_get(LISTA_WORKERS, i);
        if (w->id == id) {
            if (loggerMaster) log_warning(loggerMaster, "Worker %d desconectado (fd=%d)", w->id, w->fd);
            if (w->conectado && w->fd >= 0) close(w->fd);
            list_remove(LISTA_WORKERS, i);
            free(w);
            pthread_mutex_unlock(&mutex_workers);
            return;
        }
    }
    pthread_mutex_unlock(&mutex_workers);
    if (loggerMaster) log_warning(loggerMaster, "Se intentó eliminar Worker id=%d que no existe", id);
}

void* atenderWorker(void arg){
  t_conexionWorker* conexionWorker =  (t_conexionWorker*) arg; 

  int fd = conexionWorker ->fd;
  int id = conexionWorker ->id;
  while (1) {
      int codigoOperacion = recibir_operacion(fd); // Recibo la operacion del worker
      switch (codigoOperacion) {
            case RTA_DESALOJO: {
            // payload: [qid:int][pc:int]
            t_list* p = recibir_paquete(fd);
            if (list_size(p) < 2) {
                log_error(loggerMaster, "Worker %d: RTA_DESALOJO mal formado", id);
                list_destroy_and_destroy_elements(p, free);
                break;
            }
            int qid = *(int*) list_get(p, 0);
            int pc  = *(int*) list_get(p, 1);
            list_destroy_and_destroy_elements(p, free);

            // TODO: actualizar PC y reencolar a READY
            // query_actualizar_pc(qid, pc);
            // query_reencolar_ready(qid);

            worker_marcar_libre_por_fd(fd);
            // TODO: avisar al planificador (cuando lo tengas)
            log_info(loggerMaster, "## Query %d: desalojo confirmado por Worker %d (PC=%d)", qid, id, pc);
            break;
        }

        case OP_END: {
            // payload: [qid:int][rc:int]   rc = código de retorno de la query
            t_list* p = recibir_paquete(fd);
            if (list_size(p) < 2) {
                log_error(loggerMaster, "Worker %d: OP_END mal formado", id);
                list_destroy_and_destroy_elements(p, free);
                break;
            }
            int qid = *(int*) list_get(p, 0);
            int rc  = *(int*) list_get(p, 1);
            list_destroy_and_destroy_elements(p, free);

            // TODO: marcar EXIT y avisar al QC correspondiente (usando q->fd_qc)
            // query_marcar_exit(qid, rc);
            // qc_notificar_fin(qid, rc);

            worker_marcar_libre_por_fd(fd);
            // TODO: avisar al planificador si hay READY esperando
            log_info(loggerMaster, "## Query %d: finalizada en Worker %d (rc=%d)", qid, id, rc);
            break;
        }

        case OP_READ: {
            // payload: [qid:int][n:int][bytes:n]
            t_list* p = recibir_paquete(fd);
            if (list_size(p) < 3) {
                log_error(loggerMaster, "Worker %d: OP_READ mal formado", id);
                list_destroy_and_destroy_elements(p, free);
                break;
            }
            int qid = *(int*) list_get(p, 0);
            int n   = *(int*) list_get(p, 1);
            void* buf = malloc(n);
            memcpy(buf, list_get(p, 2), n);
            list_destroy_and_destroy_elements(p, free);

            // TODO: forward de la lectura al QC de esa query (q->fd_qc)
            // qc_forward_lectura(qid, buf, n);

            free(buf);
            // El worker sigue ejecutando; no lo marcamos libre
            break;
        }

        case OP_WRITE: {
            // Definí el payload en tu worker. Ejemplos posibles:
            // a) ACK de un write: [qid:int][status:int]
            // b) Datos escritos:  [qid:int][n:int][bytes:n]
            t_list* p = recibir_paquete(fd);
            if (list_size(p) < 2) {
                log_error(loggerMaster, "Worker %d: OP_WRITE mal formado", id);
                list_destroy_and_destroy_elements(p, free);
                break;
            }
            int qid = *(int*) list_get(p, 0);

            // TODO: según tu protocolo, interpretá el resto y reenviá/registrá:
            // int status = *(int*) list_get(p, 1);
            // ó bien:
            // int n = *(int*) list_get(p, 1); void* bytes = list_get(p, 2) ...

            // TODO: si hay que forwardear al QC: qc_forward_write(qid, ...);

            list_destroy_and_destroy_elements(p, free);
            break;
        }

        default:
            log_warning(loggerMaster, "Worker %d: op_code desconocido %d", id, codigoOperacion);
            // Si el op desconocido tenía payload, tu worker está mandando cualquier cosa.
            break;
        }
    }
  }
