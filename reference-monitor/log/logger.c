#include <linux/slab.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/cred.h>
#include <linux/unistd.h>
#include <linux/dcache.h>
#include <linux/fs_struct.h>
#include <linux/init.h>
#include <linux/limits.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/errno.h>
#include <linux/uaccess.h>

#include "logger.h"
#include "../stack_reference_monitor.h"
#include "../utils/utils.h"

spinlock_t def_work_lock;


/**
 * Deferred work to be carried out at each invalid access (i.e., offending program file's hash computation and writing to the log)
 * @param data address of a packed_work element
 */
void deferred_work(unsigned long data)
{

        packed_work *the_work;
        struct log_data *log_data;
        char *hash;
        char row[256];
        struct file *file;
        ssize_t ret;

        the_work = container_of((void *)data, packed_work, the_work);
        log_data = the_work->log_data;

        hash = encrypt_password((const char *)log_data->exe_path);
        

        /* string to be written to the log */
        snprintf(row, 256, "%d, %d, %u, %u, %s, %s\n", log_data->tid, log_data->tgid,
                 log_data->uid, log_data->euid, log_data->exe_path, hash);


        file = filp_open(LOG_FILE, O_WRONLY, 0644);
        if (IS_ERR(file))
        {
                pr_err("Error in opening log file (maybe the VFS is not mounted): %ld\n", PTR_ERR(file));
                return;
        }

        ret = kernel_write(file, row, strlen(row), &file->f_pos);

        AUDIT
        {
                pr_info("%s: written %ld bytes on log\n", MODNAME, ret);
        }

        filp_close(file, NULL);

        kfree((void *)container_of((void *)data, packed_work, the_work));
}

/**
 * Collect TID, TGID, UID, EUID and the offending program's full path, and schedule the deferred work (fingerprint
 * computation and writing to log)
 */
void write_on_log(void)
{

        struct log_data *log_data;
        struct mm_struct *mm;
        struct dentry *exe_dentry;
        char *exe_path;
        packed_work *def_work;

        log_data = kmalloc(sizeof(struct log_data), GFP_KERNEL);
        if (!log_data)
        {
                pr_err("%s: [ERROR] Error in kmalloc allocation (log_info)\n", MODNAME);
                spin_unlock(&def_work_lock);
                return;
        }


        /* get path of the offending program */
        mm = current->mm;
        exe_dentry = mm->exe_file->f_path.dentry;
        exe_path = get_path_from_dentry(exe_dentry);

        log_data->exe_path = kstrdup(exe_path, GFP_ATOMIC);
        log_data->tid = current->pid;
        log_data->tgid = task_tgid_vnr(current);
        log_data->uid = current_uid().val;
        log_data->euid = current_euid().val;

        /* Schedule hash computation and writing on file in deferred work */
        def_work = kmalloc(sizeof(packed_work), GFP_ATOMIC);
        if (def_work == NULL)
        {
                pr_err("%s: tasklet buffer allocation failure\n", MODNAME);
                spin_unlock(&def_work_lock);
                return;
        }

        def_work->buffer = def_work;
        def_work->log_data = log_data;

        __INIT_WORK(&(def_work->the_work), (void *)deferred_work, (unsigned long)(&(def_work->the_work)));

        schedule_work(&def_work->the_work);

}