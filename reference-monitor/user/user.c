#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    printf("%ld\n", syscall(134, 3,"1234") );
    return EXIT_SUCCESS;
}

