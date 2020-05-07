#ifndef HEADER_H
#define HEADER_H

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

// arguments to create keys
#define PathForKey "./header.h"
#define IntForSemaforsSetKey 191
#define IntForOrdersArrayKey 131

// size of array
#define OrdersArraySize 12

// semaphors

// wheter array is free
#define SEM_ARRAY 0

// how many orders to pack
#define SEM_TO_PACK_COUNT 1
// how many orders to send
#define SEM_TO_SEND_COUNT 2

// where to create next order
#define SEM_TO_CREATE_INDEX 3
// which order pack next
#define SEM_TO_PACK_INDEX 4
// which order send next
#define SEM_TO_SEND_INDEX 5

// number of workers to create
#define Workers1Num 4
#define Workers2Num 4
#define Workers3Num 4

#define AllWorkNum (Workers1Num + Workers2Num + Workers3Num)

// types of orders
#define TYPE_NULL 0
#define TO_PACK 1
#define TO_SEND 2

// other usefull values
#define MaxRandomValue 1000
#define HeadLength 40

// from Internet
union semun {
    int              val;    /* Value for SETVAL */
    struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
    unsigned short  *array;  /* Array for GETALL, SETALL */
    struct seminfo  *__buf;  /* Buffer for IPC_INFO
                                (Linux-specific) */
};

typedef struct Order {
    int value;              /* number representing... something */
    int type;               /* what to do with order            */
} Order;

typedef struct Orders {
    Order arr[OrdersArraySize];
} Orders;

void error(const char * e);
void errorToLogs(const char * msg);

key_t getSemaforsSetKey();

key_t getOrdersArrayKey();

int nextIndex(int index);

// from StackOverflow
void prepareHead(char buffer[]);

void printMsg(const char * msg);

void printMsgFromWorker(const char * msg, int val, int toPack, int toSend);

#endif