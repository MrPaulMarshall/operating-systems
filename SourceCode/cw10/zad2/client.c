#include "header.h"

void error(const char * e);
void at_exit(void);
void checkArgs(int argc, char * args[]);
void printGame(char board[9]);
void h_sigint(int signal);
void * inputGetter(void * arg);

int sockfd;
char * PATH = NULL;
char * name = NULL;
pthread_t sub_thr;

char input;
char board[9];
int inputReady = 0;
int getterThrActive = 0;

struct addrsock * addr = NULL;
socklen_t addr_size = 0;

// main - start

int main(int argc, char * args[]) {
    signal(SIGINT, h_sigint);
    atexit(at_exit);
    printf("\n");

    checkArgs(argc, args);

    // connection starts
    char playerID = ' ';
    // int n;
    int myTurn = 0;
    char msgType;
    char buffer[MaxMsgLength];

    if (mInet(args[2])) {
        struct sockaddr_in serv_addr;
        struct hostent * server;
        int portno = atoi(args[4]);

        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) error("Socket wasn't created");
        
        server = gethostbyname(args[3]);
        if (server == NULL) error("No such host");
        
        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        bcopy((char *)server->h_addr,
            (char *) &serv_addr.sin_addr.s_addr,
            server->h_length);
        serv_addr.sin_port = htons(portno);
        
        if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            error("Connection wasn't established");
        }
    }
    if (mUnix(args[2])) {
        PATH = args[3];

        struct sockaddr_un serv_addr;

        sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
        if (sockfd < 0) error("Socket wasn't created");

        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sun_family = AF_UNIX;
        strcpy(serv_addr.sun_path, args[3]);

        if (bind(sockfd, (struct sockaddr *) &serv_addr,
                sizeof(serv_addr)) < 0)
                error("Error on unix binding");

        if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            error("Connection wasn't established");
        }
    }

    // here goes game
    name = args[1];
    if (sendMsg(sockfd, msgNewName, name) < 0) error("Error writing to socket");
    
    if (readMsg(sockfd, &msgType, buffer) < 0) error("Error reading from socket");
    // printf("[From server] \"%s\"\n", buffer);

    if (msgType == nameIsFree) {
        printf("You have been signed in\n\n");

        do {
            if (myTurn == 1) {
                if (getterThrActive == 0) {
                    inputReady = 0;
                    if (pthread_create(&sub_thr, NULL, inputGetter, (void *)&playerID) < 0) error("Input getter wasn't created");
                    getterThrActive = 1;
                } else {
                    if (inputReady == 1) {
                        pthread_join(sub_thr, NULL);
                        getterThrActive = 0;

                        board[ctoInt(input) - 1] = playerID;
                        myTurn = 0;

                        printf("\n");
                        printGame(board);

                        if (sendMsg(sockfd, input, name) < 0) error("Write field to socket");
                    }
                }
            }

            // read signal
            if (readMsg(sockfd, &msgType, buffer) < 0) error("Read from socket");

            // for all circumstances
            if (msgType == serverPing) {
                if (sendMsg(sockfd, clientPing, name) < 0) error("Write ping to socket");
            }
            else if (msgType == serverExit) {
                printf("Server ordered an exit\n\n");
                break;
            }
            else if (gameResult(msgType)) {
                printf("---------------\n\n");
                printGame(board);
                printf("Game ended: ");
                if (msgType == msgVictory) printf("VICTORY!\n");
                if (msgType == msgDraw) printf("DRAW\n");
                if (msgType == msgDefeat) printf("DEFEAT\n");
                printf("\n");
                break;
            }
            // game hasn't started yet
            else if (playerID == ' ' && isID(msgType)) {
                playerID = msgType;
                if (playerID == 'X') myTurn = 1;
                for (int i = 1; i < 10; i++) {
                    if (buffer[i] == '\0') error("Incorrect board");
                    board[i - 1] = buffer[i];
                }
                printGame(board);
            }
            // in game, not my turn
            else if (isID(playerID) && myTurn == 0) {
                // enemy's move
                if (isDigit(msgType) == 1) {
                    board[ctoInt(msgType) - 1] = enemyID(playerID);
                    printGame(board);
                    myTurn = 1;
                }
            }
        } while (1);
    } else {
        printf("You haven't been singed in\n");
    }

    close(sockfd);

    exit(EXIT_SUCCESS);

    return 0;
}

// main - end

void error(const char * e) {
    perror(e);
    exit(EXIT_FAILURE);
}

void at_exit(void) {
    if (PATH != NULL) unlink(PATH);
}

void checkArgs(int argc, char * args[]) {
    if (argc == 1) help();
    if (argc < 3) error("Too few number of arguments");
    if (strlen(args[1]) > MaxMsgLength) error("Too long name");
    if (!(mInet(args[2]) || mUnix(args[2]))) error("Non-existing mode");
    if ((mInet(args[2]) && argc != 5)
        || (mUnix(args[2]) && argc != 4)) error("There is no full address");
    if (mInet(args[2]) && 
        (atoi(args[4]) < 2000 || atoi(args[4]) > 65535)) error("Inappropiate port");
    // adres serwera
}

void printGame(char board[9]) {
    printf("   |   |         |   |   \n");
    printf(" 1 | 2 | 3     %c | %c | %c \n", board[0], board[1], board[2]);
    printf("   |   |         |   |   \n");
    printf("---+---+---   ---+---+---\n");
    printf("   |   |         |   |   \n");
    printf(" 4 | 5 | 6     %c | %c | %c \n", board[3], board[4], board[5]);
    printf("   |   |         |   |   \n");
    printf("---+---+---   ---+---+---\n");
    printf("   |   |         |   |   \n");
    printf(" 7 | 8 | 9     %c | %c | %c \n", board[6], board[7], board[8]);
    printf("   |   |         |   |   \n");
    printf("\n");
}

void h_sigint(int signal) {
    printf("Received Ctrl+C\n");

    sendMsg(sockfd, clientExit, name);
    close(sockfd);

    printf("\n");
    exit(EXIT_SUCCESS);
}

void * inputGetter(void * arg) {
    char playerID = *((char *)arg);
    do {
        printf("Player %c: ", playerID);
        if (scanf("%c", &input) == 1 && isDigit(input) && !isID(board[ctoInt(input) - 1])) {
            inputReady = 1;
        } else {
            // clear stdin
            printf("Scanf unsuccessful, try again\n");
        }
        char c;
        while ((c = getchar()) != '\n' && c != EOF) {}
    } while (inputReady == 0);

    return NULL;
}