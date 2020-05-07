#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <time.h>

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

    srand(time(NULL));

    FILE* input = fopen(file, "r");
    int pipe = open(pipeFile, O_WRONLY);
    if (pipe < 0 || input == NULL) {
        error("Fopen() - input or pipe");
    }

    char buffer[N];

    while (fgets(buffer, N, input) != NULL) {
        sleep(rand() % 3 + 1);
        char msg[N + 20];
        sprintf(msg, "#%d#%s\n", getpid(), buffer);
        write(pipe, msg, strlen(msg));
    }

    close(pipe);
    fclose(input);

    return 0;
}

// ---
// end
// ---

// implementations

void printHelp() {
    printf("Execute it by: \"make runProducer\"\n");
    printf("-> but first execute \"make createPipe\"\n");
    printf("-> after execute \"make runConsumer\"\n");
}

void error(char* e) {
    perror(e);
    exit(EXIT_FAILURE);
}
