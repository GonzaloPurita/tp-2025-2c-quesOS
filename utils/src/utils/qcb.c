#include "qcb.h"

static int GlobalQID = 0;
static pthread_mutex_t mutexQID = PTHREAD_MUTEX_INITIALIZER;

t_qcb* crearQCB(void) {
    t_qcb* nuevo = malloc(sizeof(t_qcb));
    if (!nuevo) return NULL;

    pthread_mutex_lock(&mutexQID);
    nuevo->QID = GlobalQID++;
    pthread_mutex_unlock(&mutexQID);

    nuevo->PC = 0;   // lo avanza el Worker y nos lo manda por RTA_DESALOJO/OP_END
    return nuevo;
}

void enviarQCB(int conexion, t_qcb* qcb, char* nombreArchivo) {
    t_paquete* p = crear_paquete();
    p->codigo_operacion = QCB;  
    agregar_a_paquete(p, &qcb->QID, sizeof(int));
    agregar_a_paquete(p, &qcb->PC,  sizeof(int));
    agregar_a_paquete(p, nombreArchivo, strlen(nombreArchivo) + 1);
    enviar_paquete(p, conexion);
    eliminar_paquete(p);
}

t_qcb* recibirQCB(int conexion) {
    t_list* elems = recibir_paquete(conexion);
    if (!elems || list_size(elems) < 2) {
        if (elems) list_destroy_and_destroy_elements(elems, free);
        return NULL;
    }
    t_qcb* q = malloc(sizeof(t_qcb));
    if (!q) { list_destroy_and_destroy_elements(elems, free); return NULL; }

    memcpy(&q->QID, list_get(elems, 0), sizeof(int));
    memcpy(&q->PC,  list_get(elems, 1), sizeof(int));
    list_destroy_and_destroy_elements(elems, free);
    return q;
}

void destruirQCB(t_qcb* qcb) { free(qcb); }