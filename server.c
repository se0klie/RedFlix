#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <limits.h>
#include <pthread.h>
#include <semaphore.h>
#include "common.h"
#include "sbuf.h"
#include "linkedlist.h"

#define BUFFERSIZE 1024
typedef enum {
    LOW, MEDIUM, HIGH
} Bitrate;

typedef enum{
    PLAY,STOP,PAUSE
} Command;

typedef struct{
    char *videoName;
    Bitrate actualBitrate;
    Command actualCommand;
    int bitrateChanged;
} Video;

Video *video;
Linkedlist *streamerBuf;
Linkedlist *visorBuf;

typedef struct{
    sem_t full;
    sem_t empty;
} Side;


Side *visorSide;
Side *streamerSide;

sem_t encoderMutex;
sem_t stateMutex;

void *streamer();
void *encoder();
void *visor();

void *initVideo();

int main(int argc, char **argv) {
    // if(argc != 2){
    //     return 1;
    // }

    initVideo();
    visorBuf = createLinkedlist();
    streamerBuf = createLinkedlist();

    streamerSide = (Side *) malloc(sizeof( Side));
    visorSide = (Side *) malloc(sizeof(Side));

    sem_init(&streamerSide->empty,0,1);
    sem_init(&streamerSide->full,0,0);

    sem_init(&encoderMutex,0,1);

    sem_init(&visorSide->empty,0,1);
    sem_init(&visorSide->full,0,0);

    pthread_t threads[3];

    pthread_create(&threads[0], NULL, encoder, NULL);
    pthread_create(&threads[1], NULL, streamer, NULL);
    pthread_create(&threads[2], NULL, visor, (void *) argv[1]);
    // pthread_create(&threads[2], NULL, visor, NULL);

    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);
    pthread_join(threads[2], NULL);

    printf("Video stopped and closed.\n");
    return 0;
}

void *initVideo(){
    video = (Video*)malloc(sizeof(Video));

    if (video == NULL) {
        perror("Failed to allocate memory");
        return -1;
    }

    // Initialize the video struct
    video->actualBitrate = MEDIUM;
    video->actualCommand = PLAY;
    video->videoName = strdup("video.txt");  // Allocate memory for the string and copy it

    // Check if memory allocation for videoName was successful
    if (video->videoName == NULL) {
        perror("Failed to allocate memory for videoName");
        free(video);
        return -1;
    }

    printf("Video Name: %s\n", video->videoName);
    

}

void *encoder(){
    FILE *file = fopen("video.txt", "r");
    Bitrate rate = LOW;
    Command commandVIdeo = PLAY;

    if (!file) {
        perror("Error abriendo el archivo de video");
        return -1;
    }

    char c[254];
    while(1 && video->actualCommand != STOP){
        sem_wait(&encoderMutex);
        if(video->actualBitrate != rate){
            video->bitrateChanged = 0;
            rate = video->actualBitrate;
        }
        sem_post(&encoderMutex);

        sem_wait(&stateMutex);
        if(video->actualCommand != commandVIdeo){
            commandVIdeo = video->actualCommand;
        }
        sem_post(&stateMutex);

        if(commandVIdeo == PLAY){
            int frame = 0;
            if (video->actualBitrate == LOW) {
                for (int i = 0; i < 100; i++) {
                    if (fgets(c, 254, file) == NULL) {
                        fclose(file);
                        return NULL;
                    }

                }
            } else if (video->actualBitrate == MEDIUM) {
                for (int i = 0; i < 10; i++) {
                    if (fgets(c, 254, file) == NULL) {
                        fclose(file);
                        return NULL;
                    }

                }
            } else {
                if (fgets(c, 254, file) == NULL) {
                    fclose(file);
                    return NULL;
                }

            }


            frame = atoi(c);

            insertLast(streamerBuf,(void *) frame);

            sem_post(&streamerSide->full);
        } else if(commandVIdeo == PAUSE){
            continue;
        } 
        
    }

    fclose(file);
    return NULL;
}

void *streamer(){
    while(1 && video->actualCommand != STOP){
        sem_wait(&streamerSide->full);

        if(streamerBuf->length>0){
            int *frame = (int *)getFromList(streamerBuf,0);
            sem_wait(&visorSide->empty);
            insertLast(visorBuf,(void*) frame);
            sem_post(&visorSide->full);
            sem_post(&streamerSide->empty);
        }
        
    }    
}

void *visor(void * arg){
    int connfd = * (int *) arg;
    while(1 && video->actualCommand != STOP){
        sem_wait(&visorSide->full);
        if(visorBuf->length>0){
            int *frame = (int *) getFromList(visorBuf,0);
            sem_post(&visorSide->empty);
            // printf("Received client: %d\n",frame);
            write(connfd,frame,sizeof(int));
        }
    }
}

void *changeQuality(void *arg) {
    char command[10];
    while (1 && video->actualCommand != STOP) {
        fgets(command, sizeof(command), stdin);

        command[strcspn(command, "\n")] = 0;
        sem_wait(&encoderMutex); 

        if (strcmp(command, "-L") == 0) {
            video->actualBitrate = LOW;
            printf("Calidad cambiada a LOW\n");
        } else if (strcmp(command, "-M") == 0) {
            video->actualBitrate = MEDIUM;
            printf("Calidad cambiada a MEDIUM\n");
        } else if (strcmp(command, "-HIGH") == 0) {
            video->actualBitrate = HIGH;
            printf("Calidad cambiada a HIGH\n");
        } 

        sem_post(&encoderMutex); // Unlock after modifying shared data
    }
    return NULL;
}

void *commandHandler(void *arg) {
    char command[10];
    while (1 && video->actualCommand != STOP) {
        fgets(command, sizeof(command), stdin);

        // Eliminar el salto de línea
        command[strcspn(command, "\n")] = 0;

        sem_wait(&stateMutex);
        if (strcmp(command, "PLAY") == 0) {
            video->actualCommand = PLAY;
            printf("Estado cambiado a PLAY\n");
        } else if (strcmp(command, "PAUSE") == 0) {
            video->actualCommand = PAUSE;
            printf("Estado cambiado a PAUSE\n");
        } else if (strcmp(command, "STOP") == 0) {
            video->actualCommand = STOP;
            printf("Estado cambiado a STOP\n");
            pthread_mutex_unlock(&stateMutex);
            break;  // Terminar el hilo si se recibe STOP
        } else {
            printf("Comando no reconocido. Use PLAY, PAUSE o STOP.\n");
        }
        sem_post(&stateMutex);
    }
    return NULL;
}
