#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>
#include <termios.h>
#define ECHOFLAGS (ECHO | ECHOE | ECHOK | ECHONL)

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
            return HOME_VALUE;
        }
        else if(errno == EPERM)
        {
            printf("%s: Client should be in EUID root mode\n", strerror(errno));
            return WRONG_EUID_VALUE;
        }else{
            printf("Unexpected error: %s\n", strerror(errno));
        }
        return ret;
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

int get_rf_state()
{
    int ret = 0;
    char state_string[16];

    ret = syscall(GET_RF_STATE);
    if (ret == -1)
    {

        printf("%s: Unexpected error\n", strerror(errno));

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

    return ret;
}

int add_to_blacklist(char *path, char *password)
{
    int ret = 0;

    ret = syscall(ADD_TO_BLACKLIST, path, password);
    if (ret == -1)
    {

        printf("%s: Unexpected error\n", strerror(errno));

        return EXIT_FAILURE;
    }

    printf("%s succesfully added to blacklist\n", path);

    return ret;
}


int remove_from_blacklist(char *path, char *password)
{
    int ret = 0;

    ret = syscall(REMOVE_FROM_BLACKLIST, path, password);
    if (ret == -1)
    {

        printf("%s: Unexpected error\n", strerror(errno));

        return EXIT_FAILURE;
    }

    printf("%s succesfully added to blacklist\n", path);

    return ret;
}


void print_blacklist()
{
    int ret = 0;
    char *blacklist;
    int blacklist_size;

    ret = syscall(GET_BLACKLIST_SIZE);
    if (ret == -1)
    {

        printf("%s: Unexpected error\n", strerror(errno));

        return;
    }

    blacklist_size = ret;

    blacklist = (char *)malloc(sizeof(char) * blacklist_size * 4096);

    ret = syscall(PRINT_BLACKLIST, blacklist);
    if (ret == -1)
    {

        printf("%s: Unexpected error\n", strerror(errno));

        return;
    }

    printf("%s", blacklist);
    return;
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

    rf_state = get_rf_state();

    system("clear");
    printf("=========================================================================\n");
    printf("=                   STACK1 REFERENCE MONITOR GUI                        =\n");
    printf("=                                                                       =\n");
    printf("= 1 - Change reference monitor state                                    =\n");
    printf("= 2 - Get reference monitor state                                       =\n");

    if (rf_state == 2 || rf_state == 3)
    {
        printf("= 3 - Add a file or path to the blacklist                               =\n");
        printf("= 4 - Remove a file or path to the blacklist                            =\n");
        printf("= 5 - Print blacklist                                                   =\n");
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

    if (input_number < 1)
    {
        printf("Error: Invalid choice for function selection\n");

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

int display_module_info(mod_info *mod_info)
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
        printf("= MODULE 1: Systemcall Table Discoverer (the_usctm) is ON               =\n");
    }
    else
    {
        printf("= MODULE 1: Systemcall Table Discoverer (the_usctm) is OFF              =\n");
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

    printf("\nType \"install/remove <mod_number>\" to modify this setting\n");
    if (discoverer_module && fs_module && rf_module)
    {
        printf("Type \"home\" to interact with the system\n");
    }
    printf("Type \"exit\" to shut down the application\n");

    scanf(" %1023[^\n]", input_string);

    command = strtok(input_string, (const char *)" ");

    if (strcmp(command, "exit") == 0)
    {
        return_value = EXIT_VALUE;
    }
    else if (discoverer_module && fs_module && rf_module && !strcmp(command, "home"))
    {
        return_value = HOME_VALUE;
    }
    else if (strcmp(command, "install") != 0 && strcmp(command, "remove") != 0)
    {
        if (discoverer_module && fs_module && rf_module)
        {
            printf("Error: Invalid command %s, should be install/remove, exit or home\n", input_string);
        }
        else
        {
            printf("Error: Invalid command %s, should be install/remove or exit\n", input_string);
        }
        FFLUSH(stdout);
        PRESS_ANY_KEY();
        return_value = -1;
    }
    else
    {
        mod_number = strtol(strtok(NULL, (const char *)" "), NULL, 0);
        if (mod_number < 1 || mod_number > 3)
        {
            printf("Error: Invalid module number, should be 1,2 or 3\n");

            PRESS_ANY_KEY();
            return_value = -1;
        }
    }
    strncpy(mod_info->command, command, sizeof(command));
    mod_info->mod_number = mod_number;

    return return_value;
}

void exit_function()
{
    printf("\nApllication shutting down");
    fflush(stdout);
    sleep(1);
    printf(".");
    fflush(stdout);
    usleep(800000);
    printf(".");
    fflush(stdout);
    usleep(800000);
    printf(".");
    fflush(stdout);
    usleep(800000);
    system("clear");
}

int getpasswd(char *passwd, int size)
{
    int c;
    int n = 0;
    do
    {
        c = getchar();
        if (c != '\n' || c != '\r')
        {
            passwd[n++] = c;
        }
        else if (c == '\b')
        {
            passwd[n--] = 0;
        }
    } while (c != '\n' && c != '\r' && n < (size - 1));
    passwd[n] = '\0';
    return n;
}

int set_disp_mode(int fd, int option)
{
    int err;
    struct termios term;
    if (tcgetattr(fd, &term) == -1)
    {
        perror("Cannot get the attribution of the terminal\n");
        return 1;
    }
    if (option)
        term.c_lflag |= ECHOFLAGS;
    else
        term.c_lflag &= ~ECHOFLAGS;
    err = tcsetattr(fd, TCSAFLUSH, &term);
    if (err == -1 && err == EINTR)
    {
        perror("Cannot set the attribution of the terminal");
        return 1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    int ret;
    mod_info *mod_info;
    char *password;
    char path[4096];
    char psw[MAX_PASS_LEN];
    int passw_size;
    int rf_state;
    int single_char;

    password = (char *)malloc(sizeof(char) * MAX_PASS_LEN);

MODULE_INFO_DISPLAY:
    mod_info = (struct mod_info *)malloc(sizeof(struct mod_info));

    memset(mod_info->command, 0, sizeof(mod_info->command));

    do
    {
        ret = display_module_info(mod_info);

    } while (ret == -1);

    switch (ret)
    {
    case 0:
        if (strcmp(mod_info->command, "install") == 0)
        {
            if (mod_info->mod_number == 1)
            {
                system("cd ../syscall-table-discoverer/; sudo make");
            }
            else if (mod_info->mod_number == 2)
            {
                system("cd ../log-filesystem/; sudo make");
            }
            else if (mod_info->mod_number == 3)
            {
                system("cd ../reference-monitor/; sudo make");
            }
            else
            {
                printf("Error: Unexpected value for module number, application shutting down\n");
                return EXIT_FAILURE;
            }
        }
        else if (strcmp(mod_info->command, "remove") == 0)
        {
            if (mod_info->mod_number == 1)
            {
                system("sudo rmmod the_usctm");
            }
            else if (mod_info->mod_number == 2)
            {
                system("sudo rmmod singlefilefs");
            }
            else if (mod_info->mod_number == 3)
            {
                system("sudo rmmod the_stack_reference_monitor");
            }
            else
            {
                printf("Error: Unexpected value for module number, application shutting down\n");
                return EXIT_FAILURE;
            }
        }
        else
        {
            printf("Error: Unexpected value for command, application shutting down\n");
            fflush(stdout);
            PRESS_ANY_KEY();
            return EXIT_FAILURE;
        }

        FFLUSH(stdout);
        PRESS_ANY_KEY();
        goto MODULE_INFO_DISPLAY;
        break;
    case HOME_VALUE:
        goto HOME;
        break;
    case EXIT_VALUE:
        exit_function();
        return EXIT_SUCCESS;
        break;
    default:
        printf("Error: Unexpected error, application shutting down\n");
        FFLUSH(stdout);
        return EXIT_FAILURE;
        break;
    }

HOME:
    // Get a integer representing which function to call
    do
    {
        ret = get_function_number();

    } while (ret == -1);

    switch (ret)
    {
    case 1:
        printf("Insert reference monitor state you want to provide \n0 - OM\n1 - OFF\n2 - REC_ON\n3 - REC_OFF\n");
        scanf(" %d", &rf_state);
        if (rf_state < 0 || rf_state > 3)
        {
            printf("Error: Invalid state for reference monitor");
            fflush(stdout);
            PRESS_ANY_KEY();
            goto HOME;
        }
        printf("Insert reference monitor password\n");
        getchar();
        set_disp_mode(STDIN_FILENO, 0);
        passw_size = getpasswd(password, MAX_PASS_LEN);
        set_disp_mode(STDIN_FILENO, 1);
        memset(psw, 0, MAX_PASS_LEN);
        strncpy(psw, password, passw_size - 1);

        ret = change_state(rf_state, psw);


        if (ret != 0)
        {
            if(ret == WRONG_EUID_VALUE || ret == HOME_VALUE)
            {
                PRESS_ANY_KEY();
                goto HOME;
            }
            else 
            {
                printf("Error: Unexpected error in switch rf state syscall, system will shout down\n");
                PRESS_ANY_KEY();
                return EXIT_FAILURE;
            }
        }

        fflush(stdout);
        break;
    case 2:
        get_rf_state();
        FFLUSH(stdout);
        break;
    case 3:
        printf("Insert the full path of the file to add to blacklist\n");
        scanf("%s", path);

        printf("Insert reference monitor password\n");
        getchar();
        set_disp_mode(STDIN_FILENO, 0);
        passw_size = getpasswd(password, MAX_PASS_LEN);
        set_disp_mode(STDIN_FILENO, 1);
        add_to_blacklist(path, password);
        FFLUSH(stdout);
        break;
    case 4:
        printf("Insert the full path of the file to remove from blacklist\n");
        scanf("%s", path);

        printf("Insert reference monitor password\n");
        getchar();
        set_disp_mode(STDIN_FILENO, 0);
        passw_size = getpasswd(password, MAX_PASS_LEN);
        set_disp_mode(STDIN_FILENO, 1);
        remove_from_blacklist(path, password);
        FFLUSH(stdout);
        break;
    case 5:
        print_blacklist();
        FFLUSH(stdout);
        break;
    case EXIT_VALUE:
        FFLUSH(stdout);
        goto MODULE_INFO_DISPLAY;
        break;
    default:
        FFLUSH(stdout);
        printf("Error: No valid number as been selected\n");
        break;
    }

    PRESS_ANY_KEY();
    goto HOME;

    return EXIT_SUCCESS;
}