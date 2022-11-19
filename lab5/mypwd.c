#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#define NO_INODE 0
#define NO_DEV 0

extern int errno;

char path[PATH_MAX + 1];

void appendPath(char *toAppend);
void helper(ino_t prevInode, dev_t prevDevice); 

ino_t rootInode;
ino_t currentInode;
dev_t rootDevice;

int main(int argc, char *argv[]) {
    ino_t prevInode = NO_INODE;
    struct stat rootBuff;
    struct stat currentBuff;
    
    /* Avoid relying on uninitialied memory */
    path[0] = '\0';

    if(lstat("/", &rootBuff)) {
        perror("mypwd");
        exit(errno);
    }

    if(lstat(".", &currentBuff)) {
        perror("mypwd");
        exit(errno);
    }

    rootInode = rootBuff.st_ino;
    currentInode = currentBuff.st_ino;
    rootDevice = rootBuff.st_dev;

    helper(prevInode, NO_DEV);
    printf("%s\n", path);

    return 0;
}

void helper(ino_t prevInode, dev_t prevDevice) { 
    DIR *dir;
    struct dirent *read;
    struct stat statBuff;
    struct stat currentStatBuff;

    /* Catch end-case and top it all off with '/' */
    if(prevInode == rootInode && prevDevice == rootDevice) {
        strcpy(path, "/");
        return;
    }

    /* Catch starting case */
    if(prevInode == NO_INODE) {
        if(lstat(".", &statBuff)) {
            perror("mypwd");
            exit(errno);
        }
        prevInode = statBuff.st_ino;
        prevDevice = statBuff.st_dev;
        if(chdir("..")) {
            perror("mypwd");
            exit(errno);
        }
    }

    if((dir = opendir(".")) == NULL) {
        perror("mypwd");
        exit(errno);
    }

    /* General case- traverse directory */
    while((read = readdir(dir)) != NULL) {
        if(lstat(read->d_name, &statBuff)) {
            perror("mypwd");
            exit(errno);
        }

        if(S_ISDIR(statBuff.st_mode) && prevInode == statBuff.st_ino
            && prevDevice == statBuff.st_dev) {
            if(lstat(".", &currentStatBuff)) {
                perror("mypwd");
                exit(errno);
            }
            if(chdir("..")) {
                perror("mypwd");
                exit(errno);
            }
            
            /* Recur, THEN print to keep path in right order */
            helper(currentStatBuff.st_ino, currentStatBuff.st_dev);
            
            /* Catch path too long */
            if((strlen(path) + strlen(read->d_name) + 1) > PATH_MAX) {
                fprintf(stderr, "path too long");
                exit(1);
            }
            strcat(path, read->d_name);
            strcat(path, "/");
            
            if(closedir(dir)) {
                perror("mypwd");
                exit(errno);
            }
            return;
        }
    }
}
