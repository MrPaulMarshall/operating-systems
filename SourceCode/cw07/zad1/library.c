#include "header.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <time.h>
#include <math.h>
#include <string.h>

#include <sys/ipc.h>
#include <sys/types.h>

// functions

void error(const char * e) {
    perror(e);
    exit(1);
}

void errorToLogs(const char * msg) {
    printMsg(msg);
    error(msg);
}

key_t getSemaforsSetKey() {
    return ftok(PathForKey, IntForSemaforsSetKey);
}

key_t getOrdersArrayKey() {
    return ftok(PathForKey, IntForOrdersArrayKey);
}

int nextIndex(int index) {
    return (index + 1) % OrdersArraySize;
}

// from StackOverflow
void prepareHead(char buffer[]) {
    strcpy(buffer, "");
    char tBuff1[HeadLength] = {0};
    char tBuff2[HeadLength] = {0};

    struct timespec curtime = {0};
    struct tm       gmtval  = {0};
    struct tm      *tmval   = NULL;
    time_t seconds;
    long   miliseconds;

    // get current time
    clock_gettime(CLOCK_REALTIME, &curtime);

    // set the variables
    seconds = curtime.tv_sec;
    miliseconds = round(curtime.tv_nsec / 1.0e6);

    // print PID and white space
    sprintf(buffer, "(%d, ", getpid());

    if ((tmval = localtime_r(&seconds, &gmtval)) != NULL) {
        // build first part of time
        strftime(tBuff1, sizeof tBuff1, "%Y-%m-%d %H:%M:%S", &gmtval);

        // add the miliseconds part and build the time string
        snprintf(tBuff2, sizeof tBuff2, "%s.%03ld", tBuff1, miliseconds);
    }

    strcat(buffer, tBuff2);
    strcat(buffer, ")");
}

void printMsg(const char * msg) {
    char head[HeadLength] = {0};
    prepareHead(head);

    FILE *fp = fopen("logs.txt", "a");
    if (fp != NULL) {
        fprintf(fp, "%s: %s\n", head, msg);
        fclose(fp);
    }
}

void printMsgFromWorker(const char * msg, int val, int toPack, int toSend) {
    char head[HeadLength] = {0};
    prepareHead(head);

    FILE *fp = fopen("logs.txt", "a");
    if (fp != NULL) {
        fprintf(fp, "%s: %s %i. Liczba zamowien do przygotowania: %i. Liczba zamowien do wyslania: %i.\n", head, msg, val, toPack, toSend);
        fclose(fp);
    }
}