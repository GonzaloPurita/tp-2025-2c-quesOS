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
  while (1) {
    sem_wait(&hay_query_ready);

    pthread_mutex_lock(&mutex_cola_ready);
    t_query* query = list_is_empty(cola_ready) ? NULL : list_get(cola_ready, 0);
    pthread_mutex_unlock(&mutex_cola_ready);

    if (query == NULL) continue;

    t_conexionWorker* workerLibre = obtenerWorkerLibre();
    if (workerLibre != NULL) {
        pthread_mutex_lock(&mutex_cola_ready);
        list_remove_element(cola_ready, query);
        pthread_mutex_unlock(&mutex_cola_ready);

        pthread_mutex_lock(&mutex_cola_exec);
        list_add(cola_exec, query);
        pthread_mutex_unlock(&mutex_cola_exec);

        actualizarMetricas(Q_READY, Q_EXEC, query);
        enviarQueryAWorkerEspecifico(query, workerLibre);
        continue;
    }

    // No hay worker libre: buscar víctima
    t_query* candidatoDesalojo = buscarQueryConMenorPrioridad();
    if (candidatoDesalojo == NULL) {
        // reintento: tal vez entró un worker justo ahora
        workerLibre = obtenerWorkerLibre();
        if (workerLibre != NULL) {
            pthread_mutex_lock(&mutex_cola_ready);
            list_remove_element(cola_ready, query);
            pthread_mutex_unlock(&mutex_cola_ready);

            pthread_mutex_lock(&mutex_cola_exec);
            list_add(cola_exec, query);
            pthread_mutex_unlock(&mutex_cola_exec);

            actualizarMetricas(Q_READY, Q_EXEC, query);
            enviarQueryAWorkerEspecifico(query, workerLibre);
        } else {
            log_error(loggerMaster, "No hay queries en EXEC ni Workers libres");
        }
        continue;
    }

    int prioridadVictima = candidatoDesalojo ->prioridad_actual; // devuelve prioridad_actual (int)
    log_debug(loggerMaster,"Comparo para desalojar: NUEVA QID=%d (p=%d) vs VICTIMA QID=%d (p=%d)",query->QCB->qid, query->prioridad_actual, candidatoDesalojo->QCB->qid, prioridadVictima);

    // Prioridad más BAJA es MEJOR. Desalojar si la nueva es mejor que la víctima.
    if (query->prioridad_actual < prioridadVictima) {
        log_debug(loggerMaster, "Desalojo por prioridad");
        realizarDesalojo(candidatoDesalojo, query);
    }
    // Si no es mejor, no hacemos nada y esperamos próximo evento (aging, worker libre, otra query).
  }
}
void planificarSinDesalojo() {
  while(1) {
    sem_wait(&hay_query_ready); 
    sem_wait(&sem_workers_disponibles); 

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
    conexionWorker->qid_actual = query ->QCB->QID;// Le asigno el QID de la query a ejecutar.
    enviarQCB(conexionWorker ->fd, query ->QCB); // 
    pthread_t hiloWORKER;
    pthread_create(&hiloWORKER, NULL, atender_worker, (void*) conexionWorker); 
    pthread_detach(hiloWORKER);
}

void enviarQueryAWorkerEspecifico(t_query* query, t_conexionWorker* conexionWorker) {
    conexionWorker->qid_actual = query ->QCB->QID;// Le asigno el QID de la query a ejecutar.
    enviarQCB(conexionWorker ->fd, query ->QCB); // 
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

t_query* buscarQueryConMenorPrioridad() {
    t_query* victima = NULL;

    pthread_mutex_lock(&mutex_cola_exec);
    int n = list_size(cola_exec);
    for (int i = 0; i < n; i++) {
        t_query* q = list_get(cola_exec, i);
        if (victima == NULL || q->prioridad_actual > victima->prioridad_actual) {
            victima = q;
        } else if (victima && q->prioridad_actual == victima->prioridad_actual) {
            // desempate consistente: desalojar la "más nueva" (la última que veamos)
            victima = q;
        }
    }
    pthread_mutex_unlock(&mutex_cola_exec);

    return victima;
}

void realizarDesalojo(t_query* candidatoDesalojo, t_query* nuevoQuery) {
    // 1) Logs y métricas
    log_info(loggerMaster, "## (%d) - Query desalojada por Prioridades", candidatoDesalojo->QCB->qid);

    // 2) Movemos a las queries de colas
    pthread_mutex_lock(&mutex_cola_exec);
    list_remove_element(cola_exec, candidatoDesalojo);
    list_add(cola_exec, nuevoQuery);
    pthread_mutex_unlock(&mutex_cola_exec);

    pthread_mutex_lock(&mutex_cola_ready);
    list_remove_element(cola_ready, nuevoQuery);
    // reinsertar candidato en READY manteniendo orden por prioridad
    agregarAReadyPorPrioridad(candidatoDesalojo);
    pthread_mutex_unlock(&mutex_cola_ready);

    actualizarMetricas(Q_EXEC, Q_READY, candidatoDesalojo);
    actualizarMetricas(Q_READY, Q_EXEC, nuevoQuery);

    // 3) Preparamos el hilo para hacer el desalojo
    t_datos_desalojo* datos = malloc(sizeof(t_datos_desalojo));
    datos->candidatoDesalojo = candidatoDesalojo;
    datos->nuevoQuery = nuevoQuery;

    // Buscamos el worker donde corre el candidato 
    datos->worker = encontrarWorkerPorQid(candidatoDesalojo->QCB->qid);
    if (datos->worker == NULL) {
        log_error(loggerMaster, "No se encontró el Worker para la QID=%d", candidatoDesalojo->QCB->qid);
        free(datos);
        return;
    }

    // 4) Lanzar hilo que hace el PREEMPT bloqueante
    pthread_t hiloDesalojo;
    pthread_create(&hiloDesalojo, NULL, desalojar, (void*) datos);
    pthread_detach(hiloDesalojo);
}

void* desalojar(void* arg) {
    t_datos_desalojo* d = (t_datos_desalojo*) arg;

    // A) Pedir PREEMPT al worker del candidato
    t_paquete* p = crear_paquete();
    p->codigo_operacion = DESALOJO;
    agregar_a_paquete(p, &d->candidatoDesalojo->QCB->qid, sizeof(int));
    if (enviar_paquete(p, d->worker->fd) != 0) {
        log_error(loggerMaster, "Worker %d: fallo envío PREEMPT QID=%d",
                  d->worker->id, d->candidatoDesalojo->QCB->qid);
        eliminar_paquete(p);
        free(d);
        return NULL;
    }
    eliminar_paquete(p);

   // B) Esperar RTA_DESALOJO: el hilo atender_worker hará sem_post(&worker->semaforo)
    sem_wait(&d->worker->semaforo);

    // C) Enviar la NUEVA query a ese mismo worker
    enviarQueryAWorkerEspecifico(d->nuevoQuery, d->worker);

    free(d);
    return NULL;
}

