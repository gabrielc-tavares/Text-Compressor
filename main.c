#include "huffman.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    char *destPath = (char*) malloc(strlen(argv[1]) + 1);
    char *dir = (char*) malloc(strlen(argv[1])), *srcExt;
    FILE *src, *dest;

    strcpy(dir, argv[1]);
    dir = strtok(dir, ".");
    srcExt = strtok(NULL, "\0");

    if(strcmp(srcExt, "txt") == 0) {
        if((src = fopen(argv[1], "r")) == NULL) {
            printf("\nError while creating/opening source file.\n");
            return -1;
        }
        strcpy(destPath, dir);
        strcat(destPath, ".hzip");

        if((dest = fopen(destPath, "wb")) == NULL) {
            printf("\nError while creating/opening destiny file.\n");
            return -1;
        }

        if(compressFile(src, dest)) {
            printf("\nThe file was compressed succesfully.\n");
            return 0;
        }
    } else if(strcmp(srcExt, "hzip") == 0) {
        if((src = fopen(argv[1], "rb")) == NULL) {
            printf("\nError while creating/opening source file.\n");
            return -1;
        }
        strcpy(destPath, dir);
        strcat(destPath, ".txt");

        if((dest = fopen(destPath, "w")) == NULL) {
            printf("\nError while creating/opening destiny file.\n");
            return -1;
        }

        if(decompressFile(src, dest)) {
            printf("\nThe file was decompressed succesfully.\n");
            return 0;
        }
    } else
        printf("\nInvalid file format.\n");

    return -1;
}