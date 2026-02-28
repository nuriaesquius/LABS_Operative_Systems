#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>
#include "parsePGM.h"

int blockSize = 1024 * 16; 
int readPos; 
int totalbuffer; 
int elements_in_buffer = 0; 
unsigned char** sharedBuffer;
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
    int nBytesRead; 
    int readPosLocal;

    while (1) {
        pthread_mutex_lock(&lock_read);
        readPosLocal = readPos;
        readPos += blockSize;
        pthread_mutex_unlock(&lock_read);
        lseek(fd, readPosLocal, SEEK_SET); //go to the position to read 
        unsigned char* buff = malloc(blockSize);
        nBytesRead = read(fd, buff, blockSize);

        if (nBytesRead <= 0) {
            free(buff);
            break;
        }

        sem_wait(&sem_empty); //wait if buffer full
        pthread_mutex_lock(&lock_buffer);
        sharedBuffer[in] = buff; 
        in = (in + 1) % totalbuffer; //next buffer position
        elements_in_buffer++;
        pthread_mutex_unlock(&lock_buffer);

        sem_post(&sem_full); 
    }
    close(fd);
    return NULL; //MIRAT FINS AQUÍ!!!
}

// --- Función Consumidor ---
void * Consumer (void* arg) {
    while (1) {
        sem_wait(&sem_full); // Espera si está vacío [cite: 22]
        
        pthread_mutex_lock(&lock_buffer);
        // Si no hay datos y la producción terminó, salir [cite: 26]
        if (producers_finished && nBuffer == 0) {
            pthread_mutex_unlock(&lock_buffer);
            break; 
        }

        unsigned char* block = sharedBuffer[out];
        out = (out + 1) % elementsInBuffer;
        nBuffer--;
        pthread_mutex_unlock(&lock_buffer);
        
        sem_post(&sem_empty); // Libera un espacio [cite: 7]

        // Procesar Histograma [cite: 6]
        pthread_mutex_lock(&lock_histo);
        for (int i = 0; i < blockSize; i++) {
            global_histogram[block[i]]++;
        }
        pthread_mutex_unlock(&lock_histo);

        free(block); // Liberar bloque procesado [cite: 30]
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 6) { [cite: 32]
        printf("Uso: %s <input> <output> <N_prod> <N_cons> <sizeBuffer>\n", argv[0]);
        return 1;
    }

    pthread_mutex_init(&lock_read, NULL);
    pthread_mutex_init(&lock_buffer, NULL);
    pthread_mutex_init(&lock_histo, NULL);

    int n_prods = atoi(argv[3]);
    int n_cons = atoi(argv[4]);
    elementsInBuffer = atoi(argv[5]);

    // Inicialización [cite: 30]
    sharedBuffer = malloc(sizeof(unsigned char*) * elementsInBuffer);
    sem_init(&sem_empty, 0, elementsInBuffer);
    sem_init(&sem_full, 0, 0);
    readPos = 0; // Deberías usar parsePGM para saltar el header aquí

    pthread_t prods[n_prods], cons[n_cons];

    for (int i = 0; i < n_prods; i++) pthread_create(&prods[i], NULL, Producer, argv[1]);
    for (int i = 0; i < n_cons; i++) pthread_create(&cons[i], NULL, Consumer, NULL);

    for (int i = 0; i < n_prods; i++) pthread_join(prods[i], NULL);

    // Finalización: Despertar consumidores [cite: 28]
    pthread_mutex_lock(&lock_buffer);
    producers_finished = 1;
    pthread_mutex_unlock(&lock_buffer);
    for (int i = 0; i < n_cons; i++) sem_post(&sem_full); 

    for (int i = 0; i < n_cons; i++) pthread_join(cons[i], NULL);

    // Guardar histograma y liberar memoria...
    free(sharedBuffer);
    return 0;
}