#include "inicializacion.h"

// Funciones privadas --> No están en el .h porque no se usa afuera
void formatearFS();
void crearBitmap();
void crearRutaFiles();

void inicializarFS() {
    inicializarConfigs();
    if (configStorage->fresh_start) { // Hay que generar todo
        formatearFS();
        crearBitmap();
        crearRutaFiles();
        crearFileInicial();
    }
    else { // Tenemos que usar lo que ya tenemos

    }
}

void formatearFS() { // Borro todo lo que haya de antes.
    // Creo las rutas completas
    char* rutaBitmap = rutaCompleta("bitmap.bin");
    char* rutaBlocks = rutaCompleta("blocks_hash_index.config");
    char* rutaPhysical = rutaCompleta("/physical_blocks");
    char* rutaFiles = rutaCompleta("/files");

    // Borro los archivos y directorios si existen
    borrar(rutaBitmap);
    borrar(rutaBlocks);
    borrar(rutaPhysical);
    borrar(rutaFiles);

    // Libero las rutas
    free(rutaBitmap);
    free(rutaBlocks);
    free(rutaPhysical);
    free(rutaFiles);
}

void crearBitmap() {
    // TODO: Usar el mmap().
}

void crearRutaFiles() {
    char* rutaFiles = rutaCompleta("/files");
    if (mkdir(rutaFiles, 0700) == -1) { // El 0700 es para que solo el dueño tenga permisos
        log_error(loggerStorage, "Error creando el directorio files");
    }
    free(rutaFiles);
}