#include "main.h"

int main(int argc, char* argv[]) {
    iniciar_config();
    log_debug(loggerQueryControl, "Configuracion cargada. IP Master: %s, Puerto Master: %d, Log Level: %s",
              configQueryControl->ip_master,
              configQueryControl->puerto_master,
              configQueryControl->log_level);

    char* puertoEscucha = string_itoa(configMaster->puerto_escucha);
    int conexionServer = crearConexionServidor(puertoEscucha);
    free(puertoEscucha);

    while(1) {
        int cliente = esperarCliente(conexionServer);
        pthread_t hilo;
        pthread_create(&hilo, NULL, recibirCliente, (void*) cliente);
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
            log_info(loggerQueryControl, "Mensaje recibido: %s", mensaje);
            list_destroy_and_destroy_elements(datos, free);
            break;
        default:
            log_warning(loggerQueryControl, "Operacion desconocida. No se puede atender al cliente.");
            break;
    }
    return NULL;
}
