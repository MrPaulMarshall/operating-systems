#include "library.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include <mqueue.h>

#include <sys/stat.h>
#include <sys/types.h>

void exitClient();

void handleServerMsg(int signal);
void handleChatMsg(int signal);

void handleServerSTDIN(char line[]);
void handleChatSTDIN(char line[]);

void createAndOpenQueues(const char * name);
void raportStopToServer(int signal);
void quitChat();

int clientID = -1;
char ID_string[MaxIDStringLength];

char name[MaxNameLength];
mqd_t clientQueue = -1;
mqd_t serverQueue = -1;
mqd_t companQueue = -1;

// main

int main() {
    signal(SIGUSR1, handleServerMsg);
    // signal(SIGUSR2, handleChatMsg);

    signal(SIGINT, raportStopToServer);
    atexit(exitClient);
    
    generateName(name);

    createAndOpenQueues(name);

    // buffor for message
    char msg[MaxMsgLength];

    // send client queue's name to server
    strcpy(msg, C_INIT);
    strcat(msg, ":");
    strcat(msg, name);

    if (mq_send(serverQueue, msg, strlen(msg) + 1, INIT_P) < 0) {
        error("INIT order failed");
    } else

    // wait for response with Client ID
    while (clientID <= 0) {}

    // proceed

    char line[MaxMsgLength];

    while (1) {
        
        // check if chat or not
        if (companQueue > 0) {
            handleChatSTDIN(line);
        } else {
            handleServerSTDIN(line);
        }

        sleep(1);
    }

    raportStopToServer(0);
}

// main end

// implementations

void createAndOpenQueues(const char * name) {
    // create client queue
    if ((clientQueue = createQueue(name)) == -1) {
        error("Nie utworzono kolejki klienta\n");
    } else {
        printf("clientQ: %i\n", clientQueue);
    }

    struct sigevent notification;
    notification.sigev_notify = SIGEV_SIGNAL;
    notification.sigev_signo = MAIN_SIG;

    mq_notify(clientQueue, &notification);

    // open server queue
    serverQueue = openQueue(serverQueueName);
    if (serverQueue == -1) {
        error("Nieotwarta kolejka serwera");
    } else {
        printf("serverQ: %i\n", serverQueue);
    }
}

void handleServerMsg(int signal) {
    // proceed msg from Server
    char msg[MaxMsgLength];
    unsigned int prio;
    ssize_t len = mq_receive(clientQueue, msg, MaxMsgLength, &prio);

    if (len > 0 && checkType(msg[0]) > 0) {
        // handle msg from Server
        char * ptr = strtok(msg, ":");
    
        if (strcmp(ptr, S_STOP) == 0) {

            if (companQueue >= 0) {
                printf("Server :> Stop podczas czatu\n");
            }
            raportStopToServer(0);
        
        } else if (strcmp(ptr, S_DISC) == 0) {
        
            printf("\nServer :>  Disconect\n");
            companQueue = -1;
        
        } else if (strcmp(ptr, C_CHAT) == 0) {
        
            ptr = strtok(NULL, ":");
            if (ptr != NULL && companQueue >= 0) {
                printf("\n");
                printf("Chat: Him :> %s", ptr);
            }
        
        } else if (strcmp(ptr, S_LIST) == 0) {
        
            ptr = strtok(NULL, ":");
            printf("Server :> Lista klientow\n%s\n", ptr);
        
        } else if (strcmp(ptr, S_CONN) == 0) {
        
            ptr = strtok(NULL, ":\n");
            // printf("ptr = companQName = %s\n", ptr);
            mqd_t queueToConn = openQueue(ptr);
            printf("Server :> You entered chat with other Client %i. You can quit writing \"exit\"\n", queueToConn);
            companQueue = queueToConn;
        
        } else if (strcmp(ptr, S_INIT) == 0) {
        
            ptr = strtok(NULL, ":\n");
            int newID = atoi(ptr);
            if (0 < newID && newID <= MaxClientsNumber) {
                clientID = newID;
                strcpy(ID_string, "");
                sprintf(ID_string, "%i", newID);
            } else {
                error("Server has send bad ID");
            }
        
        } else {
            printf("\nProblem :> Unknown msg\n\n");
        }
    }

    // sign up again
    struct sigevent notification;
    notification.sigev_notify = SIGEV_SIGNAL;
    notification.sigev_signo = MAIN_SIG;

    mq_notify(clientQueue, &notification);
}

void handleChatSTDIN(char line[]) {
    char msg[MaxMsgLength];

    printf("Chat: you :> ");
    fgets(line, MaxMsgLength, stdin);
    if (strcmp(line, "") == 0 || strcmp(line, "\n") == 0) {
        return;
    }

    if (companQueue < 0) {
        printf("You are disconnected\n");
        return;
    }

    if (strcmp(line, "exit") == 0 || strcmp(line, "exit\n") == 0) {
        quitChat();
        sleep(1);
        return;
    }

    strcpy(msg, C_CHAT);
    strcat(msg, ":");
    strcat(msg, line);
    mq_send(companQueue, msg, strlen(msg) + 1, CHAT_P);
}

void handleServerSTDIN(char line[]) {
    char msg[MaxMsgLength];

    printf("Client %i :> ", clientID);
    fgets(line, MaxMsgLength, stdin);

    if (strcmp(line, "") == 0 || strcmp(line, "\n") == 0) {
        return;
    }

    char * rest = strtok(line, " \n");
    const char * type = orderType(line);
    
    if (strcmp(type, C_STOP) == 0) {
        raportStopToServer(0);
    } else if (strcmp(type, C_LIST) == 0) {
        strcpy(msg, C_LIST);
        strcat(msg, ":");
        strcat(msg, ID_string);
        mq_send(serverQueue, msg, strlen(msg) + 1, LIST_P);
    } else if (strcmp(type, C_CONN) == 0) {
        if (rest == NULL) {
            return;
        }
        rest = strtok(NULL, " \n");
        
        int maybeCompanID = atoi(rest);
        // printf("\"Connect\": arg=\"%s\", companID=%i\n", rest, maybeCompanID);

        if (0 < maybeCompanID && maybeCompanID <= MaxClientsNumber) {
            if (clientID == maybeCompanID) { return; }

            printf("Attempt to connect to Client %i\n", maybeCompanID);

            strcpy(msg, C_CONN);
            strcat(msg, ":");
            strcat(msg, ID_string);
            strcat(msg, ":");
            strcat(msg, rest);
            if (mq_send(serverQueue, msg, strlen(msg) + 1, CONN_P) < 0) {
                printf("Attempt to send to Server \"Connect\" order failed\n");
            }
        }
    } else {
        printf("main: %s\n", line);
        printf("arg:  %s\n", rest);
    }
}

void exitClient() {
    closeQueue(serverQueue);
    closeQueue(clientQueue);
    deleteQueue(name);

    printf("Kolejka zamknieta, klient konczy prace\n");
}

void raportStopToServer(int signal) {
    // quitChat();

    char msg[MaxMsgLength];
    strcpy(msg, C_STOP);
    strcat(msg, ":");
    strcat(msg, ID_string);

    mq_send(serverQueue, msg, strlen(msg) + 1, STOP_P);

    exit(0);
}

void quitChat() {
    if (companQueue > 0) {
        // disconnect
        char msg[MaxMsgLength];
        strcpy(msg, C_DISC);
        strcat(msg, ":");
        strcat(msg, ID_string);

        mq_send(serverQueue, msg, strlen(msg) + 1, DISC_P);

        printf("Koniec chatu z klientem %i\n", companQueue);
        companQueue = -1;
    }
}