#include "fileTags.h"

// Funciones privadas
bool crearMetaData(char* path);
bool eliminarHardlink(char* rutaBloqueLogico, int numeroBloqueLogico, int bloqueFisico);
void free_wrapper(char* line);

bool crearFileInicial() {
    if (!crearFileTag("initial_file", "BASE")) {
        return false;
    }
    if (!agregarBloqueLogicoHL("initial_file", "BASE", 0, 0)) {
        return false;
    }
    //completarBloque(); TODO: Averiguar a que se refiere.
    if(!cambiarEstadoMetaData("initial_file", "BASE", COMMITED)) { // Supongo que arranca como COMMITED, CHEQUEAR
        return false;
    }
    return true;
}

op_code crearFileTag(char* nombreFile, char* nombreTag) {
    // Validación de datos
    if (nombreFile == NULL || nombreTag == NULL) {
        log_error(loggerStorage, "Error creando file tag, el nombre del file o del tag es NULL");
        return OP_FAILED;
    }
    if (string_is_empty(nombreFile) || string_is_empty(nombreTag)) {
        log_error(loggerStorage, "Error creando file tag, el nombre del file o del tag está vacío");
        return OP_FAILED;
    }

    // Creamos la ruta files
    char* rutaFile = rutaCompleta("/files/");
    string_append(&rutaFile, nombreFile);
    if(crearDirectorio(rutaFile) == -1) { // Me genera la carpeta con el nombre del file.
        free(rutaFile);
        return OP_FAILED;
    }

    // Creamos la ruta tag
    string_append(&rutaFile, "/");
    string_append(&rutaFile, nombreTag);
    if(crearDirectorio(rutaFile) == -1) { // Me genera la carpeta con el nombre del tag.
        free(rutaFile);
        return OP_FAILED;
    }

    // Creo el metadata.config para el tag
    if(!crearMetaData(rutaFile)) { 
        free(rutaFile);
        return OP_FAILED;
    }

    // Creamos la ruta logical_blocks
    string_append(&rutaFile, "/logical_blocks");
    if(crearDirectorio(rutaFile) == -1) { // Me genera la carpeta logical_blocks.
        free(rutaFile);
        return OP_FAILED;
    }

    free(rutaFile);
    return OP_SUCCESS;
}

bool crearMetaData(char* path) {
    // Validación de datos
    if (path == NULL) {
        log_error(loggerStorage, "Error creando metadata, la ruta es NULL");
        return false;
    }

    t_config* metadata = malloc(sizeof(t_config));
    if(metadata == NULL) {
        log_error(loggerStorage, "Error creando metadata, no se pudo reservar memoria");
        return false;
    }

    // Creamos los valores de metadata
    t_dictionary* data = dictionary_create();
    dictionary_put(data, "TAMAÑO", "0");
    dictionary_put(data, "BLOQUES", "[]");
    dictionary_put(data, "ESTADO", "WORK_IN_PROGRESS");
    metadata->properties = data;

    // Construimos el path del metadata.config
    metadata->path = string_duplicate(path);
    string_append(&metadata->path, "/metadata.config"); 

    // Guardamos el MetaData como archivo
    if(config_save_in_file(metadata, metadata->path) == -1) {
        log_error(loggerStorage, "Error creando metadata, no se pudo guardar en el archivo");
        config_destroy(metadata);
        return false;
    }
    return true;
}

char* rutaFileTag(char* nombreFile, char* nombreTag) {
    // Validación de datos
    if (nombreFile == NULL || nombreTag == NULL) {
        log_error(loggerStorage, "Error generando ruta file tag, el nombre del file o del tag es NULL");
        return NULL;
    }
    if (string_is_empty(nombreFile) || string_is_empty(nombreTag)) {
        log_error(loggerStorage, "Error generando ruta file tag, el nombre del file o del tag está vacío");
        return NULL;
    }

    char* ruta = rutaCompleta("/files/");
    string_append(&ruta, nombreFile);
    string_append(&ruta, "/");
    string_append(&ruta, nombreTag);

    if(!directorioExiste(ruta)) {
        log_error(loggerStorage, "Error generando ruta file tag, la ruta %s no existe", ruta);
        free(ruta);
        return NULL;
    }

    return ruta;
}

bool agregarBloqueLogicoHL(char* nombreFile, char* nombreTag, int numeroBloqueLogico, int numeroBloqueFisico) {
    // Validación de datos
    if (nombreFile == NULL || nombreTag == NULL) {
        log_error(loggerStorage, "Error agregando bloque lógico, el nombre del file o del tag es NULL");
        return false;
    }
    if (string_is_empty(nombreFile) || string_is_empty(nombreTag)) {
        log_error(loggerStorage, "Error agregando bloque lógico, el nombre del file o del tag está vacío");
        return false;
    }
    if (numeroBloqueLogico < 0 || numeroBloqueFisico < 0) {
        log_error(loggerStorage, "Error agregando bloque lógico, el número de bloque lógico o físico es negativo");
        return false;
    }

    // Creo la ruta completa a donde va el bloque lógico
    char* rutaBloqueLogico = rutaCompleta("/files/");
    string_append(&rutaBloqueLogico, nombreFile);
    string_append(&rutaBloqueLogico, "/");
    string_append(&rutaBloqueLogico, nombreTag);
    string_append(&rutaBloqueLogico, "/logical_blocks/");

    if(!crearHardlink(rutaBloqueLogico, numeroBloqueLogico, numeroBloqueFisico)) {
        free(rutaBloqueLogico);
        return false;
    }

    free(rutaBloqueLogico);
    return true;
}

bool eliminarBloqueLogicoHL(char* nombreFile, char* nombreTag, int numeroBloqueLogico) {
    // Validación de datos
    if (nombreFile == NULL || nombreTag == NULL) {
        log_error(loggerStorage, "Error eliminando bloque lógico, el nombre del file o del tag es NULL");
        return false;
    }
    if (string_is_empty(nombreFile) || string_is_empty(nombreTag)) {
        log_error(loggerStorage, "Error eliminando bloque lógico, el nombre del file o del tag está vacío");
        return false;
    }
    if (numeroBloqueLogico < 0) {
        log_error(loggerStorage, "Error eliminando bloque lógico, el número de bloque lógico es negativo");
        return false;
    }

    // Creo la ruta completa a donde va el bloque lógico
    char* rutaBloqueLogico = rutaCompleta("/files/");
    string_append(&rutaBloqueLogico, nombreFile);
    string_append(&rutaBloqueLogico, "/");
    string_append(&rutaBloqueLogico, nombreTag);
    string_append(&rutaBloqueLogico, "/logical_blocks/");

    int bloqueFisico = obtenerBloqueFisico(nombreFile, nombreTag, numeroBloqueLogico);
    if(bloqueFisico == -1) {
        free(rutaBloqueLogico);
        return false;
    }

    eliminarHardlink(rutaBloqueLogico, numeroBloqueLogico, bloqueFisico);

    free(rutaBloqueLogico);
    return true;
}

bool eliminarHardlink(char* rutaBloqueLogico, int numeroBloqueLogico, int bloqueFisico) {
    // Validación de parámetros
    if (rutaBloqueLogico == NULL || numeroBloqueLogico < 0 || bloqueFisico < 0) {
        log_error(loggerStorage, "Parámetros inválidos en eliminarHardlink");
        return false;
    }

    // Genero la ruta para el bloque lógico
    char* rutaBloqueLogicoTemp = string_duplicate(rutaBloqueLogico); // El append modifica el puntero, por eso hago una copia
    char* nombre = crearNombreBloque(numeroBloqueLogico);
    string_append(&rutaBloqueLogicoTemp, nombre);
    free(nombre);

    // Verifico si el bloque físico es único
    if (esHardlinkUnico(rutaBloqueLogicoTemp) && (bloqueFisico != 0)) {
        // Marco el bloque físico como libre en el bitmap
        bitarray_clean_bit(bitmap, bloqueFisico);
        log_debug(loggerStorage, "Bloque físico %d marcado como libre", bloqueFisico);
    }

    // Elimino el hardlink
    if (unlink(rutaBloqueLogicoTemp) == -1) {
        log_error(loggerStorage, "Error al eliminar el hardlink: %s", rutaBloqueLogicoTemp);
        return false;
    }

    // Libero la memoria
    free(rutaBloqueLogicoTemp);
    return true;
}

int obtenerBloqueFisico(char* file, char* tag, int numeroBloqueLogico) {
    // Obtener metadata del File:Tag
    t_config* metadata = getMetaData(file, tag);
    if (metadata == NULL) {
        log_error(loggerStorage, "No se pudo obtener el metadata para %s:%s", file, tag);
        return -1;
    }

    // Obtener la lista de bloques físicos
    char** bloques = config_get_array_value(metadata, "BLOQUES");
    if (bloques == NULL) {
        log_error(loggerStorage, "No se pudo obtener la lista de bloques físicos para %s:%s", file, tag);
        config_destroy(metadata);
        return -1;
    }

    // Verificar si el bloque lógico tiene un bloque físico asociado
    char* bloqueFisicoStr = bloques[numeroBloqueLogico];
    if (bloqueFisicoStr == NULL) {
        log_error(loggerStorage, "El bloque lógico %d no tiene un bloque físico asociado en %s:%s", numeroBloqueLogico, file, tag);
        
        string_iterate_lines(bloques, free_wrapper);
        free(bloques);
        config_destroy(metadata);
        return -1;
    }

    int numeroBloqueFisico = atoi(bloqueFisicoStr);

    // Liberar memoria
    string_iterate_lines(bloques, free_wrapper);
    free(bloques);
    config_destroy(metadata);

    return numeroBloqueFisico;
}

void free_wrapper(char* line) {
    free(line);
}

// Funciones genéricas de metadata

t_config* getMetaData(char* file, char* tag) {
    // Validación de datos
    if (file == NULL || tag == NULL) {
        log_error(loggerStorage, "Error obteniendo metadata, el nombre del file o del tag es NULL");
        return NULL;
    }
    if (string_is_empty(file) || string_is_empty(tag)) {
        log_error(loggerStorage, "Error obteniendo metadata, el nombre del file o del tag está vacío");
        return NULL;
    }

    char* ruta = rutaFileTag(file, tag);
    if(ruta == NULL) {
        return NULL;
    }

    string_append(&ruta, "/metadata.config"); // Tendria algo como ruta_montaje/files/file1//tag1/metadata.config

    t_config* metadata = config_create(ruta);
    if (metadata == NULL) {
        log_error(loggerStorage, "Error obteniendo metadata, no se pudo abrir el archivo metadata.config en la ruta %s", ruta);
        free(ruta);
        return NULL;
    }

    free(ruta);
    return metadata;
}

bool cambiarEstadoMetaData(char* file, char* tag, t_estado_fileTag estadoNuevo) {
    // Validación de datos
    if (file == NULL || tag == NULL) {
        log_error(loggerStorage, "Error cambiando estado metadata, el nombre del file o del tag es NULL");
        return false;
    }
    if (string_is_empty(file) || string_is_empty(tag)) {
        log_error(loggerStorage, "Error cambiando estado metadata, el nombre del file o del tag está vacío");
        return false;
    }
    
    char* ruta = rutaFileTag(file, tag);
    if(ruta == NULL) {
        return false;
    }

    string_append(&ruta, "/metadata.config"); // Tendria algo como ruta_montaje/files/file1//tag1/metadata.config

    t_config* metadata = config_create(ruta);
    if (metadata == NULL) {
        log_error(loggerStorage, "Error cambiando estado metadata, no se pudo abrir el archivo metadata.config en la ruta %s", ruta);
        free(ruta);
        return false;
    }
    
    config_set_value(metadata, "ESTADO", estadoToString(estadoNuevo));
    config_save(metadata);
    config_destroy(metadata);
    free(ruta);
    return true;
}