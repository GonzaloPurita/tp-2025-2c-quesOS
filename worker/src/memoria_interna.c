include "memoria_interna.h"

t_list* paginas_necesarias(int direccion_base, int tamanio) {
    t_list* paginas = list_create();

    int primera = direccion_base / TAM_PAGINA;
    int ultima = (direccion_base + tamanio - 1) / TAM_PAGINA;

    for (int p = primera; p <= ultima; p++) {
        int* nro = malloc(sizeof(int)); // guardo punteros porque t_list maneja punteros
        *nro = p;
        list_add(paginas, nro);
    }
    return paginas;
}

bool esta_en_memoria(t_formato* formato, int nro_pagina) {
    char* clave = string_itoa(nro_pagina);
    tabla_pag* tabla = dictionary_get(diccionario_tablas, string_from_format("%s:%s", formato->file_name, formato->tag));

    if (tabla == NULL) {
        free(clave);
        log_error(loggerWorker, "No existe tabla de páginas para %s:%s", formato->file_name, formato->tag);
        return false;
    }

    entrada_pag* entrada = dictionary_get(tabla->paginas, clave);
    free(clave);

    if (entrada == NULL) {
        log_error(loggerWorker, "No existe entrada para la página %d en %s:%s", nro_pagina, formato->file_name, formato->tag);
        return false;
    } 

    return entrada->presente;
}

char* leer_desde_memoria(t_formato* formato, int direccion_base, int tamanio) {
    char* buffer = calloc(1, tamanio + 1); // el buffer arranca en 0, con un solo bloque de memoria, y con un tamaño

    int pagina_inicio = direccion_base / TAM_PAGINA;
    int pagina_fin = (direccion_base + tamanio - 1) / TAM_PAGINA;
    int offset = direccion_base % TAM_PAGINA;
    int bytes_restantes = tamanio;
    int posicion_buffer = 0;

    for (int p = pagina_inicio; p <= pagina_fin; p++) {
        char* clave = string_itoa(p);
        tabla_pag* tabla = dictionary_get(diccionario_tablas, string_from_format("%s:%s", formato->file_name, formato->tag));
        entrada_pag* entrada = dictionary_get(tabla->paginas, clave);
        free(clave);

        int frame = entrada->indice_frame;
        int dir_fisica = frame * TAM_PAGINA + offset;

        int bytes_a_usar = (bytes_restantes < TAM_PAGINA - offset) ? bytes_restantes : TAM_PAGINA - offset; // los bytes a usar van ser los que quedan o los que entran en la página

        memcpy(buffer + posicion_buffer, MEMORIA + dir_fisica, bytes_a_usar);

        bytes_restantes -= bytes_a_usar;
        posicion_buffer += bytes_a_usar;
        offset = 0; // a partir de la segunda página empieza desde 0, porque antes tal vez arrancaba con la primer pagina empezada, pero ya la segunda si o si arranc de cero
    }

    return buffer;
}

void escribir_en_memoria(t_formato* formato, int direccion_base, char* valor) {
    int tamanio_valor = strlen(valor);
    int pagina_inicio = direccion_base / TAM_PAGINA;
    int pagina_fin = (direccion_base + tamanio_valor - 1) / TAM_PAGINA;
    int offset = direccion_base % TAM_PAGINA;
    int bytes_restantes = tamanio_valor;
    int posicion_valor = 0;

    for (int p = pagina_inicio; p <= pagina_fin; p++) {
        char* clave = string_itoa(p);
        tabla_pag* tabla = dictionary_get(diccionario_tablas, string_from_format("%s:%s", formato->file_name, formato->tag));
        entrada_pag* entrada = dictionary_get(tabla->paginas, clave);
        free(clave);

        int frame = entrada->indice_frame;
        int dir_fisica = frame * TAM_PAGINA + offset;

        int bytes_a_usar = (bytes_restantes < TAM_PAGINA - offset) ? bytes_restantes : TAM_PAGINA - offset;

        memcpy(MEMORIA + dir_fisica, valor + posicion_valor, bytes_a_usar); // estoy guardando la posicion en donde se queda el string valor porque ese string puede quedar repartido en varias paginas

        entrada->modificado = true; // marco como la entrada como modificada
        frames[frame].modificado = true; // marco el frame como modificado

        bytes_restantes -= bytes_a_usar;
        posicion_valor += bytes_a_usar;
        offset = 0;
    }
}

int obtener_marco_libre_o_victima() {
    for (int i = 0; i < CANTIDAD_MARCOS; i++) {
        if (!frames[i].ocupado) return i;
    }
    return elegir_victima(); // según algoritmo de reemplazo
}

int elegir_victima() {
    // TODO: implementar algoritmo de reemplazo
    for (int i = 0; i < CANTIDAD_MARCOS; i++) {
        if (frames[i].ocupado) return i;
    }
    return 0;
}

void pedir_pagina_a_storage(t_formato* formato, int nro_pagina){
    int marco = obtener_marco_libre_o_victima();
    int base_pagina = nro_pagina * TAM_PAGINA;

    t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion = PED_PAG;
    agregar_a_paquete(paquete, formato->file_name, strlen(formato->file_name) + 1);
    agregar_a_paquete(paquete, formato->tag, strlen(formato->tag) + 1);
    agregar_a_paquete(paquete, &base_pagina, sizeof(int));
    enviar_paquete(paquete, conexionStorage);
    eliminar_paquete(paquete);

    op_code cod_op = recibir_operacion(conexionStorage);
    if (cod_op == PED_PAG) {
        t_list* lista = recibir_paquete(conexionStorage);
        char* contenido = list_get(lista, 0);

        memcpy(MEMORIA + marco * TAM_PAGINA, contenido, TAM_PAGINA); // copio el contenido recibido en el marco asignado

        // seteo el frame
        frames[marco].ocupado = true;
        frames[marco].modificado = false;
        frames[marco].uso = true;
        frames[marco].file = strdup(formato->file_name);
        frames[marco].tag = strdup(formato->tag);
        frames[marco].page_num = nro_pagina;

        // actualizo la tabla de páginas
        char* clave = string_itoa(nro_pagina);
        tabla_pag* tabla = dictionary_get(diccionario_tablas, string_from_format("%s:%s", formato->file_name, formato->tag));
        entrada_pag* entrada = malloc(sizeof(entrada_pag));
        entrada->indice_frame = marco;
        entrada->presente = true;
        entrada->modificado = false;
        entrada->uso = true;
        dictionary_put(tabla->paginas, clave, entrada);

        free(clave);
        list_destroy_and_destroy_elements(lista, free);

        log_debug(loggerWorker, "Contenido de página %d recibido desde Storage en marco %d", nro_pagina, marco);
    }
}