#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parsePGM.h"

#define BUFF_SIZE 1024
#define HIST_SIZE 256

// struct to pass info to each thread
typedef struct {
    char* path;
    int offset;            // where this thread starts reading
    int bytes;             // how many bytes it must read
    unsigned int hist[HIST_SIZE];  // local histogram of the thread
} ThreadData;


// function executed by each thread
void* thread_func(void* arg) {

    ThreadData* data = (ThreadData*) arg;

    // initialize local histogram to zero
    for (int i = 0; i < HIST_SIZE; i++)
        data->hist[i] = 0;

    // each thread opens the file separately
    int fd = open(data->path, O_RDONLY);
    if (fd < 0) {
        perror("error opening file");
        pthread_exit(NULL);
    }

    // move file pointer to assigned offset
    lseek(fd, data->offset, SEEK_SET);

    unsigned char buffer[BUFF_SIZE];
    int remaining = data->bytes;

    // read in blocks of max 1024 bytes
    while (remaining > 0) {

        int to_read = remaining > BUFF_SIZE ? BUFF_SIZE : remaining;

        int n = read(fd, buffer, to_read);
        if (n <= 0) {
            perror("error reading file");
            close(fd);
            pthread_exit(NULL);
        }

        // update local histogram
        for (int i = 0; i < n; i++) {
            data->hist[buffer[i]]++;
        }

        remaining -= n;
    }

    close(fd);
    pthread_exit(NULL);
}


int main(int argc, char* argv[]) {

    if (argc != 4) {
        printf("usage: %s input.pgm output.txt numThreads\n", argv[0]);
        return 1;
    }

    int nThreads = atoi(argv[3]);
    if (nThreads <= 0) {
        printf("invalid number of threads\n");
        return 1;
    }

    int width, height, maxval;

    // read pgm header
    int header = parse_pgm_header(argv[1], &width, &height, &maxval);

    if (maxval > 255) {
        printf("only 1 byte per pixel images supported\n");
        return 1;
    }

    int totalPixels = width * height;

    pthread_t threads[nThreads];
    ThreadData info[nThreads];

    int chunk = totalPixels / nThreads;
    int extra = totalPixels % nThreads;

    int offset = header;

    // create threads
    for (int i = 0; i < nThreads; i++) {

        info[i].path = argv[1];
        info[i].offset = offset;

        int bytes_for_this = chunk;
        if (i == nThreads - 1)
            bytes_for_this += extra;  // last thread gets the remainder

        info[i].bytes = bytes_for_this;

        offset += bytes_for_this;

        if (pthread_create(&threads[i], NULL, thread_func, &info[i]) != 0) {
            perror("error creating thread");
            return 1;
        }
    }

    // wait for all threads to finish
    for (int i = 0; i < nThreads; i++)
        pthread_join(threads[i], NULL);


    // merge all local histograms into final one
    unsigned int finalHist[HIST_SIZE];
    for (int i = 0; i < HIST_SIZE; i++)
        finalHist[i] = 0;

    for (int t = 0; t < nThreads; t++)
        for (int i = 0; i < HIST_SIZE; i++)
            finalHist[i] += info[t].hist[i];


    // write result to output file
    int fd_out = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_out < 0) {
        perror("error creating output file");
        return 1;
    }

    for (int i = 0; i <= maxval; i++) {
        char line[64];
        sprintf(line, "%d,%u\n", i, finalHist[i]);
        write(fd_out, line, strlen(line));
    }

    close(fd_out);

    return 0;
}