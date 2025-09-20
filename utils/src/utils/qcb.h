#include <pthread.h>
#include <stdlib.h>
#include <commons/collections/list.h> // por si usás recibir_paquete de commons
#include <utils/paquete.h>           // tu utils con opcodes y paquetes

typedef struct {
    int QID; // Query ID autoincremental
    int PC;  // Program Counter
} t_qcb;

/* Autoincremental global (arranca en 0) y thread-safe, igual que tu PCB */
t_qcb* crearQCB(void);

/* Serialización (opcional: usala solo si tu protocolo manda el QCB “como paquete”) */
void enviarQCB(int conexion, t_qcb* qcb);     // op_code: QCB (o el que definas)
t_qcb* recibirQCB(int conexion);

/* Destroy */
void destruirQCB(t_qcb* qcb);