#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    int ret = 0;


    ret = syscall(174, "/home/","1234");
    if (ret == -1)
    {
        perror("...");
        return EXIT_FAILURE;
    }


    ret = syscall(174, "/home/temp.txt","1234");
    if (ret == -1)
    {
        perror("...");
        return EXIT_FAILURE;
    }

    ret = syscall(177, 1);
    if (ret == -1)
    {
        perror("...");
        return EXIT_FAILURE;
    }


    return EXIT_SUCCESS;
}
