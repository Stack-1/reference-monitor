#define _GNU_SOURCE
#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    char input_char[1];
    

    int file = syscall(SYS_open, argv[1], 0, 0777);

    printf("%s\n", argv[1]);
    printf("%d\n", file);

    int ret = syscall(SYS_read, file, input_char, 1);
    printf("Read returned %d, reading %s\n",ret,input_char);
    syscall(SYS_write, 1, input_char, 1);

    /*while (syscall(SYS_read, file, input_char, 1) > 0)
    {
        syscall(SYS_write, stdout, input_char, 1);
    }*/

    puts("");
    return 0;
}