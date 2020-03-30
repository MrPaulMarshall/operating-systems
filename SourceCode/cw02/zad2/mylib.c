#include <sys/sysmacros.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdbool.h>
#include <limits.h>
#define __USE_XOPEN_EXTENDED 1
#include <ftw.h>

#include "mylib.h"

#define _XOPEN_SOURCE 1
#define _XOPEN_SOURCE_EXTENDED 1

#define OPTION_NOT_CHOSEN -100
#define OLDER_THAN 1
#define EXACTLY_AT 0
#define NEWER_THAN -1

#define INF_DEPTH 10000

int maxdepth = INF_DEPTH;
int atimeSign = OPTION_NOT_CHOSEN;
int mtimeSign = OPTION_NOT_CHOSEN;
int atime = 0;
int mtime = 0;

int isDigit(const char c) {
    if (c == '0' || c == '1' || c == '2' || c == '3' || c == '4' || c == '5' || c == '6' || c == '7' || c == '8' || c == '9') {
        return true;
    }
    return false;
}

int isNumber(const char* toCheck, int start) {
    int outcome = true;
    
    for (int i = start; i < strlen(toCheck); i++) {
        if (isDigit(toCheck[i]) == false) {
            outcome = false;
        }
    }

    return outcome;
}

char* append(char* string, const char* toAppend) {
    char* newStr = (char *)calloc(1 + strlen(string) + strlen(toAppend), sizeof(char));
    strcpy(newStr, string);
    strcat(newStr, toAppend);

    free(string);
    return newStr;
}

char* appendSlash(char* path) {
    if (strlen(path) < 1) {
        return NULL;
    }
    char* newStr = path;
    if (path[strlen(path) - 1] != '/') {
        newStr = append(path, "/");
    }
    return newStr;
}

const char* fileType(const struct stat* stats) {
    switch (stats->st_mode & __S_IFMT) {
        case __S_IFBLK:  return "block dev";       break;
        case __S_IFCHR:  return "char dev";        break;
        case __S_IFDIR:  return "dir";             break;
        case __S_IFIFO:  return "fifo";            break;
        case __S_IFLNK:  return "slink";           break;
        case __S_IFREG:  return "file";            break;
        case __S_IFSOCK: return "sock";            break;
        default:       return "unknown?";
    }
}

int daysSince(time_t oldTime) {
    return difftime(time(NULL), oldTime) / 86400;
}


// TU ZACZYNA SIE ZABAWA

void parseArguments(const char* atimeStr, const char* mtimeStr, const char* maxdepthStr) {
    atimeSign = OPTION_NOT_CHOSEN;
    atime = 0;
    mtimeSign = OPTION_NOT_CHOSEN;
    mtime = 0;
    maxdepth = INF_DEPTH;

    // analiza atimeStr
    if (atimeStr != NULL && strlen(atimeStr) >= 1) {
        if (atimeStr[0] == '+') {
            atimeSign = OLDER_THAN;
            if (isNumber(atimeStr, 1) == true) {
                atime = abs(atoi(atimeStr));
            } else {
                printf("Wszyscy zginiemy\n");
            }
        } else if (atimeStr[0] == '-') {
            atimeSign = NEWER_THAN;
            if (isNumber(atimeStr, 1) == true) {
                atime = abs(atoi(atimeStr));
            } else  {
                printf("Wszyscy zgina\n");
            }
        } else if (isNumber(atimeStr, 0) == true) {
            atimeSign = EXACTLY_AT;
            atime = atoi(atimeStr);
        } else {
            printf("Natrafiono na blad argumentu -atime\n");
            return;
        }
    }

    // analiza mtimeStr
    if (mtimeStr != NULL && strlen(mtimeStr) >= 1) {
        if (mtimeStr[0] == '+') {
            mtimeSign = OLDER_THAN;
            if (isNumber(mtimeStr, 1) == true) {
                mtime = abs(atoi(mtimeStr));
            } else {
                printf("Wszyscy zginiemy\n");
            }
        } else if (mtimeStr[0] == '-') {
            mtimeSign = NEWER_THAN;
            if (isNumber(mtimeStr, 1) == true) {
                mtime = abs(atoi(mtimeStr));
            } else  {
                printf("Wszyscy zgina\n");
            }
        } else if (isNumber(mtimeStr, 0) == true) {
            mtimeSign = EXACTLY_AT;
            mtime = atoi(mtimeStr);
        } else {
            printf("Natrafiono na blad argumentu -mtime\n");
            return;
        }
    }

    // analiza maxdepthStr
    if (maxdepthStr == NULL) {
        maxdepth = INF_DEPTH;
    } else if (strcmp(maxdepthStr, "0") == 0) {
        maxdepth = 0;
    } else {
        maxdepth = atoi(maxdepthStr);
        if (maxdepth <= 0) {
            printf("Niewlasciwa wartosc parametru -maxdepth\n");
            return;
        }
    }
}



int matchATime(const struct stat* stats) {
    int outcome = true;
    if (atimeSign != OPTION_NOT_CHOSEN) {
        int diff = daysSince(stats->st_atime);
        if (atimeSign == EXACTLY_AT && diff != atime) {
            outcome = false;
        }
        if (atimeSign == OLDER_THAN && diff <= atime) {
            outcome = false;
        }
        if (atimeSign == NEWER_THAN && diff >= atime) {
            outcome = false;
        }
    }
    return outcome;
}

int matchMTime(const struct stat* stats) {
    int outcome = true;
    if (mtimeSign != OPTION_NOT_CHOSEN) {
        int diff = daysSince(stats->st_mtime);
        if (mtimeSign == EXACTLY_AT && diff != mtime) {
            outcome = false;
        }
        if (mtimeSign == OLDER_THAN && diff <= mtime) {
            outcome = false;
        }
        if (mtimeSign == NEWER_THAN && diff >= mtime) {
            outcome = false;
        }
    }
    return outcome;
}

int matchRequirments(const struct stat* stats) {
    return (matchATime(stats) == true) && (matchMTime(stats) == true);
}

void printfFileDetails(const char* path, const struct stat* statistics) {
    printf("%s\n", path);
    printf("-> LINKS TO FILE:    %li\n", statistics->st_nlink);
    printf("-> FILE TYPE:        %s\n", fileType(statistics));
    printf("-> FILE SIZE:        %li bytes\n", statistics->st_size);

    char accessDate[24];
    strftime(accessDate, sizeof(accessDate), "%d %m %Y %H:%M", localtime(&statistics->st_atime));    
    printf("-> LAST ACCESS DATE: %s\n", accessDate);

    char modificationDate[24];
    strftime(modificationDate, sizeof(modificationDate), "%d %m %Y %H:%M", localtime(&statistics->st_mtime));
    printf("-> LAST MODIFY DATE: %s\n", modificationDate);

    printf("\n");
}


// WLASCIWE FUNKCJE

// wyszukiwanie za pomoca readdir() i stat()

void searchDIRInner(char* root, int depth) {
    DIR* dir = opendir(root);
    if (dir == NULL) {
        printf("Blad otwarcia katalogu \"%s\"\n", root);
        return;
    }
    
    // buffor dla metadanych pliku
    struct stat stats;

    struct dirent* file = readdir(dir);
    while (file != NULL) {
        // dont include . and ..
        if (strcmp(file->d_name, ".") == 0 || strcmp(file->d_name, "..") == 0) {
            file = readdir(dir);
            continue;
        }

        char* directory = strdup(root);
        char* path = appendSlash(directory);
        path = append(path, file->d_name);

        if (stat(path, &stats) < 0) {
            printf("Blad otwarcia pliku o sciezce: %s\n", path);
            free(path);
            break;
        }

        if (matchRequirments(&stats) == true) {
            printfFileDetails(path, &stats);
        }

        if (strcmp(fileType(&stats), "dir") == 0 && depth + 1 <= maxdepth) {
            searchDIRInner(path, depth + 1);
        }

        free(path);
        file = readdir(dir);
    }

    closedir(dir);
}

void searchDIR(const char* root, const char* atimeStr, const char* mtimeStr, const char* maxdepthStr) {
    parseArguments(atimeStr, mtimeStr, maxdepthStr);

    char* directory = realpath(root, NULL);
    searchDIRInner(directory, 0);
    free(directory);
}


// Funkce do wypisywania za pomoca nftw()


static int printfNFTW(const char* path, const struct stat* stats, int typeFlag, struct FTW* ftwBuff) {
    if (matchRequirments(stats) == true && ftwBuff->level <= maxdepth) {
        printfFileDetails(path, stats);
    }
    return 0;
}

void searchNFTW(const char* root, const char* atimeStr, const char* mtimeStr, const char* maxdepthStr) {
    parseArguments(atimeStr, mtimeStr, maxdepthStr);

    char* directory = realpath(root, NULL);
    nftw(directory, printfNFTW, 10, FTW_PHYS);
    free(directory);
}

