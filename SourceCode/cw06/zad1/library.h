#ifndef LIBRARY_H
#define LIBRARY_H

#include <unistd.h>
#include <stdbool.h>

#include <sys/types.h>

#define MaxMsgLength 1024
#define MaxClientsNumber 10

#define ToServerQueueKey 191

#define INIT 1024
#define S_INIT 1024

#define STOP 1
#define S_STOP 1

#define DISCONNECT 2
#define S_DISC 2

#define LIST 4
#define S_LIST 4

#define CONNECT 8
#define S_CONN 8

#define CHAT 16

#define UNKNOWN -1

typedef struct User {
    int companID;               /* ID rozmowcy                  */
    int queue;                  /* ID kolejki uzytkownika       */
} User;

// struktura reprezentujaca komunikat zlecenia
typedef struct OrderMsg {
    long mtype;                 /* typ komunikatu           */
    key_t clientKey;            /* klucz do kolejki klienta */
    int clientID;               /* ID klienta               */
    int companID;               /* ID potencjalnego rozmowcy*/
} OrderMsg;

// struktura reprezentujaca komunikat serwera
typedef struct ServerMsg {
    long mtype;                 /* typ komunikatu - ID klienta  */
    int clientID;               /* ID przydzielonego rozmowcy   */
    char mtext[MaxMsgLength];   /* tekst komunikatu             */
} ServerMsg;

// struktura reprezuntujaca komunikat czatu
typedef struct ChatMsg {
    long mtype;                 /* typ komunikatu       */
    char mtext[MaxMsgLength];   /* tekst wiadomosci     */
} ChatMsg;

// wypisz blad i zakoncz program
void error(const char *e);      

// zwraca klucz kolejki serwera
key_t serverQueueKey();
// wygeneruj klucz dla nowej kolejki
key_t generateKey();

int createQueue(key_t key);
int getQueue(key_t key);
int deleteQueue(int queue);

int orderMsgSize();
int serverMsgSize();
int chatMsgSize();

long orderType(char * order);

void usersList(char * list, User * users[]);

#endif