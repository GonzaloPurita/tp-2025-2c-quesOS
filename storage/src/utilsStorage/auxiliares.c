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

bool directorioExiste(const char* path) {
    struct stat path_stat;
    // Verifica si la ruta existe y obtiene información sobre ella
    if (stat(path, &path_stat) == 0) {
        // Verifica si es un directorio
        return S_ISDIR(path_stat.st_mode);
    }
    return false; // La ruta no existe
}

bool archivoExiste(const char* path) {
    struct stat path_stat;
    // Verifica si la ruta existe y obtiene información sobre ella
    if (stat(path, &path_stat) == 0) {
        // Verifica si es un archivo regular
        return S_ISREG(path_stat.st_mode);
    }
    return false; // La ruta no existe
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

char* buscarBloqueFisico(int numeroBloqueFisico) {
    // Validación de datos
    if (numeroBloqueFisico < 0) {
        log_error(loggerStorage, "Error buscando bloque físico, el número de bloque físico es negativo");
        return NULL;
    }

    char* nombreBloque = crearNombreBloque(numeroBloqueFisico);
    char* rutaBloqueFisico = rutaCompleta("/physical_blocks/");
    string_append(&rutaBloqueFisico, nombreBloque);

    if (!archivoExiste(rutaBloqueFisico)) {
        log_error(loggerStorage, "Error buscando bloque físico, el bloque físico %s no existe", nombreBloque);
        free(rutaBloqueFisico);
        free(nombreBloque); 
        return NULL;
    }

    free(nombreBloque); 
    return rutaBloqueFisico;
}

char* crearNombreBloque(int nroBloque) {
    // Generar el nombre del bloque con ceros a la izquierda y extensión .dat
    char nombreBloque[20]; // Suficiente para "block" + 4 dígitos + ".dat" + '\0'
    snprintf(nombreBloque, sizeof(nombreBloque), "block%04d.dat", nroBloque);
    return string_duplicate(nombreBloque);
}