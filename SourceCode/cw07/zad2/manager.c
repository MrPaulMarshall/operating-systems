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
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/msg.h>


const char * SEM_ADDRESSES[6] = {"/SEM_ARRAY", "/SEM_TO_PACK_COUNT", "/SEM_TO_SEND_COUNT", "/SEM_TO_CREATE_INDEX", "SEM_TO_PACK_INDEX", "SEM_TO_SEND_INDEX"};

void atExit(void);
void killHandler(int signal);

// global variables

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
    sem_t * sem_set = sem_open(SEM_ADDRESSES[0], O_CREAT | O_RDWR, S_IRWXO | S_IRWXU | S_IRWXG, 1);

    if (sem_set == SEM_FAILED) {
        error("Couldn't open set of semaphores");
    } else {
        printf("Created set of semaphors\n");
    }

    // initialize semaphors to 0s
    for(int i = 1; i<6; i++){
        sem_set = sem_open(SEM_ADDRESSES[i], O_CREAT | O_RDWR, S_IRWXO | S_IRWXU | S_IRWXG, 0);
        if(sem_set == SEM_FAILED){
            error("Semaphore creation problem!");
        }
        sem_close(sem_set);
    }

    // create shared array
    shm_open(SHM_ADDR, O_CREAT | O_RDWR, S_IRWXO | S_IRWXU | S_IRWXG);

    int shm_desc = shm_open(SHM_ADDR, O_CREAT | O_RDWR, S_IRWXO | S_IRWXU | S_IRWXG);
    if (shm_desc < 0) {
        error("Didn't create orders array");
    } else {
        printf("Created array of orders\n");
    }
    ftruncate(shm_desc, sizeof(Orders));

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

    // delete set of semaphores
    for (int i = 0; i < 6; i++) {
        sem_unlink(SEM_ADDRESSES[i]);
    }

    // delete shared memory
    if (shm_unlink(SHM_ADDR) < 0) {
        printf("Error while deleting orders array\n");
    }
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
    for (int i = 0; i < 6; i++) {
        sem_unlink(SEM_ADDRESSES[i]);
    }

    // delete shared memory
    if (shm_unlink(SHM_ADDR) < 0) {
        printf("Error while deleting orders array\n");
    }
}

void killHandler(int signal) {
    exit(EXIT_SUCCESS);
}