#include "library.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>

#include <errno.h>

#include <sys/msg.h>
#include <sys/types.h>
#include <sys/ipc.h>

void turnOffServer(int signal);
void forceExit(int signal);

void beforeExit();

void deleteUser(OrderMsg orderMsg);
void disconnectUsers(OrderMsg orderMsg);
void sendList(OrderMsg orderMsg);
void connectUsers(OrderMsg orderMsg);
void addUser(OrderMsg orderMsg);

int serverQueue = -1;

User * users [MaxClientsNumber + 1];
int countUsers = 0;

// main

int main() {
    // for Ctrl+C
    signal(SIGINT, turnOffServer);
    // for Ctrl+Z
    signal(SIGTSTP, forceExit);

    atexit(beforeExit);

    for (int i = 0; i <= MaxClientsNumber; i++) {
        users[i] = NULL;
    }

    key_t mainKey = serverQueueKey();
    serverQueue = createQueue(mainKey);
    if (serverQueue == -1) {
        error("Nieotwarta kolejka serwera");
    } else {
        printf("Kolejka serwera: ID = %i\n", serverQueue);
    }

    OrderMsg orderMsg;

    while (true) {
        if (msgrcv(serverQueue, &orderMsg, orderMsgSize(), STOP, IPC_NOWAIT) >= 0) {
            deleteUser(orderMsg);
        }
        else if (msgrcv(serverQueue, &orderMsg, orderMsgSize(), DISCONNECT, IPC_NOWAIT) >= 0) {
            disconnectUsers(orderMsg);
        }
        else if (msgrcv(serverQueue, &orderMsg, orderMsgSize(), LIST, IPC_NOWAIT) >= 0) {
            sendList(orderMsg);
        }
        else if (msgrcv(serverQueue, &orderMsg, orderMsgSize(), CONNECT, IPC_NOWAIT) >= 0) {
            connectUsers(orderMsg);
        }
        else if (msgrcv(serverQueue, &orderMsg, orderMsgSize(), INIT, IPC_NOWAIT) >= 0) {
            addUser(orderMsg);
        }
    }

    turnOffServer(0);
}

// main end

// implementations

void informClientThatHisCompanQuited(int clientID) {
    ServerMsg serverMsg;

    if (0 < clientID && clientID <= MaxClientsNumber && users[clientID] != NULL && users[clientID]->companID > 0) {
        serverMsg.mtype = S_DISC;
        msgsnd(users[clientID]->queue, &serverMsg, serverMsgSize(), 0);

        users[clientID]->companID = -1;
    }
}

void deleteUser(OrderMsg orderMsg) {
    printf("Got STOP\n");

    if (users[orderMsg.clientID] != NULL) {
        informClientThatHisCompanQuited(users[orderMsg.clientID]->companID);
        
        free(users[orderMsg.clientID]);
        users[orderMsg.clientID] = NULL;
        countUsers--;
    }
}

void disconnectUsers(OrderMsg orderMsg) {
    printf("Got DISCONNECT\n");

    if (users[orderMsg.clientID] != NULL) {
        informClientThatHisCompanQuited(users[orderMsg.clientID]->companID);
        users[orderMsg.clientID]->companID = -1;
    }
}

void sendList(OrderMsg orderMsg) {
    printf("Got LIST\n");

    ServerMsg serverMsg;
    serverMsg.mtype = S_LIST;
    usersList(serverMsg.mtext, users);

    if (msgsnd(users[orderMsg.clientID]->queue, &serverMsg, serverMsgSize(), 0) < 0) {
        printf("List not send to client %i\n", orderMsg.clientID);
    }
}

void connectUsers(OrderMsg orderMsg) {
    printf("Got CONNECT\n");

    ServerMsg serverMsg;

    int clientID = orderMsg.clientID;
    int companID = orderMsg.companID;
    printf("Connect: %i -> %i\n", clientID, companID);

    if (users[clientID] != NULL && users[companID] != NULL && users[clientID]->companID <= 0 && users[companID]->companID <= 0) {
        // wysyla do kazdego numer kolejki rozmowcy (i ustawia to w users[])

        serverMsg.mtype = S_CONN;
        serverMsg.clientID = users[companID]->queue;
        if (msgsnd(users[clientID]->queue, &serverMsg, serverMsgSize(), 0) >= 0) {
            users[clientID]->companID = companID;
        } else {
            return;
        }

        serverMsg.mtype = S_CONN;
        serverMsg.clientID = users[clientID]->queue;
        if (msgsnd(users[companID]->queue, &serverMsg, serverMsgSize(), 0) >= 0) {
            users[companID]->companID = clientID;
        } else {
            informClientThatHisCompanQuited(clientID);
            return;
        }
    }
}

void addUser(OrderMsg orderMsg) {
    printf("Got INIT with key %i\n", orderMsg.clientKey);

    ServerMsg serverMsg;

    int i = 1;
    while (i <= MaxClientsNumber && users[i] != NULL) { i++; }
    if (i <= MaxClientsNumber) {
        int newQueue = getQueue(orderMsg.clientKey);
        if (newQueue < 0) {
            printf("Nie otworzono kolejki klienta o kluczu %i\n", orderMsg.clientKey);
            return;
        }

        serverMsg.mtype = S_INIT;
        serverMsg.clientID = i;
        if (msgsnd(newQueue, &serverMsg, serverMsgSize(), 0) >= 0) {
            users[i] = malloc(sizeof(User));
            users[i]->companID = -1;
            users[i]->queue = newQueue;
            countUsers++;
            
            printf("Serwer przyjal klienta %i o kolejce %i\n", i, newQueue);
        }
    }
}

void turnOffServer(int signal) {
    if (countUsers > 0) {
        ServerMsg serverMsg;
        OrderMsg orderMsg;

        for (int i = 1; i <= MaxClientsNumber; i++) {
            if (users[i] != NULL) {
                serverMsg.mtype = S_STOP;
                msgsnd(users[i]->queue, &serverMsg, serverMsgSize(), 0);
            }
        }

        while (countUsers > 0) {
            if (msgrcv(serverQueue, &orderMsg, orderMsgSize(), 0, IPC_NOWAIT) >= 0) {
                if (users[orderMsg.clientID] != NULL) {
                    free(users[orderMsg.clientID]);
                    users[orderMsg.clientID] = NULL;
                    countUsers--;
                }
            }
        }
    }

    exit(0);
}

void forceExit(int signal) {    
    exit(signal);
}

void beforeExit() {
    for (int i = 1; i <= MaxClientsNumber; i++) {
        if (users[i] != NULL) {
            free(users[i]);
        }
    }

    if (serverQueue >= 0) {
        deleteQueue(serverQueue);
    }

    printf("Kolejka serwera usunieta, tablica wyczyszczona, wylaczam program\n");
}