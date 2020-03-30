#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#include <time.h>
#include <sys/wait.h>

void printHelp();
void checkArgs(int argc, char* args[]);

void loadFileList(const char* file, char A[], char B[], char C[]);
char* fullPath(const char* file);
char* temporaryFile(const char* C, const int i);

int countRows(const char* file);
int countCols(const char* file);

// --
void readRow(const char* file, int index, int* buff, int rows, int cols);
void readCol(const char* file, int index, int* buff, int rows, int cols);

int calculateElem(int* row, int* col, int len);

int executeWorker(const char* A, const char* B, const char* fileToSafe, const char* safeMode, int n, int k, int m, int firstCol, int lastCol);
int executeWorkerPaste(const char* A, const char* B, const char* fileToSafe, int n, int k, int m, int firstCol, int lastCol, int maxTime);
int executeWorkerOneFile(const char* A, const char* B, const char* fileToSafe, int n, int k, int m, int firstCol, int lastCol, int maxTime);


// ----------------------
// ASSUMPTION: files with matrices are inside dictionary "./matrices/", so arguments in _file_list are only names of files
// example of _file_list: "A2.txt B2.txt C2.txt"
// ----------------------

int main(int argc, char* args[]) {
    if (argc == 1) {
        printHelp();
        return 0;
    }

    // exit if given invalid arguments
    checkArgs(argc, args);

    //      printf("Arguments checked\n");

    // parse arguments
    char A[20], B[20], C[20];
    loadFileList(args[1], A, B, C);

    //      printf("Files: A = %s, B = %s, C = %s\n", A, B, C);

    int numberOfWorkers = atoi(args[2]);
    int maxTime = atoi(args[3]);

    // do stuff
    char* fullA = fullPath(A);
    char* fullB = fullPath(B);

    //      printf("A = %s, B = %s\n", fullA, fullB);

    int n = countRows(fullA);
    //      printf("n = %i\n", n);

    int k = countCols(fullA);
    //      printf("k = %i\n", k);

    int m = countCols(fullB);
    //      printf("m = %i\n", m);

    if (k != countRows(fullB)) {
        printf("Matrix A has to have as much columns as matrix B has rows in order ro multiply them\n");
        free(fullA);
        free(fullB);
        exit(1);
    }

    if (n == 0 || k == 0 || m == 0) {
        printf("There are problems with your files to multiply\n");
        free(fullA);
        free(fullB);
        return 1;
    }

    //      printf("n = %i, k = %i, m = %i\n", n, k, m);

    char* fullC = fullPath(C);

    // partition of m columns of matrix B into numberOfWorkers blocks to work with
    // last block can potentionally be shorter than the rest, if m % numberOfWorkers != 0
    int partLength, lastPartL;

    // this formula guarantees, that the length is calculated correctly
    partLength = (m + numberOfWorkers - 1) / numberOfWorkers;
    lastPartL = m % numberOfWorkers;
    
    // printf("length = %i, lastLength = %i\n", partLength, lastPartL);


    // if lastPartL == 0, then there are numberOfWorkers workers of length partLength
    // if lastPartL > 0, then there (numberOfWorkers - 1) workers of length partLength, and one last of length lastPartL
    
    // store PID procesu
    pid_t PID = getpid();

    // time to decide which method to use to make final file
    if (strcmp(args[4], "paste") == 0) {
        char** temporaryFiles = (char **)calloc(numberOfWorkers, sizeof(char *));

        // create names for temporary files
        for (int i = 0; i < numberOfWorkers; i++) {
            temporaryFiles[i] = temporaryFile(C, i);
        }

        // printf("Test - wybrana opcja paste, utworzono pliki\n");

        pid_t* childrenPIDs = (pid_t *)calloc(numberOfWorkers, sizeof(pid_t));
        int* results = (int *)calloc(numberOfWorkers, sizeof(int));
        pid_t workerPID;

        // create first n-1 workers
        for (int i = 0; i < numberOfWorkers - 1; i++) {
            workerPID = fork();

            if (workerPID == 0) {
                printf("Jestem w procesie PID=%i, o indeksie %i\n", getpid(), i);

                // in worker
                int exitStatus = executeWorkerPaste(fullA, fullB, temporaryFiles[i], n, k, m, i*partLength, (i+1)*partLength - 1, maxTime);
                printf("Status: %i\n", exitStatus);
                exit(exitStatus);
            } else {
                // in manager
                childrenPIDs[i] = workerPID;
            }
        }
        // and last worker
        workerPID = fork();
        if (workerPID == 0) {
            printf("Jestem w procesie PID=%i, o indeksie %i\n", getpid(), numberOfWorkers - 1);

            // in worker
            int exitStatus = executeWorkerPaste(fullA, fullB, temporaryFiles[numberOfWorkers - 1], n, k, m, (numberOfWorkers - 1)*partLength, m - 1, maxTime);
            printf("Status: %i\n", exitStatus);
            exit(exitStatus);
        } else {
            // in manager
            childrenPIDs[numberOfWorkers - 1] = workerPID;
        }

        // wait for all children -- waitpid()
        for (int i = 0; i < numberOfWorkers; i++) {
            waitpid(childrenPIDs[i], &results[i], 0);
        }

        // exec() needs to be called on the end of a proccess, so I make child proccess exactly for that task
        char** argv = (char **)calloc(3 + numberOfWorkers, sizeof(char));
        argv[0] = strdup("paste");
        argv[1] = strdup("-d ''");
        for (int i = 0; i < numberOfWorkers; i++) {
            argv[2 + i] = strdup(temporaryFiles[i]);
        }
        argv[numberOfWorkers + 2] = NULL;

        if (fork() == 0) {
            // make stdout to file C
            char* fullC = fullPath(C);
            int fd = open(fullC, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
            
            dup2(fd, 1);

            free(fullC);
            close(fd);

            // execute "paste"
            execv("/bin/paste", argv);
            exit(0);
        }
    
        // free array of temporary files
        free(childrenPIDs);
        free(results);

        for (int i = 1; i < numberOfWorkers + 3; i++) {
            // last argument is null, so i don't free it
            free(argv[i]);
        }
        free(argv);

        for (int i = 0; i < numberOfWorkers; i++) {
            free(temporaryFiles[i]);
        }
        free(temporaryFiles);
    } else if (strcmp(args[4], "flock") == 0) {
        // NA RAZIE WYLACZONY
        return 1;

        // implement later

        // prepare final file -- write as many '\n' as rows in A = n
        touch(fullC);
        FILE* fp = fopen(fullC, "w");
        if (fp == NULL) {
            printf("Final file %s not opened\n", C);
            return 1;
        }
        for (int i = 0; i < n; i++) {
            fprintf(fp, "\n");
        }
        fclose(fp);

        pid_t workerPID;
        int result;

        // create first n-1 workers
        for (int i = 0; i < numberOfWorkers - 1; i++) {
            workerPID = fork();

            if (workerPID == 0) {
                printf("Jestem w procesie PID=%i, o indeksie %i\n", getpid(), i);

                // in worker
                int exitStatus = executeWorkerOneFile(fullA, fullB, fullC, n, k, m, i*partLength, (i+1)*partLength - 1, maxTime);
                printf("Status: %i\n", exitStatus);
                exit(exitStatus);
            } else {
                // in manager
                waitpid(workerPID, &result, 0);
            }
        }
        // and last worker
        workerPID = fork();
        if (workerPID == 0) {
            printf("Jestem w procesie PID=%i, o indeksie %i\n", getpid(), numberOfWorkers - 1);

            // in worker
            int exitStatus = executeWorkerOneFile(fullA, fullB, fullC, n, k, m, (numberOfWorkers - 1)*partLength, m - 1, maxTime);
            printf("Status: %i\n", exitStatus);
            exit(exitStatus);
        } else {
            // in manager
            waitpid(workerPID, &result, 0);
        }
    }

    free(fullA);
    free(fullB);
    free(fullC);

    return 0;
}


// ------------------

void printHelp() {
    printf("Execute program like:\n");
    printf("-> ./macierz _file_list _number_of_subprocesses _max_time_for_subproccess _how_to_make_final_file\n\n");
}

void checkArgs(int argc, char* args[]) {
    if (argc != 5) {
        printf("Wrong number of arguments\n");
        exit(1);
    }

    if (strcmp(args[4], "paste") != 0 && strcmp(args[4], "flock") != 0) {
        printf("Invalid option how to make final file\n");
        exit(1);
    }

    if (atoi(args[2]) <= 0 || atoi(args[3]) <= 0) {
        printf("Invalid numerical arguments\n");
        exit(1);
    }
}

// ------------------

void loadFileList(const char* file, char A[], char B[], char C[]) {
    // file - file with list: "A.txt B.txt C.txt"
    char* path = fullPath(file);
    FILE* fp = fopen(path, "r");
    free(path);

    if (fp != NULL) {
        char line[128];
        char* ptr;
        if (fgets(line, 128, fp)) {
            ptr = strtok(line, " \n");
            strcpy(A, ptr);

            ptr = strtok(NULL, " \n");
            strcpy(B, ptr);
            
            ptr = strtok(NULL, " \n");
            strcpy(C, ptr);
        } else {
            printf("Error while loading line from %s\n", file);
            fclose(fp);
            exit(1);
        }
    } else {
        printf("Error while opening file: %s\n", file);
        exit(1);
    }

    fclose(fp);
}

char* fullPath(const char* file) {
    char* full = (char *)calloc(strlen(file) + 20, sizeof(char));
    strcpy(full, "./matrices/");
    strcat(full, file);
    
    return full;
}

char* temporaryFile(const char* C, const int i) {
    char* name = (char *)calloc(strlen(C) + 20, sizeof(char));
    strcpy(name, "./matrices/");
    strcat(name, C);
    strcat(name, "-");
    
    char buff[12];
    sprintf(buff, "%d", i);
    strcat(name, buff);

    // printf("utworzono nazwe: %s\n", name);
    return name;
}

int touch(const char* path) {
    char* command = (char *)calloc(strlen(path) + 20, sizeof(char));
    strcpy(command, "touch ");
    strcat(command, path);
    system(command);
    free(command);
    return true;
}

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

// counts number of 'words' in the 0-th line in file
int countCols(const char* file) {
    FILE* fp = fopen(file, "r");
    if (fp == NULL) {
        return 0;
    }
    int size = 0;
    char chr;
    
    chr = getc(fp);
    while (chr != EOF && chr != '\n') {
        if (chr == ' ') {
            size++;
        }
        chr = getc(fp);
    }

    fclose(fp);
    return size;
}



// ------------------------------

void readRow(const char* file, int index, int* buff, int rows, int cols) {    
    const int max_len = 8192;
    char line[max_len]; // assumption: each line is at max 1024 characters long

    FILE* fp = fopen(file, "r");
    if (fp != NULL) {
        int i = 0;
        
        // loads index-th line
        while ( fgets(line, max_len, fp) ) {
            if (i == index) {
                break;
            }
            i++;
        }

        // time to parse loaded line to array
        if (strlen(line) > 0) {
            char* ptr = strtok(line, " \n");
            int j = 0;

            while (ptr != NULL && j < cols) {
                int num = atoi(ptr);
                if (num == 0 && strcmp(ptr, "0") != 0) {
                    printf("Blad odczytu pliku %s w wierszu %i - wyraz nie bedacy liczba\n", file, index);
                }
                buff[j] = num;
                j++;

                ptr = strtok(NULL, " \n");
            }
        } else {
            printf("Blad odczytu pliku %s: pusta linia\n", file);
        }

        fclose(fp);
    } else {
        printf("Blad odczytu pliku %s\n", file);
        return;
    }
}

void readCol(const char* file, int index, int* buff, int rows, int cols) {    
    const int max_len = 8192;
    char line[max_len]; // assumption: each line is at max 1024 characters long

    FILE* fp = fopen(file, "r");
    if (fp != NULL) {
        int i = 0;

        // for each line
        while ( fgets(line, max_len, fp) ) {
            // gets index-th word from line
            char* ptr = strtok(line, " \n");
            int j = 0;

            while (ptr != NULL && j < index) {
                j++;
                ptr = strtok(NULL, " \n");
            }
        
            if (ptr == NULL || (atoi(ptr) == 0 && strcmp(ptr, "0") != 0)) {
                printf("Blad odczytu pliku %s w wierszu %i - wyraz nie bedacy liczba\n", file, index);
            }

            // printf("col[%i]: %s\n", i, ptr);
            buff[i] = atoi(ptr);
            i++;
        }

        fclose(fp);
    } else {
        printf("Blad odczytu pliku %s\n", file);
        return;
    }
}

int calculateElem(int* row, int* col, int len) {
    int sum = 0;
    // elem of matrix is scalar product of row and col
    for (int i = 0; i < len; i++) {
        sum += row[i] * col[i];
    }
    return sum;
}

int executeWorker(const char* A, const char* B, const char* fileToSafe, const char* safeMode, int n, int k, int m, int firstCol, int lastCol) {
    // do more stuff
    int elem;
    int* row = (int *)calloc(k, sizeof(int));
    int* col = (int *)calloc(k, sizeof(int));

    // here i have to decide by witch method safe to file
    if (strcmp(safeMode, "paste") == 0) {
        if (touch(fileToSafe) == false) {
            printf("Nie utworzono pliku %s\n", fileToSafe);
            return 0;
        }

        FILE* fp = fopen(fileToSafe, "w");
        
        if (fp == NULL) {
            printf("problem with file %s\n", fileToSafe);
            
            free(row);
            free(col);
            return 0;
        }

        // for each row in matrix A
        for (int i = 0; i < n; i++) {
            readRow(A, i, row, n, k);

            // for each colums in matrix B which is in interval [firstCol, lastCol]
            for (int j = firstCol; j <= lastCol; j++) {
                readCol(B, j, col, k, m);

                elem = calculateElem(row, col, k);
                fprintf(fp, "%d ", elem);
            }

            // ends line
            fprintf(fp, "\n");
        }

        fclose(fp);
    } else if (strcmp(safeMode, "flock") == 0) {
        // maybe later
        
    }

    // printf("Zwalniam row i col\n");
    free(row);
    free(col);
    // printf("Zwolnilem row i col\n");

    return 1;
}


int executeWorkerPaste(const char* A, const char* B, const char* fileToSafe, int n, int k, int m, int firstCol, int lastCol, int maxTime) {    
    if (touch(fileToSafe) == false) {
        printf("Nie utworzono pliku %s\n", fileToSafe);
        return 0;
    }

    FILE* fp = fopen(fileToSafe, "w");
    
    if (fp == NULL) {
        printf("problem with file %s\n", fileToSafe);  
        return 0;
    }

    time_t start = time(NULL);

    int elem;
    int* row = (int *)calloc(k, sizeof(int));
    int* col = (int *)calloc(k, sizeof(int));

    // for each row in matrix A
    for (int i = 0; i < n; i++) {
        readRow(A, i, row, n, k);

        // for each colums in matrix B which is in interval [firstCol, lastCol]
        for (int j = firstCol; j <= lastCol; j++) {
            readCol(B, j, col, k, m);

            elem = calculateElem(row, col, k);
            fprintf(fp, "%d ", elem);
        }

        // ends line
        fprintf(fp, "\n");

        if (maxTime < difftime(time(NULL), start)) {
            printf("Proces %i wymnozyl %i fragmentow\n", getpid(), i);
            free(row);
            free(col);

            return i;
        }
    }

    fclose(fp);

    free(row);
    free(col);

    return n;
}

int executeWorkerOneFile(const char* A, const char* B, const char* fileToSafe, int n, int k, int m, int firstCol, int lastCol, int maxTime) {
    if (touch(fileToSafe) == false) {
        printf("Nie utworzono pliku %s\n", fileToSafe);
        return 0;
    }

    printf("-----\n");
    system("cat ./matrices/C3.txt");
    printf("-----\n");

    FILE* fp = fopen(fileToSafe, "r+");

    if (fp == NULL) {
        printf("problem with file %s\n", fileToSafe);  
        return 0;
    }

    int elem;
    int* row = (int *)calloc(k, sizeof(int));
    int* col = (int *)calloc(k, sizeof(int));

    time_t start = time(NULL);

    // for each row in matrix A
    for (int i = 0; i < n; i++) {
        int position = 0;
        int countEndlines = 0;
        
        fseek(fp, 0L, 2);
        int lengthOfFile = ftell(fp);
        rewind(fp);

        char chr;
        // assumption -- there are n endline characters in file
        
        do {
            chr = getc(fp);
            if (chr == '\n')
                countEndlines++;
            if (countEndlines == i + 1)
                break;
            position++;
        } while (chr != EOF);

        /*
        while (countEndlines < i) {
            chr = getc(fp);
            position++;
            if (chr == '\n') {
                countEndlines++;
            }
        }
        */

        // printf("position = %i\n", i);

        // i need to write from "position"
        int bufforLength = lengthOfFile - position;
        char* buffor = (char *)calloc(bufforLength, sizeof(char));
        fread(buffor, sizeof(char), bufforLength, fp);

        fseek(fp, position * sizeof(char), 0);

        printf("position = %i, EOF = %i\n", position, lengthOfFile);

        readRow(A, i, row, n, k);

        // for each colums in matrix B which is in interval [firstCol, lastCol]
        for (int j = firstCol; j <= lastCol; j++) {
            readCol(B, j, col, k, m);

            elem = calculateElem(row, col, k);
            fprintf(fp, "%d ", elem);
            // char number[12];
            // sprintf(number, "%d ", elem);
            // fwrite(number, sizeof(char), strlen(number), fp);

            printf("-----\n");
            system("cat ./matrices/C3.txt");
            printf("-----\n");
        }

        // ends line
        // fprintf(fp, "\n");
    
        // write buffor again
        // printf("%s\n", buffor);
        
        printf("-----\n");
        system("cat ./matrices/C3.txt");
        printf("-----\n");

        fprintf(fp, "%s", buffor);
        free(buffor);

        if (maxTime < difftime(time(NULL), start)) {
            free(row);
            free(col);
            fclose(fp);
            return 0;
        }
    }

    fclose(fp);

    free(row);
    free(col);

    return 1;
}
