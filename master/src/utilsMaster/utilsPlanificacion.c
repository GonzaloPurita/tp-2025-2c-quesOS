#include "utilsPlanificacion.h"

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
        if (q->IDAging < AGING_TICK_GLOBAL) {
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