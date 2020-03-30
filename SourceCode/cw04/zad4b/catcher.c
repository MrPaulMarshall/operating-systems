#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#include <execinfo.h>


#define MD_KILL 1
#define MD_SIGQUEUE 2
#define MD_SIGRT 3

#define SIG_NORMAL 1
#define SIG_END 2

void parseArgs(int argc, char* args[], int* mode);


pid_t senderPID = 0;
int countGotSignals = 0;
bool gotEndSignal = false;
bool canSendNext = false;

void setSigaction(struct sigaction* act, int normalSignal, int endSignal);
void sendSignal(int signal, int targetPID, int signalIndex, int mode);

void gotSignal(int signal, siginfo_t* info, void* ucontext);

// ---

int main(int argc, char* args[]) {
    int mode;
    parseArgs(argc, args, &mode);

    printf("PID-catcher: %i\n\n", getpid());

    // to set handlers
    struct sigaction act;
    act.sa_sigaction = (void *)gotSignal;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_SIGINFO;

    if (mode == MD_KILL || mode == MD_SIGQUEUE) {
        setSigaction(&act, SIGUSR1, SIGUSR2);
    } else {
        setSigaction(&act, SIGRTMIN+1, SIGRTMIN+2);
    }

    // wait for first signal to identify sender
    while (senderPID == 0) {}

    // loop - send signals
    int i = 0;
    while (gotEndSignal == false) {
        if (canSendNext) {
            canSendNext = false;
            i++;
            printf("Catcher is sending signal %i\n", i);
            sendSignal(SIG_NORMAL, senderPID, i, mode);
        }
    }

    // last signal only if got ending signal from sender
    while (gotEndSignal == false) {}
    sendSignal(SIG_END, senderPID, countGotSignals, mode);

    // print raport
    printf("Catcher: I got %i signals from sender\n\n", countGotSignals);

    return 0;
}

// ---


void printHelp() {
    printf("To execute it properly, follow these steps:\n");
    printf("First execute './catcher __mode__' and read PID written on console\n");
    printf("Then execute './sender __catcher_PID__ __number_of_signals_to_send__ __mode__'\n");
    printf("Argument __mode__ is member of set: {kill, sigqueue, sigrt}\n");
    printf("Remember, that __mode__ needs to be the same in both programs\n");
    printf("\n");
}

void parseArgs(int argc, char* args[], int* mode) {
    if (argc == 1) {
        printHelp();
        exit(0);    
    }
    
    if (argc != 2) {
        printf("Wrong number of arguments - execute without arguments to get help\n\n");
    }
    
    // parse mode - how to send signals
    if (strcmp(args[1], "kill") == 0) {
        *mode = MD_KILL;
    } else if (strcmp(args[1], "sigqueue") == 0) {
        *mode = MD_SIGQUEUE;
    } else if (strcmp(args[1], "sigrt") == 0) {
        *mode = MD_SIGRT;
    } else {
        printf("Invalid mode\n");
        exit(1);
    }
}


void setSigaction(struct sigaction* act, int normalSignal, int endSignal) {
    sigaction(normalSignal, act, NULL);
    sigaction(endSignal, act, NULL);
}


void sendSignal(int signalType, int targetPID, int signalIndex, int mode) {
    int signal;    
    switch (mode) {
        case MD_KILL:
            if (signalType == SIG_NORMAL) signal = SIGUSR1;
            else signal = SIGUSR2;
            
            kill(targetPID, signal);
            break;
        case MD_SIGQUEUE:
            if (signalType == SIG_NORMAL) signal = SIGUSR1;
            else signal = SIGUSR2;

            union sigval value;
            value.sival_int = signalIndex;
            sigqueue(targetPID, signal, value);
            break;
        case MD_SIGRT:
            if (signalType == SIG_NORMAL) signal = SIGRTMIN+1;
            else signal = SIGRTMIN+2;

            kill(targetPID, signal);
            break;
        default:
            printf("Switch error\n");
            exit(1);
    }
}

void gotSignal(int signal, siginfo_t* info, void* ucontext) {
    // take senderPID
    if (senderPID == 0) {
        canSendNext = true;
        senderPID = info->si_pid;
    }

    // for standard signals
    if (signal == SIGUSR1 && gotEndSignal == false) {
        countGotSignals++;
        canSendNext = true;
    }
    if (signal == SIGUSR2) {
        gotEndSignal = true;
    }

    // for real-time signals
    if (signal == SIGRTMIN+1 && gotEndSignal == false) {
        countGotSignals++;
        canSendNext = true;
    }
    if (signal == SIGRTMIN+2) {
        gotEndSignal = true;
    }
}