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


## Design & Implementation

### myls

The total number of system calls that were used for implementing the myls is 8: WRITE(1), OPEN(2), STAT(4), MMAP(9), MUNMAP(11), ACCESS(21), GETDENTS(78), and TIME(201).


### mycp

## TODO

- read
- stat
- mmap
- munmap
- getdents