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

int worker_registrar(char* id, int fd) {
    if (!id) return 0;

    t_conexionWorker* w = malloc(sizeof(*w));
    if (!w) {
        log_error(loggerMaster, "worker_registrar: malloc fallo");
        return 0;
    }

    char* id_copy = strdup(id);
    if (!id_copy) {
        log_error(loggerMaster, "worker_registrar: strdup fallo");
        free(w);
        return 0;
    }

    w->id = id_copy;
    w->fd = fd;
    w->conectado = true;
    w->qid_actual = -1;

    if (sem_init(&w->semaforo, 0, 0) != 0) {
        log_error(loggerMaster, "worker_registrar: sem_init fallo para worker %s", id_copy);
        free(w->id);
        free(w);
        return 0;
    }

    pthread_mutex_lock(&mutex_workers);
    if (!LISTA_WORKERS) {
        LISTA_WORKERS = list_create();
        if (!LISTA_WORKERS) {
            log_error(loggerMaster, "worker_registrar: list_create fallo");
            pthread_mutex_unlock(&mutex_workers);
            sem_destroy(&w->semaforo);
            free(w->id);
            free(w);
            return 0;
        }
    }
    list_add(LISTA_WORKERS, w);
    int list_size_now = list_size(LISTA_WORKERS);
    pthread_mutex_unlock(&mutex_workers);

    return 1;
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
   int n = 0;
    pthread_mutex_lock(&mutex_workers);
    if (LISTA_WORKERS != NULL) {
        n = list_size(LISTA_WORKERS);
    }
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
    pthread_mutex_lock(&mutex_workers);
    t_conexionWorker* w = NULL;
    for (int i = 0; i < list_size(LISTA_WORKERS); i++) {
        t_conexionWorker* it = list_get(LISTA_WORKERS, i);
        if (it->fd == fd) { w = it; break; }
    }
    pthread_mutex_unlock(&mutex_workers);

    if (!w) return;

    int qid_en_ejecucion = w->qid_actual;  
    w->conectado = false;
    close(fd);

    int total_workers = workers_conectados();

    if (qid_en_ejecucion != -1) {
        // sacar la query de EXEC
        pthread_mutex_lock(&mutex_cola_exec);
        t_query* q = NULL;
        for (int i = 0; i < list_size(cola_exec); i++) {
            t_query* it = list_get(cola_exec, i);
            if (it->QCB->QID == qid_en_ejecucion) {
                q = it;
                list_remove(cola_exec, i);
                break;
            }
        }
        pthread_mutex_unlock(&mutex_cola_exec);

        if (q) {
            // log completo EXACTO como lo querés
            log_info(loggerMaster,
                "## Se desconecta el Worker %s - Se finaliza la Query %d - Cantidad total de Workers: %d",
                w->id,
                qid_en_ejecucion,
                total_workers
            );

            // notificar al Query Control
            t_paquete* paquete = crear_paquete();
            paquete->codigo_operacion = QUERY_FINALIZADA;
            agregar_a_paquete(paquete, &qid_en_ejecucion, sizeof(int));
            enviar_paquete(paquete, q->fd_qc);
            eliminar_paquete(paquete);

            actualizarMetricas(Q_EXEC, Q_EXIT, q);

            pthread_mutex_lock(&mutex_cola_exit);
            list_add(cola_exit, q);
            pthread_mutex_unlock(&mutex_cola_exit);
        }
    } else {
        // Worker no tenía query en ejecución
        log_info(loggerMaster,
            "## Se desconecta el Worker %s - No tenía query asignada - Cantidad total de Workers: %d",
            w->id,
            total_workers
        );
    }

    w->qid_actual = -1;
}

t_conexionWorker* worker_por_fd(int fd) {
    pthread_mutex_lock(&mutex_workers);
    t_conexionWorker* w = NULL;
    for (int i = 0; i < list_size(LISTA_WORKERS); i++) {
        t_conexionWorker* it = list_get(LISTA_WORKERS, i);
        if (it->fd == fd) { w = it; break; }
    }
    pthread_mutex_unlock(&mutex_workers);
    return w;
}

void* atenderWorker(void* arg){
  t_conexionWorker* conexionWorker =  (t_conexionWorker*) arg; 

  int fd = conexionWorker ->fd;
  char* id = conexionWorker ->id;
  while (1) {
      int codigoOperacion = recibir_operacion(fd); // Recibo la operacion del worker
      if(codigoOperacion <= 0){
        worker_desconectar_por_fd(fd);
        break;
      }  
        
      switch (codigoOperacion) {
            case RTA_DESALOJO: {
            t_list* p = recibir_paquete(fd);
            if (!p || list_size(p) < 2) {
                log_error(loggerMaster, "Worker %s: RTA_DESALOJO mal formado", conexionWorker->id);
                if (p) list_destroy_and_destroy_elements(p, free);
                break;
            }
            int qid = *(int*) list_get(p, 0);
            int pc  = *(int*) list_get(p, 1);
            list_destroy_and_destroy_elements(p, free);

            // 1) Actualizar PC en la query víctima (ya la movimos a READY en realizarDesalojo)
            int actualizado = 0;
            pthread_mutex_lock(&mutex_cola_exec);
            for (int i = 0; i < list_size(cola_exec); i++) {
                t_query* q = list_get(cola_exec, i);
                if (q->QCB->QID == qid) {
                    q->QCB->PC = pc;
                    actualizado = 1;
                    break;
                }
            }
            pthread_mutex_unlock(&mutex_cola_exec);

            if (!actualizado) {
                // Si no está en READY puede ser carrera rara: log y seguimos 
                log_warning(loggerMaster, "RTA_DESALOJO QID=%d no encontrada en READY para actualizar PC=%d", qid, pc);
            } else {
                log_info(loggerMaster, "## (%d) - PC actualizado por desalojo a %d", qid, pc);
            }

            // 2) Despertar al hilo desalojar() para que envíe la nueva query
            sem_post(&conexionWorker->semaforo);
            return NULL;
        }
        case OP_END: {
            // 1) Recibir el paquete del Worker
            t_list* p = recibir_paquete(fd);
            if (list_size(p) < 1) {
                log_error(loggerMaster, "Worker %s: OP_END mal formado", id);
                list_destroy_and_destroy_elements(p, free);
                break;
            }
            
            int qid = *(int*) list_get(p, 0);
            list_destroy_and_destroy_elements(p, free);

            // 2) Buscar y sacar de EXEC -> EXIT
            pthread_mutex_lock(&mutex_cola_exec);
            t_query* q = NULL;
            for (int i = 0; i < list_size(cola_exec); i++) {
                t_query* it = list_get(cola_exec, i);
                if (it->QCB->QID == qid) {
                    q = it;
                    list_remove(cola_exec, i);
                    break;
                }
            }
            pthread_mutex_unlock(&mutex_cola_exec);

            if (!q) {
                log_warning(loggerMaster, "OP_END: QID=%d no estaba en EXEC", qid);
                break;
            }

            // 3) Guardar fd_qc antes de pasar la query a exit
            int fd_qc = q->fd_qc;

            // 4) Movemos la query a exit
            actualizarMetricas(Q_EXEC, Q_EXIT, q);
            pthread_mutex_lock(&mutex_cola_exit);
            list_add(cola_exit, q);
            pthread_mutex_unlock(&mutex_cola_exit);

            // 5) Marcamos el Worker como libre
            worker_marcar_libre_por_fd(fd);
            sem_post(&sem_workers_disponibles);

            // 6) Enviar respuesta al Query Control
            t_paquete* r = crear_paquete();
            r->codigo_operacion = QUERY_FINALIZADA;
            agregar_a_paquete(r, &qid, sizeof(int));
            enviar_paquete(r, fd_qc);
            eliminar_paquete(r);
            log_info(loggerMaster, "## Se terminó la Query <%d> en el Worker <%s>", qid, id);
            return NULL;
        }
        case OP_READ: {
            // payload desde Worker: [qid:int][n:int][bytes:n][tag:char*]
            t_list* p = recibir_paquete(fd);
            if (!p || list_size(p) < 4) {
                log_error(loggerMaster, "Worker %s: OP_READ mal formado", id);
                if (p) list_destroy_and_destroy_elements(p, free);
                break;
            }

            int   qid = *(int*) list_get(p, 0);
            int   n   = *(int*) list_get(p, 1);
            void* src =        list_get(p, 2);
            char* tag_src =    list_get(p, 3);

            // copio el buffer de datos
            void* buf = malloc(n);
            memcpy(buf, src, n);

            // copio la tag para no usar memoria liberada
            char* tag = strdup(tag_src);

            list_destroy_and_destroy_elements(p, free);

            // 1) Verificar que el QID esté en EXEC y recuperar fd del QC
            int fd_qc = -1;
            if (!exec_buscar_por_qid(qid, &fd_qc)) {
                log_warning(loggerMaster,
                            "Worker %s: READ para QID=%d no encontrada en EXEC", id, qid);
                free(buf);
                free(tag);
                break;
            }

            // 2) Forward al Query Control: [qid:int][n:int][bytes:n][tag:char*]
            t_paquete* out = crear_paquete();
            out->codigo_operacion = MENSAJE_LECTURA;
            agregar_a_paquete(out, &qid, sizeof(int));
            agregar_a_paquete(out, &n,   sizeof(int));
            agregar_a_paquete(out, buf,  n);
            agregar_a_paquete(out, tag,  strlen(tag) + 1);

            enviar_paquete(out, fd_qc);
            log_info(loggerMaster,"## Se envía un mensaje de lectura de la Query <%d> en el worker <%s> al Query Control", qid, id);
            eliminar_paquete(out);

            free(buf);
            free(tag);
            break;
        }

        case OP_WRITE: {
            // payload: [valor:char*]
            t_list* p = recibir_paquete(fd);
            if (p && list_size(p) >= 1) {
                char* valor = list_get(p, 0);
                log_info(loggerMaster, "## QID=%d: WRITE consumado (%s)", conexionWorker->qid_actual, valor);
            } else {
                log_error(loggerMaster, "Worker %s: OP_WRITE mal formado", id);
            }
            if (p) list_destroy_and_destroy_elements(p, free);
            break;
        }

        default:
            log_warning(loggerMaster, "Worker %s: op_code desconocido %d", id, codigoOperacion);
            t_list* paquete = recibir_paquete(fd);
            list_destroy_and_destroy_elements(paquete,free);
            break;
        }
    }
    return NULL;
  }
