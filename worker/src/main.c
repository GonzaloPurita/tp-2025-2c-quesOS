#include "./main.h"

int main(int argc, char* argv[]) { 
    iniciar_config(argv[1]);
    int id = atoi(argv[2]);

    // --- Conexión a Storage ---
    char* puertoStorage = string_itoa(configWorker->puerto_storage);
    int conexionStorage = crearConexionCliente(configWorker->ip_storage, puertoStorage);
    free(puertoStorage);

    pedir_tamanio_de_bloque(); // llega el TAM_BLOQUE (global)
    iniciar_memoria(); // inicializa memoria interna usando el TAM_BLOQUE

    // --- Conexión a Master ---
    char* puertoMaster = string_itoa(configWorker->puerto_master);
    int conexionMaster = crearConexionCliente(configWorker->ip_master, puertoMaster);
    free(puertoMaster);

    //enviar ID a Master
    enviar_id_a_master(id);

    liberar_config();
    close(conexionStorage);
    close(conexionMaster);
    return 0;
}
