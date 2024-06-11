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
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/kprobes.h>
#include <linux/mutex.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/vmalloc.h>
#include <linux/cred.h>
#include <asm/page.h>
#include <asm/cacheflush.h>
#include <asm/apic.h>
#include <asm/io.h>
#include <linux/syscalls.h>
#include <linux/err.h>
#include <linux/unistd.h>
#include <linux/spinlock.h>
#include <linux/namei.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/file.h>
#include <linux/limits.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/dcache.h>
#include <linux/fs_struct.h>
#include <linux/path.h>
#include <linux/proc_fs.h>

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
unsigned long new_sys_call_array[] = {0x0,0x0,0x0,0x0,0x0};                          /* new syscalls addresses array */
#define HACKED_ENTRIES (int)(sizeof(new_sys_call_array)/sizeof(unsigned long))       /* number of entries to be hacked */
int restore[HACKED_ENTRIES] = {[0 ... (HACKED_ENTRIES-1)] -1};                       /* array of free entries on the syscall table */



/* syscall codes */
int dummy_print;



/* update state syscall */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0) 
__SYSCALL_DEFINEx(1, _dummy_print, int, state){
#else
asmlinkage long sys_dummy_print(int state) {
#endif 

        printk("Syscall called, state is: %d\n",state);
        return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
        long sys_dummy_print = (unsigned long) __x64_sys_dummy_print;    
#else
#endif



/**
 * @brief This function adds the new syscalls to the syscall table's free entries
*/
int initialize_syscalls(void) {
        int i;
        int ret;

        if (syscalls_table_address == 0x0){
           printk("%s: [ERROR] Cannot find the_ni_syscall", MODNAME);
        }

	AUDIT{
	   printk("%s: [INFO] The syscall table base address is 9x%px\n",MODNAME,(void*)syscalls_table_address);
     	   printk("%s: [INFO] Initializing - hacked entries %d\n",MODNAME,HACKED_ENTRIES);
	}

        AUDIT{
                printk("%s: [DEBUG] New system call address points to 0x%p",MODNAME,(void *)new_sys_call_array[0]);
        }

        new_sys_call_array[0] = (unsigned long)sys_dummy_print;
        
        AUDIT{
                printk("%s: [DEBUG] New system call address points to 0x%p",MODNAME,(void *)new_sys_call_array[0]);
        }

        /* get free entries on the syscall table */
        ret = get_entries(restore,HACKED_ENTRIES,(unsigned long*)syscalls_table_address,&the_ni_syscall);

        if (ret != HACKED_ENTRIES){
                printk("%s: [ERROR] Could not hack %d entries (just %d)\n",MODNAME,HACKED_ENTRIES,ret); 
                return -1;      
        }


	unprotect_memory();

        /* the free entries will point to the new syscalls */
        for(i=0;i<HACKED_ENTRIES;i++){
                ((unsigned long *)syscalls_table_address)[restore[i]] = (unsigned long)new_sys_call_array[i];
        }

	protect_memory();

        AUDIT {
                printk("%s: [DEBUG] Syscall table restore 0 is %d", MODNAME, restore[0]);
                printk("%s: [DEBUG] Syscall table entry point to 0x%p",MODNAME, (void *)((unsigned long *)syscalls_table_address)[restore[0]]);

                printk("%s: [INFO] All new system-calls correctly installed on sys-call table\n",MODNAME);
        }

        /* set syscall codes */
        dummy_print = restore[0];

        return 0;
}


int init_module(void) {
        int ret;
        printk("%s: ******************************************* \n",MODNAME);
        printk("%s: [INFO] Module correctly started\n",MODNAME);
        
        /* Adding reference monitor systemcalls the the syscalss table */

        printk("%s: [INFO] Number of entries to hack %d\n",MODNAME,HACKED_ENTRIES);

        ret = initialize_syscalls();

        if (ret != 0) {
                return ret;
        }	

        // TODO: Stuff....


        printk("%s: [INFO] Module correctly installed\n",MODNAME);
        return 0;
}

void cleanup_module(void) {
        int i;

        AUDIT{
                printk("%s: [INFO] Shutting down reference monitor\n",MODNAME);
        }

        /* syscall table restoration */
	unprotect_memory();
        for(i=0;i<HACKED_ENTRIES;i++){
                ((unsigned long *)syscalls_table_address)[restore[i]] = the_ni_syscall;
        }
	protect_memory();

        AUDIT{
                printk("%s: [INFO] System call table restored to its original content\n",MODNAME);
                printk("%s: ******************************************* \n",MODNAME);        
        }

}
