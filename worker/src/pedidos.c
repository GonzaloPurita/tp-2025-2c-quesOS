#include "pedidos.h"
    
void pedir_tamanio_de_bloque(){
    // Le pido el tam de bloque al storage mediante un handshake
    t_paquete* paqueteStorage = crear_paquete();
    paqueteStorage->codigo_operacion = TAMANIO_BLOQUE;
    int envio = 1;
    agregar_a_paquete(paqueteStorage, &envio, sizeof(int));
    enviar_paquete(paqueteStorage, conexionStorage);
    eliminar_paquete(paqueteStorage);
    
    // Recibo el tamanio de bloque, el cual manejamos como GLOBAL
    op_code codigo_operacion = recibir_operacion(conexionStorage);
    if(codigo_operacion != TAMANIO_BLOQUE){
        log_error(loggerWorker, "Error: esperaba TAMANIO_BLOQUE y llegó %d", codigo_operacion);
        return;
    }

    t_list* lista = recibir_paquete(conexionStorage);
        
    if (list_size(lista) < 1) {
        log_error(logger, "Error al recibir el tamanio de bloque");
        list_destroy(lista);
        return;
    }
        
    memcpy(&TAM_BLOQUE, list_get(lista, 0), sizeof(int)); // es global 

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
    t_list* paquete = recibir_paquete(conexionMaster);
    int result = list_get(paquete, 0);

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
