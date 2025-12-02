#include "main.h"

int main(int argc, char* argv[]) {
    char* archivoConfiguracion = argv[1];

    iniciar_config(archivoConfiguracion);   // crea loggerMaster también
    inicializarListasQueries();
    iniciarConexionesMaster();     // levanta el server y el aceptador
    return 0;
}
