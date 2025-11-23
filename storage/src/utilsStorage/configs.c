#include "configs.h"
#include "auxiliares.h"

t_config_storage* configStorage;
t_superblock* superblock;
t_log* loggerStorage;
t_config* hashMap; 

void inicializarConfigStorage(char* archivoConfiguracion) {
    configStorage = malloc(sizeof(t_config_storage));
    t_config* config = config_create(archivoConfiguracion);

    if (config == NULL) {
        fprintf(stderr, "Error: No se pudo cargar el archivo de configuración: %s\n", archivoConfiguracion);
        exit(EXIT_FAILURE); // Terminar el programa si no se puede cargar el archivo

    }
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

void incializarHashMap() {
    // Ruta del índice de hashes bajo el punto de montaje
    char* ruta = string_duplicate(configStorage->punto_montaje);
    // Aseguro separador
    if (ruta[strlen(ruta) - 1] != '/') {
        string_append(&ruta, "/");
    }
    string_append(&ruta, "blocks_hash_index.config");

    // Si existe, lo abro. Si no, lo creo vacío y lo abro.
    if (archivoExiste(ruta)) {
        hashMap = config_create(ruta);
        if (hashMap == NULL) {
            log_error(loggerStorage, "blocks_hash_index.config existe pero no pudo leerse. Re-creando vacío.");
        }
    }

    if (hashMap == NULL) {
        t_config* nuevo = malloc(sizeof(t_config));
        if (nuevo == NULL) {
            log_error(loggerStorage, "Sin memoria para crear hashMap");
            free(ruta);
            return;
        }
        nuevo->properties = dictionary_create();
        nuevo->path = string_duplicate(ruta);
        if (config_save_in_file(nuevo, nuevo->path) == -1) {
            log_error(loggerStorage, "No se pudo crear %s", ruta);
            // Liberar recursos parcialmente creados
            dictionary_destroy(nuevo->properties);
            free(nuevo->path);
            free(nuevo);
            hashMap = NULL;
            free(ruta);
            return;
        }
        hashMap = nuevo;
    }

    free(ruta);
}

void inicializarConfigs(char* archivoConfiguracion) {
    inicializarConfigStorage(archivoConfiguracion);
    incializarSuperblock();
    // incializarHashMap(); // Mover a inicializarFS, post formateo/carga
}

void liberarConfigs() {
    free(configStorage->punto_montaje);
    free(configStorage->log_level);
    free(configStorage);
    free(superblock);
    log_destroy(loggerStorage);
    if (hashMap != NULL) {
        config_destroy(hashMap);
        hashMap = NULL;
    }
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