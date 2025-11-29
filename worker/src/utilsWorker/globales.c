#include "globales.h"
#include <stdlib.h>
#include "configWorker.h"
#include <pthread.h>

// Conexiones globales
int conexionMaster = 0;
int conexionStorage = 0;

t_log* loggerWorker;
t_config_worker* configWorker;

bool interrupt_flag = false;
bool query_error_flag = false;

// Datos globales
int TAM_PAGINA;
int TAM_MEMORIA;
int CANTIDAD_MARCOS;
void* MEMORIA;

int TAM_BLOQUE;
int PC_ACTUAL;

int puntero_clock = 0;

frame* frames;

pthread_mutex_t mutex_memoria;

// Diccionarios
t_dictionary* diccionario_tablas; // clave = "file:tag", valor = tabla_pag*

void iniciar_memoria(){
    TAM_PAGINA = TAM_BLOQUE;                // viene de Storage
    TAM_MEMORIA = configWorker->tam_memoria;    // viene del archivo de config
    CANTIDAD_MARCOS = TAM_MEMORIA / TAM_PAGINA;
    MEMORIA = calloc(1, TAM_MEMORIA); // memoria inicializada en 0, y solo se reserva un bloque de memoria

    if (!MEMORIA) {
        log_error(loggerWorker, "Error al reservar memoria interna");
        exit(1);
    }

    frames = calloc(CANTIDAD_MARCOS, sizeof(frame));

    pthread_mutex_init(&mutex_memoria, NULL);

   diccionario_tablas = dictionary_create();

    log_debug(loggerWorker, "Memoria interna inicializada. TAM_MEMORIA=%d, TAM_PAGINA=%d, MARCOS=%d", TAM_MEMORIA, TAM_PAGINA, CANTIDAD_MARCOS);
}