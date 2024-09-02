#include <linux/version.h>


// Definition of macros to map RF syscalls to numbers
#define SWITCH_STATE            134
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0)
#define GET_RF_STATE            156
#define ADD_TO_BLACKLIST        174
#define REMOVE_FROM_BLACKLIST   177
#define PRINT_BLACKLIST         178
#define GET_BLACKLIST_SIZE      180
#else
#define GET_RF_STATE            174
#define ADD_TO_BLACKLIST        177
#define REMOVE_FROM_BLACKLIST   178
#define PRINT_BLACKLIST         180
#define GET_BLACKLIST_SIZE      181
#endif


// Definition of macros to map RF state to numbers
#define RF_ON           0
#define RF_OFF          1
#define RF_REC_ON       2
#define RF_REC_OFF      3

// Definition of exit value macro for GUI menu
#define EXIT_VALUE          9999
#define HOME_VALUE          8888
#define WRONG_EUID_VALUE    7777


#define PRESS_ANY_KEY() printf("Press ENTER to continue\n"); while(!getchar()){};  

// Macro to flush stdout
#define FFLUSH(stdout) while(getchar() != '\n')


#define MAX_PASS_LEN    32

/* Struct used to pass infos of the module to install/remove
 * 
 * @param mod_number is the nu,ber of the module selected (1,2,3)
 * @param command (install/remove)
 */ 
typedef struct mod_info{
    int mod_number;
    char command[16];
}mod_info;