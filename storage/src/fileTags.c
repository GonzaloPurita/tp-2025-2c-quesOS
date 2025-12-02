#include "fileTags.h"

// Funciones privadas
bool crearMetaData(char* path);
bool eliminarHardlink(char* rutaBloqueLogico, int numeroBloqueLogico, int bloqueFisico, int query_id);
void free_wrapper(char* line);
op_code crearDirectorioYMetaData(char* rutaBase, char* nombreTag);
// Nueva función auxiliar para actualizar la metadata de BLOCKS
static bool setBloqueEnMetadata(char* file, char* tag, int numeroBloqueLogico, int numeroBloqueFisico);

bool crearFileInicial() {
    if (!crearFileTag("initial_file", "BASE")) {
        return false;
    }
    if (!agregarBloqueLogicoHL("initial_file", "BASE", 0, 0, 0)) {
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
    t_config* metadata = getMetaData(nombreFile, nombreTag);
    if (metadata != NULL) {
        log_error(loggerStorage, "Error creando file tag, el file tag %s:%s ya existe", nombreFile, nombreTag);
        config_destroy(metadata);
        return ERROR_FILE_TAG_EXISTS;
    }

    // Creamos la ruta files
    char* rutaFile = rutaCompleta("/files/");
    string_append(&rutaFile, nombreFile);
    if (crearDirectorio(rutaFile) == -1) { // Me genera la carpeta con el nombre del file.
        free(rutaFile);
        return OP_FAILED;
    }

    // Crear directorio y metadata para el tag
    op_code resultado = crearDirectorioYMetaData(rutaFile, nombreTag);
    free(rutaFile);
    return resultado;
}

op_code crearTag(char* nombreFile, char* nombreTag) {
    // Validación de datos
    if (nombreFile == NULL || nombreTag == NULL) {
        log_error(loggerStorage, "Error creando tag, el nombre del file o del tag es NULL");
        return OP_FAILED;
    }
    if (string_is_empty(nombreFile) || string_is_empty(nombreTag)) {
        log_error(loggerStorage, "Error creando tag, el nombre del file o del tag está vacío");
        return OP_FAILED;
    }

    // Verifico que el File exista
    char* rutaFile = rutaCompleta("/files/");
    string_append(&rutaFile, nombreFile);
    if (!directorioExiste(rutaFile)) {
        log_error(loggerStorage, "Error creando tag, el file %s no existe", nombreFile);
        free(rutaFile);
        return ERROR_FILE_NOT_FOUND;
    }

    // Crear directorio y metadata para el tag
    op_code resultado = crearDirectorioYMetaData(rutaFile, nombreTag);
    free(rutaFile);
    return resultado;
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
    dictionary_put(data, "BLOCKS", "[]");
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
        log_warning(loggerStorage, "La ruta %s no existe", ruta);
        free(ruta);
        return NULL;
    }

    return ruta;
}

bool agregarBloqueLogicoHL(char* nombreFile, char* nombreTag, int numeroBloqueLogico, int numeroBloqueFisico, int query_id) {
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

    if(!crearHardlink(rutaBloqueLogico, numeroBloqueLogico, numeroBloqueFisico, query_id)) {
        free(rutaBloqueLogico);
        return false;
    }

    log_info(loggerStorage, "##%d - %s:%s Se agregó el hard link del bloque lógico %d al bloque físico %d", query_id, nombreFile, nombreTag, numeroBloqueLogico, numeroBloqueFisico);

    free(rutaBloqueLogico);

    // Actualizo metadata BLOCKS en el índice correspondiente
    if (!setBloqueEnMetadata(nombreFile, nombreTag, numeroBloqueLogico, numeroBloqueFisico)) {
        log_warning(loggerStorage, "No se pudo actualizar metadata BLOCKS para %s:%s[%d]", nombreFile, nombreTag, numeroBloqueLogico);
    }

    return true;
}

bool eliminarBloqueLogicoHL(char* nombreFile, char* nombreTag, int numeroBloqueLogico, int query_id) {
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

    log_info(loggerStorage, "##%d - %s:%s Se eliminó el hard link del bloque lógico %d al bloque físico %d", query_id, nombreFile, nombreTag, numeroBloqueLogico, bloqueFisico);

    bool ok = eliminarHardlink(rutaBloqueLogico, numeroBloqueLogico, bloqueFisico, query_id);
    free(rutaBloqueLogico);

    // Pongo 0 en metadata para ese índice si salió bien
    if (ok) {
        if (!setBloqueEnMetadata(nombreFile, nombreTag, numeroBloqueLogico, 0)) {
            log_warning(loggerStorage, "No se pudo actualizar metadata BLOCKS (0) para %s:%s[%d]", nombreFile, nombreTag, numeroBloqueLogico);
        }
    }

    return ok;
}

bool eliminarHardlink(char* rutaBloqueLogico, int numeroBloqueLogico, int bloqueFisico, int query_id) {
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
        log_info(loggerStorage, "##%d - Bloque Físico Liberado - Número de Bloque: %d", query_id, bloqueFisico);
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
    char** bloques = config_get_array_value(metadata, "BLOCKS");
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

op_code crearDirectorioYMetaData(char* rutaBase, char* nombreTag) {
    // Trabajar sobre una copia para no invalidar el puntero original del caller
    char* rutaTag = string_duplicate(rutaBase);
    string_append(&rutaTag, "/");
    string_append(&rutaTag, nombreTag);
    if (crearDirectorio(rutaTag) == -1) { // Me genera la carpeta con el nombre del tag.
        free(rutaTag);
        return OP_FAILED;
    }

    // Creo el metadata.config para el tag
    if (!crearMetaData(rutaTag)) {
        free(rutaTag);
        return OP_FAILED;
    }

    // Creamos la ruta logical_blocks
    string_append(&rutaTag, "/logical_blocks");
    if (crearDirectorio(rutaTag) == -1) { // Me genera la carpeta logical_blocks.
        free(rutaTag);
        return OP_FAILED;
    }

    free(rutaTag);
    return OP_SUCCESS;
}

op_code validarBloqueLogico(char* nombreFile, char* nombreTag, int numeroBloqueLogico, int query_id) {
    int bloqueFisico = obtenerBloqueFisico(nombreFile, nombreTag, numeroBloqueLogico);
    if (bloqueFisico == -1) {
        log_error(loggerStorage, "Error validando bloque lógico %d de %s:%s, no existe bloque físico asociado", numeroBloqueLogico, nombreFile, nombreTag);
        return ERROR_OUT_OF_BOUNDS;
    }
    if(bloqueFisico == 0) {
        return OP_SUCCESS;
    }

    char* contenido = leerBloqueFisico(bloqueFisico);
    if (contenido == NULL) {
        log_error(loggerStorage, "Error leyendo contenido del bloque físico %d", bloqueFisico);
        return OP_FAILED;
    }

    char* hash = crypto_md5(contenido, superblock->blocksize);
    free(contenido);

    if(config_has_property(hashMap, hash)) {
        char* bloqueConEseHash = config_get_string_value(hashMap, hash);
        int nroBloqueConEseHash = obtenerNumeroBloqueFisico(bloqueConEseHash);
        
        // Verificar que no sea el mismo bloque físico (evitar reemplazo innecesario)
        if (nroBloqueConEseHash == bloqueFisico) {
            free(hash);
            return OP_SUCCESS;
        }

        log_info(loggerStorage, "##%d - %s:%s Bloque Lógico %d se reasigna de %d a %d", query_id, nombreFile, nombreTag, numeroBloqueLogico, bloqueFisico, nroBloqueConEseHash);
        
        // Eliminar el hardlink antiguo (esto puede liberar el bloque si es único)
        if (!eliminarBloqueLogicoHL(nombreFile, nombreTag, numeroBloqueLogico, query_id)) {
            log_error(loggerStorage, "Error eliminando hardlink del bloque %d", bloqueFisico);
            free(hash);
            return OP_FAILED;
        }
        
        // Agregar hardlink al bloque con el mismo hash
        if (!agregarBloqueLogicoHL(nombreFile, nombreTag, numeroBloqueLogico, nroBloqueConEseHash, query_id)) {
            log_error(loggerStorage, "Error agregando hardlink al bloque %d", nroBloqueConEseHash);
            free(hash);
            return OP_FAILED;
        }
    } else {
        // El hash no existe, agregarlo al hashMap
        char* nombreBloque = crearNombreBloque(bloqueFisico);
        config_set_value(hashMap, hash, nombreBloque);
        config_save(hashMap);
        log_debug(loggerStorage, "Nuevo hash agregado al hashMap: %s -> %s", hash, nombreBloque);
        free(nombreBloque);
    }
    
    free(hash);
    return OP_SUCCESS;
}

// Implementación auxiliar: setea/expande BLOCKS y guarda metadata
static bool setBloqueEnMetadata(char* file, char* tag, int numeroBloqueLogico, int numeroBloqueFisico) {
    if (file == NULL || tag == NULL || numeroBloqueLogico < 0) return false;

    t_config* metadata = getMetaData(file, tag);
    if (metadata == NULL) return false;

    char** bloques = config_get_array_value(metadata, "BLOCKS");

    // Calcular longitud actual
    int lenActual = 0;
    if (bloques != NULL) {
        while (bloques[lenActual] != NULL) lenActual++;
    }

    int lenNuevo = (numeroBloqueLogico + 1 > lenActual) ? numeroBloqueLogico + 1 : lenActual;

    // Construir string del array actualizado
    char* bloquesActualizados = string_new();
    string_append(&bloquesActualizados, "[");
    for (int i = 0; i < lenNuevo; i++) {
        int valor = 0;
        if (i < lenActual && bloques != NULL) {
            valor = atoi(bloques[i]);
        }
        if (i == numeroBloqueLogico) {
            // log_info(loggerStorage, "DEBUG LOOP: i=%d Coincide! Cambiando valor %d por %d", i, valor, numeroBloqueFisico);
            valor = numeroBloqueFisico;
        }
        char* svalor = string_itoa(valor);
        string_append(&bloquesActualizados, svalor);
        free(svalor);
        if (i < lenNuevo - 1) string_append(&bloquesActualizados, ",");
    }
    string_append(&bloquesActualizados, "]");

    config_set_value(metadata, "BLOCKS", bloquesActualizados);
    if(config_save(metadata) < 0) {
        log_error(loggerStorage, "Error guardando metadata para %s:%s", file, tag);
    }

    // Liberaciones
    if (bloques != NULL) {
        string_iterate_lines(bloques, free_wrapper);
        free(bloques);
    }
    free(bloquesActualizados);
    config_destroy(metadata);

    return true;
}

op_code duplicarFileTag(char* fileOrigen, char* tagOrigen, char* fileDestino, char* tagDestino, int query_id) {
    // Validar que el tag origen exista
    t_config* metaDataOriginal = getMetaData(fileOrigen, tagOrigen);
    if(metaDataOriginal == NULL) {
        log_error(loggerStorage,"El tag origen %s:%s no existe", fileOrigen, tagOrigen);
        return ERROR_FILE_NOT_FOUND;
    }

    // Copiar los bloques del tag origen al tag destino
    char** blocks = config_get_array_value(metaDataOriginal, "BLOCKS");
    int bloquesCount = 0;
    while(blocks[bloquesCount] != NULL) {
        bool resultado = agregarBloqueLogicoHL(fileDestino, tagDestino, bloquesCount, atoi(blocks[bloquesCount]), query_id);
        if (!resultado) {
            log_error(loggerStorage, "Error al agregar bloque lógico %d al tag destino %s:%s", bloquesCount, fileDestino, tagDestino);
            string_array_destroy(blocks);
            config_destroy(metaDataOriginal);
            return OP_FAILED;
        }
        bloquesCount++;
    }

    // Actualizar metadata del tag destino
    t_config* metaDataDestino = getMetaData(fileDestino, tagDestino);
    if (metaDataDestino == NULL) {
        log_error(loggerStorage, "Error al obtener metadata del tag destino %s:%s", fileDestino, tagDestino);
        string_array_destroy(blocks);
        config_destroy(metaDataOriginal);
        return OP_FAILED;
    }

    config_set_value(metaDataDestino, "TAMAÑO", config_get_string_value(metaDataOriginal, "TAMAÑO"));
    config_set_value(metaDataDestino, "ESTADO", "WORK_IN_PROGRESS");
    config_save(metaDataDestino);

    string_array_destroy(blocks);
    config_destroy(metaDataOriginal);
    config_destroy(metaDataDestino);
    
    return OP_SUCCESS;
}