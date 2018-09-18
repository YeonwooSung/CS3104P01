#ifndef MYLS_H
#define MYLS_H

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <dirent.h>

int checkFileStat(char *fileName);
int openDirectory(char *name);
int printOut(char *);

#endif //MYLS_H