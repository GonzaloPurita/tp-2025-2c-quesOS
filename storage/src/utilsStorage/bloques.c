#include "bloques.h"
#include <fcntl.h>

// Variables globales
t_bitarray* bitmap;

// Metodos privados
bool validarTamanioEscritura(char* rutaBloque, size_t sizeDatos, bool agregar);

bool bloqueFisicoValido(int numeroBloqueFisico) {
    return numeroBloqueFisico >= 0 && numeroBloqueFisico < superblock->nroBloques;
}

char* buscarBloqueFisico(int numeroBloqueFisico) {
    // Validación de datos
    if (!bloqueFisicoValido(numeroBloqueFisico)) {
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

bool escribirBloqueInicial() {
    char* datos = malloc(superblock->blocksize);
    if (datos == NULL) {
        log_error(loggerStorage, "Error escribiendo bloque inicial, no se pudo reservar memoria");
        return false;
    }
    for(int i = 0; i < superblock->blocksize; i++) {
        datos[i] = '0';
    }
    bool resultado = escribirBloqueFisico(0, datos, superblock->blocksize, false, 0);
    free(datos);
    return resultado;
}

bool escribirBloqueFisico(int numeroBloqueFisico, char* datos, size_t sizeDatos, bool agregar, int query_id) {
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

void inicializarBitmap() {
    for(int i = 0; i < bitarray_get_max_bit(bitmap); i++) {
        bitarray_clean_bit(bitmap, i); // Pongo todos los bits en 0 (libres)
    }
}

void cargarBitmap() {
    int bytes_bitmap = (superblock->nroBloques + 7) / 8; // Cantidad de bytes necesarios para el bitmap (redondeo hacia arriba)

    char* path = rutaCompleta("/bitmap.bin");

    // Abrir o crear el archivo
    int fd = open(path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR); // Open es una syscall de Linux que devuelve un file descriptor (lo que necesito para el mmap)
    if (fd == -1) {
        log_error(loggerStorage, "Error abriendo bitmap.bin");
        exit(1);
    }

    // Ajustar tamaño del archivo
    if (ftruncate(fd, bytes_bitmap) == -1) { // Estoy corrigiendo el tamaño del archivo al tamaño del bitmap (que no sea ni mas pequeño ni mas grande)
        log_error(loggerStorage, "Error ajustando tamaño del bitmap");
        close(fd);
        exit(1);
    }

    // Mapear archivo a memoria
    void* bitmap_mem = mmap(NULL, bytes_bitmap, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (bitmap_mem == MAP_FAILED) {
        perror("Error en mmap del bitmap");
        close(fd);
        exit(1);
    }

    // Transformo ese bitmap en el struct de las commons
    bitmap = bitarray_create_with_mode(bitmap_mem, bytes_bitmap, LSB_FIRST);
    free(path);
    close(fd); // Cierro el file descriptor, el mmap sigue existiendo
}

// Bloques lógicos

bool crearHardlink(char* rutaBloqueLogico, int numeroBloqueLogico, int numeroBloqueFisico, int query_id) {
    // Validación de datos
    if (rutaBloqueLogico == NULL) {
        log_error(loggerStorage, "Error creando hard link, la ruta del bloque lógico es NULL");
        return false;
    }
    if (string_is_empty(rutaBloqueLogico)) {
        log_error(loggerStorage, "Error creando hard link, la ruta del bloque lógico está vacía");
        return false;
    }
    if (numeroBloqueLogico < 0 || numeroBloqueFisico < 0) {
        log_error(loggerStorage, "Error creando hard link, el número de bloque lógico o físico es negativo");
        return false;
    }

    // Busco el bloque físico
    char* rutaBloqueFisico = buscarBloqueFisico(numeroBloqueFisico);
    if(rutaBloqueFisico == NULL) {
        return false;
    }

    // Genero la ruta para el bloque lógico
    char* rutaBloqueLogicoTemp = string_duplicate(rutaBloqueLogico); // El append modifica el puntero, por eso hago una copia
    char* nombre = crearNombreBloque(numeroBloqueLogico);
    string_append(&rutaBloqueLogicoTemp, nombre);
    free(nombre);

    int linkResult = link(rutaBloqueFisico, rutaBloqueLogicoTemp); // Esto genera el hard link
    if (linkResult == -1) {
        log_error(loggerStorage, "Error creando hard link de %s a %s", rutaBloqueFisico, rutaBloqueLogicoTemp);
        free(rutaBloqueFisico);
        free(rutaBloqueLogicoTemp);
        return false;
    }
    free(rutaBloqueFisico);
    free(rutaBloqueLogicoTemp);
    return true;
}

int obtenerBloqueLibre(int query_id) {
    for (int i = 0; i < superblock->nroBloques; i++) {
        if (!bitarray_test_bit(bitmap, i)) { // Si el bit está en 0, está libre
            bitarray_set_bit(bitmap, i); // Marco el bloque como usado
            log_info(loggerStorage, "##%d - Bloque Físico Reservado - Número de Bloque: %d", query_id, i);
            return i;
        }
    }
    log_warning(loggerStorage, "No hay bloques físicos libres disponibles");
    return -1; // No hay bloques libres
}

int contarHardlinks(const char *path) {
    struct stat file_stat;
    
    if (stat(path, &file_stat) == -1) {
        perror("stat");
        return -1;
    }
    
    return file_stat.st_nlink;
}

bool esHardlinkUnico(const char *path) {
    struct stat file_stat;
    
    if (stat(path, &file_stat) == -1) {
        perror("stat");
        return false;
    }
    
    // Si st_nlink == 1, solo existe este enlace
    return (file_stat.st_nlink <= 2); // Considero 2 porque el primer hardlink es el archivo en sí mismo
}

char* leerBloqueFisico(int numeroBloqueFisico) {
    if (!bloqueFisicoValido(numeroBloqueFisico)) return NULL;

    char* rutaBloque = rutaBloqueFisico(numeroBloqueFisico);
    FILE* bloque = fopen(rutaBloque, "rb");
    if (bloque == NULL) {
        log_error(loggerStorage, "Error abriendo el bloque físico %s para lectura", rutaBloque);
        free(rutaBloque);
        return NULL;
    }

    char* datos = malloc(superblock->blocksize + 1); // +1 para el '\0'
    if (datos == NULL) {
        log_error(loggerStorage, "Error leyendo bloque físico %s, no se pudo reservar memoria", rutaBloque);
        fclose(bloque);
        free(rutaBloque);
        return NULL;
    }

    size_t elementosLeidos = fread(datos, superblock->blocksize, 1, bloque);
    if (elementosLeidos != 1) {
        log_error(loggerStorage, "Error leyendo del bloque físico %s", rutaBloque);
        free(datos);
        fclose(bloque);
        free(rutaBloque);
        return NULL;
    }

    datos[superblock->blocksize] = '\0';
    fclose(bloque);
    free(rutaBloque);
    return datos;
}

int obtenerNumeroBloqueFisico(const char* nombreBloque) {
    // Validar que el nombre del bloque no sea NULL
    if (nombreBloque == NULL) {
        log_error(loggerStorage, "Error: El nombre del bloque es NULL");
        return -1;
    }

    // Verificar que el nombre del bloque tenga el prefijo esperado "block"
    if (strncmp(nombreBloque, "block", 5) != 0) {
        log_error(loggerStorage, "Error: El nombre del bloque no tiene el prefijo esperado: %s", nombreBloque);
        return -1;
    }

    // Convertir la parte numérica del nombre del bloque a un entero
    int numeroBloque = atoi(nombreBloque + 5);
    return numeroBloque;
}

bool actualizarBloqueFileTag(t_config* metadata, int nroBloqueLogico, int nroBloqueFisico) {
    if (metadata == NULL || nroBloqueLogico < 0 || nroBloqueFisico < 0) {
        log_error(loggerStorage, "Error actualizando bloque en metadata, parámetros inválidos");
        return false;
    }

    char** bloques = config_get_array_value(metadata, "BLOCKS");
    if (bloques == NULL) {
        log_error(loggerStorage, "Error actualizando bloque en metadata, no se pudo obtener la lista de bloques");
        return false;
    }

    int len = string_array_size(bloques);
    if (nroBloqueLogico >= len) {
        log_error(loggerStorage, "Error actualizando bloque en metadata, el número de bloque lógico %d está fuera de los límites (tamaño actual: %d)", nroBloqueLogico, len);
        string_array_destroy(bloques);
        free(bloques);
        return false;
    }

    // Actualizar el bloque lógico con el nuevo bloque físico
    bloques[nroBloqueLogico] = string_itoa(nroBloqueFisico);

    // Reconstruir el string de bloques actualizado
    char* bloquesActualizados = string_new();
    string_append(&bloquesActualizados, "[");
    for (int i = 0; i < len; i++) {
        string_append(&bloquesActualizados, bloques[i]);
        if (i < len - 1) {
            string_append(&bloquesActualizados, ",");
        }
    }
    string_append(&bloquesActualizados, "]");

    // Actualizar y guardar la metadata
    config_set_value(metadata, "BLOCKS", bloquesActualizados);
    config_save(metadata);

    // Liberar memoria
    string_array_destroy(bloques);
    free(bloquesActualizados);

    return true;
}