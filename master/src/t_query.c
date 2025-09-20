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