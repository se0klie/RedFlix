#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFFER_SIZE 256

int connfd; // Socket del cliente
int running = 1; // Estado del cliente (1: funcionando, 0: terminado)

// Función para recibir frames del servidor
void *receive_frames(void *arg) {
    char buffer[BUFFER_SIZE];
    while (running) {
        ssize_t bytes = read(connfd, buffer, sizeof(buffer) - 1);
        if (bytes > 0) {
            buffer[bytes] = '\0'; // Agregar terminador de cadena
            printf("Frame recibido: %s\n", buffer);
        } else if (bytes == 0) {
            printf("Conexión cerrada por el servidor.\n");
            running = 0;
            break;
        } else {
            perror("Error al leer del socket");
            running = 0;
            break;
        }
    }
    return NULL;
}

// Función para enviar comandos al servidor
void *send_commands(void *arg) {
    char command[BUFFER_SIZE];
    while (running) {
        printf("\nComandos disponibles:\n");
        printf("1. PLAY\n");
        printf("2. PAUSE\n");
        printf("3. STOP\n");
        printf("4. CHANGE QUALITY (LOW, MEDIUM, HIGH)\n");
        printf("Escribe tu comando: ");
        fgets(command, sizeof(command), stdin);

        // Eliminar el salto de línea al final del comando
        size_t len = strlen(command);
        if (len > 0 && command[len - 1] == '\n') {
            command[len - 1] = '\0';
        }

        // Enviar el comando al servidor
        if (write(connfd, command, strlen(command)) < 0) {
            perror("Error al enviar el comando");
            running = 0;
            break;
        }

        // Si el comando es STOP, termina la ejecución
        if (strcmp(command, "STOP") == 0) {
            printf("Deteniendo cliente...\n");
            running = 0;
            break;
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <IP_SERVIDOR> <PUERTO>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *server_ip = argv[1];
    int server_port = atoi(argv[2]);

    // Crear socket
    connfd = socket(AF_INET, SOCK_STREAM, 0);
    if (connfd < 0) {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }

    // Configurar dirección del servidor
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Error en la dirección IP");
        close(connfd);
        exit(EXIT_FAILURE);
    }

    // Conectar al servidor
    if (connect(connfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error al conectar con el servidor");
        close(connfd);
        exit(EXIT_FAILURE);
    }

    printf("Conectado al servidor en %s:%d\n", server_ip, server_port);
    pthread_t recv_thread, cmd_thread;
    pthread_create(&recv_thread, NULL, receive_frames, NULL);
    pthread_create(&cmd_thread, NULL, send_commands, NULL);

    // Esperar a que los hilos terminen
    pthread_join(recv_thread, NULL);
    pthread_join(cmd_thread, NULL);

    // Cerrar conexión
    close(connfd);
    printf("Cliente terminado.\n");
    return 0;
}