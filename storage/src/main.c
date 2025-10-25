#include "main.h"

void* recibirCliente(void* cliente);

int main(int argc, char* argv[]) { 
      if (argc < 2) {
        fprintf(stderr, "Uso: %s <archivo_configuracion>\n", argv[0]);
        return 1;
    }

    char* archivoConfiguracion = argv[1]; 
    printf("Intentando cargar el archivo de configuración: %s\n", archivoConfiguracion);
    
    inicializarConfigStorage(archivoConfiguracion);
    inicializarFS();
    iniciarServidor();
    return 0;
}