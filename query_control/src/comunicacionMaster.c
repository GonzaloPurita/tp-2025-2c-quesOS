#include "comunicacionMaster.h"

void escucharMaster(int conexionMaster){
    while (1) {
        int cod_op = recibir_operacion(conexionMaster);
        if (cod_op == -1) {
            log_error(loggerQueryControl, "Se perdió la conexión con el Master.");
            break;
        }

        t_list* valores = recibir_paquete(conexionMaster);
        if (valores == NULL) {
            log_error(loggerQueryControl, "Error al recibir paquete del Master.");
            break;
        }

        switch(cod_op){
            case MENSAJE_LECTURA:
                // recibe <File:Tag> y <Contenido>
                if (list_size(valores) < 4) {
                    log_error(loggerQueryControl, "Paquete MENSAJE_LECTURA incompleto");
                    break;
                }
                char* file = list_get(valores, 0);
                int tamanio = *(int*) list_get(valores, 1);
                char* contenido = list_get(valores, 2);
                char* file_tag = list_get(valores, 3);

                // Usamos %.*s para decirle: "Imprimí contenido, pero solo hasta 'tamanio' caracteres"
                log_info(loggerQueryControl,"## Lectura realizada: Archivo %s:%s, contenido: %.*s", file, file_tag, tamanio, contenido);
                break;
            case RTA_SUBMIT_QUERY:
            // recibe: <Mensaje de confirmación>
                if (list_size(valores) < 1) {
                    log_error(loggerQueryControl, "Paquete RTA_SUBMIT_QUERY incompleto");
                }
                break;
            case QUERY_FINALIZADA:
            // recibe: <Motivo>
                if (list_size(valores) < 1) {
                    log_error(loggerQueryControl, "Paquete QUERY_FINALIZADA incompleto");
                    break;
                }

                char* motivo = list_get(valores, 0);
                log_info(loggerQueryControl, "## Query Finalizada - %s", motivo);
                list_destroy_and_destroy_elements(valores, free);
                return; //terminamos de escuchar xq la query termino
            default:
                log_warning(loggerQueryControl,"Código de operación desconocido recibido: %d", cod_op);
                list_destroy_and_destroy_elements(valores, free);
                return;
        }
        list_destroy_and_destroy_elements(valores, free);
    }
}