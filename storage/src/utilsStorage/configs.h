#ifndef CONFIGURACION_STORAGE_H_
#define CONFIGURACION_STORAGE_H_

#include <commons/config.h>
#include <commons/string.h>
#include <commons/log.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int puerto_escucha; //9002
    bool fresh_start; // 1=TRUE 0=FALSE
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

typedef enum {
    WORK_IN_PROGRESS,
    COMMITED
}t_estado_fileTag;

extern t_config_storage* configStorage;
extern t_superblock* superblock;
extern t_log* loggerStorage;
// TODO: Implementar semáforos para el hashMap
extern t_config* hashMap; // Mapa de hashes de bloques físicos

/**
 * @brief Inicializa la configuración del módulo de almacenamiento.
 * 
 * Esta función se encarga de cargar y configurar los parámetros necesarios
 * para el correcto funcionamiento del módulo de almacenamiento. Debe ser
 * llamada antes de utilizar cualquier funcionalidad que dependa de estas
 * configuraciones.
 */
void inicializarConfigs(char* archivoConfiguracion);

/**
 * @brief Libera los recursos asignados para la configuración.
 * 
 * Esta función libera la memoria o los recursos que fueron asignados
 * durante la inicialización de la configuración. Debe ser llamada cuando
 * el módulo de almacenamiento ya no esté en uso para evitar fugas de memoria.
 */
void liberarConfigs();

/**
 * @brief Convierte un estado de tipo t_estado_fileTag a su representación en cadena de caracteres.
 *
 * @param estado El estado de tipo t_estado_fileTag que se desea convertir.
 * @return Un puntero a una cadena de caracteres que representa el estado.
 *         La cadena devuelta no necesariamente debe ser liberada, depende de como se pida.
 */
char* estadoToString(t_estado_fileTag estado);

/**
 * @brief Inicializa el índice de hashes (blocks_hash_index.config) y asigna hashMap.
 * 
 * @param crearVacio Indica si se debe crear un índice vacío (true) o abrir el existente (false).
 * 
 * Esta función se encarga de crear o abrir el archivo de índice de hashes
 * y asignar el puntero global hashMap para su uso.
 */
void incializarHashMap(bool crearVacio);

void inicializarConfigStorage(char* archivoConfiguracion);

#endif