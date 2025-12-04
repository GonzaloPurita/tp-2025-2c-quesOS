#include "utilsPlanificacion.h"

int idTemporizador = 0;

void* aging(void* arg);

bool cmp_query_por_prioridad(void* _a, void* _b) {
    t_query* a = (t_query*) _a;
    t_query* b = (t_query*) _b;

    if (a->prioridad_actual != b->prioridad_actual)
        return a->prioridad_actual < b->prioridad_actual; // 0 es mejor
    return false;
}

void ordenar_ready(void) {
    pthread_mutex_lock(&mutex_cola_ready);
    list_sort(cola_ready, cmp_query_por_prioridad);
    pthread_mutex_unlock(&mutex_cola_ready);
}

void aplicar_aging_ready(void) {
    int hubo_cambios = 0;
    pthread_mutex_lock(&mutex_cola_ready);

    int n = list_size(cola_ready);
    for (int i = 0; i < n; i++) {
        t_query* q = list_get(cola_ready, i);

        // Envejecer solo si todavía no se aplicó en este tick
        if (q->IDAging < AGING_TICK_GLOBAL && q->prioridad_actual > 0) {
            int anterior = q->prioridad_actual;
            q->prioridad_actual = anterior - 1; // 0 es mejor
            q->IDAging = AGING_TICK_GLOBAL;
            hubo_cambios = 1;
            log_info(loggerMaster, "Cambio de prioridad de Query: “##<%d> Cambio de prioridad: <%d> - <%d>", q->QCB->QID, anterior, q->prioridad_actual);
        }
    }

    if (hubo_cambios) {
        //TODO: chequear que respete orden de llegada entre iguales y sino cambiar por list_add_index
        list_sort(cola_ready, cmp_query_por_prioridad);
        sem_post(&rePlanificar);
    }

    pthread_mutex_unlock(&mutex_cola_ready);
}

void* hilo_aging(void* arg) {
    const int ms = configMaster->tiempo_aging;
    if (ms <= 0) return NULL;

    while(1) {
        usleep(ms * 1000);
        AGING_TICK_GLOBAL++;
        aplicar_aging_ready();
    }
    return NULL;
}

void iniciarAging(t_query* q){
    if(!hayAging) return;
    t_info_aging* info = malloc(sizeof(t_info_aging));

    pthread_mutex_lock(&mutex_idTemporizador);
    q->idTemporizador = idTemporizador++;
    info->idTemporizador = q->idTemporizador;
    pthread_mutex_unlock(&mutex_idTemporizador);
    info->query = q;

    pthread_t hilo;
    pthread_create(&hilo, NULL, aging, info);
    pthread_detach(hilo);
}

void* aging(void* arg){
    t_info_aging* info = (t_info_aging*) arg;
    t_query* q = info->query;
    int id = info->idTemporizador;
    free(info);
    const int ms = configMaster->tiempo_aging;
    if (ms <= 0) return NULL;

    usleep(ms * 1000);
    if(q == NULL) return NULL;
    if(id == q->idTemporizador) {
        if(q->prioridad_actual > 0) {
            int anterior = q->prioridad_actual;
            q->prioridad_actual = anterior - 1; 

            log_info(loggerMaster, "Cambio de prioridad de Query: “##<%d> Cambio de prioridad: <%d> - <%d>", q->QCB->QID, anterior, q->prioridad_actual);

            pthread_mutex_lock(&mutex_cola_ready);
            list_remove_element(cola_ready, q);
            agregarAReadyPorPrioridad(q);
            pthread_mutex_unlock(&mutex_cola_ready);
            if(q->prioridad_actual > 0){
                iniciarAging(q);
            }
        }
    }
    return NULL;
}