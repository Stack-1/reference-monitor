#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "user.h"

int main(int argc, char *argv[])
{
    int ret = 0;
    char *buf;
    char file_string[256];
    int error_flag = 0;
    buf = getlogin();

    sprintf(file_string,"/home/%s/Desktop/rf/",buf);

    ret = syscall(REMOVE_FROM_BLACKLIST, file_string, "1234");
    if (ret == -1)
    {
        perror("...");
        error_flag = 1;
    }

    ret = syscall(PRINT_BLACKLIST, 1);
    if (ret == -1)
    {
        perror("...");
        error_flag = 1;
    }



    return error_flag;
}