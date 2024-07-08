#include <linux/workqueue.h>

struct log_data {
        int tid;
        int tgid;
        unsigned int uid;
        unsigned int euid;
        char *exe_path;
        char *hash;
};



typedef struct _packed_work{
        void* buffer;
        struct log_data *log_data;
        struct work_struct the_work;
} packed_work;

void write_on_log(void);