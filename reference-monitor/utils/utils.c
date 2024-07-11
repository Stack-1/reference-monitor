#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/namei.h>
#include <linux/crypto.h>
#include <linux/fdtable.h>
#include <linux/uaccess.h>
#include <linux/file.h>
#include <crypto/hash.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/module.h>

#include "../stack_reference_monitor.h"

/**
 * @brief Password encryption (SHA256)
 * @param password Password to be encrypted
 * @returns Encrypted password
 */
char *encrypt_password(const char *password)
{
        struct crypto_shash *hash_tfm;
        struct shash_desc *desc;
        unsigned char *digest;
        char *result = NULL;
        int ret = -ENOMEM;
        int i;

        /* hash transform allocation */
        hash_tfm = crypto_alloc_shash("sha256", 0, 0);
        if (IS_ERR(hash_tfm))
        {
                printk(KERN_ERR "Failed to allocate hash transform\n");
                return NULL;
        }

        /* hash descriptor allocation */
        desc = kmalloc(sizeof(struct shash_desc) + crypto_shash_descsize(hash_tfm), GFP_ATOMIC);
        if (!desc)
        {
                printk(KERN_ERR "Failed to allocate hash descriptor\n");
                goto out;
        }
        desc->tfm = hash_tfm;

        /* digest allocation */
        digest = kmalloc(32, GFP_ATOMIC);
        if (!digest)
        {
                printk(KERN_ERR "Failed to allocate hash buffer\n");
                goto out;
        }

        /* hash computation */
        ret = crypto_shash_digest(desc, password, strlen(password), digest);
        if (ret)
        {
                printk(KERN_ERR "Failed to calculate hash\n");
                goto out;
        }

        /* result allocation */
        result = kmalloc(2 * 32 + 1, GFP_ATOMIC);
        if (!result)
        {
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

int is_rf_rec(struct reference_monitor *rf)
{
        if (rf->state < 2)
        {
                printk(KERN_ERR "%s: [ERROR] The reference monitor is not in a reconfiguration state\n", MODNAME);
                return -EPERM;
        }

        return 0;
}

int password_check(char *password, struct reference_monitor *rf)
{
        char *kernel_password;
        int ret;

        kernel_password = kmalloc(PASSW_LEN, GFP_KERNEL);
        if (!kernel_password)
        {
                pr_err("%s: [ERROR] Error in kmalloc allocation for kernel password\n", MODNAME);
                return -ENOMEM;
        }


        // Use Cross-Ring Data Move to copy password from user to kernel space
        ret = copy_from_user(kernel_password, (void *)password, PASSW_LEN);
        if (ret)
        {
                pr_err("%s: [ERROR] Error while copying password from user address space to kernel address space\n", MODNAME);
                kfree(kernel_password);
                return -EAGAIN;
        }

        if (strcmp(rf->password, encrypt_password(kernel_password)) != 0)
        {
                pr_err("%s: [INFO] Access denied: invalid password\n", MODNAME);
                kfree(kernel_password);
                return -EACCES;
        }

        kfree(kernel_password);

        return 0;
}

int euid_check(kuid_t euid)
{
        // check EUID
        if (!uid_eq(euid, GLOBAL_ROOT_UID))
        {
                pr_err("%s: [INFO] Access denied: only root (EUID 0) can change the state,add or remove files on blacklist\n", MODNAME);
                return -EPERM;
        }
        return 0;
}

int rf_state_check(int state)
{
        // Check if RF state is compliant with the possible ones
        if (state < 0 || state > 3)
        {
                pr_err("%s: [ERROR] Wrong state selected for reference monitor, admissible states are 0-on, 1-off, 2-rec_om, 3-rec_off\n", MODNAME);
                return -EINVAL;
        }
        return 0;
}

char *get_path_from_dentry(struct dentry *dentry)
{

        char *buffer, *full_path, *ret;
        int len;

        buffer = (char *)__get_free_page(GFP_ATOMIC);
        if (!buffer)
                return NULL;

        ret = dentry_path_raw(dentry, buffer, PATH_MAX);
        if (IS_ERR(ret))
        {
                pr_err("dentry_path_raw failed: %li", PTR_ERR(ret));
                free_page((unsigned long)buffer);
                return NULL;
        }

        len = strlen(ret);

        full_path = kmalloc(len + 2, GFP_ATOMIC);
        if (!full_path)
        {
                pr_err("%s: error in kmalloc allocation (get_path_from_dentry)\n", MODNAME);
                return NULL;
        }

        strncpy(full_path, ret, len);
        full_path[len + 1] = '\0';

        free_page((unsigned long)buffer);
        return full_path;
}
