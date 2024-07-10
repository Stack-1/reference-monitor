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

/*****Please include following header files*****/
// stdio.h
// stdlib.h
// string.h
/***********************************************/

#define uchar unsigned char
#define uint unsigned int

#define DBL_INT_ADD(a, b, c)      \
        if (a > 0xffffffff - (c)) \
                ++b;              \
        a += c;
#define ROTLEFT(a, b) (((a) << (b)) | ((a) >> (32 - (b))))
#define ROTRIGHT(a, b) (((a) >> (b)) | ((a) << (32 - (b))))

#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x, 2) ^ ROTRIGHT(x, 13) ^ ROTRIGHT(x, 22))
#define EP1(x) (ROTRIGHT(x, 6) ^ ROTRIGHT(x, 11) ^ ROTRIGHT(x, 25))
#define SIG0(x) (ROTRIGHT(x, 7) ^ ROTRIGHT(x, 18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x, 17) ^ ROTRIGHT(x, 19) ^ ((x) >> 10))

typedef struct
{
        uchar data[64];
        uint datalen;
        uint bitlen[2];
        uint state[8];
} SHA256_CTX;

uint k[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

void SHA256Transform(SHA256_CTX *ctx, uchar data[])
{
        uint a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];

        for (i = 0, j = 0; i < 16; ++i, j += 4)
                m[i] = (data[j] << 24) | (data[j + 1] << 16) | (data[j + 2] << 8) | (data[j + 3]);
        for (; i < 64; ++i)
                m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];

        a = ctx->state[0];
        b = ctx->state[1];
        c = ctx->state[2];
        d = ctx->state[3];
        e = ctx->state[4];
        f = ctx->state[5];
        g = ctx->state[6];
        h = ctx->state[7];

        for (i = 0; i < 64; ++i)
        {
                t1 = h + EP1(e) + CH(e, f, g) + k[i] + m[i];
                t2 = EP0(a) + MAJ(a, b, c);
                h = g;
                g = f;
                f = e;
                e = d + t1;
                d = c;
                c = b;
                b = a;
                a = t1 + t2;
        }

        ctx->state[0] += a;
        ctx->state[1] += b;
        ctx->state[2] += c;
        ctx->state[3] += d;
        ctx->state[4] += e;
        ctx->state[5] += f;
        ctx->state[6] += g;
        ctx->state[7] += h;
}

void SHA256Init(SHA256_CTX *ctx)
{
        ctx->datalen = 0;
        ctx->bitlen[0] = 0;
        ctx->bitlen[1] = 0;
        ctx->state[0] = 0x6a09e667;
        ctx->state[1] = 0xbb67ae85;
        ctx->state[2] = 0x3c6ef372;
        ctx->state[3] = 0xa54ff53a;
        ctx->state[4] = 0x510e527f;
        ctx->state[5] = 0x9b05688c;
        ctx->state[6] = 0x1f83d9ab;
        ctx->state[7] = 0x5be0cd19;
}

void SHA256Update(SHA256_CTX *ctx, uchar data[], uint len)
{
        uint i;
        for (i = 0; i < len; ++i)
        {
                ctx->data[ctx->datalen] = data[i];
                ctx->datalen++;
                if (ctx->datalen == 64)
                {
                        SHA256Transform(ctx, ctx->data);
                        DBL_INT_ADD(ctx->bitlen[0], ctx->bitlen[1], 512);
                        ctx->datalen = 0;
                }
        }
}

void SHA256Final(SHA256_CTX *ctx, uchar hash[])
{
        uint i = ctx->datalen;

        if (ctx->datalen < 56)
        {
                ctx->data[i++] = 0x80;
                while (i < 56)
                        ctx->data[i++] = 0x00;
        }
        else
        {
                ctx->data[i++] = 0x80;
                while (i < 64)
                        ctx->data[i++] = 0x00;
                SHA256Transform(ctx, ctx->data);
                memset(ctx->data, 0, 56);
        }

        DBL_INT_ADD(ctx->bitlen[0], ctx->bitlen[1], ctx->datalen * 8);
        ctx->data[63] = ctx->bitlen[0];
        ctx->data[62] = ctx->bitlen[0] >> 8;
        ctx->data[61] = ctx->bitlen[0] >> 16;
        ctx->data[60] = ctx->bitlen[0] >> 24;
        ctx->data[59] = ctx->bitlen[1];
        ctx->data[58] = ctx->bitlen[1] >> 8;
        ctx->data[57] = ctx->bitlen[1] >> 16;
        ctx->data[56] = ctx->bitlen[1] >> 24;
        SHA256Transform(ctx, ctx->data);

        for (i = 0; i < 4; ++i)
        {
                hash[i] = (ctx->state[0] >> (24 - i * 8)) & 0x000000ff;
                hash[i + 4] = (ctx->state[1] >> (24 - i * 8)) & 0x000000ff;
                hash[i + 8] = (ctx->state[2] >> (24 - i * 8)) & 0x000000ff;
                hash[i + 12] = (ctx->state[3] >> (24 - i * 8)) & 0x000000ff;
                hash[i + 16] = (ctx->state[4] >> (24 - i * 8)) & 0x000000ff;
                hash[i + 20] = (ctx->state[5] >> (24 - i * 8)) & 0x000000ff;
                hash[i + 24] = (ctx->state[6] >> (24 - i * 8)) & 0x000000ff;
                hash[i + 28] = (ctx->state[7] >> (24 - i * 8)) & 0x000000ff;
        }
}

char *SHA256(char *data)
{
        int strLen = strlen(data);
        SHA256_CTX ctx;
        unsigned char hash[32];
        int i;
        char *hashStr = kmalloc(65, GFP_KERNEL);
        char s[3];

        if (!hashStr)
        {
                pr_err("%s: [ERROR] Error in kmalloc allocation for kernel password\n", MODNAME);
                return "";
        }

        strcpy(hashStr, "");

        SHA256Init(&ctx);
        SHA256Update(&ctx, data, strLen);
        SHA256Final(&ctx, hash);

        for (i = 0; i < 32; i++)
        {
                sprintf(s, "%02x", hash[i]);
                strcat(hashStr, s);
        }

        return hashStr;
}

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

        /* fingerprint (hash) computation */
        // hash = calc_fingerprint(log_data->exe_path);
        hash = SHA256(log_data->exe_path);
        

        /* string to be written to the log */
        snprintf(row, 256, "%d, %d, %u, %u, %s, %s\n", log_data->tid, log_data->tgid,
                 log_data->uid, log_data->euid, log_data->exe_path, hash);

        printk("%s: [DEBUG] Row is %s", MODNAME, row);

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
        printk("%s: [DEBUG] CUID %u\n", MODNAME, current_uid().val);

        spin_lock(&def_work_lock);

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

        spin_unlock(&def_work_lock);
}