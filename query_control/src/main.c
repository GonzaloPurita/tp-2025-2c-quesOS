#include "main.h"

int main(int argc, char* argv[]) {
    iniciar_config(argv[1], "Query_Control");
    log_debug(loggerQueryControl, "Configuracion cargada. IP Master: %s, Puerto Master: %d, Log Level: %s",
              configQueryControl->ip_master,
              configQueryControl->puerto_master,
              configQueryControl->log_level);

    int conexion = crearConexionCliente(configQueryControl->ip_master, string_itoa(configQueryControl->puerto_master));
    log_debug(loggerQueryControl, "Conectado al Master en %s:%d", configQueryControl->ip_master, configQueryControl->puerto_master);

    t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion = MENSAJE;
    char* mensaje = "Hola, soy Query Control";
    agregar_a_paquete(paquete, mensaje, strlen(mensaje) + 1);
    enviar_paquete(paquete, conexion);
    log_debug(loggerQueryControl, "Mensaje enviado al Master: %s", mensaje);
    liberar_conexion(conexion);
    eliminar_paquete(paquete);

    liberar_config();
}
