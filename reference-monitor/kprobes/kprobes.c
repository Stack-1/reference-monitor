#include <linux/device.h>
#include <linux/kprobes.h>
#include <linux/errno.h>
#include <linux/slab.h>

#include "kprobes.h"
#include "../stack_reference_monitor.h"
#include "../utils/utils.h"
#include "../log/logger.h"

// kretprobes structs
struct kretprobe file_open;
struct kretprobe file_delete;
struct kretprobe file_create;
struct kretprobe file_mkdir;
struct kretprobe file_rename;
struct kretprobe file_rmdir;

// kretprobes array
struct kretprobe **kprobe_array;

static int ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    struct probe_data *probe_data = (struct probe_data *)ri->data;
    pr_err("%s", probe_data->error_message);

    regs->ax = -EACCES;
    write_on_log();

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

    path = (struct path *)regs->di;
    dentry = path->dentry;

    // flags is the fourth parameter
    file = (struct file *)regs->si;
    flags = file->f_flags;

    // Check if the file is opened WRITE-ONLY or READ-WRITE
    if (flags & O_WRONLY || flags & O_RDWR || flags & O_CREAT || flags & O_APPEND || flags & O_TRUNC)
    {
        full_path = get_path_from_dentry(dentry);
        if (is_blacklisted(full_path) == 1)
        {
            probe_data = (struct probe_data *)ri->data;
            sprintf(error_message, "%s: [ERROR] vfs open on file %s blocked\n", MODNAME, full_path);
            probe_data->error_message = kstrdup(error_message, GFP_KERNEL);
            return 0;
        }
    }

    return 1;
}

static int inode_unlink_entry_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    char error_message[256];
    struct probe_data *probe_data;
    struct path *dir;
    struct dentry *dentry;
    char *full_path;

    dir = (struct path *)regs->di;
    dentry = (struct dentry *)regs->si;

    full_path = get_path_from_dentry(dentry);

    if (is_blacklisted(full_path) == 1)
    {
        probe_data = (struct probe_data *)ri->data;
        sprintf(error_message, "%s: [ERROR] Unlink on dir %s blocked\n", MODNAME, full_path);
        probe_data->error_message = kstrdup(error_message, GFP_KERNEL);
        return 0;
    }

    return 1;
}

static int inode_create_entry_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    struct dentry *dentry;
    char *full_path;
    char error_message[256];
    struct probe_data *probe_data;

    dentry = (struct dentry *)regs->si;

    full_path = get_path_from_dentry(dentry);

    if (is_blacklisted(full_path) == 1)
    {
        probe_data = (struct probe_data *)ri->data;
        sprintf(error_message, "%s: [ERROR] Create on file %s blocked\n", MODNAME, full_path);
        probe_data->error_message = kstrdup(error_message, GFP_KERNEL);
        return 0;
    }

    return 1;
}

static int inode_mkdir_entry_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    struct dentry *dentry;
    struct dentry *parent_dentry;
    char *full_path;
    char error_message[256];
    struct probe_data *probe_data;

    dentry = (struct dentry *)regs->si;

    parent_dentry = dentry->d_parent;
    full_path = get_path_from_dentry(dentry);

    if (is_blacklisted(full_path) == 1)
    {
        probe_data = (struct probe_data *)ri->data;
        sprintf(error_message, "%s: [ERROR] Mkdir on file %s blocked\n", MODNAME, full_path);
        probe_data->error_message = kstrdup(error_message, GFP_KERNEL);
        return 0;
    }

    return 1;
}

static int inode_rename_entry_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    struct dentry *dentry;
    char *full_path;
    char error_message[256];
    struct probe_data *probe_data;

    dentry = (struct dentry *)regs->si;

    full_path = get_path_from_dentry(dentry);

    if (is_blacklisted(full_path) == 1)
    {
        probe_data = (struct probe_data *)ri->data;
        sprintf(error_message, "%s: [ERROR] Rename on file %s blocked\n", MODNAME, full_path);
        probe_data->error_message = kstrdup(error_message, GFP_KERNEL);
        return 0;
    }

    return 1;
}

static int inode_rmdir_entry_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    struct dentry *dentry;
    struct dentry *parent_dentry;
    char *full_path;
    char error_message[256];
    struct probe_data *probe_data;

    dentry = (struct dentry *)regs->si;

    parent_dentry = dentry->d_parent;
    full_path = get_path_from_dentry(dentry);


    if (is_blacklisted(full_path) == 1 || is_blacklisted(get_path_from_dentry(parent_dentry)) == 1)
    {
        probe_data = (struct probe_data *)ri->data;
        sprintf(error_message, "%s: [ERROR] Rmdir on file %s blocked\n", MODNAME, full_path);
        probe_data->error_message = kstrdup(error_message, GFP_KERNEL);
        return 0;
    }

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
    set_kretprobe(&file_delete, "security_inode_unlink", (kretprobe_handler_t)inode_unlink_entry_handler);
    set_kretprobe(&file_create, "security_inode_create", (kretprobe_handler_t)inode_create_entry_handler);
    set_kretprobe(&file_mkdir, "security_inode_mkdir", (kretprobe_handler_t)inode_mkdir_entry_handler);
    set_kretprobe(&file_rename, "security_inode_rename", (kretprobe_handler_t)inode_rename_entry_handler);
    set_kretprobe(&file_rmdir, "security_inode_rmdir", (kretprobe_handler_t)inode_rmdir_entry_handler);

    /* kretprobes array allocation */
    kprobe_array = kmalloc(NUM_KRETPROBES * sizeof(struct kretprobe *), GFP_KERNEL);
    if (kprobe_array == NULL)
    {
        pr_err("%s: [ERROR] Kmalloc allocation of kprobe array failed\n", MODNAME);
        return -ENOMEM;
    }

    kprobe_array[0] = &file_open;
    kprobe_array[1] = &file_delete;
    kprobe_array[2] = &file_create;
    kprobe_array[3] = &file_mkdir;
    kprobe_array[4] = &file_rename;
    kprobe_array[5] = &file_rmdir;

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
