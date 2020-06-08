#ifndef HEADER_H
#define HEADER_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <netdb.h> 
#include <sys/types.h>
#include <poll.h>
#include <arpa/inet.h>
#include <endian.h>

// symbols 'X' and 'O' are reserved as players Symbols
#define nameIsUsed 'Z'
#define nameIsFree 'L'

#define serverPing 'S'
#define clientPing 'C'

#define serverExit 'T'
#define clientExit 'E'

#define msgVictory 'V'
#define msgDefeat  'D'
#define msgDraw    '='
#define notOverYet '-'

#define MaxMsgLength 20
#define ServerCapacity 12

#define PING_TIME 4

// boolean 'functions'

#define mUnix(a) strcmp(a, "unix") == 0
#define mInet(a) strcmp(a, "inet") == 0
#define ifInUse(a) a == nameIsUsed
#define isID(a) (a == 'X' || a == 'O')
//#define isDigit(a) (a == '1' || a == '2' || a == '3' || a == '4' || a == '5' || a == '6' || a == '7' || a = '8' || a == '9')
#define isDigit(a) ((int)a >= 49 && (int)a <= 57)
#define ctoInt(c) ((int)c - 48)
#define enemyID(c) c == 'X' ? 'O' : 'X'
#define gameResult(c) (c == msgVictory || c == msgDefeat || c == msgDraw)

#define isNull(u) u.isNull == 1

// structs

typedef struct Game {
    char board[9];
    char whoseTurn;         /* player X starts */
    int playerX;
    int playerO;
} Game;

typedef struct User {
    char plName[MaxMsgLength];      /* name         */
    char alias;                     /* 'X' or 'O'   */
    int coPlayer;                   /* array' index */
    int sockfd;                     /* socket fd    */
    int stillAlive;                 /* for pings    */
    
    int isNull;                     /* if null user */

    Game * game;                    /* current game */
} User;


void help() {
    printf("HELP:\n\n");
    printf("make Server P=... S=...\n");
    printf("P - port for inet, S - path for unix\n\n");
    printf("make Client N=... M=... A=... P=...\n");
    printf("N - player name, M - mode (unix/inet), A - addres (inet->machine, unix->path), P - port (only for inet)\n\n");
    printf("You should open server first\n\n");
    exit(EXIT_SUCCESS);
}

int sendMsg(int sockfd, char type, char * board) {
    char buffer[MaxMsgLength];
    bzero(buffer, sizeof(buffer));

    // it's every other message
    if (board == NULL) {
        buffer[0] = type;
    } else {
        // it's name
        if (type == '\0') strcpy(buffer, board);
        // it's board
        else {
            buffer[0] = type;
            for (int i = 1; i < 10; i++) buffer[i] = board[i - 1];
        }
    }

    return write(sockfd, buffer, MaxMsgLength);
}


int readMsg(int sockfd, char * type, char buffer[MaxMsgLength]) {
    bzero(buffer, MaxMsgLength);
    int n = read(sockfd, buffer, MaxMsgLength);
    if (n >= 0 && type != NULL) {
        *type = buffer[0];
    }
    return n;
}

#endif
