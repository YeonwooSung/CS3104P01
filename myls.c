#include <stdio.h> //TODO should remove this line before submit this

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <time.h>
#include <dirent.h>

/* System call numbers */
#define WRITE_SYSCALL 1      //to print out the output message of the ls command
#define OPEN_SYSCALL 2       //to open the directory
#define STAT_SYSCALL 4       //to get the file stat of the specific file.
#define MMAP_SYSCALL 9       //to implement the custom malloc
#define MUNMAP_SYSCALL 11    //to unmap the dynamically mapped memory
#define GETDENTS_SYSCALL 78  //to get the directory entries

#define GETDENT_BUFFER_SIZE 1024 //this will be used for the getdents syscall

/* mode of the open syscall */
#define OPEN_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) //this will be used for the open syscall

/* preprocessors for the custom malloc function */
#define MAX_HEAP_SIZE 268435456 //4 * 1024 * 4096
#define CUSTOM_PROT (PROT_READ | PROT_WRITE)
#define MMAP_FLAG (MAP_PRIVATE | MAP_ANONYMOUS)


/* The global variables for the custom memory allocating function */
char *heap;             //the pointer that points the custom heap
char *brkp = NULL;      //the pointer that points the custom break of the heap
char *endp = NULL;      //the pointer that points the end of the custom heap

/* function prototype */
int printOut(char *); //the prototype of the printOut function.

/**
 * This function initialises the global variables for the custom memory allocating function.
 * The mmap syscall is used to initialise the custom heap memory.
 */
void initHeap() {
    char *ptr = NULL;
    unsigned long fd = -1;
    unsigned long offset = 0;

    asm("movq %1, %%rax\n\t" // %1 == (long) MMAP_SYSCALL
        "movq %2, %%rdi\n\t" // %2 == NULL
        "movq %3, %%rsi\n\t" // %3 == (unsigned long) MAX_HEAP_SIZE
        "movq %4, %%rdx\n\t" // %4 == (unsigned long) CUSTOM_PROT
        "movq %5, %%r10\n\t" // %5 == (unsigned long) MMAP_FLAG
        "movq %6, %%r8\n\t"  // %6 == fd
        "movq %7, %%r9\n\t"   // %7 == offset
        "syscall\n\t"
        "movq %%rax, %0\n\t" // %0 == ptr
        : "=r"(ptr)          /* if the syscall success, the memory address of the mapped memory will be stored in the ptr */
        : "r"((long)MMAP_SYSCALL), "r"(NULL), "r"((unsigned long)MAX_HEAP_SIZE), "r"((unsigned long)CUSTOM_PROT), "r"((unsigned long)MMAP_FLAG), "r"(fd), "r"(offset)
        : "%rax", "%rdi", "%rsi", "%rdx", "%r10", "%r8", "%r9", "memory");

    heap = ptr;
    brkp = ptr;
    endp = ptr + MAX_HEAP_SIZE;
}

/**
 * The aim of this function is to unmap the mapped memory by using the syscall munmap.
 *
 * @return ret If the munmap syscall success, returns 0. Otherwise, returns -1.
 */
int myUnMap() {
    long ret = -1;

    asm("movq %1, %%rax\n\t" // %1 == (long) MUNMAP_SYSCALL
        "movq %2, %%rdi\n\t" // %2 == (unsigned long) heap
        "movq %3, %%rsi\n\t" // %3 == (unsigned long) MAX_HEAP_SIZE
        "syscall\n\t"
        "movq %%rax, %0\n\t"
        : "=r"(ret)
        : "r"((long)MUNMAP_SYSCALL), "r"((unsigned long) heap), "r"((unsigned long) MAX_HEAP_SIZE)
        : "%rax", "%rdi", "%rsi", "memory");
    
    return ret;
}

/**
 * The aim of this function is to move the break of the custom heap to allocate the memory dynamically.
 *
 * To implement this custom sbrk function, I copied some of the codes from the following article.
 * @reference <https://people.kth.se/~johanmon/ose/assignments/maplloc.pdf>
 */
void *mysbrk(size_t size) {
    if (size == 0) {
        return (void *) brkp;
    }

    void *free = (void *)brkp;
    brkp += size;    
    if (brkp >= endp) {
        return NULL;
    }

    return free;
}


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
 * The aim of this function is to copy the string to the new string.
 *
 * @param str the pointer that points the new string
 * @param s the pointer that points the string that contains the value that should be copied
 * @param length the length of the string that should be copied
 */
void strcopy(char *str, char *s, int length) {
    for (int i = 0; i < length; i++) {
        *str = *s;
        str += 1;
        s += 1;
    }
}

/**
 * This function concatenates 2 given strings.
 *
 * @param str1 the pointer that points the first string
 * @param str2 the pointer that points the second string
 * @return newStr the pointer that points the concatenated string
 */
char *strconcat(char *str1, char *str2) {
    int length1 = strlength(str1);
    int length2 = strlength(str2);
    int length = length1 + length2 + 1;

    char *newStr = (char *) mysbrk(length); //dynamically allocate the memory to concatenate strings
    char *temp = newStr;

    strcopy(temp, str1, length1);
    temp += length1;

    strcopy(temp, str2, length2);
    temp += length2;

    *temp = '\0';

    return newStr;
}

/**
 * This function uses the getdents system call to read the opened directory, and check the file stats of all files in this directory.
 *
 * @param fd the file descriptor of the target directory
 */
void getDirectoryEntries(long fd) {
    long nread = -1;
    int bpos;
    struct linux_dirent *ld;

    for (;;) {
        /* 
        * If the system call success, the getdents syscall returns the number of bytes read. 
        * On end of directory, the getdents syscall returns 0. 
        * Otherwise, it returns -1.
        */
        asm("movq %1, %%rax\n\t" // %1 = (long) OPEN_SYSCALL
            "movq %2, %%rdi\n\t" // %2 = fd
            "movq %3, %%rsi\n\t" // %3 = ld
            "movq %4, %%rdx\n\t" // %4 = (long) GETDENT_BUFFER_SIZE
            "syscall\n\t"
            "movq %%rax, %0\n\t"
            : "=r"(nread)
            : "r"((long)GETDENTS_SYSCALL), "r"(fd), "r"(ld), "r"((long)GETDENT_BUFFER_SIZE)
            : "%rax", "%rdi", "%rsi", "%rdx", "memory");

        if (nread == -1) {
            char errorMsg[40] = "Error occurred in the getdents syscall\n";
            printOut(errorMsg); //print out the error message
            break;
        } else if (nread == 0) { //check the getdents syscall is on the end of the directory
            break;
        }

        printf("--------------- nread=%ld ---------------\n", nread);

        //TODO iterate the linux_dirent list, and call the checkFileStat
    }
}

/**
 * This function opens the file by using the syscall.
 * The openDirectory() will be used to open the directory for the ls command.
 * This function uses the extended inline assembler to make interaction with the kernel more explicit.
 * 
 * @param name the name of the directory that should be opened for the ls command
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
 * This function uses the syscall to get the stat of the specific file.
 *
 * @param fileName the name of the target file
 * @param openFlag to check if the program should call the open syscall
 * @return ret If the syscall success, returns 0. Otherwise, returns -1.
 */
int checkFileStat(char *fileName, char openFlag) {
    long ret = -1;

    struct stat statBuffer;

    asm("movq %1, %%rax\n\t" // %1 = (long) STAT_SYSCALL
        "movq %2, %%rdi\n\t" // %2 = fileName
        "movq %3, %%rsi\n\t" // %3 = statBuffer
        "syscall\n\t"
        "movq %%rax, %0\n\t"
        : "=r"(ret)
        : "r"((long)STAT_SYSCALL), "r"(fileName), "r"(&statBuffer) //covert the type from int to long for the movq instruction
        : "%rax", "%rdi", "%rsi", "memory");

    mode_t mode = statBuffer.st_mode; //mode of file

    //use the bitwise operators to check if the current file is a directory and check if the program should call the open syscall
    if ( (S_IFDIR & mode) && openFlag) {
        int fd = openDirectory(fileName); //open the directory
        getDirectoryEntries(fd);
    } else {
        uid_t user = statBuffer.st_uid;        //user id
        gid_t group = statBuffer.st_gid;       //group id
        time_t modTime = statBuffer.st_mtime;  //last modified time
        struct tm *modT = localtime(&modTime); //struct time of the last modified time

        time_t time;
        struct tm *now = localtime(&time);     //this pointer points to the time struct which shows the current date time

        //TODO compare the year of modT with now, and print out the suitable output
    }

    //TODO at the end of the checkFileStat, move back the brk

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

int main(int argc, char **argv) {
    if (argc > 1) {
        initHeap();

        int i;
        for (i = 1; i < argc; i++) {
            checkFileStat(argv[i], 1);
        }

        myUnMap(); //unmap the dynamically mapped memory
    } else {
        char usageMsg[27] = "Usage: ./myls \"file_path\"\n";
        printOut(usageMsg);
    }

    return 1;
}