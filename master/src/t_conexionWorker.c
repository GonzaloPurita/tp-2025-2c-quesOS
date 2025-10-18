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
        workers_iniciar()
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

bool exec_buscar_por_qid(int qid, int* out_fd_qc) {
    bool ok = false;
    pthread_mutex_lock(&mutex_cola_exec);
    int n = list_size(cola_exec);
    for (int i = 0; i < n; i++) {
        t_query* q = list_get(cola_exec, i);
        if (q->QCB->qid == qid) {
            *out_fd_qc = q->fd_qc;   
            ok = true;
            break;
        }
    }
    pthread_mutex_unlock(&mutex_cola_exec);
    return ok;
}

void* atenderWorker(void arg){
  t_conexionWorker* conexionWorker =  (t_conexionWorker*) arg; 

  int fd = conexionWorker ->fd;
  int id = conexionWorker ->id;
  while (1) {
      int codigoOperacion = recibir_operacion(fd); // Recibo la operacion del worker
      switch (codigoOperacion) {
            case RTA_DESALOJO: {
            t_list* p = recibir_paquete(fd_worker);
            if (!p || list_size(p) < 2) {
                log_error(loggerMaster, "Worker %d: RTA_DESALOJO mal formado", worker->id);
                if (p) list_destroy_and_destroy_elements(p, free);
                break;
            }
            int qid = *(int*) list_get(p, 0);
            int pc  = *(int*) list_get(p, 1);
            list_destroy_and_destroy_elements(p, free);

            // 1) Actualizar PC en la query víctima (ya la movimos a READY en realizarDesalojo)
            int actualizado = 0;
            pthread_mutex_lock(&mutex_cola_ready);
            for (int i = 0; i < list_size(cola_ready); i++) {
                t_query* q = list_get(cola_ready, i);
                if (q->QCB->qid == qid) {
                    q->QCB->pc = pc;
                    actualizado = 1;
                    break;
                }
            }
            pthread_mutex_unlock(&mutex_cola_ready);

            if (!actualizado) {
                // Si no está en READY puede ser carrera rara: log y seguimos para no quedarnos colgados
                log_warning(loggerMaster, "RTA_DESALOJO QID=%d no encontrada en READY para actualizar PC=%d", qid, pc);
            } else {
                log_info(loggerMaster, "## (%d) - PC actualizado por desalojo a %d", qid, pc);
            }

            // 2) Despertar al hilo desalojar() para que envíe la nueva query
            sem_post(&worker->semaforo);
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

            // 1) Verificamos que el el QID esté en ejecución
            int fd_qc = -1;

            if (!exec_buscar_por_qid(qid, &fd_qc)) {
                log_warning(loggerMaster, "Worker %d: READ para QID=%d no encontrada en EXEC", id, qid);
                free(buf);
                break;
            }

            // 2) Le mandamos el mensaje al Query_Control
            t_paquete* out = crear_paquete();
            out->codigo_operacion = MENSAJE_LECTURA;   // op code hacia el QC
            agregar_a_paquete(out, &qid, sizeof(int));
            agregar_a_paquete(out, &n,   sizeof(int));
            agregar_a_paquete(out, buf,  n);

            int rc = enviar_paquete(out, q->fd_qc);
            eliminar_paquete(out);
            free(buf);

            if (rc != 0) {
                // 3) El QC está caído -> La QUERY tiene que ir a EXIT
                log_warning(loggerMaster, "QID=%d: QC desconectado durante READ. Se cancela la query.", qid);
                close(q->fd_qc);
                q->fd_qc = -1;

                // Sacamos a la query de Exec pidiendo el desalojo
                // el hilo atender_worker va a recibir el PC y como el fd_qc == -1, NO va a reencolar a READY, finaliza la query.
                enviar_preempt_a_worker(id, qid);

                // Importante: no mandamos a EXIT acá a la query. Esperamos el PC del worker para cerrar limpio
                // y liberar el slot del worker (sem_post(&workers_disponibles) en ese handler).
            }
            // El worker sigue ejecutando; no se marca libre.
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
