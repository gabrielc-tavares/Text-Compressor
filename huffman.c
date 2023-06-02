#include "huffman.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <stdbool.h>

typedef struct {
    enum {is_u8, is_u16, is_u32, is_u64} size;
    union {
        uint8_t u8;
        uint16_t u16;
        uint32_t u32;
        uint64_t u64;
    };
} UnsIntV;

void merge(Node *n, int16_t start, int16_t mid, int16_t end) {
    int16_t p1 = start, p2 = mid + 1, len = end - start + 1, i, j;
    Node *temp = (Node*) malloc(len * sizeof(Node));
    bool end1 = false, end2 = false;

    for(i = 0; i < len; i++) {
        if(!end1 && !end2) {
            if(n[p1].freq > n[p2].freq)
                temp[i] = n[p1++];
            else
                temp[i] = n[p2++];

            if(p1 > mid) end1 = true;
            if(p2 > end) end2 = true;
        }
        else if(!end1)
            temp[i] = n[p1++];
        else
            temp[i] = n[p2++];
    }

    for(i = 0, j = start; i < len; i++, j++)
        n[j] = temp[i];

    free(temp);
}

void mergeSort(Node *n, int16_t start, int16_t end) {
    if(start < end) {
        int16_t mid = (int16_t) floor((double) (start + end) / 2);
        mergeSort(n, start, mid);
        mergeSort(n, mid + 1, end);
        merge(n, start, mid, end);
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
    if((tree->uniqueCharArray = (Node*) malloc(128 * sizeof(Node))) == NULL)
        return NULL;

    int i, j = 1;
    char buf;

    // '\0' char will represent EOF
    tree->uniqueCharArray[0].data.corresp = 0;
    tree->uniqueCharArray[0].freq = 1;
    tree->uniqueCharArray[0].type = is_leaf;
    tree->uniqueCharArray[0].pred = NULL;

    for(i = 1; i < 128; i++) {
        tree->uniqueCharArray[i].data.corresp = i;
        tree->uniqueCharArray[i].freq = 0;
        tree->uniqueCharArray[i].type = is_leaf;
        tree->uniqueCharArray[i].pred = NULL;
    }

    while((buf = fgetc(src)) != EOF)
        tree->uniqueCharArray[buf].freq++;

    rewind(src); // Go back to the beginning of the file

    while(j < i) {
        if(tree->uniqueCharArray[j].freq == 0) {
            i--;
            Node aux = tree->uniqueCharArray[i];
            tree->uniqueCharArray[i] = tree->uniqueCharArray[j];
            tree->uniqueCharArray[j] = aux;
        } else j++;
    }

    tree->uniqueCharCount = i;
    tree->uniqueCharArray = (Node*) realloc(tree->uniqueCharArray, i * sizeof(Node));
    mergeSort(tree->uniqueCharArray, 0, i); // Sort nodes
    return tree;
}

Node* makeNode(Node *left, Node *right) {
    Node *n = (Node*) malloc(sizeof(Node));
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
    HuffmanTree *tree = (HuffmanTree*) malloc(sizeof(HuffmanTree));

    if(tree == NULL || (tree = buildLeaves (src, tree)) == NULL)
        return NULL;

    if(tree->uniqueCharCount == 1) {
        tree->root = tree->uniqueCharArray;
        return tree;
    }
    Node *subt1 = makeNode(tree->uniqueCharArray + tree->uniqueCharCount - 1,
                           tree->uniqueCharArray + tree->uniqueCharCount - 2), *subt2 = NULL;
    int16_t i = (int16_t) tree->uniqueCharCount - 2;

    while(i >= 2) {
        if(tree->uniqueCharArray[i - 1].freq >= subt1->freq) {
            if(subt2 != NULL && subt2->freq <= subt1->freq)
                subt2 = makeNode(tree->uniqueCharArray + i - 1, subt2);
            else subt1 = makeNode(tree->uniqueCharArray + i - 1, subt1);
            i--;
        } else if(subt2 != NULL) {
            if(subt2->freq <= subt1->freq)
                subt2 = makeNode(tree->uniqueCharArray + i - 1, subt2);
            else subt1 = makeNode(tree->uniqueCharArray + i - 1, subt1);
            i--;
        } else {
            subt2 = makeNode(tree->uniqueCharArray + i - 1, tree->uniqueCharArray + i - 2);
            i -= 2;
        }
    }
    if(i == 1) {
        if(subt2 != NULL) {
            if(subt2->freq <= subt1->freq)
                subt2 = makeNode(tree->uniqueCharArray + i - 1, subt2);
            else subt1 = makeNode(tree->uniqueCharArray + i - 1, subt1);
            tree->root = makeNode(subt1, subt2);
        } else tree->root = makeNode(tree->uniqueCharArray + i - 1, subt1);
    }
    else if(subt2 != NULL) tree->root = makeNode(subt2, subt1);
    else tree->root = subt1;

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
        if(aux == pred->right)
            comp |= 1;
        aux = aux->pred;
    }
    n->data.code = reverse(comp);
    n->data.code >>= 64 - n->data.size;
}

HuffmanTree* setCompactChars(HuffmanTree *tree) {
    for(unsigned char i = 0; i < tree->uniqueCharCount; i++)
        getCompactChar(tree->uniqueCharArray + i);
    return tree;
}

CompactChar searchCompactChar(Node *leaves, char c) {
    unsigned char i = 0;
    do {
        if(leaves[i].data.corresp == c)
            return leaves[i].data;
        i++;
    } while(1);
}

void writeCompactCharData(FILE *dest, HuffmanTree *tree) {
    unsigned char freqSize, i;
    UnsIntV freq;

    if(tree->uniqueCharArray[0].freq < 256) {
        freqSize = 1;
        fwrite(&freqSize, 1, 1, dest);

        for(i = 0; i < tree->uniqueCharCount; i++) {
            fwrite(&tree->uniqueCharArray[i].data.corresp, 1, 1, dest);
            freq.u8 = (uint8_t) tree->uniqueCharArray[i].freq;
            fwrite(&freq.u8, 1, 1, dest);
        }
    } else if(tree->uniqueCharArray[0].freq < 65536) {
        freqSize = 2;
        fwrite(&freqSize, 1, 1, dest);

        for(i = 0; i < tree->uniqueCharCount; i++) {
            fwrite(&tree->uniqueCharArray[i].data.corresp, 1, 1, dest);
            freq.u16 = (uint16_t) tree->uniqueCharArray[i].freq;
            fwrite(&freq.u16, 1, 2, dest);
        }
    } else if(tree->uniqueCharArray[0].freq < 4294967296) {
        freqSize = 4;
        fwrite(&freqSize, 1, 1, dest);

        for(i = 0; i < tree->uniqueCharCount; i++) {
            fwrite(&tree->uniqueCharArray[i].data.corresp, 1, 1, dest);
            freq.u32 = (uint32_t) tree->uniqueCharArray[i].freq;
            fwrite(&freq.u32, 1, 4, dest);
        }
    } else {
        freqSize = 8;
        fwrite(&freqSize, 1, 1, dest);

        for(i = 0; i < tree->uniqueCharCount; i++) {
            fwrite(&tree->uniqueCharArray[i].data.corresp, 1, 1, dest);
            freq.u64 = (uint64_t) tree->uniqueCharArray[i].freq;
            fwrite(&freq.u64, 1, 8, dest);
        }
    }
}

int compressFile(FILE *src, FILE *dest) {
    HuffmanTree *tree = buildHuffmanTree(src);

    if(tree == NULL) {
        printf("\nThe file could not be compressed.\n");
        return 0;
    }
    char srcBuff;
    unsigned char bits = 64, shr;
    uint64_t destBuff, *comptxt, i = 0;
    UnsIntV lastChar;
    CompactChar comp;

    tree = setCompactChars(tree);
    fwrite(&tree->uniqueCharCount, 1, 1, dest);
    writeCompactCharData(dest, tree);

    comptxt = (uint64_t*) malloc (tree->root->freq * sizeof(uint64_t));
    comptxt[0] = 0;

    do {
        if((srcBuff = fgetc (src)) == EOF) srcBuff = '\0';
        comp = searchCompactChar (tree->uniqueCharArray, srcBuff);

        if(bits >= comp.size) {
            bits -= comp.size;
            comp.code <<= bits;
            comptxt[i] |= comp.code;

            if(srcBuff == '\0') {
                if(bits >= 56) {
                    comptxt[i] >>= bits;
                    bits -= 56;
                    lastChar.u8 = (uint8_t) comptxt[i];
                    lastChar.u8 <<= bits;
                    fwrite(&lastChar.u8, 1, 1, dest);
                } else if(bits >= 48) {
                    comptxt[i] >>= bits;
                    bits -= 48;
                    lastChar.u16 = (uint16_t) comptxt[i];
                    lastChar.u16 <<= bits;
                    fwrite(&lastChar.u16, 1, 2, dest);
                } else if(bits >= 32) {
                    comptxt[i] >>= bits;
                    bits -= 32;
                    lastChar.u32 = (uint32_t) comptxt[i];
                    lastChar.u32 <<= bits;
                    fwrite (&lastChar.u32, 1, 4, dest);
                } else fwrite(&comptxt[i], 1, 8, dest);

                fclose(src);
                fclose(dest);
                free(comptxt);
                return 1;
            }
            if(bits == 0) {
                fwrite(&comptxt[i], 1, sizeof(uint64_t), dest);
                bits = 64;
                i++;
                comptxt[i] = 0;
            }
        } else {
            shr = comp.size - bits;
            bits = 64 - shr;
            destBuff = 0;
            while(shr) {
                destBuff >>= 1;
                if(comp.code % 2)
                    destBuff |= 0x8000000000000000;
                comp.code >>= 1;
                shr--;
            }
            comptxt[i] |= comp.code;
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
                    fwrite(&lastChar.u8, 1, 1, dest);
                } else if(bits >= 48) {
                    comptxt[i] >>= bits;
                    bits -= 48;
                    lastChar.u16 = (uint16_t) comptxt[i];
                    lastChar.u16 <<= bits;
                    fwrite(&lastChar.u16, 1, 2, dest);
                } else if(bits >= 32) {
                    comptxt[i] >>= bits;
                    bits -= 32;
                    lastChar.u32 = (uint32_t) comptxt[i];
                    lastChar.u32 <<= bits;
                    fwrite(&lastChar.u32, 1, 4, dest);
                } else fwrite(&comptxt[i], 1, 8, dest);

                fclose(src);
                fclose(dest);
                free(comptxt);
                return 1;
            }
        }
    } while(1);
}

HuffmanTree* rebuildLeaves(FILE *src) {
    HuffmanTree *tree = (HuffmanTree*) malloc(sizeof(HuffmanTree));

    if(tree == NULL) return NULL;

    fread(&tree->uniqueCharCount, 1, 1, src);

    if((tree->uniqueCharArray = (Node*) malloc(tree->uniqueCharCount * sizeof(Node))) == NULL)
        return NULL;

    UnsIntV freq;
    unsigned char freqSize, i;
    fread(&freqSize, 1, 1, src);

    switch(freqSize) {
        case 1:
            for(i = 0; i < tree->uniqueCharCount; i++) {
                tree->uniqueCharArray[i].type = is_leaf;
                tree->uniqueCharArray[i].pred = NULL;
                fread(&tree->uniqueCharArray[i].data.corresp, 1, 1, src);
                fread(&freq.u8, 1, freqSize, src);
                tree->uniqueCharArray[i].freq = (uint64_t) freq.u8;
            }
            break;
        case 2:
            for(i = 0; i < tree->uniqueCharCount; i++) {
                tree->uniqueCharArray[i].type = is_leaf;
                tree->uniqueCharArray[i].pred = NULL;
                fread(&tree->uniqueCharArray[i].data.corresp, 1, 1, src);
                fread(&freq.u16, 1, freqSize, src);
                tree->uniqueCharArray[i].freq = (uint64_t) freq.u16;
            }
            break;
        case 4:
            for(i = 0; i < tree->uniqueCharCount; i++) {
                tree->uniqueCharArray[i].type = is_leaf;
                tree->uniqueCharArray[i].pred = NULL;
                fread(&tree->uniqueCharArray[i].data.corresp, 1, 1, src);
                fread(&freq.u32, 1, freqSize, src);
                tree->uniqueCharArray[i].freq = (uint64_t) freq.u32;
            }
            break;
        default:
            for(i = 0; i < tree->uniqueCharCount; i++) {
                tree->uniqueCharArray[i].type = is_leaf;
                tree->uniqueCharArray[i].pred = NULL;
                fread(&tree->uniqueCharArray[i].data.corresp, 1, 1, src);
                fread(&freq.u64, 1, freqSize, src);
                tree->uniqueCharArray[i].freq = (uint64_t) freq.u64;
            }
            break;
    }
    return tree;
}

HuffmanTree* rebuildHuffmanTree(FILE *src) {
    HuffmanTree *tree = rebuildLeaves(src);

    if(tree == NULL) {
        printf("\nError while decompressing file\n");
        return NULL;
    }
    if(tree->uniqueCharCount == 1) {
        tree->root = tree->uniqueCharArray;
        return tree;
    }
    Node *subt1 = makeNode (tree->uniqueCharArray + tree->uniqueCharCount - 1,
                            tree->uniqueCharArray + tree->uniqueCharCount - 2), *subt2 = NULL;
    unsigned char i = tree->uniqueCharCount - 2;

    while(i >= 2) {
        if(tree->uniqueCharArray[i - 1].freq >= subt1->freq) {
            if(subt2 != NULL && subt2->freq <= subt1->freq)
                subt2 = makeNode(tree->uniqueCharArray + i - 1, subt2);
            else subt1 = makeNode(tree->uniqueCharArray + i - 1, subt1);
            i--;
        } else if(subt2 != NULL) {
            if(subt2->freq <= subt1->freq)
                subt2 = makeNode(tree->uniqueCharArray + i - 1, subt2);
            else subt1 = makeNode(tree->uniqueCharArray + i - 1, subt1);
            i--;
        } else {
            subt2 = makeNode(tree->uniqueCharArray + i - 1, tree->uniqueCharArray + i - 2);
            i -= 2;
        }
    }
    if(i == 1) {
        if(subt2 != NULL) {
            if(subt2->freq <= subt1->freq)
                subt2 = makeNode(tree->uniqueCharArray + i - 1, subt2);
            else subt1 = makeNode(tree->uniqueCharArray + i - 1, subt1);
            tree->root = makeNode(subt1, subt2);
        } else tree->root = makeNode(tree->uniqueCharArray + i - 1, subt1);
    }
    else if(subt2 != NULL) tree->root = makeNode(subt2, subt1);
    else tree->root = subt1;
    return tree;
}

int decompressFile(FILE *src, FILE *dest) {
    fseek(src, 0, SEEK_END); // Get file size in bytes
    int64_t size = ftell (src);
    rewind (src);

    HuffmanTree *tree = rebuildHuffmanTree(src);

    if(tree == NULL) return 0;

    int16_t bits;
    uint64_t srcBuff;
    char destBuff;
    Node *aux = tree->root;
    size -= ftell (src); // Get compacted text size in bytes

    do {
        bits = fread (&srcBuff, 1, 8, src);
        size -= bits;
        bits *= 8;
        if(bits < 64) srcBuff <<= 64 - bits;
        do {
            if(srcBuff >= 0x8000000000000000)
                aux = aux->right;
            else aux = aux->left;

            if(aux->type == is_leaf) {
                destBuff = aux->data.corresp;
                if(destBuff == '\0') {
                    fclose(src);
                    fclose(dest);
                    return 1;
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
    return 1;
}