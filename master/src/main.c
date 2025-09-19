int main(int argc, char* argv[]) {
    iniciar_config();              // crea loggerMaster también
    iniciarConexionesMaster();     // levanta el server y el aceptador

    // Bloquear el main para que el proceso no termine
    for(;;) pause();               // o pthread_exit(NULL);

    liberar_config();
    return 0;
}
