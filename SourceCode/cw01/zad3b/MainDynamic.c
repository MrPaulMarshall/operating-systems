#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <time.h>
#include <sys/times.h>
#include <unistd.h>
#include <ctype.h>
#include <dlfcn.h>

#include "mylib.h"

// #include "lab1_library_structc.h"


double executionTime(clock_t start, clock_t end) {
    return (double)(end - start) / (double)sysconf(_SC_CLK_TCK);
}

/*
FilePair* parsePair(const char* arg) {
    char* file1 = NULL;
    char* file2 = NULL;
    char* token;
    char* toParse = strdup(arg);

    token = strtok(toParse, ":");
    file1 = strdup(token);

    token = strtok(NULL, ":");
    file2 = strdup(token);

    if(file1 == NULL || file2 == NULL) {
        printf("Niewlasciwy input - f1: %s, f2: %s\n", file1, file2);
        return NULL;
    }

    FilePair* pair = createFilePair(file1, file2);

    free(file1);
    free(file2);

    return pair;
}
*/
int compareStr(const char* str1, const char* str2) {
    int i;
    int result = 0;
    for(i = 0; result == 0 && i < strlen(str1) && strlen(str2); i++) {
        if(str1[i] != str2[i]) result = -1;
    }
    return result;
}

void printTree(BlocksArray* blocksArray) {
    if( blocksArray == NULL) {
        printf("Tablica glowna nie istnieje\n");
        return;
    }
    printf("%i blokow\n", blocksArray->size);
    int i;
    for(i = 0; i < blocksArray->size; i++) {
        printf("->blok %i - %i operacji:\n", i, blocksArray->arr[i]->numberOfOperations);
        int j;
        for(j = 0; j < blocksArray->arr[i]->numberOfOperations; j++) {
            printf("->->operacja %i\n%s", j, blocksArray->arr[i]->diffResult[j]);
        }
    }
    printf("\n");
}

int main(int argc, char* args[]) {
    // operacje:
    // - create_table rozmiar
    // - compare_pairs f1A.txt:f1B.txt f2A.txt:f2B.txt
    // - remove_block index
    // - remove_operation block_index operation_index

    if(argc == 1) {
        printf("Mozesz wywolac ten program z kilkoma argumentami:\n");
        printf("show <- wyswietla przechowywana strukture\n");
        printf("create_table size <-- tworzy tablice glowna\n");
        printf("compare_pairs f1A.txt:f1B.txt f2A.txt:f2B.txt <-- porownuje pary i zapisuje wyniki do utworzonej tablicy\n");
        printf("remove_block index <-- usuwa dany blok\n");
        printf("remove_operation block_index operation_index <-- usuwa dana operacje\n");
        return 0;
    }

    // Otwieram biblioteke

    FILE* resultFile = fopen("./results3b.txt", "a");
    if(resultFile == NULL) {
        printf("Blad otwarcia pliku\n");
        exit(1);
    }

    void *handle = dlopen("./libmylib.so", RTLD_LAZY);
    if(!handle) {
        printf("Blad otwarcia bibliotek\n");
        exit(1);
    }

    BlocksArray* (*createBlocksArray)(int maxsize) = (BlocksArray* (*)())dlsym(handle, "createBlocksArray");
    void (*freeBlocksArray)(BlocksArray* blocksArray) = (void (*)())dlsym(handle, "freeBlocksArray");
    void (*addToBlocksArray)(BlocksArray* blocksArray, Block* block) = (void (*)())dlsym(handle, "addToBlocksArray");
    void (*removeBlock)(BlocksArray* blocksArray, int index) = (void (*)())dlsym(handle, "removeBlock");
    void (*removeOperation)(Block* block, int index) = (void (*)())dlsym(handle, "removeOperation");
    FilePair* (*createFilePair)(char* file1, char* file2) = (FilePair* (*)())dlsym(handle, "createFilePair");
    Block* (*parsePairToBlock)(FilePair* pair) = (Block* (*)())dlsym(handle, "parsePairToBlock");

    int i, j;

    // zmienne do mierzenia czasu wykonania
    clock_t totalTime_Command[2], totalTime_Program[2];
    
    struct tms** timeProgram = (struct tms**)calloc(2, sizeof(struct tms*));
    for(i = 0; i < 2; i++)
        timeProgram[i] = (struct tms*)calloc(1, sizeof(struct tms));

    struct tms** tmsTime = (struct tms**)calloc(2, sizeof(struct tms*));
    for(i = 0; i < 2; i++)
        tmsTime[i] = (struct tms*)calloc(1, sizeof(struct tms));

    totalTime_Program[0] = times(timeProgram[0]);
  
    // poczatek egzekucji programu
    BlocksArray* blocksArray = NULL;

    // parsowanie argumentow programu i wykonanie
    for(i = 1; i < argc; i++) {
        char* command = strdup(args[i]);

        // mierzenie czasu - start
        totalTime_Command[0] = times(tmsTime[0]);

        if(compareStr(args[i], "show") == 0) {
        
            // printf("show:\n");
            printTree(blocksArray);
        
        }
        else if(compareStr(args[i], "create_table") == 0) {
            
            // printf("create_table:\n");
            if(i + 1 >= argc) {
                printf("Brak argumentu: rozmiar\n");
                exit(1);
            }
            blocksArray = (*createBlocksArray)( atoi(args[i+1]) );
            i++;
        
        }
        else if(compareStr(args[i], "compare_pairs") == 0) {
        
            // printf("compaire_pairs:\n");            
            if(blocksArray == NULL) {
                printf("Nie istnieje tablica glowna\n");
                exit(1);
            }
            if(i + blocksArray->maxSize >= argc) {
                printf("Podano za malo par plikow do porownania\n");
                (*freeBlocksArray)(blocksArray);
                exit(1);
            }
            
            for(j = 0; j < blocksArray->maxSize; j++) {
                // FilePair* pair = parsePair(args[i+1 + j]);
                char* file1 = NULL;
                char* file2 = NULL;
                char* token;
                char* toParse = strdup(args[i+1 + j]);

                token = strtok(toParse, ":");
                file1 = strdup(token);

                token = strtok(NULL, ":");
                file2 = strdup(token);

                if(file1 == NULL || file2 == NULL) {
                    printf("Niewlasciwy input - f1: %s, f2: %s\n", file1, file2);
                    return NULL;
                }

                FilePair* pair = (*createFilePair)(file1, file2);

                free(file1);
                free(file2);

                if(pair == NULL) {
                    printf("Blad utworzenia pary plikow\n");
                    (*freeBlocksArray)(blocksArray);
                    exit(1);
                }
                (*addToBlocksArray)(blocksArray, parsePairToBlock(pair));
            }
            i += j;
        
        }
        else if(compareStr(args[i], "remove_block") == 0) {
        
            // printf("remove_block;\n");
            if(i + 1 >= argc) {
                printf("Brak argumentu: index\n");
                (*freeBlocksArray)(blocksArray);
                exit(1);
            }
            (*removeBlock)(blocksArray, atoi(args[i+1]));
            i++;
        
        }
        else if(compareStr(args[i], "remove_operation") == 0) {
        
            // printf("remove_operation:\n");
            if(i + 2 >= argc) {
                printf("Brak argumentow: block_index, operation_index");
                (*freeBlocksArray)(blocksArray);
                exit(1);
            }
            (*removeOperation)(blocksArray->arr[atoi(args[i+1])], atoi(args[i+2]));
            i++;
            i++;
        
        }
        else {
        
            printf("Bledne wywolanie programu - zle argumenty\n");
            (*freeBlocksArray)(blocksArray);
            exit(1);
        
        }

        // sprawdzanie czasu
        totalTime_Command[1] = times(tmsTime[1]);
        
        fprintf(resultFile, "Total_time:  command %s took %fs\n", command, executionTime(totalTime_Command[0], totalTime_Command[1]) );
        fprintf(resultFile, "User_time:   command %s took %fs\n", command, executionTime(tmsTime[0]->tms_utime, tmsTime[1]->tms_utime) );
        fprintf(resultFile, "System_time: command %s took %fs\n", command, executionTime(tmsTime[0]->tms_stime, tmsTime[1]->tms_stime) );
        fprintf(resultFile, "\n");
        
        printf("Total_time:  command %s took %fs\n", command, executionTime(totalTime_Command[0], totalTime_Command[1]) );
        printf("User_time:   command %s took %fs\n", command, executionTime(tmsTime[0]->tms_utime, tmsTime[1]->tms_utime) );
        printf("System_time: command %s took %fs\n", command, executionTime(tmsTime[0]->tms_stime, tmsTime[1]->tms_stime) );
        printf("\n");
    }

    (*freeBlocksArray)(blocksArray);

    dlclose(handle);

    totalTime_Program[1] = times(timeProgram[1]);

    fprintf(resultFile, "Total_time:  Main.c took %fs\n", executionTime(totalTime_Program[0], totalTime_Program[1]) );
    fprintf(resultFile, "User_time:   Main.c took %fs\n", executionTime(timeProgram[0]->tms_utime, timeProgram[1]->tms_utime) );
    fprintf(resultFile, "System_time: Main.c took %fs\n", executionTime(timeProgram[0]->tms_stime, timeProgram[1]->tms_stime) );

    printf("Total_time:  Main.c took %fs\n", executionTime(totalTime_Program[0], totalTime_Program[1]) );
    printf("User_time:   Main.c took %fs\n", executionTime(timeProgram[0]->tms_utime, timeProgram[1]->tms_utime) );
    printf("System_time: Main.c took %fs\n", executionTime(timeProgram[0]->tms_stime, timeProgram[1]->tms_stime) );

    fprintf(resultFile, "\n-----------------\n\n");

    for(i = 0; i < 2; i++)
        free(timeProgram[i]);
    free(timeProgram);

    for(i = 0; i < 2; i++)
        free(tmsTime[i]);
    free(tmsTime);

    fclose(resultFile);

    return 0;
}