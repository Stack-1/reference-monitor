#include <asm/atomic.h>
#include <linux/limits.h>



#define MODNAME "STACK REFERENCE MONITOR"
#define PASSW_LEN 32
#define AUDIT if(1)
#define DEBUG
#define LOG_FILE "/home/stack1/Desktop/reference-monitor/log-filesystem/mount/ref_monitor_log.txt"

// Definition of macros to map RF satte to numbers
#define RF_ON           0
#define RF_OFF          1
#define RF_REC_ON       2
#define RF_REC_OFF      3


#ifdef DEBUG 
#define CONDITIONAL if(0)
#else
#define CONDITIONAL if(1)
#endif


typedef struct blacklist_node{
        char *path;
        struct blacklist_node *next;
}blacklist_node;


/** @struct reference_monitor
 *  @brief Reference Monitor Basic Structure
 */
struct reference_monitor {
        int state;                              /**< The state can be one of the following: OFF (0), ON (1), REC-OFF (2), REC-ON (3)*/
        char *password;                         /**< Password for Reference Monitor reconfiguration */
        blacklist_node *blacklist_head;             /**< Files to be protected */
        spinlock_t lock;                        /**< Lock for synchronization */
        int blacklist_size;
};

int is_blacklisted(char *path);