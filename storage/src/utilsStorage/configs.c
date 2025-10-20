#include "configs.h"

t_config_storage* configStorage;
t_superblock* superblock;
t_log* loggerStorage;

void inicializarConfigStorage() {
    configStorage = malloc(sizeof(t_config_storage));
    t_config* config = config_create("storage.config");

    // Strings
    configStorage->punto_montaje = strdup(config_get_string_value(config, "PUNTO_MONTAJE"));
    configStorage->log_level = strdup(config_get_string_value(config, "LOG_LEVEL"));

    // Numericos
    configStorage->puerto_escucha = config_get_int_value(config, "PUERTO_ESCUCHA");
    configStorage->retardo_operacion = config_get_int_value(config, "RETARDO_OPERACION");
    configStorage->retardo_acceso_bloque = config_get_int_value(config, "RETARDO_ACCESO_BLOQUE");
    
    // Booleanos
    char* valorFresh = config_get_string_value(config, "FRESH_START");
    if (strcmp(valorFresh, "TRUE") == 0) {
        configStorage->fresh_start = true;
    } else if (strcmp(valorFresh, "FALSE") == 0) {
        configStorage->fresh_start = false;
    } else {
        log_error(loggerStorage, "Error en el valor de FRESH_START en storage.config. Usando FALSE por defecto.");
        configStorage->fresh_start = false;
    }
    
    config_destroy(config);

    loggerStorage = log_create("storage.log", "Storage", 1, log_level_from_string(configStorage->log_level));
}

void incializarSuperblock() {
    superblock = malloc(sizeof(t_superblock));
    char* rutaSuperBlock = string_duplicate(configStorage->punto_montaje);
    string_append(&rutaSuperBlock, "/superblock.config");
    t_config* config = config_create(rutaSuperBlock);

    superblock->fssize = config_get_int_value(config, "FS_SIZE");
    superblock->blocksize = config_get_int_value(config, "BLOCK_SIZE");

    superblock->nroBloques = superblock->fssize / superblock->blocksize;

    config_destroy(config);
    free(rutaSuperBlock);
}

void inicializarConfigs() {
    inicializarConfigStorage();
    incializarSuperblock();
}

void liberarConfigs() {
    free(configStorage->punto_montaje);
    free(configStorage->log_level);
    free(configStorage);
    free(superblock);
    log_destroy(loggerStorage);
}

char* estadoToString(t_estado_fileTag estado) {
    switch (estado) {
        case WORK_IN_PROGRESS:
            return "WORK_IN_PROGRESS";
        case COMMITED:
            return "COMMITED";
        default:
            return "UNKNOWN";
    }
}