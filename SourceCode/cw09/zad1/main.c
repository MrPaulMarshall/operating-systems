#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <signal.h>

#include <pthread.h>

#include <sys/time.h>

void atExit();
void atSIGINT(int signal);
void error(const char * e);

void init();

void * barber(void * arg);
void * client(void * arg);

int afterLast();
int nextFirst();

void mySleep();
int randSleep();
int randUSleep();

// K, N
int K;
int N;

// main shared variables: waiting room
int * waitingRoom = NULL;
int first = -1;
int countWaitings = 0;
int isBarberBusy = 0;

// array of indexes of threads, used by them to recognize their part of job
int * indexesArr = NULL;
// array of IDs of threads
pthread_t * threads = NULL;
pthread_t barberThr;

pthread_mutexattr_t mutexattr;

// mutex for variable: isBarberBusy
pthread_mutex_t mutexBarber;
// mutex for tuple: {first, countWaitings}
pthread_mutex_t mutexQueue;

pthread_cond_t condIsBusy = PTHREAD_COND_INITIALIZER;

// main

int main(int argc, char * args[]) {
    // ---
    srand(time(NULL));
    atexit(atExit);
    signal(SIGINT, atSIGINT);

    // make console output more clear
    printf("\n");

    // ---

    // check args
    if (argc != 3 || atoi(args[1]) <= 0 || atoi(args[2]) <= 0) {
        error("Wrong arguments, try again");
    }

    // get K, N
    K = atoi(args[1]);
    N = atoi(args[2]);

    // ---

    // initialize waiting room

    waitingRoom = calloc(K, sizeof(int));
    for (int i = 0; i < K; i++) {
        waitingRoom[i] = 0;
    }

    // HERE STARTS EXECUTION OF REAL TASK

    // initialize mutex-es
    init();

    // array of indexes of threads, used by them to recognize which client they are
    //      they MUST start at 1, because 0 means that chair is free
    indexesArr = calloc(N, sizeof(int));
    for (int i = 0; i < N; i++) {
        indexesArr[i] = i + 1;
    }

    // array of IDs of threads
    threads = calloc(N, sizeof(pthread_t));

    // create and execute barber's thread
    if (pthread_create(&barberThr, NULL, barber, NULL) != 0) {
        error("Thread 'barber' wasn't created");
    }
    mySleep();

    // create and execute client's threads
    for (int i = 0; i < N; i++) {
        if (pthread_create(&threads[i], NULL, client, (void *)&indexesArr[i]) != 0) {
            error("Thread 'client' wasn't created");
        }
        mySleep();
    }

    // wait for threads to end
    for (int i = 0; i < N; i++) {
        pthread_join(threads[i], NULL);
    }
    pthread_join(barberThr, NULL);

    // exit
    exit(EXIT_SUCCESS);
}

// end main

void atExit() {
    // print farewell
    printf("\nTime to close all that sh*t\n\n");

    pthread_mutex_destroy(&mutexBarber);
    pthread_mutex_destroy(&mutexQueue );
    
    pthread_mutexattr_destroy(&mutexattr);
    pthread_cond_destroy(&condIsBusy);

    if (indexesArr != NULL) {
        free(indexesArr);
        indexesArr = NULL;
    }    
    if (threads != NULL) {
        free(threads);
        threads = NULL;
    }    
    if (waitingRoom != NULL) {
        free(waitingRoom);
        waitingRoom = NULL;
    }
    
    printf("QUIT SUCCESSFUL\n\n");
}

void atSIGINT(int signal) {
    exit(EXIT_FAILURE);
}

void error(const char * e) {
    perror(e);
    exit(EXIT_FAILURE);
}

// other f.

void init() {
    pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_ERRORCHECK);

    pthread_mutex_init(&mutexBarber, &mutexattr);
    pthread_mutex_init(&mutexQueue,  &mutexattr);
}



void * barber(void * arg) {
    // arg should be NULL, besides, it'll be never used
    int alreadyShaven = 0;
    isBarberBusy = 0;

    // here is barber's day

    while (alreadyShaven < N) {
        // claim isBarberBusy
        pthread_mutex_lock(&mutexBarber);

        // go to sleep
        if (countWaitings == 0) {
            printf("Golibroda: ide spac\n");
            while (countWaitings == 0) {
                pthread_cond_wait(&condIsBusy, &mutexBarber);
            }
        }

        // take client and shave him

        // claim {first, countWaitings}
        pthread_mutex_lock(&mutexQueue);

        int clientID = waitingRoom[first];
        waitingRoom[first] = 0;   
        first = nextFirst();

        printf("Golibroda: czeka %i klientow, gole klienta %i\n", countWaitings, clientID);

        isBarberBusy++;
        countWaitings--;

        alreadyShaven++;

        // zwolnij {first, countWaitings}
        pthread_mutex_unlock(&mutexQueue);
        pthread_mutex_unlock(&mutexBarber);

        // sleep - barber is shaving client
        mySleep();
        
        // decreament variable -> client leaves the room
        pthread_mutex_lock(&mutexBarber);
        isBarberBusy--;
        pthread_mutex_unlock(&mutexBarber);
    }

    printf("Golibroda: KONIEC ROBOTY NA DZIS\n");

    // end
    return NULL;
}

void * client(void * arg) {
    // arg should be pointer to client thread ID
    if (arg == NULL) error("Client received NULL ID");
    int ID = *((int *)arg);

    // here is client's try to get shaved
    int outside = 1;

    while (outside > 0) {
        // claim {isBarberBusy} 
        pthread_mutex_lock(&mutexBarber);
        
        // claim {first, countWaitings}
        pthread_mutex_lock(&mutexQueue);

        // barber is busy
        if (isBarberBusy > 0) {

            // there are free spots, sit down
            if (countWaitings < K) {
        
                waitingRoom[afterLast()] = ID;

                countWaitings++;
                outside = 0;

                printf("Poczekalnia, wolne miejsca: %i; %i\n", K - countWaitings, ID);

            }
            // waiting room is full
            else {
                printf("Zajete; %i\n", ID);
            }

        }
        // barber is sleeping
        else {
            
            // sit down and wake barber
            first = nextFirst();
            waitingRoom[first] = ID;

            countWaitings++;
            outside = 0;

            printf("Budze golibrode; %i\n", ID);
            pthread_cond_broadcast(&condIsBusy);
        
        }

        pthread_mutex_unlock(&mutexBarber);
        pthread_mutex_unlock(&mutexQueue);
    
        // wait until you can try again
        if (outside > 0) {
            mySleep();
        }
    }

    // end
    return NULL;
}

int afterLast() {
    return (first + countWaitings) % K;
}

int nextFirst() {
    return (first + 1) % K;
}

int randSleep() {
    int maxSeconds = 4;
    return (rand() % maxSeconds + 1);
}

int randUSleep() {
    // 1 s = 1 000 000 us
    int t_stamps = 10;

    return (rand() % t_stamps + 1) * (1000000 / t_stamps);
}

void mySleep() {
    // sleep(randSleep());
    usleep(randUSleep());
}