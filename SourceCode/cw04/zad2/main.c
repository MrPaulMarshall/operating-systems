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

void gotSignal(int signal) {
    printf("Otrzymano sygnal %i\n", signal);
}
void ignore(int signal) {}

void setMask(int signal);

void printHelp();
int parse(const char* arg);

// ------------

int main(int argc, char* args[]) {
    if (argc == 1) {
        printHelp();
        exit(0);
    }
    if (argc > 2) {
        printf("Za duzo argumentow\n");
        exit(1);
    }

    int option = parse(args[1]);


    switch(option) {
        case IGNORE:
            signal(SIGUSR1, ignore); break;
        case HANDLER:
            signal(SIGUSR1, gotSignal); break;
        case MASK:
            signal(SIGUSR1, gotSignal);
            setMask(SIGUSR1);
            break;
        case PENDING:
            signal(SIGUSR1, gotSignal);
            setMask(SIGUSR1);
            
            // sprawdzam PRZED
            sigset_t pending;
            sigpending(&pending);
            if (sigismember(&pending, SIGUSR1) == 1) {
                printf("Parent: SIGUSR1 is pending\n");
            } else {
                printf("Parent: SIGUSR1 not yet pending\n");
            }
            break;
        default:
            printf("Switch error\n");
            exit(1);
    }

    printf("Parent: Beware! I send signal\n");
    raise(SIGUSR1);

    if (option == PENDING) {
        // sprawdzam PO
        sigset_t pendingP;
        sigpending(&pendingP);
        if (sigismember(&pendingP, SIGUSR1) == 1) {
            printf("Parent: SIGUSR1 is pending\n");
        } else {
            printf("Parent: SIGUSR1 not yet pending\n");
        }
    }

    // dla procesu potomnego
    if (fork() == 0) {
        if (option != PENDING) {
            printf("Child: Beware! I send signal\n");
            raise(SIGUSR1);
        } else {
            sigset_t pendingC;
            sigpending(&pendingC);
            if (sigismember(&pendingC, SIGUSR1) == 1) {
                printf("Child: SIGUSR1 is pending\n");
            } else {
                printf("Child: SIGUSR1 not pending\n");
            }
        }
        exit(0);
    }

    // po wywolaniu exec()
    if (option != HANDLER) {
        char* newArgs[2];
        newArgs[0] = strdup(args[1]);
        newArgs[1] = NULL;
        // newArgs[1] = strdup(args[1]);
        execv("./auxiliary", newArgs);
    }

    return 0;
} 

// -------------

void printHelp() {
    printf("Execute program like this:\n");
    printf("$> ./main __option__\n");
    printf("Gdzie __option__ nalezy do zbioru {ignore, handler, mask, pending}\n");
}

int parse(const char* arg) {
    int option;
    if (strcmp(arg, "ignore") == 0) {
        option = IGNORE;
    } else if (strcmp(arg, "handler") == 0) {
        option = HANDLER;
    } else if (strcmp(arg, "mask") == 0) {
        option = MASK;
    } else if (strcmp(arg, "pending") == 0) {
        option = PENDING;
    } else {
        printf("Wrong argument: invalid option\n");
        exit(1);
    }

    return option;
}

void setMask(int signal) {
    sigset_t oldmask;
    sigset_t blockmask;
    sigemptyset(&blockmask);
    sigaddset(&blockmask, signal);
    sigprocmask(SIG_BLOCK, &blockmask, &oldmask);
}
