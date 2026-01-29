#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "circularBuffer.h"

int process_binary(const char *pathToFile, int bufferSize){
    int fd = open(pathToFile, O_RDONLY);
    if (fd < 0) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
    int size = bufferSize - (bufferSize % sizeof(int));
    int *buffer = malloc(size);
    int suma = 0;
    ssize_t bytes;

    while ((bytes = read(fd, buffer, size)) > 0) {
        int n = bytes / sizeof(int); //num of ints read
        for (int i = 0; i < n; i++) {
            suma += buffer[i];
        }
    }
    free(buffer);
    close(fd);
    return suma;
}

long long process_text(const char *pathToFile, int bufferSize) {
    int fd = open(pathToFile, O_RDONLY);
    if (fd < 0) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    CircularBuffer buffer;
    if (buffer_init(&buffer, bufferSize) != 0) {
        perror("Error initializing buffer");
        close(fd);
        exit(EXIT_FAILURE);
    }

    char secbuffer[bufferSize];
    char temp_num[64];
    long long suma = 0;
    ssize_t bytesRead;

    while ((bytesRead = read(fd, secbuffer, bufferSize)) > 0) {
        for (ssize_t i = 0; i < bytesRead; i++) {
            if (buffer_free_bytes(&buffer) > 0) { //no overflow
                buffer_push(&buffer, secbuffer[i]);
            }
        }

        int elementSize;
        while ((elementSize = buffer_size_next_element(&buffer, ',', 0)) != -1) {
            for (int j = 0; j < elementSize - 1; j++) { //not include comma
                if (buffer_used_bytes(&buffer) > 0) {
                    temp_num[j] = buffer_pop(&buffer);
                }
            }
            buffer_pop(&buffer);//remove the comma
            temp_num[elementSize - 1] = '\0'; //close string
            suma += atoi(temp_num);
            //empty(temp_num);
        }
    }

    int elementSize = buffer_size_next_element(&buffer, ',', 1); //EOF reached
    if (elementSize > 0) {
        for (int j = 0; j < elementSize; j++) {
            temp_num[j] = buffer_pop(&buffer);
        }
        temp_num[elementSize] = '\0';
        suma += atoi(temp_num);
    }

    buffer_deallocate(&buffer);
    close(fd);
    return suma;
}



int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Incorrect number of arguments\n");
        return 1;
    }
    char *mode = argv[1];
    char *path = argv[2];
    int bufferSize = atoi(argv[3]);

    long long suma;

    if (strcmp(mode, "binary") == 0) {
        suma = process_binary(path, bufferSize);
    }
    else if (strcmp(mode, "text") == 0) {
        suma = process_text(path, bufferSize);
    }
    else{
        fprintf(stderr, "Invalid mode\n");
        return 1;
    }

    printf("%lld\n", suma);
    return 0;
}
