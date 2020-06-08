#include "header.h"

void error(const char * e);
void checkArgs(int argc, char * args[]);
void nullUser(int index);
void initialize();
void turnOffServer();
void h_sigint(int signal);
char currentState(Game * game);
void sendKillToUser(User user);
void killUser(int index);
void at_exit();
int getUserByAddr(struct sockaddr * addr);
int getUserByName(char name[MaxMsgLength]);

int addUser(char * name, int newsockfd, struct sockaddr_in * addr_in, struct sockaddr_un * addr_un, size_t size);
int tryToSignUpNewUser(int sockfd, struct sockaddr * addr, socklen_t size, char msg[MaxMsgLength], const char * socket_type);

void destroyGame(Game * game);
void launchGame(int p1, int p2);

void * pinger(void *);
void * main_server(void *);

char * PATH;
int portno;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t thr_pinger;
pthread_t thr_connection;

int socket_inet;
struct sockaddr_in serv_addr_in;
int socket_unix;
struct sockaddr_un serv_addr_un;

User users[ServerCapacity];
int waiting = -1;

// for readfrom(), sendto()
struct sockaddr_in cli_addr_in;
struct sockaddr_un cli_addr_un;
socklen_t addr_size;
socklen_t addr_size_in = sizeof(cli_addr_in);
socklen_t addr_size_un = sizeof(cli_addr_un);

int TO_BREAK = 0;

// main - start

int main(int argc, char * args[]) {
    // at start
    signal(SIGINT, h_sigint);
    atexit(at_exit);
    checkArgs(argc, args);
    initialize();
    srand(time(NULL));
    printf("\n");

    // get args
    portno = atoi(args[1]);
    PATH = args[2];

    // CONNECT TO SOCKETS
    struct hostent * hostent = gethostbyname("localhost");
    struct in_addr host_addr = *(struct in_addr *) hostent->h_addr;
    serv_addr_in.sin_family = AF_INET;
    serv_addr_in.sin_addr.s_addr = host_addr.s_addr;
    serv_addr_in.sin_port = htons(portno);
    socket_inet = socket(AF_INET, SOCK_DGRAM, 0);
    if (bind(socket_inet, (struct sockaddr *) &serv_addr_in,
            sizeof(serv_addr_in)) < 0)
            error("Error on inet binding");
    printf("[SERVER] Inet socket has been established; ADDR: %s:%d\n", inet_ntoa(host_addr), portno);

    serv_addr_un.sun_family = AF_UNIX;
    strcpy(serv_addr_un.sun_path, PATH);
    socket_unix = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (bind(socket_unix, (struct sockaddr *) &serv_addr_un,
            sizeof(serv_addr_un)) < 0)
            error("Error on unix binding");
    // listen(socket_unix, ServerCapacity);
    printf("[SERVER] Unix socket has been established; PATH: %s\n", PATH);

    // START WORK
    if (pthread_create(&thr_pinger, NULL, pinger, NULL) != 0) error("Pinger thread wasn't created");
    if (pthread_create(&thr_connection, NULL, main_server, &portno) != 0) error("Main server thread wasn't created");

    pthread_join(thr_connection, NULL);
    pthread_join(thr_pinger, NULL);

    turnOffServer();
    printf("\n");
    exit(EXIT_SUCCESS);
}

// main - end

void error(const char * e) {
    perror(e);
    exit(EXIT_FAILURE);
}

void checkArgs(int argc, char * args[]) {
    if (argc == 1) help();
    if (argc != 3) error("Wrong number of arguments");
    if (atoi(args[1]) < 2000 || atoi(args[1]) > 65535) error("Inappropiate port");
    if (strcmp(args[2], "") == 0) error("Empty path");
}

void at_exit() {
    unlink(PATH);
}

void nullUser(int index) {
    users[index].isNull = 1;
    strcpy(users[index].plName, "");
    users[index].alias = ' ';
    users[index].coPlayer = -1;
    users[index].sockfd = -1;
    if (users[index].addr_in != NULL) {
        free(users[index].addr_in);
        users[index].addr_in = NULL;
    }
    if (users[index].addr_un != NULL) {
        free(users[index].addr_un);
        users[index].addr_un = NULL;
    }
    users[index].addr_size = 0;
    users[index].game = NULL;
    users[index].stillAlive = 0;
}

void initialize() {
    for (int i = 0; i < ServerCapacity; i++) {
        users[i].addr_in = NULL;
        users[i].addr_un = NULL;
        nullUser(i);
    }
}

void * pinger(void * arg) {    
    while (1) {
        printf("[SERVER] Time to ping the players\n");

        pthread_mutex_lock(&mutex);
        for (int i = 0; i < ServerCapacity; i++) {
            if (!(isNull(users[i]))) {
                users[i].stillAlive = 0;
                serverSendMsg(users[i].sockfd, getAddr(users[i]), users[i].addr_size, serverPing, NULL);
            }
        }
        pthread_mutex_unlock(&mutex);

        sleep(PING_TIME);

        pthread_mutex_lock(&mutex);
        for (int i = 0; i < ServerCapacity; i++) {
            if (!(isNull(users[i])) && users[i].stillAlive == 0) {
                printf("[SERVER] Player \"%s\" does not respond\n", users[i].plName);                
                killUser(i);
            }
        }
        pthread_mutex_unlock(&mutex); 

        sleep(PING_TIME);
    }

    pthread_exit(NULL);
}

void * main_server(void * arg) {
    waiting = -1;
    char msgType;
    char buffer[MaxMsgLength];
    
    struct pollfd allPolls[2];

    allPolls[0].fd = socket_inet;
    allPolls[0].events = POLLIN;
    
    allPolls[1].fd = socket_unix;
    allPolls[1].events = POLLIN;

    while (1) {
        pthread_mutex_lock(&mutex);
        for (int i = 0; i < 2; i++) {
            allPolls[i].events = POLLIN;
            allPolls[i].revents = 0;
        }
        pthread_mutex_unlock(&mutex);

        poll(allPolls, 2, -1);

        pthread_mutex_lock(&mutex);
        for (int i = 0; i < 2; i++) {            
            // SOME MESSAGE HAS COME
            if (allPolls[i].revents & POLLIN) {
                if (allPolls[i].fd == socket_unix) {
                    if (serverReadMsg(allPolls[i].fd, (struct sockaddr *) &cli_addr_un, &addr_size_un, &msgType, buffer) < 0) error("Reading from socket");
                } else if (allPolls[i].fd == socket_inet) {
                    if (serverReadMsg(allPolls[i].fd, (struct sockaddr *) &cli_addr_in, &addr_size_in, &msgType, buffer) < 0) error("Reading from socket");                    
                }

                // printf("[SERVER] Received msg: type=%c, name=\"%s\"\n", msgType, buffer);

                // ZAIMPLEMENTOWAC
                int index = getUserByName(buffer);
                if (index == -1 && msgType != msgNewName) {
                    printf("Unknown client\n");
                    continue;
                }

                if (msgType == msgNewName) {
                    int newIndex = -1;
                    if (allPolls[i].fd == socket_unix) {
                        // new player - unix
                        newIndex = tryToSignUpNewUser(allPolls[i].fd, (struct sockaddr *) &cli_addr_un, addr_size_un, buffer, "unit");
                    } else {
                        // new player - inet
                        newIndex = tryToSignUpNewUser(allPolls[i].fd, (struct sockaddr *) &cli_addr_in, addr_size_in, buffer,  "inet");
                    }
                    if (newIndex >= 0) {
                        if (waiting < 0) {
                            waiting = newIndex;
                        } else {
                            launchGame(waiting, newIndex);
                            waiting = -1;
                        }
                    }
                }
                else if (msgType == clientPing) {
                    users[index].stillAlive = 1;
                }
                else if (msgType == clientExit) {
                    killUser(index);
                }
                else if (isDigit(msgType)) {
                    Game * game = users[index].game;
                    char field = msgType;
                    game->board[ctoInt(field) - 1] = game->whoseTurn;

                    char result = currentState(game);
                    
                    // bzero(buffer, sizeof(buffer));
                    if (result == notOverYet) {
                        game->whoseTurn = enemyID(game->whoseTurn);
                        int toSend = game->whoseTurn == 'X' ? game->playerX : game->playerO;
                        
                        // sendMsg(users[toSend].sockfd, field, NULL);
                        serverSendMsg(users[toSend].sockfd, getAddr(users[toSend]), users[toSend].addr_size, field, NULL);
                    }
                    else {
                        if (result == 'X') {
                            // sendMsg(users[game->playerX].sockfd, msgVictory, NULL);
                            serverSendMsg(users[game->playerX].sockfd, getAddr(users[game->playerX]), users[game->playerX].addr_size, msgVictory, NULL);
                            // sendMsg(users[game->playerO].sockfd, msgDefeat, NULL);
                            serverSendMsg(users[game->playerO].sockfd, getAddr(users[game->playerO]), users[game->playerO].addr_size, msgDefeat, NULL);
                        }
                        else if (result == 'O') {
                            // sendMsg(users[game->playerX].sockfd, msgDefeat, NULL);
                            // sendMsg(users[game->playerO].sockfd, msgVictory, NULL);
                            // sendMsg(users[game->playerX].sockfd, msgVictory, NULL);
                            serverSendMsg(users[game->playerX].sockfd, getAddr(users[game->playerX]), users[game->playerX].addr_size, msgDefeat, NULL);
                            // sendMsg(users[game->playerO].sockfd, msgDefeat, NULL);
                            serverSendMsg(users[game->playerO].sockfd, getAddr(users[game->playerO]), users[game->playerO].addr_size, msgVictory, NULL);
                        }
                        else {
                            // sendMsg(users[game->playerX].sockfd, msgDraw, NULL);
                            // sendMsg(users[game->playerO].sockfd, msgDraw, NULL); 
                            serverSendMsg(users[game->playerX].sockfd, getAddr(users[game->playerX]), users[game->playerX].addr_size, msgDraw, NULL);
                            serverSendMsg(users[game->playerO].sockfd, getAddr(users[game->playerO]), users[game->playerX].addr_size, msgDraw, NULL);
                        }

                        printf("Game between %i and %i ended\n", game->playerX, game->playerO);
                        nullUser(game->playerX);
                        nullUser(game->playerO);
                        free(game);
                    }                    
                }
            }
        
        }
        pthread_mutex_unlock(&mutex);
    }

    pthread_exit(NULL);
}

void turnOffServer() {
    pthread_cancel(thr_pinger);
    pthread_cancel(thr_connection);

    close(socket_inet);
    close(socket_unix);
    unlink(PATH);
}

int addUser(char * name, int newsockfd, struct sockaddr_in * addr_in, struct sockaddr_un * addr_un, size_t size) {
    int index = -1;

    for (int i = 0; i < ServerCapacity; i++) {
        if (strcmp(name, users[i].plName) == 0) {
            return -1; // name is used
        }
    }

    for (int i = 0; i < ServerCapacity; i++) {
        if (isNull(users[i])) {
            index = i;
            break;
        }
    }

    if (0 <= index && index < ServerCapacity) {
        users[index].isNull = 0;
        strcpy(users[index].plName, name);
        users[index].alias = ' ';
        users[index].coPlayer = -1;
        users[index].stillAlive = 1;
        users[index].sockfd = newsockfd;

        if (addr_in != NULL) {
            users[index].addr_in = malloc(sizeof(struct sockaddr_in));
            bcopy(addr_in, users[index].addr_in, sizeof(struct sockaddr_in));
        } else {
            users[index].addr_un = malloc(sizeof(struct sockaddr_un));
            bcopy(addr_un, users[index].addr_un, sizeof(struct sockaddr_un));
        }
        users[index].addr_size = size;

        users[index].game = NULL;
    }

    return index;
}

int tryToSignUpNewUser(int sockfd, struct sockaddr * addr, socklen_t size, char name[MaxMsgLength], const char * socket_type) {
    int newIndex = -1;

    if (mUnix(socket_type)) {
        newIndex = addUser(name, sockfd, NULL, (struct sockaddr_un *)addr, size);
    }
    if (mInet(socket_type)) {
        newIndex = addUser(name, sockfd, (struct sockaddr_in *)addr, NULL, size);
    }

    if (newIndex >= 0) {
        printf("[SERVER] Player \"%s\" has been added\n", name);
        if (serverSendMsg(sockfd, addr, size, nameIsFree, NULL) < 0) error("Sending to socket");
    } else {
        printf("[SERVER] Player \"%s\" hasn't been added\n", name);
        if (serverSendMsg(sockfd, addr, size, nameIsUsed, NULL) < 0) error("Sending to socket");
    }


    return newIndex;
}

void launchGame(int p1, int p2) {
    int n1, n2;
    Game * newGame = malloc(sizeof(Game));
    for (int i = 0; i < 9; i++) newGame->board[i] = ' ';
    newGame->whoseTurn = 'X';
    if (rand() % 2 == 0) {
        newGame->playerX = p1;
        newGame->playerO = p2;
    } else {
        newGame->playerX = p2;
        newGame->playerO = p1;
    }
    users[p1].game = newGame;
    users[p2].game = newGame;

    int x = newGame->playerX;
    int o = newGame->playerO;

    n1 = serverSendMsg(users[o].sockfd, getAddr(users[o]), users[o].addr_size, 'O', newGame->board);
    n2 = serverSendMsg(users[x].sockfd, getAddr(users[x]), users[x].addr_size, 'X', newGame->board);

    if (n1 < 0 || n2 < 0) {
        printf("[SERVER] Game wasn't created\n");
        killUser(p1); // should kill both
    }
    else {
        printf("[SERVER] Created new game, for players %i and %i\n", p1, p2);
    }
}

void destroyGame(Game * game) {
    nullUser(game->playerX);
    nullUser(game->playerO);

    free(game);
}

// returns: 'X' \ 'O' if someone wins, '=' if it is a draw, or ' ' if game is still going
char currentState(Game * game) {
    if (game == NULL) return '\0';

    // game->whoseTurn hasn't been changed yet - it shows player who last made a move
    int flag;
    char ID = game->whoseTurn;
    
    // check columns
    for (int i = 0; i < 3; i++) {
        flag = 1;
        for (int j = 0; j < 3; j++) {
            if (game->board[3 * j + i] != ID) flag = 0; 
        }
        if (flag == 1) return ID;
    }
    // check rows
    for (int i = 0; i < 3; i++) {
        flag = 1;
        for (int j = 0; j < 3; j++) {
            if (game->board[3 * i + j] != ID) flag = 0;
        }
        if (flag == 1) return ID;
    }
    // check diagonal
    if (game->board[0] == ID && game->board[4] == ID && game->board[8] == ID)
        return ID;
    // check antidiagonal
    if (game->board[2] == ID && game->board[4] == ID && game->board[6] == ID) 
        return ID;

    // OK, NOONE WINS, maybe draw?
    flag = 1;
    for (int i = 0; i < 9; i++) {
        if (!(game->board[i] == 'X' || game->board[i] == 'O')) flag = 0;
    }
    if (flag == 1)
        return msgDraw;
    
    // OK, game is still going
    return notOverYet;
}

void h_sigint(int signal) {
    for (int i = 0; i < ServerCapacity; i++) {
        if (!(isNull(users[i]))) {
            killUser(i);
        }
    }

    turnOffServer();

    printf("\n");
    exit(EXIT_SUCCESS);
}

void sendKillToUser(User user) {
    // sendMsg(user.sockfd, serverExit, NULL);
    serverSendMsg(user.sockfd, getAddr(user), user.addr_size, serverExit, NULL);
    // shutdown(user.sockfd, SHUT_RDWR);
}

void killUser(int i) {
    User user = users[i];
    sendKillToUser(user);
    nullUser(i);
    printf("Player from index %i has been removed\n", i);
    if (user.game != NULL) {
        int enemy = i == user.game->playerX ? user.game->playerO : user.game->playerX;
        free(user.game);
        sendKillToUser(users[enemy]);
        nullUser(enemy);
        printf("Player from index %i has been removed\n", enemy);
    } else {
        // he waits for game
        if (waiting == i) waiting = -1;
    }
}

int getUserByName(char buffer[MaxMsgLength]) {
    for (int i = 0; i < ServerCapacity; i++) {
        if (!(isNull(users[i])) ) {
            // printf("Comparison: \"%s\"~~\"%s\"\n", users[i].plName, buffer);
            if (strcmp(users[i].plName, buffer) == 0) return i;
        }
    }

    return -1;
}