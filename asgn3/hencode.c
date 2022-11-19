#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <arpa/inet.h>
#include "util.h"

extern int errno;

int main(int argc, char *argv[]) {
    uint32_t *freq;
    unsigned char charBuffer;
    int inFile;
    int outFile;
    struct ListNode *list = NULL;
    int i;
    struct HuffNode *huffTree;
    char traverseDefault[MAX_CODE_LENGTH + 1];
    uint8_t uniqueElements = 0;
    int lastIndex = -1;
    uint8_t writeBuffer = 0;
    int bufferCount = 0;
    char codes[ASCII_TABLE_SIZE][MAX_CODE_LENGTH + 1];
    
    /* Set every first char to NULL */
    for(i = 0; i < ASCII_TABLE_SIZE; i++) {
        codes[i][0] = '\0';
    }
       
    /* Catch any mallocing errors */ 
    errno = 0;
    freq = calloc(ASCII_TABLE_SIZE, sizeof(uint32_t));
    if(errno) {
        fprintf(stderr, "Couldn't calloc the array: %s\n", strerror(errno));
        exit(errno);
    }

    /* Catch any errors caused by opening this file */
    errno = 0;
    
    /* Opens with read-only perms */
    inFile = open(argv[1], O_RDONLY);

    if(errno) {
        fprintf(stderr, "Couldn't open the input file %s: %s\n",
             argv[1],
             strerror(errno));
        exit(errno);
    }

    errno = 0;

    /* If there's a 3rd arg (the outfile) open it. */
    /* Otherwise, we should use STDOUT, as per the spec */
    if(argv[2]) {
        /* Opens as write only, will create if needed, will overwrite */
        /* Gives the current user read, write, execute perms if created */
        outFile = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
    }
    else {
        outFile = STDOUT_FILENO;
    }

    if(errno) {
        fprintf(stderr, "Couldn't open the output file %s: %s\n",
             argv[2],
             strerror(errno));
        exit(errno);
    }
   
    /* Catch empty file */
    if(atEOF(inFile)) {
        exit(0);
    }

    while(!atEOF(inFile)) {
        /* Uses the ASCII value of the char as an index into freq. */
        /* Then increments the character's count! */
        if(read(inFile, &charBuffer, sizeof(char)) == -1) {
            fprintf(stderr, "Couldn't read a char from fd %d: %s",
                 inFile, strerror(errno));
            exit(errno);
        }
        freq[(int)charBuffer]++;
    }

    /* Check how many unique chars are in the file for later use */
    for(i = 0; i < ASCII_TABLE_SIZE; i++) {
        if(freq[i] > 0) {
            uniqueElements++;
            lastIndex = i;
        }
    }
    
    /* This is that later use! Catch a single-char file. */
    if(uniqueElements == 1) {
        uint32_t networkOrder;
        uint8_t zeroes = 0;
        /* Endian-ness doesn't matter here- both symmetrical and single-bit */
        write(outFile, &zeroes, sizeof(uint8_t));
        /* Again, just single-bit */
        write(outFile, &lastIndex, sizeof(uint8_t));
        
        /* Need to convert to network, but no casting- sizes match */
        networkOrder = htonl(freq[lastIndex]);
        write(outFile, &networkOrder, sizeof(uint32_t));
        exit(0);
    }
   
    /* Build into list using freq */
    buildTree(&list, freq);    

    /* The data of the one node left in the list will be our Huffman tree. */
    huffTree = list->data;
   
    /* Making our empty string */
    traverseDefault[0] = '\0';
    /* And feed it into traverse */
    traverse(huffTree, traverseDefault, &codes);

    /* Decrement uniqueElements in preparation to fit range 0-255 */
    uniqueElements--;

    /* Convert uniqueElements to network order */
    /* There's a lot to unpack here. We're converting it, which
     * results in it being a uint32_t, as htonl returns that.
     * We now need to right shift 24 bits to make it fit into
     * a uint8_t, and then cast as such. This will truncate
     * anything more significant than the 8th digit, which
     * if my code is decent, should be totally fine. */
    uniqueElements = (uint8_t)(htonl(uniqueElements) >> 24);
    write(outFile, &uniqueElements, sizeof(uint8_t));

    for(i = 0; i < ASCII_TABLE_SIZE; i++) {
        if(freq[i] > 0) {
            unsigned char networkOrder;
            uint32_t count;
            charBuffer = i;
            networkOrder = htonl(charBuffer) >> 24;
            count = freq[(uint8_t)charBuffer];
            /* No shift magic needed for this, input/output types match */ 
            count = htonl(count);
            write(outFile, &networkOrder, sizeof(char));
            write(outFile, &count, sizeof(uint32_t));
        }
    }

    /* Seek back to beginning of inFile (and catch any errors) */
    errno = 0;
    lseek(inFile, 0, SEEK_SET);
    if(errno) {
        fprintf(stderr, "Couldn't seek to beginning: %s", strerror(errno));
        exit(errno);
    }
    
    /* Now, we loop through every char in inFile and write its code */
    while(!atEOF(inFile)) {
        unsigned char temp;
        char *code;
        read(inFile, &temp, sizeof(char));
        code = codes[(int)temp];

        while(*code != '\0') {
            writeBuffer <<= 1;
            if(*code == '1') {
                writeBuffer++;
            }
            code++;

            /* Mod bufferCount by the size of writeBuffer (in bits) to check
             * if the buffer is full. */ 
            bufferCount = (bufferCount + 1) % (sizeof(writeBuffer) * 8);
            
            if(bufferCount == 0) {
                writeBuffer = (uint8_t)(htonl(writeBuffer) >> 24);
                write(outFile, &writeBuffer, sizeof(writeBuffer));
                writeBuffer = 0;
            }
        }
    }
    
    /* Pad at the end if needed. */
    /* We know "how incomplete" the byte is from the count. */
    if(bufferCount != 0) {
        /* We need to shift it by the size of writeBuffer (in bits),
         * minus how full the buffer already is. This will pad it with
         * that many zeroes. */ 
        int bitsToShift = (sizeof(writeBuffer) * 8)  - bufferCount;
        writeBuffer <<= bitsToShift;
        writeBuffer = (uint8_t)(htonl(writeBuffer) >> 24);
        write(outFile, &writeBuffer, sizeof(writeBuffer));
    }

    close(inFile);
    if(outFile != STDOUT_FILENO) {
        close(outFile);
    }
    return 0;
}


