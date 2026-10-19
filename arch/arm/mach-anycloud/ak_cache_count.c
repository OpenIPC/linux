/*
* l1 cache counting  Driver
*
* Copyright anyka
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/stddef.h>
#include <linux/proc_fs.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/of_address.h>

#include <asm/uaccess.h>
#include <asm/io.h>


static void __iomem *dram_base;
static struct proc_dir_entry *ak_cache_dir;
static struct proc_dir_entry *ak_cache_enable_proc;
static struct proc_dir_entry *ak_icache_hit_proc;
static struct proc_dir_entry *ak_icache_miss_proc;
static struct proc_dir_entry *ak_dcache_hit_proc;
static struct proc_dir_entry *ak_dcache_miss_proc;

/*get cache enable status*/
static int ak_cache_enable_proc_show(struct seq_file *m, void *v)
{
    u32 reg;

    reg = __raw_readl(dram_base + 0xbc);
    seq_printf(m, "%s\n", (reg & 1)? "disabled": "enabled");
    return 0;
}

static int ak_cache_enable_proc_open(struct inode *inode, struct file *filp)
{
    return single_open(filp, ak_cache_enable_proc_show, PDE_DATA(inode));
}

/*set cache enable*/
static ssize_t ak_cache_enable_proc_write(struct file *filp,
                const char __user *buf, size_t count, loff_t *f_pos)
{
    char tmp[1];
    u32 reg;

    if (count < 1)
        return count;

    if (copy_from_user(tmp, buf, 1)) {
        return -EFAULT;
    }

    /* enable l1 cache hit/miss counting */
    reg = __raw_readl(dram_base + 0xbc);
    reg &= ~1;
    reg |= (tmp[0] == '0')? 1: 0;
    __raw_writel(reg, dram_base + 0xbc);

    return count;
}

/* /proc/cache/cache_enable  */
static const struct file_operations ak_cache_enable_proc_ops = {
    .open       = ak_cache_enable_proc_open,
    .read       = seq_read,
    .llseek     = seq_lseek,
    .release    = seq_release,
    .write      = ak_cache_enable_proc_write,
};

static int ak_icache_hit_proc_show(struct seq_file *m, void *v)
{
    u32 reg;

    /*read register value to get I-cache hit times*/
    reg = __raw_readl(dram_base + 0xbc);
    if ((reg & 1) == 0) {
        reg = __raw_readl(dram_base + 0xac);
        seq_printf(m, "%u\n", reg);
    } else {
        seq_printf(m, "0\n");
    }
    return 0;
}

static int ak_icache_hit_proc_open(struct inode *inode, struct file *filp)
{
    return single_open(filp, ak_icache_hit_proc_show, PDE_DATA(inode));
}

/* /proc/cache/icache_hit */
static const struct file_operations ak_icache_hit_proc_ops = {
    .open       = ak_icache_hit_proc_open,
    .read       = seq_read,
    .llseek     = seq_lseek,
    .release    = seq_release,
};

static int ak_icache_miss_proc_show(struct seq_file *m, void *v)
{
    u32 reg;

    /*read register value to get I-cache miss times*/
    reg = __raw_readl(dram_base + 0xbc);
    if ((reg & 1) == 0) {
        reg = __raw_readl(dram_base + 0xb0);
        seq_printf(m, "%u\n", reg);
    } else {
        seq_printf(m, "0\n");
    }
    return 0;
}

static int ak_icache_miss_proc_open(struct inode *inode, struct file *filp)
{
    return single_open(filp, ak_icache_miss_proc_show, PDE_DATA(inode));
}

/* /proc/cache/icache_miss */
static const struct file_operations ak_icache_miss_proc_ops = {
    .open       = ak_icache_miss_proc_open,
    .read       = seq_read,
    .llseek     = seq_lseek,
    .release    = seq_release,
};

/**
*
*@brief: ak_dcache_hit_proc_show
*@param[in] struct seq_file *m
*@param[in] void *v
*@return: int
*
**/
static int ak_dcache_hit_proc_show(struct seq_file *m, void *v)
{
    u32 reg;

    reg = __raw_readl(dram_base + 0xbc);
    if ((reg & 1) == 0) {
        reg = __raw_readl(dram_base + 0xb4);
        seq_printf(m, "%u\n", reg);
    } else {
        seq_printf(m, "0\n");
    }
    return 0;
}

/**
*
*@brief: ak_dcache_hit_proc_open
*@param[in] struct inode *inode
*@param[in] filp
*@return: int
*
**/
static int ak_dcache_hit_proc_open(struct inode *inode, struct file *filp)
{
    return single_open(filp, ak_dcache_hit_proc_show, PDE_DATA(inode));
}

/* /proc/cache/dcache_hit */
static const struct file_operations ak_dcache_hit_proc_ops = {
    .open       = ak_dcache_hit_proc_open,
    .read       = seq_read,
    .llseek     = seq_lseek,
    .release    = seq_release,
};

static int ak_dcache_miss_proc_show(struct seq_file *m, void *v)
{
    u32 reg;

    reg = __raw_readl(dram_base + 0xbc);
    if ((reg & 1) == 0) {
        reg = __raw_readl(dram_base + 0xb8);
        seq_printf(m, "%u\n", reg);
    } else {
        seq_printf(m, "0\n");
    }
    return 0;
}

static int ak_dcache_miss_proc_open(struct inode *inode, struct file *filp)
{
    return single_open(filp, ak_dcache_miss_proc_show, PDE_DATA(inode));
}

/* /proc/cache/dcache_miss */
static const struct file_operations ak_dcache_miss_proc_ops = {
    .open       = ak_dcache_miss_proc_open,
    .read       = seq_read,
    .llseek     = seq_lseek,
    .release    = seq_release,
};

int ak_cache_hit_miss_counting_init(void)
{
    struct device_node *np;
    struct resource iomem;
    int ret;

    np = of_find_node_by_name(NULL, "dram-controller");
    if (!np) {
        pr_err("can not find dram-controller node!\n");
        return -ENODEV;
    }
    /*get iomem form the dts */
    ret = of_address_to_resource(np, 0, &iomem);
    if (ret) {
        pr_err("could not get dram-controller memory!\n");
        return -ENODEV;
    }

    /* Creat the directory which name is /proc/cache */
    ak_cache_dir = proc_mkdir("cache", NULL);
    if (!ak_cache_dir) {
        pr_err("cannot create /proc/cache!\n");
        return -ENODEV;
    }

    dram_base = ioremap(iomem.start, resource_size(&iomem));
    if (IS_ERR(dram_base)) {
        pr_err("dram-controller ioremap failed %ld!\n", PTR_ERR(dram_base));
        return -ENODEV;
    }

    pr_debug("DRAM_BASE PA 0x%x VA 0x%p SIZE 0x%x\n",
                iomem.start, dram_base, resource_size(&iomem));

    /* /proc/cache/cache_enable */
    ak_cache_enable_proc = proc_create_data("cache_enable", S_IRUGO,
             ak_cache_dir, &ak_cache_enable_proc_ops, NULL);

    /* /proc/cache/icache_hit */
    ak_icache_hit_proc = proc_create_data("icache_hit", S_IRUGO,
             ak_cache_dir, &ak_icache_hit_proc_ops, NULL);

    /* /proc/cache/icache_miss */
    ak_icache_miss_proc = proc_create_data("icache_miss", S_IRUGO,
             ak_cache_dir, &ak_icache_miss_proc_ops, NULL);

    /* /proc/cache/dcache_hit */
    ak_dcache_hit_proc = proc_create_data("dcache_hit", S_IRUGO,
             ak_cache_dir, &ak_dcache_hit_proc_ops, NULL);

    /* /proc/cache/dcache_miss */
    ak_dcache_miss_proc = proc_create_data("dcache_miss", S_IRUGO,
             ak_cache_dir, &ak_dcache_miss_proc_ops, NULL);

    return 0;
}
/*end of file*/
