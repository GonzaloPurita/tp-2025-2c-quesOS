#ifndef INICIALIZAR_MASTER_H_
#define INICIALIZAR_MASTER_H_

#include <t_conexionWorker.h>
#include <utilsMaster/semaforos.h>
#include <utilsMaster/configMaster.h>
#include <utils/paquete.h>

void iniciarConexionesMaster(void);
void cerrarConexionesMaster(void);

void* recibirConexiones(void* arg);
void* atenderCliente(void* arg);

#endif /* INICIALIZAR_MASTER_H_*/