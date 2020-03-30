#include <stdio.h>
#include <stdlib.h>
#include "mylib.h"
#include <string.h>
#include <stdbool.h>

// functions about Strings

String* createString(int maxSize) {
    String* string = (String *)calloc(1, sizeof(String));
    string->maxSize = maxSize;
    string->size = 0;
    string->string = (char *)calloc(maxSize, sizeof(char));

    return string;
}

void freeString(String* string) {
    free(string->string);
    free(string);
}

void appendToString(String* string, char* str) {
    if(string->maxSize < string->size + strlen(str)) {
        string->maxSize *= 2;
        string->string = realloc(string->string, string->maxSize);
    }
    string->size += strlen(str);
    strcat(string->string, str);
}


// functions about pairs of files

FilePair* createFilePair(char* file1, char* file2) {
    FilePair* pair = (FilePair*)calloc(1, sizeof(FilePair));
    
    pair->file1 = (char *)calloc(strlen(file1), sizeof(char));
    pair->file1[0] = '\0';
    strcpy(pair->file1, file1);
    
    pair->file2 = (char *)calloc(strlen(file2), sizeof(char));
    pair->file2[0] = '\0';
    strcpy(pair->file2, file2);

    return pair;
}

void freeFilePair(FilePair* pair) {
    free(pair->file1);
    free(pair->file2);
    free(pair);
}


// functions about BlocksArray

BlocksArray* createBlocksArray(int maxSize) {
    BlocksArray* blocksArray = (BlocksArray *)calloc(1, sizeof(BlocksArray));
    blocksArray->maxSize = maxSize;
    blocksArray->size = 0;
    blocksArray->arr = (Block **)calloc(maxSize, sizeof(Block*));
    
    int i;
    for(i = 0; i < maxSize; i++) {
        blocksArray->arr[i] = 0;
    }
    return blocksArray;
}

void freeBlocksArray(BlocksArray* blocksArray) {
    int i;
    for(i = 0; i < blocksArray->size; i++) {
        freeBlock(blocksArray->arr[i]);
    }
    free(blocksArray->arr);
    free(blocksArray);
}

void addToBlocksArray(BlocksArray* blocksArray, Block* block) {
    if(blocksArray->size >= blocksArray->maxSize) {
        return;
    }
    blocksArray->arr[blocksArray->size] = block;
    blocksArray->size++;
}

void removeBlock(BlocksArray* blocksArray, int index) {
    if(blocksArray == NULL || index < 0 || index >= blocksArray->size) {
        return;
    }
    freeBlock(blocksArray->arr[index]);
    int i;
    for(i = index; i < blocksArray->size - 1; i++) {
        blocksArray->arr[i] = blocksArray->arr[i+1];
    }
    blocksArray->size--;
}

// functions about Blocks

Block* parsePairToBlock(FilePair* pair) {
    Block* block = (Block *)calloc(1, sizeof(Block));

    compareFiles(pair, "tempFile.txt");
    block->files = pair;

    loadOperationsFromFile(block);

    return block;
}

// parser pary plikow do bloku jest ponizej

int numberOfOperations(Block* block) {
    return block->numberOfOperations;
}

void removeOperation(Block* block, int index) {
    if(block == NULL || index < 0 || index >= block->numberOfOperations) return;
    
    free(block->diffResult[index]);
    int i;
    for(i = index; i < block->numberOfOperations - 1; i++) {
        block->diffResult[i] = block->diffResult[i+1];
    }
    block->numberOfOperations--;
}

void freeBlock(Block* block) {
    int i;
    for(i = 0; i < block->numberOfOperations; i++) {
        free(block->diffResult[i]);
    }
    freeFilePair(block->files);
    free(block);
}

// other functions

void compareFiles(FilePair* pair, const char* tempFile) {
    // przykladowa komenda:
    //  diff a.txt b.txt > tempFile.txt

    int length = strlen(pair->file1) + strlen(pair->file2) + 40;
    char* command = (char *)calloc(length, sizeof(char));
    command[0] = '\0';

    strcat(command, "diff ");
    strcat(command, pair->file1);
    strcat(command, " ");
    strcat(command, pair->file2);
    strcat(command, " > ");
    strcat(command, tempFile);

    system(command);

    free(command);
}

void showTemporaryFile() {
    system("cat tempFile.txt");
}


// wczytywanie operacji z pliku tymczasowego do blokow

bool checkIfNewOperation(char c) {
    return !(c == '<' || c == '>' || c == '-');
}

int countOperations(const char* fileName) {
    int read;
    char* line = NULL;
    int length = 0;
    int countOperations = 0;

    FILE* fp = fopen(fileName, "r");
    if(fp == NULL) {
        printf("Nieudana proba odczytu pliku\n");
        exit(1);
    }
    while((read = getline(&line, &length, fp)) != -1) {
        if(checkIfNewOperation(line[0])) {
            countOperations++;
        }
    }

    fclose(fp);
    if(line) free(line);

    return countOperations;
}

void loadOperationsFromFile(Block* block) {
    FILE* file = NULL;
    char* line = NULL;
    int length = 0;
    int read;

    // zapisuje dotyczhczasowe informacje do bloku
    int numberOfOperations = countOperations("tempFile.txt");
    block->numberOfOperations = numberOfOperations;
    block->diffResult = (char **)calloc(numberOfOperations, sizeof(char *));

    String** strArr = (String **)calloc(numberOfOperations, sizeof(String *));
    int i;
    for(i = 0; i < numberOfOperations; i++) {
        strArr[i] = createString(100);
    }

    // poczatek czytania pliku
    file = fopen("tempFile.txt", "r");

    if(file == NULL) {
        printf("Blad w trakcie otwierania pliku tymczasowego\n");
        exit(1);
    }

    // czytanie pliku
    int blockIndex = 0;
    while((read = getline(&line, &length, file)) != -1) {
        if(checkIfNewOperation(line[0])) {
            blockIndex++;
        }
        appendToString(strArr[blockIndex - 1], line);
    }

    // koniec czytania pliku
    fclose(file);

    // przenosze operacje do bloku
    for(i = 0; i < numberOfOperations; i++) {
        strcpy(block->diffResult[i], strArr[i]->string);
    }

    // czyszczenie pamieci
    for(i = 0; i < numberOfOperations; i++) {
        freeString(strArr[i]);
    }
    free(strArr);
}
