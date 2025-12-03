#ifndef PLANIFICADOR_H_
#define PLANIFICADOR_H_

#include <utilsMaster/semaforos.h>
#include <utilsMaster/configMaster.h>
#include <utilsMaster/utilsPlanificacion.h>
#include <t_query.h>
#include <t_conexionWorker.h>
#include <utils/qcb.h>

typedef struct {
    t_query* candidatoDesalojo;   // La que está en EXEC y va a salir
    t_query* nuevoQuery;        // La que entra desde READY
    t_conexionWorker* worker;     // Worker donde está ejecutando el candidato
} t_datos_desalojo;

void planificador_lanzar(void);
void planificarSinDesalojo();
void planificarConDesalojoYAging();
void* planificador(void* arg);
void enviarQueryAWorker(t_query* query);
void enviarQueryAWorkerEspecifico(t_query* query, t_conexionWorker* conexionWorker);
t_conexionWorker* obtenerWorkerLibre();
t_query* buscarQueryConMenorPrioridad();
void realizarDesalojo(t_query* candidatoDesalojo, t_query* nuevoProceso);
void* desalojar(void* arg);

#endif /* PLANIFICADOR_H_ */