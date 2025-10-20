#include "./main.h"

int main(int argc, char* argv[]) { 
    iniciar_config(argv[1]);
    char* id = argv[2];

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
    enviar_identificador_a_master(id);

    pthread_create(&hilo_listener, NULL, listener_master, NULL); // hilo que escucha interrupciones del Master

    recibir_queries();

    pthread_join(hilo_listener, NULL);
    liberar_config();
    close(conexionStorage);
    close(conexionMaster);
    return 0;
}

void* listener_master() {
    while (1) {
        int op = recibir_operacion(conexionMaster);
        if (op == DESALOJO) {
            log_debug(loggerWorker, "Master pidió desalojo");
            atomic_store(&interrupt_flag, 1);
        }
    }
    return NULL;
}