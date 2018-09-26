#include <stdio.h>

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
#define TIME_SYSCALL 201     //to get the current time

/* The buffer size for the getdents syscall */
#define GETDENT_BUFFER_SIZE 8192 //this will be used for the getdents syscall

/* mode of the open syscall */
#define OPEN_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) //this will be used for the open syscall

/* preprocessors for the custom malloc function */
#define MAX_HEAP_SIZE 4194304 //1024 * 4096
#define CUSTOM_PROT (PROT_READ | PROT_WRITE)
#define MMAP_FLAG (MAP_PRIVATE | MAP_ANONYMOUS)

/* struct for the getdents syscall */
struct linux_dirent {
    long d_ino;                /* Inode number */
    off_t d_off;               /* Offset to next linux_dirent */
    unsigned short d_reclen;   /* Length of this linux_dirent */
    char d_name[];             /* Filename (null-terminated) */
};

/* struct to store the file stat strings */
struct fileStat {
    char *fileInfo; // file permission, number of hard links, user id, group id
    char *link;     // the number of hard links
    char *uid;      // the user id
    char *gid;      // the group id
    char *modTime;  // the last modified time
    char *fileName; // the name of the file
    char *fileSize; // the size of the file
    struct fileStat *next; // pointer that points to the next node
};

/* The global variables for the custom memory allocating function */
char *heap;             //the pointer that points the custom heap
char *brkp = NULL;      //the pointer that points the custom break of the heap
char *endp = NULL;      //the pointer that points the end of the custom heap

/* The global variables that will be used for printing out the file stat with a suitable length */
struct fileStat *fs = NULL;
struct fileStat *currentNode = NULL;
char lengthOfFileSize = 0;   //maximum digits of the file size
char lengthOfLink = 0;       //maximum digits of the number of hard links
char lengthOfUID = 0;        //maximum digits of the user id
char lengthOfGID = 0;        //maximum digits of the group id
int currentYear;             //the time as the number of years since 1900

/* function prototype */
int printOut(char *);             //a wrapper function of write() system call.
int checkFileStat(char *, char);  //a wrapper function of stat() system call.
int openDirectory(char *);        //a wrapper function of open() system call.

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

    void *memp = (void *)brkp;
    brkp += size;       //move the brkp to allocate memory
    if (brkp >= endp) { //to check if there is empty space in the heap
        brkp -= size;   //move back the brkp
        return NULL;    //returns NULL if there is no free space in the heap
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
    int length1 = strlength(str1);          //check the length of the first string
    int length2 = strlength(str2);          //check the length of the second string
    int length = length1 + length2 + 1;     // add 1 for the terminator.

    char *newStr = (char *) mysbrk(length); //dynamically allocate the memory to concatenate strings
    char *temp = newStr;

    strcopy(temp, str1, length1);           //copy the characters in the first string to the new string
    temp += length1;

    strcopy(temp, str2, length2);           //copy the characters in the second string to the new string
    temp += length2;

    *temp = '\0';                           //append the terminator at the end of the new string

    return newStr;
}

/**
 * This function is a wrapper function of the getdents system call.
 * The system call getdents() reads several linux_dirent structures from
 * the directory referred to by the open file descriptor into the buffer.
 *
 * @param fd the file descriptor of the target directory
 */
void getDirectoryEntries(long fd) {
    long nread = -1;
    int bpos;
    char d_type, buf[GETDENT_BUFFER_SIZE];
    struct linux_dirent *ld;

    for (;;) {
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
            : "r"((long)GETDENTS_SYSCALL), "r"(fd), "r"(buf), "r"((unsigned long)GETDENT_BUFFER_SIZE)
            : "%rax", "%rdi", "%rsi", "%rdx", "memory");

        if (nread == -1) {
            char errorMsg[40] = "Error occurred in the getdents syscall\n";
            printOut(errorMsg); //print out the error message
            break;
        } else if (nread == 0) { //check if the getdents syscall is on the end of the directory
            break;
        }

        for (bpos = 0; bpos < nread;) {
            ld = (struct linux_dirent *)(buf + bpos);

            if (*ld->d_name != '.') {
                checkFileStat(ld->d_name, 0);
            }

            bpos += ld->d_reclen;
        }

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
 * This function converts the month from integer to string, and copies that string to the given char pointer.
 *
 * @param month From 0 for January to 11 for December
 * @param str the pointer that points to the new string.
 */
void convertMonthToStr(int month, char *str) {
    /* use the switch statement to convert the month from number to string */
    switch (month) {
        case 0 : 
            strcopy(str, "Jan", 3);
            break;
        case 1 : 
            strcopy(str, "Feb", 3);
            break;
        case 2:
            strcopy(str, "Mar", 3);
            break;
        case 3:
            strcopy(str, "Apr", 3);
            break;
        case 4:
            strcopy(str, "May", 3);
            break;
        case 5:
            strcopy(str, "Jun", 3);
            break;
        case 6:
            strcopy(str, "Jul", 3);
            break;
        case 7:
            strcopy(str, "Aug", 3);
            break;
        case 8:
            strcopy(str, "Sep", 3);
            break;
        case 9:
            strcopy(str, "Oct", 3);
            break;
        case 10:
            strcopy(str, "Nov", 3);
            break;
        case 11:
            strcopy(str, "Dec", 3);
    }
}

/**
 * The aim of this funciton is to check the digits of the given number.
 * @param num the number to check the digits
 * @return i ((the number of digits) - 1)
 */
int checkDigits(int num) {
    int i = 0, j = 10;

    while (j < num) {
        j *= 10;
        i += 1;
    }

    return i;
}

/**
 * This function converts the type of the given integer from in to string.
 *
 * @param str the pointer points to the string
 * @param num the target number
 * @return i the digits of the given number
 */
int convertNumToStr(char *str, int num) {
    int i = 0, j = checkDigits(num);

    while (num) {
        *(str + j) = (char) ('0' + (num % 10));
        num /= 10;
        i += 1;
        j -= 1;
    }

    return i;
}

/**
 * This function checks the file permission of the particular file and convert the file permission to the string.
 *
 * @param fp the pointer that points to the character array
 * @param mode the variable that shows the file permission of the specific file. 
 */
void checkFilePermission(char *fp, mode_t mode) {
    if (mode & S_IFDIR) {
        fp[0] = 'd';
    } else {
        fp[0] = '-';
    }

    if (mode & S_IRUSR) {
        fp[1] = 'r';
    } else {
        fp[1] = '-';
    }

    if (mode & S_IWUSR) {
        fp[2] = 'w';
    } else {
        fp[2] = '-';
    }

    if (mode & S_IXUSR) {
        fp[3] = 'x';
    } else {
        fp[3] = '-';
    }

    if (mode & S_IRGRP) {
        fp[4] = 'r';
    } else {
        fp[4] = '-';
    }

    if (mode & S_IWGRP) {
        fp[5] = 'w';
    } else {
        fp[5] = '-';
    }

    if (mode & S_IXGRP) {
        fp[6] = 'x';
    } else {
        fp[6] = '-';
    }

    if (mode & S_IROTH) {
        fp[7] = 'r';
    } else {
        fp[7] = '-';
    }

    if (mode & S_IWOTH) {
        fp[8] = 'w';
    } else {
        fp[8] = '-';
    }

    if (mode & S_IXOTH) {
        fp[9] = 'x';
    } else {
        fp[9] = '-';
    }

    fp[10] = ' ';
    fp[11] = '\0';
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

        char *str = (char *)mysbrk(14);
        char *temp = str;

        if (modT->tm_mday < 10) {
            *str = ' ';
            temp += 1;
        }

        convertNumToStr(temp, modT->tm_mday); //convert the type of the day of the month from number to string

        temp += 2;
        *temp++ = ' ';

        convertMonthToStr(modT->tm_mon, temp); //convert the type of the month from number to string

        temp += 3;
        *temp++ = ' ';

        if (modT->tm_year != currentYear) {
            *temp++ = ' ';
            convertNumToStr(temp, (modT->tm_year + 1900)); //convert the type of the year from number to string
            temp += 4;
            *temp++ = ' ';
            *temp = '\0';
        } else {
            
            if (modT->tm_hour == 0) {
                *temp++ = '0';
                *temp++ = '0';
            } else if (modT->tm_hour < 10) { //check whether the value of the hour of the last modified time is less than 10 or not
                //if so, append 0 in front of the time string.
                *temp++ = '0';
                convertNumToStr(temp++, modT->tm_hour); //convert the type of the hour from number to string
            } else {
                convertNumToStr(temp, modT->tm_hour); //convert the type of the hour from number to string
                temp += 2;
            }

            *temp++ = ':';

            if (modT->tm_min == 0) {
                *temp++ = '0';
                *temp++ = '0';
            } else if (modT->tm_min < 10) { //check whether the value of the minute of the last modified time is less than 10 or not
                //if so, append 0 in front of the time string.
                *temp++ = '0';
                convertNumToStr(temp++, modT->tm_min); //convert the type of the minute from number to string
            } else {
                convertNumToStr(temp, modT->tm_min); //convert the type of the minute from number to string
                temp += 2;
            }

            *temp++ = ' ';
            *temp = '\0';
        }

        currentNode->modTime = str;

        int length = strlength(fileName);
        currentNode->fileName = (char *)mysbrk(length + 1);
        strcopy(currentNode->fileName, fileName, length); //copy the file name to the fileName field of the current file stat node.

        char fp[12];
        checkFilePermission(fp, mode);
        currentNode->fileInfo = (char *) mysbrk(12); //allocate the memory to store the file permission and the file type
        strcopy(currentNode->fileInfo, fp, 11);      //copy the file permission string to store it

        int digits_uid = checkDigits(user) + 1;
        int digits_gid = checkDigits(group) + 1;

        if (digits_gid > lengthOfGID) lengthOfGID = digits_gid;
        if (digits_uid > lengthOfUID) lengthOfUID = digits_uid;

        char *str_uid = (char *) mysbrk(digits_uid + 2); //allocate the memory for the user id
        convertNumToStr(str_uid, user);
        *(str_uid + digits_uid) = ' '; //append whitespace for the output
        *(str_uid + digits_uid + 1) = '\0'; //append the terminator
        currentNode->uid = str_uid;

        char *str_gid = (char *) mysbrk(digits_gid + 2); //allocate the memory for the group id
        convertNumToStr(str_gid, group);
        *(str_gid + digits_gid) = ' '; //append whitespace for the output
        *(str_gid + digits_gid + 1) = '\0'; //append the terminator
        currentNode->gid = str_gid;

        nlink_t links = statBuffer.st_nlink; //get the number of hard links
        int digits_link = checkDigits(links) + 1;
        if (digits_link > lengthOfLink) lengthOfLink = digits_link;
        char *str_link = (char *) mysbrk(digits_link + 2); //allocate the memory for the number of hard links
        convertNumToStr(str_link, links);
        *(str_link + digits_link) = ' '; //append whitespace for the output
        *(str_link + digits_link + 1) = '\0'; //append the terminator
        currentNode->link = str_link;

        off_t size = statBuffer.st_size; //get the file size
        int digits_size = checkDigits(size) + 1;
        if (digits_size > lengthOfFileSize) lengthOfFileSize = digits_size;
        char *str_size = (char *) mysbrk(digits_size + 2); //allocate the memory for the file size
        convertNumToStr(str_size, size);
        *(str_size + digits_size) = ' '; //append whitespace character for the output
        *(str_size + digits_size + 1) = '\0'; //append the terminator
        currentNode->fileSize = str_size;

        /* make a new node for the next file (if exist) */
        struct fileStat *newNode = (struct fileStat *)mysbrk(sizeof(struct fileStat));
        currentNode->next = newNode;
        newNode->next = NULL;
        currentNode = newNode;
    }

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

/**
 * This is a wrapper function of the time syscall.
 * The aim of this function is to get the current time by using the time system call.
 *
 * @return ret On success, the value of time in seconds since the Epoch is returned. Otherwise, returns -1.
 */
int getCurrentTime() {
    long ret = -1;

    asm("movq %1, %%rax\n\t" // %1 == (long) TIME_SYSCALL
        "movq %2, %%rdi\n\t" // %2 == NULL
        "syscall\n\t"
        "movq %%rax, %0\n\t" // %0 == ret
        : "=r"(ret)
        : "r"((long)TIME_SYSCALL), "r"(NULL)
        : "%rax", "%rdi", "memory");

    return ret;
}

void checkLengthForOutput(int length, char *str) {
    int lengthOf = strlength(str);
    char *output = (char *) mysbrk(length);
    char *temp = output;

    int limit = length - lengthOf;

    int i;
    for (i = 0; i <= limit; i++) {
        *temp++ = ' ';
    }
    strcopy(temp, str, lengthOf);

    printOut(output);
}

void util(char *argv) {

    fs = (struct fileStat *)mysbrk(sizeof(struct fileStat)); // allocate the memory for the linked list that stores the file stat.
    currentNode = fs;
    currentNode->next = NULL;

    checkFileStat(argv, 1);

    while (fs->next) {
        printOut(fs->fileInfo); //print out the file permission

        /* make each line the same fixed length by appending suitable number of whitespace characters */
        checkLengthForOutput(lengthOfLink, fs->link);
        checkLengthForOutput(lengthOfUID, fs->uid);
        checkLengthForOutput(lengthOfGID, fs->gid);
        checkLengthForOutput(lengthOfFileSize, fs->fileSize);

        printOut(fs->modTime); //print out the last modified time

        char *name = strconcat(fs->fileName, "\n"); //concatenate the file name and next line character
        printOut(name);                             //print out the file name, and move to the next line

        fs = fs->next;
    }
}

int main(int argc, char **argv) {
    time_t time = getCurrentTime(); //use the system call "time" to get the current time
    struct tm *now = localtime(&time);
    currentYear = now->tm_year;

    if (argc == 2) {
        initHeap(); //use the mmap syscall to get the virtual memory for dynamic memory allocation

        util(argv[1]);

        myUnMap(); // use the munmap syscall to unmap the virtual memory.
    } else if (argc > 2) {
        int i;
        for (i = 1; i < argc; i++) {
            initHeap(); //use the mmap syscall to get the virtual memory for dynamic memory allocation

            char *msg = strconcat(argv[i], " :\n");
            printOut(msg);

            util(argv[i]);

            char nl[2] = "\n";
            printOut(nl);

            myUnMap(); // use the munmap syscall to unmap the virtual memory.
        }
 
    } else {
        char usageMsg[27] = "Usage: ./myls \"file_path\"\n";
        printOut(usageMsg);
    }

    return 1;
}