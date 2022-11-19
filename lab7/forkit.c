#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

extern int errno;

int main(int argc, char *argv[]) {
    pid_t child;

    printf("Hello, World!\n");

    child = fork();    

    if(child == 0) {
        printf("This is the child, pid %d\n", getpid());
        exit(0);
    }
    else if(child > 0) {
        printf("This is the parent, pid %d\n", getpid());
        wait(NULL);
    }
    else {
        perror("Fork failed");
        exit(errno);
    }

    printf("This is the parent, pid %d, signing off.\n", getpid());

    return 0;
}
