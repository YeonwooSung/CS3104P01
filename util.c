#include "myls.h"

#define WRITE_SYSCALL 1  //to print out the output message of the ls command
#define OPEN_SYSCALL 2   //to open the directory
#define STAT_SYSCALL 4   //to get the file stat of the specific file.

/**
 * The custom strlen function.
 *
 * @param str the string to check it's length
 * @return the length of the string
 */
int strlength(char *str) {
    int count = 0;

    while (*str != '\0') {
        count += 1;
        str += 1;
    }

    return count;
}

int  openFile(char *fileName) {
    long ret = -1;
    //filename(rdi) flag(rsi) mode(rdx)

    asm("movq %1, %%rax\n\t" // %1 = (long) OPEN_SYSCALL
        "movq %2, %%rdi\n\t" // %2 = fileName
        "syscall\n\t"
        "movq %%rax, %0\n\t"
        : "=r"(ret)
        : "r"((long) OPEN_SYSCALL), "r"(fileName)
        : "%rax", "%rdi", "%rsi", "%rdx");

    return ret;
}


/**
 * This function prints out the given string.
 * This function uses the inline assembly function to make interaction with the kernel more explicit.
 * To implement this function, I reused the given code, which is written by Kasim Terzic.
 *
 * @param text the target text that should be printed out
 * @return ret If the syscall success, returns 1. Otherwise, returns -1.
 */
int printOut(char *text) {

    //using long ints here to avoid converting them for asm (rxx registers are 64 bits)

    size_t len = strlength(text);  //Length of our string, which we need to pass to write syscall
    long handle = 1;  //1 for stdout, 2 for stderr, file handle from open() for files
    long ret = -1;    //Return value received from the system call


    /* 
     * Using inline assembler(extended assembler syntax), the long way. Here all the registers used are visible.
     * Note that we have to protect rax etc. in the very last line to stop the gnu compiler
     * from using them (and thus overwriting our data)
     * Parameters go to RAX, RDI, RSI, RDX, in that order.
     */
    asm("movq %1, %%rax\n\t" // %1 == (long) WRITE_SYSCALL
        "movq %2, %%rdi\n\t" // %1 == handle
        "movq %3, %%rsi\n\t" // %3 == text
        "movq %4, %%rdx\n\t" // %4 == len
        "syscall\n\t"
        "movq %%rax, %0\n\t" // %0 == ret
        : "=r"(ret) /* output */
        : "r"((long) WRITE_SYSCALL), "r"(handle), "r"(text), "r"(len) /* input */
        : "%rax", "%rdi", "%rsi", "%rdx", "memory"); /* clobbered registers */

    return ret;
}