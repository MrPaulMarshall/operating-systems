#include "header.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>

void atExit(int signal);
void packOrder();

// global variables

int semID = -1;
int ordersArrID = -1;

// main

int main() {
    srand(time(NULL));
    signal(SIGINT, atExit);

    // open set of semaphores
    semID = semget(getSemaforsSetKey(), 0, 0);
    
    // open shared memory
    ordersArrID = shmget(getOrdersArrayKey(), 0, 0);

    do {        
        // if there is order to pack
        if (semctl(semID, SEM_TO_PACK_COUNT, GETVAL, NULL) > 0) {
            packOrder();
        }

        // delay for other workers
        usleep( (1 + rand() % 99) * 10000 );
    } while (1);
}

// other functions

void atExit(int signal) {
    exit(EXIT_SUCCESS);
}

void packOrder() {
    struct sembuf * opers = calloc(4, sizeof(struct sembuf));

    // first operations

    // wait until array is free
    opers[0].sem_num = SEM_ARRAY;
    opers[0].sem_op = 0;
    opers[0].sem_flg = 0;

    // claim array for yourself
    opers[1].sem_num = SEM_ARRAY;
    opers[1].sem_op = 1;
    opers[1].sem_flg = 0;

    // decrease number of orders to pack
    opers[2].sem_num = SEM_TO_PACK_COUNT;
    opers[2].sem_op = -1;
    opers[2].sem_flg = 0;

    // increase number of orders to send
    opers[3].sem_num = SEM_TO_SEND_COUNT;
    opers[3].sem_op = 1;
    opers[3].sem_flg = 0;

    semop(semID, opers, 4);

    // open shared memory
    Orders * orders = shmat(ordersArrID, NULL, 0);

    int index = semctl(semID, SEM_TO_PACK_INDEX, GETVAL, NULL);
    orders->arr[index].type = TO_SEND;
    orders->arr[index].value *= 2;

    // increase index of first order to pack
    union semun arg;
    arg.val = nextIndex(index);
    semctl(semID, SEM_TO_PACK_INDEX, SETVAL, arg);

    // print logs
    int value = orders->arr[index].value;
    int toPack = semctl(semID, SEM_TO_PACK_COUNT, GETVAL, NULL);
    int toSend = semctl(semID, SEM_TO_SEND_COUNT, GETVAL, NULL);

    printMsgFromWorker("Przygotowalem zamowienie o wartosci", value, toPack, toSend);

    // close shared memory
    shmdt(orders);

    // final operations
    opers = calloc(1, sizeof(struct sembuf));

    // free Array for other processes
    opers[0].sem_num = SEM_ARRAY;
    opers[0].sem_op = -1;
    opers[0].sem_flg = 0;

    semop(semID, opers, 1);
}