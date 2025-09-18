#include <commons/collections/dictionary.h>
#include <commons/collections/queue.h>
#include <utils/paquete.h> //op_code reales

void iniciarConexionesMaster(void);
void cerrarConexionesMaster(void);

// hilos aceptadores
void* recibirConexionesWorkers(void* arg);
void* recibirConexionesQC(void* arg);

// handlers (se llaman desde los aceptadores)
void aceptarWorker(int cliente_fd);
void aceptarQueryControl(int cliente_fd, int primer_op); // SUBMIT_QUERY esperado