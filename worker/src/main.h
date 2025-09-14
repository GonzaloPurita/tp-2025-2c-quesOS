#ifndef MAIN_H_
#define MAIN_H_

#include <utils/conexiones.h>
#include <utils/paquete.h>
#include <utilsWorker/configWorker.h>
#include "pedidos.h"

pthread_t hilo_listener;

atomic_bool interrupcion;

int enviar_identificador_a_master(char* id);

#endif /* MAIN_H_ */