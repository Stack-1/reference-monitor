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
 *        - reference_on     --> reference monitor is operative and protects all the blacklisted paths
 *        - reference_off    --> reference monitor is off
 *        - reference_rec_on --> reference monitor is on and it is possible to modify the blacklist using rott privilages
 *        - reference_rec_off--> reference monitor is off and it is possible to modify the blacklist using rott privilages
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

#include "lib/include/scth.h"
#include "stack_reference_monitor.h"
#include "utils/utils.h"
#include "utils/blacklist.h"

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
int switch_state_code;
int add_to_blacklist_code;
int remove_from_blacklist_code;
int print_blacklist_code;
int get_blacklist_size_code;

/* update RF state syscall */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0)
__SYSCALL_DEFINEx(2, _switch_rf_state, int, state, char *, password)
{
#else
asmlinkage long sys_switch_rf_state(int state, char *password)
{
#endif
    char *state_string;
    int ret;

    // Check if the monitor state is admissible
    ret = rf_state_check(&reference_monitor);
    if (ret != 0)
    {
        return ret;
    }

    // Check if the password is equal to the one stored in the reference monitor state
    ret = password_check(password, &reference_monitor);
    if (ret != 0)
    {
        return ret;
    }

    // Check if the user is running as root
    ret = euid_check(current_euid());
    if (ret != 0)
    {
        return ret;
    }

    spin_lock(&reference_monitor.lock);
    reference_monitor.state = state;
    spin_unlock(&reference_monitor.lock);

    AUDIT
    {
        // Debug to print the correct state of the RF as string
        state_string = kmalloc(sizeof(char) * 16, GFP_KERNEL); // GFP_KERNEL?
        if (!state_string)
        {
            pr_err("%s: [ERROR] Error in kmalloc allocation while reserving memory for RF satte string to debug in kernel space\n", MODNAME);
            return -ENOMEM;
        }
        switch (state)
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
        printk("%s: [INFO] Password check successful, state changed to %s\n", MODNAME, state_string);
    }

    return reference_monitor.state;
}

/* update state syscall */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0)
__SYSCALL_DEFINEx(2, _add_to_blacklist, char *, relative_path, char *, passowrd)
{
#else
asmlinkage long sys_add_to_blacklist(char *relative_path, char *password)
{
#endif
    int ret;

    // Check if the reference monitor is in reconfiguration state
    ret = is_rf_rec(&reference_monitor);
    if (ret != 0)
    {
        return ret;
    }

    // Check if the password is equal to the one stored in the reference monitor state
    ret = password_check(password, &reference_monitor);
    if (ret != 0)
    {
        return ret;
    }

    // Check if the user is running as root
    ret = euid_check(current_euid());
    if (ret != 0)
    {
        return ret;
    }

    // Try to add to blacklist a new path
    ret = add_to_blacklist(relative_path, &reference_monitor);
    if (ret != 0)
    {
        return ret;
    }

    return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0)
__SYSCALL_DEFINEx(2, _remove_from_blacklist, char *, relative_path, char *, passowrd)
{
#else
asmlinkage long sys_remove_from_blacklist(char *relative_path, char *password)
{
        int ret;

    // Check if the reference monitor is in reconfiguration state
    ret = is_rf_rec(&reference_monitor);
    if (ret != 0)
    {
        return ret;
    }

    // Check if the password is equal to the one stored in the reference monitor state
    ret = password_check(password, &reference_monitor);
    if (ret != 0)
    {
        return ret;
    }

    // Check if the user is running as root
    ret = euid_check(current_euid());
    if (ret != 0)
    {
        return ret;
    }

    // Try to add to blacklist a new path
    ret = remove_from_blacklist(relative_path, &reference_monitor);
    if (ret != 0)
    {
        return ret;
    }

    return 0;
    return 0;
#endif
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0)
__SYSCALL_DEFINEx(1, _get_blacklist_size, int, dummy)
#else
asmlinkage long sys_get_blacklist_size(void)
{
    return reference_monitor.blacklist_size;
#endif
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0)
__SYSCALL_DEFINEx(2, _print_blacklist, char *__user, files, char *__user, dirs, size_t, files_size, size_t, dirs_size)
{
#else
asmlinkage long sys_print_blacklist(void)
{
#endif
    blacklist_node *curr = reference_monitor.blacklist_head;
    if (curr == NULL)
    {
        printk("Blacklist is empty\n");
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
long sys_remove_from_blacklist = (unsigned long)__x64_sys_remove_from_blacklist;
long sys_print_blacklist = (unsigned long)__x64_sys_print_blacklist;
long sys_get_blacklist_size = (unsigned long)__x64_sys_get_blacklist_size;
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
    new_sys_call_array[2] = (unsigned long)sys_remove_from_blacklist;
    new_sys_call_array[3] = (unsigned long)sys_print_blacklist;
    new_sys_call_array[4] = (unsigned long)sys_get_blacklist_size;

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
    switch_state_code = restore[0];
    add_to_blacklist_code = restore[1];
    remove_from_blacklist_code = restore[2];
    print_blacklist_code = restore[3];
    get_blacklist_size_code = restore[4];

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
    reference_monitor.blacklist_size = 0;
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
