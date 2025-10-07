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
                if (list_size(valores) < 2) {
                    log_error(loggerQueryControl, "Paquete MENSAJE_LECTURA incompleto");
                    list_destroy_and_destroy_elements(valores, free);
                    break;
                }
                char* file_tag = list_get(valores, 0);
                char* contenido = list_get(valores, 1);

                log_info(loggerQueryControl,"## Lectura realizada: Archivo %s, contenido: %s", file_tag, contenido);
                break;
            case QUERY_FINALIZADA:
            // recibe: <Motivo>
                if (list_size(valores) < 1) {
                    log_error(loggerQueryControl, "Paquete QUERY_FINALIZADA incompleto");
                    list_destroy_and_destroy_elements(valores, free);
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