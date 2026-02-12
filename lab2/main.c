#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "splitCommand.h"
#include "circularBuffer.h"

#define MAX_LINE 1024

void execute_single(char *line, int wait) {
    char **args = split_command(line);;

    pid_t pid;
    pid = fork();

    if (pid == 0) { //child
        execvp(args[0], args);
        perror("execvp failed");
        exit(1);
    } else if (pid > 0) { //parent
        if (wait==1){
            waitpid(pid, NULL, 0);
        }
        free(args);
    } else { //fork failed
        perror("fork failed");
    }
}

void execute_piped(char *line1, char *line2) {
    char **args1;
    char **args2;

    args1 = split_command(line1); 
    args2 = split_command(line2);

    int fd[2];
    
    if (pipe(fd) == -1) {
        perror("pipe failed");
    return;
    }


    pid_t pid1 = fork();

    if (pid1 < 0) { 
        perror("fork failed"); 
        return; 
    }

    else if (pid1 == 0) { //child 1
        dup2(fd[1], STDOUT_FILENO); //stdout to write pipe
        close(fd[0]);
        close(fd[1]);
        execvp(args1[0], args1);
        perror("execvp failed");
        exit(1);
    }

    pid_t pid2 = fork();

    if (pid2 < 0) { 
        perror("fork failed"); 
        return; 
    } 

    else if (pid2 == 0) {
        dup2(fd[0], STDIN_FILENO); //stdin to read pipe
        close(fd[1]);
        close(fd[0]);
        execvp(args2[0], args2);
        perror("execvp failed");
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

    int c;
    int reachedEOF = 0;

    while (reachedEOF==0) {
        c = fgetc(stdin);
        if (c == EOF) {
            reachedEOF = 1;
        } else {
            buffer_push(&cb, c);
        }

        int line_size;
        while ((line_size = buffer_size_next_element(&cb, '\n', reachedEOF)) > 0) {
            char line[MAX_LINE];
            for (int i = 0; i < line_size; i++) {
                line[i] = buffer_pop(&cb);
            }
            line[line_size-1] = '\0'; // quitar \n

            if (strcmp(line, "SINGLE") == 0) {
                int size_next = buffer_size_next_element(&cb, '\n', reachedEOF);
                char command[MAX_LINE];
                for (int i = 0; i < size_next; i++){
                    command[i] = buffer_pop(&cb);
                } 
                command[size_next-1] = '\0';
                execute_single(command, 1);
            }
            else if (strcmp(line, "CONCURRENT") == 0) {
                int size_next = buffer_size_next_element(&cb, '\n', reachedEOF);
                char command[MAX_LINE];
                for (int i = 0; i < size_next; i++){
                    command[i] = buffer_pop(&cb);
                } 
                command[size_next-1] = '\0';
                execute_single(command, 0);
            }
            else if (strcmp(line, "PIPED") == 0) {
                int size1 = buffer_size_next_element(&cb, '\n', reachedEOF);
                int size2 = buffer_size_next_element(&cb, '\n', reachedEOF);
                char command1[MAX_LINE], command2[MAX_LINE];
                for (int i = 0; i < size1; i++){
                    command1[i] = buffer_pop(&cb); 
                }
                for (int i = 0; i < size2; i++){
                    command2[i] = buffer_pop(&cb); 
                } 
                command1[size1-1] = '\0';
                command2[size2-1] = '\0';
                execute_piped(command1, command2);
            }
            else if (strcmp(line, "EXIT") == 0) {
                reachedEOF = 1;
            }
        }
    }

    buffer_deallocate(&cb);
}