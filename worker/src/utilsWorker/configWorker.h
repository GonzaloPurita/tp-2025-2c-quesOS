#ifndef CONFIGURACION_WORKER_H_
#define CONFIGURACION_WORKER_H_

typedef struct t_config_worker {
    char* ip_master;
    char* ip_storage;
    char* algoritmo_reemplazo;
    char* path_scripts;
    char* log_level;
    int puerto_master;
    int puerto_storage;
    int tam_memoria;
    int retardo_memoria;
} t_config_worker;

extern t_config_worker* configWorker;

void iniciar_config(char* path);
void liberar_config();

#endif