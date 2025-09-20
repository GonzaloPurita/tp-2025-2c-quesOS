#include <stdint.h>
#include <utils/semaforos.h>
#include <utils/qcb.h>

typedef enum { Q_READY, Q_EXEC, Q_EXIT } estado_query;

typedef struct {
    t_qcb* QCB; // PCB pero de Query
    char* path; // La ruta al archivo
    int prioridad; // La prioridad que tiene
    int IDAging; // Se explica en la parte de AGING
    estado_query estado; // OPCIONAL: Podemos guardar el estado en el que esta.
}t_query;

t_query* crearQuery(char* path, int prioridad);
void destruirQuery(t_query* q);

extern t_list* cola_ready;
extern t_list*  cola_exec;
extern t_list*  cola_exit;