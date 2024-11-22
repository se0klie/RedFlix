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

void *streamer();
void *encoder();
void *visor();

void *initVideo();

int main(int argc, char **argv) {
    if(argc != 2){
        return 1;
    }

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

    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);
    pthread_join(threads[2], NULL);

    return 0;
}

void *initVideo(){
    video = (Video*)malloc(sizeof(Video));

    if (video == NULL) {
        perror("Failed to allocate memory");
        return -1;
    }

    // Initialize the video struct
    video->actualBitrate = LOW;
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

    if (!file) {
        perror("Error abriendo el archivo de video");
        return -1;
    }

    char c[254];
    while(1){
        sem_wait(&encoderMutex);
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
                fgets(c,254,file);
                if (fgets(c, 254, file) == NULL) {
                    fclose(file);
                    return NULL;
                }

            }
        } else {
            fgets(c,254,file);
            if (fgets(c, 254, file) == NULL) {
                fclose(file);
                return NULL;
            }

        }


        frame = atoi(c);

        insertLast(streamerBuf,(void *) frame);

        sem_post(&streamerSide->full);
        sem_post(&encoderMutex);
    }

    fclose(file);
    return NULL;
}

void *streamer(){
    while(1){
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
    while(1){
        sem_wait(&visorSide->full);
        if(visorBuf->length>0){
            int *frame = (int *) getFromList(visorBuf,0);
            sem_post(&visorSide->empty);
            write(connfd,frame,sizeof(int));
        }
    }
}

void *changeQuality(void *arg) {
    char command[10];
    while (1) {
        fgets(command, sizeof(command), stdin);

        // Eliminar el salto de línea
        command[strcspn(command, "\n")] = 0;
        sem_wait(&encoderMutex); // Lock before modifying shared data

        if (strcmp(command, "-L") == 0) {
            video->actualBitrate = LOW;
            printf("Calidad cambiada a LOW\n");
        } else if (strcmp(command, "-M") == 0) {
            video->actualBitrate = MEDIUM;
            printf("Calidad cambiada a MEDIUM\n");
        } else if (strcmp(command, "-HIGH") == 0) {
            video->actualBitrate = HIGH;
            printf("Calidad cambiada a HIGH\n");
        } else {
            printf("Comando no reconocido. Use -L, -M o -H.\n");
        }

        video->bitrateChanged = 1;
        sem_post(&encoderMutex); // Unlock after modifying shared data
    }
    return NULL;
}