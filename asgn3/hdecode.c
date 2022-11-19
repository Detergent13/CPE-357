#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <arpa/inet.h>
#include "util.h"

/* This is equivalent to 1000 0000 */
#define STARTING_MASK 128

extern int errno;

int main(int argc, char *argv[]) {
    int inFile;
    int outFile;
    int uniqueChars;
    int i;
    uint32_t *freq;
    uint64_t totalChars = 0;
    struct ListNode *list = NULL;
    struct HuffNode *huffTree;
    uint8_t charBuffer;
    int bufferCount = 0;
    uint8_t charMask;
    struct HuffNode *currentNode;
    uint8_t lastChar;


    errno = 0;
    freq = calloc(ASCII_TABLE_SIZE, sizeof(uint32_t));
    if(errno) {
        fprintf(stderr, "Couldn't calloc freq: %s", strerror(errno));
    }

    if(!argv[1] || strcmp("-", argv[1]) == 0) {
        inFile = STDIN_FILENO;
    }
    else {
        errno = 0;
        inFile = open(argv[1], O_RDONLY);
        if(errno) {
            fprintf(stderr, "Couldn't open file %s: %s",
                argv[1], strerror(errno));
            exit(errno);
        }
    }

    if(argv[2]) {
        errno = 0;
        outFile = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
        if(errno) {
            fprintf(stderr, "Couldn't open file %s: %s",
                argv[2], strerror(errno));
            exit(errno);
        }
    }
    else {
        outFile = STDOUT_FILENO;
    }
    
    /* Catch empty file */
    if(atEOF(inFile)) {
        exit(0);
    }

    /* Read the first byte (char count) into numChars */
    /* No need to worry about endianness - single byte. */
    read(inFile, &uniqueChars, sizeof(uint8_t));
    /* Compensate for 0-255 range */
    uniqueChars++;

    /* Reconstruct frequency table */
    for(i = 0; i < uniqueChars; i++) {
        uint8_t buffer;
        uint32_t count;
        
        read(inFile, &buffer, sizeof(buffer));
        read(inFile, &count, sizeof(count));
        count = htonl(count);        
        totalChars += count;

        freq[buffer] = count;
        lastChar = buffer;
    }

    /* Catch single char case! */
    if(uniqueChars == 1) {
        for(i = 0; i < totalChars; i++) {
            write(outFile, &lastChar, sizeof(lastChar));
        }
        exit(0);
    }

    buildTree(&list, freq);
    huffTree = list->data;

    /* initialise currentNode to root of list */
    currentNode = huffTree;
    while(totalChars > 0) {
        /* Read a byte into charBuffer if the last one is processed */
        if(bufferCount == 0) {
            read(inFile, &charBuffer, sizeof(charBuffer));
            /* And reset the mask */
            charMask = STARTING_MASK;
        }
           
        /* If the current bit is a 1, go right, otherwise, left */
        /* Essentially if the mask hits a 1 at the bit it's masking,
         * this boolean logic will return true */
        if((charBuffer & charMask) != 0) {
            currentNode = currentNode->right; 
        }
        else {
            currentNode = currentNode->left;
        }

        /* If it's a leaf, we write the char */ 
        if(currentNode->left == NULL && currentNode->right == NULL) {
            uint8_t currentChar = currentNode->ch;
            write(outFile, &currentChar, sizeof(currentChar));
            totalChars--;
            currentNode = huffTree;
        }
        
        /* Shift the mask to the right to check next bit */
        charMask >>= 1;
        /* Add 1 to bufferCount, and set it to 0 if now full. */
        bufferCount = (bufferCount + 1) % (sizeof(charBuffer) * 8);
    }
    
    if(inFile != STDIN_FILENO) {
        close(inFile);
    }
    if(outFile != STDOUT_FILENO) {
        close(outFile);
    }

    return 0;
}
