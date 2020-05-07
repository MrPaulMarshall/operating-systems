#include "header.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>

void atExit(void);
void killHandler(int signal);

// global variables

int semID = -1;
int ordersArrID = -1;

int workersPIDs[AllWorkNum] = {-1};

// main

int main() {
    srand(time(NULL));
    atexit(atExit);
    signal(SIGINT, killHandler);

    // clear "logs.txt" file
    FILE * fp = fopen("logs.txt", "w");
    if(fp != NULL) fclose(fp);

    // printf("Create set of semaphors\n");

    // create set of semaphores
    semID = semget(getSemaforsSetKey(), 6, IPC_CREAT | IPC_EXCL | 0666);

    if (semID < 0) {
        error("Couldn't open set of semaphores");
    } else {
        printf("Created set of semaphors\n");
    }

    // initialize semaphors to 0s
    union semun arg;
    arg.val = 0;

    semctl(semID, SEM_ARRAY, SETVAL, arg);

    semctl(semID, SEM_TO_PACK_COUNT, SETVAL, arg);
    semctl(semID, SEM_TO_SEND_COUNT, SETVAL, arg);
    
    semctl(semID, SEM_TO_CREATE_INDEX, SETVAL, arg);
    // semctl(semID, SEM_TO_PACK_INDEX, SETVAL, arg);
    // semctl(semID, SEM_TO_SEND_INDEX, SETVAL, arg);


    // create shared array
    if ((ordersArrID = shmget(getOrdersArrayKey(), sizeof(Orders), IPC_CREAT | IPC_EXCL | 0666)) < 0) {
        error("Didn't create orders array");
    } else {
        printf("Created array of orders\n");
    }

    // initialize shared memory as 0s
    Orders * orders = shmat(ordersArrID, NULL, 0);
    for (int i = 0; i < OrdersArraySize; i++) {
        orders->arr[i].type = TYPE_NULL;
        orders->arr[i].value = 0;
    }
    shmdt(orders);

    // print timestamp
    printMsg("START!");

    // send workers to the Rice Fields
    for (int i = 0; i < Workers1Num; i++) {
        int cPID = fork();
        if (cPID == 0) {
            char * execArgs[2] = {"./worker_1", NULL};
            if (execvp(execArgs[0], execArgs) < 0) {
                printf("Worker1 process created, but not executed");
                exit(EXIT_FAILURE);
            }
        }
        workersPIDs[i] = cPID;
    }
    for (int i = 0; i < Workers2Num; i++) {
        int cPID = fork(); 
        if (cPID == 0) {
            char * execArgs[2] = {"./worker_2", NULL};
            if (execvp(execArgs[0], execArgs) < 0) {
                printf("Worker2 process created, but not executed");
                exit(EXIT_FAILURE);
            }
        }
        workersPIDs[Workers1Num + i] = cPID;
    }
    for (int i = 0; i < Workers3Num; i++) {
        int cPID = fork();
        if (cPID == 0) {
            char * execArgs[2] = {"./worker_3", NULL};
            if (execvp(execArgs[0], execArgs) < 0) {
                printf("Worker3 process created, but not executed");
                exit(EXIT_FAILURE);
            }
        }
        workersPIDs[Workers1Num + Workers2Num + i] = cPID;
    }

    // wait for workers to end
    for (int i = 0; i < AllWorkNum; i++) {
        if (workersPIDs[i] > 0) {
            waitpid(workersPIDs[i], NULL, 0);
        }
    }

    semctl(semID, SEM_ARRAY, IPC_RMID, NULL);
    semctl(ordersArrID, IPC_RMID, IPC_RMID, NULL);
}

// other functions

void atExit(void) {
    // print timestamp
    printMsg("The end!");
    
    for (int i = 0; i < AllWorkNum; i++) {
        if (workersPIDs[i] > 0) {
            kill(SIGINT, workersPIDs[i]);
            waitpid(workersPIDs[i], NULL, 0);
        }
    }

    // delete set of semaphores
    if (semctl(semID, 0, IPC_RMID, 0) < 0) {
        printf("Couldn't close set od semaphores\n");
    }

    // delete shared memory
    if (shmctl(ordersArrID, IPC_RMID, NULL) < 0) {
        printf("Error while deleting orders array\n");
    }
}

void killHandler(int signal) {
    exit(EXIT_SUCCESS);
}