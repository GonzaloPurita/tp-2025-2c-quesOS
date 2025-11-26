#include "./main.h"

int main(int argc, char* argv[]) { 
    iniciar_config(argv[1]);
    char* id = argv[2];

    // --- Conexión a Storage ---
    char* puertoStorage = string_itoa(configWorker->puerto_storage);
    conexionStorage = crearConexionCliente(configWorker->ip_storage, puertoStorage);
    free(puertoStorage);
    log_info(loggerWorker, "la conexion con storage es: %d", conexionStorage);
    pedir_tamanio_de_bloque(); // llega el TAM_BLOQUE (global)

    iniciar_memoria(); // inicializa memoria interna usando el TAM_BLOQUE

    // --- Conexión a Master ---
    char* puertoMaster = string_itoa(configWorker->puerto_master);
    conexionMaster = crearConexionCliente(configWorker->ip_master, puertoMaster);
    free(puertoMaster);

    //enviar ID a Master
    enviar_identificador_a_master(id);
    
    pthread_t hilo_master;
    pthread_create(&hilo_master, NULL, hilo_escuchar_master, NULL);
    pthread_detach(hilo_master);

    recibir_queries();

    pthread_join(hilo_listener, NULL);
    liberar_config();
    close(conexionStorage);
    close(conexionMaster);
    return 0;
}

// void* listener_master() {
//     while (1) {
//         int op = recibir_operacion(conexionMaster);

//         if (op <= 0) {
//             log_error(loggerWorker, "Master desconectado o error en recibir_operacion (op=%d). Cerrando listener.", op);
//             close(conexionMaster);
//             break;
//         }

//         if (op == DESALOJO) {
//             log_debug(loggerWorker, "Master pidió desalojo");
//             atomic_store(&interrupt_flag, 1);
//         } else {
//             log_debug(loggerWorker, "Listener Master recibió op desconocida: %d", op);
//         }
//     }

//     return NULL;
// }
