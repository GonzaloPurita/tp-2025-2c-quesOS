#ifndef T_CONEXIONWORKER_H_
#define T_CONEXIONWORKER_H_

#include <utilsMaster/semaforos.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <commons/collections/list.h> 
#include <stdbool.h> 
#include <utilsMaster/configMaster.h> 
#include <utils/paquete.h>
#include <t_query.h>


extern t_list* LISTA_WORKERS;


typedef struct {
    char*  id;
    int  fd;
    bool conectado;   // true si el socket sigue vivo
    int  qid_actual;  // -1 si está libre; si no, el QID que ejecuta
    sem_t semaforo;
} t_conexionWorker;

// Inicialización 
void workers_iniciar(void);

// Altas/bajas
void worker_registrar(char* id, int fd);
void worker_desconectar_por_fd(int fd);

// Estado de ejecución

// Consultas
int  workers_conectados(void);                // cantidad total en la lista
int  workers_disponibles(void);               // qid_actual == -1
t_conexionWorker* worker_por_fd(int fd);

void* atenderWorker(void* arg);
bool exec_buscar_por_qid(int qid, int* out_fd_qc);
void worker_marcar_libre_por_fd(int fd);
t_conexionWorker* encontrarWorkerPorQid(int qid);
#endif /* T_CONEXIONWORKER_H_ */

