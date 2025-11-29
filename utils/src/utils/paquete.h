#ifndef PAQUETES_H_
#define PAQUETES_H_

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<commons/collections/list.h>
#include "conexiones.h"

typedef enum {
	MENSAJE,
  
	// Master <---> QUERY CONTROL
	SUBMIT_QUERY,
	RTA_SUBMIT_QUERY,
	MENSAJE_LECTURA,

	QUERY_FINALIZADA,

	// WORKER <---> MASTER
	ID_WORKER,
	RTA_ID_WORKER,
	OP_END,
	OP_READ,
	DESALOJO,
	RTA_DESALOJO,
	OP_WRITE,
	OP_ERROR,
	QCB,

	// WORKER <---> STORAGE
	TAMANIO_BLOQUE,
	OP_CREATE,
	OP_TRUNCATE,
	OP_DELETE,
	OP_TAG,
	OP_COMMIT,
	PED_PAG,
	GUARDAR_MODIFICADAS,

	// RESPUESTAS - ERRORES
	OP_SUCCESS,
	OP_FAILED,
	ERROR_FILE_NOT_FOUND,
	ERROR_TAG_NOT_FOUND,
	ERROR_NO_SPACE,
	ERROR_WRITE_NOT_ALLOWED,
	ERROR_OUT_OF_BOUNDS,
	ERROR_FILE_TAG_EXISTS
}op_code;


typedef struct {
	int size;
	void* stream;
} t_buffer;

typedef struct {
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

/**
* @brief Crea un paquete vacío.
* @return Un puntero al paquete creado.
*/
t_paquete* crear_paquete(void);

/**
* @brief Agrega un valor genérico al paquete.
* @param paquete Puntero al paquete donde se agregará el valor.
* @param valor Puntero al valor a agregar.
* @param tamanio Tamaño en bytes del valor a agregar.
* @return No devuelve nada.
*/
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);

/**
* @brief Agrega una cadena de texto al paquete.
* @param paquete Puntero al paquete donde se agregará la cadena.
* @param valor Cadena de texto a agregar.
* @param tamanio Tamaño en bytes de la cadena.
* @return No devuelve nada.
*/
void agregar_a_paquete_string(t_paquete* paquete, const char* valor, int tamanio);

/**
* @brief Envía un paquete a través de un socket.
* @param paquete Puntero al paquete a enviar.
* @param socket_cliente Descriptor del socket del cliente.
* @return No devuelve nada.
*/
void enviar_paquete(t_paquete* paquete, int socket_cliente);

/**
* @brief Libera la memoria asociada a un paquete.
* @param paquete Puntero al paquete a eliminar.
* @return No devuelve nada.
*/
void eliminar_paquete(t_paquete* paquete);

/**
* @brief Recibe un paquete desde un socket.
* @param socket_cliente Descriptor del socket del cliente.
* @return Una lista con los datos recibidos en el paquete.
*/
t_list* recibir_paquete(int socket_cliente);

/**
* @brief Serializa un paquete en un buffer de bytes.
* @param paquete Puntero al paquete a serializar.
* @param bytes Puntero donde se almacenará el tamaño del buffer serializado.
* @return Un puntero al buffer serializado.
*/
void* serializar_paquete(t_paquete* paquete, int bytes);

/**
* @brief Recibe un buffer desde un socket.
* @param size Puntero donde se almacenará el tamaño del buffer recibido.
* @param socket_cliente Descriptor del socket del cliente.
* @return Un puntero al buffer recibido.
*/
void* recibir_buffer(int* size, int socket_cliente);

/**
* @brief Crea un buffer vacío dentro de un paquete.
* @param paquete Puntero al paquete donde se creará el buffer.
* @return No devuelve nada.
*/
void crear_buffer(t_paquete* paquete);

/**
* @brief Libera la memoria asociada a un buffer.
* @param buffer Puntero al buffer a eliminar.
* @return No devuelve nada.
*/
void eliminar_buffer(t_buffer* buffer);

/**
 * @brief Recibe el código de operación desde un socket.
 * @param socket_cliente Descriptor del socket del cliente.
 * @return El código de operación recibido.
 */
int recibir_operacion(int socket_cliente);

#endif /* PAQUETES_H_ */