#include "main.h"

int main(int argc, char* argv[]) {
    iniciar_config();              // crea loggerMaster también
    iniciarConexionesMaster();     // levanta el server y el aceptador
    return 0;
}
