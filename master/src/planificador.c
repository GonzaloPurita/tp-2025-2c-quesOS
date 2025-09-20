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

pthread_mutex_t* obtenerMutexPorEstado(char* estado) {
    if (strcmp(estado, "READY") == 0) {
      return &mutex_cola_ready;
  } else if (strcmp(estado, "EXEC") == 0) {
      return &mutex_cola_exec;
  } else if (strcmp(estado, "EXIT") == 0) {
      return &mutex_cola_exit;
  } else {
      log_error(loggerKernel, "Estado desconocido: %s", estado);
      return NULL; // Devuelve NULL si el estado no es válido
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

}