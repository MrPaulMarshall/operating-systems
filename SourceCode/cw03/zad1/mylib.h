#include <stdio.h>
#include <dirent.h>
#include <sys/times.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

void searchDIR(const char* root, const char* atimeStr, const char* mtimeStr, const char* maxdepthStr);
void searchNFTW(const char* root, const char* atimeStr, const char* mtimeStr, const char* maxdepthStr);

