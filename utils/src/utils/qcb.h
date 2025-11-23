#ifndef QCB_H_
#define QCB_H_


#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/config.h>
#include <pthread.h>
#include <commons/collections/list.h> 
#include <utils/paquete.h>     
#include <semaphore.h>      

typedef struct {
    int QID; // Query ID autoincremental
    int PC;  // Program Counter
} t_qcb;

/* Autoincremental global (arranca en 0) y thread-safe, igual que tu PCB */
t_qcb* crearQCB(void);

/* Serialización (opcional: usala solo si tu protocolo manda el QCB “como paquete”) */
void enviarQCB(int conexion, t_qcb* qcb, char* nombreArchivo);     // op_code: QCB (o el que definas)
t_qcb* recibirQCB(int conexion);

/* Destroy */
void destruirQCB(t_qcb* qcb);

#endif /* QCB_H_ */