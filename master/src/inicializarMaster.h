#ifndef INICIALIZAR_MASTER_H_
#define INICIALIZAR_MASTER_H_

#include <t_conexionWorker.h>
#include <utilsMaster/semaforos.h>
#include <utilsMaster/configMaster.h>
#include <utilsMaster/utilsPlanificacion.h>
#include <utils/paquete.h>
#include <planificador.h>

void iniciarConexionesMaster(void);
void cerrarConexionesMaster(void);

void* recibirConexiones(void* arg);
void* atenderCliente(void* arg);

void* monitorear_query_control(void* arg) ;
void query_control_desconectado(int qid);

typedef struct {
    int fd;
    int qid;
} t_conexion_qc;

#endif /* INICIALIZAR_MASTER_H_*/