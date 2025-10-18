#include <utilsMaster/semaforos.h>
#include <utilsMaster/configMaster.h>
#include <t_query.h>
#include <t_conexionWorker.h>
#include <utils/qcb.h>

typedef struct {
    t_query* candidatoDesalojo;   // La que está en EXEC y va a salir
    t_query* nuevoProceso;        // La que entra desde READY
    t_conexionWorker* worker;     // Worker donde está ejecutando el candidato
} t_datos_desalojo;

void planificarSinDesalojo();
void* planificador(void* arg);
void enviarQueryAWorker(t_query* query);
void enviarQueryAWorkerEspecifico(t_query* query, t_conexionWorker* conexionWorker);
bool cmp_query_por_prioridad(void* _a, void* _b);
void ordenar_ready(void);
t_conexionWorker* obtenerWorkerLibre();
t_query* buscarQueryConMenorPrioridad();
int64_t calcularPrioridadRestante(t_query* candidatoDesalojo);
void realizarDesalojo(t_query* candidatoDesalojo, t_query* nuevoProceso);
void* desalojar(void* arg);