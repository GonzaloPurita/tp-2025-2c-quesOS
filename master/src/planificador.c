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
        pthread_mutex_lock(&cola_exec);
        t_query* candidatoDesalojo = buscarQueryConMenorPrioridad(); //TODO LA FUNCION

        if(candidatoDesalojo == NULL) { 
          workerLibre = obtenerWorkerLibre(); // Vuelvo a buscar un Worker libre
          if(workerLibre != NULL) { // Si había una CPU libre, no hay que desalojar.
            pthread_mutex_lock(&cola_ready); // Bloqueo el mutex para la cola de READY
            list_remove_element(cola_ready, query); // Elimino el nuevo proceso de la cola de READY
            pthread_mutex_unlock(&cola_ready); // Desbloqueo el mutex
            list_add(cola_exec, query); // Agrego el proceso a la cola de EXEC
            pthread_mutex_unlock(&cola_exec); // Desbloqueo el mutex
            actualizarMetricas(Q_READY, Q_EXEC, query);
            enviarQueryWorkerEspecifico(query, workerLibre); // Envio el proceso a esa CPU en particular.
          }
          else {
            log_error(loggerKernel, "No hay queries en EXEC, ni CPU libres");
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
    enviarQCB // Envio el QCB a la CPU
    pthread_t hiloWORKER;
    pthread_create(&hiloWORKER, NULL, atender_worker, (void*) conexionWorker); // Creo un hilo para atender la CPU a la que le envio el PCB.
    pthread_detach(hiloWORKER);
}

t_conexionWorker* obtenerWorkerLibre() {
    pthread_mutex_lock(&mutex_workers);
    t_conexionWorker* conexionWorker = list_remove(LISTA_WORKERS, 0); // Saco el primer worker de la lista de workers libres
    pthread_mutex_unlock(&mutex_workers);
    return conexionWorker;
}

void enviarProcesoWorkerEspecifico(t_query* query, t_conexionWorker* worker) {
    worker->conectado = true; // La CPU ya no esta libre, porque le voy a enviar un proceso.
    worker->qid_actual = query->QCB->QID; // Le asigno el PID del proceso que va a ejecutar.
    enviarQCB(worker->fd, query->qcb); // Envio el PCB a la CPU
    pthread_t hiloWorker;
    pthread_create(&hiloWorker, NULL, atender_worker, (void*) worker); // Creo un hilo para atender la CPU a la que le envio el PCB.
    pthread_detach(hiloWorker); 
  }

