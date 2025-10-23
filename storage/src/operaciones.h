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

/**
 * @brief Realiza un commit de una etiqueta (tag) de un archivo en el sistema de almacenamiento.
 *
 * Esta función realiza un commit de una etiqueta asociada a un archivo utilizando los datos
 * proporcionados en la lista `data`. Además, envía una respuesta al cliente a través
 * del socket especificado.
 *
 * @param data Lista de datos necesarios para realizar el commit.
 * @param socket_cliente Descriptor del socket del cliente para enviar la respuesta.
 */
void commit(t_list* data, int socket_cliente);

/**
 * @brief Escribe datos en un bloque lógico de un archivo y etiqueta (tag) en el sistema de almacenamiento.
 *
 * Esta función escribe los datos proporcionados en un bloque lógico específico
 * de un archivo y etiqueta (tag) utilizando los datos en la lista `data`. Además,
 * envía una respuesta al cliente a través del socket especificado.
 *
 * @param data Lista de datos necesarios para escribir en el bloque lógico.
 * @param socket_cliente Descriptor del socket del cliente para enviar la respuesta.
 */
void writeFileTag(t_list* data, int socket_cliente);

/**
 * @brief Lee datos de un bloque lógico de un archivo y etiqueta (tag) en el sistema de almacenamiento.
 *
 * Esta función lee los datos de un bloque lógico específico de un archivo y etiqueta (tag)
 * utilizando los datos en la lista `data`. Además, envía una respuesta al cliente a través
 * del socket especificado.
 *
 * @param data Lista de datos necesarios para leer del bloque lógico.
 * @param socket_cliente Descriptor del socket del cliente para enviar la respuesta.
 */
void readBloqueLogico(t_list* data, int socket_cliente);

#endif // _OPERACIONES_H