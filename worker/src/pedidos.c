#include "pedidos.h"
#include <utilsWorker/globales.h>
#include "query_interpreter.h"
    
void pedir_tamanio_de_bloque(){
    // Le pido el tam de bloque al storage mediante un handshake
    t_paquete* paqueteStorage = crear_paquete();
    paqueteStorage->codigo_operacion = TAMANIO_BLOQUE;
    int envio = 1;
    log_info(loggerWorker, "mando este opcode: %d", paqueteStorage->codigo_operacion);
    log_info(loggerWorker, "→ Enviando handshake TAMANIO_BLOQUE al Storage...");
    agregar_a_paquete(paqueteStorage, &envio, sizeof(int));
    enviar_paquete(paqueteStorage, conexionStorage);
    log_info(loggerWorker, "la conexion con storage es: %d", conexionStorage);
    eliminar_paquete(paqueteStorage);

    // Recibo el tamanio de bloque, el cual manejamos como GLOBAL
    op_code codigo_operacion = recibir_operacion(conexionStorage);

    log_info(loggerWorker, "la conexion con storage es: %d, cod op: %d", conexionStorage, codigo_operacion);
    if(codigo_operacion != TAMANIO_BLOQUE){
        log_error(loggerWorker, "Error: esperaba TAMANIO_BLOQUE y llegó %d", codigo_operacion);
        return;
    }

    t_list* lista = recibir_paquete(conexionStorage);
        
    if (list_size(lista) < 1) {
        log_error(loggerWorker, "Error al recibir el tamanio de bloque");
        list_destroy(lista);
        return;
    }

    if (list_get(lista, 0) <= 0) {
        log_error(loggerWorker, "Handshake con Storage falló. No se pudo obtener TAM_BLOQUE: %d.", TAM_BLOQUE);
        exit(EXIT_FAILURE);
    }

    memcpy(&TAM_BLOQUE, list_get(lista, 0), sizeof(int)); // es global 

    log_info(loggerWorker, "Handshake con Storage exitoso. TAM_BLOQUE: %d.", TAM_BLOQUE);

    list_destroy_and_destroy_elements(lista, free);
}

int enviar_identificador_a_master(char* id) {
    t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion = ID_WORKER;

    agregar_a_paquete(paquete, id, sizeof(int));
    enviar_paquete(paquete, conexionMaster);
    eliminar_paquete(paquete);

    int cod_op = recibir_operacion(conexionMaster);
    if(cod_op != RTA_ID_WORKER) {
        log_error(loggerWorker, "Error: esperaba ID_WORKER y llegó %d", cod_op);
        return -1;
    }
    t_list* lista_paquete = recibir_paquete(conexionMaster);
    int result = *(int*) list_get(lista_paquete, 0);

	return result;
}

void notificar_master_desalojo(int pc) {
    t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion = RTA_DESALOJO;
    agregar_a_paquete(paquete, &query_actual->query_id, sizeof(int));
    agregar_a_paquete(paquete, &pc, sizeof(int));

    enviar_paquete(paquete, conexionMaster);
    eliminar_paquete(paquete);

    log_info(loggerWorker, "## Query %d: Desalojada en PC=%d por pedido del Master", query_actual->query_id, pc);
}
