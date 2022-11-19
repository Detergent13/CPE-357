#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

extern int errno;

int main(int argc, char *argv[]) {
    pid_t child;
    int status;

    if(argc != 2) {
        fprintf(stderr, "usage: tryit command\n");
        exit(EXIT_FAILURE);
    }

    child = fork();    

    if(child == 0) {
        if(execl(argv[1], argv[1], NULL) == -1) {
            perror(argv[1]);
            exit(errno);
        }
    }
    else if(child > 0) {
        wait(&status);
        if(WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            printf("Process %d exited with an error value.\n", child);
        }
        else {
            printf("Process %d succeeded.\n", child);
        }
    }
    else {
        perror("Fork failed");
        exit(errno);
    }

    return 0;
}
