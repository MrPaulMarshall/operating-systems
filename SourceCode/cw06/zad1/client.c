#include "library.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <signal.h>

#include <sys/msg.h>
#include <sys/types.h>
#include <sys/ipc.h>

void exitClient();

void handleServerMsg(ServerMsg serverMsg);
void handleChat(char line[]);
void handleSTDIN(char line[]);

void createAndOpenQueues(key_t key);
void raportStopToServer(int signal);
void quitChat();

int clientID = -1;
int clientQueue = -1;
int serverQueue = -1;
int companQueue = -1;

// main

int main() {
    signal(SIGINT, raportStopToServer);
    atexit(exitClient);

    key_t key = generateKey();
    
    createAndOpenQueues(key);

    OrderMsg order;
    ServerMsg serverMsg;

    // send key to client queue to server
    order.mtype = INIT;
    order.companID = clientQueue;
    order.clientKey = key;

    if (msgsnd(serverQueue, &order, orderMsgSize(), 0) < 0) {
        deleteQueue(clientQueue);
        error("INIT send failed");
    }

    // wait for response with Client ID
    while (true) {
        if (msgrcv(clientQueue, &serverMsg, serverMsgSize(), S_INIT, IPC_NOWAIT) >= 0) {
            clientID = serverMsg.clientID;
            printf("Got response for INIT: ClientID = %i\n", clientID);
            break;
        }
    }

    // proceed

    char line[MaxMsgLength];

    while (true) {                
        if (msgrcv(clientQueue, &serverMsg, serverMsgSize(), S_STOP, IPC_NOWAIT) >= 0) {
            handleServerMsg(serverMsg);
        } else if (msgrcv(clientQueue, &serverMsg, serverMsgSize(), S_DISC, IPC_NOWAIT) >= 0) {
            handleServerMsg(serverMsg);
        } else if (msgrcv(clientQueue, &serverMsg, serverMsgSize(), S_LIST, IPC_NOWAIT) >= 0) {
            handleServerMsg(serverMsg);
        } else if (msgrcv(clientQueue, &serverMsg, serverMsgSize(), S_CONN, IPC_NOWAIT) >= 0) {
            handleServerMsg(serverMsg);
        }
        
        // check if chat or not
        if (companQueue > 0) {
            handleChat(line);
        } else {
            handleSTDIN(line);
        }

        sleep(1);
    }

    raportStopToServer(0);
}

// main end

// implementations

void createAndOpenQueues(key_t key) {
    // create client queue
    if ((clientQueue = createQueue(key)) == -1) {
        error("Nie utworzono kolejki klienta\n");
    } else {
        // printf("clientQ: %i\n", clientQueue);
    }

    // open server queue
    serverQueue = getQueue(serverQueueKey());
    if (serverQueue == -1) {
        error("Nieotwarta kolejka serwera");
    }
}

void handleServerMsg(ServerMsg serverMsg) {
    if (serverMsg.mtype == S_STOP) {
        raportStopToServer(0);
    } else if (serverMsg.mtype == S_DISC && companQueue > 0) {
        companQueue = -1;
    } else if (serverMsg.mtype == S_CONN && serverMsg.clientID > 0 && companQueue <= 0) {
        printf("Server :> You enter chat with other Client. You can quit writing \"exit\"\n");
        companQueue = serverMsg.clientID;
    } else if (serverMsg.mtype == S_LIST) {
        printf("Server :> Lista aktywnych klientow\n%s\n", serverMsg.mtext);
    }
}

void handleChat(char line[]) {
    ChatMsg chatMsg;

    if (msgrcv(clientQueue, &chatMsg, chatMsgSize(), CHAT, IPC_NOWAIT) >= 0) {
        // his message already contains '\n'
        printf("Chat: him :> %s", chatMsg.mtext);
        return;
    }

    printf("Chat: you :> ");
    fgets(line, MaxMsgLength, stdin);
    if (strcmp(line, "") == 0 || strcmp(line, "\n") == 0) {
        return;
    }
    if (strcmp(line, "exit") == 0 || strcmp(line, "exit\n") == 0) {
        quitChat();
        return;
    }
    chatMsg.mtype = CHAT;
    strcpy(chatMsg.mtext, line);
    msgsnd(companQueue, &chatMsg, chatMsgSize(), 0);
}

void handleSTDIN(char line[]) {
    OrderMsg order;

    printf("Client %i :> ", clientID);
    fgets(line, MaxMsgLength, stdin);

    if (strcmp(line, "") == 0 || strcmp(line, "\n") == 0) {
        return;
    }

    char * rest = strtok(line, " \n");
    long type = orderType(line);
    switch (type) {
        case STOP:
            raportStopToServer(0);
            break;
        case LIST:
            order.mtype = LIST;
            order.clientID = clientID;
            msgsnd(serverQueue, &order, orderMsgSize(), 0);
            break;
        case CONNECT:
            if (rest == NULL) {
                break;
            }
            rest = strtok(NULL, " \n");
            
            int maybeCompanID = atoi(rest);
            // printf("\"Connect\": arg=\"%s\", companID=%i\n", rest, maybeCompanID);

            if (0 < maybeCompanID && maybeCompanID <= MaxClientsNumber) {
                if (clientID == maybeCompanID) { return; }

                printf("Attempt to connect to Client %i\n", maybeCompanID);

                order.mtype = CONNECT;
                order.clientID = clientID;
                order.companID = maybeCompanID;
                if (msgsnd(serverQueue, &order, orderMsgSize(), 0) < 0) {
                    printf("Attempt to send to Server \"Connect\" order failed\n");
                }
            }
            break;
        default:
            printf("main: %s\n", line);
            printf("arg:  %s\n", rest);
            break;
    }
}

// ---

void exitClient() {
    deleteQueue(clientQueue);
    printf("Kolejka zamknieta, klient konczy prace\n");
}

void raportStopToServer(int signal) {
    quitChat();

    OrderMsg order;
    order.mtype = STOP;
    order.clientID = clientID;
    msgsnd(serverQueue, &order, orderMsgSize(), 0);

    exit(0);
}

void quitChat() {
    if (companQueue > 0) {
        OrderMsg order;
        order.mtype = DISCONNECT;
        order.clientID = clientID;
        msgsnd(serverQueue, &order, orderMsgSize(), 0);

        printf("Koniec chatu z klientem %i\n", companQueue);
        companQueue = -1;
    }
}