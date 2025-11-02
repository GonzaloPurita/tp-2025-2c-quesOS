#ifndef API_MEMORIA_H_
#define API_MEMORIA_H_

// esta seria como la "interfaz" de memoria interna para el query interpreter, porque sino hay tremenda dependencia circular

#include <stdbool.h>
#include <commons/collections/list.h>
#include "formato.h"

t_list* paginas_necesarias(int direccion_base, int tamanio);
bool esta_en_memoria(t_formato* formato, int nro_pagina);
void pedir_pagina_a_storage(t_formato* formato, int nro_pagina);
char* leer_desde_memoria(t_formato* formato, int direccion_base, int tamanio);
void escribir_en_memoria(t_formato* formato, int direccion_base, char* valor);
void guardar_paginas_modificadas(void);
void notificar_master_desalojo(int pc_actual);

#endif