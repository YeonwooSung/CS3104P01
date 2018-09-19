#include "myls.h"

#define WRITE_SYSCALL 1  //to print out the output message of the ls command
#define OPEN_SYSCALL 2   //to open the directory
#define STAT_SYSCALL 4   //to get the file stat of the specific file.

#define OPEN_MODE S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH //this will be used for the open syscall


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


/**
 * This function opens the file by using the syscall.
 * The openDirectory() will be used to open the directory for the ls command.
 * This function uses the extended inline assembler to make interaction with the kernel more explicit.
 * 
 * @param name the name of the directory that should be opened for the ls command
 * @return ret If the syscall success, the lowest numbered unused file descriptor will be returned. Otherwise, returns some negative value.
 */
int  openDirectory(char *name) {
    long ret = -1;
    int flag = O_RDONLY;
    int mode = OPEN_MODE;

    asm("movq %1, %%rax\n\t" // %1 = (long) OPEN_SYSCALL
        "movq %2, %%rdi\n\t" // %2 = name
        "movq %3, %%rsi\n\t" // %3 = flag
        "movq %4, %%rdx\n\t" // %4 = mode
        "syscall\n\t"
        "movq %%rax, %0\n\t"
        : "=r"(ret)
        : "r"((long) OPEN_SYSCALL), "r"(name), "r"((long) flag), "r"((long) mode) //convert the type from int to long for the movq instruction
        : "%rax", "%rdi", "%rsi", "%rdx", "memory");

    return ret;
}


int checkFileStat(char *fileName) {
    long ret = -1;
    struct stat *statBuffer = NULL;

    asm("movq %1, %%rax\n\t" // %1 = (long) STAT_SYSCALL
        "movq %2, %%rdi\n\t" // %2 = fileName
        "movq %3, %%rsi\n\t" // %3 = statBuffer
        "syscall\n\t"
        "movq %%rax, %0\n\t"
        : "=r"(ret)
        : "r"((long) STAT_SYSCALL), "r"(fileName), "r"(statBuffer) //covert the type from int to long for the movq instruction
        : "%rax", "%rdi", "%rsi", "memory");

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
        : "=r"(ret) /* if the syscall success, 1 will be stored in the ret. */
        : "r"((long) WRITE_SYSCALL), "r"(handle), "r"(text), "r"(len)
        : "%rax", "%rdi", "%rsi", "%rdx", "memory");

    return ret;
}