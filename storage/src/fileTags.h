#ifndef _FILETAGS_H
#define _FILETAGS_H

#include <unistd.h>
#include "utilsStorage/bloques.h"
#include <utils/paquete.h>

// bool existeFileTag(char* nombreFile, char* nombreTag); Va a haber que implementarlo.

/**
 * @brief Crea un nuevo fileTag con el nombre de archivo y nombre de etiqueta especificados.
 * 
 * Esta función asocia un archivo con una etiqueta específica, creando un fileTag.
 * 
 * @param nombreFile Nombre del archivo al que se asociará la etiqueta.
 * @param nombreTag Nombre de la etiqueta que se asociará al archivo.
 * @return op_code Código de operación que indica el resultado:
 *         - true: Si el fileTag se creó exitosamente.
 *         - false: En caso de error durante la creación.
 */
op_code crearFileTag(char* nombreFile, char* nombreTag);

/**
 * @brief Crea una nueva etiqueta asociada a un archivo.
 * 
 * Esta función permite agregar una etiqueta a un archivo existente.
 * 
 * @param nombreFile Nombre del archivo al que se asociará la nueva etiqueta.
 * @param nombreTag Nombre de la etiqueta que se desea crear y asociar.
 * @return op_code Código de operación que indica el resultado:
 *         - true: Si la etiqueta se creó exitosamente.
 *         - false: En caso de error durante la creación.
 */
op_code crearTag(char* nombreFile, char* nombreTag);

/**
 * @brief Duplica un tag de origen a un tag de destino, creando una copia completa de todos los bloques.
 * 
 * @param fileOrigen Nombre del archivo que contiene el tag origen.
 * @param tagOrigen Nombre del tag origen a duplicar.
 * @param fileDestino Nombre del archivo donde se creará el tag destino.
 * @param tagDestino Nombre del tag destino.
 * @return op_code Código de operación que indica el resultado de la operación.
 */
op_code duplicarFileTag(char* fileOrigen, char* tagOrigen, char* fileDestino, char* tagDestino);

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

/**
 * @brief Valida un bloque lógico, chequeando su contenido en el hashmap. En caso de encontrar un parecido cambia el hardlink.
 * 
 * @param nombreFile Nombre del archivo asociado al fileTag.
 * @param nombreTag Nombre del Tag asociado al fileTag.
 * @param numeroBloqueLogico Número del bloque lógico.
 * @return El código de la operación que indica el resultado de la misma.
 */
op_code validarBloqueLogico(char* nombreFile, char* nombreTag, int numeroBloqueLogico);

#endif /* _FILETAGS_H */