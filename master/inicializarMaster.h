#include <t_conexionWorker.h>
#include <utilsMaster/configMaster.h>
#include <utils/paquete.h>
#include <utilsMaster/semaforos.h>
#include <utilsMaster/configMaster.h>

void iniciarConexionesMaster(void);
void cerrarConexionesMaster(void);

void* recibirConexiones(void* arg);
void* atenderCliente(void* arg);
void* atender_worker(void* arg);