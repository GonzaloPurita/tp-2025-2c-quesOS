#include "fileTags.h"

bool crearFileTag(char* nombreFile, char* nombreTag) {
    // Validación de datos
    if (nombreFile == NULL || nombreTag == NULL) {
        log_error(loggerStorage, "Error creando file tag, el nombre del file o del tag es NULL");
        return false;
    }

    char* rutaFile = rutaCompleta("/files/");
    string_append(&rutaFile, nombreFile);
    crearDirectorio(rutaFile); // Me genera la carpeta con el nombre del file.
    return true;
}