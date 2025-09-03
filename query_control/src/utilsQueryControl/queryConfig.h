#ifndef CONFIGURACION_QUERY_CONTROL_H_
#define CONFIGURACION_QUERY_CONTROL_H_

#include <commons/config.h>
#include <commons/string.h>
#include <commons/log.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char* ip_master;   // IP_MASTER=127.0.0.1
    int puerto_master;   // PUERTO_MASTER=9001
    char* log_level;    // LOG_LEVEL=TRACE
} t_config_query_control;

// Config del Kernel Global
extern t_config_query_control* configQueryControl;
extern t_log* loggerQueryControl;

void iniciar_config(char* path);
void liberar_config();

#endif /* CONFIGURACION_QUERY_CONTROL_H_ */