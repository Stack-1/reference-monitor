#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "user.h"

int main(int argc, char *argv[])
{
    int ret = 0;
    int state;
    char *passw;
    char state_string[16];

    if (argc != 3)
    {
        puts("Error: This program should be called with 2 parameters, the state number and the password");
        return EXIT_FAILURE;
    }

    state = strtol(argv[1], NULL, 0);
    if (state == -1)
    {
        perror("");
        return EXIT_FAILURE;
    }

    passw = argv[2];
    if (passw == NULL)
    {
        printf("Error: Password in NULL");
        return EXIT_FAILURE;
    }

    ret = syscall(SWITCH_STATE, state, passw);
    if (ret == -1)
    {
        if (errno == EACCES)
        {
            printf("%s: Invalid Password\n",strerror(errno));
        }else{
            perror("...");
        }
        
        return EXIT_FAILURE;
    }

    switch (ret)
    {
    case RF_ON:
        strcpy(state_string, (const char *)"ON");
        break;
    case RF_OFF:
        strcpy(state_string, (const char *)"OFF");
        break;
    case RF_REC_ON:
        strcpy(state_string, (const char *)"REC_ON");
        break;
    case RF_REC_OFF:
        strcpy(state_string, (const char *)"REC_OFF");
        break;
    default:
        break;
    }

    printf("Reference monitor state is set to %s\n", state_string);

    return EXIT_SUCCESS;
}