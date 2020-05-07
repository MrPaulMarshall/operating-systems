#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

// declarations

void printHelp();

void error(char* e);

// ----
// main
// ----

int main(int argc, char* args[]) {
    if (argc == 1) {
        printHelp();
        exit(0);
    }

    if (argc != 4) {
        error("Bad number of arguments");
    }

    char* pipeFile = args[1];
    char* file = args[2];
    int N = atoi(args[3]);

    if (N <= 0) {
        error("Bad argument - number of chars");
    }

    FILE* pipe = fopen(pipeFile, "r");
    FILE* output = fopen(file, "w");
    if (pipe == NULL || output == NULL) {
        error("Fopen() - output or pipe");
    }

    char buffer[N];

    while (fgets(buffer, N, pipe) != NULL) {
        fprintf(output, buffer, strlen(buffer));
    }

    fclose(pipe);
    fclose(output);

    return 0;
}

// ---
// end
// ---

// implementations

void printHelp() {
    printf("Execute it by: \"make runConsumer\"\n");
    printf("-> but first execute \"make createPipe\"\n");
    printf("-> and then execute \"make runProducer\"\n");
}

void error(char* e) {
    perror(e);
    exit(EXIT_FAILURE);
}
