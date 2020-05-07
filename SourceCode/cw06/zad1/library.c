#include "library.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/msg.h>

#include <sys/types.h>
#include <sys/ipc.h>

void error(const char *e) {
    perror(e);
    exit(1);
}

key_t serverQueueKey() {
    return ftok("./Makefile", ToServerQueueKey);
}

key_t generateKey() {
    int number = 251 * getpid() % 256;
    return ftok("./Makefile", number);
}

int createQueue(key_t key) {
    return msgget(key, IPC_CREAT | IPC_EXCL | 0666);
}

int getQueue(key_t key) {
    return msgget(key, 0666);
}

int deleteQueue(int queue) {
    return msgctl(queue, IPC_RMID, NULL);
}

int orderMsgSize() {
    return 2 * sizeof(int) + sizeof(key_t);
}

int serverMsgSize() {
    return sizeof(int) + MaxMsgLength;
}

int chatMsgSize() {
    return MaxMsgLength;
}

long orderType(char *order) {
    // to lowercase
    for (int i = 0; i < strlen(order); i++)
        if ('A' <= order[i] && order[i] <= 'Z')
            order[i] = (char)(order[i] + 32);

    if (strcmp(order, "stop") == 0) {
        return STOP;
    } else if (strcmp(order, "disconnect") == 0) {
        return DISCONNECT;
    } else if (strcmp(order, "list") == 0) {
        return LIST;
    } else if (strcmp(order, "connect") == 0) {
        return CONNECT;
    } else if (strcmp(order, "chat") == 0) {
        return CHAT;
    } else {
        return UNKNOWN;
    }
}

void usersList(char * list, User * users[]) {
    strcpy(list, "");
    int count = 0;

    for (int i = 1; i <= MaxClientsNumber; i++) {
        if (users[i] != NULL) {
            if (count > 0) {
                strcat(list, "\n");
            }
            
            char id[12];
            sprintf(id, "%i: ", i);
            strcat(list, id);

            char state[6];
            if (users[i]->companID == -1) {
                strcpy(state, "Ready");
            } else {
                strcpy(state, "Busy");
            }
            strcat(list, state);

            count++;
        }
    }
    // printf("usersList(): END = \"%s\"\n", list);
}
