#ifndef MAIN_H_
#define MAIN_H_

#include <utils/conexiones.h>
#include <utils/paquete.h>
#include <utilsWorker/configWorker.h>
#include <utilsWorker/globales.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/string.h>
#include <pthread.h>
#include <stdbool.h>
#include "pedidos.h"
#include <utilsWorker/globales.h>
#include "query_interpreter.h"

pthread_t hilo_listener;

int enviar_identificador_a_master(char* id);
void* listener_master();

#endif