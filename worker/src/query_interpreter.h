#ifndef QUERY_INTERPRETER_H_
#define QUERY_INTERPRETER_H_

#include "main.h"
#include "globales.h"
#include "paquete.h"
#include "pedidos.h"

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

typedef struct {
    t_opcode opcode;
    char** parametros;   // lista de parámetros como strings
    int num_parametros;
} t_instruccion;

typedef struct{ // MATERIAS:BASE
    char* file_name; //ej: "MATERIAS"
    char* tag; //ej "BASE"
} t_formato;

 typedef struct {
    int query_id;
    char* nombre_query;
    int pc_inicial;    
} t_query_context;


void recibir_queries();
void ejercutar_query(char* path_query);
t_instruccion* decode(char* instruccion_raw);
t_opcode obtener_opcode(char* nombre);
void execute(t_instruccion* inst);
void destruir_instruccion(t_instruccion* inst);
t_formato* mapear_formato(t_instruccion* inst);
void destruir_formato(t_formato* formato);


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

void pedir_pagina_a_storage(t_formato* formato, int nro_pagina);

#ifndef /* QUERY_INTERPRETER_H_ */