#include <utilsMaster/semaforos.h>
#include <utilsMaster/configMaster.h>
#include <t_query.h>

void planificarSinDesalojo();
pthread_mutex_t* obtenerMutexPorEstado(char* estado);
void* planificador(void* arg);
