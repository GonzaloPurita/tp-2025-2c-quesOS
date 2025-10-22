#ifndef _OPERACIONES_H
#define _OPERACIONES_H

#include "fileTags.h"
#include <commons/collections/list.h>
#include <utilsStorage/bloques.h>
#include <utils/paquete.h>
#include <stdbool.h>


/**
 * @brief Crea un archivo en el sistema de almacenamiento.
 *
 * Esta función se encarga de crear un archivo utilizando los datos proporcionados
 * en la lista `data`. Además, envía una respuesta al cliente a través del socket
 * especificado.
 *
 * @param data Lista de datos necesarios para la creación del archivo.
 * @param socket_cliente Descriptor del socket del cliente para enviar la respuesta.
 */
void crearFile(t_list* data, int socket_cliente);
 
/**
 * @brief Trunca un archivo en el sistema de almacenamiento.
 *
 * Esta función modifica el tamaño de un archivo existente según los datos
 * proporcionados en la lista `data`. También envía una respuesta al cliente
 * a través del socket especificado.
 *
 * @param data Lista de datos necesarios para truncar el archivo.
 * @param socket_cliente Descriptor del socket del cliente para enviar la respuesta.
 */
void truncar(t_list* data, int socket_cliente);

/**
 * @brief Crea una etiqueta (tag) para un archivo en el sistema de almacenamiento.
 *
 * Esta función crea una nueva etiqueta asociada a un archivo utilizando los datos
 * proporcionados en la lista `data`. Además, envía una respuesta al cliente a través
 * del socket especificado.
 *
 * @param data Lista de datos necesarios para crear la etiqueta.
 * @param socket_cliente Descriptor del socket del cliente para enviar la respuesta.
 */
void tag(t_list* data, int socket_cliente);

/**
 * @brief Elimina una etiqueta (tag) de un archivo en el sistema de almacenamiento.
 *
 * Esta función elimina una etiqueta asociada a un archivo utilizando los datos
 * proporcionados en la list `data`. Además, envía una respuesta al cliente a través
 * del socket especificado.
 *
 * @param data Lista de datos necesarios para eliminar la etiqueta.
 * @param socket_cliente Descriptor del socket del cliente para enviar la respuesta.
 */
void eliminarTag(t_list* data, int socket_cliente);

#endif // _OPERACIONES_H