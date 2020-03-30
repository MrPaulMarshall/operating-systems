#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>

#include <execinfo.h>

int countTSTP = 0;

void my_sigaction(int signal, siginfo_t* info, void* ucontext);

// ---

int main () {
    struct sigaction act;
    
    act.sa_sigaction = (void *)my_sigaction;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_SIGINFO;

    sigaction(SIGTSTP, &act, NULL);
    sigaction(SIGUSR1, &act, NULL);
    sigaction(SIGINT, &act, NULL);

    while (1) {
        // co 3 CTRL+Z program wysyla do siebie SIGUSR1
        if (countTSTP > 0 && countTSTP % 3 == 0) {
            raise(SIGUSR1);
        } else {
            printf(".\n");
        }

        sleep(1);
    }

}

// ---

void my_sigaction(int signal, siginfo_t* info, void* ucontext) {
    printf("Signal: %i\n", signal);
    printf("si_uid: %i\n", info->si_uid);
    printf("si_utime: %li\n", info->si_utime);
    printf("si_addr: %p\n", info->si_addr);

    if (signal == SIGTSTP)
        countTSTP++;
    if (signal == SIGINT)
        exit(0);

    printf("\n");
}
