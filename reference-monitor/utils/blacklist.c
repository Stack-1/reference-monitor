#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "../stack_reference_monitor.h"

int add_to_blacklist(char *path, struct reference_monitor *rf)
{
    char *kernel_rel_path;
    blacklist_node *curr;
    blacklist_node *new;
    int ret;

    kernel_rel_path = kmalloc(PATH_MAX, GFP_KERNEL);
    if (!kernel_rel_path)
    {
        pr_err("%s: [ERROR] Error in kmalloc allocation\n", MODNAME);
        return -ENOMEM;
    }

    ret = copy_from_user(kernel_rel_path, path, PATH_MAX);
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

    spin_lock(&rf->lock);

    curr = rf->blacklist_head;
    if (curr == NULL)
    {
        rf->blacklist_head = new;
    }
    else if (curr->next == NULL)
    {
        if (strcmp(curr->path, kernel_rel_path) == 0)
        {
            spin_unlock(&rf->lock);
            kfree(kernel_rel_path);
            return -EEXIST;
        }
        curr->next = new;
    }
    else
    {
        while (curr->next != NULL)
        {
            if (strcmp(curr->path, kernel_rel_path) == 0)
            {
                spin_unlock(&rf->lock);
                kfree(kernel_rel_path);
                return -EEXIST;
            }
            curr = curr->next;
        }

        if (strcmp(curr->path, kernel_rel_path) == 0)
        {
            spin_unlock(&rf->lock);
            kfree(kernel_rel_path);
            return -EEXIST;
        }
        curr->next = new;
    }

    rf->blacklist_size++;
    spin_unlock(&rf->lock);

    printk("%s: [INFO] Path %s added succesfully to blacklist", MODNAME, kernel_rel_path);

    kfree(kernel_rel_path);
    return 0;
}

int remove_from_blacklist(char *path, struct reference_monitor *rf)
{
    char *kernel_path;
    blacklist_node *curr;
    blacklist_node *prev;
    int ret;

    kernel_path = kmalloc(PATH_MAX, GFP_KERNEL);
    if (!kernel_path)
    {
        pr_err("%s: [ERROR] Error in kmalloc allocation\n", MODNAME);
        return -ENOMEM;
    }

    ret = copy_from_user(kernel_path, path, PATH_MAX);
    if (ret == -1)
    {
        pr_err("%s: [ERROR] Error in copy_from_user (return value %d)\n", MODNAME, ret);
        kfree(kernel_path);
        return -EAGAIN;
    }

    spin_lock(&rf->lock);

    curr = rf->blacklist_head;
    if (curr == NULL)
    {
        goto not_found;
    }
    else if (strcmp(curr->path, kernel_path) == 0)
    {
        rf->blacklist_head = curr->next;
        rf->blacklist_size--;
        goto found;
    }
    else
    {
        while (curr != NULL && strcmp(curr->path, kernel_path) != 0)
        {
            prev = curr;
            curr = curr->next;
        }

        if(curr == NULL){
            goto not_found;
        }

        // Unlink center node
        prev->next = curr->next;
        goto found;
    }

not_found:
    spin_unlock(&rf->lock);
    printk("%s: [INFO] Path %s not found in blacklist\n", MODNAME, kernel_path);
    return -EINVAL;

found:

    spin_unlock(&rf->lock);
    printk("%s: [INFO] Path %s removed succesfully from blacklist\n", MODNAME, kernel_path);
    return 0;
}
