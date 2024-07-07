#include <linux/device.h>
#include <linux/kprobes.h>
#include <linux/errno.h>
#include <linux/slab.h>

#include "kprobes.h"
#include "../stack_reference_monitor.h"
#include "../utils/utils.h"

// kretprobes structs
struct kretprobe file_open;
struct kretprobe file_delete;

// kretprobes array
struct kretprobe **kprobe_array;

static int ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    struct probe_data *probe_data = (struct probe_data *)ri->data;
    pr_err("%s", probe_data->error_message);

    regs->ax = -EACCES;
    //  log_data();

    kfree(probe_data->error_message);

    return 0;
}

static int open_entry_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    char error_message[256];
    struct probe_data *probe_data;
    struct path *path;
    struct dentry *dentry;
    char *full_path;
    int flags;
    struct file *file;

    // x86-64 syscall calling convention: %rdi, %rsi, %rdx, %r10, %r8 and %r9.

    // int vfs_open(const struct path *path, struct file *file)
    //  path is the first parameter, it contains the dentry of the file.
    path = (struct path *)regs->di;
    dentry = path->dentry;
    // inode = dentry->d_inode;
    // pathname = dentry->d_name.name;

    // flags is the fourth parameter
    file = (struct file *)regs->si;
    flags = file->f_flags;

    // Check if the file is opened WRITE-ONLY or READ-WRITE
    if (flags & O_WRONLY || flags & O_RDWR || flags & O_CREAT || flags & O_APPEND || flags & O_TRUNC)
    {
        full_path = get_path_from_dentry(dentry);
        if (is_blacklisted(full_path) == 1)
        {
            printk("%s: [DEBUG] %s", MODNAME, full_path);
            probe_data = (struct probe_data *)ri->data;
            sprintf(error_message, "%s: [ERROR] vfs open on file %s blocked\n", MODNAME, full_path);
            probe_data->error_message = kstrdup(error_message, GFP_KERNEL);
            return 0;
        }
    }

    // printk("%s: [DEBUG] %d %d %x %x %x --- path %s\n", MODNAME, (int)access_flags == 32768, (int)access_flags, access_flags & (O_RDWR | O_CREAT | O_APPEND), access_flags & (O_RDWR | O_CREAT | O_TRUNC), access_flags & (O_WRONLY | O_CREAT | O_TRUNC), get_path_from_dentry(dentry));

    /*if (
        !(access_flags & (O_WRONLY | O_CREAT | O_TRUNC)) ||
        !(access_flags & (O_RDWR | O_CREAT | O_TRUNC)) ||
        !(access_flags & O_RDWR) ||
        !(access_flags & (O_RDWR | O_CREAT | O_APPEND)) ||
        !(access_flags & (O_WRONLY | O_CREAT | O_APPEND)))*/

    // if((int)access_flags != 32768) //Magic number for readonly

    // full_path = get_path_from_dentry(dentry);
    // printk("%s: [DEBUG] Full path %s %d \n", MODNAME, real_path, !(flags & O_RDWR) && !(flags & O_WRONLY) && !(flags & (O_CREAT | __O_TMPFILE | O_EXCL )));

    /*if(!(flags & O_RDWR) && !(flags & O_WRONLY) && !(flags & (O_CREAT | __O_TMPFILE | O_EXCL )))
    {

    }*/

    return 1;
}

static int may_delete_entry_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    struct dentry *dentry;
    char *full_path;
    char error_message[256];
    struct probe_data *probe_data;

    // dentry = (struct dentry *)regs->si;

    /*full_path = get_path_from_dentry(dentry);

    if (is_blacklisted(full_path) == 1)
    {
        printk("%s: [DEBUG] %s", MODNAME, full_path);
        probe_data = (struct probe_data *)ri->data;
        sprintf(error_message, "%s: [ERROR] vfs open on file %s blocked\n", MODNAME, full_path);
        probe_data->error_message = kstrdup(error_message, GFP_KERNEL);
        return 0;
    }*/

    return 1;
}

void set_kretprobe(struct kretprobe *krp, char *symbol_name, kretprobe_handler_t entry_handler)
{
    krp->kp.symbol_name = symbol_name;
    krp->kp.flags = KPROBE_FLAG_DISABLED; // set kretprobe as disable (initial state is OFF)
    krp->handler = (kretprobe_handler_t)ret_handler;
    krp->entry_handler = entry_handler;
    krp->maxactive = -1;
    krp->data_size = sizeof(struct probe_data);
}

void enable_kprobes()
{
    int i, ret;

    if (kprobe_array == NULL)
    {
        pr_err("%s: [ERROR] Probe array is null", MODNAME);
    }
    for (i = 0; i < NUM_KRETPROBES; i++)
    {
        ret = enable_kretprobe(kprobe_array[i]);
        if (ret == -1)
        {
            pr_err("%s: [INFO] Kretprobe enabling failed\n", MODNAME);
        }
    }

    AUDIT
    {
        printk("%s: [INFO] Kretprobes enabled\n", MODNAME);
    }
}

void disable_kprobes()
{
    int i, ret;

    for (i = 0; i < NUM_KRETPROBES; i++)
    {
        ret = disable_kretprobe(kprobe_array[i]);
        if (ret == -1)
        {
            pr_err("%s: [INFO] Kretprobe disabling failed\n", MODNAME);
        }
    }

    AUDIT
    {
        printk("%s: [INFO] Kretprobes disabled\n", MODNAME);
    }
}

int kretprobe_init()
{
    int ret;

    /* initialize all kretprobes */
    set_kretprobe(&file_open, "vfs_open", (kretprobe_handler_t)open_entry_handler);
    // set_kretprobe(&file_read, "security_inode_link", (kretprobe_handler_t)inode_link_entry_handler);
    // set_kretprobe(&file_write, "security_inode_symlink", (kretprobe_handler_t)inode_symlink_entry_handler);
    set_kretprobe(&file_delete, "may_delete", (kretprobe_handler_t)may_delete_entry_handler);
    // set_kretprobe(&file_create, "security_inode_create", (kretprobe_handler_t)indoe_create_entry_handler);
    // set_kretprobe(&file_mkdir, "security_inode_mkdir", (kretprobe_handler_t)inode_mkdir_entry_handler);

    /* kretprobes array allocation */
    kprobe_array = kmalloc(NUM_KRETPROBES * sizeof(struct kretprobe *), GFP_KERNEL);
    if (kprobe_array == NULL)
    {
        pr_err("%s: [ERROR] Kmalloc allocation of kprobe array failed\n", MODNAME);
        return -ENOMEM;
    }

    kprobe_array[0] = &file_open;
    kprobe_array[1] = &file_delete;

    ret = register_kretprobes(kprobe_array, NUM_KRETPROBES);
    if (ret != 0)
    {
        pr_err("%s: [ERROR] Kretprobes registration failed, returned %d\n", MODNAME, ret);
        return ret;
    }
    AUDIT
    {
        printk("%s: [INFO] Kretprobes correctly installed\n", MODNAME);
    }

    return 0;
}

void kretprobe_clean()
{
    unregister_kretprobes(kprobe_array, NUM_KRETPROBES);

    AUDIT
    {
        pr_info("%s: [INFO] Kretprobes correctly removed\n", MODNAME);
    }

    kfree(kprobe_array);
}
