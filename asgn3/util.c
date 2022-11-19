#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <arpa/inet.h>
#include "nodes.h"
#define NO_CHAR -1
#define ASCII_TABLE_SIZE 256
#define MAX_CODE_LENGTH 40

void priorityInsert(struct ListNode **list, struct HuffNode *toInsert);
void dumbInsert(struct ListNode **list, struct HuffNode *toInsert);
int huffCompare(struct HuffNode *a, struct HuffNode *b);

/* Pre-order traversal of the tree to create codes */
void traverse(struct HuffNode *node, char *code,
    char (*codes)[ASCII_TABLE_SIZE][MAX_CODE_LENGTH + 1]) {
    if(node->ch != NO_CHAR) {
        /* Base case- copy the code that we've created to where it belongs. */
        strcpy((*codes)[node->ch], code);
    }
    if(node->left) {
        char temp[MAX_CODE_LENGTH];
        strcpy(temp, code);
        traverse(node->left, strcat(temp, "0"), codes);
    }
    if(node->right) {
        char temp[MAX_CODE_LENGTH];
        strcpy(temp, code);
        traverse(node->right, strcat(temp, "1"), codes);
    }
}

void buildTree(struct ListNode **list, uint32_t *freq) {
    int listSize = 0;
    int i;
    /* Initially add all non-zero freqs as nodes. */
    for(i = 0; i < ASCII_TABLE_SIZE; i++) {
        if(freq[i] > 0) {
            struct HuffNode *toInsert;
            errno = 0;
            toInsert = malloc(sizeof(HuffNode));
            if(errno) {
                fprintf(stderr, "Couldn't malloc that node: %s",
                     strerror(errno));
                exit(errno);
            }

            toInsert->ch = i;
            toInsert->freq = freq[i];
            toInsert->left = NULL;
            toInsert->right = NULL;
       
            priorityInsert(list, toInsert);
            listSize++;
        }
    }

    /* Combine nodes into our Huffman tree. */
    while(listSize > 1) {
        struct HuffNode *newNode;
        struct HuffNode *first = (*list)->data;
        struct HuffNode *second = (*list)->next->data;
        errno = 0;
        newNode = malloc(sizeof(HuffNode));
        if(errno) {
            fprintf(stderr, "Couldn't malloc that node: %s", strerror(errno));
            exit(errno); 
        }
        *list = (*list)->next->next;

        newNode->left = first;
        newNode->right = second;
        newNode->ch = NO_CHAR;
        newNode->freq = (first->freq + second->freq);

        /* This now follows standard for conflict res. */
        /* Turns out it was a good idea to modify this! */
        dumbInsert(list, newNode);
        listSize--;
    }
}
/* Inserts a HuffNode into its proper location in the LinkedList. */
/* This double pointer jankiness is due to that fact that
 * we may need to modify the list pointer itself. */
void priorityInsert(struct ListNode **list, struct HuffNode *toInsert) {
    ListNode *newLinkedNode;
    errno = 0;
    newLinkedNode = malloc(sizeof(ListNode));
    if(errno) {
        fprintf(stderr, "Couldn't malloc that node: %s", strerror(errno));
        exit(errno);
    }
    if((*list) == NULL) {
        (*list) = newLinkedNode;
        (*list)->data = toInsert;
        (*list)->next = NULL;
    }
    else if(huffCompare(toInsert, (*list)->data) < 0) {
        newLinkedNode->next = *list;
        newLinkedNode->data = toInsert;
        *list = newLinkedNode;
    }
    else {
        ListNode *current = *list;
        
        while(current->next && huffCompare(toInsert, current->next->data) > 0) {
            current = current->next;
        }

        newLinkedNode->next = current->next;
        newLinkedNode->data = toInsert;
        current->next = newLinkedNode;
    }
}

/* Insertion without conflict resolution, for the sake of secondary insertion */
void dumbInsert(struct ListNode **list, struct HuffNode *toInsert) {
    if((*list) == NULL) {
        errno = 0;
        (*list) = malloc(sizeof(ListNode));
        if(errno) {
            fprintf(stderr, "Couldn't malloc that node: %s", strerror(errno));
            exit(errno);
        }
        (*list)->data = toInsert;
        (*list)->next = NULL;
    }
    else if(toInsert->freq <= (*list)->data->freq) {
        struct ListNode *newLinkedNode;
        errno = 0;
        newLinkedNode = malloc(sizeof(ListNode));
        if(errno) {
            fprintf(stderr, "Couldn't malloc that node: %s", strerror(errno));
            exit(errno); 
        }
        newLinkedNode->next = *list;
        newLinkedNode->data = toInsert;
        *list = newLinkedNode;
    }
    else {
        ListNode *current = *list;
        struct ListNode *newLinkedNode;
        errno = 0;
        newLinkedNode = malloc(sizeof(ListNode));
        if(errno) {
            fprintf(stderr, "Couldn't malloc that node: %s", strerror(errno));
            exit(errno); 
        } 

        while(current->next && (toInsert->freq) > (current->next->data->freq)) {
            current = current->next;
        }

        newLinkedNode->next = current->next;
        newLinkedNode->data = toInsert;
        current->next = newLinkedNode;
    }
}

/* Compares two HuffmanNodes, complete with tiebreaking behaviour. */
int huffCompare(struct HuffNode *a, struct HuffNode *b) {
    if(a->freq > b->freq) {
        return 1;
    }
    else if(a->freq < b->freq) {
        return -1;
    }
    else {
        /* In the case of a tie, break by comparing chars */
        return a->ch - b->ch;
    }
}

int atEOF(int fd) {
    char ch;
    int atEOFBool;
   
    /* Read one char to ch */ 
    int result = read(fd, &ch, sizeof(char));

    /* If read(...) returns 0, we are at EOF (as per man page) */	
    /* I know this could be simplified... but readability! */
	if(result == 0) {
        atEOFBool = 1;
    }
    else if(result == -1) {
        fprintf(stderr, "Couldn't get a char from fd %d: %s",
             fd, strerror(errno));
        exit(errno);
    }
    else {
        atEOFBool = 0;
    }

    /* Lseek will return -1 on error. Let's catch this! */
    /* Also... remember that we will still seek back if it's successful. */
    if(result != 0 && lseek(fd, -sizeof(char), SEEK_CUR) == -1) {
        fprintf(stderr, "Couldn't seek back on fd %d: %s",
            fd, strerror(errno));
        exit(errno);
    }
    
	return atEOFBool;
}
