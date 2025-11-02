#include "configWorker.h"

t_config_worker* configWorker;
t_log* loggerWorker;

void iniciar_config(char* path) {
    configWorker = malloc(sizeof(t_config_worker));
    t_config * config = config_create(path);
    
    
    configWorker->ip_master = strdup(config_get_string_value(config, "IP_MASTER"));
    configWorker->ip_storage = strdup(config_get_string_value(config, "IP_STORAGE"));
    configWorker->algoritmo_reemplazo = strdup(config_get_string_value(config, "ALGORITMO_REEMPLAZO"));
    configWorker->path_scripts = strdup(config_get_string_value(config, "PATH_SCRIPTS"));
    configWorker->log_level = strdup(config_get_string_value(config, "LOG_LEVEL"));
    configWorker->puerto_master = config_get_int_value(config, "PUERTO_MASTER");
    configWorker->puerto_storage = config_get_int_value(config, "PUERTO_STORAGE");
    configWorker->tam_memoria= config_get_int_value(config, "TAM_MEMORIA");
    configWorker->retardo_memoria = config_get_int_value(config, "RETARDO_MEMORIA");
    
    config_destroy(config);
    
    loggerWorker = log_create("worker.log", "Worker", 1, log_level_from_string(configWorker->log_level));
}

void liberar_config() {
    free(configWorker->ip_master);
    free(configWorker->ip_storage);
    free(configWorker->algoritmo_reemplazo);
    free(configWorker->path_scripts);
    free(configWorker->log_level);
    free(configWorker);
    log_destroy(loggerWorker);
}