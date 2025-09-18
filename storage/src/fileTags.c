#include "fileTags.h"

// Funciones privadas
bool crearMetaData(char* path);
bool crearHardlink(char* rutaBloqueLogico, int numeroBloqueLogico, int numeroBloqueFisico);

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
}

bool crearFileTag(char* nombreFile, char* nombreTag) {
    // Validación de datos
    if (nombreFile == NULL || nombreTag == NULL) {
        log_error(loggerStorage, "Error creando file tag, el nombre del file o del tag es NULL");
        return false;
    }
    if (string_is_empty(nombreFile) || string_is_empty(nombreTag)) {
        log_error(loggerStorage, "Error creando file tag, el nombre del file o del tag está vacío");
        return false;
    }

    // Creamos la ruta files
    char* rutaFile = rutaCompleta("/files/");
    string_append(&rutaFile, nombreFile);
    if(crearDirectorio(rutaFile) == -1) { // Me genera la carpeta con el nombre del file.
        free(rutaFile);
        return false;
    }

    // Creamos la ruta tag
    string_append(&rutaFile, "/");
    string_append(&rutaFile, nombreTag);
    if(crearDirectorio(rutaFile) == -1) { // Me genera la carpeta con el nombre del tag.
        free(rutaFile);
        return false;
    }

    // Creo el metadata.config para el tag
    if(!crearMetaData(rutaFile)) { 
        free(rutaFile);
        return false;
    }

    // Creamos la ruta logical_blocks
    string_append(&rutaFile, "/logical_blocks");
    if(crearDirectorio(rutaFile) == -1) { // Me genera la carpeta logical_blocks.
        free(rutaFile);
        return false;
    }

    free(rutaFile);
    return true;
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
    dictionary_put(data, "TAMAÑO", 0);
    dictionary_put(data, "BLOQUES", "[]");
    dictionary_put(data, "ESTADO", "WORK_IN_PROGRESS");
    metadata->properties = data;

    // Construimos el path del metadata.config
    metadata->path = string_duplicate(path);
    string_append(&metadata->path, "/metadata.config");

    // Guardamos el archivo
    if(config_save_in_file(metadata, metadata->path) == -1) {
        log_error(loggerStorage, "Error creando metadata, no se pudo guardar en el archivo");
        config_destroy(metadata);
        return false;
    }
    config_destroy(metadata);
    return true;
}

bool crearHardlink(char* rutaBloqueLogico, int numeroBloqueLogico, int numeroBloqueFisico) {
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
    char* nombre = crearNombreBloque(numeroBloqueLogico);
    string_append(&rutaBloqueLogico, nombre);
    free(nombre);

    int linkResult = link(rutaBloqueFisico, rutaBloqueLogico); // Esto genera el hard link
    if (linkResult == -1) {
        log_error(loggerStorage, "Error creando hard link, no se pudo crear el enlace entre %s y %s", rutaBloqueFisico, rutaBloqueLogico);
        free(rutaBloqueFisico);
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

// Funciones genéricas de metadata

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