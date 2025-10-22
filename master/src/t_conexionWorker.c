#include <t_conexionWorker.h>

t_list* LISTA_WORKERS = NULL;

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
    sem_init(&w->semaforo, 0, 0);

    pthread_mutex_lock(&mutex_workers);
    if(!LISTA_WORKERS){
        workers_iniciar();
    }
    list_add(LISTA_WORKERS, w);
    pthread_mutex_unlock(&mutex_workers);

    if (loggerMaster) log_info(loggerMaster, "Worker %d conectado (fd=%d)", id, fd);
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

t_conexionWorker* encontrarWorkerPorQid(int qid) {
    pthread_mutex_lock(&mutex_workers);
    t_conexionWorker* w = NULL;
    for (int i = 0; i < list_size(LISTA_WORKERS); i++) {
        t_conexionWorker* it = list_get(LISTA_WORKERS, i);
        if (it->conectado && it->qid_actual == qid) { w = it; break; }
    }
    pthread_mutex_unlock(&mutex_workers);
    return w;
}

bool exec_buscar_por_qid(int qid, int* out_fd_qc) {
    bool ok = false;
    pthread_mutex_lock(&mutex_cola_exec);
    int n = list_size(cola_exec);
    for (int i = 0; i < n; i++) {
        t_query* q = list_get(cola_exec, i);
        if (q->QCB->QID == qid) {
            *out_fd_qc = q->fd_qc;   
            ok = true;
            break;
        }
    }
    pthread_mutex_unlock(&mutex_cola_exec);
    return ok;
}

void worker_desconectar_por_fd(int fd) {
    // Buscar el worker por fd sin lambdas
    pthread_mutex_lock(&mutex_workers);
    t_conexionWorker* w = NULL;
    for (int i = 0; i < list_size(LISTA_WORKERS); i++) {
        t_conexionWorker* it = list_get(LISTA_WORKERS, i);
        if (it->fd == fd) { w = it; break; }
    }
    pthread_mutex_unlock(&mutex_workers);

    if (!w) return;

    log_info(loggerMaster, "Se detectó desconexión del Worker %d (fd=%d)", w->id, fd);

    w->conectado = false;
    // si tenés otra parte que se encarga de cerrar, ok; si no, cerralo acá
    close(fd);

    // Si tenía una query en ejecución, finalizala con error y notificá al QC
    if (w->qid_actual != -1) {
        int qid = w->qid_actual;   // cachear por seguridad

        pthread_mutex_lock(&mutex_cola_exec);
        for (int i = 0; i < list_size(cola_exec); i++) {
            t_query* q = list_get(cola_exec, i);
            if (q->QCB->QID == qid) {
                list_remove(cola_exec, i);
                pthread_mutex_unlock(&mutex_cola_exec);

                log_warning(loggerMaster,
                    "Finalizando Query %d con error por desconexión del Worker %d",
                    q->QCB->QID, w->id);

                // Notificar al QC solo con qid
                t_paquete* paquete = crear_paquete();
                paquete->codigo_operacion = QUERY_FINALIZADA; // tu opcode real
                agregar_a_paquete(paquete, &qid, sizeof(int));
                eliminar_paquete(paquete);

                actualizarMetricas(Q_EXEC, Q_EXIT, q);

                pthread_mutex_lock(&mutex_cola_exit);
                list_add(cola_exit, q);
                pthread_mutex_unlock(&mutex_cola_exit);

                // ya no tiene nada en ejecución
                w->qid_actual = -1;
                return;
            }
        }
        pthread_mutex_unlock(&mutex_cola_exec);
    }
}

void* atenderWorker(void* arg){
  t_conexionWorker* conexionWorker =  (t_conexionWorker*) arg; 

  int fd = conexionWorker ->fd;
  int id = conexionWorker ->id;
  while (1) {
      int codigoOperacion = recibir_operacion(fd); // Recibo la operacion del worker
      if(codigoOperacion <= 0){
        log_warning(loggerMaster, "Worker %d: desconectado", id);
        worker_desconectar_por_fd(fd);
        break;
      }  
        
      switch (codigoOperacion) {
            case RTA_DESALOJO: {
            t_list* p = recibir_paquete(fd);
            if (!p || list_size(p) < 2) {
                log_error(loggerMaster, "Worker %d: RTA_DESALOJO mal formado", conexionWorker->id);
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
                if (q->QCB->QID == qid) {
                    q->QCB->PC = pc;
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
            sem_post(&conexionWorker->semaforo);
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
            if (!p || list_size(p) < 3) {
                log_error(loggerMaster, "Worker %d: OP_READ mal formado", id);
                if (p) list_destroy_and_destroy_elements(p, free);
                break;
            }

            int qid = *(int*) list_get(p, 0);
            int n   = *(int*) list_get(p, 1);
            void* src = list_get(p, 2);

            void* buf = malloc(n);
            memcpy(buf, src, n);
            list_destroy_and_destroy_elements(p, free);

            // 1) Verificar que el QID esté en EXEC y recuperar fd del QC
            int fd_qc = -1;
            if (!exec_buscar_por_qid(qid, &fd_qc)) {
                log_warning(loggerMaster, "Worker %d: READ para QID=%d no encontrada en EXEC", id, qid);
                free(buf);
                break;
            }

            // 2) Forward al Query Control
            t_paquete* out = crear_paquete();
            out->codigo_operacion = MENSAJE_LECTURA;   // tu opcode hacia el QC
            agregar_a_paquete(out, &qid, sizeof(int));
            agregar_a_paquete(out, &n,   sizeof(int));
            agregar_a_paquete(out, buf,  n);
            enviar_paquete(out, fd_qc);
            eliminar_paquete(out);
            free(buf);

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
        return NULL;
    }
  }
