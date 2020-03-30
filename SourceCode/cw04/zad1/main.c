#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#define STOPPED 1
#define RUNNING 0

int programState = RUNNING;

void gotSIGTSTP(int signal) {
    if (programState == RUNNING) {
        programState = STOPPED;
        printf("Oczekuje na CTRL+Z - kontynuacja albo CTR+C - zakonczenie programu\n");
    } else {
        programState = RUNNING;
    }
}

void gotSIGINT(int signal) {
    printf("Odebrano signal SIGINT\n");
    exit(0);
}

int main() {
    struct sigaction act;
    act.sa_handler = gotSIGTSTP;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGTSTP, &act, NULL);

    signal(SIGINT, gotSIGINT);

    while (1) {
        if (programState == RUNNING) {
            system("ls -l");
            sleep(1);
        }
    }
    return 0;
} 
