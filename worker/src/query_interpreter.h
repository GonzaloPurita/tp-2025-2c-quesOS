#ifndef QUERY_INTERPRETER_H_
#define QUERY_INTERPRETER_H_

#include "utilsWorker/api_memoria.h"
#include "utilsWorker/configWorker.h"
#include <utilsWorker/globales.h>
#include <commons/string.h>
#include <unistd.h>
#include <utils/paquete.h>
#include <utilsWorker/globales.h>

typedef enum {
    CREATE,
    TRUNCATE,
    WRITE,
    READ,
    FLUSH,
    COMMIT,
    DELETE,
    TAG,
    END,
    UNKNOWN
} t_opcode;

typedef enum {
    QUERY_EXITO,
    QUERY_DESALOJADA,
    QUERY_ERROR
} t_estado_query;

typedef struct {
    t_opcode opcode;
    char** parametros;   // lista de parámetros como strings
    int num_parametros;
} t_instruccion;

typedef struct {
    int index_frame;
    char* file;
    char* tag;
    int page_num;
    void* contenido;
} t_pagina_a_guardar;

// typedef struct{ // MATERIAS:BASE
//     char* file_name; //ej: "MATERIAS"
//     char* tag; //ej "BASE"
// } t_formato;

 typedef struct t_query_context {
    int query_id;
    char* nombre_query;
    int pc_inicial;    
} t_query_context;

typedef struct {
    int query_id;
    char* path_script;
} t_hilo_args;

extern t_query_context* query_actual;

void recibir_queries();
t_estado_query ejecutar_query(char* path_query);
t_instruccion* decode(char* instruccion_raw);
t_opcode obtener_opcode(char* nombre);
void execute(t_instruccion* inst);
void destruir_instruccion(t_instruccion* inst);
t_formato* mapear_formato(char* recurso);
void destruir_formato(t_formato* formato);
void destruir_query_context(t_query_context* ctx);
void flush_paginas_modificadas_de_tabla(tabla_pag* tabla, t_formato* formato);
void manejar_respuesta_storage(op_code respuesta, char* operacion);
void notificar_error_a_master(char* motivo);
void* correr_query_en_hilo(void* arg);
char* generar_clave(t_formato* f);

//ejecutar instrucciones
void ejecutar_create(t_instruccion* inst);
void ejecutar_truncate(t_instruccion* inst);
void ejecutar_write(t_instruccion* inst);
void ejecutar_read(t_instruccion* inst);
void ejecutar_tag(t_instruccion* inst);
void ejecutar_commit(t_instruccion* inst);
void ejecutar_flush(t_instruccion* inst);
void ejecutar_delete(t_instruccion* inst);
void ejecutar_end(t_instruccion* inst);

//void pedir_pagina_a_storage(t_formato* formato, int nro_pagina);

#endif