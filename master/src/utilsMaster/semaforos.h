#include <semaphore.h>
#include <pthread.h>

extern pthread_mutex_t mutex_workers; // Mutex para proteger la lista de workers
extern pthread_mutex_t mutex_qid;     // Mutex para proteger el acceso a los QIDs
extern pthread_mutex_t mutex_cola_ready; // Mutex para proteger la cola de queries listas
extern pthread_mutex_t mutex_cola_exec;  // Mutex para proteger la cola de queries en ejecución
extern pthread_mutex_t mutex_cola_exit;  // Mutex para proteger la cola de queries terminadas
extern pthread_mutex_t mutex_queries

void inicializarSemaforos();
void destruirSemaforos();

typedef enum { Q_READY, Q_EXEC, Q_EXIT } estado_query;
