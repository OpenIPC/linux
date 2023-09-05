/*
* linux-rtos-adaptor.c- Sigmastar
*
* Copyright (C) 2018 Sigmastar Technology Corp.
* All rights reserved.
*
* Author: 
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/
#include <linux/module.h>
#include <linux/device.h>
#include <linux/major.h>
#include <linux/fs.h>
#include <linux/arm-smccc.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/semaphore.h>
#include <linux/atomic.h>
#include <asm/pgtable.h>
#include <asm/io.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>

#define  E_MI_MODULE_ID_MAX 28

#define __MI_DEVICE_CONNECT 0
#define __MI_DEVICE_DISCONNECT 1
#define __MI_DEVICE_QUERY 2
const unsigned int ALKAID_CALL_START =      0xff000000;
const unsigned int ALKAID_CALL_END   =      0xff000040;
const int ALKAID_RTKTRACE = 0;
//#define MMA_BASE (0x20000000ul+E_MMAP_ID_RTK_mma_heap_ADR)
//#define MMA_SIZE (0x0ul+E_MMAP_ID_RTK_mma_heap_LEN)
#define CTX_NUM 2
#define CTX_BASE 8
typedef struct {
    struct {
        union {
            void *curr_base;
            int pid;
        };
        long idx;
        unsigned long mma_base;
        unsigned int arg_size;
    };
    char __pad[64];
} linux_ctx __attribute__((aligned(64)));

/*
 * from ARM Architecture Reference Manual
 *                    ARMv7-A and ARMv7-R edition
 * B3.18.6 Cache maintenance operations, functional group, VMSA
 * Table B3-49 Cache and branch predictor maintenance operations, VMSA
 */
static void flush_cache_area(void *ptr, int len){
    const unsigned long cache_line_size = 64;
    unsigned long iter, end;
    iter = (unsigned long)ptr, end = (unsigned long)ptr + len;
    iter = iter/cache_line_size*cache_line_size;
    end = end/cache_line_size*cache_line_size;
    asm __volatile__("dsb st":::"memory"); /* data sync barrier for store */
    while(iter <= end){
        //asm __volatile__("mcr p15, 0, %0, c7, c11, 1"::"r"(iter):"memory"); /* DCCMVAC: flush to PoU (aka last level cache) */
        asm __volatile__("mcr p15, 0, %0, c7, c10, 1"::"r"(iter):"memory"); /* DCCMVAU: flush to PoC (aka main memory) */
        iter += cache_line_size;
    }
}
static void invalid_cache_area(void *ptr, int len){
    const unsigned long cache_line_size = 64;
    unsigned long iter, end;
    iter = (unsigned long)ptr, end = (unsigned long)ptr + len;
    iter = iter/cache_line_size*cache_line_size;
    end = end/cache_line_size*cache_line_size;
    while(iter <= end){
        asm __volatile__("mcr p15, 0, %0, c7, c6, 1"::"r"(iter):"memory"); /* DCIMVAC: invalidate to PoC */
        iter += cache_line_size;
    }
}
static void flush_and_invalid_cache_area(void *ptr, int len){
    const unsigned long cache_line_size = 64;
    unsigned long iter, end;
    iter = (unsigned long)ptr, end = (unsigned long)ptr + len;
    iter = iter/cache_line_size*cache_line_size;
    end = end/cache_line_size*cache_line_size;
    asm __volatile__("dsb st":::"memory"); /* data sync barrier for store */
    while(iter <= end){
        asm __volatile__("mcr p15, 0, %0, c7, c14, 1"::"r"(iter):"memory"); /* DCCIMVAC: flush & invalid to PoC (aka main memory) */
        iter += cache_line_size;
    }
}

static struct class *device_class;
static struct device *device_list[E_MI_MODULE_ID_MAX];
static int device_major;
static struct semaphore device_sem[CTX_NUM];
static struct semaphore ctx_sem;
static spinlock_t ctx_lock;
static DECLARE_BITMAP(ctx_bitmap, 32);
static atomic_t device_ref;
static struct resource *rtk_res;
static unsigned long mma_base = 0x25000000;
static unsigned long mma_size = 0x2800000;

static atomic_t ctx_cost[CTX_NUM][sizeof(unsigned long)*8] = {};
static atomic_t ctx_freq[CTX_NUM] = {};
static struct proc_dir_entry *debug_tools;
struct debug_tool {
    struct proc_dir_entry *entry;
    void *obj;
    ssize_t (*write)(void *obj, const char **args, int count);
    ssize_t (*read)(void *obj);
};
struct debug_tool_freq {
    struct debug_tool dt;
    int interval;
};
struct debug_tool_info {
    struct debug_tool dt;
    const char *version;
};
static ssize_t ctx_cost_erase(void *obj, const char **args, int count){
    atomic_t (*cost)[sizeof(unsigned long)*8] = ctx_cost;
    int i, j;
    for(i = 0; i < CTX_NUM; ++i){
        for(j = 0; j < sizeof(unsigned long)*8; ++j){
            atomic_set(&cost[i][j], 0);
        }
    }
    return 0;
}
static ssize_t ctx_cost_hist(void *obj){
    atomic_t (*cost)[sizeof(unsigned long)*8] = ctx_cost;
    int i, j;
    for(i = 0; i < CTX_NUM; ++i){
        for(j = 0; j < sizeof(unsigned long)*8; ++j){
            printk("CTX_%d|%02d:%d\n", i, j, atomic_read(&cost[i][j]));
        }
        printk("CTX--------------------\n");
    }
    return 0;
}
static ssize_t ctx_cost_freq_setup(void *obj, const char **args, int count){
    struct debug_tool_freq *dtf = obj;
    if(count == 1){
	 	if (kstrtoint((const char *)args[0], 0, &dtf->interval))
			return -EFAULT;
        printk("freq watch interval=%dms\n", dtf->interval);
        return count;
    }
    return -EINVAL;
}
static ssize_t ctx_cost_freq(void *obj){
    atomic_t *freq = ctx_freq;
    int i;
    for(i = 0; i < CTX_NUM; ++i){
        atomic_xchg(freq+i, 0);
    }
    while(schedule_timeout_interruptible(msecs_to_jiffies(200)) == 0){
        char buf[8*(CTX_NUM+1)] = {0};
        unsigned long rval;
        unsigned long tmp;
        tmp = atomic_xchg(freq, 0);
        rval = sprintf(buf, "%8lu", tmp);
        for(i = 1; i < CTX_NUM; ++i){
            tmp = atomic_xchg(freq+i, 0);
            rval+=sprintf(buf+rval, "|%8lu", tmp);
        }
        printk("CTX_FREQ:%s\n", buf);
    }
    return 0;
}

static ssize_t compile_version_info(void *obj){
    struct debug_tool_info *dti = obj;
    printk("version string:%s\n", dti->version);
    return 0;
}

static struct debug_tool syscall_cost_column = {
    .write = ctx_cost_erase,
    .read = ctx_cost_hist,
};
static struct debug_tool_freq syscall_freq_linear = {
    {
        .obj = &syscall_freq_linear,
        .write = ctx_cost_freq_setup,
        .read = ctx_cost_freq,
    }
};
static struct debug_tool_info info_tool = {
    {
        .obj = &info_tool,
        .read = compile_version_info,
    },
    .version = "version",
};

static unsigned int time_log2(ktime_t start, ktime_t end){
    unsigned int idx = 0;
    unsigned long us = ktime_to_us(ktime_sub(end, start));
    while(us){
        idx = idx + 1;
        us = us >> 1;
    }
    return idx;
}

void alkaid_registe_notify(int cpu, void *notify);
void *alkaid_unregiste_notify(int cpu);
static bool alkaid_notify(int idx){
    if(down_trylock(device_sem+idx-CTX_BASE) == 0){
        printk("bug found at %s %d, %d\n", __func__, __LINE__, idx);
        *(int*)0 = 0;
    }
    up(device_sem+idx-CTX_BASE);
    return true;
}
static int MI_DEVICE_Open(struct inode *inode, struct file *filp) {
    int id = iminor(inode);
    struct arm_smccc_res res;
    linux_ctx ctx = {.pid = current->pid, .idx = -1,};
    if(atomic_inc_return(&device_ref) == 1){
        alkaid_registe_notify(0, alkaid_notify);
        printk("register notify\n");
    }

    while(1) {
        if(down_timeout(&ctx_sem, HZ) == 0){
            if(CTX_NUM > 1){
                spin_lock(&ctx_lock);
                ctx.idx = find_first_zero_bit(ctx_bitmap, 32);
                if(ctx.idx < CTX_NUM) {
                    set_bit(ctx.idx, ctx_bitmap);
                }
                spin_unlock(&ctx_lock);
            } else {
                ctx.idx = 0;
            }
            if(ctx.idx < CTX_NUM) {
                extern void Chip_Flush_MIU_Pipe(void);
                flush_cache_area(&ctx, sizeof(ctx));
                Chip_Flush_MIU_Pipe();
                break;
            }
        } else {
            printk(KERN_WARNING "dead lock check at %s %d!\n", __func__, __LINE__);
        }
    }
    arm_smccc_smc(ALKAID_CALL_START+id, __pa((long)&ctx), -1, __MI_DEVICE_CONNECT,
                        0, 0, 0, 0, &res);
    while(1) {
        if (down_timeout(device_sem+ctx.idx, HZ) == 0) {
            arm_smccc_smc(ALKAID_CALL_START+id, ctx.idx, -1, __MI_DEVICE_QUERY,
                        0, 0, 0, 0, &res);
            if(res.a0 != -2) {
                break;
            } else {
                printk(KERN_ERR "bug found at %s %d\n", __func__, __LINE__);
                *(int*)0 = 0;
            }
        } else {
            printk(KERN_WARNING "dead lock check at %s %d!\n", __func__, __LINE__);
        }
    }
    clear_bit(ctx.idx, ctx_bitmap);
    up(&ctx_sem);
    /* map mma range */
    filp->private_data = (void*)vm_mmap(filp, 0, mma_size, PROT_READ|PROT_WRITE, MAP_SHARED, rtk_res->start);
    if((unsigned long)filp->private_data < PAGE_OFFSET){
        return res.a0;
    }
    return (long)filp->private_data;
}

static int MI_DEVICE_Release(struct inode *inode, struct file *filp) {
    int id = iminor(inode);
    struct arm_smccc_res res;
    linux_ctx ctx = {.pid = current->pid, .idx = -1,};
    while(1) {
        if(down_timeout(&ctx_sem, HZ) == 0){
            if(CTX_NUM > 1){
                spin_lock(&ctx_lock);
                ctx.idx = find_first_zero_bit(ctx_bitmap, 32);
                if(ctx.idx < CTX_NUM) {
                    set_bit(ctx.idx, ctx_bitmap);
                }
                spin_unlock(&ctx_lock);
            } else {
                ctx.idx = 0;
            }
            if(ctx.idx < CTX_NUM) {
                extern void Chip_Flush_MIU_Pipe(void);
                flush_cache_area(&ctx, sizeof(ctx));
                Chip_Flush_MIU_Pipe();
                break;
            }
        } else {
            printk(KERN_WARNING "dead lock check at %s %d!\n", __func__, __LINE__);
        }
    }
    arm_smccc_smc(ALKAID_CALL_START+id, __pa((long)&ctx), -1, __MI_DEVICE_DISCONNECT,
                        0, 0, 0, 0, &res);
    while(1) {
        if (down_timeout(device_sem+ctx.idx, HZ) == 0) {
            arm_smccc_smc(ALKAID_CALL_START+id, ctx.idx, -1, __MI_DEVICE_QUERY,
                        0, 0, 0, 0, &res);
            if(res.a0 != -2) {
                break;
            } else {
                printk(KERN_ERR "bug found at %s %d\n", __func__, __LINE__);
                *(int*)0 = 0;
            }
        } else {
            printk(KERN_WARNING "dead lock check at %s %d,id=%d!\n", __func__, __LINE__,id);
        }
    }
    clear_bit(ctx.idx, ctx_bitmap);
    up(&ctx_sem);
    if(atomic_dec_return(&device_ref) == 0){
        alkaid_unregiste_notify(0);
        printk("unregister notify\n");
    }
    return res.a0;
}

static unsigned long vir2phy(struct task_struct *curr, void *ptr){
    unsigned long addr = (unsigned long)ptr;
    pgd_t *pgd = pgd_offset(curr->mm,addr);
    pud_t *pud = pud_offset(pgd,addr);
    pmd_t *pmd = pmd_offset(pud,addr);
    pte_t *pte = pmd_page_vaddr(*pmd)+pte_index(addr);
    return __pfn_to_phys(pte_pfn(*pte))+(addr&~PAGE_MASK);
}

static long MI_DEVICE_Ioctl(struct file *filp, unsigned int cmd, unsigned long ptr) {
    int id = iminor(file_inode(filp));
    long rval = -EIO;
    if(_IOC_TYPE(cmd) == 'i') {
        struct arm_smccc_res res = {0};
        atomic_t (*cost)[sizeof(unsigned long)*8];
        atomic_t *freq;
        ktime_t t1, t2;
        t1 = ktime_get();
        if(ptr) {
            extern void Chip_Flush_MIU_Pipe(void);
            linux_ctx ctx = {.curr_base = filp->private_data, .idx = -1, .mma_base = mma_base};
            struct {
                int len;
                unsigned long long ptr;
            } tr;
            void *arg = NULL;
		 	if (copy_from_user((char *)&tr, (void*)ptr, sizeof(tr)))
				return -EFAULT;
            if(tr.len > _IOC_SIZE(cmd)) {
                printk(KERN_ERR "write cmd(0x%08x) overflow!", cmd);
                return -EINVAL;
            }

            if(tr.len > 4096) {
                printk(KERN_WARNING "write cmd(0x%08x) Send Big Data size(%d)!", cmd, tr.len);
            }

            if(_IOC_DIR(cmd) & _IOC_WRITE) {
                if(tr.len == 0) {
                    printk(KERN_ERR "write cmd(0x%08x) send null data!", cmd);
                    return -EINVAL;
                }
                arg = memdup_user((void*)(long)tr.ptr, tr.len);
                if(!arg)
                    return -ENOMEM;
                if(_IOC_DIR(cmd) & _IOC_READ) {
                    flush_and_invalid_cache_area(arg, tr.len);
                }else{
                    flush_cache_area(arg, tr.len);
                }
            } else if(_IOC_DIR(cmd) & _IOC_READ) {
                arg = kmalloc(tr.len+sizeof(long), GFP_KERNEL);
                if(!arg)
                    return -ENOMEM;
                invalid_cache_area(arg, tr.len);
            } else {
                printk(KERN_ERR "send a buffer to cmd(0x%08x) with_IOC_TYPE_NONE!\n", cmd);
                return -EINVAL;
            }
            ctx.arg_size = _IOC_SIZE(cmd);
            while(1) {
                if(down_timeout(&ctx_sem, HZ) == 0){
                    if(CTX_NUM > 1){
                        spin_lock(&ctx_lock);
                        ctx.idx = find_first_zero_bit(ctx_bitmap, 32);
                        if(ctx.idx < CTX_NUM) {
                            set_bit(ctx.idx, ctx_bitmap);
                        }
                        spin_unlock(&ctx_lock);
                    } else {
                        ctx.idx = 0;
                    }
                    if(ctx.idx < CTX_NUM) {
                        flush_cache_area(&ctx, sizeof(ctx));
                        break;
                    }
                } else {
                    printk(KERN_WARNING "dead lock check at %s %d!\n", __func__, __LINE__);
                }
            }
            Chip_Flush_MIU_Pipe();
            while(1) {
                arm_smccc_smc(ALKAID_CALL_START+id, __pa((long)&ctx), cmd, __pa((unsigned long)arg),
                                0, 0, 0, 0, &res);
                if (down_timeout(device_sem+ctx.idx, HZ) == 0) {
                    arm_smccc_smc(ALKAID_CALL_START+id, __pa((long)&ctx), cmd, __pa((unsigned long)arg),
                                0, 0, 0, 0, &res);
                    if(res.a0 != -2) {
                        break;
                    } else {
                        printk(KERN_ERR "bug found at %s %d\n", __func__, __LINE__);
                        *(int*)0 = 0;
                    }
                } else {
                    printk(KERN_WARNING "dead lock check at %s %d!\n", __func__, __LINE__);
                }
            }
            clear_bit(ctx.idx, ctx_bitmap);
            up(&ctx_sem);
            cost = ctx_cost+ctx.idx;
            freq = ctx_freq+ctx.idx;
            rval = res.a0;
            if(_IOC_DIR(cmd) & _IOC_READ) {
		 	if (copy_to_user((char*)(long)tr.ptr, arg, tr.len))
				return -EFAULT;
            }
            kfree(arg);
        } else {
            int ctxid;
            while(1) {
                if(down_timeout(&ctx_sem, HZ) == 0){
                    if(CTX_NUM > 1){
                        spin_lock(&ctx_lock);
                        ctxid = find_first_zero_bit(ctx_bitmap, 32);
                        if(ctxid < CTX_NUM) {
                            set_bit(ctxid, ctx_bitmap);
                        }
                        spin_unlock(&ctx_lock);
                        if(ctxid < CTX_NUM)
                            break;
                    } else {
                        ctxid = 0;
                        break;
                    }
                } else {
                    printk(KERN_WARNING "dead lock check at %s %d!\n", __func__, __LINE__);
                }
            }
            while(1) {
                arm_smccc_smc(ALKAID_CALL_START+id, ctxid, cmd, 0,
                                0, 0, 0, 0, &res);
                if (down_timeout(device_sem+ctxid, HZ) == 0) {
                    arm_smccc_smc(ALKAID_CALL_START+id, ctxid, cmd, 0,
                                0, 0, 0, 0, &res);
                    if(res.a0 != -2) {
                        break;
                    } else {
                        printk(KERN_ERR "bug found at %s %d\n", __func__, __LINE__);
                        *(int*)0 = 0;
                    }
                } else {
                    printk(KERN_WARNING "dead lock check at %s %d!\n", __func__, __LINE__);
                }
            }
            clear_bit(ctxid, ctx_bitmap);
            up(&ctx_sem);
            cost = ctx_cost+ctxid;
            freq = ctx_freq+ctxid;
            rval = res.a0;
        }
        t2 = ktime_get();
        atomic_inc(freq);
        atomic_inc(*cost+time_log2(t1, t2));
    }else{
        unsigned long *vir = filp->private_data+cmd;
        unsigned long uval;
        get_user(uval, vir);
        printk("uva:%p,phy:%lx,off=%x,uval=%lx\n", vir, vir2phy(current, vir), cmd, uval);
        rval = 0;
    }
    return rval;
}
static int MI_DEVICE_Mmap(struct file *file, struct vm_area_struct *vma) {
    static const struct vm_operations_struct vma_ops = {};
    size_t size = vma->vm_end - vma->vm_start;

    vma->vm_page_prot = phys_mem_access_prot(file, vma->vm_pgoff,
                        size,
                        vma->vm_page_prot);

    vma->vm_ops = &vma_ops;

    /* Remap-pfn-range will mark the range VM_IO */
    if (remap_pfn_range(vma,
                        vma->vm_start,
                        vma->vm_pgoff,
                        size,
                        vma->vm_page_prot)) {
        return -EAGAIN;
    }
    return 0;
}
static const struct file_operations fops = {
    .owner      = THIS_MODULE,
    .open       = MI_DEVICE_Open,
    .release    = MI_DEVICE_Release,
    .unlocked_ioctl = MI_DEVICE_Ioctl,
    .mmap       = MI_DEVICE_Mmap,
    .llseek     = noop_llseek,
};
module_param(mma_base,ulong,0644);
module_param(mma_size,ulong,0644);
uint RTK_CALL = 0x800;
module_param(RTK_CALL,uint,0444);
static unsigned long rtk_base(void){
    struct arm_smccc_res res;
    arm_smccc_smc(RTK_CALL, 0, 0, 0,
                  0, 0, 0, 0, &res);
    return res.a0;
}

static ssize_t debug_tool_read(struct seq_file* q, void* v)
{
    struct debug_tool *dt = q->private;
    if(dt->read)
        return dt->read(dt->obj);
    return -EIO;
}

static ssize_t debug_tool_write(struct file* file, const char __user* user_buf, size_t count, loff_t* ppos)
{
    const char *args[32] = {NULL};
    int c = 0;
    char *p, tc, *kbuf;

    struct seq_file *q = file->private_data;
    struct debug_tool *dt = q->private;

    if(!dt->write)
        return -EIO;

    kbuf = memdup_user(user_buf, count);
    if(!kbuf)
        return -ENOMEM;

    for(p = kbuf, tc = '\0'; tc != '\n' && (c < 32); ++p){
        p += strspn(p, " \t\r\f\v");
        if(*p == '\n')
            break;
        args[c++] = p;
        p += strcspn(p, " \t\n\r\f\v");
        tc = *p;
        *p = '\0';
    }

    if(c < 32) {
        dt->write(dt->obj,args,c);
        kfree(kbuf);
        return count;
    }
    kfree(kbuf);
    return -EINVAL;
}

static int debug_tool_open(struct inode *inode, struct file *file)
{
    return single_open(file, debug_tool_read, PDE_DATA(inode));
}

static const struct file_operations debug_tool_ops = {
    .owner      = THIS_MODULE,
    .open       = debug_tool_open,
    .read       = seq_read,
    .write      = debug_tool_write,
    .llseek     = seq_lseek,
    .release    = single_release,
};

static bool debug_tool_create(const char *name, struct debug_tool *dt){
    dt->entry = proc_create_data(name, 0640, debug_tools, &debug_tool_ops, dt);
    if (!dt->entry)
    {
        printk(KERN_ERR "failed  to  create  procfs  file  %s.\n",name);
        return false;
    }
    return true;
}

static void debug_tool_delete(struct debug_tool *dt){
    proc_remove(dt->entry);
}

static int __init linux_adaptor_init(void) {
    const char *dev_list[] = {
        "ive",    /* 0 */
        "vdf",    /* 1 */
        "venc",   /* 2 */
        "rgn",    /* 3 */
        "ai",     /* 4 */
        "ao",     /* 5 */
        "vif",    /* 6 */
        "vpe",    /* 7 */
        "vdec",   /* 8 */
        "sys",    /* 9 */
        "fb",     /* 10 */
        "hdmi",   /* 11 */
        "divp",   /* 12 */
        "gfx",    /* 13 */
        "vdisp",  /* 14 */
        "disp",   /* 15 */
        "os",     /* 16 */
        "iae",    /* 17 */
        "md",     /* 18 */
        "od",     /* 19 */
        "shadow", /* 20 */
        "warp",   /* 21 */
        "uac",    /* 22 */
        "ldc",    /* 23 */
        "sd",     /* 24 */
        "panel",  /* 25 */
        "cipher", /* 26 */
        "sensor",    /* 27 */
    };
    int err = 0, i;
    if(E_MI_MODULE_ID_MAX < sizeof(dev_list)/sizeof(*dev_list)){
        return -EINVAL;
    }
    device_class = class_create(THIS_MODULE, "rtos-adaptor");
    err = PTR_ERR(device_class);
    if (IS_ERR(device_class))
        goto fail_class_create;

    err = -EIO;
    device_major = register_chrdev(0, "rtos-adaptor", &fops);
    if(device_major <= 0)
        goto fail_register_chrdev;

    for(i = 0; i < E_MI_MODULE_ID_MAX; ++i)
        if(i < sizeof(dev_list)/sizeof(*dev_list)){
            device_list[i] = device_create(device_class,
                                    NULL,
                                    MKDEV(device_major, i),
                                    device_list+i,
                                    "mi_%s",
                                    dev_list[i]);
        }else{
            device_list[i] = NULL;
        }

    for(i = 0; i < CTX_NUM; ++i)
        sema_init(device_sem+i, 0);
    sema_init(&ctx_sem, CTX_NUM);
    atomic_set(&device_ref, 0);
    spin_lock_init(&ctx_lock);
    for(i = 0; i < 32; ++i)
        clear_bit(i, ctx_bitmap);
    rtk_res = request_mem_region(mma_base, mma_size, "mma");
    if(!rtk_res){
        goto fail_mem_req;
    }
    printk("map req:(0x%x,0x%x)|0x%lx\n", rtk_res->start, resource_size(rtk_res), rtk_base());
    debug_tools = proc_mkdir("adaptor-debug-tools", NULL);
    if(!debug_tools)
        goto fail_create_debug_tools;

    if(!debug_tool_create("syscall_cost", &syscall_cost_column))
        goto fail_create_syscall_cost;

    if(!debug_tool_create("syscall_freq", &syscall_freq_linear.dt))
        goto fail_create_syscall_freq;

    if(!debug_tool_create("info_tool", &info_tool.dt))
        goto fail_create_info_tool;

    printk("linux-adaptor init success!(%s)\n", __TIME__);
    return 0;

fail_create_info_tool:
    printk(KERN_ERR "create info tool failed!\n");
    debug_tool_delete(&syscall_freq_linear.dt);
fail_create_syscall_freq:
    printk(KERN_ERR "create syscall freq analyzer failed!\n");
    debug_tool_delete(&syscall_cost_column);
fail_create_syscall_cost:
    printk(KERN_ERR "create syscall cost analyzer failed!\n");
    proc_remove(debug_tools);
fail_create_debug_tools:
    printk(KERN_ERR "proc mkdir failed!\n");
    release_mem_region(rtk_res->start, mma_size);
fail_mem_req:
    printk(KERN_ERR "request mem failed\n");
    for(i = 0; i < E_MI_MODULE_ID_MAX; ++i){
        if(device_list[i])
            device_destroy(device_class, MKDEV(device_major, i));
    }
fail_register_chrdev:
    printk(KERN_ERR "unable to get mi device\n");
    class_destroy(device_class);
fail_class_create:
    printk(KERN_ERR "fail create class\n");
    return err;
}
module_init(linux_adaptor_init)

static void __exit linux_adaptor_exit(void) {
    int i;
    debug_tool_delete(&info_tool.dt);
    debug_tool_delete(&syscall_freq_linear.dt);
    debug_tool_delete(&syscall_cost_column);
    proc_remove(debug_tools);
    release_mem_region(rtk_res->start, mma_size);
    for(i = 0; i < E_MI_MODULE_ID_MAX; ++i){
        if(device_list[i])
            device_destroy(device_class, MKDEV(device_major, i));
    }
    unregister_chrdev(device_major, "rtos-adaptor");
    class_destroy(device_class);
}
module_exit(linux_adaptor_exit)

MODULE_LICENSE("GPL v2");
