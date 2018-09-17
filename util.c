#include "myls.h"

#define WRITE_SYSCALL 1

#include <stdio.h>

int strlength(char *str) {
    int count = 0;

    while (*str != '\0') {
        count += 1;
        str += 1;
        printf("%d\n", count);
    }

    return count;
}

/**
 * This function prints out the given string.
 * This function uses the inner assembly function to make interaction with the kernel more explicit.
 * I reuse the Kasim's code to implement this function.
 *
 * @param text the target text that should be printed out
 * @return ret
 */
int printOut(char *text) {

    //using long ints here to avoid converting them for asm (rxx registers are 64 bits)

    size_t len = strlength(text);  //Length of our string, which we need to pass to write syscall
    long handle = 1;  //1 for stdout, 2 for stderr, file handle from open() for files
    long ret = -1;    //Return value received from the system call

    // Using inline assembler with shortcuts.
    // The registers are the same as above. We use a constraint to force variables into particular registers:
    // a = rax, D = rdi, S = rsi, d = rdx... This is a property of GNU inline assembler.
    // The only asm instruction we have to execute is syscall, since all the arguments are in the right registers
    asm("syscall"
        : "=a"(ret)
        : "0"(WRITE_SYSCALL), "D"(handle), "S"(text), "d"(len)
        : "cc", "rcx", "r11", "memory");

    return ret;
}