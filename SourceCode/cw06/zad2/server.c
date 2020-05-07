#include "library.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include <mqueue.h>

#include <sys/stat.h>
#include <sys/types.h>

void turnOffServer(int signal);
void forceExit(int signal);

void beforeExit();

void handleOrder(int signal);

void deleteUser(int clientID);
void disconnectUsers(int clientID);
void sendList(int clientID);
void connectUsers(int clientID, int companID);
void addUser(char clientName[]);

mqd_t serverQueue = -1;

User * users [MaxClientsNumber + 1];
int countUsers = 0;

// main

int main() {
    signal(MAIN_SIG, handleOrder);

    // for Ctrl+C
    signal(SIGINT, turnOffServer);
    // for Ctrl+Z
    signal(SIGTSTP, forceExit);

    atexit(beforeExit);

    for (int i = 0; i <= MaxClientsNumber; i++) {
        users[i] = NULL;
    }

    serverQueue = createQueue(serverQueueName);
    if (serverQueue == -1) {
        error("Nieotwarta kolejka serwera");
    } else {
        printf("Kolejka serwera: ID = %i\n", serverQueue);
    }

    // sign up for notification
    struct sigevent notification;
    notification.sigev_notify = SIGEV_SIGNAL;
    notification.sigev_signo = MAIN_SIG;
    mq_notify(serverQueue, &notification);

    // printf
    printf("serverQ: %i\n", serverQueue);

    // server works
    while (1) {}

    turnOffServer(0);
}

// main end

// implementations

void handleOrder(int signal) {
    // proceed msg from Client
    // printf("Server tries to read and handle msg\n");

    char msg[MaxMsgLength];
    unsigned int prio;
    ssize_t len = mq_receive(serverQueue, msg, MaxMsgLength, &prio);
    
    // printf("Server got msg \"%s\"\n", msg);

    if (len > 0 && checkType(msg[0]) > 0) {

        // handle msg from Client
        char * ptr = strtok(msg, ":");

        if (strcmp(ptr, C_STOP) == 0) {
            printf("Got STOP\n");

            int clientID;
            if ((ptr = strtok(NULL, ":\n")) != NULL) {
                if (0 < (clientID = atoi(ptr)) && clientID <= MaxClientsNumber) {
                    deleteUser(clientID);
                }
            }
        } else if (strcmp(ptr, C_DISC) == 0) {
            printf("Got DISCONNECT\n");

            int clientID;
            if ((ptr = strtok(NULL, ":\n")) != NULL) {
                if (0 < (clientID = atoi(ptr)) && clientID <= MaxClientsNumber) {
                    disconnectUsers(clientID);
                }
            }
        } else if (strcmp(ptr, C_LIST) == 0) {
            printf("Got LIST\n");

            int clientID;
            if ((ptr = strtok(NULL, ":\n")) != NULL) {
                if (0 < (clientID = atoi(ptr)) && clientID <= MaxClientsNumber) {
                    sendList(clientID);
                }
            }
        } else if (strcmp(ptr, C_CONN) == 0) {
            printf("Got CONNECT\n");

            int clientID, companID;
            if ((ptr = strtok(NULL, ":\n")) != NULL) {
                clientID = atoi(ptr);
                if ((ptr = strtok(NULL, ":\n")) != NULL) {
                    companID = atoi(ptr);
                    if (0 < clientID && clientID <= MaxClientsNumber &&
                        0 < companID && companID <= MaxClientsNumber) {
                            connectUsers(clientID, companID);
                    }
                }
            }
        } else if (strcmp(ptr, C_INIT) == 0) {
            printf("Got INIT");

            if ((ptr = strtok(NULL, ":\n")) != NULL) {
                char clientName[MaxNameLength];
                strcpy(clientName, ptr);

                printf(" with name %s\n", clientName);
                addUser(clientName);
            } else {
                printf(" without name\n");
            }
        }
    }

    // sign up again
    struct sigevent notification;
    notification.sigev_notify = SIGEV_SIGNAL;
    notification.sigev_signo = MAIN_SIG;

    mq_notify(serverQueue, &notification);
}

void informClientThatHisCompanQuited(int clientID) {
    if (0 < clientID && clientID <= MaxClientsNumber && users[clientID] != NULL && users[clientID]->companID > 0) {
        char msg[MaxMsgLength];
        strcpy(msg, S_DISC);
        strcat(msg, ":");
        strcat(msg, "test");

        if (mq_send(users[clientID]->queue, msg, strlen(msg) + 1, DISC_P) < 0) {
            printf("Client wasn't informed\n");
        } else printf("%s send to %i\n", msg, clientID);

        users[clientID]->companID = -1;
    }
}

void deleteUser(int clientID) {
    if (users[clientID] != NULL) {
        informClientThatHisCompanQuited(users[clientID]->companID);
        
        free(users[clientID]);
        users[clientID] = NULL;
        countUsers--;
    }
}

void disconnectUsers(int clientID) {
    if (users[clientID] != NULL) {
        informClientThatHisCompanQuited(users[clientID]->companID);
        users[clientID]->companID = -1;
    }
}

void sendList(int clientID) {
    if (users[clientID] == NULL) { return; }

    char msg[MaxMsgLength];
    strcpy(msg, S_LIST);
    strcat(msg, ":");

    char list[MaxMsgLength];
    usersList(list, users);
    strcat(msg, list);

    if (mq_send(users[clientID]->queue, msg, strlen(msg) + 1, LIST_P) < 0) {
        printf("List not send to client %i\n", clientID);
    }
}

void connectUsers(int clientID, int companID) {

    printf("Connect: clientID = %i\n", clientID);
    printf("Connect: companID = %i\n", companID);

    if (users[clientID] != NULL && users[companID] != NULL) {
        printf("Connect: users[%i].companID = %i\n", clientID, users[clientID]->companID);
        printf("Connect: users[%i].companID = %i\n", companID, users[companID]->companID);
    }

    if (users[clientID] != NULL && users[companID] != NULL && users[clientID]->companID <= 0 && users[companID]->companID <= 0) {
        // wysyla do kazdego numer kolejki rozmowcy (i ustawia to w users[])
        printf("Connect: attempt to connect Clients\n");
        char msg[MaxMsgLength];

        strcpy(msg, S_CONN);
        strcat(msg, ":");
        strcat(msg, users[companID]->queueName);
        if (mq_send(users[clientID]->queue, msg, strlen(msg) + 1, CONN_P) >= 0) {
            users[clientID]->companID = companID;
        } else {
            printf("Attempt failed for ClientID\n");
            return;
        }

        strcpy(msg, S_CONN);
        strcat(msg, ":");
        strcat(msg, users[clientID]->queueName);
        if (mq_send(users[companID]->queue, msg, strlen(msg) + 1, CONN_P) >= 0) {
            users[companID]->companID = clientID;
        } else {
            printf("Attempt failed for CompanID\n");
            informClientThatHisCompanQuited(clientID);
            return;
        }
    }
}

void addUser(char clientName[]) {
    int i = 1;
    while (i <= MaxClientsNumber && users[i] != NULL) { i++; }
    if (i <= MaxClientsNumber) {
        mqd_t newQueue = openQueue(clientName);
        if (newQueue < 0) {
            printf("Nie otworzono kolejki klienta o nazwie %s\n", clientName);
            return;
        }

        User * newUser = malloc(sizeof(User));
        newUser->queue = newQueue;
        strcpy(newUser->queueName, clientName);
        newUser->companID = -1;
        sprintf(newUser->ID_string, "%i", i);

        char msg[MaxMsgLength];
        strcpy(msg, S_INIT);
        strcat(msg, ":");
        strcat(msg, newUser->ID_string);

        if (mq_send(newQueue, msg, strlen(msg) + 1, INIT_P) >= 0) {
            users[i] = newUser;
            countUsers++;
            
            printf("Serwer przyjal klienta %i o kolejce %s\n", i, clientName);
        } else {
            free(newUser);
            printf("Serwer nie przyjal klienta %i o kolejce %s\n", i, clientName);
        }
    }
}

void turnOffServer(int signal) {
    if (countUsers > 0) {
        char msg[MaxClientsNumber];

        for (int i = 1; i <= MaxClientsNumber; i++) {
            if (users[i] != NULL) {
                countUsers--;

                strcpy(msg, S_STOP);
                if (mq_send(users[i]->queue, msg, strlen(msg) + 1, STOP_P) < 0) {
                    printf("Error while trying to close client %i\n", i);
                }
                closeQueue(users[i]->queue);
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
            closeQueue(users[i]->queue);
            free(users[i]);
            countUsers--;
        }
    }

    if (serverQueue >= 0) {
        closeQueue(serverQueue);
        deleteQueue(serverQueueName);
    }

    printf("Kolejka serwera usunieta, tablica wyczyszczona, wylaczam program\n");
}