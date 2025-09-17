#define _XOPEN_SOURCE 500
#include "auxiliares.h"

// Función para eliminar un archivo o directorio
int eliminar(const char* path, const struct stat* sb, int typeflag, struct FTW* ftwbuf) {
    return remove(path);
}

// Función para borrar un directorio y su contenido
int borrarDirectorio(const char* path) {
    return nftw(path, eliminar, 10, FTW_DEPTH | FTW_PHYS);
}

int borrar(const char* path) {
    if(path == NULL) {
        log_error(loggerStorage, "Error borrando, el path es NULL");
        return -1;
    }
    if (access(path, F_OK) != -1) { // Reviso si existe
        struct stat path_stat;
        stat(path, &path_stat);
        if (S_ISDIR(path_stat.st_mode)) { // Es un directorio
            return borrarDirectorio(path);
        } else { // Es un archivo
            return remove(path); 
        }
    }
    return 0; // El archivo o directorio no existe
}

int crearDirectorio(const char* path) {
    if(path == NULL) {
        log_error(loggerStorage, "Error creando el directorio, el path es NULL");
        return -1;
    }
    int resultado = mkdir(path, 0700); // Permisos de solo lectura, escritura y ejecución para el propietario
    if (resultado == -1) {
        log_error(loggerStorage, "Error creando el directorio, para el path: %s", path);
    }
    return resultado;
}

char* rutaCompleta(char* nombre) {
    if (nombre == NULL) {
        log_error(loggerStorage, "Error generando la ruta completa, el nombre es NULL");
        return NULL;
    }
    char* ruta = string_duplicate(configStorage->punto_montaje);
    string_append(&ruta, nombre);
    return ruta;
}