#ifndef PEDIDOS_H_
#define PEDIDOS_H_

#include "main.h"

void pedir_tamanio_de_bloque();
int enviar_identificador_a_master(char* id);
void notificar_master_desalojo(int pc);

#endif