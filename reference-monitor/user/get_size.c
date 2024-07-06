#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SWITCH_STATE 134
#define ADD_TO_BLACKLIST 174
#define REMOVE_FROM_BLACKLIST 177
#define PRINT_BLACKLIST 178
#define GET_BLACKLIST_SIZE 180

int main(int argc, char *argv[])
{
    int ret = 0;


    ret = syscall(GET_BLACKLIST_SIZE, 1);
    if (ret == -1)
    {
        perror("...");
        ret = EXIT_FAILURE;
    }
    printf("Blacklist size: %d\n", ret);
    

    return EXIT_SUCCESS;
}