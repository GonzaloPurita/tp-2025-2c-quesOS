#ifndef CONFIGURACION_WORKER_H_
#define CONFIGURACION_WORKER_H_

#include <commons/config.h>
#include <commons/string.h>
#include <commons/log.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char* ip_master; // IP_MASTER = 127.0.0.1
    char* ip_storage; // IP_STORAGE = 127.0.0.1
    char* algoritmo_reemplazo; // ALGORITMO_REEMPLAZO = LRU o CLOCK-M
    char* path_queries; // 
    char* log_level; // LOG_LEVEL = TRACE
    int puerto_master; // PUERTO_MASTER = 9001
    int puerto_storage; // PUERTO_STORAGE = 9002
    int tam_memoria; // 4096
    int retardo_memoria; // 1500
} t_config_worker;

// Config del Kernel Global
extern t_config_worker* configMaster;
extern t_log* loggerMaster;

void iniciar_config(char* path);
void liberar_config();

#endif /* CONFIGURACION_MASTER */