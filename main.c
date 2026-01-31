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

    char read_buffer[bufferSize];
    char temp_num[64];
    long long suma = 0;
    ssize_t bytesRead;

    while ((bytesRead = read(fd, read_buffer, bufferSize)) > 0) {
        for (ssize_t i = 0; i < bytesRead; i++) {
            while (buffer_free_bytes(&buffer) == 0) { // buffer full
                int elemSize = buffer_size_next_element(&buffer, ',', 0);
                if (elemSize == -1) {
                    fprintf(stderr, "Num bigger than the buffer, no aviable\n");
                    buffer_deallocate(&buffer);
                    close(fd);
                    exit(EXIT_FAILURE);
                }

                if (elemSize >= (int)sizeof(temp_num)) {
                    fprintf(stderr, "Num bigger than the temporary space number, no aviable\n");
                    buffer_deallocate(&buffer);
                    close(fd);
                    exit(EXIT_FAILURE);
                }

                for (int j = 0; j < elemSize - 1; j++)
                    temp_num[j] = buffer_pop(&buffer); //copy number without comma

                buffer_pop(&buffer); //delete comma
                temp_num[elemSize - 1] = '\0';
                suma += atoll(temp_num);
            } //end while buffer full

            buffer_push(&buffer, read_buffer[i]);
        }

        int elemSize; // process all complete elements in the buffer
        while ((elemSize = buffer_size_next_element(&buffer, ',', 0)) != -1) {
            if (elemSize >= (int)sizeof(temp_num)) {
                fprintf(stderr, "Num bigger than the temporary space number, no aviable\n");
                buffer_deallocate(&buffer);
                close(fd);
                exit(EXIT_FAILURE);
            }

            for (int j = 0; j < elemSize - 1; j++)
                temp_num[j] = buffer_pop(&buffer);

            buffer_pop(&buffer); //delete comma
            temp_num[elemSize - 1] = '\0';
            suma += atoll(temp_num);
        }
    }

    int elemSize = buffer_size_next_element(&buffer, ',', 1); //EOF: process last element
    if (elemSize > 0) {
        if (elemSize >= (int)sizeof(temp_num)) {
            fprintf(stderr, "Num bigger than the temporary space number, no aviable\n");
            buffer_deallocate(&buffer);
            close(fd);
            exit(EXIT_FAILURE);
        }

        for (int j = 0; j < elemSize; j++)
            temp_num[j] = buffer_pop(&buffer);

        temp_num[elemSize] = '\0';
        suma += atoll(temp_num);
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

    int suma;

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

    printf("%d\n", suma);
    return 0;
}
