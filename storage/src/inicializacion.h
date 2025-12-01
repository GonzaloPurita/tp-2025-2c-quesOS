#ifndef _INICIALIZACION_H
#define _INICIALIZACION_H

#include "utilsStorage/configs.h"
#include "utilsStorage/bloques.h"
#include "fileTags.h"

/**
 * @brief Inicializa el sistema de archivos (File System).
 * 
 * Esta función se encarga de realizar todas las configuraciones y 
 * preparativos necesarios para que el sistema de archivos esté listo 
 * para su uso. Esto puede incluir la creación de estructuras de datos 
 * internas, la inicialización de bloques de memoria, y cualquier otra 
 * tarea requerida para el correcto funcionamiento del sistema de archivos.
 */
void inicializarFS(char* archivoConfiguracion);

#endif