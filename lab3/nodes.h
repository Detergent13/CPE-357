typedef struct HuffNode {
    struct HuffNode *left;
    struct HuffNode *right;
    int freq;
    int ch;
} HuffNode;

typedef struct ListNode {
    struct ListNode *next;
    struct HuffNode *data;
} ListNode;
