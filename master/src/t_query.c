#include "t_query.h"

t_query* crearQuery(const char* path, int prioridad) {
    pthread_mutex_lock(&mutex_queries);

    t_query* q = malloc(sizeof(t_query));
    if (!q) { log_error(loggerMaster, "Error al crear la query"); pthread_mutex_unlock(&mutex_queries); return NULL; }

    q->QCB = crearQCB();
    if (!q->QCB) { log_error(loggerMaster, "Error al crear QCB"); free(q); pthread_mutex_unlock(&mutex_queries); return NULL; }

    q->path = strdup(path);
    q->prioridad = prioridad;
    q->prioridad_actual = prioridad;

    q->IDAging = AGING_TICK_GLOBAL;  // clave para no envejecer YA
    q->pc_actual = 0;

    q->estado = Q_READY;

    pthread_mutex_unlock(&mutex_queries);
    return q;
}

void destruirQuery(t_query* q) {
    if (!q) return;
    if (q->QCB) destruirQCB(q->QCB);
    free(q->path);
    free(q);
}

void actualizarMetricas(estado_query estadoActual, estado_query estadoNuevo, t_query* q) {
    // timestamps básicos y contadores útiles
    if (estadoActual == Q_READY && estadoNuevo == Q_EXEC) q->ts_inicio_exec = now_ms();
    if (estadoActual == Q_EXEC && estadoNuevo == Q_READY) { q->desalojos++; q->ts_ultimo_aging = now_ms(); }
    q->estado = estadoNuevo;
}

void agregarAReadyPorPrioridad(t_query* q){
    pthread_mutex_lock(&mutex_cola_ready);

    int i = 0, n = list_size(cola_ready);
    while (i < n) {
        t_query* quer = list_get(cola_ready, i);

        // 0 es MEJOR que 1. Queremos insertar ANTES del primer peor.
        // Si quer es PEOR (quer->prioridad_actual > q->prioridad_actual), cortamos.
        if (quer->prioridad_actual > q->prioridad_actual) break;

        // Si es igual o mejor, seguimos avanzando para que los "más viejos" queden antes.
        i++;
    }

    list_add_in_index(cola_ready, i, q); // agregamos con index para que ante prioridades iguales se respete el orden de llegada
    pthread_mutex_unlock(&mutex_cola_ready);

    log_info(loggerMaster, "READY <- Q (p=%d, p_act=%d) en idx %d",
             q->prioridad, q->prioridad_actual, i);

    sem_post(&hay_query_ready); // despertar planificador
}