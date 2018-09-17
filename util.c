#include "myls.h"

#define WRITE_SYSCALL 1

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

    size_t len = 31;  //Length of our string, which we need to pass to write syscall
    long handle = 1;  //1 for stdout, 2 for stderr, file handle from open() for files
    long ret = -1;    //Return value received from the system call

    //Using inline assembler. Here all the registers used are visible
    //Note that we have to protect rax etc. in the very last line to stop the gnu compiler
    //from using them (and thus overwriting our data)
    //Parameters go to RAX, RDI, RSI, RDX, in that order
    asm("movq %1, %%rax\n\t"
        "movq %2, %%rdi\n\t"
        "movq %3, %%rsi\n\t"
        "movq %4, %%rdx\n\t"
        "syscall\n\t"
        "movq %%rax, %0\n\t"
        : "=r"(ret)
        : "r"((long)WRITE_SYSCALL), "r"(handle), "r"(text), "r"(len)
        : "%rax", "%rdi", "%rsi", "%rdx", "memory");

    return ret;
}