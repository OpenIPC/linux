/*
 * proxyfs_server.c- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */
#include <linux/kernel.h>
#include <linux/vmalloc.h>
#include <linux/of.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include "ms_platform.h"
#include "cam_inter_os.h"
#include "cam_os_wrapper.h"

#define PROXYFS_PATHNAME_MAX 256

#define INTEROS_SC_R2L_PROXYFS_OPEN  0xF1010000
#define INTEROS_SC_R2L_PROXYFS_CLOSE 0xF1010001
#define INTEROS_SC_R2L_PROXYFS_READ  0xF1010002
#define INTEROS_SC_R2L_PROXYFS_WRITE 0xF1010003
#define INTEROS_SC_R2L_PROXYFS_LSEEK 0xF1010004

static u32 proxyfs_server_open(u32 arg0, u32 arg1, u32 arg2, u32 arg3)
{
    struct file *ptFp = NULL;
    mm_segment_t tFs;
    char *       pathname;

    pathname = (char *)ioremap_cache(arg1, PROXYFS_PATHNAME_MAX);
    if (!pathname)
    {
        printk("ProxyFs(S): map pathname fail\n");
        return -1;
    }

    CamOsMemInvalidate((void *)pathname, PROXYFS_PATHNAME_MAX);

    tFs = get_fs();
    set_fs(get_ds());
    ptFp = filp_open(pathname, arg2, arg3);
    set_fs(tFs);

    iounmap(pathname);

    return IS_ERR(ptFp) ? 0 : (u32)ptFp;
}

static u32 proxyfs_server_close(u32 arg0, u32 arg1, u32 arg2, u32 arg3)
{
    if (arg1)
        return (u32)filp_close((struct file *)arg1, NULL);
    else
        return -1;
}

static u32 proxyfs_server_read(u32 arg0, u32 arg1, u32 arg2, u32 arg3)
{
    struct file *ptFp = (struct file *)arg1;
    mm_segment_t tFs;
    loff_t       tPos;
    s32          nRet;
    void *       read_buf = NULL;

    if (!arg1)
    {
        return -1;
    }

    read_buf = (char *)ioremap_cache(arg2, arg3);
    if (!read_buf)
    {
        printk("ProxyFs(S): map read buf fail\n");
        return -1;
    }

    tFs = get_fs();
    set_fs(get_ds());
    tPos        = ptFp->f_pos;
    nRet        = vfs_read(ptFp, (void *)read_buf, arg3, &tPos);
    ptFp->f_pos = tPos;
    set_fs(tFs);

    CamOsMemFlush((void *)read_buf, arg3);
    iounmap(read_buf);

    return nRet;
}

static u32 proxyfs_server_write(u32 arg0, u32 arg1, u32 arg2, u32 arg3)
{
    struct file *ptFp = (struct file *)arg1;
    mm_segment_t tFs;
    loff_t       tPos;
    s32          nRet;
    void *       write_buf = NULL;

    if (!arg1)
    {
        return -1;
    }

    write_buf = (char *)ioremap_cache(arg2, arg3);
    if (!write_buf)
    {
        printk("ProxyFs(S): map write buf fail\n");
        return -1;
    }
    CamOsMemInvalidate((void *)write_buf, arg3);

    tFs = get_fs();
    set_fs(get_ds());
    tPos        = ptFp->f_pos;
    nRet        = vfs_write(ptFp, (void *)write_buf, arg3, &tPos);
    ptFp->f_pos = tPos;
    set_fs(tFs);

    iounmap(write_buf);

    return nRet;
}

static u32 proxyfs_server_lseek(u32 arg0, u32 arg1, u32 arg2, u32 arg3)
{
    if (arg1)
        return vfs_llseek((struct file *)arg1, arg2, arg3);
    else
        return -1;
}

int proxyfs_server_init(void)
{
    s32 reg_ret = 0;

    reg_ret |= CamInterOsSignalReg(INTEROS_SC_R2L_PROXYFS_OPEN, proxyfs_server_open, "proxyfs_open");
    reg_ret |= CamInterOsSignalReg(INTEROS_SC_R2L_PROXYFS_CLOSE, proxyfs_server_close, "proxyfs_close");
    reg_ret |= CamInterOsSignalReg(INTEROS_SC_R2L_PROXYFS_READ, proxyfs_server_read, "proxyfs_read");
    reg_ret |= CamInterOsSignalReg(INTEROS_SC_R2L_PROXYFS_WRITE, proxyfs_server_write, "proxyfs_write");
    reg_ret |= CamInterOsSignalReg(INTEROS_SC_R2L_PROXYFS_LSEEK, proxyfs_server_lseek, "proxyfs_lseek");

    if (reg_ret != 0)
    {
        printk("ProxyFs(S): register signal callback fail\n");
    }

    return reg_ret;
}

late_initcall(proxyfs_server_init);
