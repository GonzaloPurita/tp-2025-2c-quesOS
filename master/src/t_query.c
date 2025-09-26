#include "t_query.h"

t_query* crearQuery(char* path, int prioridad) {
    pthread_mutex_lock(&mutex_queries);
    t_query* nuevaQuery = malloc(sizeof(t_query));
    if (nuevaQuery== NULL) {
        log_error(loggerMaster, "Error al crear el proceso");
        pthread_mutex_unlock(&mutex_queries);
        return NULL;
    }
    nuevoProceso->estado = Q_READY; // Estado inicial de la query
    nuevaQuery->path = path;
    nuevaQuery->prioridad = prioridad;
    nuevaQuery->IDAging = 0; // Inicialmente el IDAging es 0


    
    nuevaQuery->QCB = crearQCB();
    if (nuevaQuery->QCB == NULL) {
        free(nuevaQuery);
        log_error(loggerMaster, "Error al crear QCB");
        pthread_mutex_unlock(&mutex_queries);
        return NULL;
    }

void destruirQuery(t_query* query) {
    if (!query) return;
    if (query->QCB) destruirQCB(query->QCB);
    if (query->path) free(q->path);
    free(query);
}

void actualizarMetricas(estado_query estadoActual, estado_query estadoNuevo, t_query* query) {
    query->estadoActual = estadoNuevo; 

    //TODO: Implementar para prioridades y aging
}

void agregarAReadyPorPrioridad(t_query* query){
    pthread_mutex_lock(&mutex_cola_ready);
    // Agregar la query a la cola de ready según su prioridad
    // Suponiendo que cola_ready es una lista ordenada por prioridad
    int i = 0;
    while (i < list_size(cola_ready) && ((t_query*)list_get(cola_ready, i))->prioridad >= query->prioridad) {
        i++;
    }
    list_add_in_index(cola_ready, i, query);
    pthread_mutex_unlock(&mutex_cola_ready);
}