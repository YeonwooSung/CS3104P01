#include <stdio.h>
#include "myls.h"

int main(int argc, char **argv) {
    //check the number of the command line arguments
    if (argc == 2) {
        //printOut(argv[1]);
        int i = openDirectory(argv[1]);
        printf("%d", i);
    } else {
        char usageMsg[27] = "Usage: ./myls \"file_path\"\n";
        printOut(usageMsg);
    }

    return 1;
}