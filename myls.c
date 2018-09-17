#include <stdio.h>
#include "myls.h"

// A complete list of linux system call numbers can be found in: /usr/include/asm/unistd_64.h
#define WRITE_SYSCALL 1

int main(int argc, char **argv) {
    int i = printOut(argv[1]);
    printf("\n%d\n", i);

    return i;
}