#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <signal.h>

#include <pthread.h>

#include <sys/time.h>

// THREADS SOMEWHERE AROUND LINE 130-140
// Assumption: threads are numerated from 1

#define UNDEFINED -1
#define SIGN 1
#define BLOCK 2
#define INTERLEAVED 3

#define MaxLine 71
#define MaxColor 256

void atExit();
void atSIGINT(int signal);
void error(const char * e);
void checkArgs(int argc, char * args[]);
int getOption(const char * arg);

void * variant_1(void * k);
void * variant_2(void * k);
void * variant_2(void * k);

FILE * fIn = NULL;
FILE * fOut = NULL;

// Assumption: M < 256 ()
int rows = 0, cols = 0, M = 0, cThreads = 0;
int ** array = NULL;
int histogram[MaxColor] = {0};

// array of indexes of threads, used by them to recognize their part of job
int * indexesArr = NULL;
// array of IDs of threads
pthread_t * threads = NULL;
// array of execution times of threads
long ** exec_times = NULL;

// main

int main(int argc, char * args[]) {
    // ---

    atexit(atExit);
    signal(SIGINT, atSIGINT);
    
    // make console output more clear
    printf("\n");

    // ---

    // handle arguments

    checkArgs(argc, args);

    cThreads = atoi(args[1]);

    int option = getOption(args[2]);

    // open proper files

    fIn = fopen(args[3], "r");
    if (fIn == NULL) {
        error("Source file not found");
    }
    fOut = fopen(args[4], "w");
    if (fOut == NULL) {
        error("Output file not opened");
    }

    // ---

    // read input

    // Assumptions about file:
    // line 0 - header "P2"
    // line 1 - doesn't exists or it's comment (#...)
    // line 2 - 'cols rows'
    // line 3 - max color
    // further - pixels

    // int foundError = 0;
    char buff[MaxLine];
    // int len = 0;

    // first line
    fgets(buff, MaxLine, fIn);
    if (strcmp(buff, "P2\n") != 0) {
        error("Header 'P2' not found\n");
    }
    
    // ignore comment line, get line 'cols rows'
    fgets(buff, MaxLine, fIn);
    if (buff[0] == '#') {
        // printf("Comment: %s\n", buff);
        fgets(buff, MaxLine, fIn);
    }
    
    // extract (rows, cols)
    if (sscanf(buff, "%i %i", &cols, &rows) < 2 || cols <= 0 || rows <= 0) {
        error("Dimensions of matrix not read correctly");
    }
    
    // get upper limit on colors
    fgets(buff, MaxLine, fIn);
    sscanf(buff, "%i", &M);
    
    // get pixels
    array = calloc(rows, sizeof(int *));
    for (int i = 0; i < rows; i++) {
        array[i] = calloc(cols, sizeof(int));
    }

    int pixels = rows * cols;
    int temp;
    for (int i = 0; i < pixels; i++) {
        if (fscanf(fIn, "%i", &temp) < 1) {
            printf("Problem at [%i, %i]\n", i / cols, i % cols);
            error("Pixel wasn't read correctly");
        } else {
            array[i / cols][i % cols] = temp;
        }
    }

    // ---

    // HERE STARTS EXECUTION OF REAL TASK

    // variables to measure time
    struct timeval stime, etime;

    // start time
    gettimeofday(&stime, NULL);

    // create threads and prepare histogram

    // array of indexes of threads, used by them to recognize their part of job
    indexesArr = calloc(cThreads, sizeof(int));
    // array of IDs of threads
    threads = calloc(cThreads, sizeof(pthread_t));
    // array of execution times of threads
    exec_times = calloc(cThreads, sizeof(long *));

    // create and execute threads
    for (int i = 0; i < cThreads; i++) {
        indexesArr[i] = i + 1;
        switch (option) {
            case SIGN:
                if (pthread_create(&threads[i], NULL, variant_1, &indexesArr[i]) != 0) {
                    error("Thread v.1 wasn't created");
                }
                break;
            case BLOCK:
                if (pthread_create(&threads[i], NULL, variant_2, &indexesArr[i]) != 0) {
                    error("Thread v.2 wasn't created");
                }
                break;
            case INTERLEAVED:
                if (pthread_create(&threads[i], NULL, variant_1, &indexesArr[i]) != 0) {
                    error("Thread v.3 wasn't created");
                }
                break;
        }
    }

    // wait for threads to end
    for (int i = 0; i < cThreads; i++) {
        pthread_join(threads[i], (void **)&exec_times[i]);
    }

    // end time
    gettimeofday(&etime, NULL);

    // total execution time in microseconds
    long totalTime = ((etime.tv_sec - stime.tv_sec) * 1000000) + (etime.tv_usec - stime.tv_usec);

    printf("Threads: %i, mode: %s\n", cThreads, args[2]);
    for (int i = 0; i < cThreads; i++) {
        printf("[%i] execution time: %fs\n", indexesArr[i], (float)(*exec_times[i]) / 1000000);
        free(exec_times[i]);
    }
    printf("Total execution time: %fs\n", (float)totalTime / 1000000);

    // ---

    // print histogram to file
    fprintf(fOut, "[%i x %i] = %i pixels\n", rows, cols, pixels);
    for (int i = 0; i < MaxColor; i++) {
        fprintf(fOut, "[%i]: %i, %f%%\n", i, histogram[i], 100 * (float)histogram[i] / pixels);
    }

    // ---

    // exit
    exit(EXIT_SUCCESS);
}

// end main

void atExit() {
    // free main array
    if (array != NULL) {
        for (int i = 0; i < rows; i++) {
            if (array[i] != NULL) free(array[i]);
        }
        free(array);
    }
    if (indexesArr != NULL) free(indexesArr);
    if (threads != NULL) free(threads);
    if (exec_times != NULL) free(exec_times);

    // close files
    if (fIn != NULL) fclose(fIn);
    if (fOut != NULL) fclose(fOut);
    
    // print farewell
    printf("\nQUIT\n\n");
}

void atSIGINT(int signal) {
    exit(EXIT_FAILURE);
}

void error(const char * e) {
    perror(e);
    exit(EXIT_FAILURE);
}

void checkArgs(int argc, char * args[]) {
    // for (int i = 0; i < argc; i++) {
    //     printf("args[%i] = \"%s\", ", i, args[i]);
    // }
    // printf("\n\n");

    if (argc != 5) {
        error("Wrong number of arguments");
    }

    if (atoi(args[1]) <= 0 || 
        !(strcasecmp(args[2], "sign") == 0 || strcasecmp(args[2], "block") == 0 || strcasecmp(args[2], "interleaved") == 0)) {
        error("Wrong arguments");
    }
}

int getOption(const char * arg) {
    if (strcasecmp(arg, "sign") == 0) {
        return SIGN;
    }
    if (strcasecmp(arg, "block") == 0) {
        return BLOCK;
    }
    if (strcasecmp(arg, "interleaved") == 0) {
        return INTERLEAVED;
    }
    return UNDEFINED; 
}

// this function search matrix by values (it writes only some specific values of pixels)
void * variant_1(void * arg) {
    struct timeval start_time, end_time;
    int k = *((int *)arg);

    // start
    gettimeofday(&start_time, NULL);

    // action
    int perThr = (MaxColor / cThreads) + (MaxColor % cThreads != 0 ? 1 : 0);
    int from = (k - 1) * perThr;
    int until = k * perThr;

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (from <= array[i][j] && array[i][j] < until) {
                histogram[ array[i][j] ]++;
            }
        }
    }

    // end
    gettimeofday(&end_time, NULL);
    long * exec_time = malloc(sizeof(long));
    *exec_time = ((end_time.tv_sec - start_time.tv_sec) * 1000000) + (end_time.tv_usec - start_time.tv_usec);

    return (void *)exec_time;
}

// this function search matrix by indexes (blocks of columns in particular)
void * variant_2(void * arg) {
    struct timeval start_time, end_time;
    int k = *((int *)arg);

    // start
    gettimeofday(&start_time, NULL);

    // action
    int perThr = (cols / cThreads) + (cols % cThreads != 0 ? 1 : 0);
    int from = (k - 1) * perThr;
    int until = k * perThr;
    // maybe until is out of bounds?
    if (until > cols) until = cols;

    for (int i = 0; i < rows; i++) {
        for (int j = from; j < until; j++) {
            histogram[ array[i][j] ]++;
        }
    }

    // end
    gettimeofday(&end_time, NULL);
    long * exec_time = malloc(sizeof(long));
    *exec_time = ((end_time.tv_sec - start_time.tv_sec) * 1000000) + (end_time.tv_usec - start_time.tv_usec);

    return (void *)exec_time;
}

// this function search matrix by indexes (columns, circular, in particular)
void * variant_3(void * arg) {
    struct timeval start_time, end_time;
    int k = *((int *)arg);

    // start
    gettimeofday(&start_time, NULL);

    // action
    for (int i = 0; i < rows; i++) {
        for (int j = (k - 1); j < cols; j += cThreads) {
            histogram[ array[i][j] ]++;
        }
    }

    // end
    gettimeofday(&end_time, NULL);
    long * exec_time = malloc(sizeof(long));
    *exec_time = ((end_time.tv_sec - start_time.tv_sec) * 1000000) + (end_time.tv_usec - start_time.tv_usec);

    return (void *)exec_time;
}