#ifndef _BLOQUES_H_
#define _BLOQUES_H_

#include <stdbool.h>
#include <utilsStorage/auxiliares.h>

bool bloqueFisicoValido(int numeroBloqueFisico);
bool escribirBloqueFisico(int numeroBloqueFisico, void* datos, size_t sizeDatos, bool agregar);
bool escribirBloqueInicial();
char* rutaBloqueFisico(int nroBloqueFisico);

#endif /* _BLOQUES_H_ */