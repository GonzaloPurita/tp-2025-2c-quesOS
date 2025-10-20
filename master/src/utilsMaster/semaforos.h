#ifndef SEMAFOROS_H_
#define SEMAFOROS_H_

#include <semaphore.h>
#include <pthread.h>

extern pthread_mutex_t mutex_workers; // Mutex para proteger la lista de workers
extern pthread_mutex_t mutex_qid;     // Mutex para proteger el acceso a los QIDs
extern pthread_mutex_t mutex_cola_ready; // Mutex para proteger la cola de queries listas
extern pthread_mutex_t mutex_cola_exec;  // Mutex para proteger la cola de queries en ejecución
extern pthread_mutex_t mutex_cola_exit;  // Mutex para proteger la cola de queries terminadas
extern pthread_mutex_t mutex_queries;

extern sem_t workers_disponibles; // Semáforo para contar los workers disponibles
extern sem_t hay_query_ready;     // Semáforo para contar las queries en estado READY


void inicializarSemaforos();
void destruirSemaforos();


#endif /* SEMAFOROS_H_ */
