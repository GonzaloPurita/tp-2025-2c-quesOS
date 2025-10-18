#include <utilsMaster/semaforos.h>
#include <stdlib.h>
#include <unistd.h>
#include <commons/collections/list.h>
#include <stdbool.h>
#include <commons/collections/list.h>
#include <utilsMaster/configMaster.h>
#include <utils/paquete.h>

t_list* LISTA_WORKERS = NULL;           // definición (no extern)

extern t_list* LISTA_WORKERS;

extern uint64_t AGING_TICK_GLOBAL = 0;

typedef struct {
    int  id;
    int  fd;
    bool conectado;   // true si el socket sigue vivo
    int  qid_actual;  // -1 si está libre; si no, el QID que ejecuta
} t_conexionWorker;

// Inicialización 
void workers_iniciar(void);

// Altas/bajas
void worker_registrar(int id, int fd);
void worker_eliminar_por_fd(int fd);          // remueve de la lista y cierra fd

// Estado de ejecución
void worker_marcar_libre_por_fd(int fd);

// Consultas
int  workers_conectados(void);                // cantidad total en la lista
int  workers_disponibles(void);               // qid_actual == -1
int  worker_id_por_fd(int fd);                // -1 si no existe

void* atender_worker(void* arg);
bool exec_buscar_por_qid(int qid, int* out_fd_qc);

