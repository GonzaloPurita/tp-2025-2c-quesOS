#include "main.h"

int main(int argc, char* argv[]) {

    if(argc != 4){
        printf("No se pasaron los parametros necesarios");
        return EXIT_FAILURE;
    }

    char* path_config = argv[1];
    char* path_query = argv[2];
    int prioridad = atoi(argv[3]);

    iniciar_config(path_config, "Query_Control");
    
    log_info(loggerQueryControl, "## Conexión al Master exitosa. IP: %s, Puerto: %d",
             configQueryControl->ip_master, configQueryControl->puerto_master);

    int conexion = crearConexionCliente(configQueryControl->ip_master, string_itoa(configQueryControl->puerto_master));
    log_debug(loggerQueryControl, "Conectado al Master en %s:%d", configQueryControl->ip_master, configQueryControl->puerto_master);

    t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion = SUBMIT_QUERY;;
    agregar_a_paquete(paquete, path_query, strlen(path_query) + 1);
    agregar_a_paquete(paquete, &prioridad, sizeof(int) + 1);
    enviar_paquete(paquete, conexion);

    log_info(loggerQueryControl, "## Solicitud de ejecución de Query: %s, prioridad: %d",
             path_query, prioridad);

    escucharMaster(conexion);

    liberar_conexion(conexion);
    eliminar_paquete(paquete);

    liberar_config();
}
