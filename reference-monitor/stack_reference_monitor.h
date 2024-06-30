#include <asm/atomic.h>


#define MODNAME "STACK REFERENCE MONITOR"
#define PASSW_LEN 32
#define AUDIT if(1)

struct blacklist_entry {
        struct list_head list;
        char *path;
        char *filename;
        unsigned long inode_number;
};

struct blacklist_dir_entry {
        struct list_head list;
        char *path;
};


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
        struct list_head blacklist_dir;         /**< Directories to be protected */
        spinlock_t lock;                        /**< Lock for synchronization */
};
