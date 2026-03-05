#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "parsePGM.h"

typedef struct {
    unsigned char* data;
    int size;
} Block;

int blockSize = 1024 * 16; 
int readPos; 
int totalbuffer; 
int elements_in_buffer = 0; 
Block** sharedBuffer;
int in = 0, out = 0;
long global_histogram[256] = {0}; 
int producers_finished = 0;

pthread_mutex_t lock_read; 
pthread_mutex_t lock_buffer; 
pthread_mutex_t lock_histo; 
sem_t sem_empty; 
sem_t sem_full;  


void * Producer (void* arg) {
    char * path = (char *) arg;
    int fd = open(path, O_RDONLY);
    int readPosLocal;

    while (1) {
        pthread_mutex_lock(&lock_read);
        readPosLocal = readPos;
        readPos += blockSize;
        pthread_mutex_unlock(&lock_read);

        lseek(fd, readPosLocal, SEEK_SET);
        unsigned char* buf = malloc(blockSize);
        int nBytesRead = read(fd, buf, blockSize);

        if (nBytesRead <= 0) {
            free(buf);
            break;
        }

        // wrap data and real size together
        Block* block = malloc(sizeof(Block));
        block->data = buf;
        block->size = nBytesRead;

        sem_wait(&sem_empty); //wait if buffer full
        pthread_mutex_lock(&lock_buffer);
        sharedBuffer[in] = block; 
        in = (in + 1) % totalbuffer;
        elements_in_buffer++;
        pthread_mutex_unlock(&lock_buffer);
        sem_post(&sem_full); 
    }
    close(fd);
    return NULL; 
}

void * Consumer (void* arg) {
    long local_hist[256] = {0}; //local to avoid holding lock during processing

    while (1) {
        sem_wait(&sem_full);
        pthread_mutex_lock(&lock_buffer);
        if (producers_finished && elements_in_buffer == 0) {
            pthread_mutex_unlock(&lock_buffer);
            sem_post(&sem_full); //wake next consumer
            break; 
        }
        Block* block = sharedBuffer[out];
        out = (out + 1) % totalbuffer;
        elements_in_buffer--;
        pthread_mutex_unlock(&lock_buffer);
        sem_post(&sem_empty); 

        for (int i = 0; i < block->size; i++)
            local_hist[block->data[i]]++;

        free(block->data);
        free(block);
    }

    // merge into global
    pthread_mutex_lock(&lock_histo);
    for (int i = 0; i < 256; i++)
        global_histogram[i] += local_hist[i];
    pthread_mutex_unlock(&lock_histo);

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        printf("Correct usage: %s <input> <o> <N_prod> <N_cons> <sizeBuffer>\n", argv[0]);
        return 1;
    }

    int width, height, maxval;
    int header = parse_pgm_header(argv[1], &width, &height, &maxval);
    if (header < 0) { printf("error parsing header\n"); return 1; }

    int n_prods = atoi(argv[3]);
    int n_cons = atoi(argv[4]);
    totalbuffer = atoi(argv[5]);

    pthread_mutex_init(&lock_read, NULL);
    pthread_mutex_init(&lock_buffer, NULL);
    pthread_mutex_init(&lock_histo, NULL);
    sharedBuffer = malloc(sizeof(Block*) * totalbuffer);
    sem_init(&sem_empty, 0, totalbuffer);
    sem_init(&sem_full, 0, 0);
    readPos = header; //skip pgm header

    pthread_t prods[n_prods], cons[n_cons];

    for (int i = 0; i < n_prods; i++) pthread_create(&prods[i], NULL, Producer, argv[1]);
    for (int i = 0; i < n_cons; i++) pthread_create(&cons[i], NULL, Consumer, NULL);
    for (int i = 0; i < n_prods; i++) pthread_join(prods[i], NULL);

    pthread_mutex_lock(&lock_buffer);
    producers_finished = 1;
    pthread_mutex_unlock(&lock_buffer);
    for (int i = 0; i < n_cons; i++) sem_post(&sem_full); //wake sleeping consumers
    for (int i = 0; i < n_cons; i++) pthread_join(cons[i], NULL);

    // write output
    int fd_out = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    for (int i = 0; i <= maxval; i++) {
        char line[64];
        int len = sprintf(line, "%d,%ld\n", i, global_histogram[i]);
        write(fd_out, line, len);
    }
    close(fd_out);

    free(sharedBuffer);
    pthread_mutex_destroy(&lock_read);
    pthread_mutex_destroy(&lock_buffer);
    pthread_mutex_destroy(&lock_histo);
    sem_destroy(&sem_empty);
    sem_destroy(&sem_full);

    return 0;
}