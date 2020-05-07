#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#define commandsLimit 10

// declaration of my functions

void error(char* e);

void printHelp();

void printArray(char** arr, int size);

char** splitBy(char* toSplit, char delimeters[]);

char** splitLineToCommands(char* line);

char** splitCommandToArgs(char* line);

int executeLine(char* line);

// ----
// main
// ----

int main(int argc, char* args[]) {
    // args[1] = file
    if (argc == 1) {
        printHelp();
        return 0;
    }
    if (argc > 2) {
        error("Error: wrong number of arguments");
    }

    FILE* fp = fopen(args[1], "r");
    if (fp == NULL) {
        error("Error: fopen failed");
    }

    // int countLine = 0;
    char* line = NULL;
    size_t length = 0;
    ssize_t ifRead;
    while ((ifRead = getline(&line, &length, fp)) != -1) {
        // printf("Line %i:\n%s\n", ++countLine, line);

        pid_t vPID = vfork();
        if (vPID == 0) {
            executeLine(line);
            error("Error: child process should have ended");
        } else if (vPID > 0) {
            waitpid(vPID, NULL, 0);
        } else {
            error("Error: fork inside main()");
        }

        printf("\n");
    }

    fclose(fp);
    if (line) {
        free(line);
    }

    return 0;
}

// ----
// end
// ----

// implementation of my functions

void error(char* e) {
    perror(e);
    exit(EXIT_FAILURE);
}

void printHelp() {
    printf("Proper execution of this program looks like this:\n");
    printf("-> ./main __file_with_commands__\n");
    printf("Alternative:\n");
    printf("-> make run path=__file__\n");
    printf("ALERT: your arguments in file should NOT contains ' ' char\n");
    printf("\n");
}

void printArray(char** arr, int size) {
    printf("\nArray to print:\n");
    for (int i = 0; i < size || arr[i] != NULL; i++) {
        printf("%s\n", arr[i]);
    }
    printf("\n");
}

char** splitBy(char* toSplit, char delimetersIgnore[]) {
    // printf("To split:\n");
    // printf("\"%s\"\n", toSplit);
    
    int count = 0;
    char** arr = NULL;

    char delimeters[2] = {' ', '\n'};

    // split string to array
    char* ptr = strtok(toSplit, delimeters);
    while (ptr) {
        arr = realloc(arr, (count + 1) * sizeof(char *));
        arr[count] = ptr;
        count++;
        ptr = strtok(NULL, delimeters);
    }

    // append NULL to mark end of the array
    arr = realloc(arr, (count + 1) * sizeof(char *));
    arr[count] = NULL;

    // printArray(arr, count+1);

    return arr;
}

char** splitLineToCommands(char* line) {
    return splitBy(line, "|");
}

char** splitCommandToArgs(char* command) {
    char delimeters[2] = {' ', '\n'};
    return splitBy(command, delimeters);
}

int executeLine(char* line) {
    int i;
    int pipes[2][2];
    
    char* commands[commandsLimit];
    int countCommands = 0;

    char* ptr = strtok(line, "|");
    while (ptr) {
        if (countCommands < commandsLimit) {
            commands[countCommands++] = ptr;
        }
        ptr = strtok(NULL, "|");
    }

    // Processes 0, 2, 4 ... (2n) ... read from OUT (0) of pipe 1, and write to IN (1) of pipe 0
    // Processes 1, 3, 5 ... (2n+1) ... read from OUT (0) of pipe 0, and write to IN (1) of pipe 1

    // That means, that i-th process:
    // -> reads "STDIN" (0) from OUT (0) of pipe (i+1) % 2
    // -> writes "STDOUT" (1) to IN  (1) of pipe     i % 2

    // pr_0 => pipe_0 -> pr_1 => pipe_1 -> pr_2 => ... => pipe_(K-1) -> pr_K

    for (i = 0; i < countCommands; i++) {
        
        // Close both IN and OUT of pipe which will get OUTPUT of i-th process, if was open already
        // in order to make new, empty pipe
        if (i > 0) {
            close(pipes[i % 2][0]); // OUT
            close(pipes[i % 2][1]); // IN
        }

        // Open new pipe, to store OUTPUT of i-th process, if it's not the last process
        if (pipe(pipes[i % 2]) < 0) {
            error("Error: pipe");
        }

        // char** args = splitCommandToArgs(commands[i]);


        // pid_t testPID = vfork();
        // if (testPID == 0) {
        //     execvp(args[0], args);
        // } else {
        //     wait(NULL);
        // }

        pid_t fPID = vfork();
        if (fPID == 0) {
        
            // Child process to execute i-th command
            char** args = splitCommandToArgs(commands[i]);

            // printf("I am about to execute:\n");
            // printArray(args, -1);

            // if it isn't the last command, use next pipe
            if (i + 1 < countCommands) {
                // push STDOUT to IN (1) of pipe i % 2
                close(pipes[i % 2][0]);
                if (dup2(pipes[i % 2][1], STDOUT_FILENO) < 0) {
                    error("Error: dup2");
                }
            }

            // if it isn't the first command, use previous pipe
            if (i > 0) {
                // take STDIN from OUT (0) of pipe (i+1) % 2
                close(pipes[(i+1) % 2][1]);
                if (dup2(pipes[(i+1) % 2][0], STDIN_FILENO) < 0) {
                    error("Error: dup2");
                }
            }

            if (execvp(args[0], args) < 0) {
                error("Error: execvp");
            }

            error("Error: you should not be here -- you are after exec()");
        
        } else if (fPID > 0) {
            // Parent
        } else {
            // Neither parent nor child
            error("Fork error");
        }
    }

    close(pipes[i % 2][0]);
    close(pipes[i % 2][1]);

    wait(NULL);
    exit(0);
}

