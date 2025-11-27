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

/**
 * @brief Genera la ruta completa a partir de un nombre dado.
 * 
 * @param nombre Nombre base para generar la ruta completa.
 * @return Un puntero a una cadena de caracteres que contiene la ruta completa.
 *         Es responsabilidad del llamador liberar la memoria asignada.
 */
char* rutaCompleta(char* nombre);

/**
 * @brief Verifica si un directorio existe en la ruta especificada.
 * 
 * @param path Ruta del directorio a verificar.
 * @return true si el directorio existe, false en caso contrario.
 */
bool directorioExiste(const char* path);

/**
 * @brief Verifica si un archivo existe en la ruta especificada.
 *
 * @param path Ruta del archivo a verificar.
 * @return true si el archivo existe, false en caso contrario.
 */
bool archivoExiste(const char* path);

/**
 *@brief Convierte una cantidad de bytes en cantidad de bloques, redondeando para arriba
 *
 *@param bytes Cantidad de bytes a convertir
 *@return Cantidad de bloques
 */
int pasarABloques(int bytes);

/**
 * @brief Imprime una lista de cadenas de caracteres.
 * 
 * @param lista Lista de cadenas de caracteres a imprimir. La lista debe estar terminada con un puntero NULL.
 */
void imprimirLista(char** lista);

#endif