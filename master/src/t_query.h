#include <stdint.h>
#include <semaforos.h>

typedef enum { Q_READY, Q_EXEC, Q_EXIT } q_state;

typedef struct {
    int QID;
    int PC;          // Program Counter
} t_qcb;

typedef struct {
    t_qcb* QCB; // PCB pero de Query
    char* path; // La ruta al archivo
    int prioridad; // La prioridad que tiene
    int IDAging; // Se explica en la parte de AGING
    estado_query estado; // OPCIONAL: Podemos guardar el estado en el que esta.
}t_query;

t_query* crearQuery(int fd_qc, const char* path, int prioridad, int qid, uint64_t arrival);
void destruirQuery(t_query* q);