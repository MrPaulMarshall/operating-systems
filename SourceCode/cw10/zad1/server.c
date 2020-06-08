#include "header.h"

void error(const char * e);
void at_exit();
void checkArgs(int argc, char * args[]);
void nullUser(int index);
void initialize();
void turnOffServer();
void h_sigint(int signal);
char currentState(Game * game);
void sendKillToUser(User user);
void killUser(int index);

int addUser(char * name, int newsockfd);
int tryToSignUpNewUser(int sockfd);
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

int TO_BREAK = 0;

// main - start

int main(int argc, char * args[]) {
    // at start
    // system("clear");
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
    socket_inet = socket(AF_INET, SOCK_STREAM, 0);
    if (bind(socket_inet, (struct sockaddr *) &serv_addr_in,
            sizeof(serv_addr_in)) < 0)
            error("Error on inet binding");
    listen(socket_inet, ServerCapacity);
    printf("[SERVER] Inet socket has been established; ADDR: %s:%d\n", inet_ntoa(host_addr), portno);

    unlink(PATH);
    serv_addr_un.sun_family = AF_UNIX;
    strcpy(serv_addr_un.sun_path, PATH);
    socket_unix = socket(AF_UNIX, SOCK_STREAM, 0);
    if (bind(socket_unix, (struct sockaddr *) &serv_addr_un,
            sizeof(serv_addr_un)) < 0)
            error("Error on unix binding");
    listen(socket_unix, ServerCapacity);
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

void at_exit() {
    printf("\n");
}

void checkArgs(int argc, char * args[]) {
    if (argc == 1) help();
    if (argc != 3) error("Wrong number of arguments");
    if (atoi(args[1]) < 2000 || atoi(args[1]) > 65535) error("Inappropiate port");
    if (strcmp(args[2], "") == 0) error("Empty path");
}

void nullUser(int index) {
    users[index].isNull = 1;
    strcpy(users[index].plName, "");
    users[index].alias = ' ';
    users[index].coPlayer = -1;
    users[index].sockfd = -1;
    users[index].game = NULL;
    users[index].stillAlive = 0;
}

void initialize() {
    for (int i = 0; i < ServerCapacity; i++) {
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
                sendMsg(users[i].sockfd, serverPing, NULL);
            }
        }
        pthread_mutex_unlock(&mutex);

        sleep(PING_TIME);

        pthread_mutex_lock(&mutex);
        for (int i = 0; i < ServerCapacity; i++) {
            if (!(isNull(users[i])) && users[i].stillAlive == 0) {
                printf("[SERVER] Player \"%s\" does not respond\n", users[i].plName);
                shutdown(users[i].sockfd, SHUT_RDWR);
                close(users[i].sockfd);
                
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
    // int n;
    char msgType;
    char buffer[MaxMsgLength];
    
    struct pollfd allPolls[ServerCapacity + 2];

    allPolls[ServerCapacity].fd = socket_inet;
    allPolls[ServerCapacity].events = POLLIN;
    
    allPolls[ServerCapacity + 1].fd = socket_unix;
    allPolls[ServerCapacity + 1].events = POLLIN;

    while (1) {
        pthread_mutex_lock(&mutex);
        for (int i = 0; i < ServerCapacity; i++) {
            if (!(isNull(users[i]))) {
                allPolls[i].fd = users[i].sockfd;
            } else {
                allPolls[i].fd = -1;
            }
            allPolls[i].events = POLLIN;
            allPolls[i].revents = 0;
        }
        pthread_mutex_unlock(&mutex);

        allPolls[ServerCapacity].revents = 0;
        allPolls[ServerCapacity + 1].revents = 0;
        poll(allPolls, ServerCapacity + 2, -1);

        pthread_mutex_lock(&mutex);
        for (int i = 0; i < ServerCapacity + 2; i++) {
            // don't read from sockets not connected to server
            // i == ServerCapacity ~~ check if client has tried to connect to server
            if (!(isNull(users[i])) || i == ServerCapacity) {
                if (allPolls[i].revents & POLLHUP) {
                    // socket has been unconnected
                    shutdown(allPolls[i].fd, SHUT_RDWR);
                    close(allPolls[i].fd);

                    killUser(i);
                } else if (allPolls[i].revents & POLLIN) {
                    if (allPolls[i].fd == socket_unix || allPolls[i].fd == socket_inet) {
                        // new player
                        int newIndex = tryToSignUpNewUser(allPolls[i].fd);
                        if (newIndex >= 0) {
                            if (waiting < 0) {
                                waiting = newIndex;
                            } else {
                                launchGame(waiting, newIndex);
                                waiting = -1;
                            }
                        }

                    } else {
                        if (readMsg(allPolls[i].fd, &msgType, buffer) < 0) error("Reading from socket");

                        if (msgType == clientExit) {
                            killUser(i);
                        } else if (msgType == clientPing) {
                            users[i].stillAlive = 1;
                        } else if (isDigit(msgType)) {
                            Game * game = users[i].game;
                            char field = msgType;
                            game->board[ctoInt(field) - 1] = game->whoseTurn;

                            char result = currentState(game);
                            
                            bzero(buffer, sizeof(buffer));
                            if (result == notOverYet) {
                                game->whoseTurn = enemyID(game->whoseTurn);
                                int toSend = game->whoseTurn == 'X' ? game->playerX : game->playerO;
                                
                                sendMsg(users[toSend].sockfd, field, NULL);
                            }
                            else {
                                if (result == 'X') {
                                    sendMsg(users[game->playerX].sockfd, msgVictory, NULL);
                                    sendMsg(users[game->playerO].sockfd, msgDefeat, NULL);
                                }
                                else if (result == 'O') {
                                    sendMsg(users[game->playerX].sockfd, msgDefeat, NULL);
                                    sendMsg(users[game->playerO].sockfd, msgVictory, NULL);
                                }
                                else {
                                    sendMsg(users[game->playerX].sockfd, msgDraw, NULL);
                                    sendMsg(users[game->playerO].sockfd, msgDraw, NULL); 
                                }

                                printf("Game between %i and %i ended\n", game->playerX, game->playerO);
                                nullUser(game->playerX);
                                nullUser(game->playerO);
                                free(game);
                            }
                        }
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

    shutdown(socket_inet, SHUT_RDWR);
    close(socket_inet);

    shutdown(socket_unix, SHUT_RDWR);
    close(socket_unix);
    unlink(PATH);
}

int addUser(char * name, int newsockfd) {
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
        users[index].game = NULL;
    }

    return index;
}

int tryToSignUpNewUser(int sockfd) {
    int client_sockfd = accept(sockfd, NULL, NULL);
    char buffer[MaxMsgLength], name[MaxMsgLength];

    if (readMsg(client_sockfd, NULL, buffer) < 0) error("Reading from socket - signing up");
    bzero(name, sizeof(name));
    strcpy(name, buffer);

    int newIndex = addUser(name, client_sockfd);

    if (newIndex >= 0) {
        printf("[SERVER] Player \"%s\" has been added\n", name);
        sendMsg(client_sockfd, nameIsFree, NULL);
    } else {
        printf("[SERVER] Player \"%s\" hasn't been added\n", name);
        sendMsg(client_sockfd, nameIsUsed, NULL);
        shutdown(client_sockfd, SHUT_RDWR);
        close(client_sockfd);
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

    n1 = sendMsg(users[newGame->playerO].sockfd, 'O', newGame->board);
    n2 = sendMsg(users[newGame->playerX].sockfd, 'X', newGame->board);

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
    exit(EXIT_SUCCESS);
}

void sendKillToUser(User user) {
    sendMsg(user.sockfd, serverExit, NULL);
    shutdown(user.sockfd, SHUT_RDWR);
    close(user.sockfd);
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