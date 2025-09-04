#include "globales.h"

// Conexiones globales
int conexionMaster;
int conexionStorage;

// Datos globales
int TAM_PAGINA = 0;
int TAM_MEMORIA = 0;
int CANTIDAD_MARCOS = 0;
void* MEMORIA = NULL;

int TAM_BLOQUE;

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