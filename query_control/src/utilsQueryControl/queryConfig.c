#include "queryConfig.h"

t_config_query_control* configQueryControl;
t_log* loggerQueryControl;

void iniciar_config(char* path, char* id) {
    configQueryControl = malloc(sizeof(t_config_query_control));
    t_config* config = config_create(path);

    configQueryControl->ip_master = strdup(config_get_string_value(config, "IP_MASTER"));
    configQueryControl->log_level = strdup(config_get_string_value(config, "LOG_LEVEL"));

    // Valores numéricos
    configQueryControl->puerto_master = config_get_int_value(config, "PUERTO_MASTER");

    config_destroy(config);
    loggerQueryControl = log_create(id, "Query_Control", 1, log_level_from_string(configQueryControl->log_level));
}

void liberar_config() {
    free(configQueryControl->ip_master);
    free(configQueryControl->log_level);
    free(configQueryControl);
    log_destroy(loggerQueryControl);
}