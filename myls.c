#include "myls.h"

// A complete list of linux system call numbers can be found in: /usr/include/asm/unistd_64.h
#define WRITE_SYSCALL 1

int main(int argc, char **argv) {
    return printOut(argv[1]);
}