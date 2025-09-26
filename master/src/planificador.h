#include <utilsMaster/semaforos.h>
#include <utilsMaster/configMaster.h>
#include <t_query.h>
#include <t_conexionWorker.h>
#include <utils/qcb.h>

void planificarSinDesalojo();
void* planificador(void* arg);
void enviarQueryAWorker(t_query* query);
t_conexionWorker* obtenerWorkerLibre();