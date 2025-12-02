#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/config.h>
#include <commons/log.h>
#include "configWorker.h"
#include "globales.h"

void iniciar_config(char* path) {
    t_config* config = config_create(path);
    if (!config) {
        fprintf(stderr, "No se pudo leer el archivo de config: %s\n", path);
        exit(EXIT_FAILURE);
    }

    configWorker = malloc(sizeof(t_config_worker));
    if (!configWorker) {
        fprintf(stderr, "No hay memoria para configWorker\n");
        config_destroy(config);
        exit(EXIT_FAILURE);
    }

    configWorker->ip_master = strdup(config_get_string_value(config, "IP_MASTER"));
    configWorker->ip_storage = strdup(config_get_string_value(config, "IP_STORAGE"));
    configWorker->algoritmo_reemplazo = strdup(config_get_string_value(config, "ALGORITMO_REEMPLAZO"));
    configWorker->path_scripts = strdup(config_get_string_value(config, "PATH_SCRIPTS"));
    configWorker->log_level = strdup(config_get_string_value(config, "LOG_LEVEL"));

    configWorker->puerto_master = config_get_int_value(config, "PUERTO_MASTER");
    configWorker->puerto_storage = config_get_int_value(config, "PUERTO_STORAGE");
    configWorker->tam_memoria = config_get_int_value(config, "TAM_MEMORIA");
    configWorker->retardo_memoria = config_get_int_value(config, "RETARDO_MEMORIA");

    loggerWorker = log_create("worker.log", "Worker", true, log_level_from_string(configWorker->log_level));

    config_destroy(config);
}

void liberar_config() {
    if (!configWorker) return;

    free(configWorker->ip_master);
    free(configWorker->ip_storage);
    free(configWorker->algoritmo_reemplazo);
    free(configWorker->path_scripts);
    free(configWorker->log_level);

    free(configWorker);
    configWorker = NULL;

    if (loggerWorker) {
        log_destroy(loggerWorker);
        loggerWorker = NULL;
    }
}