#include <stdio.h>

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#include <signal.h>
#include <string.h>

#define IGNORE 0
#define HANDLER 1
#define MASK 2
#define PENDING 3
#define UNDEFINED -1

int parse(int argc, char* args[]);

// ------------

int main(int argc, char* args[]) {
    int option = parse(argc, args);

    if (option == PENDING) {
        // sprawdzam PRZED
        sigset_t pending;
        sigpending(&pending);
        if (sigismember(&pending, SIGUSR1) == 1) {
            printf("Auxiliary: SIGUSR1 is pending\n");
        } else {
            printf("Auxiliary: SIGUSR1 not yet pending\n");
        }
    }

    printf("Auxiliary: Beware! I send signal\n");
    raise(SIGUSR1);

    if (option == PENDING) {
        // sprawdzam PO
        sigset_t pendingP;
        sigpending(&pendingP);
        if (sigismember(&pendingP, SIGUSR1) == 1) {
            printf("Auxiliary: SIGUSR1 is pending\n");
        } else {
            printf("Auxiliary: SIGUSR1 not yet pending\n");
        }
    }

    return 0;
} 

// ---

int parse(int argc, char* args[]) {
    // printf("Auxiliary: %i arguments\n", argc);
    // printf("First argument: %s\n", args[0]);
    if (argc < 1) {
        printf("Auxiliary: problem\n");
        exit(1);
    }

    if (strcmp(args[0], "ignore") == 0) {
        return IGNORE;
    } else if (strcmp(args[0], "mask") == 0) {
        return MASK;
    } else if (strcmp(args[0], "pending") == 0) {
        return PENDING;
    } else {
        printf("Auxiliary: Wrong argument\n");
        exit(1);
    }
}
