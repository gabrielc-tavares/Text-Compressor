#include "huffman.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <stdbool.h>

/*
 *  The following structure serve to store unsigned integers with "variable" size.
 *  The size of an unsigned integer variable declared as "uintx_t" will be defined
 *  on the application, and it can be "changed" if necessary. Its name is inspired
 *  by the stdint.h libary types and the "x" in it represents its "variable" size.
 */
typedef struct {
    enum {is_8bits, is_16bits, is_32bits, is_64bits} size;
    union {
        uint8_t u8;
        uint16_t u16;
        uint32_t u32;
        uint64_t u64;
    };
} uintx_t;

void Merge(Node *n, int16_t start, int16_t middle, int16_t end) {
    int16_t p1 = start, p2 = middle + 1, len = end - start + 1;
    int16_t i, j;
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

void MergeSort(Node *n, int16_t start, int16_t end) {
    if(start < end) {
        int16_t middle = (int16_t)floor((double)(start + end) / 2);
        MergeSort(n, start, middle);
        MergeSort(n, middle + 1, end);
        Merge(n, start, middle, end);
    }
}

uint8_t Reverse(uint8_t c) {
    c = (c & 0xF0) >> 4 | (c & 0x0F) << 4;
    c = (c & 0xCC) >> 2 | (c & 0x33) << 2;
    c = (c & 0xAA) >> 1 | (c & 0x55) << 1;
    return c;
}

HuffmanTree* BuildLeaves(FILE *src, HuffmanTree *tree) {
    int8_t buffer;
    int16_t i;
    Node *temp = (Node*)malloc(128*sizeof(Node));

    // '\0' char:
    temp[0].data.matching = '\0';
    temp[0].freq = 1;
    temp[0].type = is_leaf;
    temp[0].pred = NULL;

    for(i = 1; i < 128; i++) {
        temp[i].data.matching = (int8_t)i;
        temp[i].freq = 0;
        temp[i].type = is_leaf;
        temp[i].pred = NULL;
    }

    do {
        buffer = (int8_t)fgetc(src);
        if(buffer != EOF)
            temp[buffer].freq++;
    } while(buffer != EOF);
    rewind(src); // Go back to the start fo the source file

    // Put the nodes with freq > 0 at the beggining
    MergeSort(temp, 0, 127);

    // Get the number of unique chars
    for(i = 0; temp[i].freq > 0; i++);

    tree->uniqueChars = i;
    tree->leaves = (Node*)malloc(i*sizeof(Node));

    while(i > 0) {
        i--;
        tree->leaves[i] = temp[i];
    }

    free(temp);
    return tree;
}

Node* MakeNode(Node *left, Node *right) {
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

HuffmanTree* BuildHuffmanTree(FILE *src) {
    HuffmanTree *tree = (HuffmanTree*)malloc(sizeof(HuffmanTree));
    tree->root = NULL;
    tree->leaves = NULL;
    tree = BuildLeaves(src, tree);

    if(tree->leaves == NULL) {
        printf("\nThe source file is empty.\n");
        free(tree);
        return NULL;
    }

    Node *subt1 = NULL, *subt2 = NULL;
    int16_t i = tree->uniqueChars;

    if(i == 1) {
        tree->root = tree->leaves;
        return tree;
    }

    do {
        if(subt1) {
            if(tree->leaves[i-1].freq >= subt1->freq) {
                if(subt2 != NULL && subt2->freq <= subt1->freq)
                    subt2 = MakeNode(tree->leaves+i-1, subt2);
                else subt1 = MakeNode(tree->leaves+i-1, subt1);
                i--;
            } else {
                if(subt2 != NULL) {
                    if (subt2->freq <= subt1->freq)
                        subt2 = MakeNode(tree->leaves+i-1, subt2);
                    else subt1 = MakeNode(tree->leaves+i-1, subt1);
                    i--;
                } else {
                    subt2 = MakeNode(tree->leaves+i-1, tree->leaves+i-2);
                    i -= 2;
                }
            }
        } else {
            subt1 = MakeNode(tree->leaves+i-1, tree->leaves+i-2);
            i -= 2;
        }
    } while(i >= 2);

    if(i == 1) {
        if(subt2 != NULL) {
            if (subt2->freq <= subt1->freq)
                subt2 = MakeNode(tree->leaves+i-1, subt2);
            else subt1 = MakeNode(tree->leaves+i-1, subt1);
            tree->root = MakeNode(subt1, subt2);
        } else tree->root = MakeNode(tree->leaves+i-1, subt1);
    } else {
        if(subt2 != NULL)
            tree->root = MakeNode(subt2, subt1);
        else tree->root = subt1;
    }
    return tree;
}

void GetCompactChar(Node *n) {
    Node *aux = n, *pred;
    uint8_t comp = 0;
    n->data.bits  = 0;

    while(aux->pred) {
        n->data.bits++;
        comp <<= 1;
        pred = aux->pred;
        if(aux == pred->right) comp |= 1;
        aux = aux->pred;
    }
    n->data.compact = Reverse(comp);
    n->data.compact >>= 8 - n->data.bits;
}

void SetCompactChars(HuffmanTree *tree) {
    int16_t i;
    for(i = 0; i < tree->uniqueChars; i++)
        GetCompactChar(tree->leaves + i);
}

CompactChar SearchCompactChar(Node *leaves, int8_t c) {
    int16_t i;
    for(i = 0; leaves[i].data.matching != c; i++);
    return leaves[i].data;
}

void Compress(FILE *src, FILE *dest) {
    HuffmanTree *tree = NULL;
    tree = BuildHuffmanTree(src);

    if(tree == NULL || tree->root == NULL) {
        printf("\nFile could not be compressed.\n");
        return;
    }

    uint8_t uniqueChars = (uint8_t)tree->uniqueChars;
    uint8_t freqSize; // Number of bytes required to store char frequency
    int8_t srcBuff;
    uint8_t destBuff, *comptxt;
    CompactChar c;
    int16_t bits = 8, i, shr;
    uintx_t freq;

    SetCompactChars(tree);

    fwrite(&uniqueChars, 1, sizeof(uint8_t), dest);

    /*
    for(i = 0; i < tree->uniqueChars; i++)
        fwrite(&tree->leaves[i].data, 1, sizeof(CompactChar), dest);
    */

    if(tree->leaves[0].freq < 256) freq.size = is_8bits;
    else if(tree->leaves[0].freq < 65536) freq.size = is_16bits;
    else if(tree->leaves[0].freq < 4294967296) freq.size = is_32bits;
    else freq.size = is_64bits;

    switch(freq.size) {
        case is_8bits:
            freqSize = 1;
            fwrite(&freqSize, 1, sizeof(uint8_t), dest);
            for(i = 0; i < tree->uniqueChars; i++) {
                fwrite(&tree->leaves[i].data.matching, 1, sizeof(int8_t), dest);
                freq.u8 = (uint8_t)tree->leaves[i].freq;
                fwrite(&(freq.u8), 1, sizeof(uint8_t), dest);
            }
            break;
        case is_16bits:
            freqSize = 2;
            fwrite(&freqSize, 1, sizeof(uint8_t), dest);
            for(i = 0; i < tree->uniqueChars; i++) {
                fwrite(&tree->leaves[i].data.matching, 1, sizeof(int8_t), dest);
                freq.u16 = (uint16_t)tree->leaves[i].freq;
                fwrite(&(freq.u16), 1, sizeof(uint16_t), dest);
            }
            break;
        case is_32bits:
            freqSize = 4;
            fwrite(&freqSize, 1, sizeof(uint8_t), dest);
            for(i = 0; i < tree->uniqueChars; i++) {
                fwrite(&tree->leaves[i].data.matching, 1, sizeof(int8_t), dest);
                freq.u32 = (uint32_t)tree->leaves[i].freq;
                fwrite(&(freq.u32), 1, sizeof(uint32_t), dest);
            }
            break;
        default:
            freqSize = 8;
            fwrite(&freqSize, 1, sizeof(uint8_t), dest);
            for(i = 0; i < tree->uniqueChars; i++) {
                fwrite(&tree->leaves[i].data.matching, 1, sizeof(int8_t), dest);
                freq.u64 = (uint64_t)tree->leaves[i].freq;
                fwrite(&(freq.u64), 1, sizeof(uint64_t), dest);
            }
            break;
    }
    comptxt = (uint8_t*)malloc(tree->root->freq);
    comptxt[0] = 0;
    i = 0;

    do {
        srcBuff = (int8_t)fgetc(src);
        if(srcBuff == EOF) srcBuff = '\0';
        c = SearchCompactChar(tree->leaves, srcBuff);
        if(bits >= c.bits) {
            bits -= c.bits;
            c.compact <<= bits;
            comptxt[i] |= c.compact;
            if(srcBuff == '\0')
                fwrite(&comptxt[i], 1, 1, dest);
            else if(bits == 0) {
                fwrite(&comptxt[i], 1, 1, dest);
                bits = 8;
                i++;
                comptxt[i] = 0;
            }
        } else {
            shr = c.bits - bits;
            bits = 8 - shr;
            destBuff = 0;
            while(shr) {
                destBuff >>= 1;
                if(c.compact % 2) destBuff |= 128;
                c.compact >>= 1;
                shr--;
            }
            comptxt[i] |= c.compact;
            fwrite(&comptxt[i], 1, 1, dest);
            i++;
            comptxt[i] = 0;
            comptxt[i] |= destBuff;
            if(srcBuff == '\0')
                fwrite(&comptxt[i], 1, 1, dest);
        }
    } while(srcBuff != '\0');

    fclose(src);
    fclose(dest);
}

HuffmanTree* RebuildLeaves(FILE *src) {
    HuffmanTree *tree = (HuffmanTree*)malloc(sizeof(HuffmanTree));
    uint8_t uniqueChars, freqSize;
    int16_t i;
    uintx_t freq;

    fread(&uniqueChars, 1, sizeof(uint8_t), src);
    fread(&freqSize, 1, sizeof(uint8_t), src);
    tree->uniqueChars = (int16_t)uniqueChars;
    tree->leaves = (Node*)malloc(tree->uniqueChars*sizeof(Node));
    tree->root = NULL;

    switch(freqSize) {
        case 1:
            for(i = 0; i < tree->uniqueChars; i++) {
                tree->leaves[i].type = is_leaf;
                tree->leaves[i].pred = NULL;
                fread(&tree->leaves[i].data.matching, 1, sizeof(int8_t), src);
                fread(&(freq.u8), 1, freqSize, src);
                tree->leaves[i].freq = (uint64_t)freq.u8;
            }
            break;
        case 2:
            for(i = 0; i < tree->uniqueChars; i++) {
                tree->leaves[i].type = is_leaf;
                tree->leaves[i].pred = NULL;
                fread(&tree->leaves[i].data.matching, 1, sizeof(int8_t), src);
                fread(&(freq.u16), 1, freqSize, src);
                tree->leaves[i].freq = (uint64_t)freq.u16;
            }
            break;
        case 4:
            for(i = 0; i < tree->uniqueChars; i++) {
                tree->leaves[i].type = is_leaf;
                tree->leaves[i].pred = NULL;
                fread(&tree->leaves[i].data.matching, 1, sizeof(int8_t), src);
                fread(&(freq.u32), 1, freqSize, src);
                tree->leaves[i].freq = (uint64_t)freq.u32;
            }
            break;
        default:
            for(i = 0; i < tree->uniqueChars; i++) {
                tree->leaves[i].type = is_leaf;
                tree->leaves[i].pred = NULL;
                fread(&tree->leaves[i].data.matching, 1, sizeof(int8_t), src);
                fread(&(freq.u64), 1, freqSize, src);
                tree->leaves[i].freq = (uint64_t)freq.u64;
            }
            break;
    }
    return tree;
}

HuffmanTree* RebuildHuffmanTree(FILE *src) {
    HuffmanTree *tree = NULL;
    tree = RebuildLeaves(src);

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

    int16_t i = tree->uniqueChars;
    Node *subt1 = NULL, *subt2 = NULL;

    do {
        if(subt1) {
            if(tree->leaves[i-1].freq >= subt1->freq) {
                if(subt2 != NULL && subt2->freq <= subt1->freq)
                    subt2 = MakeNode(tree->leaves+i-1, subt2);
                else subt1 = MakeNode(tree->leaves+i-1, subt1);
                i--;
            } else {
                if(subt2 != NULL) {
                    if (subt2->freq <= subt1->freq)
                        subt2 = MakeNode(tree->leaves+i-1, subt2);
                    else subt1 = MakeNode(tree->leaves+i-1, subt1);
                    i--;
                } else {
                    subt2 = MakeNode(tree->leaves+i-1, tree->leaves+i-2);
                    i -= 2;
                }
            }
        } else {
            subt1 = MakeNode(tree->leaves+i-1, tree->leaves+i-2);
            i -= 2;
        }
    } while(i >= 2);

    if(i == 1) {
        if(subt2 != NULL) {
            if (subt2->freq <= subt1->freq)
                subt2 = MakeNode(tree->leaves+i-1, subt2);
            else subt1 = MakeNode(tree->leaves+i-1, subt1);
            tree->root = MakeNode(subt1, subt2);
        } else tree->root = MakeNode(tree->leaves+i-1, subt1);
    } else {
        if(subt2 != NULL)
            tree->root = MakeNode(subt2, subt1);
        else tree->root = subt1;
    }
    return tree;
}

void Decompress(FILE *src, FILE *dest) {
    // Get compact file size
    fseek(src, 0, SEEK_END);
    int64_t size = ftell(src);
    rewind(src);

    HuffmanTree *tree = RebuildHuffmanTree(src);

    if(!tree || !tree->leaves || !tree->root) return;

    int16_t currentByteBits;
    int64_t totalBits;
    uint8_t srcBuff;
    int8_t destBuff;
    Node *aux = tree->root;

    // Get number of bits of the compacted text
    size -= ftell(src);
    totalBits = 8 * size;

    do {
        fread(&srcBuff, 1, sizeof(uint8_t), src);
        currentByteBits = 8;
        totalBits -= 8;
        do {
            if(srcBuff >= 128)
                aux = aux->right;
            else aux = aux->left;

            if(aux->type == is_leaf) {
                destBuff = aux->data.matching;
                if(destBuff == '\0') {
                    fclose(src);
                    fclose(dest);
                    return;
                }
                fwrite(&destBuff, 1, 1, dest);
                aux = tree->root;
            }
            currentByteBits--;
            if(currentByteBits > 0)
                srcBuff <<= 1;
        } while(currentByteBits > 0);
    } while(destBuff != '\0' && totalBits > 0);
    fclose(src);
    fclose(dest);
}

/*
HuffmanTree* RebuildLeaves(FILE *src) {
    HuffmanTree *tree = (HuffmanTree*)malloc(sizeof(HuffmanTree));
    uint8_t uniqueChars;
    int16_t i;
    fread(&uniqueChars, 1, sizeof(uint8_t), src);
    tree->uniqueChars = (int16_t)uniqueChars;
    tree->leaves = (Node*)malloc(tree->uniqueChars*sizeof(Node));
    for(i = 0; i < tree->uniqueChars; i++) {
        tree->leaves[i].type = is_leaf;
        fread(&tree->leaves[i].data, 1, sizeof(CompactChar), src);
    }
    return tree;
}

Node* RebuildRoot(Node *leaf) {
    uint8_t temp = leaf->data.compact;
    uint8_t bits = leaf->data.bits;
    Node *aux = leaf;
    do {
        aux->pred = (Node*)malloc(sizeof(Node));
        aux->pred->type = is_internal;
        if(temp % 2) {
            aux->pred->right = aux;
            aux->pred->left = NULL;
        } else {
            aux->pred->right = NULL;
            aux->pred->left = aux;
        }
        aux = aux->pred;
        temp >>= 1;
        bits--;
    } while(bits > 0);

    return aux;
}

HuffmanTree* RebuildHuffmanTree(FILE *src) {
    int16_t i;
    uint8_t code, bits;
    Node *aux;
    HuffmanTree *tree = NULL;
    tree = RebuildLeaves(src);
    tree->root = RebuildRoot(tree->leaves);
    for(i = 1; i < tree->uniqueChars; i++) {
        aux = tree->root;
        code = tree->leaves[i].data.compact;
        bits = tree->leaves[i].data.bits;
        code <<= 8 - bits;
        do {
            bits--;
            if(code >= 128) {
                if(bits == 0) {
                    aux->right = tree->leaves + i;
                } else {
                    if(aux->right == NULL) {
                        aux->right = (Node*)malloc(sizeof(Node));
                        aux->right->type = is_internal;
                        aux->right->right = NULL;
                        aux->right->left = NULL;
                    }
                    aux = aux->right;
                }
            }
            else {
                if(bits == 0) {
                    aux->left = tree->leaves + i;
                } else {
                    if(aux->left == NULL) {
                        aux->left = (Node*)malloc(sizeof(Node));
                        aux->left->type = is_internal;
                        aux->left->right = NULL;
                        aux->left->left = NULL;
                    }
                    aux = aux->left;
                }
            }
            code <<= 1;
        } while(bits > 0);
    }
    return tree;
}

void Decompress(FILE *src, FILE *dest) {
    HuffmanTree *tree = RebuildHuffmanTree(src);

    if(tree == NULL || tree->root == NULL) {
        printf("\nFile could not be decompressed.\n");
        return;
    }

    int16_t bits;
    uint8_t srcBuff;
    int8_t destBuff;
    Node *aux = tree->root;

    do {
        fread(&srcBuff, 1, 1, src);
        bits = 8;
        do {
            if(srcBuff >= 128)
                aux = aux->right;
            else aux = aux->left;

            if(aux->type == is_leaf) {
                destBuff = aux->data.matching;
                if(destBuff == '\0') {
                    fclose(src);
                    fclose(dest);
                    return;
                }
                fwrite(&destBuff, 1, 1, dest);
                aux = tree->root;
            }
            bits--;
            if(bits > 0)
                srcBuff <<= 1;
        } while(bits > 0);
    }
    while(destBuff != '\0');
}
 */