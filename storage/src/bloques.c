#include "bloques.h"

// Metodos privados
bool validarTamanioEscritura(char* rutaBloque, size_t sizeDatos, bool agregar);

bool bloqueFisicoValido(int numeroBloqueFisico) {
    return numeroBloqueFisico >= 0 && numeroBloqueFisico < superblock->nroBloques;
}

bool escribirBloqueInicial() {
    char* datos = malloc(superblock->blocksize);
    if (datos == NULL) {
        log_error(loggerStorage, "Error escribiendo bloque inicial, no se pudo reservar memoria");
        return false;
    }
    memset(datos, 0, superblock->blocksize);
    bool resultado = escribirBloqueFisico(0, datos, superblock->blocksize, false);
    free(datos);
    return resultado;
}

bool escribirBloqueFisico(int numeroBloqueFisico, void* datos, size_t sizeDatos, bool agregar) {
    if (datos == NULL) return false;
    if (!bloqueFisicoValido(numeroBloqueFisico)) return false;

    char* rutaBloque = rutaBloqueFisico(numeroBloqueFisico);

    if (!validarTamanioEscritura(rutaBloque, sizeDatos, agregar)) {
        free(rutaBloque);
        return false;
    }

    FILE* bloque = fopen(rutaBloque, agregar ? "r+b" : "wb");
    if (bloque == NULL) {
        log_error(loggerStorage, "Error abriendo el bloque físico %s para escritura", rutaBloque);
        free(rutaBloque);
        return false;
    }

    if (agregar) fseek(bloque, 0, SEEK_END); // Muevo el puntero al final del archivo si es para agregar

    size_t elementosEscritos = fwrite(datos, sizeDatos, 1, bloque);
    if (elementosEscritos != 1) {
        log_error(loggerStorage, "Error escribiendo en el bloque físico %s", rutaBloque);
        fclose(bloque);
        free(rutaBloque);
        return false;
    }

    fclose(bloque);
    free(rutaBloque);
    return true;
}

char* rutaBloqueFisico(int nroBloqueFisico) {
    // Genero la ruta del bloque físico
    char* nombre = crearNombreBloque(nroBloqueFisico);
    char* rutaBloque = string_new();
    string_append(&rutaBloque, rutaCompleta("/physical_blocks"));
    string_append(&rutaBloque, "/");
    string_append(&rutaBloque, nombre);
    free(nombre);
    return rutaBloque;    
}

bool validarTamanioEscritura(char* rutaBloque, size_t sizeDatos, bool agregar) {
    struct stat st;
    if (stat(rutaBloque, &st) == -1) {
        log_error(loggerStorage, "Error obteniendo tamaño del bloque físico %s", rutaBloque);
        return false;
    }
    if (agregar && st.st_size + sizeDatos > superblock->blocksize) { // Comparo si lo que tiene mas lo que quiero escribir supera el tamaño del bloque
        log_error(loggerStorage, "Overflow de bloque físico %s (actual=%ld, a escribir=%zu, max=%d)", rutaBloque, st.st_size, sizeDatos, superblock->blocksize);
        return false;
    }
    if (!agregar && sizeDatos > superblock->blocksize) { // Comparo si lo que quiero escribir supera el tamaño del bloque
        log_error(loggerStorage, "Datos mayores al tamaño del bloque (%zu > %d)", sizeDatos, superblock->blocksize);
        return false;
    }
    return true;
}