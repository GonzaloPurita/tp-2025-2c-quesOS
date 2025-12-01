#include "configMaster.h"

t_config_master* configMaster;
t_log* loggerMaster;

void iniciar_config() {
    configMaster = malloc(sizeof(t_config_master));
    t_config* config = config_create("master.config");

    configMaster->algoritmo_planificacion = strdup(config_get_string_value(config, "ALGORITMO_PLANIFICACION"));
    configMaster->log_level = strdup(config_get_string_value(config, "LOG_LEVEL"));
    
    // Valores numéricos
    configMaster->puerto_escucha = config_get_int_value(config, "PUERTO_ESCUCHA");
    configMaster->tiempo_aging = config_get_int_value(config, "TIEMPO_AGING");

    config_destroy(config);
    loggerMaster = log_create("master.log", "Master", 1, log_level_from_string(configMaster->log_level));
}

void liberar_config() {
    free(configMaster->algoritmo_planificacion);
    free(configMaster->log_level);
    free(configMaster);
    log_destroy(loggerMaster);
}