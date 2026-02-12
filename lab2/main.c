#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "splitCommand.h"
#include "circularBuffer.h"

#define MAX_LINE 1024

int read_next_line(CircularBuffer *cb, char *dest, int *reachedEOF) { //1->readed line, 0->no line to read
    int line_size;
    while ((line_size = buffer_size_next_element(cb, '\n', *reachedEOF)) <= 0 && !(*reachedEOF)) { //keep reading until we have a line or we reach EOF
        int c = fgetc(stdin);
        if (c == EOF) {
            *reachedEOF = 1;
        } 
        else {
            buffer_push(cb, c);
        }
    }
    if (line_size > 0) {
        memset(dest, 0, MAX_LINE);
        for (int i = 0; i < line_size; i++) {
            dest[i] = buffer_pop(cb);
        }
        dest[strcspn(dest, "\r\n")] = '\0'; //remove newline characters
        return 1; //successfully read a line
    }
    return 0; // no line to read
}

void execute_single(char *line, int wait_child) {
    char **args = split_command(line);
    if (args[0] == NULL) {
        free(args);
        return;
    }

    pid_t pid = fork();
    if (pid == 0) { //child
        execvp(args[0], args); //execute the command
        perror("Error en execvp");
        exit(1);
    } 
    else if (pid > 0) { //father
        if (wait_child) {
            waitpid(pid, NULL, 0); //wait for the child to finish
        }
        free(args);
    } 
    else {
        perror("Error en fork");
    }
}

void execute_piped(char *line1, char *line2) {
    char **args1 = split_command(line1);
    char **args2 = split_command(line2);
    int fd[2];

    if (pipe(fd) == -1) {
        perror("Error en pipe");
        return;
    }

    pid_t pid1 = fork();
    if (pid1 < 0) { 
        perror("Error en fork"); 
        return; 
    }
    else if (pid1 == 0) {
        dup2(fd[1], STDOUT_FILENO); //stdout of child 1 goes to write pipe
        close(fd[0]);
        close(fd[1]);
        execvp(args1[0], args1); //execute first command
        exit(1);
    }

    pid_t pid2 = fork();
    if (pid2 < 0) { 
        perror("Error en fork"); 
        return; 
    }
    else if (pid2 == 0) {
        dup2(fd[0], STDIN_FILENO); //stdin of child 2 comes from read pipe
        close(fd[0]);
        close(fd[1]);
        execvp(args2[0], args2); //execute second command
        exit(1);
    }

    close(fd[0]);
    close(fd[1]);

    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    free(args1);
    free(args2);
}

int main() {
    CircularBuffer cb;
    buffer_init(&cb, MAX_LINE);
    char line[MAX_LINE];
    int reachedEOF = 0;

    while (read_next_line(&cb, line, &reachedEOF)) {
        if (strcmp(line, "SINGLE") == 0) {
            if (read_next_line(&cb, line, &reachedEOF)) {
                execute_single(line, 1);
            }
        } 
        else if (strcmp(line, "CONCURRENT") == 0) {
            if (read_next_line(&cb, line, &reachedEOF)) {
                execute_single(line, 0);
            }
        } 
        else if (strcmp(line, "PIPED") == 0) {
            char cmd1[MAX_LINE], cmd2[MAX_LINE];
            if (read_next_line(&cb, cmd1, &reachedEOF) && 
                read_next_line(&cb, cmd2, &reachedEOF)) {
                execute_piped(cmd1, cmd2);
            }
        } 
        else if (strcmp(line, "EXIT") == 0) {
            break;
        }
    }

    buffer_deallocate(&cb);
    return 0;
}