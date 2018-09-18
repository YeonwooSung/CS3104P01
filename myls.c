#include <stdio.h>
#include "myls.h"


int main(int argc, char **argv) {
    //check the number of the command line arguments
    if (argc == 2) {
        printOut(argv[1]);
    } else {
        char usageMsg[27] = "Usage: ./myls \"file_path\"\n";
        printOut(usageMsg);
    }

    return 1;
}