#ifndef LIBRARY_H
#define LIBRARY_H

#include <unistd.h>

#include <mqueue.h>

#include <sys/types.h>

#define MaxQueueCapacity 10
#define MaxMsgLength 1024

#define MaxClientsNumber 10
#define MaxIDStringLength 4

#define MaxNameLength 10

#define serverQueueName "/server_queue"

#define MAIN_SIG SIGUSR1
#define CHAT_SIG SIGUSR2

#define listSend -1000

#define STOP_P 20
#define DISC_P 18
#define LIST_P 16
#define CONN_P 14
#define CHAT_P 12
#define INIT_P 10

#define UNKN_P 1

#define C_STOP "S"
#define S_STOP "T"

#define C_DISC "D"
#define S_DISC "E"

#define C_LIST "L"
#define S_LIST "M"

#define C_CONN "C"
#define S_CONN "R"

#define C_CHAT "H"

#define C_INIT "I"
#define S_INIT "A"

#define UNKN "X"

#define FREE "Free"
#define BUSY "Busy"

typedef struct User {
    int companID;                       /* ID klienta               */
    char ID_string[MaxIDStringLength];  /* ID klienta jako string   */
    mqd_t queue;                        /* ID kolejki uzytkownika   */
    char queueName[MaxNameLength];      /* nazwa kolejki klienta    */
} User;

// wypisz blad i zakoncz program
void error(const char *e);      

// wygeneruj nazwe dla nowej kolejki
void generateName(char name[]);

mqd_t createQueue(const char * name);
mqd_t openQueue(const char * name);
int closeQueue(mqd_t queue);
int deleteQueue(const char * name);

char * orderType(char * order);
int checkType(char typeChar);

void usersList(char * list, User * users[]);

#endif