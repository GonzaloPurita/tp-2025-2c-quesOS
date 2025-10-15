#include <planificador.h>

void* planificador(void* arg) {
  if(strcmp(configMaster->algoritmo_planificacion, "FIFO") == 0) {
    planificarSinDesalojo() ;
  }
  else {
    planificarConDesalojoYAging();
  }

  return NULL;
}

void planificarConDesalojoYAging() {
  while(1) {
    sem_wait(&hay_query_ready); 

    // Consigo el proceso con menor prioridad (se agregan a ready ordenados por prioridad)
    pthread_mutex_lock(&cola_ready);
    t_query* query = NULL;
    if(!list_is_empty(cola_ready)) { 
      query = list_get(cola_ready, 0);
    }
    pthread_mutex_unlock(&cola_ready);

    if(query != NULL) { 
      t_conexionWorker* workerLibre = obtenerWorkerLibre();
      if(worker != NULL) { // -> NO hace falta desalojar
        pthread_mutex_lock(&cola_ready); 
        list_remove_element(cola_ready, query); 
        pthread_mutex_unlock(&cola_ready); 
        pthread_mutex_lock(&cola_exec); 
        list_add(cola_exec, query); // Agrego la query a la cola de EXEC
        pthread_mutex_unlock(&cola_exec); 
        actualizarMetricas(Q_READY, Q_EXEC, query);
        enviarQueryAWorkerEspecifico(query, workerLibre); 
      }
      else { // No hay una CPU libre --> Tengo que intentar desalojar.
        // Consigo el proceso con la menor prioridad
        pthread_mutex_lock(&mutex_cola_exec);
        t_query* candidatoDesalojo = buscarQueryConMenorPrioridad(); //TODO LA FUNCION

        if(candidatoDesalojo == NULL) { 
          workerLibre = obtenerWorkerLibre(); // Vuelvo a buscar un Worker libre
          if(workerLibre != NULL) { // Si había una CPU libre, no hay que desalojar.
            pthread_mutex_lock(&mutex_cola_ready); // Bloqueo el mutex para la cola de READY
            list_remove_element(cola_ready, query); // Elimino el nuevo proceso de la cola de READY
            pthread_mutex_unlock(&mutex_cola_ready); // Desbloqueo el mutex
            list_add(cola_exec, query); // Agrego el proceso a la cola de EXEC
            pthread_mutex_unlock(&mutex_cola_exec); // Desbloqueo el mutex
            actualizarMetricas(Q_READY, Q_EXEC, query);
            enviarQueryWorkerEspecifico(query, workerLibre); // Envio el proceso a esa CPU en particular.
          }
          else {
            log_error(loggerMaster, "No hay queries en EXEC, ni CPU libres");
            pthread_mutex_unlock(&cola_exec); // Desbloqueo el mutex
          }
        }
        else { // Si encontre un proceso.
          pthread_mutex_unlock(&cola_exec);
          int64_t prioridadRestante = calcularPrioridadRestante(candidatoDesalojo); //TODO LA FUNCION
          log_debug(loggerMaster,"Estoy comparando para desalojar: Nuevo con QID: %d con prioridad: %" PRId64" y en ejecución QID: %d con prioridad: %" PRId64,
          query->QCB->PID, query->prioridad , candidatoDesalojo->QCB->PID, prioridadRestante);

          if(nuevoProceso->prioridad > prioridadRestante) { // TENGO QUE DESALOJAR --> Es bloqueante --> Se hace en otro hilo
            log_debug(loggerMaster, "Voy a desalojar");
            realizarDesalojo(candidatoDesalojo, query); // Realizo el desalojo //TODO LA FUNCION
          }
        }
      }
    }
  }
}
void planificarSinDesalojo() {
  while(1) {
    sem_wait(&hay_query_ready); 
    sem_wait(&workers_disponibles); 

    // Saco la primer query de la cola de READY
    pthread_mutex_lock(&mutex_cola_ready); 
    t_query* query = list_remove(cola_ready, 0); 
    pthread_mutex_unlock(&mutex_cola_ready); 

    pthread_mutex_lock(&cola_exec); 
    list_add(cola_exec, query); 
    pthread_mutex_unlock(&cola_exec); 

    actualizarMetricas(Q_READY, Q_EXEC, query);

    enviarQueryAWorker(query);

}
}

void enviarQueryAWorker(t_query* query) {
    t_conexionWorker* conexionWorker = obtenerWorkerLibre(); // Obtengo un worker libre
    conexionWorker->// Le asigno el QID de la query a ejecutar.
    enviarQCB // 
    pthread_t hiloWORKER;
    pthread_create(&hiloWORKER, NULL, atender_worker, (void*) conexionWorker); 
    pthread_detach(hiloWORKER);
}

t_conexionWorker* obtenerWorkerLibre() {
    pthread_mutex_lock(&mutex_workers);
    t_conexionWorker* conexionWorker = list_remove(LISTA_WORKERS, 0); // Saco el primer worker de la lista de workers libres
    pthread_mutex_unlock(&mutex_workers);
    return conexionWorker;
}

void enviarProcesoWorkerEspecifico(t_query* query, t_conexionWorker* worker) {
    worker->conectado = true; 
    worker->qid_actual = query->QCB->QID; 
    enviarQCB(worker->fd, query->qcb); 
    pthread_t hiloWorker;
    pthread_create(&hiloWorker, NULL, atender_worker, (void*) worker); 
    pthread_detach(hiloWorker); 
  }

bool cmp_query_por_prioridad(void* _a, void* _b) {
    t_query* a = (t_query*) _a;
    t_query* b = (t_query*) _b;

    if (a->prioridad_actual != b->prioridad_actual)
        return a->prioridad_actual < b->prioridad_actual; // 0 es mejor
    return a->ts_llegada < b->ts_llegada; // más vieja primero
}

void ordenar_ready(void) {
    pthread_mutex_lock(&mutex_cola_ready);
    list_sort(cola_ready, cmp_query_por_prioridad);
    pthread_mutex_unlock(&mutex_cola_ready);
}

ChatGPT dijo:
Pensado durante 28s

Sí, comparar “a partir de la query que quiere entrar” tiene más sentido que coronar una reina del drama en EXEC y recién ahí mirar qué llega. Te lo dejo adaptado a tu estructura, sin inventos raros, con nombres decentes y sin mover estados antes de recibir el PC.

Helpers mínimos
// Devuelve true si EXISTE alguien en EXEC a quien valga la pena desalojar
// para que entre una query con prioridad_nueva. Prioridad baja = mejor.
// Si hay varias peores, elige la peor; en empate, la "más nueva" (última).
static bool exec_encontrar_victima_para(int prioridad_nueva, t_query** out_victima, int* out_worker_id) {
    bool hallado = false;
    t_query* victima = NULL;
    int worker_victima = -1;

    pthread_mutex_lock(&mutex_cola_exec);
    int n = list_size(cola_exec);
    for (int i = 0; i < n; i++) {
        t_query* q = list_get(cola_exec, i);
        // Solo tiene sentido desalojar si la nueva es MEJOR (número menor) que la que está
        if (prioridad_nueva < q->prioridad_actual) {
            if (!hallado) {
                victima = q;
                worker_victima = worker_id_por_qid(q->QCB->qid); // tu helper existente
                hallado = true;
            } else {
                // Elegir la PEOR prioridad (número mayor); empate: la última que veamos
                if (q->prioridad_actual > victima->prioridad_actual) {
                    victima = q;
                    worker_victima = worker_id_por_qid(q->QCB->qid);
                } else if (q->prioridad_actual == victima->prioridad_actual) {
                    victima = q; // recorremos left→right; la última es la más "nueva" en EXEC
                    worker_victima = worker_id_por_qid(q->QCB->qid);
                }
            }
        }
    }
    pthread_mutex_unlock(&mutex_cola_exec);

    if (hallado) {
        *out_victima = victima;
        *out_worker_id = worker_victima;
    }
    return hallado;
}

typedef struct { int worker_id; int qid; } t_preempt_args;

void* hilo_enviar_preempt(void* arg) {
    t_preempt_args* a = (t_preempt_args*) arg;
    t_paquete* p = crear_paquete();
    p->codigo_operacion = OP_PREEMPT;
    agregar_a_paquete(p, &a->qid, sizeof(int));
    int wfd = worker_fd(a->worker_id);
    if (enviar_paquete(p, wfd) != 0) {
        log_error(loggerMaster, "Worker %d: no se pudo enviar PREEMPT de QID=%d", a->worker_id, a->qid);
    }
    eliminar_paquete(p);
    free(a);
    return NULL;
}

void lanzar_preempt_async(int worker_id, int qid) {
    t_preempt_args* a = malloc(sizeof(*a));
    a->worker_id = worker_id;
    a->qid = qid;
    pthread_t th;
    pthread_create(&th, NULL, hilo_enviar_preempt, a);
    pthread_detach(th);
}

