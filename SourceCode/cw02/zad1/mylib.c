#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#include <time.h>
#include <sys/times.h>
#include <unistd.h>
#include <fcntl.h>


void createFileTouch(const char* file) {
    char* command = (char *)calloc(10 + strlen(file), sizeof(char));
    strcpy(command, "touch ");
    strcat(command, file);

    system(command);
    free(command);
}

void errorExitSys(int file, const char* error) {
    if (file >= 0) {
        close(file);
    }
    printf("%s\n", error);
    exit(1);
}

void errorExitLib(FILE* file, const char* error) {
    if (file != NULL) {
        fclose(file);
    }
    printf("%s\n", error);
    exit(1);
}

void showFile(const char* file) {
    char* command = (char *)calloc(10 + strlen(file), sizeof(char));
    strcpy(command, "cat ");
    strcat(command, file);

    system(command);
    printf("\n");

    free(command);
}

void generateFile(const char* file, int length, int lines) {
    // utworzenie pliku za pomoca komendy "touch"
    createFileTouch(file);

    // wygenerowanie zawartosci pliku
    int commandLength = 120;
    char* command = (char *)calloc(commandLength, sizeof(char));
    for(int i = 0; i < 100; i++){
        command[i] = 0;
    }

    char itoa[12];

    strcpy(command, "head -c 1000000000 /dev/urandom | tr -dc 'a-z' | fold -w ");
    
    sprintf(itoa, "%d", length);
    strcat(command, itoa);

    strcat(command, " | head -n ");

    sprintf(itoa, "%d", lines);
    strcat(command, itoa);

    strcat(command, " > ");
    strcat(command, file);

    system(command);

    free(command);

    // dopisanie na koncu ostatniej linii znaku '\n'
    FILE* fp = fopen(file, "a");
    if (fp == NULL) {
        errorExitLib(fp, "Blad przy dopisywaniu ostatniego znaku newLine");
    }
    char* buff = (char *)calloc(1, sizeof(char));
    fwrite(buff, sizeof(char), 1, fp);

    free(buff);
    fclose(fp);
}



// funkcje w wersji SYS

void copyFileSys(const char* file1Name, const char* file2Name, int length, int lines) {
    createFileTouch(file2Name);
    
    int file1 = open(file1Name, O_RDONLY);
    if (file1 < 0) {
        errorExitSys(-1, "Blad otwarcia pliku do skopiowania");
    }
    int file2 = open(file2Name, O_WRONLY | O_CREAT);
    if (file2 < 0) {
        errorExitSys(file1, "Blad otwarcia pliku do ktorego kopiujemy");
    }

    char* buff = calloc(length + 1, sizeof(char));

    for (int i = 0; i < lines; i++) {
        if (read(file1, buff, length * sizeof(char)) != length * sizeof(char)) {
            free(buff);
            close(file1);
            printf("record %i: ", i);
            errorExitSys(file2, "Blad wczytania rekordu do skopiowania");
        }
        if (write(file2, buff, length * sizeof(char)) != length * sizeof(char)) {
            free(buff);
            close(file1);
            printf("record %i: ", i);
            errorExitSys(file2, "Blad zapisu kopiowanego rekordu");
        }
    }

    free(buff);

    close(file1);
    close(file2);
}

int compareLinesSys(const char* name, int length, int lines, int one, int two) {
    int value = 0;
    
    int file = open(name, O_RDONLY);
    if (file < 0) {
        errorExitSys(-1, "Nie otwarto pliku");
    }

    // pobieram pierwsza linie
    if (lseek(file, one * length * sizeof(char), 0) < 0) {
        errorExitSys(file, "Blad ustawienia znacznika na danej linii");
    }
    char* firstLine = (char*)calloc(length + 1, sizeof(char));
    if (read(file, firstLine, length * sizeof(char)) != length * sizeof(char)) {
        free(firstLine);
        errorExitSys(file, "Blad odczytania danej linii do porownania");
    }
    
    // pobieram druga linie
    if (lseek(file, two * length * sizeof(char), 0) < 0) {
        errorExitSys(file, "Blad ustawienia znacznika na danej linii");
    }
    char* secondLine = (char*)calloc(length + 1, sizeof(char));
    if (read(file, secondLine, length * sizeof(char)) != length) {
        free(firstLine);
        free(secondLine);
        errorExitSys(file, "Blad odczytania danej linii do porownania");
    }
    close(file);
    
    value = strcmp(firstLine, secondLine);

    free(firstLine);
    free(secondLine);

    return value;
}

void swapLinesSys(const char* fileName, int length, int lines, int i, int j) {
    if (i == j) return;

    int file = open(fileName, O_RDWR);
    if (file < 0) {
        errorExitSys(file, "Blad otwarcia pliku do swapu");
    }

    char* buffI = (char *)calloc(length + 1, sizeof(char));
    char* buffJ = (char *)calloc(length + 1, sizeof(char));

    // wczytanie pierwszego rekordu
    if (lseek(file, i * length * sizeof(char), 0) < 0) {
        errorExitSys(file, "Blad ustawienia znacznika");
    }
    if (read(file, buffI, length * sizeof(char)) != length) {
        errorExitSys(file, "Blad wczytania rekordu");
    }

    // wczytanie drugiego rekordu
    if (lseek(file, j * length * sizeof(char), 0) < 0) {
        errorExitSys(file, "Blad ustawienia znacznika");
    }
    if (read(file, buffJ, length * sizeof(char)) != length) {
        errorExitSys(file, "Blad wczytania rekordu");
    }

    // zapis na odwrotnych pozycjach
    if (lseek(file, i * length * sizeof(char), 0) < 0) {
        errorExitSys(file, "Blad ustawienia znacznika");
    }
    if (write(file, buffJ, length * sizeof(char)) != length) {
        errorExitSys(file, "Blad zapisu rekordu");
    }

    if (lseek(file, j * length * sizeof(char), 0) < 0) {
        errorExitSys(file, "Blad ustawienia znacznika");
    }
    if (write(file, buffI, length * sizeof(char)) != length) {
        errorExitSys(file, "Blad zapisu rekordu");
    }

    // zwolnienie
    free(buffI);
    free(buffJ);

    close(file);
}

void quickSortSys(const char* fileName, int length, int lines, int left, int right) {
    // warunek wyjscia
    if (left >= right) {
        return;
    }

    // tworze bufory do przechowywania porownywanych linii
    int i = left;
    int j = right;
    int mid = (left + right) / 2;

    do {
        while (compareLinesSys(fileName, length, lines, i, mid) < 0) {
            i++;
        }
        while (compareLinesSys(fileName, length, lines, j, mid) > 0) {
            j--;
        }
        if (i <= j) {
            swapLinesSys(fileName, length, lines, i, j);
            i++;
            j--;
        }
    } while (i <= j);

    quickSortSys(fileName, length, lines, left, j);
    quickSortSys(fileName, length, lines, i, right);
}

void sortFileSys(const char* path, int length, int lines) {
    quickSortSys(path, length, lines, 0, lines - 1);
}



// funkcje w wersji LIB

void copyFileLib(const char* file1Name, const char* file2Name, int length, int lines) {
    createFileTouch(file2Name);
    
    FILE* file1 = fopen(file1Name, "r");
    if(file1 == NULL){
        errorExitLib(NULL, "Blad otwarcia pliku do skopiowania");
    }

    FILE* file2 = fopen(file2Name, "w");
    if(file2 == NULL){
        errorExitLib(file1, "Blad otwarcia pliku do ktorego kopiujemy");
    }

    char* buff = calloc(length + 1, sizeof(char));

    for (int i = 0; i < lines; i++) {
        if (fread(buff, sizeof(char), length, file1) != length) {
            free(buff);
            fclose(file1);
            fclose(file2);
            printf("record %i: ", i);
            errorExitLib(NULL, "Blad wczytania rekordu do skopiowania");
        }
        if (fwrite(buff, sizeof(char), length, file2) != length) {
            free(buff);
            fclose(file1);
            fclose(file2);
            printf("record %i: ", i);
            errorExitLib(NULL, "Blad zapisu kopiowanego rekordu");
        }
    }

    free(buff);

    fclose(file1);
    fclose(file2);
}

int compareLinesLib(const char* name, int length, int lines, int one, int two) {
    int value = 0;
    
    FILE* file = fopen(name, "r");
    if (file == NULL) {
        errorExitLib(file, "Nie otwarto pliku");
    }

    // pobieram pierwsza linie
    if (fseek(file, one * length * sizeof(char), 0) != 0) {
        errorExitLib(file, "Blad ustawienia znacznika na danej linii");
    }
    char* firstLine = (char*)calloc(length + 1, sizeof(char));
    if (fread(firstLine, sizeof(char), length, file) != length) {
        errorExitLib(file, "Blad odczytania danej linii do porownania");
    }
    
    // pobieram druga linie
    if (fseek(file, two * length * sizeof(char), 0) != 0) {
        errorExitLib(file, "Blad ustawienia znacznika na danej linii");
    }
    char* secondLine = (char*)calloc(length + 1, sizeof(char));
    if (fread(secondLine, sizeof(char), length, file) != length) {
        errorExitLib(file, "Blad odczytania danej linii do porownania");
    }
    fclose(file);
    
    value = strcmp(firstLine, secondLine);

    free(firstLine);
    free(secondLine);

    return value;
}

void swapLinesLib(const char* fileName, int length, int lines, int i, int j) {
    if (i == j) return;

    FILE* file = fopen(fileName, "r+");
    if (file == NULL) {
        errorExitLib(file, "Blad otwarcia pliku do swapu");
    }

    char* buffI = (char *)calloc(length + 1, sizeof(char));
    char* buffJ = (char *)calloc(length + 1, sizeof(char));

    // wczytanie pierwszego rekordu
    if (fseek(file, i * length * sizeof(char), 0) != 0) {
        errorExitLib(file, "Blad ustawienia znacznika");
    }
    if (fread(buffI, sizeof(char), length, file) != length) {
        errorExitLib(file, "Blad wczytania rekordu");
    }

    // wczytanie drugiego rekordu
    if (fseek(file, j * length * sizeof(char), 0) != 0) {
        errorExitLib(file, "Blad ustawienia znacznika");
    }
    if (fread(buffJ, sizeof(char), length, file) != length) {
        errorExitLib(file, "Blad wczytania rekordu");
    }

    // zapis na odwrotnych pozycjach
    if (fseek(file, i * length * sizeof(char), 0) != 0) {
        errorExitLib(file, "Blad ustawienia znacznika");
    }
    if (fwrite(buffJ, sizeof(char), length, file) != length) {
        errorExitLib(file, "Blad zapisu rekordu");
    }

    if (fseek(file, j * length * sizeof(char), 0) != 0) {
        errorExitLib(file, "Blad ustawienia znacznika");
    }
    if (fwrite(buffI, sizeof(char), length, file) != length) {
        errorExitLib(file, "Blad zapisu rekordu");
    }

    // zwolnienie
    free(buffI);
    free(buffJ);

    fclose(file);
}

void quickSortLib(const char* fileName, int length, int lines, int left, int right) {
    // warunek wyjscia
    if (left >= right) {
        return;
    }

    // tworze bufory do przechowywania porownywanych linii
    int i = left;
    int j = right;
    int mid = (left + right) / 2;

    do {
        while (compareLinesLib(fileName, length, lines, i, mid) < 0) {
            i++;
        }
        while (compareLinesLib(fileName, length, lines, j, mid) > 0) {
            j--;
        }
        if (i <= j) {
            swapLinesLib(fileName, length, lines, i, j);
            i++;
            j--;
        }
    } while (i <= j);

    quickSortLib(fileName, length, lines, left, j);
    quickSortLib(fileName, length, lines, i, right);
}

void sortFileLib(const char* path, int length, int lines) { 
    quickSortLib(path, length, lines, 0, lines - 1);
}
