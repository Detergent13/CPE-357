#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "nodes.h"
#define ASCII_TABLE_SIZE 256
#define MAX_CODE_LENGTH 40
#define NO_CHAR -1

extern int errno;

int huffCompare(struct HuffNode *a, struct HuffNode *b);
void priorityInsert(struct ListNode **list, struct HuffNode *toInsert); 
void dumbInsert(struct ListNode **list, struct HuffNode *toInsert); 
void traverse(struct HuffNode *node, char *code); 
int peek(FILE *file);

char codes[ASCII_TABLE_SIZE][MAX_CODE_LENGTH + 1];

int main(int argc, char *argv[]) {
    int *freq;
    int ch = 0;
    FILE *file;
    struct ListNode *list = NULL;
    int listSize = 0;
    int i;
    struct HuffNode *huffTree;
    char traverseDefault[MAX_CODE_LENGTH + 1];
    int uniqueElements = 0;
    int lastIndex = -1;

    /* This doesn't need to be dynamically allocated, but there's no harm! */
    /* The perk is that calloc will set it all to 0 for me. */
    /* Although I probably spent more time writing this comment than
     * I would've writing a loop. */
    /* Oops. */
    freq = calloc(ASCII_TABLE_SIZE, sizeof(int));
   
    /* Set every first char to NULL */
    for(i = 0; i < ASCII_TABLE_SIZE; i++) {
        codes[i][0] = '\0';
    }
       
    /* Catch any mallocing errors */ 
    errno = 0;
    if(errno) {
        fprintf(stderr, "Couldn't calloc the array: %s\n", strerror(errno));
        exit(errno);
    }

    errno = 0;
    file = fopen(argv[1], "r");

    /* Catch any errors caused by opening this file */
    if(errno) {
        fprintf(stderr, "Couldn't open the file %s: %s\n",
             argv[1],
             strerror(errno));
        exit(errno);
    }
   
    /* Catch empty file */
    if(peek(file) == EOF) {
        exit(0);
    }

    while((ch = getc(file)) != EOF) {
        /* Uses the ASCII value of the char as an index into freq. */
        /* Then increments the character's count! */
        freq[ch]++;
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
        printf("0x%02x: \n", lastIndex);
        exit(0);
    }

    /* Initially add all non-zero freqs as nodes. */
    for(i = 0; i < ASCII_TABLE_SIZE; i++) {
        if(freq[i] > 0) {
            struct HuffNode *toInsert = malloc(sizeof(HuffNode));
            toInsert->ch = i;
            toInsert->freq = freq[i];
            toInsert->left = NULL;
            toInsert->right = NULL;
       
            priorityInsert(&list, toInsert);
            listSize++;
        }
    }

    /* Combine nodes into our Huffman tree. */
    while(listSize > 1) {
        struct HuffNode *newNode = malloc(sizeof(HuffNode));
        struct HuffNode *first = list->data;
        struct HuffNode *second = list->next->data;
        list = list->next->next;

        newNode->left = first;
        newNode->right = second;
        newNode->ch = NO_CHAR;
        newNode->freq = (first->freq + second->freq);

        /* This doesn't follow standard for conflict res */
        /* Probably a good idea to change that for asgn3! */
        dumbInsert(&list, newNode);
        listSize--;
    }
    
    /* The data of the one node left in the list will be our Huffman tree. */
    huffTree = list->data;
   
    /* Making our empty string */
    traverseDefault[0] = '\0';
    /* And feed it into traverse */
    traverse(huffTree, traverseDefault);    

    /* Print out our table! */
    for(i = 0; i < ASCII_TABLE_SIZE; i++) {
        if(codes[i][0] != '\0') {
            printf("0x%02x: %s\n", i, codes[i]);
        }
    }
    return 0;
}

/* Pre-order traversal of the tree to create codes */
void traverse(struct HuffNode *node, char *code) {
    if(node->ch != NO_CHAR) {
        /* Base case- copy the code that we've created to where it belongs. */
        strcpy(codes[node->ch], code);
    }
    if(node->left) {
        char temp[MAX_CODE_LENGTH];
        strcpy(temp, code);
        traverse(node->left, strcat(temp, "0"));
    }
    if(node->right) {
        char temp[MAX_CODE_LENGTH];
        strcpy(temp, code);
        traverse(node->right, strcat(temp, "1"));
    }
}

/* Inserts a HuffNode into its proper location in the LinkedList. */
/* This double pointer jankiness is due to that fact that
 * we may need to modify the list pointer itself. */
void priorityInsert(struct ListNode **list, struct HuffNode *toInsert) {
    if((*list) == NULL) {
        (*list) = malloc(sizeof(ListNode));
        (*list)->data = toInsert;
        (*list)->next = NULL;
    }
    else if(huffCompare(toInsert, (*list)->data) < 0) {
        struct ListNode *newLinkedNode = malloc(sizeof(ListNode));
        newLinkedNode->next = *list;
        newLinkedNode->data = toInsert;
        *list = newLinkedNode;
    }
    else {
        ListNode *current = *list;
        struct ListNode *newLinkedNode = malloc(sizeof(ListNode));
        
        while(current->next && huffCompare(toInsert, current->next->data) > 0) {
            current = current->next;
        }

        newLinkedNode->next = current->next;
        newLinkedNode->data = toInsert;
        current->next = newLinkedNode;
    }
}

/* Insertion without conflict resolution, for the sake of secondary insertion. */
void dumbInsert(struct ListNode **list, struct HuffNode *toInsert) {
    if((*list) == NULL) {
        (*list) = malloc(sizeof(ListNode));
        (*list)->data = toInsert;
        (*list)->next = NULL;
    }
    else if(toInsert->freq <= (*list)->data->freq) {
        struct ListNode *newLinkedNode = malloc(sizeof(ListNode));
        newLinkedNode->next = *list;
        newLinkedNode->data = toInsert;
        *list = newLinkedNode;
    }
    else {
        ListNode *current = *list;
        struct ListNode *newLinkedNode = malloc(sizeof(ListNode));
        
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

/* Just a handy wrapper to peek at the current char w/o disturbing it. */
/* Lifted from my lab2. What a great investment! */
int peek(FILE *file) {
	int ch;
	
	ch = fgetc(file);
	ungetc(ch, file);

	return ch;
}
