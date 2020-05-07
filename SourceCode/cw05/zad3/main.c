#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char* args[]) {
    char* argsToExec[6][5] = { 
        {"./consumer", "myPipe", "./consumer_data/output.txt", "12", NULL},
        {"./producer", "myPipe", "./producer_data/f1.txt", "4", NULL},
        {"./producer", "myPipe", "./producer_data/f2.txt", "4", NULL},
        {"./producer", "myPipe", "./producer_data/f3.txt", "4", NULL},
        {"./producer", "myPipe", "./producer_data/f4.txt", "4", NULL},
        {"./producer", "myPipe", "./producer_data/f5.txt", "4", NULL}
    };

    mkfifo("myPipe", S_IRUSR | S_IWUSR);

    pid_t pidArr[6];

    for (int i = 0; i < 6; i++) {
        if ((pidArr[i] = fork()) == 0) {
            execvp(argsToExec[i][0], argsToExec[i]);
        }
    }

    for (int i = 0; i < 6; i++) {
        waitpid(pidArr[i], NULL, 0);
    }

    printf("Main has ended\n");

    return 0;
}