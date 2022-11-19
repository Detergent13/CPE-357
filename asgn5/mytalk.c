#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <poll.h>
#include <signal.h>
#include <netdb.h>
#include <pwd.h>
#include "include/talk.h"

#define DECIMAL 10
#define MIN_PORT 1024
#define MAX_PORT 65535
#define CLIENT 0
#define HOST 1
#define ACCEPTANCE "ok"
#define REJECTION "Go away!"
#define LOCAL 0
#define REMOTE 1
#define BUFFER_SIZE 1024
#define CLOSED_MSG "Connection closed. ^C to terminate."

extern int errno;
extern int optind;

void client(char *hostname, int port, int verboseBool, int acceptBool,
    int windowingBool); 
void host(int port, int verboseBool, int acceptBool, int windowingBool);
void finish(int signum);

int main(int argc, char *argv[]) {
    int verbosity = 0;
    int acceptBool = 0;
    int windowingBool = 1;
    int port;
    char *hostname;
    int mode;
    int opt;
    int argsLeft;
    char *endPtr;

    while((opt = getopt(argc, argv, "vaN:")) != -1) {
        switch(opt) { 
            case 'v':
                verbosity++;
                break;
            case 'a':
                acceptBool = 1;
                break;
            case 'N':
                windowingBool = 0;
                break;
            case '?':
                fprintf(stderr,
                "usage: mytalk [ -v ] [ -a ] [ -N ] [ hostname ] port\n");
                exit(EXIT_FAILURE);
                break;
        }
    }

    argsLeft = argc - optind;

    /* If we only have one argument left, it'd better be the port. */
    if(argsLeft == 1) {
        /* Parse port */
        errno = 0;
        port = strtol(argv[optind], &endPtr, DECIMAL);
        if(errno) {
            perror("Couldn't parse port");
            exit(errno);
        }
        if(*endPtr != '\0') {
            fprintf(stderr, "Port is not a valid number!\n");
            exit(EXIT_FAILURE);
        }
        if(port < MIN_PORT || port > MAX_PORT) {
            fprintf(stderr, "Port out of valid range: %d\n", port);
            exit(EXIT_FAILURE);       
        }
        if(verbosity) {
            printf("We are in host mode!\n");
        }
        mode = HOST;
    }
    /* If we have two, it'd better be the hostname and then the port. */
    else if(argsLeft == 2) {
        hostname = argv[optind];
        /* Parse port */
        errno = 0;
        port = strtol(argv[optind + 1], &endPtr, DECIMAL);
        if(errno) {
            perror("Couldn't parse port");
            exit(errno);
        }
        if(*endPtr != '\0') {
            fprintf(stderr, "Port is not a valid number!\n");
            exit(EXIT_FAILURE);
        }
        if(port < MIN_PORT || port > MAX_PORT) {
            fprintf(stderr, "Port out of valid range: %d\n", port);
            exit(EXIT_FAILURE);       
        }
        if(verbosity) {
            printf("We are in client mode!\n");
        }
        mode = CLIENT;
    }
    /* Otherwise, our input is malformed, let's exit. */
    else {
        fprintf(stderr,
            "usage: mytalk [ -v ] [ -a ] [ -N ] [ hostname ] port\n");
        exit(EXIT_FAILURE);
    }

    if(mode == CLIENT) {
        client(hostname, port, verbosity, acceptBool, windowingBool);
    }
    else if(mode == HOST) {
        host(port, verbosity, acceptBool, windowingBool);
    }
    
    return 0;
}

void client(char *hostname, int port, int verbosity, int acceptBool,
    int windowingBool) {
   
    char buffer[BUFFER_SIZE];
    struct hostent *hostent;
    int remotesock;
    struct sockaddr_in sa;  
    int recv_len;
    struct pollfd fds[2];
    char *username;

    /* Set given library verbosity */
    set_verbosity(verbosity);

    /* Get host from hostname */
    hostent = gethostbyname(hostname);

    /* Create socket to remote */
    if((remotesock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Couldn't create client socket");
        exit(errno);
    }
   
    fds[LOCAL].fd = STDIN_FILENO;
    fds[LOCAL].events = POLLIN;
    fds[LOCAL].revents = 0;
    fds[REMOTE] = fds[LOCAL];
    fds[REMOTE].fd = remotesock;

    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = *(uint32_t*)hostent->h_addr_list[0];

    /* Connect to remote socket */
    if(connect(remotesock, (struct sockaddr *)&sa, sizeof(sa)) == -1) {
        perror("Couldn't connect client socket");
        exit(errno);
    }

    username = (getpwuid(getuid()))->pw_name;

    /* Send our username to the host */
    if(send(remotesock, username, strlen(username), 0) == -1) {
        perror("Failed to send username");
        exit(errno);
    }

    printf("Waiting for response from %s\n", hostname);

    /* Wait for initial packet (hopefully acceptance!) */
    if(poll(&fds[REMOTE], 1, -1) == -1) {
        perror("Failed to poll remote");
        exit(errno);
    }

    if(recv(remotesock, &buffer, sizeof(ACCEPTANCE), 0) == -1) {
        perror("Initial client recv failed");
        exit(errno);
    }

    /* Check the initial packet for approval */
    if(strncmp((char *) buffer, ACCEPTANCE, strlen(ACCEPTANCE)) != 0) {
        printf("Connection refused\n");
        exit(EXIT_SUCCESS);
    }

    if(windowingBool) {
        start_windowing();
    }
    
    while(1) {
        if(poll(fds, sizeof(fds) / sizeof(struct pollfd), -1) == -1) {
            perror("Polling failed");
            exit(errno);
        }
        if(fds[LOCAL].revents & POLLIN) {
            update_input_buffer();
            if(has_whole_line()) { 
                int read_len;
                if((read_len = read_from_input(buffer, BUFFER_SIZE)) > 0) {
                    if(send(remotesock, buffer, read_len, 0) == -1) {
                        perror("Couldn't send to remote");
                        exit(errno);
                    }
                    
                    if(has_hit_eof()) {
                        stop_windowing();
                        close(remotesock);
                        exit(EXIT_SUCCESS);
                    }

                }
                else if(read_len == ERR) {
                    fprintf(stderr, "Reading failed.\n");
                    exit(EXIT_FAILURE);
                }
            }
        }
        if(fds[REMOTE].revents & POLLIN) {
            if((recv_len = recv(remotesock, buffer, BUFFER_SIZE, 0)) == -1) {
                perror("Couldn't recv from remote");
                exit(errno);
            }
            
            /* If at EOF, exit */
            if(recv_len == 0) {
                if(write_to_output(CLOSED_MSG, strlen(CLOSED_MSG)) == ERR) {
                    fprintf(stderr, "Couldn't write to output.\n");
                    exit(EXIT_FAILURE);
                }
                close(remotesock);
                if(signal(SIGINT, finish) == SIG_ERR) {
                    perror("Couldn't install signal");
                    exit(errno);
                }
            }

            if(write_to_output(buffer, recv_len) == ERR) {
                fprintf(stderr, "Couldn't write to output.\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

void host(int port, int verbosity, int acceptBool, int windowingBool) {
    int fd, clientsock;
    struct sockaddr_in sa, peerinfo;
    char peeraddr[INET_ADDRSTRLEN];
    char remoteUsername[LOGIN_NAME_MAX + 1];
    socklen_t len;
    int recv_len;
    struct pollfd fds[2];
    char buffer[BUFFER_SIZE];

    /* Set given library verbosity */
    set_verbosity(verbosity);

    if((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Couldn't create host socket");
        exit(errno);
    }

    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);

    /* Bind to our port and catch any failures */
    if(bind(fd, (struct sockaddr *)&sa, sizeof(sa))) {
        perror("Couldn't bind to host port");
        exit(errno);
    }

    if(listen(fd, 1)) {
        perror("Failed to listen for incoming connections");
        exit(errno);
    }

    len = sizeof(peerinfo);
    if((clientsock = accept(fd, (struct sockaddr *)&peerinfo, &len)) == -1) {
        perror("Failed to accept incoming connection");
        exit(errno);
    }

    fds[LOCAL].fd = STDIN_FILENO;
    fds[LOCAL].events = POLLIN;
    fds[LOCAL].revents = 0;
    fds[REMOTE] = fds[LOCAL];
    fds[REMOTE].fd = clientsock;
    
    /* Read the remote hostname */
    inet_ntop(AF_INET, &peerinfo.sin_addr.s_addr, peeraddr, sizeof(peeraddr));

    if((recv_len = recv(clientsock, remoteUsername, sizeof(remoteUsername), 0)) == -1) {
        perror("Couldn't receive from client");
        exit(errno);
    }
    remoteUsername[recv_len] = '\0';

    printf("Mytalk request from %s@%s. Accept (y/n)? ", (char *) remoteUsername,
        (char *) peeraddr);
    if(acceptBool) {
        printf("Auto-accepting. (-a flag)\n");
    }
    if(acceptBool || tolower(getchar()) == 'y') {
        if(send(clientsock, ACCEPTANCE, strlen(ACCEPTANCE), 0) == -1) {
            perror("Failed to send acceptance");
            exit(errno);
        }
    }
    else {
        if(send(clientsock, REJECTION, strlen(REJECTION), 0) == -1) {
            perror("Failed to send rejection");
            exit(errno);
        }
    }

    if(windowingBool) {
        start_windowing();
    }

    while(1) {
        if(poll(fds, sizeof(fds) / sizeof(struct pollfd), -1) == -1) {
            perror("Couldn't poll");
            exit(errno);
        }
        if(fds[LOCAL].revents & POLLIN) {
            update_input_buffer();
            if(has_whole_line()) { 
                int read_len;
                if((read_len = read_from_input(buffer, BUFFER_SIZE)) > 0) {
                    if(send(clientsock, buffer, read_len, 0) == -1) {
                        perror("Failed to send to client");
                        exit(errno);
                    }
                    
                    if(has_hit_eof()) {
                        stop_windowing();
                        close(clientsock);
                        close(fd);
                        exit(EXIT_SUCCESS);
                    }

                }
                else if(read_len == -1) {
                    fprintf(stderr, "Couldn't read from input\n");
                    exit(EXIT_FAILURE);
                }
            }
        }
        if(fds[REMOTE].revents & POLLIN) {
            if((recv_len = recv(clientsock, buffer, BUFFER_SIZE, 0)) == -1) {
                perror("Failed to recv from client");
                exit(errno);
            }
            
            /* If at EOF, exit */
            if(recv_len == 0) {
                if(write_to_output(CLOSED_MSG, strlen(CLOSED_MSG)) == ERR) {
                    fprintf(stderr, "Couldn't write to output\n");
                    exit(EXIT_FAILURE);
                }
                close(clientsock);
                close(fd);
                signal(SIGINT, finish);
            }

            if(write_to_output(buffer, recv_len) == ERR) {
                fprintf(stderr, "Couldn't write to output\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

void finish(int signum) {
    stop_windowing();
    exit(EXIT_SUCCESS);
}
