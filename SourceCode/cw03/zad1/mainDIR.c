#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mylib.h"

// przykladowe wywolanie:
//      ./myFind PATH [-option VALUE]

// zatem:
//  args[1]    = PATH
//  args[2n]   = -option
//  args[2n+1] = VALUE

void safeFree(char* string) {
    if (string != NULL) {
        free(string);
    }
}

int main(int argc, char* args[]) {
    if (argc == 1) {
        printf("Ta wersja programu find udostepnia operacje:\n");
        printf("-> -atime\n");
        printf("-> -mtime\n");
        printf("-> -maxdepth\n\n");
        
        return 0;
    }

    char* path = strdup(args[1]);
    // printf("PATH: \"%s\"\n", path);
    
    char* isError = NULL;
    char* atimeValue = NULL;
    char* mtimeValue = NULL;
    char* maxdepthValue = NULL;

    for (int i = 2; i + 1 < argc; i += 2) {
        if (strcmp(args[i], "-atime") == 0 && atimeValue == NULL) {
            atimeValue = strdup(args[i+1]);
        } else if (strcmp(args[i], "-mtime") == 0 && mtimeValue == NULL) {
            mtimeValue = strdup(args[i+1]);
        } else if (strcmp(args[i], "-maxdepth") == 0 && maxdepthValue == NULL) {
            maxdepthValue = strdup(args[i+1]);
        } else {
            isError = strdup("Podano bledne opcje polecenia");
        }
    }

    if (isError == NULL) {
        // wykonanie funkcji z mojej biblioteki do szukania
        searchDIR(path, atimeValue, mtimeValue, maxdepthValue);
    } else {
        printf("%s\n", isError);
    }

    safeFree(isError);
    safeFree(atimeValue);
    safeFree(mtimeValue);
    safeFree(maxdepthValue);

    safeFree(path);
    return 0;
}
