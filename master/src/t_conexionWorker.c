void workers_iniciar(void) {
    LISTA_WORKERS = list_create();
}

void workers_destruir(void) {
    if (!LISTA_WORKERS) return;
    pthread_mutex_lock(&mutex_workers);
    for (int i = 0; i < list_size(LISTA_WORKERS); i++) {
        t_conexionWorker* w = list_get(LISTA_WORKERS, i);
        if (w->conectado && w->fd >= 0) close(w->fd);
        free(w);
    }
    list_destroy(LISTA_WORKERS);
    LISTA_WORKERS = NULL;
    pthread_mutex_unlock(&mutex_workers);
}

void worker_registrar(int id, int fd) {
    t_conexionWorker* w = malloc(sizeof(*w));
    w->id = id;
    w->fd = fd;
    w->conectado = true;
    w->qid_actual = -1;

    pthread_mutex_lock(&mutex_workers);
    list_add(LISTA_WORKERS, w);
    pthread_mutex_unlock(&mutex_workers);

    if (loggerMaster) log_info(loggerMaster, "Worker %d conectado (fd=%d)", id, fd);
}

void worker_eliminar_por_fd(int fd) {
    pthread_mutex_lock(&mutex_workers);
    for (int i = 0; i < list_size(LISTA_WORKERS); i++) {
        t_conexionWorker* w = list_get(LISTA_WORKERS, i);
        if (w->fd == fd) {
            if (loggerMaster) log_warning(loggerMaster, "Worker %d desconectado (fd=%d)", w->id, fd);
            if (w->conectado && w->fd >= 0) close(w->fd);
            list_remove(LISTA_WORKERS, i);
            free(w);
            break;
        }
    }
    pthread_mutex_unlock(&mutex_workers);
}


void worker_marcar_libre_por_fd(int fd) {
    pthread_mutex_lock(&mutex_workers);
    for (int i = 0; i < list_size(LISTA_WORKERS); i++) {
        t_conexionWorker* w = list_get(LISTA_WORKERS, i);
        if (w->fd == fd) { w->qid_actual = -1; break; }
    }
    pthread_mutex_unlock(&mutex_workers);
}

int workers_conectados(void) {
    pthread_mutex_lock(&mutex_workers);
    int n = LISTA_WORKERS ? list_size(LISTA_WORKERS) : 0;
    pthread_mutex_unlock(&mutex_workers);
    return n;
}

int workers_disponibles(void) {
    int libres = 0;
    pthread_mutex_lock(&mutex_workers);
    for (int i = 0; i < list_size(LISTA_WORKERS); i++) {
        t_conexionWorker* w = list_get(LISTA_WORKERS, i);
        if (w->conectado && w->qid_actual == -1) libres++;
    }
    pthread_mutex_unlock(&mutex_workers);
    return libres;
}


int worker_id_por_fd(int fd) {
    int id = -1;
    pthread_mutex_lock(&mutex_workers);
    for (int i = 0; i < list_size(LISTA_WORKERS); i++) {
        t_conexionWorker* w = list_get(LISTA_WORKERS, i);
        if (w->fd == fd) { id = w->id; break; }
    }
    pthread_mutex_unlock(&mutex_workers);
    return id;
}

void worker_eliminar_por_id(int id) {
    pthread_mutex_lock(&mutex_workers);
    for (int i = 0; i < list_size(LISTA_WORKERS); i++) {
        t_conexionWorker* w = list_get(LISTA_WORKERS, i);
        if (w->id == id) {
            if (loggerMaster) log_warning(loggerMaster, "Worker %d desconectado (fd=%d)", w->id, w->fd);
            if (w->conectado && w->fd >= 0) close(w->fd);
            list_remove(LISTA_WORKERS, i);
            free(w);
            pthread_mutex_unlock(&mutex_workers);
            return;
        }
    }
    pthread_mutex_unlock(&mutex_workers);
    if (loggerMaster) log_warning(loggerMaster, "Se intentó eliminar Worker id=%d que no existe", id);
}

void* atenderWorker(void arg){
    //TODO
}