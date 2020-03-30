#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

const int upperBound = 100;
const int lowerBound = -100;

void printHelp();
void executeTests();
void createFiles(int argc, char* args[]);
int compareFiles(const char* f1, const char* f2);
void executeTest(const char* A, const char* B, const char* C, const char* processes, const char* maxTime, const char* D);

int main(int argc, char* args[]) {
    if (argc == 1) {
        printHelp();
    } else if (strcmp(args[1], "test") == 0) {
        // executeTests();
        executeTest("A8.txt", "B8.txt", "C8.txt", "2", "10", "D8.txt");
        
        // printf("%i\n", compareFiles("./matrices/C8.txt", "./matrices/D8.txt"));
    } else if (strcmp(args[1], "create") == 0) {
        createFiles(argc, args);
    } else {
        printf("Wrong arguments: program did nothing\n");
        return 1;
    }

    return 0;
}

// -----------------

void printHelp() {
    printf("This auxiliary program can create some files with matrices or test correctness of your multiplication program\n\n");
    
    printf("-> If you want to create matrices:\n");
    printf("-> -> ./auxiliary create _number_of_files _Min _Max\n");
    printf("It will create _number_of_files pairs of matrices of random sizes in range [_Min, _Max]\n\n");

    printf("-> If you want to test correctness:\n");
    printf("-> -> ./auxiliary test");
}

// -----------------


// count number of endline characters -- assumption: last line also ends with endline character
int countRows(const char* file) {
    FILE* fp = fopen(file, "r");
    if (fp == NULL) {
        return 0;
    }
    int size = 0;
    char chr;

    chr = getc(fp);
    while (chr != EOF) {
        if (chr == '\n') {
            size++;
        }
        chr = getc(fp);
    }

    fclose(fp);
    return size;
}


int compareFiles(const char* f1, const char* f2) {
    int value = 1;

    printf("-------------------\n");
    
    char* command = (char *)calloc(strlen(f1) + 10, sizeof(char));
    strcpy(command, "cat ");
    strcat(command, f1);

    system(command);
    free(command);

    printf("\n");

    command = (char *)calloc(strlen(f2) + 10, sizeof(char));
    strcpy(command, "cat ");
    strcat(command, f2);

    system(command);
    free(command);

    printf("-----------------\n");

    FILE* fp1 = fopen(f1, "r");
    if (fp1 == NULL) {
        return 0;
    }
    FILE* fp2 = fopen(f2, "r");
    if (fp2 == NULL) {
        return 0;
    }

    size_t maxCh = 10000;
    char line1[10000];
    char* bL1 = line1;
    char line2[10000];
    char* bL2 = line2;
    
    // int maxEl = 2000;
    int buff1[2000];
    int size1;
    int buff2[2000];
    int size2;

    int countLines1 = 0, countLines2 = 0;

    countLines1 = countRows(f1);
    countLines2 = countRows(f2);

    int j = 0;
    while (value != 0 && j < countLines1 && j < countLines2) {
        getline(&bL1, &maxCh, fp1);
        getline(&bL2, &maxCh, fp2);
        size1 = 0;
        size2 = 0;
        char* ptr = strtok(line1, " ");
        while (ptr != NULL) {
            buff1[size1++] = atoi(ptr);
            ptr = strtok(NULL, " ");
        }
        ptr = strtok(line2, " ");
        while (ptr != NULL) {
            buff2[size2++] = atoi(ptr);
            ptr = strtok(NULL, " ");
        }


        // printf("size1 = %i; size2 = %i\n", size1, size2);
        if (size1 != size2) {
            value = 0;
        }
        for (int i = 0; i < size1; i++) {
            // printf("%i == %i\n", buff1[i], buff2[i]);
            if (buff1[i] != buff2[i])
                value = 0;
            // printf("value: %i\n", value);
        }

        j++;
    }

    fclose(fp1);
    fclose(fp2);

    return value;
}



void executeTests() {
    FILE* fLista = fopen("./matrices/lista.txt", "w");
    if (fLista == NULL) {
        printf("Nie mozna przeprowadzic testow\n");
        return;
    }

    fprintf(fLista, "A8.txt B8.txt C8.txt");
    fclose(fLista);

    system("./macierz lista.txt 2 1000 paste");

    if (compareFiles("./matrices/C8.txt", "./matrices/D8.txt") == 1) {
        printf("Test przeszedl\n\n");
    } else {
        printf("Test nie przeszedl\n\n");
    }
}

void executeTest(const char* A, const char* B, const char* C, const char* processes, const char* maxTime, const char* D) {
    FILE* fLista = fopen("./matrices/lista.txt", "w");
    if (fLista == NULL) {
        printf("Nie mozna przeprowadzic testow\n");
        return;
    }

    fprintf(fLista, "%s %s %s", A, B, C);
    fclose(fLista);

    char* command = (char *)calloc(strlen(processes) + strlen(maxTime) + 50, sizeof(char));
    strcpy(command, "./macierz lista.txt ");
    strcat(command, processes);
    strcat(command, " ");
    strcat(command, maxTime);
    strcat(command, " paste");
    system(command);
    free(command);

    char* fullC = (char *)calloc(strlen(C) + 20, sizeof(char));
    char* fullD = (char *)calloc(strlen(D) + 20, sizeof(char));
    strcpy(fullC, "./matrices/");
    strcat(fullC, C);

    strcpy(fullD, "./matrices/");
    strcat(fullD, D);

    if (compareFiles("./matrices/C8.txt", "./matrices/D8.txt") == 1) {
        printf("Test przeszedl\n\n");
    } else {
        printf("Test nie przeszedl\n\n");
    }

    free(fullC);
    free(fullD);
}

// -----------------

void rmAndTouch(char file[]) {
    char commTouch[30];
    char commRm[30];

    strcpy(commRm, "rm ");
    strcat(commRm, file);
    system(commRm);

    strcpy(commTouch, "touch ");
    strcat(commTouch, file);
    system(commTouch);
}

void generateFile(char file[], int rows, int cols) {
    rmAndTouch(file);
    FILE* fp = fopen(file, "w");
    if (fp != NULL) {
        // for each row...
        for (int j = 0; j < rows; j++) {
            // for each col...
            for (int p = 0; p < cols; p++) {
                int num = (int) (rand() / (RAND_MAX + 1.0) * (upperBound - lowerBound + 1)) + lowerBound;
                fprintf(fp, "%d ", num);
            }
            fprintf(fp, "\n");
        }
        fclose(fp);
    } else {
        printf("File %s not generated: fopen() error\n", file);
    }
}

void createFiles(int argc, char* args[]) {
    if (argc != 5) {
        printf("Cannot create files: wrong number of arguments\n");
        exit(1);
    }
    int numOfPairs = atoi(args[2]);
    int min = atoi(args[3]);
    int max = atoi(args[4]);
    if (numOfPairs <= 0 || min <= 0 || max <= 0 || min > max) {
        printf("Cannot create files: wrong arguments\n");
        exit(1);
    }

    // variables to generate names of new files
        // ZALOZENIE:
        //      numOfPairs is number with less than 10 digits
    char A[24];
    char B[24];
    char index[20];

    // variables needed to perform rand()
    time_t tt;
    int seed;

    // A[n x k] * B[k x m] --> C[n x m]
    int n, k, m;

    for (int i = 0; i < numOfPairs; i++) {
        sprintf(index, "%d", i);

        seed = time(&tt);
        srand(seed);

        n = (int) (rand() / (RAND_MAX + 1.0) * (max - min + 1)) + min;
        k = (int) (rand() / (RAND_MAX + 1.0) * (max - min + 1)) + min;
        m = (int) (rand() / (RAND_MAX + 1.0) * (max - min + 1)) + min;

        // make another A%i.txt file
        strcpy(A, "./matrices/A");
        strcat(A, index);
        strcat(A, ".txt");

        generateFile(A, n, k);

        // make another B%i.txt file

        strcpy(B, "./matrices/B");
        strcat(B, index);
        strcat(B, ".txt");

        generateFile(B, k, m);

        // sleep 1s to get another random numbers
        sleep(1);
    }

}
