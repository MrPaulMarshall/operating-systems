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
void sendOrder();

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
        // if there is order to send
        if (semctl(semID, SEM_TO_SEND_COUNT, GETVAL, NULL) > 0) {
            sendOrder();
        }

        // delay for other workers
        usleep( (1 + rand() % 99) * 10000 );
    } while (1);

}

// other functions

void atExit(int signal) {
    exit(EXIT_SUCCESS);
}

void sendOrder() {
    struct sembuf * opers = calloc(3, sizeof(struct sembuf));

    // first operations

    // wait until array is free
    opers[0].sem_num = SEM_ARRAY;
    opers[0].sem_op = 0;
    opers[0].sem_flg = 0;

    // claim array for yourself
    opers[1].sem_num = SEM_ARRAY;
    opers[1].sem_op = 1;
    opers[1].sem_flg = 0;

    // decrease number of orders to send
    opers[2].sem_num = SEM_TO_SEND_COUNT;
    opers[2].sem_op = -1;
    opers[2].sem_flg = 0;

    semop(semID, opers, 3);

    // open shared memory
    Orders * orders = shmat(ordersArrID, NULL, 0);

    // get the order to send
    int index = semctl(semID, SEM_TO_SEND_INDEX, GETVAL, NULL);
    int value = orders->arr[index].value * 3;

    // delete order
    orders->arr[index].type = TYPE_NULL;
    orders->arr[index].value = 0;

    // increase index of first order to pack
    union semun arg;
    arg.val = nextIndex(index);
    semctl(semID, SEM_TO_SEND_INDEX, SETVAL, arg);

    // print logs
    int toPack = semctl(semID, SEM_TO_PACK_COUNT, GETVAL, NULL);
    int toSend = semctl(semID, SEM_TO_SEND_COUNT, GETVAL, NULL);

    printMsgFromWorker("Wyslalem zamowienie o wartosci", value, toPack, toSend);

    // close shared memory
    shmdt(orders);

    // final operations
    opers = calloc(1, sizeof(struct sembuf));
    
    opers[0].sem_num = SEM_ARRAY;
    opers[0].sem_op = -1;
    opers[0].sem_flg = 0;

    semop(semID, opers, 1);
}