#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <stdio.h>
#include <stdint.h>

/*
 *  -------------------------------------------------------------------------------
 *  > Problems to fix <
 *
 *  1) The program don't accept text files with more than a certain ammount of different
 *  unique characters because it leads to CompactChars with more than 8 bits required to
 *  be represented, witch is not supported by now.
 *
 *  2) For some files (especially those with few bytes and lots of unique chars) the
 *  compacted file can be bigger than the original file.
 *
 *  -------------------------------------------------------------------------------
 *  > How data is stored in the compacted file <
 *
 *  The compacted file content will be stored in the following format:
 *
 *  1) One byte containing the number of unique characters.
 *
 *  2) One byte containing the number of bytes required to store unique chars frequency.
 *
 *  3) List of unique chars with their frequency. It will be stored in the following format:
 *
 *      int8_t c1
 *      uintx_t c1_freq
 *      (...)
 *      int8_t cn
 *      uintx_t cn_freq
 *
 *  4) The compacted text.
 *
 *  -------------------------------------------------------------------------------
 */

typedef struct {
    uint8_t compact;
    int8_t matching;
    uint8_t bits;
} CompactChar;

typedef struct node {
    enum {is_leaf, is_internal} type;
    // Only the leaf nodes will store CompactChar data
    union {
        CompactChar data;
    };
    struct node *pred, *left, *right;
    uint64_t freq;
} Node;

typedef struct {
    int16_t uniqueChars;
    Node *root, *leaves;
} HuffmanTree;

void Compress(FILE *src, FILE *dest);
void Decompress(FILE *src, FILE *dest);

#endif //HUFFMAN_H
