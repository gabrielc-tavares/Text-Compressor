#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <stdio.h>
#include <stdint.h>

/*
 *  -------------------------------------------------------------------------------
 *  > Problems to fix <
 *
 *  1) For some files (especially those with few bytes and many unique characters) the
 *  compacted file can be bigger than the original file.
 *
 *  -------------------------------------------------------------------------------
 *  > How data will be saved in the compressed file <
 *
 *  The compressed file content will be stored in the following format:
 *
 *  1) One byte containing the number of unique characters in the original file.
 *
 *  2) One byte containing the number of bytes required to store char frequency data.
 *
 *  3) All the unique chars followed by their frequency. It will be stored in the following format:
 *
 *      int8_t c1
 *      uintx_t c1_freq
 *      (...)
 *      int8_t cn
 *      uintx_t cn_freq
 *
 *  4) The compressed text.
 *
 *  -------------------------------------------------------------------------------
 */

typedef struct {
    uint64_t comp;
    uint8_t size;
    int8_t orig;
}CompactChar;

typedef struct node {
    enum {is_leaf, is_internal} type;
    // Only leaf nodes will store CompactChar data
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