#include "./main.h"

int main(int argc, char* argv[]) { 
    iniciar_config(argv[1]);
    char* id = argv[2];

    // --- Conexión a Storage ---
    char* puertoStorage = string_itoa(configWorker->puerto_storage);
    conexionStorage = crearConexionCliente(configWorker->ip_storage, puertoStorage);
    t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion = ID_WORKER;
    agregar_a_paquete(paquete, id, strlen(id)+1);
    enviar_paquete(paquete, conexionStorage);
    eliminar_paquete(paquete);

    free(puertoStorage);
    log_info(loggerWorker, "la conexion con storage es: %d", conexionStorage);
    pedir_tamanio_de_bloque(); // llega el TAM_BLOQUE (global)

    iniciar_memoria(); // inicializa memoria interna usando el TAM_BLOQUE

    // --- Conexión a Master ---
    log_debug(loggerWorker, "Conectando con Master en %s:%d", configWorker->ip_master, configWorker->puerto_master);
    char* puertoMaster = string_itoa(configWorker->puerto_master);
    conexionMaster = crearConexionCliente(configWorker->ip_master, puertoMaster);
    free(puertoMaster);

    //enviar ID a Master
    enviar_identificador_a_master(id);
    
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
