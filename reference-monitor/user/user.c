#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pwd.h>

#include "user.h"

int main(int argc, char *argv[])
{
    int ret = 0;
    char *buf;
    char file_string[256];
    int error_flag = 0;
    char *blacklist;
    int blacklist_size;
    struct passwd *pass; 

    pass = getpwuid(getuid());
    if (!pass)
    {
        printf("%s\n", strerror(errno));
        return -1;
    }
    buf = pass->pw_name;


    sprintf(file_string, "/home/stack1/Desktop/rf/");

    ret = syscall(ADD_TO_BLACKLIST, file_string, "pippo");
    if (ret == -1)
    {
        perror("...");
        error_flag = 1;
    }



    return error_flag;
}
