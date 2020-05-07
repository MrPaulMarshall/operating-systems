#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char* args[]) {
    if (argc < 2) {
        perror("Bad number of arguments");
        exit(EXIT_FAILURE);
    }
    char* command = calloc(10 + strlen(args[1]), sizeof(char));
    strcpy(command, "sort ");
    strcat(command, args[1]);

    FILE* fpipe = popen(command, "w");
    pclose(fpipe);
    free(command);
    return 0;
}