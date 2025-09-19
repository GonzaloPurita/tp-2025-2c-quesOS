#include <semaphore.h>
#include <pthread.h>

extern pthread_mutex_t mutex_workers; // Mutex para proteger la lista de workers
extern pthread_mutex_t mutex_qid;     // Mutex para proteger el acceso a los QIDs

void inicializarSemaforos();
void destruirSemaforos();

typedef enum { Q_READY, Q_EXEC, Q_EXIT } estado_query;
