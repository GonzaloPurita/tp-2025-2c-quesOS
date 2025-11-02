#include "main.h"

void* recibirCliente(void* cliente);

int main(int argc, char* argv[]) {
    iniciar_config();

    char* puertoEscucha = string_itoa(configMaster->puerto_escucha);
    int conexionServer = crearConexionServidor(puertoEscucha);
    free(puertoEscucha);

    while(1) {
        int cliente = esperarCliente(conexionServer);
        int* clientePtr = malloc(sizeof(int));
        *clientePtr = cliente;
        pthread_t hilo;
        pthread_create(&hilo, NULL, recibirCliente,  clientePtr);
        pthread_detach(hilo);
    }
}

void* recibirCliente(void* cliente) {
    int socket_cliente = *((int*) cliente);
    free(cliente);

    op_code cod = recibir_operacion(socket_cliente);
    switch(cod) {
        case MENSAJE: 
            t_list* datos = recibir_paquete(socket_cliente);
            char* mensaje = list_get(datos, 0);
            log_info(loggerMaster, "Mensaje recibido: %s", mensaje);
            list_destroy_and_destroy_elements(datos, free);
            break;
        default:
            log_warning(loggerMaster, "Operacion desconocida. No se puede atender al cliente.");
            break;
    }
    return NULL;
}
