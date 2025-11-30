#ifndef CONFIGURACION_MASTER_H_
#define CONFIGURACION_MASTER_H_

#include <commons/config.h>
#include <commons/string.h>
#include <commons/log.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int puerto_escucha; // PUERTO_ESCUCHA = 9001
    char* algoritmo_planificacion; // ALGORITMO_PLANIFICACION = FIFO o PRIORIDADES
    int tiempo_aging; // TIEMPO_AGING = 2500
    char* log_level; // LOG_LEVEL = TRACE
} t_config_master;

// Config del Kernel Global
extern t_config_master* configMaster;
extern t_log* loggerMaster;

void iniciar_config(char*);
void liberar_config();

#endif /* CONFIGURACION_MASTER */