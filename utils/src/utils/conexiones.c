#include "conexiones.h"

int crearConexionCliente(char* ip, char* puerto) {
    struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	getaddrinfo(ip, puerto, &hints, &server_info);

	// Ahora vamos a crear el socket.
	int socket_cliente = 0;

	socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

	// Ahora que tenemos el socket, vamos a conectarlo

	connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen);

	freeaddrinfo(server_info);

	return socket_cliente;
}

int crearConexionServidor(char* puerto) {
    int socket_servidor;

	struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(NULL, puerto, &hints, &servinfo);

	// Creamos el socket de escucha del servidor

	socket_servidor = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

	// Asociamos el socket a un puerto
	
	setsockopt(socket_servidor, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int)); // Hacemos que varios puertos puedan ocupar el mismo socket

	bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen); //Asociamos nuestro socket a un puerto

	// Escuchamos las conexiones entrantes

	listen(socket_servidor, SOMAXCONN); // Establecemos que este socket es de escucha

	freeaddrinfo(servinfo);
	// log_trace(logger, "Listo para escuchar a mi cliente");

	return socket_servidor;
}

int esperarCliente(int socket_servidor) {
	// Aceptamos un nuevo cliente
	int socket_cliente;

	socket_cliente = accept(socket_servidor, NULL, NULL); // Esto es bloqueante, el servidor se va a quedar esperando a ese cliente.

	return socket_cliente;
}

void liberar_conexion(int socket_cliente) {
	close(socket_cliente);
}

