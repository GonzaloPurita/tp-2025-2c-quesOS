#ifndef UTILS_PLANIFICACION_H_
#define UTILS_PLANIFICACION_H_

#include <t_query.h>
#include <utilsMaster/semaforos.h>
#include <utilsMaster/configMaster.h>

typedef struct {
    int idTemporizador;
    t_query* query;
} t_info_aging;

// Operaciones de planificación sobre las colas
bool cmp_query_por_prioridad(void* a, void* b);
void ordenar_ready(void);
void aplicar_aging_ready(void);
void* hilo_aging(void* arg);
void iniciarAging(t_query* query);

#endif /* UTILS_PLANIFICACION_H_ */