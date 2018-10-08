#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <time.h>

/* system call numbers */
#define READ_SYSCALL 0  //to read the file to copy the data of that file
#define WRITE_SYSCALL 1 //to copy the data from the original file
#define OPEN_SYSCALL 2  //to open the directory or file
#define CLOSE_SYSCALL 3 //to close the opened directory or file
#define STAT_SYSCALL 4  //to get the file stat of the specific file

/* preprocessor for the file permission mode */
#define OPEN_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) //the mode for the open syscall

/* preprocessor for the buffer size */
#define BUF_SIZE 1024
#define READ_SIZE 1023


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
 * The aim of this funciton is to check the digits of the given number.
 * @param num the number to check the digits
 * @return i ((the number of digits) - 1)
 */
int checkDigits(int num) {
    int length = 1; // whenever its an integer, the smallest length should be 1
    while (num >= 10) { // loops until number is greater than 10
        num /= 10;
        length++;
    }
    return length;
}

/**
 * This function is a wrapper function of the write syscall.
 * This function uses the inline assembly function to make interaction with the kernel more explicit.
 * To implement this function, I reused the given code, which is written by Kasim Terzic.
 *
 * @param text the target text that should be printed out
 * @param handle for stdout, 2 for stderr, file handle from open() for files
 * @return ret If the syscall success, returns 1. Otherwise, returns -1.
 */
int writeText(const char *text, long handle) {
    size_t len = strlength(text); //Length of our string, which we need to pass to write syscall
    long ret = -1;                //Return value received from the system call

    asm("movq %1, %%rax\n\t" // %1 == (long) WRITE_SYSCALL
        "movq %2, %%rdi\n\t" // %2 == handle
        "movq %3, %%rsi\n\t" // %3 == text
        "movq %4, %%rdx\n\t" // %4 == len
        "syscall\n\t"
        "movq %%rax, %0\n\t" // %0 == ret
        : "=r"(ret)          /* if the syscall success, 1 will be stored in the ret. */
        : "r"((long)WRITE_SYSCALL), "r"(handle), "r"(text), "r"(len)
        : "%rax", "%rdi", "%rsi", "%rdx", "memory"
    );

    return ret;
}

/**
 * This function is a wrapper function of open system call.
 *
 * @param name the name of the directory that should be opened
 * @return ret If the syscall success, the lowest numbered unused file descriptor will be returned. Otherwise, returns some negative value.
 */
int openFile(char *name) {
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
        : "r"((long)OPEN_SYSCALL), "r"(name), "r"((long)flag), "r"((long)mode)
        : "%rax", "%rdi", "%rsi", "%rdx", "memory");

    return ret;
}

/**
 * This is a wrapper function of the close syscall.
 *
 * @param fd the file descriptor of the opened file (or directory)
 * @return Returns zero on success. On error, negative value would be returned, which will depend on the setted error number.
 */
int closeFile(long fd) {
    long ret = -1;

    asm("movq %1, %%rax\n\t" // %1 = (long) OPEN_SYSCALL
        "movq %2, %%rdi\n\t" // %2 = fd
        "syscall\n\t"
        "movq %%rax, %0\n\t"
        : "=r"(ret)
        : "r"((long)CLOSE_SYSCALL), "r"(fd)
        : "%rax", "%rdi", "%rsi", "%rdx", "memory");

    return ret;
}

/**
 * This function is a wrapper function of the read system call.
 * It reads the data from the file that is corresponding to the given file descriptor.
 *
 * @param fd the file descriptor
 * @param buf the buffer to store the read data
 * @param count the total number of bits to read
 * @return Returns the number of bytes that were read. If value is negative, then the system call returned an error.
 */
int readFile(unsigned int fd, char *buf, long count) {
    long ret = -1;

    asm("movq %1, %%rax\n\t" // %1 = (long) READ_SYSCALL
        "movq %2, %%rdi\n\t" // %2 = (long) fd
        "movq %3, %%rsi\n\t" // %3 = buf
        "movq %4, %%rdx\n\t" // %4 = count
        "syscall\n\t"
        "movq %%rax, %0\n\t"
        : "=r"(ret)
        : "r"((long)READ_SYSCALL), "r"((long)fd), "r"(buf), "r"(count) //convert the type from int to long for the movq instruction
        : "%rax", "%rdi", "%rsi", "%rdx", "memory");

    return ret;
}

/**
 * This function uses the stat syscall to check if the given name of file is a directory or a file.
 *
 * @param name the name of the file (or directory)
 * @param statBuffer the buffer to store the file stat
 * @return If the stat syscall fails, returns -1. If the given file is a directory, returns 1. Otherwise, returns 0.
 */
int checkFileStat(char *name, struct stat *statBuffer) {
    long ret = -1;

    asm("movq %1, %%rax\n\t" // %1 = (long) STAT_SYSCALL
        "movq %2, %%rdi\n\t" // %2 = fileName
        "movq %3, %%rsi\n\t" // %3 = statBuffer
        "syscall\n\t"
        "movq %%rax, %0\n\t"
        : "=r"(ret)
        : "r"((long) STAT_SYSCALL), "r"(name), "r"(statBuffer) //covert the type from int to long for the movq instruction
        : "%rax", "%rdi", "%rsi", "memory");

    return ret;
}

/**
 * This function opens the file and check the file stat of that file to read and print out the contents in it.
 *
 * @param name the name of the file.
 */
void mycat(char *name) {
    struct stat stats; // declares struct stat to store stat of argument
    int stat = checkFileStat(name, &stats);

    if (stat != 0) { //if the stat syscall fails, some negative value will be returned
        writeText("mycat: cannot access '", 1);
        writeText(name, 1);
        writeText("' : No such file or directory\n", 1);
        return;
    }

    int fd = openFile(name);

    if (fd < 0) { //check if the open syscall fails

        //the reason of the failure would be the file permission issue.
        writeText("mycat: cannot access '", 1);
        writeText(name, 1);
        writeText("' : Permission denied\n", 1);
        return;
    }

    if (S_ISDIR(stats.st_mode)) { //when the given file is a directory
        writeText("mycat: ", 1);
        writeText(name, 1);
        writeText(": Is a directory\n", 1);
        return;
    }

    char buffer[BUF_SIZE];
    int length;
    //Reads max 1023 byte at once.
    while ((length = readFile(fd, buffer, READ_SIZE)) > 0) {
        buffer[length] = '\0';
        writeText(buffer, 1);
    }

    closeFile(fd);

    return;
}

/** 
 * This function is used when there is no argument give for this program.
 * This function will read the string via stdin stream, and print out the read string via stdout stream.
 */
void readAndCatViaStdin() {
    char buffer[BUF_SIZE]; //initialize the buffer

    int length;
    //Reads max 1023 byte at once.
    while ((length = readFile(0, buffer, READ_SIZE)) > 0) {
        buffer[length] = '\0';
        writeText(buffer, 1);
    }
    return;
}

/* The mycat is a software that does something similar with the command line tool "cat". */
int main(int argc, char **argv) {
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            mycat(argv[i]);
        }
    } else {
        readAndCatViaStdin();
    }
    return 0;
}
