#ifndef CONFIGURACION_STORAGE_H_
#define CONFIGURACION_STORAGE_H_

#include <commons/config.h>
#include <commons/string.h>
#include <commons/log.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int puerto_escucha; //9002
    int fresh_start; // 1=TRUE 0=FALSE
    char* punto_montaje; // /home/utnso/storage
    int retardo_operacion; // 8000
    int retardo_acceso_bloque; // 4000
    char* log_level; // INFO
} t_config_storage;

extern t_config_storage* configStorage;
extern t_log* loggerStorage;

void iniciar_config();
void liberar_config();

#endif 