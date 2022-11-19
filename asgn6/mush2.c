#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#include "mush.h"

#define PROMPT "8-P "
#define NO_CHILD -1

extern int errno;

void sigintHandler(int signum);
void printPrompt(FILE *input_file); 

int main(int argc, char *argv[]) {
    FILE *input_file;
    char *current_line; 
    struct pipeline *current_pipeline; 
    pid_t *children;
    int i;
    int j;
    int k;
    int (*pipes)[2];

    /* Install SIGINT handler - remember to uninstall after making a child */
    signal(SIGINT, sigintHandler);

    /* Check argc and take input/error appropriately */
    if(argc == 1) {
        input_file = stdin;
    }
    else if(argc == 2) {
        if(!(input_file = fopen(argv[1], "r"))) {
            perror("Couldn't open input file");
            exit(errno);
        }
    }
    else {
        fprintf(stderr, "usage: mush <input file>\n");
        exit(EXIT_FAILURE);
    }

    printPrompt(input_file);
   
    while(!feof(input_file)) {
        /* readLongString returns NULL on EOF OR error.
         * in the former case, just continue and let the rest of 
         * my logic handle it. In the latter case, throw a fit. */
        if((current_line = readLongString(input_file)) == NULL) {
            if(feof(input_file)) {
                continue;
            }
            else {
                perror("readLongString failed");
                exit(errno);
            }
        }
        clerror = 0;
        current_pipeline = crack_pipeline(current_line);
        if(clerror) {
            /* Library will take care of printing the error,
             * we just have to make sure we don't try to run
             * a junk command. */
            printPrompt(input_file);
            continue;
        }

        free(current_line);

        /* Check if the pipeline starts with cd and do so if so. */
        if(strcmp(current_pipeline->stage[0].argv[0], "cd") == 0) {
            if(current_pipeline->stage[0].argc > 2) {
                fprintf(stderr, "usage: cd <directory>\n");
            }
            else if(current_pipeline->stage[0].argc == 2) {
                if(chdir(current_pipeline->stage[0].argv[1]) == -1) {
                    perror(current_pipeline->stage[0].argv[1]);
                }
            }
            else {
                char *home_path;
                if((home_path = getenv("HOME"))
                        || (home_path = (getpwuid(getuid())->pw_dir))) {
                    if(chdir(home_path) == -1) {
                        perror("Failed to chdir to home path");
                        exit(errno);
                    }
                }
                else {
                    fprintf(stderr, "unable to determine home directory");
                    exit(EXIT_FAILURE);
                }
                    
            }
            printPrompt(input_file);
            continue;
        }

        errno = 0;
        children = malloc((current_pipeline->length) * sizeof(pid_t));
        if(errno) {
            perror("Couldn't malloc pid's");
            exit(errno);
        }

        for(i = 0; i < current_pipeline->length; i++) {
            children[i] = NO_CHILD;
        }

        errno = 0;
        pipes = calloc((current_pipeline->length) - 1, sizeof(int) * 2);
        if(errno) {
            perror("Failed to calloc pipes");
            exit(errno);
        }
        
        /* Set up pipes */
        for(i = 0; i < ((current_pipeline->length) - 1); i++) {
            if(pipe(pipes[i]) == -1) {
                perror("Couldn't pipe");
                exit(errno);
            } 
        }
        for(i = 0; i < (current_pipeline->length); i++) {
            if((children[i] = fork()) == -1) {
                perror("Couldn't fork");
                exit(errno);
            }

            /* If we're now the child... */
            if(children[i] == 0) {

                /* Clear the SIGINT response */
                signal(SIGINT, SIG_DFL);

                /* Handle the I */
                if(current_pipeline->stage[i].inname == NULL) {
                    /* Don't overwrite stdin if it's the first stage. */
                    /* It should be taking from stdin if stage->in is NULL. */
                    if(i != 0) {
                        if(dup2(pipes[i-1][0], STDIN_FILENO) == -1) {
                            perror("Couldn't dup pipe to stdin");
                            exit(errno);
                        }
                    }
                }
                else {
                    /* If there's a specified input, open it. */
                    int input = open(current_pipeline->stage[i].inname, O_RDONLY);
                    if(input == -1) {
                        perror(current_pipeline->stage[i].inname);
                        exit(errno);
                    }

                    /* Overwrite stdin with this new file */
                    if(dup2(input, STDIN_FILENO) == -1) {
                        perror(current_pipeline->stage[i].inname);
                        exit(errno);
                    }
                }

                /* ...and handle the O */
                if(current_pipeline->stage[i].outname == NULL) {
                    /* Avoid disturbing the stdout of the last stage
                     * if it hasn't been specified to be a file */
                    if(i != ((current_pipeline->length) - 1)) {
                        if(dup2(pipes[i][1], STDOUT_FILENO) == -1) {
                            perror("Couldn't dup pipe to stdout");
                            exit(errno);
                        }
                    } 
                }
                else {
                    /* Open the output file, and truncate it if it has contents.
                     * If it doesn't exist, create it with rw for all. */
                    int output = open(current_pipeline->stage[i].outname,
                            O_WRONLY | O_TRUNC | O_CREAT,
                            S_IRUSR | S_IRGRP | S_IROTH
                            | S_IWUSR | S_IWGRP | S_IWOTH);
                    if(output == -1) {
                        perror(current_pipeline->stage[i].outname);
                        exit(errno);
                    }

                    /* Overwrite stdout with this new file */
                    if(dup2(output, STDOUT_FILENO) == -1) {
                        perror(current_pipeline->stage[i].outname);
                        exit(errno);
                    }
                }

                /* Close up all of our open pipes. */
                for(j = 0; j < ((current_pipeline->length) - 1); j++) {
                    /* Execute this for both read and write ends */
                    for(k = 0; k < 2; k++) {
                        if(pipes[j][k]) {
                            if(close(pipes[j][k]) == -1) {
                                perror("Failed to close pipe.");
                                exit(errno);
                            }
                        }
                    }
                }

                /* Finally exec our passed command. */
                execvp(current_pipeline->stage[i].argv[0],
                    current_pipeline->stage[i].argv);

                /* Remember, the above only returns if it errored. */
                perror(current_pipeline->stage[i].argv[0]);
                exit(errno);
            }
        }
        /* Otherwise, we've remained the parent. */

            /* Close up all of our open pipes. */
            for(j = 0; j < ((current_pipeline->length) - 1); j++) {
                /* Execute this for both read and write ends */
                for(k = 0; k < 2; k++) {
                    if(pipes[j][k]) {
                        if(close(pipes[j][k]) == -1) {
                            perror("Failed to close pipe.");
                            exit(errno);
                        }
                    }
                }
            }

            /* Wait on all valid PIDs */
            for(j = 0; j < (current_pipeline->length); j++) {
                if(children[j] != NO_CHILD && waitpid(children[j], NULL, 0) == -1) {
                    if(errno != EINTR) {
                        perror("Failed to wait on child. (What a bad parent!)");
                        exit(errno);
                    }
                }
            }

            printPrompt(input_file);
            free(children);
            free(pipes);
    }
   
    yylex_destroy(); 
    printf("\n");

    return 0;
}

void sigintHandler(int signum) {
    return;
}

void printPrompt(FILE *input_file) {
    if(isatty(fileno(input_file)) && isatty(fileno(stdout))) {
        printf(PROMPT);
        fflush(stdout);
    }
}
