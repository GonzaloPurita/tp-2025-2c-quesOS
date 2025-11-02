#include "./main.h"

int main(int argc, char* argv[]) { 
    iniciar_config(argv[1]);

    char* puertoMaster = string_itoa(configWorker->puerto_master);
    int conexionMaster = crearConexionCliente(configWorker->ip_master, puertoMaster);
    free(puertoMaster);

    t_paquete* paqueteMaster = crear_paquete();
    paqueteMaster->codigo_operacion = MENSAJE;
    agregar_a_paquete(paqueteMaster, "Hola soy el Worker", strlen("Hola soy el Worker") + 1);
    enviar_paquete(paqueteMaster, conexionMaster);
    eliminar_paquete(paqueteMaster);
    close(conexionMaster);

    char* puertoStorage = string_itoa(configWorker->puerto_storage);
    int conexionStorage = crearConexionCliente(configWorker->ip_storage, puertoStorage);
    free(puertoStorage);

    t_paquete* paqueteStorage = crear_paquete();
    paqueteStorage->codigo_operacion = MENSAJE;
    agregar_a_paquete(paqueteStorage, "Hola soy el Worker", strlen("Hola soy el Worker") + 1);
    enviar_paquete(paqueteStorage, conexionStorage);
    eliminar_paquete(paqueteStorage);
    close(conexionStorage);

    liberar_config();
    return 0;
}
