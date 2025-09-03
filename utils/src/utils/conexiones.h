#ifndef CONEXIONES_H_
#define CONEXIONES_H_

#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<commons/log.h>
#include<pthread.h>

/**
 * @brief Crea una conexión de cliente a un servidor.
 * 
 * @param ip La dirección IP del servidor al que se desea conectar.
 * @param puerto El número de puerto del servidor al que se desea conectar.
 * @return El descriptor de socket para la conexión del cliente, o -1 si ocurre un error.
 */
int crearConexionCliente(char* ip, char* puerto);

/**
 * @brief Crea un socket de servidor y comienza a escuchar conexiones entrantes.
 * 
 * @param puerto El número de puerto en el que el servidor escuchará conexiones.
 * @return El descriptor de socket para el servidor, o -1 si ocurre un error.
 */
int crearConexionServidor(char* puerto);

/**
 * @brief Espera a que un cliente se conecte al servidor.
 * 
 * @param socket_servidor El descriptor de socket del servidor.
 * @return El descriptor de socket para la conexión del cliente, o -1 si ocurre un error.
 */
int esperarCliente(int socket_servidor);

/**
 * @brief Libera los recursos asociados con una conexión de cliente.
 * 
 * @param socket_cliente El descriptor de socket de la conexión del cliente que se desea cerrar.
 */
void liberar_conexion(int socket_cliente);

#endif /* CONEXIONES_H_ */