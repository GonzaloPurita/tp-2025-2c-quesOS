#include <utils/paquete.h>
#include <t_conexionesWorkers.h>

void iniciarConexionesMaster(void);
void cerrarConexionesMaster(void);

void* recibirConexiones(void* arg);
void* atenderCliente(void* arg);
void* atender_worker(void* arg);