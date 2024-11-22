#include <getopt.h>
#include <limits.h>
#include <signal.h>
#include <sys/wait.h>

#include <pthread.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/types.h>

#include "common.h"
#include "sbuf.h"

int NTHREADS = 10;
sbuf_t sbuf;
bool jflag;

void *thread(void *vargp);
void *run_logic_side(void *arg);

void print_help(char *command)
{
	printf("Servidor base para RedFlix.\n");
	printf("uso:\n %s <puerto>\n", command);
	printf(" %s -h\n", command);
	printf("Opciones:\n");
	printf(" -h\t\t\tAyuda, muestra este mensaje\n");
}


int main(int argc, char **argv)
{
	int opt, index;

	//Sockets
	int listenfd;
	unsigned int clientlen;
	//Direcciones y puertos
	struct sockaddr_in clientaddr;
	char *port,*jvalue;

	while ((opt = getopt (argc, argv, "h")) != -1){
		switch(opt)
		{
			case 'h':
				print_help(argv[0]);
				return 0;
			
			default:
				fprintf(stderr, "uso: %s <puerto>\n", argv[0]);
				fprintf(stderr, "	 %s -h\n", argv[0]);
				return 1;
		}
	}
	
	port = argv[optind];

	if(argv == NULL){
		fprintf(stderr, "uso: %s <puerto>\n", argv[0]);
		fprintf(stderr, "	 %s -h\n", argv[0]);
		return 1;
	}

	//Valida el puerto
	int port_n = atoi(port);
	if(port_n <= 0 || port_n > USHRT_MAX){
		fprintf(stderr, "Puerto: %s invalido. Ingrese un número entre 1 y %d.\n", port, USHRT_MAX);
		return 1;
	}
    
	sbuf_init(&sbuf,10);
	
	listenfd = open_listenfd(port);

	if(listenfd < 0)
		connection_error(listenfd);

	printf("server escuchando en puerto %s...\n", port);

	pthread_t tid;

	int connfd;
	for(int i=0;i<NTHREADS;i++){
		if(pthread_create(&tid, NULL, run_logic_side, NULL) != 0){
            fprintf(stderr, "Error creating thread for client\n");
        }
	}
	while (1) {

		clientlen = sizeof(clientaddr);
        connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
        sbuf_insert(&sbuf,connfd);
        
	}
	
}
void *run_logic_side(void *arg) {

    int connfd = sbuf_remove(&sbuf);

    pid_t pid = fork();
    if (pid == -1) {
        perror("Error al hacer fork");
        pthread_exit(NULL);
    } else if (pid == 0) {
        // Este es el proceso hijo
        printf("Ejecutando ./logic_side en un hilo.\n");
        execl("./logic_side", "./logic_side", connfd, NULL);
        perror("Error al ejecutar ./logic_side"); // Solo se ejecuta si execl falla
        exit(EXIT_FAILURE);
    } else {
        // Proceso padre (hilo original)
        printf("Hilo esperando que el programa logic_side termine...\n");
        wait(NULL); // Espera a que el proceso hijo termine
    }
    pthread_exit(NULL);
}
