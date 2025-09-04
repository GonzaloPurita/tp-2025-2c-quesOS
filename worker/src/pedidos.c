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

    agregar_a_paquete(paquete, id_cpu, sizeof(int));
    enviar_paquete(paquete, conexionMaster);
    eliminar_paquete(paquete);

	int result;
	recv(conexionMaster, &result, sizeof(int), MSG_WAITALL);

	return result;
}

