#include "main.h"

void* recibirCliente(void* cliente);

int main(int argc, char* argv[]) {
    inicializarFS();
    iniciarServidor();
    return 0;
}