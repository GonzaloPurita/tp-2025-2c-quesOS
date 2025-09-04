#include "globales.h"

// Conexiones globales
int conexionMaster;
int conexionStorage;

// Datos globales
int TAM_PAGINA;
int TAM_MEMORIA;
int CANTIDAD_MARCOS;
void* MEMORIA;

int TAM_BLOQUE;
int PC_ACTUAL; //inicializo en 0

void iniciar_memoria(){
    TAM_PAGINA = TAM_BLOQUE;                // viene de Storage
    TAM_MEMORIA = configWorker->tam_memoria;    // viene del archivo de config
    CANTIDAD_MARCOS = TAM_MEMORIA / TAM_PAGINA;
    MEMORIA = malloc(TAM_MEMORIA);

    if (!MEMORIA) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    log_debug(loggerWorker, "Memoria interna inicializada. TAM_MEMORIA=%d, TAM_PAGINA=%d, MARCOS=%d", TAM_MEMORIA, TAM_PAGINA, CANTIDAD_MARCOS);
}