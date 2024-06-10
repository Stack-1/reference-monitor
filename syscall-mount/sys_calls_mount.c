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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/spinlock.h>
#include <linux/unistd.h>
#include <linux/version.h>
#include <linux/crypto.h>  
#include <crypto/hash.h>

#include "sys_calls_mount.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Staccone Simone <simone.staccone@virgilio.it>");
MODULE_DESCRIPTION("SYS-CALLS-MOUNT");


/* reference monitor struct */
struct reference_monitor reference_monitor;    


/* reference monitor password, to be checked when switching to states REC-ON and REC-OFF */
char password[PASSW_LEN];
module_param_string(password, password, PASSW_LEN, 0);

/* syscall table base address */
unsigned long the_syscall_table = 0x0;
module_param(the_syscall_table, ulong, 0660);


/**
 * @brief Password encryption (SHA256)
 * @param password Password to be encrypted
 * @returns Encrypted password
 */
char *encrypt_password(const char *password) {  
        struct crypto_shash *hash_tfm;
        struct shash_desc *desc;
        unsigned char *digest;
        char *result = NULL;
        int ret = -ENOMEM;
	int i;

        /* hash transform allocation */
        hash_tfm = crypto_alloc_shash("sha256", 0, 0);
        if (IS_ERR(hash_tfm)) {
                printk(KERN_ERR "Failed to allocate hash transform\n");
                return NULL;
        }

        /* hash descriptor allocation */
        desc = kmalloc(sizeof(struct shash_desc) + crypto_shash_descsize(hash_tfm), GFP_ATOMIC);
        if (!desc) {
                printk(KERN_ERR "Failed to allocate hash descriptor\n");
                goto out;
        }
        desc->tfm = hash_tfm;

        /* digest allocation */
        digest = kmalloc(32, GFP_ATOMIC);
        if (!digest) {
                printk(KERN_ERR "Failed to allocate hash buffer\n");
                goto out;
        }

        /* hash computation */
        ret = crypto_shash_digest(desc, password, strlen(password), digest);
        if (ret) {
                printk(KERN_ERR "Failed to calculate hash\n");
                goto out;
        }

        /* result allocation */
        result = kmalloc(2 * 32 + 1, GFP_ATOMIC);
        if (!result) {
                printk(KERN_ERR "Failed to allocate memory for result\n");
                goto out;
        }

        /* printing result */
        for (i = 0; i < 32; i++)
                sprintf(&result[i * 2], "%02x", digest[i]);
        
out:
        if (digest)
                kfree(digest);
        if (desc)
                kfree(desc);
        if (hash_tfm)
                crypto_free_shash(hash_tfm);


        return result;
}





/* update state syscall */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0) 
__SYSCALL_DEFINEx(2, _write_rf_state, int, state, char*, password) {
#else
asmlinkage long sys_write_rf_state(int state, char *password) {
#endif 

        kuid_t euid;
	    int i, ret;
        char *kernel_password;

        /* check state number */
        if (state < 0 || state > 3) {
                pr_err("%s: Unexpected state", MODNAME);
                return -EINVAL;
        }

        kernel_password = kmalloc(PASSW_LEN, GFP_ATOMIC);
        if (!kernel_password) {
                pr_err("%s: Error in kmalloc allocation\n", MODNAME);
                return -ENOMEM;
        }

        if (copy_from_user(kernel_password, password, PASSW_LEN)) {
                pr_err("%s: error in copy_from_user\n", MODNAME);
                kfree(kernel_password);
                return -EAGAIN;
        }

        euid = current_euid();

        /* check EUID */
        if (!uid_eq(euid, GLOBAL_ROOT_UID)) {
                pr_err("%s: Access denied: only root (EUID 0) can change the state\n", MODNAME);
                kfree(kernel_password);
                return -EPERM;
        }   

        /* if requested state is REC-ON or REC-OFF, check password */
        if (state > 1) {
                if (strcmp(reference_monitor.password, encrypt_password(kernel_password)) != 0) {
                        pr_err("%s: Access denied: invalid password\n", MODNAME);
                        kfree(kernel_password);
                        return -EACCES;
                }
        }

        spin_lock(&reference_monitor.lock);

        /* state update */
        AUDIT {
        printk("%s: password check successful, changing the state to %d\n", MODNAME, state);
        }
        reference_monitor.state = state;

        spin_unlock(&reference_monitor.lock);

        /* enable/disable monitor */
        if (state == 1 || state == 3) {
                for (i = 0; i < NUM_KRETPROBES; i++) {
                        //ret = enable_kretprobe(rps[i]);
                        if (ret) {
                                pr_err("%s: kretprobe enabling failed\n", MODNAME);
                        }
                }
                AUDIT {
                pr_info("%s: kretprobes enabled\n", MODNAME);
                }
        } else {
                for (i = 0; i < NUM_KRETPROBES; i++) {
                        //ret = disable_kretprobe(rps[i]);
                        if (ret) {
                                pr_err("%s: kretprobe disabling failed\n", MODNAME);
                        }
                }
                AUDIT {
                pr_info("%s: kretprobes disabled\n", MODNAME);
                }
        }
        
        kfree(kernel_password);
        return reference_monitor.state;
        
}



int init_module(void) {
    printk("%s: module correctly started\n",MODNAME); 
	
    // TODO: Stuff....


    printk("%s: module correctly installed\n",MODNAME);
    return 0;
}

void cleanup_module(void) {
    printk("%s: shutting down\n",MODNAME);   
}
