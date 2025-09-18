#ifndef _AUXILIARES_H
#define _AUXILIARES_H

#include <stdio.h>
#include <stdlib.h>
#include <ftw.h>
#include <unistd.h>
#include "configs.h"

/**
 * @brief Borra un archivo o directorio en la ruta especificada.
 *
 * Esta función elimina el archivo o directorio ubicado en la ruta proporcionada.
 * Si la ruta corresponde a un directorio, se eliminará recursivamente todo su contenido.
 *
 * @param path Ruta del archivo o directorio a borrar.
 * @return Un entero que indica el resultado de la operación:
 *         - 0 si la operación fue exitosa.
 *         - -1 si ocurrió un error (por ejemplo, si el archivo o directorio no existe
 *           o si el directorio no está vacío).
 */
int borrar(const char* path);

/**
 * @brief Crea un directorio en la ruta especificada.
 *
 * @param path Ruta donde se desea crear el directorio.
 * @return Un entero que indica el resultado de la operación:
 *         - 0 si el directorio se creó exitosamente.
 *         - -1 si ocurrió un error al intentar crear el directorio.
 */
int crearDirectorio(const char* path);

char* rutaCompleta(char* nombre);
bool directorioExiste(const char* path);
char* buscarBloqueFisico(int numeroBloqueFisico);
char* crearNombreBloque(int nroBloque);

#endif