#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <time.h>
#include <dirent.h>

/* system call numbers */
#define READ_SYSCALL 0      //to read the file to copy the data of that file
#define WRITE_SYSCALL 1     //to copy the data from the original file
#define OPEN_SYSCALL 2      //to open the directory
#define STAT_SYSCALL 4      //to get the file stat of the specific file.
#define MMAP_SYSCALL 9      //to implement the custom malloc
#define MUNMAP_SYSCALL 11   //to unmap the dynamically mapped memory
#define ACCESS_SYSCALL 21   //to check if the file exists
#define GETDENTS_SYSCALL 78 //to get the directory entries

#define OPEN_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

/**
 * The custom strlen function.
 *
 * @param str the string to check it's length
 * @return the length of the string
 */
int strlength(const char *str) {
    int count = 0;

    while (*str != '\0') { //iterate the while loop until it reaches to the terminator character
        count += 1;
        str += 1;
    }

    return count;
}

/**
 * This function prints out the given string.
 * This function uses the inline assembly function to make interaction with the kernel more explicit.
 * To implement this function, I reused the given code, which is written by Kasim Terzic.
 *
 * @param text the target text that should be printed out
 * @return ret If the syscall success, returns 1. Otherwise, returns -1.
 */
int printOut(const char *text) {
    size_t len = strlength(text); //Length of our string, which we need to pass to write syscall
    long handle = 1;              //1 for stdout, 2 for stderr, file handle from open() for files
    long ret = -1;                //Return value received from the system call

    asm("movq %1, %%rax\n\t" // %1 == (long) WRITE_SYSCALL
        "movq %2, %%rdi\n\t" // %2 == handle
        "movq %3, %%rsi\n\t" // %3 == text
        "movq %4, %%rdx\n\t" // %4 == len
        "syscall\n\t"
        "movq %%rax, %0\n\t" // %0 == ret
        : "=r"(ret)          /* if the syscall success, 1 will be stored in the ret. */
        : "r"((long)WRITE_SYSCALL), "r"(handle), "r"(text), "r"(len)
        : "%rax", "%rdi", "%rsi", "%rdx", "memory");

    return ret;
}

/**
 * This function opens the file by using the syscall.
 * The openDirectory() will be used to open the directory to read the files in it.
 * This function uses the extended inline assembler to make interaction with the kernel more explicit.
 *
 * @param name the name of the directory that should be opened
 * @return ret If the syscall success, the lowest numbered unused file descriptor will be returned. Otherwise, returns some negative value.
 */
int openDirectory(char *name) {
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
        : "r"((long)OPEN_SYSCALL), "r"(name), "r"((long)flag), "r"((long)mode) //convert the type from int to long for the movq instruction
        : "%rax", "%rdi", "%rsi", "%rdx", "memory");

    return ret;
}

/**
 * This is a wrapper function of the access syscall.
 *
 * @param fileName the name of the file to check if it exists
 * @return If the file exists, returns 0. Otherwise, returns some negative integer
 */
int accessToFile(char *fileName) {
    long ret = -1;

    asm("movq %1, %%rax\n\t"
        "movq %2, %%rdi\n\t"
        "movq %3, %%rsi\n\t"
        "syscall\n\t"
        "movq %%rax, %0\n\t"
        : "=r"(ret)
        : "r"((long)ACCESS_SYSCALL), "r"(fileName), "r"((long)R_OK)
        : "%rax", "%rdi", "%rsi", "memory");

    return ret;
}

/**
 * This function prints out the error message to announce to the user that the given name of the file (or directory) does no exist.
 *
 * @param fileName the name of the file that does not exist
 */
void printFileNotExists(char *fileName) {
    char errMessage[22] = "mycp: cannot access \'";
    char errMessage2[30] = "\': No such file or directory\n";

    // print out the error message
    printOut(errMessage);
    printOut(fileName);
    printOut(errMessage2);
}

/* mycp is a program that copies the source to the destination recursively. */
int main(int argc, char **argv) {

    if (argc == 2) {
        if (accessToFile(argv[1]) != 0) { //use the access syscall to check if the file exists
            printOut("mycp: cannot stat");
            printOut(argv[1]);
            printOut(": Cannot find such file or directory\n");
        } else {
            //
        }

    } else {
        char usageMsg[38] = "Usage: ./mycp \"SOURCE\" \"DESTINATION\"\n";
        printOut(usageMsg);
    }

    return 1;
}