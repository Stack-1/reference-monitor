/*
 *
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option) any later
 * version.
 *
 * This module is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * @file sys_calls_mount.c
 * @brief This is a module that loads on the system call tables the following system calls
 *        to handle operations over the reference monitor:
 *        - reference_on     --> switch to on mode
 *        - reference_off    --> switch to off mode
 *        - reference_rec_on --> switch to rec_on mode
 *        - reference_rec_off--> switch to rec_off mode
 *
 * @author Staccone Simone
 *
 * @date June 08, 2024
 */

#define EXPORT_SYMTAB
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/version.h>
#include <linux/syscalls.h>
#include <linux/err.h>
#include <linux/unistd.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <linux/spinlock.h>
#include <linux/cred.h>
#include <linux/file.h>
#include <linux/limits.h>
#include <linux/module.h>

#include "lib/include/scth.h"
#include "stack_reference_monitor.h"
#include "utils.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Staccone Simone <simone.staccone@virgilio.it>");
MODULE_DESCRIPTION("STACK-REFERENCE-MONITOR");

/* reference monitor struct */
struct reference_monitor reference_monitor;

/* reference monitor password, to be checked when switching to states REC-ON and REC-OFF */
char password[PASSW_LEN];
module_param_string(password, password, PASSW_LEN, 0);

/* syscall table base address */
unsigned long syscalls_table_address = 0x0;
module_param(syscalls_table_address, ulong, 0660);

unsigned long the_ni_syscall;
unsigned long new_sys_call_array[] = {0x0, 0x0, 0x0, 0x0, 0x0};                  /* new syscalls addresses array */
#define HACKED_ENTRIES (int)(sizeof(new_sys_call_array) / sizeof(unsigned long)) /* number of entries to be hacked */
int restore[HACKED_ENTRIES] = {[0 ...(HACKED_ENTRIES - 1)] - 1};                 /* array of free entries on the syscall table */

/* syscall codes */
int switch_state;
int add_to_blacklist;
int remove_from_blacklist;
int print_blacklist;
int get_blacklist_size;

/* update RF state syscall */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0)
__SYSCALL_DEFINEx(2, _switch_rf_state, int, state, char *, password)
{
#else
asmlinkage long sys_switch_rf_state(int state, char *password)
{
#endif
    kuid_t euid;
    char *kernel_password;

    // Check if RF state is compliant with the possible ones
    if (state < 0 || state > 3)
    {
        pr_err("%s: [ERROR] Wrong state selected for reference monitor, admissible states are 0-on, 1-off, 2-rec_om, 3-rec_off\n", MODNAME);
        return -EINVAL;
    }

    // Get memory from slub allocator for RF password
    kernel_password = kmalloc(PASSW_LEN, GFP_KERNEL); // GFP_KERNEL?
    if (!kernel_password)
    {
        pr_err("%s: [ERROR] Error in kmalloc allocation while reserving memory for RF password kernel space\n", MODNAME);
        return -ENOMEM;
    }

    // Use Cross-Ring Data Move to copy password from user to kernel space
    if (copy_from_user(kernel_password, password, PASSW_LEN))
    {
        pr_err("%s: [ERROR] Error while copying password from user address space to kernel address space\n", MODNAME);
        goto end;
        return -EAGAIN;
    }

    euid = current_euid();

    // check EUID
    if (!uid_eq(euid, GLOBAL_ROOT_UID))
    {
        pr_err("%s: [INFO] Access denied: only root (EUID 0) can change the state\n", MODNAME);
        goto end;
        return -EPERM;
    }

    // if requested state is REC-ON or REC-OFF, check password

    if (strcmp(reference_monitor.password, encrypt_password(kernel_password)) != 0)
    {
        pr_err("%s: [INFO] Access denied: invalid password\n", MODNAME);
        goto end;
        return -EACCES;
    }
    else
    {
        spin_lock(&reference_monitor.lock);
        reference_monitor.state = state;
        spin_unlock(&reference_monitor.lock);
        AUDIT
        {
            printk("%s: [INFO] Password check successful, state changed to %d\n", MODNAME, state);
        }
    }

end:
    kfree(kernel_password);
    return reference_monitor.state;
}

/* update state syscall */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0)
__SYSCALL_DEFINEx(2, _add_to_blacklist, char *, relative_path)
{
#else
asmlinkage long sys_add_to_blacklist(char *relative_path)
{
#endif
    char *path, *kernel_rel_path;
    struct file *dir;
    int ret;
    blacklist_node *curr;
    blacklist_node *new;

    if (reference_monitor.state < 2)
    {
        printk(KERN_ERR "%s: [ERROR] The reference monitor is not in a reconfiguration state\n", MODNAME);
        return -EPERM;
    }

    kernel_rel_path = kmalloc(PATH_MAX, GFP_KERNEL);
    if (!kernel_rel_path)
    {
        pr_err("%s: [ERROR] Error in kmalloc allocation\n", MODNAME);
        return -ENOMEM;
    }

    ret = copy_from_user(kernel_rel_path, relative_path, PATH_MAX);
    if (ret == -1)
    {
        pr_err("%s: [ERROR] Error in copy_from_user (return value %d)\n", MODNAME, ret);
        kfree(kernel_rel_path);
        return -EAGAIN;
    }

    new = (blacklist_node *)kmalloc(sizeof(blacklist_node), GFP_KERNEL);
    if (!new)
    {
        pr_err("%s: [ERROR] Error in kmalloc allocation\n", MODNAME);
        return -ENOMEM;
    }

    new->path = kmalloc(PATH_MAX, GFP_KERNEL);

    strcpy(new->path, (const char *)kernel_rel_path);

    spin_lock(&reference_monitor.lock);

    curr = reference_monitor.blacklist_head;
    if (curr == NULL)
    {
        reference_monitor.blacklist_head = new;
    }
    else
    {
        while (curr->next != NULL)
        {
            if(strcmp(curr->path, new->path) == 0){
                spin_unlock(&reference_monitor.lock);
                return -EALREADY;
            }
                goto exist;
            curr = curr->next;
        }
        curr->next = new;
    }
    
    spin_unlock(&reference_monitor.lock);

    kfree(kernel_rel_path);

    return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0)
__SYSCALL_DEFINEx(4, _print_blacklist, char *__user, files, char *__user, dirs, size_t, files_size, size_t, dirs_size)
{
#else
asmlinkage long sys_print_blacklist(void)
{
#endif
    blacklist_node *curr = reference_monitor.blacklist_head;
    if (curr == NULL)
    {
        printk( "Blacklist is empty\n");
    }
    else
    {
        printk("%s: Blacklist elements:\n", MODNAME);
        while (curr->next != NULL)
        {
            printk("- %s\n", curr->path);
            curr = curr->next;
        }
        printk("- %s\n", curr->path);
    }

    return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0)
long sys_switch_rf_state = (unsigned long)__x64_sys_switch_rf_state;
long sys_add_to_blacklist = (unsigned long)__x64_sys_add_to_blacklist;
// long sys_remove_from_blacklist = (unsigned long)__x64_sys_remove_from_blacklist;
long sys_print_blacklist = (unsigned long)__x64_sys_print_blacklist;
// long sys_get_blacklist_size = (unsigned long)__x64_sys_get_blacklist_size;
#else
#endif

/**
 * @brief This function adds the new syscalls to the syscall table's free entries
 */
int initialize_syscalls(void)
{
    int i;
    int ret;

    if (syscalls_table_address == 0x0)
    {
        printk("%s: [ERROR] Cannot find the_ni_syscall", MODNAME);
    }

    AUDIT
    {
        printk("%s: [INFO] The syscall table base address is 9x%px\n", MODNAME, (void *)syscalls_table_address);
        printk("%s: [INFO] Initializing - hacked entries %d\n", MODNAME, HACKED_ENTRIES);
    }

    AUDIT
    {
        printk("%s: [DEBUG] New system call address points to 0x%p", MODNAME, (void *)new_sys_call_array[0]);
    }

    new_sys_call_array[0] = (unsigned long)sys_switch_rf_state;
    new_sys_call_array[1] = (unsigned long)sys_add_to_blacklist;
    new_sys_call_array[2] = (unsigned long)sys_print_blacklist;

    AUDIT
    {
        printk("%s: [DEBUG] New system call address points to 0x%p", MODNAME, (void *)new_sys_call_array[0]);
    }

    /* get free entries on the syscall table */
    ret = get_entries(restore, HACKED_ENTRIES, (unsigned long *)syscalls_table_address, &the_ni_syscall);

    if (ret != HACKED_ENTRIES)
    {
        printk("%s: [ERROR] Could not hack %d entries (just %d)\n", MODNAME, HACKED_ENTRIES, ret);
        return -ERANGE;
    }

    unprotect_memory();

    /* the free entries will point to the new syscalls */
    for (i = 0; i < HACKED_ENTRIES; i++)
    {
        ((unsigned long *)syscalls_table_address)[restore[i]] = (unsigned long)new_sys_call_array[i];
    }

    protect_memory();

    AUDIT
    {
        printk("%s: [DEBUG] Syscall table restore 0 is %d", MODNAME, restore[0]);
        printk("%s: [DEBUG] Syscall table entry point to 0x%p", MODNAME, (void *)((unsigned long *)syscalls_table_address)[restore[0]]);

        printk("%s: [INFO] All new system-calls correctly installed on sys-call table\n", MODNAME);
    }

    /* set syscall codes */
    switch_state = restore[0];
    add_to_blacklist = restore[1];
    print_blacklist = restore[2];
    remove_from_blacklist = restore[3];
    get_blacklist_size = restore[4];

    return 0;
}

int init_module(void)
{
    int ret;
    char *enc_password;

    printk("%s: ******************************************* \n", MODNAME);
    printk("%s: [INFO] Module correctly started\n", MODNAME);

    // Adding reference monitor systemcalls the the syscalss table
    printk("%s: [INFO] Number of entries to hack %d\n", MODNAME, HACKED_ENTRIES);

    ret = initialize_syscalls();

    if (ret != 0)
    {
        return ret;
    }

    printk("%s: [INFO] Setting RF initial state to ON\n", MODNAME);

    spin_lock(&reference_monitor.lock);
    reference_monitor.state = 0;
    reference_monitor.blacklist_head = NULL;
    spin_unlock(&reference_monitor.lock);

    printk("%s: [INFO] Encryipting password\n", MODNAME);
    enc_password = encrypt_password(password);
    reference_monitor.password = enc_password;

    printk("%s: [INFO] Password entrypted and set correctly\n", MODNAME);
    printk("%s: [INFO] Module correctly installed\n", MODNAME);
    return 0;
}

void cleanup_module(void)
{
    int i;
    blacklist_node *prev = reference_monitor.blacklist_head;
    blacklist_node *curr = reference_monitor.blacklist_head->next;

    AUDIT
    {
        printk("%s: [INFO] Shutting down reference monitor\n", MODNAME);
    }

    /* syscall table restoration */
    unprotect_memory();
    for (i = 0; i < HACKED_ENTRIES; i++)
    {
        ((unsigned long *)syscalls_table_address)[restore[i]] = the_ni_syscall;
    }
    protect_memory();

    AUDIT
    {
        printk("%s: [INFO] System call table restored to its original content\n", MODNAME);
        printk("%s: ******************************************* \n", MODNAME);
    }
}
