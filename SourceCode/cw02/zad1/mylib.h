#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void showFile(const char* file);

void generateFile(const char* file, int lines, int length);

void copyFileSys(const char* file1Name, const char* file2Name, int lines, int length);
void copyFileLib(const char* file1Name, const char* file2Name, int lines, int length);

void sortFileSys(const char* file, int length, int lines);
void sortFileLib(const char* file, int length, int lines);
