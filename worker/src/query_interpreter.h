#ifndef QUERY_INTERPRETER_H_
#define QUERY_INTERPRETER_H_

#include "main.h"
#include "globales.h"

typedef enum {
    OP_CREATE,
    OP_TRUNCATE,
    OP_WRITE,
    OP_READ,
    OP_FLUSH,
    OP_COMMIT,
    OP_DELETE,
    OP_TAG,
    OP_END,
    OP_UNKNOWN
} t_opcode;

typedef struct {
    t_opcode opcode;
    char** parametros;   // lista de parámetros como strings
    int num_parametros;
} t_instruccion;


void recibir_queries();
void ejercutar_query(char* path_query);
t_instruccion* decode(char* instruccion_raw);
t_opcode obtener_opcode(char* nombre);
void execute(t_instruccion* inst);
void destruir_instruccion(t_instruccion* inst);

#ifndef /* QUERY_INTERPRETER_H_ */