#ifndef T_QUERY_H_
#define T_QUERY_H_

#include <stdint.h>
#include <utilsMaster/semaforos.h>
#include <utilsMaster/configMaster.h>
#include <utils/qcb.h>

typedef enum { Q_READY, Q_EXEC, Q_EXIT } estado_query;

extern uint64_t AGING_TICK_GLOBAL;

typedef struct {
    t_qcb* QCB; 
    char* path; // La ruta al archivo
    int prioridad; // La prioridad que tiene
    int IDAging; // Se explica en la parte de AGING
    int prioridad_actual; // La que usa el scheduler y modifica el aging
    int pc_actual; // Para el desalojo
    int fd_qc; // Flujo de comunicacion para reenviarle al QC
    int idTemporizador;
    estado_query estado; // OPCIONAL: Podemos guardar el estado en el que esta.
}t_query;

t_query* crearQuery(const char* path, int prioridad);
void destruirQuery(t_query* q);
void agregarAReadyPorPrioridad(t_query* q);
void actualizarMetricas(estado_query estadoActual, estado_query estadoNuevo, t_query* q);
void inicializarListasQueries();
void imprimir_cola(t_list* cola, const char* nombre_cola);

extern t_list* cola_ready;
extern t_list*  cola_exec;
extern t_list*  cola_exit;


#endif /* T_QUERY_H_ */