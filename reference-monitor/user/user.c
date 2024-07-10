#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    int ret = 0;

    ret = syscall(156, "/home/stack1/Desktop/rf/", "1234");
    if (ret == -1)
    {
        perror("...");
    }

    ret = syscall(156, "/home/stack1/Desktop/temp.txt", "1234");
    if (ret == -1)
    {
        perror("...");
    }

    ret = syscall(177,1);
    if (ret == -1)
    {
        perror("...");
    }

    ret = syscall(178,1);
    if (ret == -1)
    {
        perror("...");
    }
    printf("Blacklist size %d\n",ret);

    return EXIT_SUCCESS;
}
