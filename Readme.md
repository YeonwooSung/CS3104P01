# SystemUtility


## Overview

The main aim of this project is to implement program by using the linux system calls with inline assembler.

### myls

The myls is a system utility similar to a command-line tool "ls - n".
The output format of the myls should be "file_permission the_number_of_hardlink user_id group_id last_modified_time file_name", as long as the myls should work just like the "ls -n" command.


### mycp

The mycp is a system utility similar to a command-line tool "cp -r".
The mycp should be able to copy the source file recursively.
This means that if the source file is a directory, mycp should be able to copy all files in the source directory.


### mycat

The mycat is a system utility similar to a command-line tool "cat.
The mycat should concat given files and print out it via stdout stream.
If there is no command line argument, mycat should be able to read the user input and print out it.


## Design & Implementation

### myls

The total number of system calls that were used for implementing the myls is 8: WRITE(1), OPEN(2), STAT(4), MMAP(9), MUNMAP(11), ACCESS(21), GETDENTS(78), and TIME(201).

Basically, I implemented wrapper functions of the linux system calls, and used those wrapper functions to read the file stats and print out the result.

#### Usage of each system calls

1) WRITE:

    To print out the result of the myls

2) OPEN:

    To open the directory to read file stats of the files in it

3) STAT:

    To read the file stat.

4) MMAP:

    To get the virtual memory for the simple memory allocating function.

5) MUNMAP:

    To unmap the mapped virtual memory.

6) ACCESS:

    To check if the file exists.

7) GETDENTS:

    To iterate the files in the target directory.

8) TIME:

    To get the current time.


#### Simple memory allocation

By using the mmap system call, I implemented a simple memory allocating function. The design of this memory allocating function is really simple. The mmap will return a huge virtual memory block. I used this memory block as a virtual heap, and put the custom break on this virtual heap. And when I need to allocate memory dynamically, mysbrk() function will move the custom break to allocate the memory.

The biggest feature of my custom memory allocating function is that there is no "free()" function that frees the dynamically allocated memory. Because, it is clear that to minimize the time of the malloc and free, the malloc function should just allocate the given size of memory from the heap (not checking the free list), and free function should do nothing.

As my memory allocating function does not really handle the freed memory, it is possible to say that it is not a good design of memory allocating function. Moreover, my program only uses the mmap syscall once, which means that if the program allocates all bytes in the virtual heap, the segmentation fault would be occurred.

However, the main aim of this project is make a system utility which works similar to "ls -n" command, not making a memory allocating function with a good design. Moreover, the size of the virtual heap is 4194304 bytes(4 * 1024 * 1024B = 4MB), which is big enough. That is the reason that I implemented this memory allocating function, which is fast but not memory efficient.


#### Linked list for formatting

For the formatting, I need to store the file stats of all files that myls read. So, I used linked list that stores the file stat strings in it. To implement this linked list, I used my custom memory allocating function.

The reason that I needed to implement the linked list is that I need to count the number of digits of file stats of all files. If you use the "ls -n" command on the terminal, you could see that the output of the myls is formatted by additional whitespaces. To do this, I needed to count the number of digits of all files.

Thus, while the getdents iterates the files in the directory, I read the file stat of that file, count the number of digits of the file stats to check the maximum number of digits, and store the file stat stirngs in the current node of the linked list. By doing this, myls does not need to run the getdents loop twice(once for counting the digits and once for print out the file stat with the proper format).


### mycp

The total number of system calls that were used for implementing the mycp is 17: READ(0), WRITE(1), OPEN(2),
CLOSE(3), STAT(4), MMAP(9), MUNMAP(11), ACCESS(21), EXIT(60), TRUNC(76), FTRUNC(77), GETDENTS(78), MKDIR(83), RMDIR(84), CREAT(85), UNLINK(87), CHMOD(90).


### mycat

## Usage

### myls

To compile the myls, type "gcc myls.c -o myls -Wall -Wextra" on the terminal.

The myls requires more than 1 command line argument, which is the directory (or a file) that you want to see the file stats.

    i.e. "./myls ."

### mycp

### mycat