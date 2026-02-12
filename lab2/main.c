#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "splitCommand.h"

#define MAX_LINE 1024


void execute_single(char *line, int wait) {
    char *args[64];
    args = split_command(line);;

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
    } else { //fork failed
        perror("fork failed");
    }
}

void execute_piped(char *line1, char *line2) {
    char *args1[64];
    char *args2[64];

    args1 = split_command(line1); 
    args2 = split_command(line2);

    int fd[2];
    pipe(fd);
    
    if (fd == -1) {
        perror("pipe failed");
        return;
    }

    pid_t pid1 = fork();

    if (pid1 == 0) { //child 1
        dup2(fd[1], STDOUT_FILENO); //stdout to write pipe
        close(fd[0]);
        close(fd[1]);
        execvp(args1[0], args1);
        perror("execvp failed");
        exit(1);
    }

    pid_t pid2 = fork();

    if (pid2 == 0) {
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
}

int main() {
    char mode[MAX_LINE];
    char line1[MAX_LINE];
    char line2[MAX_LINE];

    while (1) {
        if (fgets(mode, MAX_LINE, stdin) == NULL)
            break;

        mode[strcspn(mode, "\n")] = 0; //remove \n

        if (strcmp(mode, "EXIT") == 0) { //exit
            break; // 
        }

        if (strcmp(mode, "SINGLE") == 0) { 
            fgets(line1, MAX_LINE, stdin);
            line1[strcspn(line1, "\n")] = 0; //remove \n
            execute_single(line1, 1);
        }
        else if (strcmp(mode, "CONCURRENT") == 0) {
            fgets(line1, MAX_LINE, stdin);
            line1[strcspn(line1, "\n")] = 0; //remove \n
            execute_single(line1, 0);
        }
        else if (strcmp(mode, "PIPED") == 0) {
            fgets(line1, MAX_LINE, stdin);
            fgets(line2, MAX_LINE, stdin);
            line1[strcspn(line1, "\n")] = 0; //remove \n
            line2[strcspn(line2, "\n")] = 0; //remove \n
            execute_piped(line1, line2);
        }
    }

    return 0;
}
