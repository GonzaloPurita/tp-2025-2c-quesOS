#ifndef _FILETAGS_H
#define _FILETAGS_H

#include <unistd.h>
#include "utilsStorage/bloques.h"
#include <utils/paquete.h>

// bool existeFileTag(char* nombreFile, char* nombreTag); Va a haber que implementarlo.

/**
 * @brief Crea un nuevo fileTag con el nombre de archivo y nombre de etiqueta especificados.
 * 
 * @param nombreFile Nombre del archivo asociado al fileTag.
 * @param nombreTag Nombre de la etiqueta asociada al fileTag.
 * @return true si el fileTag se creó exitosamente, false en caso contrario.
 */
op_code crearFileTag(char* nombreFile, char* nombreTag);

/**
 * @brief Obtiene la ruta completa de un fileTag dado el nombre del archivo y la etiqueta.
 * 
 * @param nombreFile Nombre del archivo asociado al fileTag.
 * @param nombreTag Nombre de la etiqueta asociada al fileTag.
 * @return Un puntero a una cadena de caracteres que contiene la ruta completa del fileTag.
 */
char* rutaFileTag(char* nombreFile, char* nombreTag);

/**
 * @brief Cambia el estado de la metadata de un fileTag específico.
 * 
 * @param file Nombre del archivo asociado al fileTag.
 * @param tag Nombre de la etiqueta asociada al fileTag.
 * @param estadoNuevo Nuevo estado que se asignará al fileTag.
 * @return true si el estado se cambió exitosamente, false en caso contrario.
 */
bool cambiarEstadoMetaData(char* file, char* tag, t_estado_fileTag estadoNuevo);

/**
 * @brief Crea un archivo inicial con la configuración predeterminada.
 * 
 * @return true si el archivo inicial se creó exitosamente, false en caso contrario.
 */
bool crearFileInicial();

/**
 * @brief Agrega un bloque lógico y su correspondiente bloque físico a un fileTag.
 * 
 * @param nombreFile Nombre del archivo asociado al fileTag.
 * @param nombreTag Nombre de la etiqueta asociada al fileTag.
 * @param numeroBloqueLogico Número del bloque lógico que se desea agregar.
 * @param numeroBloqueFisico Número del bloque físico correspondiente al bloque lógico.
 * @return true si el bloque se agregó exitosamente, false en caso contrario.
 */
bool agregarBloqueLogicoHL(char* nombreFile, char* nombreTag, int numeroBloqueLogico, int numeroBloqueFisico);

/**
 * @brief Elimina un bloque lógico de un fileTag específico.
 * 
 * @param nombreFile Nombre del archivo asociado al fileTag.
 * @param nombreTag Nombre de la etiqueta asociada al fileTag.
 * @param numeroBloqueLogico Número del bloque lógico que se desea eliminar.
 * @return true si el bloque se eliminó exitosamente, false en caso contrario.
 */
bool eliminarBloqueLogicoHL(char* nombreFile, char* nombreTag, int numeroBloqueLogico);

/**
 * @brief Obtiene la metadata de un fileTag específico.
 * 
 * @param file Nombre del archivo asociado al fileTag.
 * @param tag Nombre de la etiqueta asociada al fileTag.
 * @return Un puntero a la estructura t_config que contiene la metadata del fileTag.
 */
t_config* getMetaData(char* file, char* tag);

/**
 * @brief Obtiene el número de bloque físico asociado a un bloque lógico específico de un fileTag.
 * 
 * @param file Nombre del archivo asociado al fileTag.
 * @param tag Nombre de la etiqueta asociada al fileTag.
 * @param numeroBloqueLogico Número del bloque lógico cuyo bloque físico se desea obtener.
 * @return El número del bloque físico asociado al bloque lógico, o -1 en caso de error.
 */
int obtenerBloqueFisico(char* file, char* tag, int numeroBloqueLogico);

#endif /* _FILETAGS_H */