#include "main.h"

void* recibirCliente(void* cliente);

int main(int argc, char* argv[]) { // TODO: Recibir el archivo de configuración por parámetro
    inicializarFS();
    iniciarServidor();
    return 0;
}