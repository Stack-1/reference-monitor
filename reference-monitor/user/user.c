#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    int ret = 0;

    ret = syscall(174, "/home/stack1/Desktop/temp.txt", "1234");
    if (ret == -1)
    {
        perror("...");
        return EXIT_FAILURE;
    }

    ret = syscall(174, "/home/stack1/Desktop/rf/", "1234");
    if (ret == -1)
    {
        perror("...");
        return EXIT_FAILURE;
    }

    ret = syscall(178, 1);
    if (ret == -1)
    {
        perror("...");
        return EXIT_FAILURE;
    }

    ret = syscall(180, 1);
    if (ret == -1)
    {
        perror("...");
        return EXIT_FAILURE;
    }
    printf("Blacklist size: %d\n", ret);

    return EXIT_SUCCESS;
}
