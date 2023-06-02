#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <stdint.h>
#include <stdio.h>

typedef struct {
    /*
     * It is not feasible for all cases. Requires repair
     * (can't support codes up to 127 bits long or empty files).
     */
    uint64_t code;
    unsigned char size;
    char corresp;
}CompactChar;

typedef struct node {
    enum {is_leaf, is_internal} type;
    // Only leaf nodes will store CompactChar data.
    union {
        CompactChar data;
    };
    struct node *left, *right, *pred;
    uint64_t freq;
}Node;

typedef struct {
    unsigned char uniqueCharCount;
    Node *root, *uniqueCharArray;
}HuffmanTree;

int compressFile(FILE *src, FILE *dest);
int decompressFile(FILE *src, FILE *dest);

#endif //HUFFMAN_H
