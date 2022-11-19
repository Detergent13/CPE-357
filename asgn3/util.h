#ifndef UTIL_H
#define UTIL_H

#include "nodes.h"
#define ASCII_TABLE_SIZE 256
#define MAX_CODE_LENGTH 40

void traverse(struct HuffNode *node, char *code,
    char (*codes)[ASCII_TABLE_SIZE][MAX_CODE_LENGTH + 1]);
void buildTree(struct ListNode **list, uint32_t *freq);
void priorityInsert(struct ListNode **list, struct HuffNode *toInsert);
void dumbInsert(struct ListNode **list, struct HuffNode *toInsert);
int huffCompare(struct HuffNode *a, struct HuffNode *b);
int atEOF(int fd); 

#endif 
