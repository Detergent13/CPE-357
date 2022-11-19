#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#define DECIMAL 10
#define HALFSEC 500000

extern int errno;

void handler(); 

int main(int argc, char *argv[]) {
    struct sigaction action;
    struct itimerval timer;
    sigset_t mask, oldmask;
    char *end;
    int arg;
    int halfsecs;
   
    /* Catch too few args */
    if(argc < 2) {
        printf("usage: timeit <seconds>\n");
        exit(EXIT_FAILURE);
    }
     
    /* Catch malformed input */
    errno = 0;
    arg = strtol(argv[1], &end, DECIMAL);
    if(errno || *end != '\0' || arg < 0) {
        printf("%s: malformed time.\n", argv[1]);
        printf("usage: timeit <seconds>\n");
        exit(errno ? errno : EXIT_FAILURE);
    }

    halfsecs = arg * 2;

    /* Catch too many args */
    if(argc > 2) {
        printf("usage: timeit <seconds>\n");
        exit(EXIT_FAILURE);
    }

    /* Install actions and handlers */
    action.sa_handler = handler;
    if(sigemptyset(&action.sa_mask)) {
        fprintf(stderr, "sigemptyset");
        exit(EXIT_FAILURE);
    }
    action.sa_flags = 0;
    if(sigaction(SIGALRM, &action, NULL)) {
        perror("sigaction");
        exit(errno); 
    }
    if(sigemptyset(&mask)) {
        fprintf(stderr, "sigemptyset");
        exit(EXIT_FAILURE); 
    }
    if(sigaddset(&mask, SIGALRM)) {
        fprintf(stderr, "sigaddset");
        exit(EXIT_FAILURE);
    }
    if(sigprocmask(SIG_BLOCK, &mask, &oldmask)) {
        perror("sigprocmask");
        exit(errno);
    }

    /* Set interval and start to .5 sec */
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = HALFSEC;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = HALFSEC;

    if(setitimer(ITIMER_REAL, &timer, NULL)) {
        perror("setitimer");
        exit(errno);
    }

    if(sigdelset(&oldmask, SIGALRM)) {
        perror("sigdelset");
        exit(errno);
    }

    while(halfsecs--) {
        sigsuspend(&oldmask);
    }

    printf("Time's up!\n");
    return 0;
}

void handler() {
    static int count = 0;
    
    if(++count % 2) {
        printf("Tick...");
        errno = 0;
        fflush(stdout);
        if(errno) {
            perror("fflush");
            exit(errno);
        }
    }
    else {
        printf("Tock\n");
    }
}
