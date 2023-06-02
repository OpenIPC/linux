/*!
*****************************************************************************
** \file        arch/arm/mach-gk/include/plat/syncproc.h
**
** \version
**
** \brief
**
** \attention   THIS SAMPLE CODE IS PROVIDED AS IS. GOKE MICROELECTRONICS
**              ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**              OMMISSIONS
**
** (C) Copyright 2012-2015 by GOKE MICROELECTRONICS CO.,LTD
**
*****************************************************************************
*/

#ifndef __PLAT_SYNC_PROC_H
#define __PLAT_SYNC_PROC_H

/* ==========================================================================*/
#define GK_SYNC_PROC_MAX_ID             (31)
#define GK_SYNC_PROC_PAGE_SIZE          (PAGE_SIZE - 16)

/* ==========================================================================*/
#ifndef __ASSEMBLER__
#include <linux/idr.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
typedef    int(sync_read_proc_t)(char *start, void *data);

struct gk_sync_proc_pinfo {
    u32                 id;
    u32                 mask;
    char                *page;
};

struct gk_sync_proc_hinfo {
    u32                 maxid;
    wait_queue_head_t   sync_proc_head;
    atomic_t            sync_proc_flag;
    struct idr          sync_proc_idr;
    struct mutex        sync_proc_lock;
    sync_read_proc_t    *sync_read_proc;
    void                *sync_read_data;
};

/* ==========================================================================*/

/* ==========================================================================*/
extern int gk_sync_proc_hinit(struct gk_sync_proc_hinfo *hinfo);
extern int gk_sync_proc_open(struct inode *inode, struct file *file);
extern int gk_sync_proc_release(struct inode *inode, struct file *file);
extern ssize_t gk_sync_proc_read(struct file *file, char __user *buf, size_t size, loff_t *ppos);
extern ssize_t gk_sync_proc_write(struct file *file, const char __user *buf, size_t size, loff_t *ppos);

struct gk_async_proc_info {
    char                    proc_name[256];
    struct file_operations  fops;
    void                    *private_data;
    struct fasync_struct    *fasync_queue;
    struct mutex            op_mutex;
    int                     use_count;
};

extern int gk_async_proc_create(struct gk_async_proc_info *pinfo);
extern int gk_async_proc_remove(struct gk_async_proc_info *pinfo);


#endif /* __ASSEMBLER__ */

#endif

