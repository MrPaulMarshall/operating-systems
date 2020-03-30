#ifndef mylib_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct String {
    int maxSize;
    int size;
    char* string;
} String;

typedef struct FilePair {
    char* file1;
    char* file2;
} FilePair;

typedef struct Block {
    FilePair* files;
    int numberOfOperations;
    char** diffResult;
} Block;

typedef struct BlocksArray {
    int maxSize;
    int size;
    Block** arr;
} BlocksArray;

String* createString(int maxSize);
void freeString(String* string);
void appendToString(String* string, char* str);

FilePair* createFilePair(char* file1, char* file2);
void freeFilePair(FilePair* pair);

BlocksArray* createBlocksArray(int maxSize);
void freeBlocksArray(BlocksArray* blocksArray);
void addToBlocksArray(BlocksArray* blocksArray, Block* block);
void removeBlock(BlocksArray* blocksArray, int index);

Block* parsePairToBlock(FilePair* pair);
int numberOfOperations(Block* block);
void removeOperation(Block* block, int index);
void freeBlock(Block* block);

void compareFiles(FilePair* pair, const char* tempFile);
void showTemporaryFile();
bool checkIfNewOperation(char c);
int countOperations(const char* fileName);
void loadOperationsFromFile(Block* block);

#endif