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
#define GETDENTS_SYSCALL 78 //to get the directory entries

int main(int argc, char **argv) {
    //trying to implement the recursive copy
}