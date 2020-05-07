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

void atExit(int signal);

void createOrder();
int getSem(int index);

// global variables

sem_t * sem_set[6];
int shm_desc;

// main

int main() {
    srand(time(NULL));
    signal(SIGINT, atExit);

    // open set of sem_set
    for(int i = 0; i < 6; i++){
        sem_set[i] = sem_open(SEM_ADDRESSES[i], O_RDWR);
        if(sem_set[i] < 0){
            error("Set of sempahores error");
        }
    }
    
    // open shared memory
    shm_desc = shm_open(SHM_ADDR, O_RDWR, S_IRWXO | S_IRWXU | S_IRWXG);
    if(shm_desc < 0){
        error("Shared array error");
    }

    do {        
        // if there is place for new orders
        if (getSem(SEM_TO_PACK_COUNT) + getSem(SEM_TO_SEND_COUNT) < OrdersArraySize) {
            createOrder();
        }

        // delay for other workers
        usleep( (1 + rand() % 99) * 10000 );
    } while (1);
}

// other functions

void atExit(int signal) {
    for (int i = 0; i < 6; i++) {
        sem_close(sem_set[i]);
    }
    exit(EXIT_SUCCESS);
}

int getSem(int index) {
    int result;
    sem_getvalue(sem_set[index], &result);
    return result;
}

void createOrder() {

    sem_wait(sem_set[SEM_ARRAY]);
    sem_post(sem_set[SEM_TO_PACK_COUNT]);

    Orders * orders = mmap(NULL, sizeof(Orders), PROT_READ | PROT_WRITE, MAP_SHARED, shm_desc, 0);
    if (orders == (void *)(-1)) {
        error("Shared array error\n");
    }

    int index = getSem(SEM_TO_CREATE_INDEX) % OrdersArraySize;
    int value = rand() % MaxRandomValue;

    orders->arr[index].type = TO_PACK;
    orders->arr[index].value = value;
    
    // increase index in array, where you should create new Order
    sem_post(sem_set[SEM_TO_CREATE_INDEX]);

    // print logs
    int toPack = getSem(SEM_TO_PACK_COUNT);
    int toSend = getSem(SEM_TO_SEND_COUNT);

    printMsgFromWorker("Dodalem liczbe:", value, toPack, toSend);

    // close shared memory
    munmap(orders, sizeof(Orders));

    // free array for others
    sem_post(sem_set[SEM_ARRAY]);
}