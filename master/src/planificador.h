#include <utilsMaster/semaforos.h>
#include <utilsMaster/configMaster.h>
#include <t_query.h>
#include <t_conexionWorker.h>
#include <utils/qcb.h>

void planificarSinDesalojo();
void* planificador(void* arg);
void enviarQueryAWorker(t_query* query);
bool cmp_query_por_prioridad(void* _a, void* _b);
void ordenar_ready(void);
t_conexionWorker* obtenerWorkerLibre();