#ifndef _FILETAGS_H
#define _FILETAGS_H

#include <unistd.h>
#include "utilsStorage/auxiliares.h"

// bool existeFileTag(char* nombreFile, char* nombreTag); Va a haber que implementarlo.

bool crearFileTag(char* nombreFile, char* nombreTag);
bool agregarBloqueLogicoHL(char* nombreFile, char* nombreTag, int numeroBloqueLogico, int numeroBloqueFisico);
bool crearFileInicial();
char* rutaFileTag(char* nombreFile, char* nombreTag);
bool cambiarEstadoMetaData(char* file, char* tag, t_estado_fileTag estadoNuevo);

#endif /* _FILETAGS_H */