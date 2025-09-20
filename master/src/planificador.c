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