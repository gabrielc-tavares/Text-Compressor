#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <stdio.h>
#include <stdint.h>

typedef struct {
    uint64_t comp;
    uint8_t size;
    int8_t orig;
}CompactChar;

typedef struct node {
    enum {is_leaf, is_internal} type;
    // Only the leaf nodes will store CompactChar data
    union {
        CompactChar data;
    };
    struct node *left, *right, *pred;
    uint64_t freq;
}Node;

typedef struct {
    uint8_t uniqueChars;
    Node *root, *leaves;
}HuffmanTree;

void compressFile(FILE *src, FILE *dest);
void decompressFile(FILE *src, FILE *dest);

#endif // HUFFMAN_H