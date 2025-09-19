#include <semaforos.h>

// --- MUTEX ---
pthread_mutex_t mutex_workers;
pthread_mutex_t mutex_qid;

void inicializarSemaforos(void) {
    pthread_mutex_init(&mutex_workers, NULL);
    pthread_mutex_init(&mutex_qid, NULL);
}