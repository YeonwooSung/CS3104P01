#include <stdio.h>
#include "myls.h"

// A complete list of linux system call numbers can be found in: /usr/include/asm/unistd_64.h
#define WRITE_SYSCALL 1

int main(int argc, char **argv) {
    if (argc > 1) {
        printOut(argv[1]);
    } else {
        char usageMsg[26] = "Usage: ./myls \"file_path\"";
        printOut(usageMsg);
    }

    return 1;
}