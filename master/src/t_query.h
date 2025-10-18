#include <stdint.h>
#include <utilsMaster/semaforos.h>
#include <utilsMaster/configMaster.h>
#include <utils/qcb.h>

typedef enum { Q_READY, Q_EXEC, Q_EXIT } estado_query;
extern uint64_t AGING_TICK_GLOBAL;

typedef struct {
    t_qcb* QCB; // PCB pero de Query
    char* path; // La ruta al archivo
    int prioridad; // La prioridad que tiene
    int IDAging; // Se explica en la parte de AGING
    int prioridad_actual; // La que usa el scheduler y modifica el aging
    int pc_actual; // Para el desalojo
    int fd_qc; // Flujo de comunicacion para reenviarle al QC
    estado_query estado; // OPCIONAL: Podemos guardar el estado en el que esta.
}t_query;

t_query* crearQuery(char* path, int prioridad);
void destruirQuery(t_query* q);

extern t_list* cola_ready;
extern t_list*  cola_exec;
extern t_list*  cola_exit;

//AGING

void* hilo_aging(void* arg);
void aplicar_aging_ready(void);