#include <semaforos.h>

// --- Binarios ---
sem_t workers_disponibles; // Contador de workers libres
sem_t hay query_ready; // Contador de queries en READY

// --- MUTEX ---
pthread_mutex_t mutex_workers;
pthread_mutex_t mutex_qid;
pthread_mutex_t mutex_cola_ready;
pthread_mutex_t mutex_cola_exec;
pthread_mutex_t mutex_cola_exit;
pthread_mutex_t mutex_queries;

void inicializarSemaforos(void) {
    pthread_mutex_init(&mutex_workers, NULL);
    pthread_mutex_init(&mutex_qid, NULL);
    pthread_mutex_init(&mutex_cola_ready, NULL);
    pthread_mutex_init(&mutex_cola_exec, NULL);
    pthread_mutex_init(&mutex_cola_exit, NULL);
    pthread_mutex_init (&mutex_queries, NULL);

    sem_init(&workers_disponibles, 0, 0);
    sem_init(&hay_query_ready, 0, 0);
}