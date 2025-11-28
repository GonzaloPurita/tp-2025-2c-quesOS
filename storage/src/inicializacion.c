#include "inicializacion.h"

// Funciones privadas --> No están en el .h porque no se usa afuera
void formatearFS();
void crearRutaFiles();
void crearBloques();

void inicializarFS(char* archivoConfiguracion) {
    inicializarConfigs(archivoConfiguracion);
    log_info(loggerStorage, "Storage iniciado");
    if (configStorage->fresh_start) { // Hay que generar todo
        formatearFS();
        crearRutaFiles();
        crearBloques();
        escribirBloqueInicial();
        cargarBitmap();
        inicializarBitmap();
        bitarray_set_bit(bitmap, 0); 
        // Inicializo índice de hashes vacío
        incializarHashMap(true);
        crearFileInicial();
    }
    else { // Tenemos que usar lo que ya tenemos
        cargarBitmap();
        // Abro índice de hashes existente (o lo creo si falta)
        incializarHashMap(false);
    }
    log_debug(loggerStorage, "Sistema de archivos listo para usarse");
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

void crearRutaFiles() {
    char* rutaFiles = rutaCompleta("/files");
    if (mkdir(rutaFiles, 0700) == -1) { // El 0700 es para que solo el dueño tenga permisos
        log_error(loggerStorage, "Error creando el directorio files");
    }
    free(rutaFiles);
}

void crearBloques() {
    char* rutaBloquesFisicos = rutaCompleta("/physical_blocks");

    // Creo la carpeta physical_blocks
    if (mkdir(rutaBloquesFisicos, 0700) == -1) { // El 0700 es para que solo el dueño tenga permisos
        log_error(loggerStorage, "Error creando el directorio physical_blocks");
    }

    char* nombreBloque = crearNombreBloque(0);
    char* rutaBloque = string_new();
    string_append(&rutaBloque, rutaBloquesFisicos);
    string_append(&rutaBloque, "/");
    string_append(&rutaBloque, nombreBloque);

    // Creo los archivos que representan los bloques físicos
    for (int i = 0; i < superblock->nroBloques; i++) {
        char* nombreBloque = crearNombreBloque(i);
        char* rutaBloque = string_new();
        string_append(&rutaBloque, rutaBloquesFisicos);
        string_append(&rutaBloque, "/");
        string_append(&rutaBloque, nombreBloque);
        FILE *bloque = fopen(rutaBloque, "wb"); // Creo el archivo
        if (bloque == NULL) {
            log_error(loggerStorage, "Error creando el bloque físico %s", nombreBloque);
            free(nombreBloque);
            free(rutaBloque);
            return;
        }
        fclose(bloque);
        free(nombreBloque);
        free(rutaBloque);
    }
}