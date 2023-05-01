#include "huffman.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <stdbool.h>

/*
 *  The following structure serves to store unsigned integers with "variable" length.
 *  The size of an unsigned integer variable declared as "uintx_t" will be defined on
 *  the application, and it can be "changed" if necessary. Its name is inspired by the
 *  stdint.h libary types and the "x" in it represents its "variable" size.
 */
typedef struct {
    enum {is_u8, is_u16, is_u32, is_u64} size;
    union {
        uint8_t u8;
        uint16_t u16;
        uint32_t u32;
        uint64_t u64;
    };
}uintx_t;

void merge(Node *n, uint8_t start, uint8_t middle, uint8_t end) {
    uint8_t p1 = start, p2 = middle + 1, len = end - start + 1;
    uint8_t i, j;
    bool end1 = false, end2 = false;
    Node *temp = (Node*)malloc(len*sizeof(Node));

    for(i = 0; i < len; i++) {
        if(!end1 && !end2) {
            if(n[p1].freq > n[p2].freq)
                temp[i] = n[p1++];
            else temp[i] = n[p2++];

            if(p1 > middle) end1 = true;
            if(p2 > end) end2 = true;
        } else {
            if(!end1)
                temp[i] = n[p1++];
            else temp[i] = n[p2++];
        }
    }
    for(i = 0, j = start; i < len; i++, j++)
        n[j] = temp[i];
    free(temp);
}

void mergeSort(Node *n, uint8_t start, uint8_t end) {
    if(start < end) {
        uint8_t middle = (uint8_t)floor((double) (start + end) / 2);
        mergeSort(n, start, middle);
        mergeSort(n, middle + 1, end);
        merge(n, start, middle, end);
    }
}

uint64_t reverse(uint64_t n) {
    uint32_t count = sizeof(n) * 8 - 1;
    uint64_t rev = n;

    n >>= 1;
    while(n) {
        rev <<= 1;
        rev |= n & 1;
        n >>= 1;
        count--;
    }
    rev <<= count;
    return rev;
}

HuffmanTree* buildLeaves(FILE *src, HuffmanTree *tree) {
    int8_t buffer;
    int16_t i;
    Node *temp = (Node*)malloc(128*sizeof(Node));

    // '\0' char:
    temp[0].data.orig = '\0';
    temp[0].freq = 1;
    temp[0].type = is_leaf;
    temp[0].pred = NULL;

    for(i = 1; i < 128; i++) {
        temp[i].data.orig = (int8_t) i;
        temp[i].freq = 0;
        temp[i].type = is_leaf;
        temp[i].pred = NULL;
    }

    buffer = (int8_t)fgetc(src);
    while(buffer != EOF) {
        temp[buffer].freq++;
        buffer = (int8_t)fgetc(src);
    }

    rewind(src); // Go back to the start of the source file

    mergeSort(temp, 0, 127); // Sort nodes

    for(i = 0; temp[i].freq > 0; i++); // Get the number of unique chars

    tree->uniqueChars = (uint8_t) i;
    tree->leaves = (Node*)malloc(i*sizeof(Node));

    while(i > 0) {
        i--;
        tree->leaves[i] = temp[i];
    }
    free(temp);
    return tree;
}

Node* makeNode(Node *left, Node *right) {
    Node *n = (Node*)malloc(sizeof(Node));
    n->type = is_internal;
    n->left = left;
    n->right = right;
    n->pred = NULL;
    n->freq = left->freq + right->freq;
    right->pred = n;
    left->pred = n;
    return n;
}

HuffmanTree* buildHuffmanTree(FILE *src) {
    HuffmanTree *tree = (HuffmanTree*)malloc(sizeof(HuffmanTree));
    tree->root = NULL;
    tree->leaves = NULL;
    tree = buildLeaves(src, tree);

    if(tree->leaves == NULL) {
        printf("\nThe source file is empty.\n");
        free(tree);
        return NULL;
    }

    if(tree->uniqueChars == 1) {
        tree->root = tree->leaves;
        return tree;
    }

    Node *subt1 = NULL, *subt2 = NULL;
    int16_t i = (int16_t) tree->uniqueChars;

    do {
        if(subt1) {
            if(tree->leaves[i-1].freq >= subt1->freq) {
                if(subt2 != NULL && subt2->freq <= subt1->freq)
                    subt2 = makeNode(tree->leaves+i-1, subt2);
                else subt1 = makeNode(tree->leaves+i-1, subt1);
                i--;
            } else {
                if(subt2 != NULL) {
                    if (subt2->freq <= subt1->freq)
                        subt2 = makeNode(tree->leaves+i-1, subt2);
                    else subt1 = makeNode(tree->leaves+i-1, subt1);
                    i--;
                } else {
                    subt2 = makeNode(tree->leaves+i-1, tree->leaves+i-2);
                    i -= 2;
                }
            }
        } else {
            subt1 = makeNode(tree->leaves+i-1, tree->leaves+i-2);
            i -= 2;
        }
    } while(i >= 2);

    if(i == 1) {
        if(subt2 != NULL) {
            if (subt2->freq <= subt1->freq)
                subt2 = makeNode(tree->leaves+i-1, subt2);
            else subt1 = makeNode(tree->leaves+i-1, subt1);
            tree->root = makeNode(subt1, subt2);
        } else tree->root = makeNode(tree->leaves+i-1, subt1);
    } else {
        if(subt2 != NULL)
            tree->root = makeNode(subt2, subt1);
        else tree->root = subt1;
    }
    return tree;
}

void getCompactChar(Node *n) {
    Node *aux = n, *pred;
    uint64_t comp = 0;
    n->data.size = 0;

    while(aux->pred) {
        n->data.size++;
        comp <<= 1;
        pred = aux->pred;
        if(aux == pred->right) comp |= 1;
        aux = aux->pred;
    }
    n->data.comp = reverse(comp);
    n->data.comp >>= 64 - n->data.size;
}

HuffmanTree* setCompactChars(HuffmanTree *tree) {
    int16_t i;
    for(i = 0; i < tree->uniqueChars; i++)
        getCompactChar(tree->leaves + i);
    return tree;
}

CompactChar searchCompactChar(Node *leaves, int8_t c) {
    int16_t i;
    for(i = 0; leaves[i].data.orig != c; i++);
    return leaves[i].data;
}

void compressFile(FILE *src, FILE *dest) {
    HuffmanTree *tree = buildHuffmanTree(src);

    if(tree == NULL || tree->root == NULL) {
        printf("\nFile could not be compressed.\n");
        return;
    }
    uint8_t freqSize; // Number of bytes required to store char frequency
    uint64_t destBuff, *comptxt, i;
    int8_t srcBuff;
    uint8_t bits = 64, shr;
    uintx_t freq, lastChar;
    CompactChar c;

    tree = setCompactChars(tree);
    fwrite(&tree->uniqueChars, 1, sizeof(uint8_t), dest);

    if(tree->leaves[0].freq < 256) freq.size = is_u8;
    else if(tree->leaves[0].freq < 65536) freq.size = is_u16;
    else if(tree->leaves[0].freq < 4294967296) freq.size = is_u32;
    else freq.size = is_u64;

    switch(freq.size) {
        case is_u8:
            freqSize = 1;
            fwrite(&freqSize, 1, sizeof(uint8_t), dest);
            for(i = 0; i < tree->uniqueChars; i++) {
                fwrite(&tree->leaves[i].data.orig, 1, sizeof(int8_t), dest);
                freq.u8 = (uint8_t)tree->leaves[i].freq;
                fwrite(&(freq.u8), 1, sizeof(uint8_t), dest);
            }
            break;
        case is_u16:
            freqSize = 2;
            fwrite(&freqSize, 1, sizeof(uint8_t), dest);
            for(i = 0; i < tree->uniqueChars; i++) {
                fwrite(&tree->leaves[i].data.orig, 1, sizeof(int8_t), dest);
                freq.u16 = (uint16_t)tree->leaves[i].freq;
                fwrite(&(freq.u16), 1, sizeof(uint16_t), dest);
            }
            break;
        case is_u32:
            freqSize = 4;
            fwrite(&freqSize, 1, sizeof(uint8_t), dest);
            for(i = 0; i < tree->uniqueChars; i++) {
                fwrite(&tree->leaves[i].data.orig, 1, sizeof(int8_t), dest);
                freq.u32 = (uint32_t)tree->leaves[i].freq;
                fwrite(&(freq.u32), 1, sizeof(uint32_t), dest);
            }
            break;
        default:
            freqSize = 8;
            fwrite(&freqSize, 1, sizeof(uint8_t), dest);
            for(i = 0; i < tree->uniqueChars; i++) {
                fwrite(&tree->leaves[i].data.orig, 1, sizeof(int8_t), dest);
                freq.u64 = (uint64_t)tree->leaves[i].freq;
                fwrite(&(freq.u64), 1, sizeof(uint64_t), dest);
            }
            break;
    }
    comptxt = (uint64_t*)malloc(tree->root->freq*sizeof(uint64_t));
    comptxt[0] = 0;
    i = 0;

    do {
        srcBuff = (int8_t)fgetc(src);
        if(srcBuff == EOF) srcBuff = '\0';
        c = searchCompactChar(tree->leaves, srcBuff);
        if(bits >= c.size) {
            bits -= c.size;
            c.comp <<= bits;
            comptxt[i] |= c.comp;
            if(srcBuff == '\0') {
                if(bits >= 56) {
                    comptxt[i] >>= bits;
                    bits -= 56;
                    lastChar.u8 = (uint8_t) comptxt[i];
                    lastChar.u8 <<= bits;
                    fwrite(&(lastChar.u8), 1, 1, dest);
                } else if(bits >= 48) {
                    comptxt[i] >>= bits;
                    bits -= 48;
                    lastChar.u16 = (uint16_t) comptxt[i];
                    lastChar.u8 <<= bits;
                    fwrite(&(lastChar.u16), 1, 2, dest);
                } else if(bits >= 32) {
                    comptxt[i] >>= bits;
                    bits -= 32;
                    lastChar.u32 = (uint32_t) comptxt[i];
                    lastChar.u8 <<= bits;
                    fwrite(&(lastChar.u16), 1, 4, dest);
                } else {
                    fwrite(&comptxt[i], 1, 8, dest);
                }
                fclose(src);
                fclose(dest);
                free(comptxt);
                return;
            }
            else if(bits == 0) {
                fwrite(&comptxt[i], 1, sizeof(uint64_t), dest);
                bits = 64;
                i++;
                comptxt[i] = 0;
            }
        } else {
            shr = c.size - bits;
            bits = 64 - shr;
            destBuff = 0;
            while(shr) {
                destBuff >>= 1;
                if(c.comp % 2) destBuff |= 0x8000000000000000;
                c.comp >>= 1;
                shr--;
            }
            comptxt[i] |= c.comp;
            fwrite(&comptxt[i], 1, sizeof(uint64_t), dest);
            i++;
            comptxt[i] = 0;
            comptxt[i] |= destBuff;
            if(srcBuff == '\0') {
                if(bits >= 56) {
                    comptxt[i] >>= bits;
                    bits -= 56;
                    lastChar.u8 = (uint8_t) comptxt[i];
                    lastChar.u8 <<= bits;
                    fwrite(&(lastChar.u8), 1, 1, dest);
                } else if(bits >= 48) {
                    comptxt[i] >>= bits;
                    bits -= 48;
                    lastChar.u16 = (uint16_t) comptxt[i];
                    lastChar.u8 <<= bits;
                    fwrite(&(lastChar.u16), 1, 2, dest);
                } else if(bits >= 32) {
                    comptxt[i] >>= bits;
                    bits -= 32;
                    lastChar.u32 = (uint32_t) comptxt[i];
                    lastChar.u8 <<= bits;
                    fwrite(&(lastChar.u16), 1, 4, dest);
                } else {
                    fwrite(&comptxt[i], 1, 8, dest);
                }
                fclose(src);
                fclose(dest);
                free(comptxt);
                return;
            }
        }
    } while(1);
}

HuffmanTree* rebuildLeaves(FILE *src) {
    HuffmanTree *tree = (HuffmanTree*)malloc(sizeof(HuffmanTree));
    uint8_t freqSize;
    int16_t i;
    uintx_t freq;

    fread(&tree->uniqueChars, 1, sizeof(uint8_t), src);
    tree->leaves = (Node*)malloc(tree->uniqueChars*sizeof(Node));
    tree->root = NULL;
    fread(&freqSize, 1, sizeof(uint8_t), src);

    switch(freqSize) {
        case 1:
            for(i = 0; i < tree->uniqueChars; i++) {
                tree->leaves[i].type = is_leaf;
                tree->leaves[i].pred = NULL;
                fread(&tree->leaves[i].data.orig, 1, sizeof(int8_t), src);
                fread(&(freq.u8), 1, freqSize, src);
                tree->leaves[i].freq = (uint64_t)freq.u8;
            }
            break;
        case 2:
            for(i = 0; i < tree->uniqueChars; i++) {
                tree->leaves[i].type = is_leaf;
                tree->leaves[i].pred = NULL;
                fread(&tree->leaves[i].data.orig, 1, sizeof(int8_t), src);
                fread(&(freq.u16), 1, freqSize, src);
                tree->leaves[i].freq = (uint64_t)freq.u16;
            }
            break;
        case 4:
            for(i = 0; i < tree->uniqueChars; i++) {
                tree->leaves[i].type = is_leaf;
                tree->leaves[i].pred = NULL;
                fread(&tree->leaves[i].data.orig, 1, sizeof(int8_t), src);
                fread(&(freq.u32), 1, freqSize, src);
                tree->leaves[i].freq = (uint64_t)freq.u32;
            }
            break;
        default:
            for(i = 0; i < tree->uniqueChars; i++) {
                tree->leaves[i].type = is_leaf;
                tree->leaves[i].pred = NULL;
                fread(&tree->leaves[i].data.orig, 1, sizeof(int8_t), src);
                fread(&(freq.u64), 1, freqSize, src);
                tree->leaves[i].freq = (uint64_t)freq.u64;
            }
            break;
    }
    return tree;
}

HuffmanTree* rebuildHuffmanTree(FILE *src) {
    HuffmanTree *tree = NULL;
    tree = rebuildLeaves(src);

    if(tree == NULL) {
        printf("\nError while decompressing file\n");
        return NULL;
    }
    if(tree->leaves == NULL) {
        printf("\nThe source file is empty.\n");
        free(tree);
        return NULL;
    }
    if(tree->uniqueChars == 1) {
        tree->root = tree->leaves;
        return tree;
    }
    uint8_t i = tree->uniqueChars;
    Node *subt1 = NULL, *subt2 = NULL;

    do {
        if(subt1) {
            if(tree->leaves[i-1].freq >= subt1->freq) {
                if(subt2 != NULL && subt2->freq <= subt1->freq)
                    subt2 = makeNode(tree->leaves+i-1, subt2);
                else subt1 = makeNode(tree->leaves+i-1, subt1);
                i--;
            } else {
                if(subt2 != NULL) {
                    if (subt2->freq <= subt1->freq)
                        subt2 = makeNode(tree->leaves+i-1, subt2);
                    else subt1 = makeNode(tree->leaves+i-1, subt1);
                    i--;
                } else {
                    subt2 = makeNode(tree->leaves+i-1, tree->leaves+i-2);
                    i -= 2;
                }
            }
        } else {
            subt1 = makeNode(tree->leaves+i-1, tree->leaves+i-2);
            i -= 2;
        }
    } while(i >= 2);

    if(i == 1) {
        if(subt2 != NULL) {
            if (subt2->freq <= subt1->freq)
                subt2 = makeNode(tree->leaves+i-1, subt2);
            else subt1 = makeNode(tree->leaves+i-1, subt1);
            tree->root = makeNode(subt1, subt2);
        } else tree->root = makeNode(tree->leaves+i-1, subt1);
    } else {
        if(subt2 != NULL)
            tree->root = makeNode(subt2, subt1);
        else tree->root = subt1;
    }
    return tree;
}

void decompressFile(FILE *src, FILE *dest) {
    fseek(src, 0, SEEK_END); // Get compact file size in bytes
    int64_t size = ftell(src);
    rewind(src);

    HuffmanTree *tree = rebuildHuffmanTree(src);

    if(!tree || !tree->leaves || !tree->root) return;

    int16_t bits;
    uint64_t srcBuff;
    int8_t destBuff;
    Node *aux = tree->root;

    size -= ftell(src); // Get compacted text size in bytes

    do {
        bits = fread(&srcBuff, 1, sizeof(uint64_t), src);
        size -= bits;
        bits *= 8;
        if(bits < 64)
            srcBuff <<= 64 - bits;

        do {
            if(srcBuff >= 0x8000000000000000)
                aux = aux->right;
            else aux = aux->left;

            if(aux->type == is_leaf) {
                destBuff = aux->data.orig;
                if(destBuff == '\0') {
                    fclose(src);
                    fclose(dest);
                    return;
                }
                fwrite(&destBuff, 1, 1, dest);
                aux = tree->root;
            }
            srcBuff <<= 1;
            bits--;
        } while(bits > 0);
    } while(destBuff != '\0' && size > 0);

    fclose(src);
    fclose(dest);
}
