#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "client.h"

int change_state(int rf_state, char *password)
{
    int ret = 0;
    char state_string[16];

    if (rf_state < 0 && rf_state > 3)
    {
        return EXIT_FAILURE;
    }

    ret = syscall(SWITCH_STATE, rf_state, password);
    if (ret == -1)
    {
        if (errno == EACCES)
        {
            printf("%s: Invalid Password\n", strerror(errno));
        }
        else
        {
            printf("%s: Unexpected error\n", strerror(errno));
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

/*
 * Thid function should print on screen the main menu of the client to let
 * the user select what to do.
 *
 * @return integer representing the reference monitor state
 */
int get_function_number()
{
    int rf_state, input_number;
    char input_buffer[1];

    rf_state = 3;

    system("clear");
    printf("=========================================================================\n");
    printf("=                   STACK1 REFERENCE MONITOR GUI                        =\n");
    printf("=                                                                       =\n");
    printf("= 1 - Change reference monitor state                                    =\n");
    printf("= 2 - Add a file or path to the blacklist                               =\n");
    printf("= 3 - Remove a file or path to the blacklist                            =\n");

    if (rf_state == 2 || rf_state == 3)
    {
        printf("= 4 - Add a file or path to the blacklist                               =\n");
        printf("= 5 - Remove a file or path to the blacklist                            =\n");
        printf("= 6 - Exit                                                              =\n");
    }
    else
    {
        printf("= 4 - Exit                                                              =\n");
    }
    printf("=                                                                       =\n");
    printf("=========================================================================\n");
    printf("\nChoose an option: ");

    scanf("%d", &input_number);

    if (input_number < 1 || input_number > 6)
    {
        printf("Error: Invalid choice for function selection\n");
        printf("Press any key\n");

        PRESS_ANY_KEY();
        input_number = -1;
    }

    if ((input_number == 4) && (rf_state == 0 || rf_state == 1))
    { // RF is NOT in reconfiguration state
        input_number = EXIT_VALUE;
    }
    else if ((input_number == 6) && (rf_state == 2 || rf_state == 3))
    { // RF is in reconfiguration state
        input_number = EXIT_VALUE;
    }

    return input_number;
}

int display_module_info()
{
    int discoverer_module, rf_module, fs_module;
    char input_string[256];
    char *command;
    int mod_number, return_value;

    discoverer_module = 0;
    fs_module = 0;
    rf_module = 0;
    return_value = 0;

    if (access("/sys/module/the_usctm", F_OK) == 0)
    {
        discoverer_module = 1;
    }

    if (access("/sys/module/singlefilefs", F_OK) == 0)
    {
        fs_module = 1;
    }

    if (access("/sys/module/the_stack_reference_monitor", F_OK) == 0)
    {
        rf_module = 1;
    }

    system("clear");
    printf("=========================================================================\n");
    printf("=                   STACK1 REFERENCE MONITOR GUI                        =\n");
    printf("=                                                                       =\n");
    if (discoverer_module)
    {
        printf("= MODULE 1: Systemcall Table Discoverer (the_utcsm) is ON               =\n");
    }
    else
    {
        printf("= MODULE 1: Systemcall Table Discoverer (the_utcsm) is OFF              =\n");
    }

    if (fs_module)
    {
        printf("= MODULE 2: Filesystem (singlefilefs) is ON                             =\n");
    }
    else
    {
        printf("= MODULE 2: Filesystem (singlefilefs) is OFF                            =\n");
    }

    if (rf_module)
    {
        printf("= MODULE 3: Reference Monitor (the_stack_reference_monitor) is ON       =\n");
    }
    else
    {
        printf("= MODULE 3: Reference Monitor (the_stack_reference_monitor) is OFF      =\n");
    }

    printf("=                                                                       =\n");
    printf("=========================================================================\n");

    printf("\nType install/remove <mod_number> to modify this setting\n");
    printf("Type exit to shut down the application\n");

    scanf("%1023[^\n]", input_string);

    command = strtok(input_string, (const char *)" ");

    if (strcmp(command, "exit") == 0)
    {
        return_value = EXIT_VALUE;
    }
    else if (strcmp(command, "install") != 0 && strcmp(command, "remove") != 0)
    {
        printf("Error: Invalid command, should be install or remove\n");
        printf("Press any key\n");
        PRESS_ANY_KEY();
        return_value = -1;
    }
    else
    {
        mod_number = strtol(strtok(NULL, (const char *)" "), NULL, 0);
        if (mod_number < 1 || mod_number > 3)
        {
            printf("Error: Invalid module number, should be 1,2 or 3\n");
            printf("Press any key\n");
            PRESS_ANY_KEY();
            return_value = -1;
        }
    }

    return return_value;
}

void exit_function()
{
    printf("\nApllication shutting down");
    fflush(stdout);
    sleep(1);
    printf(".");
    fflush(stdout);
    sleep(1);
    printf(".");
    fflush(stdout);
    sleep(1);
    printf(".");
    fflush(stdout);
    sleep(1);
    system("clear");
}

int main(int argc, char **argv)
{
    int ret;
    do
    {
        ret = display_module_info();
    } while (ret == -1);

    switch(ret){
        case 1:
            break;
        case 2:
            break;
        case 6:
            break;
        case EXIT_VALUE:
            exit_function();
            break;
        default:
            printf("Error: Unexpected error, application shutting down\n");
            return EXIT_FAILURE;
            break;
    }

    // Get a integer representing which function to call
    /*do
    {
        ret = get_function_number();

    } while (ret == -1);

    switch (ret)
    {
        case 1:
            break;
        case 2:
            break;
        case 6:
            break;
        case EXIT_VALUE:
            exit_function();
            break;
        default:
            printf("Error: Unexpected error, application shutting down\n");
            return EXIT_FAILURE;
            break;
    }*/

    return EXIT_SUCCESS;
}