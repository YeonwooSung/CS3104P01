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
#define OPEN_SYSCALL 2      //to open the directory or file
#define CLOSE_SYSCALL 3     //to close the opened directory or file
#define STAT_SYSCALL 4      //to get the file stat of the specific file
#define MMAP_SYSCALL 9      //to implement the custom malloc
#define MUNMAP_SYSCALL 11   //to unmap the dynamically mapped memory
#define ACCESS_SYSCALL 21   //to check if the file exists
#define EXIT_SYSCALL 60     //to terminate the process when the error occurred
#define TRUNC_SYSCALL 76    //to truncate the file that is specified by the file path
#define FTRUNC_SYSCALL 77   //to truncate the file that is specified by the file descriptor
#define GETDENTS_SYSCALL 78 //to get the directory entries
#define MKDIR_SYSCALL 83    //to make the directory
#define RMDIR_SYSCALL 84    //to remove the directory
#define CREAT_SYSCALL 85    //to create the file
#define CHMOD_SYSCALL 90    //to change the mode(file permission) of the file

/* preprocessors for the file permission mode */
#define OPEN_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)    //the mode for the open syscall
#define MKDIR_MODE (S_IRWXU | S_IRWXG | S_IROTH)             //the mode for the mkdir syscall
#define CREATE_MODE (S_IRWXU | S_IRWXG | S_IRWXO)            //the mode for the creat syscall

/* preprocessors for the buffer size */
#define READ_SIZE 4096     //buffer size for the read syscall
#define GETDENTS_SIZE 2048 //buffer size for the getdents syscall

/* 
 * The struct for the getdents syscall 
 * I found this struct from the linux man page.
 *
 * @reference http://man7.org/linux/man-pages/man2/getdents.2.html
 */
struct linux_dirent {
    long d_ino;              /* The inode number */
    off_t d_off;             /* Offset to next linux_dirent */
    unsigned short d_reclen; /* Length of this linux_dirent */
    char d_name[];           /* Filename (null-terminated) */
};


/* The global variables for the custom memory allocating function */
char *heap;        //the pointer that points the custom heap
char *brkp = NULL; //the pointer that points the custom break of the heap
char *endp = NULL; //the pointer that points the end of the custom heap

/* preprocessors for the custom malloc function */
#define MAX_HEAP_SIZE 4194304 //1024 * 4096
#define CUSTOM_PROT (PROT_READ | PROT_WRITE)
#define MMAP_FLAG (MAP_PRIVATE | MAP_ANONYMOUS)

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
        "movq %7, %%r9\n\t"  // %7 == offset
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
 * This function is a wrapper function of the munmap system call.
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
        : "r"((long)MUNMAP_SYSCALL), "r"((unsigned long)heap), "r"((unsigned long)MAX_HEAP_SIZE)
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
        return (void *)brkp;
    }

    void *memp = (void *)brkp;
    brkp += size; //move the brkp to allocate memory

    if (brkp >= endp) { //to check if there is empty space in the heap
        brkp -= size; //move back the brkp
        return NULL;  //returns NULL if there is no free space in the heap
    }

    return memp;
}

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
 * The aim of this function is to copy the string to the new string.
 *
 * @param str the pointer that points the new string
 * @param s the pointer that points the string that contains the value that should be copied
 * @param length the length of the string that should be copied
 */
void strcopy(char *str, const char *s, int length) {
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
char *strconcat(const char *str1, const char *str2) {
    int length1 = strlength(str1);      //check the length of the first string
    int length2 = strlength(str2);      //check the length of the second string
    int length = length1 + length2 + 1; // add 1 for the terminator.

    char *newStr = (char *)mysbrk(length); //dynamically allocate the memory to concatenate strings
    char *temp = newStr;

    strcopy(temp, str1, length1); //copy the characters in the first string to the new string
    temp += length1;

    strcopy(temp, str2, length2); //copy the characters in the second string to the new string
    temp += length2;

    *temp = '\0'; //append the terminator at the end of the new string

    return newStr;
}

/**
 * This function compares the two strings.
 *
 * @param str1 the pointer that points to the first string
 * @param str2 the pointer that points to the second string
 *
 * @return Returns an integer less than, equal to, or greater than zero if str1 is found, 
 *         respectively, to be less than, to match, or be greater than str2.
 */
int strCompare(const char *str1, const char *str2) {
    int i = 0;

    while (str1[i] != '\0') {
        if (str1[i] != str2[i]) {
            break;
        }
        i+= 1;
    }

    return (str1[i] - str2[i]);
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
        : "r"((long) WRITE_SYSCALL), "r"(handle), "r"(text), "r"(len)
        : "%rax", "%rdi", "%rsi", "%rdx", "memory");

    return ret;
}

/**
 * Prints out the given text via stdout stream.
 *
 * @param text the text
 * @return If the write syscall success, returns 1. Otherwise, returns -1.
 */
int printOut(const char *text) {
    long l = 1; //1 for stdout
    return writeText(text, l);
}

/**
 * Prints out the given text via stderr stream.
 *
 * @param text the error message
 * @return If the write syscall success, returns 1. Otherwise, returns -1.
 */
int printErr(const char *text) {
    long l = 2; //2 for stderr
    return writeText(text, l);
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
        : "r"((long) OPEN_SYSCALL), "r"(name), "r"((long)flag), "r"((long)mode)
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
        : "r"((long) CLOSE_SYSCALL), "r"(fd)
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
        : "r"((long) READ_SYSCALL), "r"((long) fd), "r"(buf), "r"(count) //convert the type from int to long for the movq instruction
        : "%rax", "%rdi", "%rsi", "%rdx", "memory");

    return ret;
}

/**
 * This function uses the stat syscall to check if the given name of file is a directory or a file.
 *
 * @param name the name of the file (or directory)
 * @return If the stat syscall fails, returns -1. If the given file is a directory, returns 1. Otherwise, returns 0.
 */
int checkFileStat(char *name) {
    long ret = -1;

    struct stat statBuffer;

    asm("movq %1, %%rax\n\t" // %1 = (long) STAT_SYSCALL
        "movq %2, %%rdi\n\t" // %2 = fileName
        "movq %3, %%rsi\n\t" // %3 = statBuffer
        "syscall\n\t"
        "movq %%rax, %0\n\t"
        : "=r"(ret)
        : "r"((long) STAT_SYSCALL), "r"(name), "r"(&statBuffer) //covert the type from int to long for the movq instruction
        : "%rax", "%rdi", "%rsi", "memory");

    if (ret != 0) { //0 will be stored in the ret only when the stat syscall success.
        printErr("Failed to get the stat of ");
        printErr(name);
        printErr("\n");

        return -1;
    }

    if (statBuffer.st_mode & S_IFDIR) { //check if the file is directory
        return 1;
    }

    return 0;
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
        : "r"((long) ACCESS_SYSCALL), "r"(fileName), "r"((long)R_OK)
        : "%rax", "%rdi", "%rsi", "memory");

    return ret;
}

/**
 * This is a wrapper function of the exit syscall.
 *
 * @param exitCode the exit code
 */
void exitProcess(int exitCode) {
    long l = -1;

    asm("movq %1, %%rax\n\t"
        "movq %2, %%rdi\n\t"
        "syscall\n\t"
        "movq %%rax, %0\n\t"
        : "=r"(l)
        : "r"((long) EXIT_SYSCALL), "r"((long) exitCode)
        : "%rax", "%rdi"
    );
}

/**
 * This is a wrapper function of the truncate syscall.
 * This function causes the regular file named by path to be truncated to a size of precisely length bytes.
 *
 * @param filePath the file path of the target file
 * @param length the size to truncate
 * @return On success, zero is returned. On error, some negative value, which depends to the error number, will be returned.
 */
int truncateFile(char *filePath, long length) {
    long ret = -1;

    asm("movq %1, %%rax\n\t" //%1 == (long) TRUNC_SYSCALL
        "movq %2, %%rdi\n\t" //%2 == filePath
        "movq %3, %%rsi\n\t" //%3 == length
        "syscall\n\t"
        "movq %%rax, %0\n\t" //%0 == ret
        : "=r"(ret)
        : "r"((long) TRUNC_SYSCALL), "r"(filePath), "r"(length)
        : "%rax", "%rdi", "%rsi", "memory"
    );

    return ret;
}

/**
 * This is a wrapper function of the ftruncate syscall.
 * This function causes the regular file named by the file descriptor to be truncated to a size of precisely length bytes.
 *
 * @param fd the file descriptor of the target file
 * @param length the size to truncate
 * @return On success, zero is returned. On error, some negative value, which depends to the error number, will be returned.
 */
int ftruncateFile(int fd, long length) {
    long ret = -1;

    asm("movq %1, %%rax\n\t" //%1 == (long) TRUNC_SYSCALL
        "movq %2, %%rdi\n\t" //%2 == (long) fd
        "movq %3, %%rsi\n\t" //%3 == length
        "syscall\n\t"
        "movq %%rax, %0\n\t" //%0 == ret
        : "=r"(ret)
        : "r"((long) FTRUNC_SYSCALL), "r"((long) fd), "r"(length)
        : "%rax", "%rdi", "%rsi", "memory");

    return ret;
}

/**
 * This function is a wrapper function of the getdents system call.
 * The system call getdents() reads several linux_dirent structures from
 * the directory referred to by the open file descriptor into the buffer.
 *
 * @param fd the file descriptor of the target directory
 */
void getDirectoryEntries(char *directoryName, long fd) {
    long nread = -1;
    int bpos;
    char d_type, buf[GETDENTS_SIZE];
    struct linux_dirent *ld;

    for (;;) { //use the endless loop, which will loop until the getdents syscall reads all files in the directory.
        /* 
         * If the system call success, the getdents syscall returns the number of bytes read. 
         * On end of directory, the getdents syscall returns 0. 
         * Otherwise, it returns -1.
         */
        asm("movq %1, %%rax\n\t" // %1 = (long) OPEN_SYSCALL
            "movq %2, %%rdi\n\t" // %2 = fd
            "movq %3, %%rsi\n\t" // %3 = buf
            "movq %4, %%rdx\n\t" // %4 = (long) GETDENT_BUFFER_SIZE
            "syscall\n\t"
            "movq %%rax, %0\n\t"
            : "=r"(nread)
            : "r"((long) GETDENTS_SYSCALL), "r"(fd), "r"(buf), "r"((unsigned long)GETDENTS_SIZE)
            : "%rax", "%rdi", "%rsi", "%rdx", "memory"
        );

        if (nread == -1) {
            char errorMsg[40] = "Error occurred in the getdents syscall\n";
            printErr(errorMsg); //print out the error message
            break;
        } else if (nread == 0) { //check if the getdents syscall is on the end of the directory
            break;
        }

        for (bpos = 0; bpos < nread;) {
            ld = (struct linux_dirent *)(buf + bpos);

            //TODO iterate the directories

            bpos += ld->d_reclen;
        }
    }
}

/**
 * This is a wrapper function of the creat syscall.
 * It creates the file with a given path name.
 *
 * @param pathName the path name of the file that would be created
 * @return Returns the new file descriptor on success. Otherwise, some negative integer will be returned.
 */
int createFile(char *pathName) {
    long ret = -1;

    asm("movq %1, %%rax\n\t"
        "movq %2, %%rdi\n\t"
        "movq %3, %%rsi\n\t"
        "syscall\n\t"
        "movq %%rax, %0\n\t"
        : "=r"(ret)
        : "r"((long) CREAT_SYSCALL), "r"(pathName), "r"((long) CREATE_MODE)
        : "%rax", "%rdi", "%rsi", "memory"
    );

    return ret;
}

/**
 * This is a wrapper function for the mkdir system call.
 * It creates a new directory with a given name.
 *
 * @param name the name of the new directory
 * @return On success, 0 will be returned. Otherwise, some negative value will be returned.
 */
int makeDirectory(char *name) {
    long ret = -1;

    asm("movq %1, %%rax\n\t" //%1 == (long) MKDIR_SYSCALL
        "movq %2, %%rdi\n\t" //%2 == name
        "movq %3, %%rsi\n\t" //%3 == (long) MKDIR_MODE
        "syscall\n\t"
        "movq %%rax, %0\n\t" //%0 == ret
        : "=r"(ret)
        : "r"((long) MKDIR_SYSCALL), "r"(name), "r"((long)MKDIR_MODE)
        : "%rax", "%rdi", "%rsi", "memory");

    return ret;
}

/**
 * This function is a wrapper function of the rmdir system call.
 * It removes the particular directory.
 *
 * @param name the name of the target directory.
 * @return On success, 0 will be returned. Otherwise, some negative value will be returned.
 */
int removeDirectory(char *name) {
    long ret = -1;

    asm("movq %1, %%rax\n\t" //%1 == (long) RMDIR_SYSCALL
        "movq %2, %%rdi\n\t" //%2 == name
        "syscall\n\t"
        "movq %%rax, %0\n\t" //%0 == ret
        : "=r"(ret)
        : "r"((long) RMDIR_SYSCALL), "r"(name)
        : "%rax", "%rdi", "memory");

    return ret;
}

/**
 * This function removes the generated directory(if required), and exit the process.
 *
 * @param generated to check whether the process should remove the directory or not
 * @param name the name of the target directory
 */
void terminateAndRemoveDir(char generated, char *name) {
    if (generated) {
        removeDirectory(name);
    }
    exitProcess(0);
}

/* mycp is a program that copies the source to the destination recursively. */
int main(int argc, char **argv) {

    if (argc != 3){

        char usageMsg[38] = "Usage: ./mycp \"SOURCE\" \"DESTINATION\"\n";
        printErr(usageMsg);
        exitProcess(0);

    } else {
        if (accessToFile(argv[1]) != 0) { //use the access syscall to check if the source file exists

            printErr("mycp: cannot stat ");
            printErr(argv[1]);
            printErr(": Cannot find such file or directory\n");

            exitProcess(1);

        } else {
            char notExists = 1; //to check if the destination directory exists.

            if (accessToFile(argv[2]) == 0) { //use the access syscall to check if the destination directory exists.
                if (checkFileStat(argv[2]) != 1) { //use the stat syscall to check whether the destination is a directory or not.
                    printErr(argv[2]);
                    printErr(" is not a directory!\n");
                    exitProcess(0);
                }

                notExists = 0;
            }

            if (strCompare(argv[1], argv[2]) == 0) { //check if the source name and the destination name are same
                printErr("mycp: ");
                printErr(argv[1]);
                printErr(" and ");
                printErr(argv[2]);
                printErr(" are the same file.\n");
                exitProcess(0);
            }

            if (notExists) {
                makeDirectory(argv[2]); //create the destination directory.
            }

            int val = checkFileStat(argv[2]);

            if (val > 0) { //checkFileStat returns 1 when the target file is a directory
                //TODO directory
            } else if (val != 0) { //when the val < 0, error is occurred in the stat syscall
                printErr("mycp failed\n");
                exitProcess(0);
            } else { //checkFileStat returns 1 when the target file is not a directory.
                //TODO non directory
            }
        }

    }

    return 1;
}