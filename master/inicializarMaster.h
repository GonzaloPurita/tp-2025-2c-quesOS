#include <t_conexionWorker.h>
#include <utilsMaster/configMaster.h>
#include <utils/paquete.h>

void iniciarConexionesMaster(void);
void cerrarConexionesMaster(void);

void* recibirConexiones(void* arg);
void* atenderCliente(void* arg);
void* atender_worker(void* arg);