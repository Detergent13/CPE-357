#ifndef NODES_H
#define NODES_H

typedef struct HuffNode {
    struct HuffNode *left;
    struct HuffNode *right;
    int freq;
    int32_t ch;
} HuffNode;

typedef struct ListNode {
    struct ListNode *next;
    struct HuffNode *data;
} ListNode;

#endif
