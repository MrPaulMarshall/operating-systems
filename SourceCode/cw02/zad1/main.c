#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#include <time.h>
#include <sys/times.h>
#include <unistd.h>

#include "mylib.h"

// sluzy do tworzenia nazw plikow tymczasowych przy kolejnych wywolaniach quicksorta
// int subSortIndex;

// FUNKCJONALNOSCI:
// 1. generate - tworzy plik z losowym tekstem
// 2. sort  - sortuje rekordy w pliku w porzadku leksykograficznym
// 3. copy  - kopiuje plik f1 do pliku f2

// porzadek leksykograficzny:
// -> aabbc
// -> aabbcc
// -> abbbcc
// -> ccddaa

// przy sortowaniu ma NIE BYC tablicy rekordow
// ma to byc w plikach tymaczasowych, zeby uzywac operacji I/O

double executionTime(clock_t start, clock_t end) {
    return (double)(end - start) / (double)sysconf(_SC_CLK_TCK);
}

int main(int argc, char* args[]) {
    if (argc == 1) {
        printf("Poprawne wywolanie programu ma postac:\n");
        printf("user]$ %s operacja argumenty\n\n", args[0]);
        printf("Dostepne operacje to:\n");
        printf("generate plik liczba_wierszy liczba_znakow\n");
        printf("sort plik liczba_wierszy liczba_znakow sys/lib\n");
        printf("copy plik1 plik2 liczba_wierszy liczba_znakow sys/lib\n\n");

        return 0;
    }

    char* operation = strdup(args[1]);
    
    if (strcmp(operation, "generate") == 0) {
        if (argc != 5) {
            printf("Zla liczba argumentow\n");
            exit(1);
        }
        int lines = atoi(args[3]);
        int length = atoi(args[4]);
        if (lines <= 0 || length <= 0) {
            printf("Zle argumenty\n");
            exit(1);
        }
        generateFile(args[2], length, lines);
        
        return 0;
    }

    // operacje sortowania lub kopiowania - zaczynam mierzenie czasu

    char* methodUsed = NULL;

    if (strcmp(operation, "copy") == 0) {
        if (argc != 7) {
            printf("Zla liczba argumentow");
            exit(1);
        }

        int lines = atoi(args[4]);
        // dodaje 1, zeby uwzglednic \n
        int length = atoi(args[5]) + 1;
        if (lines <= 0 && length <= 1) {
            printf("Zle argumenty\n");
            exit(1);
        }
        methodUsed = strdup(args[6]);
        if (strcmp(methodUsed, "lib") == 0) {
            copyFileLib(args[2], args[3], length, lines);
        } else if (strcmp(methodUsed, "sys") == 0) {
            copyFileSys(args[2], args[3], length, lines);
        } else {
            printf("Nie podano prawidlowego sposobu wykonania\n");
        }
    } else if (strcmp(operation, "sort") == 0) {
        
        // zmienne do mierzenia czasu wykonania
        clock_t totalTime[2];
        
        struct tms** tms = (struct tms**)calloc(2, sizeof(struct tms*));
        for(int i = 0; i < 2; i++)
            tms[i] = (struct tms*)calloc(1, sizeof(struct tms));

        // start
        totalTime[0] = times(tms[0]);
  
        // wykonanie sortowania        
        if (argc != 6) {
            printf("Zla liczba argumentow\n");
            exit(1);
        }
        int lines = atoi(args[3]);
        // dodaje 1, zeby uwzglednic \n
        int length = atoi(args[4]) + 1;
        if (lines == 0 || length == 0) {
            printf("Zle argumenty\n");
            exit(1);
        }
        methodUsed = strdup(args[5]);
        if (strcmp(methodUsed, "lib") == 0) {
            sortFileLib(args[2], length, lines);
        } else if (strcmp(methodUsed, "sys") == 0) {
            sortFileSys(args[2], length, lines);
        } else {
            printf("Nie podano prawidlowego sposobu wykonania\n");
        }

        // koniec
        totalTime[1] = times(tms[1]);

        printf("%s sort, %i lines, %i bytes:\n", methodUsed, lines, length-1);
        printf("Total_time:  sort took %fs\n", executionTime(totalTime[0], totalTime[1]) );
        printf("User_time:   sort took %fs\n", executionTime(tms[0]->tms_utime, tms[1]->tms_utime) );
        printf("System_time: sort took %fs\n", executionTime(tms[0]->tms_stime, tms[1]->tms_stime) );
        printf("\n");

        // zwolnienie pamieci do mierzenia czasu
        for(int i = 0; i < 2; i++)
            free(tms[i]);
        free(tms);
    } else {
        printf("Zla operacja\n");
    }

    if (methodUsed != NULL) {
        free(methodUsed);
    }
    if (operation != NULL) {
        free(operation);
    }

    return 0;
}
