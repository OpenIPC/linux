/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/io.h>
#include "ax_riscv_drv.h"
#include "ax_riscv_debug.h"
#include "ax_riscv_utils.h"

#define RISCV_PROC_DIR              "ax_proc/riscv"
#define RISCV_PROC_NODE_LOG_MEM     "log_dump"

#define LOG_MAGIC           0x55aa55aa
#define LOG_VERSION_LEN     8

typedef struct {
    u32 magic;
    char version[LOG_VERSION_LEN];
    u32 mem_len;
    u32 header_addr;
    u32 header_len;
    u32 log_addr;
    u32 log_total_len;
    u32 log_mem_write;
    u32 log_mem_cnt;
} log_header_t;

static struct proc_dir_entry *riscv_dir;

static void riscv_print_log(struct seq_file *m, char *base, u32 offset, u32 len)
{
    int i;
    char *log_start = base + offset;
    for (i = 0; i < (len - offset); i++) {
        seq_printf(m, "%c", *(log_start + i));
    }
    for (i = 0; i < offset; i++) {
        seq_printf(m, "%c", *(base + i));
    }
}

static void riscv_debug_print_log(char *base, u32 offset, u32 len)
{
    int i;
    char *log_start = base + offset;

    for (i = 0; i < (len - offset); i++) {
        pr_cont("%c", *(log_start + i));
    }
    for (i = 0; i < offset; i++) {
        pr_cont("%c", *(base + i));
    }
}

static int riscv_proc_show(struct seq_file *m, void *v)
{
    int ret;
    u64 addr, size;
    void *log_mem_base;
    log_header_t *log_header;
    char *log_base;
    u32 len, offset;

    ret = ax_riscv_utils_get_dts_reg(RISCV_DTS_NODE_LOG_MEM, &addr, &size);
    if (ret != 0) {
        seq_printf(m, "get log memory info fail\n");
        return -1;
    }

    log_mem_base = ioremap_wc(addr, size);
    if (log_mem_base == NULL) {
        seq_printf(m, "remap riscv log memory 0x%llx size %llu fail", addr, size);
        return -1;
    }
    log_header = (log_header_t *)log_mem_base;
    log_base = (char *)log_mem_base + sizeof(log_header_t);
    if (log_header->magic != LOG_MAGIC) {
        seq_printf(m, "riscv log header magic error, 0x%x\n", log_header->magic);
        iounmap(log_mem_base);
        return -1;
    }
    seq_printf(m, "riscv log dump version %s\n", log_header->version);
    if (log_header->log_mem_cnt <= log_header->log_total_len) {
        offset = 0;
        len = log_header->log_mem_cnt;
    } else {
        offset = log_header->log_mem_cnt % log_header->log_total_len;
        len = log_header->log_total_len;
    }
    seq_printf(m, "riscv log addr 0x%llx size 0x%llx offset %u len %u cnt %u\n",
            addr, size, offset, len, log_header->log_mem_cnt);
    riscv_print_log(m, log_base, offset, len);
    iounmap(log_mem_base);

    return 0;
}

int riscv_debug_log_print(void)
{
    int ret;
    u64 addr, size;
    void *log_mem_base;
    log_header_t *log_header;
    char *log_base;
    u32 len, offset;

    printk(">>>>>>>>>>>>>>>riscv debug log start<<<<<<<<<<<<<<<<<<<\n");

    ret = ax_riscv_utils_get_dts_reg(RISCV_DTS_NODE_LOG_MEM, &addr, &size);
    if (ret != 0) {
        printk("get log memory info fail\n");
        return -1;
    }

    log_mem_base = ioremap_wc(addr, size);
    if (log_mem_base == NULL) {
        printk("remap riscv log memory 0x%llx size %llu fail", addr, size);
        return -1;
    }
    log_header = (log_header_t *)log_mem_base;
    log_base = (char *)log_mem_base + sizeof(log_header_t);
    if (log_header->magic != LOG_MAGIC) {
        printk("riscv log header magic error, 0x%x\n", log_header->magic);
        iounmap(log_mem_base);
        return -1;
    }
    printk("riscv log dump version %s\n", log_header->version);
    if (log_header->log_mem_cnt <= log_header->log_total_len) {
        offset = 0;
        len = log_header->log_mem_cnt;
    } else {
        offset = log_header->log_mem_cnt % log_header->log_total_len;
        len = log_header->log_total_len;
    }
    printk("riscv log addr 0x%llx size 0x%llx offset %u len %u cnt %u\n",
            addr, size, offset, len, log_header->log_mem_cnt);
    riscv_debug_print_log(log_base, offset, len);
    iounmap(log_mem_base);
    printk(">>>>>>>>>>>>>>>riscv debug log end<<<<<<<<<<<<<<<<<<<\n");

    return 0;
}

EXPORT_SYMBOL(riscv_debug_log_print);

static int riscv_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, riscv_proc_show, NULL);
}

static const struct file_operations riscv_proc_fops = {
    .owner   = THIS_MODULE,
    .open    = riscv_proc_open,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release,
};

int ax_riscv_debug_init(void)
{
    riscv_dir = proc_mkdir(RISCV_PROC_DIR, NULL);
    if (!riscv_dir) {
        pr_err("failed to create proc directory %s\n", RISCV_PROC_DIR);
        return -ENOMEM;
    }

    if(!proc_create_data(RISCV_PROC_NODE_LOG_MEM, 0, riscv_dir, &riscv_proc_fops, NULL)) {
        pr_err("failed to create proc node %s\n", RISCV_PROC_NODE_LOG_MEM);
        return -ENOMEM;
    }

    return 0;
}

int ax_riscv_debug_deinit(void)
{
    remove_proc_entry(RISCV_PROC_NODE_LOG_MEM, riscv_dir);
    remove_proc_entry(RISCV_PROC_DIR, NULL);
    return 0;
}
