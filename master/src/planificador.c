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

void planificador_lanzar(void) {
    pthread_t th_planificador;
    pthread_create(&th_planificador, NULL, planificador, NULL);
    pthread_detach(th_planificador);
    log_debug(loggerMaster, "Hilo planificador iniciado");
}

void planificarConDesalojoYAging() {
  while (1) {
    log_debug(loggerMaster, "Planificador con desalojo esperando...");
    sem_wait(&rePlanificar);
    log_debug(loggerMaster, "Planificador con desalojo activado");

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

    //? Me parece innecessario
    // if (candidatoDesalojo == NULL) {
    //     reintento: tal vez entró un worker justo ahora
    //     workerLibre = obtenerWorkerLibre();
    //     if (workerLibre != NULL) {
    //         pthread_mutex_lock(&mutex_cola_ready);
    //         list_remove_element(cola_ready, query);
    //         pthread_mutex_unlock(&mutex_cola_ready);

    //         pthread_mutex_lock(&mutex_cola_exec);
    //         list_add(cola_exec, query);
    //         pthread_mutex_unlock(&mutex_cola_exec);

    //         actualizarMetricas(Q_READY, Q_EXEC, query);
    //         enviarQueryAWorkerEspecifico(query, workerLibre);
    //     } else {
    //          log_error(loggerMaster, "No hay queries en EXEC ni Workers libres");
    //     }
    //     continue;
    // }

    int prioridadVictima = candidatoDesalojo->prioridad_actual; // devuelve prioridad_actual (int)
    log_debug(loggerMaster,"Comparo para desalojar: NUEVA QID=%d (p=%d) vs VICTIMA QID=%d (p=%d)",query->QCB->QID, query->prioridad_actual, candidatoDesalojo->QCB->QID, prioridadVictima);

    // Prioridad más BAJA es MEJOR. Desalojar si la nueva es mejor que la víctima.
    if (query->prioridad_actual < prioridadVictima) {
        log_debug(loggerMaster, "Desalojo por prioridad");
        realizarDesalojo(candidatoDesalojo, query);
    }  else {
        // La nueva query no tiene mayor prioridad, queda en ready
        log_debug(loggerMaster, "Query %d (p=%d) no desaloja a %d (p=%d), se queda en READY", query->QCB->QID, query->prioridad_actual, candidatoDesalojo->QCB->QID, prioridadVictima);
    }

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

    pthread_mutex_lock(&mutex_cola_exec); 
    list_add(cola_exec, query); 
    pthread_mutex_unlock(&mutex_cola_exec); 

    actualizarMetricas(Q_READY, Q_EXEC, query);

    enviarQueryAWorker(query);

}
}

void enviarQueryAWorker(t_query* query) {
    t_conexionWorker* conexionWorker = obtenerWorkerLibre();
    if (!conexionWorker) {
        log_error(loggerMaster, "No hay workers libres!");
        return;
    }
    pthread_mutex_lock(&mutex_workers);
    conexionWorker->qid_actual = query->QCB->QID;
    pthread_mutex_unlock(&mutex_workers);
    log_info(loggerMaster, "## Se envía la Query <%d> (<%d>) al worker <%s>", 
             query->QCB->QID, query->prioridad_actual, conexionWorker->id);
    
    enviarQCB(conexionWorker->fd, query->QCB, query->path);
    sem_post(&conexionWorker->esperarQuery);
}

void enviarQueryAWorkerEspecifico(t_query* query, t_conexionWorker* conexionWorker) {
    pthread_mutex_lock(&mutex_workers);
    conexionWorker->qid_actual = query->QCB->QID;
    pthread_mutex_unlock(&mutex_workers);

    log_info(loggerMaster, "## Se envía la Query <%d> (<%d>) al worker <%s>", query->QCB->QID, query->prioridad_actual, conexionWorker->id);

    enviarQCB(conexionWorker->fd, query->QCB, query->path); 
    sem_post(&conexionWorker->esperarQuery);
}

t_conexionWorker* obtenerWorkerLibre() {
    pthread_mutex_lock(&mutex_workers);
    t_conexionWorker* w = NULL;
    for (int i = 0; i < list_size(LISTA_WORKERS); i++) {
        t_conexionWorker* it = list_get(LISTA_WORKERS, i);
        if (it->conectado && it->qid_actual == -1) { w = it; break; }
    }
    pthread_mutex_unlock(&mutex_workers);
    log_debug(loggerMaster, "Worker libre: %s", w ? w->id : "Ninguno");
    return w;
}

t_query* buscarQueryConMenorPrioridad() {
    t_query* victima = NULL;

    pthread_mutex_lock(&mutex_cola_exec);
    int n = list_size(cola_exec);

    log_debug(loggerMaster, "Buscando víctima entre %d queries en EXEC", n);

    t_query* q1 = list_get(cola_exec, 0);
    t_query* q2 = list_get(cola_exec, 1);

    log_debug(loggerMaster, "Comparando primeras dos queries en EXEC: QID=%d (p=%d) vs QID=%d (p=%d)", q1->QCB->QID, q1->prioridad_actual, q2->QCB->QID, q2->prioridad_actual);

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
    log_info(loggerMaster, "## (%d) - Query desalojada por Prioridades", candidatoDesalojo->QCB->QID);

    // 2) Preparar datos para el desalojo
    t_datos_desalojo* datos = malloc(sizeof(t_datos_desalojo));
    datos->candidatoDesalojo = candidatoDesalojo;
    datos->nuevoQuery = nuevoQuery;

    // 3) Buscar el worker que está ejecutando la víctima
    datos->worker = encontrarWorkerPorQid(candidatoDesalojo->QCB->QID);

    log_debug(loggerMaster, "Worker encontrado para desalojo: %s", datos->worker ? datos->worker->id : "Ninguno");

    if (datos->worker == NULL) {
        log_error(loggerMaster, "No se encontró el Worker para la QID=%d", candidatoDesalojo->QCB->QID);
        free(datos);
        return;
    }
    
    log_info(loggerMaster, "## Se desaloja la Query %d (p=%d) del Worker %s (fd=%d) - Motivo: PRIORIDAD",
    candidatoDesalojo->QCB->QID,
    candidatoDesalojo->prioridad_actual,
    datos->worker->id,
    datos->worker->fd);

    desalojar((void*) datos);
    // 4) Lanzar hilo que hace el desalojo
    // pthread_t hiloDesalojo;
    // pthread_create(&hiloDesalojo, NULL, desalojar, (void*) datos);
    // pthread_detach(hiloDesalojo);
}

void* desalojar(void* arg) {
    t_datos_desalojo* d = (t_datos_desalojo*) arg;

    // 1) Pedimos el preempt al worker
    t_paquete* p = crear_paquete();
    p->codigo_operacion = DESALOJO;
    agregar_a_paquete(p, &d->candidatoDesalojo->QCB->QID, sizeof(int));
    enviar_paquete(p, d->worker->fd);
    eliminar_paquete(p);

    // 2) esperamos la respuesta (mandamos el post desde el hilo que atiende al worker)
    sem_wait(&d->worker->semaforo);

    if(!d->worker->conectado) { // Validamos que no se haya desconectado durante el desalojo.
        log_error(loggerMaster, "Desalojo abortado: Worker %s desconectado", d->worker->id);
        return NULL;
    }

   // 3) Movemos las queries entre colas
    pthread_mutex_lock(&mutex_cola_exec);
    list_remove_element(cola_exec, d->candidatoDesalojo);
    list_add(cola_exec, d->nuevoQuery);
    pthread_mutex_unlock(&mutex_cola_exec);
    
    pthread_mutex_lock(&mutex_cola_ready);
    list_remove_element(cola_ready, d->nuevoQuery);
    agregarAReadyPorPrioridad(d->candidatoDesalojo);
    pthread_mutex_unlock(&mutex_cola_ready);

    actualizarMetricas(Q_EXEC, Q_READY, d->candidatoDesalojo);
    actualizarMetricas(Q_READY, Q_EXEC, d->nuevoQuery);
    
    // 4) Mandamos la query nueva a ese worker
    log_debug(loggerMaster, "Enviando nueva QID=%d al Worker %s (fd=%d) tras desalojo", d->nuevoQuery->QCB->QID, d->worker->id, d->worker->fd);
    enviarQueryAWorkerEspecifico(d->nuevoQuery, d->worker);

    free(d);
    return NULL;
}

