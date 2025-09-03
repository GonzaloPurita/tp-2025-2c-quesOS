#include "main.h"

int main(int argc, char* argv[]) {
    iniciar_config()

    char* puertoMaster = string_itoa(configWorker->puerto_master);
    int conexionMaster = crearConexionCliente(configWorker->ip_master, puertoMaster);
    free(puertoMaster);

    


    return 0;
}
