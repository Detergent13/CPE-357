#include <stdio.h> /* IO */
#include <unistd.h> /* getopt */
#include <stdlib.h> /* misc. */
#include <string.h> /* string ops */
#include <errno.h> /* errors */
#include <ctype.h> /* strtol */
#include "node.h" /* node typedef */
#define TABLE_SIZE 1024

extern int errno;
extern char *optarg;
extern int optind;

char *read_word(FILE *file);
int hash(char *word);
int getFreq(char *word);
int peek(FILE *file);
void insert(char *word);
int topContains(char *word);
int compare(const void *a, const void *b); 

/* These are global, as they will be accessed by multiple methods. */
struct Node *table[TABLE_SIZE];
char **top;
long int n;
int wordCount = 0;

/* TODO: Test/handle empty file */
/* TODO: Test/handle user asking for more unique words than in file */
/* TODO: Memory management */
/* TODO: Stdin */
int main(int argc, char *argv[]) {
    int opt;
    char *endPtr;
    int i;
    FILE *file;
    int validCount;

    n = 10; /* Default value for n. Will be overriden if user passes -n flag. */

    while((opt = getopt(argc, argv, "n:")) != -1) {
        switch(opt) { 
            case 'n':
                /* set errno to 0, so if the next block errors we know */
                errno = 0; 
                n = strtol(optarg, &endPtr, 10); /* base 10 */
                if(*endPtr != '\0' || n < 0) {
                    fprintf(stderr,
                        "usage: fw [-n num] [ file1 [ file2 ... ] ]\n");
                    exit(1);
                }
                else if(endPtr == optarg) {
                    /* Turns out this is unnecessary- getopt catches this */
                    fprintf(stderr,
                        "usage: fw [-n num] [ file1 [ file2 ... ] ]\n");
                    exit(1);
                }
                else if(errno) {
                    /* Sorry for this ugly statement. Under 80! */
                    fprintf(stderr,
                         "The -n flag is invalid: %s\n",
                         strerror(errno));

                    exit(errno);
                }
                break;
            case '?':
                fprintf(stderr,
                        "usage: fw [-n num] [ file1 [ file2 ... ] ]\n");
                exit(1);
                break;
        }
    }
    /* malloc array of char *'s with size n for top */
    top = malloc(sizeof(char *) * n);

    if(!top) {
        fprintf(stderr, "Something went wrong with mallocing top.\n");
    }

    /* Set all of top to NULL */
    for(i = 0; i < n; i++) {
        top[i] = NULL;
    }

    /* Set all of table to NULL */
    for(i = 0; i < TABLE_SIZE; i++) {
        table[i] = NULL;
    }


    /* If we're not at the end of our args, there must be files passed. */
    /* Otherwise, let's just open stdin and be done with it! */
    if(optind != argc) {
        /* For every file */
        for(i =optind; i < argc; i++) {
            errno = 0; /* again, set errno to 0 to catch any errors */
            file = fopen(argv[i], "r"); /* read-only perms */
            if(errno) {
                fprintf(stderr, "%s: %s\n", argv[i], strerror(errno));
            }
            else {
                while(peek(file) != EOF) {
                    char *word = read_word(file);
                    insert(word);
                }
                fclose(file);
            }
        }
    }
    else {
        file = stdin;
        while(peek(file) != EOF) {
            char *word = read_word(file);
            insert(word);
        }
        fclose(file);
    }

    /* Just seeing how many valid entries there are.
     * Qsort will throw a fit if it encounters a NULL!
     * Luckily all NULL's will fall *after* the valid elements. */
    validCount = 0;
    for(i = 0; i < n; i++) {
        if(top[i] == NULL) {
            break;
        }
        validCount++;
    }
    
    /* Sort top from beginning to last valid element */
    qsort(top, validCount, sizeof(char *), compare);

    printf("The top %ld words (out of %d) are:\n", n, wordCount);
    /* Print in reverse order, since they're sorted in ascending */
    for(i = n-1; i >= 0; i--) {
        if(top[i]) {
            printf("%9d %s\n", getFreq(top[i]), top[i]);
        }
    }
    return 0;
}

/* reads the next word from the file */
/* lifted and modified from my lab2! */
char *read_word (FILE *file) {
    int wordLength = 16;
    char *buffer = (char *)malloc(sizeof(char) * wordLength);
    int ch;
    int count = 0;
    char *line;

    if(buffer == NULL) {
        fprintf(stderr, "Something went wrong with mallocing!\n");
        exit(1);
    }

    ch = tolower(getc(file));

    /* pass by everything that's not alphabetical */
    while(!(isalpha(ch))) {
        ch = tolower(getc(file));
    }

    while((isalpha(ch)) && (ch != EOF)) {
        if(count >= wordLength) {
            wordLength *= 2; /* double acceptable line length */
            buffer = realloc(buffer, wordLength);

            if(buffer == NULL) {
                fprintf(stderr, "Something went wrong with reallocing!\n");
                exit(1);
            }
        }

        buffer[count] = ch;
        count++;

        ch = tolower(getc(file));
    }

    while(!isalpha(peek(file)) && peek(file) != EOF) {
        getc(file);
    }

    buffer[count] = '\0';
    line = (char *)malloc(sizeof(char) * (count + 1));
    strncpy(line, buffer, (count + 1));
    free(buffer);
    return line;
}

/* a simple string hashing function */
int hash(char *word) {
    int sum = 0;

    while((*word) != '\0') {
        sum += (*word); /* add the ASCII value of the current char to sum */
        word++; /* next char */ 
    }
    
    return (sum % TABLE_SIZE);
}

/* inserts word into table, and updates top if appropriate */
void insert(char *word) {
    int i;

    struct Node *currentNode;
    int wordHashed = hash(word);
    currentNode = table[wordHashed];

    /* If the word isn't in the table, insert it.
    *  Otherwise, find it and increment it. */
    if(getFreq(word) == 0) {
        wordCount++;
        /* Checking for the first element being NULL */
        if(table[wordHashed]) {
            /* Find where it should go in top and loop
             * through that element until we're at the
             * end of the linked list there */
            while(currentNode->next != NULL) {
                currentNode = currentNode->next;
            }

            currentNode->next = malloc(sizeof(Node));

            if(!(currentNode->next)) {
                fprintf(stderr, "Couldn't malloc that Node.");
                exit(1);
            }

            currentNode->next->next = NULL;
            currentNode->next->count = 1;
            currentNode->next->word = word;
        }
        else {  
            table[wordHashed] = malloc(sizeof(Node));
            
            if(!(table[wordHashed])) {
                fprintf(stderr, "Couldn't malloc that Node.");
                exit(1);
            }

            table[wordHashed]->next = NULL;
            table[wordHashed]->count = 1;
            table[wordHashed]->word = word;
        }
    }
    else {
        /* We shouldn't need to worry about NULL here,
         * because we've already established that 
         * it exists at this index of table */
        while(strcmp(currentNode->word, word) != 0) {
            currentNode = currentNode->next;
        }

        currentNode->count++;
    }

    /* Should only think about inserting word into top
     * if it's not already there! */
    if(!topContains(word)) {
        int alreadyInserted;
        /* if there's an empty spot in top, fill it regardless of freq */
        for(i = 0; i < n; i++) {
            alreadyInserted = 0;
            if(top[i] == NULL) {
                top[i] = word;
                alreadyInserted = 1;
                break;
            }       
        }

        /* Only try to insert word if we didn't just add it to top. */
        if(!(alreadyInserted)) {
            int minIndex = 0;
            char *minElement = top[0];
            
            for(i = 0; i < n; i++) {
                if(compare(&top[i], &minElement) < 0) {
                    minElement = top[i];
                    minIndex = i;
                }
            }
            
            if(compare(&minElement, &word) < 0) {
                top[minIndex] = word;
            }
        }
    }
}

int getFreq(char *word) {
    int wordHashed = hash(word);
    struct Node *currentNode = table[wordHashed];

    if(currentNode == NULL) {
        return 0;
    }
    else {
        while(currentNode && strcmp(currentNode->word, word) != 0) {
            currentNode = currentNode->next;
        }
        if(!currentNode) {
            return 0;
        }
        else {
            return currentNode->count;
        }
    }
}

/* Just a handy wrapper to peek at the current char w/o disturbing it */
/* Lifted from my lab2! */
int peek(FILE *file) {
    int ch;
    
    ch = fgetc(file);
    ungetc(ch, file);

    return ch;
}

/* Simple boolean for if top contains the passed word */
int topContains(char *word) {
    int i;
    for(i = 0; i < n; i++) {
        if((top[i] != NULL) && (strcmp(top[i], word) == 0)) {
            return 1;
        }   
    }
    return 0;
}

/* Compare function with some weird pointer stuff
 * thanks to Qsort's specifications. */
int compare(const void *a, const void *b) {
    char *apointer = *((char**)a);
    char *bpointer = *((char**)b);
    if(getFreq(apointer) == getFreq(bpointer)) {
        return strcmp(apointer, bpointer);
    }
    else if(getFreq(apointer) < getFreq(bpointer)) {
        return -1;
    }
    else {
        return 1;
    }
}
