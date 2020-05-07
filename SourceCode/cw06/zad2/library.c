#include "library.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <mqueue.h>

#include <sys/types.h>

void error(const char *e) {
    perror(e);
    exit(1);
}

void generateName(char name[]) {
    int PID = getpid();
    strcpy(name, "");
    if (1 <= PID && PID < 10) sprintf(name, "/00000%i", PID);
    else if (10 <= PID && PID < 100) sprintf(name, "/0000%i", PID);
    else if (100 <= PID && PID < 1000) sprintf(name, "/000%i", PID);
    else if (1000 <= PID && PID < 10000) sprintf(name, "/00%i", PID);
    else if (10000 <= PID && PID < 100000) sprintf(name, "/0%i", PID);
    else sprintf(name, "/%i", PID);
}

mqd_t createQueue(const char * name) {
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = MaxQueueCapacity;
    attr.mq_msgsize = MaxMsgLength;
    attr.mq_curmsgs = 0;

    printf("Attempt to create queue for \"%s\"\n", name);
    return mq_open(name, O_RDONLY | O_CREAT | O_EXCL, 0666, &attr);
}

mqd_t openQueue(const char * name) {
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = MaxQueueCapacity;
    attr.mq_msgsize = MaxMsgLength;
    return mq_open(name, O_WRONLY, 0666, &attr);
}

int closeQueue(mqd_t queue) {
    return mq_close(queue);
}

int deleteQueue(const char * name) {
    return mq_unlink(name);
}

char * orderType(char *order) {
    // to lowercase
    for (int i = 0; i < strlen(order); i++)
        if ('A' <= order[i] && order[i] <= 'Z')
            order[i] = (char)(order[i] + 32);

    if (strcmp(order, "stop") == 0) {
        return C_STOP;
    } else if (strcmp(order, "disconnect") == 0) {
        return C_DISC;
    } else if (strcmp(order, "list") == 0) {
        return C_LIST;
    } else if (strcmp(order, "connect") == 0) {
        return C_CONN;
    } else if (strcmp(order, "chat") == 0) {
        return C_CHAT;
    } else {
        return UNKN;
    }
}

int checkType(char typeChar) {
    char type[2];
    type[0] = typeChar;
    type[1] = '\0';

    if (strcmp(type, C_STOP) == 0 || strcmp(type, S_STOP) == 0 ||
        strcmp(type, C_DISC) == 0 || strcmp(type, S_DISC) == 0 ||
        strcmp(type, C_LIST) == 0 || strcmp(type, S_LIST) == 0 ||
        strcmp(type, C_CONN) == 0 || strcmp(type, S_CONN) == 0 ||
        strcmp(type, C_INIT) == 0 || strcmp(type, S_INIT) == 0 ||
        strcmp(type, C_CHAT) == 0) {
        return 1;
    }
    return -1;
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
            sprintf(id, "%i > ", i);
            strcat(list, id);

            char state[6];
            if (users[i]->companID == -1) {
                strcpy(state, "Free");
            } else {
                strcpy(state, "Busy");
            }
            strcat(list, state);

            count++;
        }
    }
}