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

typedef struct {
    int fssize; // 4096
    int blocksize; // 64
    int nroBloques; // 64
} t_superblock;

extern t_config_storage* configStorage;
extern t_superblock* superblock;
extern t_log* loggerStorage;

/**
 * @brief Inicializa la configuración del módulo de almacenamiento.
 * 
 * Esta función se encarga de cargar y configurar los parámetros necesarios
 * para el correcto funcionamiento del módulo de almacenamiento. Debe ser
 * llamada antes de utilizar cualquier funcionalidad que dependa de estas
 * configuraciones.
 */
void inicializarConfigs();

/**
 * @brief Libera los recursos asignados para la configuración.
 * 
 * Esta función libera la memoria o los recursos que fueron asignados
 * durante la inicialización de la configuración. Debe ser llamada cuando
 * el módulo de almacenamiento ya no esté en uso para evitar fugas de memoria.
 */
void liberarConfigs();

#endif 