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
    PLAY,STOP,PAUSE,NOT_STARTED,REPLAY
} Command;

typedef struct{
    char *videoName;
    Bitrate actualBitrate;
    Command actualCommand;
    int bitrateChanged;
    int endedFlag;
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
void *command_listener(void *arg);

void salir(int signal){
	printf("BYE\n");
	exit(0);
}

int main(int argc, char **argv) {
    if(argc != 2){
        return 1;
    }
    int connfd = atoi(argv[1]);
    int *connfd_ptr = malloc(sizeof(int));
    *connfd_ptr = connfd;

    
    signal(SIGINT, salir);
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

    pthread_t threads[4];

    pthread_create(&threads[0], NULL, encoder, connfd_ptr);
    pthread_create(&threads[1], NULL, streamer, NULL);
    pthread_create(&threads[2], NULL, visor, connfd_ptr);
    pthread_create(&threads[3],NULL, command_listener,connfd_ptr);

    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);
    pthread_join(threads[2], NULL);
    pthread_join(threads[3], NULL);

    printf("Video stopped and closed.\n");
    free(connfd_ptr);
    return 0;
}

void *initVideo(){
    video = (Video*)malloc(sizeof(Video));

    if (video == NULL) {
        perror("Failed to allocate memory");
        return NULL;
    }

    // Initialize the video struct
    video->actualBitrate = LOW;
    video->actualCommand = NOT_STARTED;
    video->videoName = strdup("video.txt");  // Allocate memory for the string and copy it
    video -> endedFlag = 0;
    // Check if memory allocation for videoName was successful
    if (video->videoName == NULL) {
        perror("Failed to allocate memory for videoName");
        free(video);
        return NULL;
    }

    printf("Video Name: %s\n", video->videoName);
}

void *encoder( void * arg){
    int connfd = *(int *) arg;
    FILE *file = fopen("video.txt", "r");
    Bitrate rate = LOW;
    Command commandVIdeo = PLAY;
    char *message = "Video ended. Type REPLAY or STOP.\n";

    if (!file) {
        perror("Error abriendo el archivo de video");
        return NULL;
    }

    char c[254];
    while(1 && video->actualCommand != STOP){
        while(video->actualCommand == NOT_STARTED);
        while(video->endedFlag == 1);

        sem_wait(&encoderMutex);
        if(video->actualBitrate != rate){
            video->bitrateChanged = 0;
            rate = video->actualBitrate;
        }
        if(video->actualCommand != commandVIdeo){
            if(video->actualCommand == REPLAY){
                fseek(file, 0, SEEK_SET);  // Reset file pointer to the beginning
                printf("Replaying video...\n");
                video->actualCommand = PLAY;
                video->endedFlag = 0;
            }
            commandVIdeo = video->actualCommand;
        }
        sem_post(&encoderMutex);


        if(commandVIdeo == PLAY){
            int frame = 0;
            if (video->actualBitrate == LOW) {
                for (int i = 0; i < 100; i++) {
                    if (fgets(c, 254, file) == NULL) {
                        if(!video->endedFlag){
                            write(connfd, message,strlen(message));
                            video->endedFlag = 1;
                        }
                    }

                }
                usleep(500000);
            } else if (video->actualBitrate == MEDIUM) {
                for (int i = 0; i < 10; i++) {
                    if (fgets(c, 254, file) == NULL) {
                        if(!video->endedFlag){
                            write(connfd, message,strlen(message));
                            video->endedFlag = 1;
                        }
                    }

                }
                usleep(250000);
            } else {
                if (fgets(c, 254, file) == NULL) {
                   if(!video->endedFlag){
                        write(connfd, message,strlen(message));
                        video->endedFlag = 1;
                    }
                }
               usleep(100000);
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
        while(video->actualCommand == NOT_STARTED);
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
void *visor(void *arg) {
    int connfd = *(int *)arg;

    while (1) {
        while (video->actualCommand == NOT_STARTED);

        if (video->actualCommand == STOP) {
            break;
        }

        sem_wait(&visorSide->full);
        if (visorBuf->length > 0) {
            int *frame = (int *)getFromList(visorBuf, 0);
            printf("Visor got %d\n",frame);
            char *frame_str = malloc(sizeof(char) * 16);
            snprintf(frame_str, 16, "%d", frame);

            printf("Sent to client: %s\n", frame_str);
            write(connfd, frame_str, strlen(frame_str) + 1); 
            sem_post(&visorSide->empty);
        }

        if (video->actualCommand == PAUSE) {
            printf("Stream paused. Waiting for RESUME...\n");
            while (video->actualCommand == PAUSE) {
                usleep(100000);
            }
        }
    }

    printf("Stream stopped.\n");
    return NULL;
}


void *command_listener(void *arg) {
    int connfd = *(int *)arg;
    char buffer[256];

    if (fcntl(connfd, F_GETFD) == -1) {
        perror("Invalid file descriptor");
        return NULL;
    }

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_read = read(connfd, buffer, sizeof(buffer) - 1); 

        if (bytes_read == 0) {
            printf("Client disconnected.\n");
            video->actualCommand = STOP;  
            break;
        } else if (bytes_read < 0) {
          
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;  
            } else {
                perror("Error reading from client");
                video->actualCommand = STOP; 
                break;
            }
        }
        buffer[bytes_read] = '\0';

        sem_wait(&encoderMutex);

        if (strcmp(buffer, "PAUSE") == 0) {
            video->actualCommand = PAUSE;
            printf("Received command: PAUSE\n");
        } else if (strcmp(buffer, "PLAY") == 0) {
            video->actualCommand = PLAY;
            printf("Received command: PLAY\n");
        } else if(strcmp(buffer,"REPLAY") == 0){
            video->actualCommand = REPLAY;
            video->endedFlag = 0;
            printf("Received command: REPLAY\n");
        } else if (strcmp(buffer, "STOP") == 0) {
            video->actualCommand = STOP;
            printf("Received command: STOP\n");
            sem_post(&encoderMutex);
            close(connfd);
            break;  // Exit the loop for STOP command
        } else if (strcmp(buffer, "-L") == 0) {
            video->actualBitrate = LOW;
            printf("Received command: LOW\n");
        } else if (strcmp(buffer, "-M") == 0) {
            video->actualBitrate = MEDIUM;
            printf("Received command: MEDIUM\n");
        } else if (strcmp(buffer, "-H") == 0) {
            video->actualBitrate = HIGH;
            printf("Received command: HIGH\n");
        } else {
            printf("Unrecognized command: %s", buffer);
        }

        sem_post(&encoderMutex);
    }

    return NULL;
}


