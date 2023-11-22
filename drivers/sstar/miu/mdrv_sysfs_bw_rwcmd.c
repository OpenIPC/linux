/*
 * mdrv_sysfs_bw_rwcmd.c- Sigmastar
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
#include <linux/irqchip.h>
#include <linux/of_platform.h>
#include <linux/of_fdt.h>
#include <linux/delay.h>
#include <linux/mutex.h>

#include <asm/uaccess.h>
#include "registers.h"
#include "ms_platform.h"
#include "ms_types.h"
#include "cam_os_wrapper.h"
#include "mdrv_miu.h"
#include "mhal_bw.h"
#include "mhal_miu_client.h"
#ifdef CONFIG_CAM_CLK
#include "camclk.h"
#include "drv_camclk_Api.h"
#endif
/*=============================================================*/
// Structure definition
/*=============================================================*/

static struct miu_device miu0;
#define DUMP_FILE_TEMP_BUF_SZ (PAGE_SIZE * 10)

#ifdef CONFIG_PM_SLEEP
int         miu_subsys_suspend(struct device *dev);
int         miu_subsys_resume(struct device *dev);
extern void miu_arb_resume(void);
static SIMPLE_DEV_PM_OPS(miu_pm_ops, miu_subsys_suspend, miu_subsys_resume);
#endif

static struct bus_type miu_subsys = {
    .name     = "miu",
    .dev_name = "miu",
#ifdef CONFIG_PM_SLEEP
    .pm = &miu_pm_ops,
#endif
};

static int gmonitor_duration[MIU_NUM]    = {DURATION};
static int gmonitor_output_kmsg[MIU_NUM] = {KMSG};

static char m_bOutputFilePath[128] = "/mnt/dump_miu_bw.txt";

/*=============================================================*/
// Local function
/*=============================================================*/
static struct file *miu_bw_open_file(char *path, int flag, int mode)
{
    struct file *filp = NULL;
    mm_segment_t oldfs;

    oldfs = get_fs();
    set_fs(KERNEL_DS);

    filp = filp_open(path, flag, mode);

    set_fs(oldfs);
    if (IS_ERR(filp))
    {
        return NULL;
    }
    return filp;
}

static int miu_bw_write_file(struct file *fp, char *buf, int writelen)
{
    mm_segment_t oldfs;
    int          ret;

    oldfs = get_fs();
    set_fs(KERNEL_DS);
    ret = vfs_write(fp, buf, writelen, &fp->f_pos);

    set_fs(oldfs);
    return ret;
}

static int miu_bw_close_file(struct file *fp)
{
    filp_close(fp, NULL);
    return 0;
}

static int set_miu_client_enable(struct device *dev, const char *buf, size_t n, int enabled, int wcmd)
{
    long idx = -1;
    int  client_NUM;

    if (wcmd)
        client_NUM = MIU0_CLIENT_WR_NUM;
    else
        client_NUM = MIU0_CLIENT_RD_NUM;

    if ('0' == (dev->kobj.name[6]))
    {
        if (kstrtol(buf, 10, &idx) != 0 || idx < 0 || idx >= client_NUM)
        {
            return -EINVAL;
        }

        if (wcmd && miu0_wr_clients[idx].bw_rsvd == 0)
            miu0_wr_clients[idx].bw_enabled = enabled;
        else if (!wcmd && miu0_rd_clients[idx].bw_rsvd == 0)
            miu0_rd_clients[idx].bw_enabled = enabled;
    }

    return n;
}

static ssize_t monitor_wcmd_client_enable_store(struct device *dev, struct device_attribute *attr, const char *buf,
                                                size_t n)
{
    int   i;
    char *pt;
    char *opt;

    if (halSetCheckIPBwLock(buf))
        return n;

    if (!strncmp(buf, "all", strlen("all")))
    {
        if ('0' == (dev->kobj.name[6]))
        {
            for (i = 0; i < MIU0_CLIENT_WR_NUM; i++)
            {
                if (miu0_wr_clients[i].bw_rsvd == 0)
                    miu0_wr_clients[i].bw_enabled = 1;
            }
        }
        return n;
    }

    pt = kmalloc(strlen(buf) + 1, GFP_KERNEL);
    strcpy(pt, buf);
    while ((opt = strsep(&pt, ";, ")) != NULL)
    {
        set_miu_client_enable(dev, opt, n, 1, 1);
    }
    kfree(pt);

    return n;
}

static ssize_t monitor_wcmd_client_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    int   i   = 0;

    if ('0' == (dev->kobj.name[6]))
    {
        str += scnprintf(str, end - str, "Num:IP_name	[BW_Idx][Enable(1)/Disable(0)]\n");

        for (i = 0; i < MIU0_CLIENT_WR_NUM; i++)
        {
            // if (miu0_clients[i].bw_enabled && !miu0_clients[i].bw_rsvd)
            {
                str += scnprintf(str, end - str, "%3d:%s[0x%02X][%d]\n", (short)i, miu0_wr_clients[i].name,
                                 (short)miu0_wr_clients[i].bw_client_id, (char)miu0_wr_clients[i].bw_enabled);
            }
        }
    }

    if (str > buf)
        str--;

    str += scnprintf(str, end - str, "\n");

    return (str - buf);
}

static ssize_t monitor_wcmd_client_disable_store(struct device *dev, struct device_attribute *attr, const char *buf,
                                                 size_t n)
{
    int   i;
    char *pt;
    char *opt;

    if (halSetCheckIPBwLock(buf))
        return n;

    if (!strncmp(buf, "all", strlen("all")))
    {
        if ('0' == (dev->kobj.name[6]))
        {
            for (i = 0; i < MIU0_CLIENT_WR_NUM; i++)
            {
                if (miu0_wr_clients[i].bw_rsvd == 0)
                    miu0_wr_clients[i].bw_enabled = 0;
            }
        }
        return n;
    }

    pt = kmalloc(strlen(buf) + 1, GFP_KERNEL);
    strcpy(pt, buf);
    while ((opt = strsep(&pt, ";, ")) != NULL)
    {
        set_miu_client_enable(dev, opt, n, 0, 1);
    }
    kfree(pt);

    return n;
}

static ssize_t monitor_wcmd_client_disable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    int   i   = 0;

    if ('0' == (dev->kobj.name[6]))
    {
        str += scnprintf(str, end - str, "Num:IP_name	[BW_Idx][Enable(1)/Disable(0)]\n");

        for (i = 0; i < MIU0_CLIENT_WR_NUM; i++)
        {
            str += scnprintf(str, end - str, "%3d:%s[0x%02X][%d]\n", (short)i, miu0_wr_clients[i].name,
                             (short)miu0_wr_clients[i].bw_client_id, (char)miu0_wr_clients[i].bw_enabled);
        }
    }

    if (str > buf)
        str--;

    str += scnprintf(str, end - str, "\n");

    return (str - buf);
}

static ssize_t monitor_set_counts_avg_store(struct device *dev, struct device_attribute *attr, const char *buf,
                                            size_t count)
{
    u32 input = 0;

    input = simple_strtoul(buf, NULL, 10);

    if ('0' == (dev->kobj.name[6]))
        gmonitor_duration[0] = input;

    return count;
}

static ssize_t monitor_set_counts_avg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    if ('0' == (dev->kobj.name[6]))
        return sprintf(buf, "%d\n", gmonitor_duration[0]);
    else
        return 0;
}

static ssize_t monitor_rcmd_client_enable_store(struct device *dev, struct device_attribute *attr, const char *buf,
                                                size_t n)
{
    int   i;
    char *pt;
    char *opt;

    if (halSetCheckIPBwLock(buf))
        return n;

    if (!strncmp(buf, "all", strlen("all")))
    {
        if ('0' == (dev->kobj.name[6]))
        {
            for (i = 0; i < MIU0_CLIENT_RD_NUM; i++)
            {
                if (miu0_rd_clients[i].bw_rsvd == 0)
                    miu0_rd_clients[i].bw_enabled = 1;
            }
        }
        return n;
    }

    pt = kmalloc(strlen(buf) + 1, GFP_KERNEL);
    strcpy(pt, buf);
    while ((opt = strsep(&pt, ";, ")) != NULL)
    {
        set_miu_client_enable(dev, opt, n, 1, 0);
    }
    kfree(pt);

    return n;
}

static ssize_t monitor_rcmd_client_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    int   i   = 0;

    if ('0' == (dev->kobj.name[6]))
    {
        str += scnprintf(str, end - str, "Num:IP_name   [BW_Idx][Enable(1)/Disable(0)]\n");

        for (i = 0; i < MIU0_CLIENT_RD_NUM; i++)
        {
            // if (miu0_clients[i].bw_enabled && !miu0_clients[i].bw_rsvd)
            {
                str += scnprintf(str, end - str, "%3d:%s[0x%02X][%d]\n", (short)i, miu0_rd_clients[i].name,
                                 (short)miu0_rd_clients[i].bw_client_id, (char)miu0_rd_clients[i].bw_enabled);
            }
        }
    }

    if (str > buf)
        str--;

    str += scnprintf(str, end - str, "\n");

    return (str - buf);
}

static ssize_t monitor_rcmd_client_disable_store(struct device *dev, struct device_attribute *attr, const char *buf,
                                                 size_t n)
{
    int   i;
    char *pt;
    char *opt;

    if (halSetCheckIPBwLock(buf))
        return n;

    if (!strncmp(buf, "all", strlen("all")))
    {
        if ('0' == (dev->kobj.name[6]))
        {
            for (i = 0; i < MIU0_CLIENT_RD_NUM; i++)
            {
                if (miu0_rd_clients[i].bw_rsvd == 0)
                    miu0_rd_clients[i].bw_enabled = 0;
            }
        }
        return n;
    }

    pt = kmalloc(strlen(buf) + 1, GFP_KERNEL);
    strcpy(pt, buf);
    while ((opt = strsep(&pt, ";, ")) != NULL)
    {
        set_miu_client_enable(dev, opt, n, 0, 0);
    }
    kfree(pt);

    return n;
}

static ssize_t monitor_rcmd_client_disable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    int   i   = 0;

    if ('0' == (dev->kobj.name[6]))
    {
        str += scnprintf(str, end - str, "Num:IP_name   [BW_Idx][Enable(1)/Disable(0)]\n");

        for (i = 0; i < MIU0_CLIENT_RD_NUM; i++)
        {
            str += scnprintf(str, end - str, "%3d:%s[0x%02X][%d]\n", (short)i, miu0_rd_clients[i].name,
                             (short)miu0_rd_clients[i].bw_client_id, (char)miu0_rd_clients[i].bw_enabled);
        }
    }

    if (str > buf)
        str--;

    str += scnprintf(str, end - str, "\n");

    return (str - buf);
}

static ssize_t measure_all_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    u32 input = 0;

    input = simple_strtoul(buf, NULL, 10);

    if ('0' == (dev->kobj.name[6]))
        gmonitor_output_kmsg[0] = input;

    return count;
}

static ssize_t measure_bw_overall_store(struct device *dev, struct device_attribute *attr, const char *buf,
                                        size_t count)
{
    if ('0' == (dev->kobj.name[6]))
        halBWMeasureRegsStore(buf, MIU_BW_HDR_DDRINFO, MIU_CH_NONE);

    return count;
}

#ifdef MIU_BW_SYNOPSYS
static ssize_t measure_rd_latency_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *             str = buf;
    char *             end = buf + PAGE_SIZE;
    int                i   = 0;
    int                id;
    int                iMiuClientNum      = 0;
    int                iMonitorDuration   = 0;
    int                iMonitorOutputKmsg = 1;
    int                iMonitorDumpToFile = 0;
    int                iMiuBankAddr       = 0;
    struct miu_client *pstMiuClient       = NULL;
    struct miu_client *pstMiuRcmdClient   = NULL;
    char *             u8Buf              = NULL;
    char *             u8Ptr              = NULL;
    char *             u8PtrEnd           = NULL;
    struct file *      pstOutFd           = NULL;
    char               measure_header[128];

    memset(measure_header, 0, sizeof(measure_header));

    if ('0' == (dev->kobj.name[6]))
    {
        iMiuClientNum      = MIU0_CLIENT_WR_NUM;
        iMonitorDuration   = gmonitor_duration[0];
        iMonitorOutputKmsg = gmonitor_output_kmsg[0];
        iMiuBankAddr       = BASE_REG_MIU_MISC_CH0;
        pstMiuClient       = &miu0_wr_clients[0];
        pstMiuRcmdClient   = &miu0_rd_clients[0];
    }
    else
    {
        return 0;
    }

    if (iMonitorDumpToFile)
    {
        u8Buf    = kmalloc(DUMP_FILE_TEMP_BUF_SZ, GFP_KERNEL);
        u8Ptr    = u8Buf;
        u8PtrEnd = u8Buf + (DUMP_FILE_TEMP_BUF_SZ);

        pstOutFd = miu_bw_open_file(m_bOutputFilePath, O_WRONLY | O_CREAT, 0644);
    }

    if (iMonitorOutputKmsg)
    {
        halBWGetMeasureHeader(measure_header, MIU_BW_RD_LATENCY, MIU_CH_NONE);
        printk("%s\r\n", measure_header);
        printk("---------------------------------------------------------\r\n");
    }
    else
    {
        halBWGetMeasureHeader(measure_header, MIU_BW_RD_LATENCY, MIU_CH_NONE);
        str += scnprintf(str, end - str, "%s\r\n", measure_header);
        str += scnprintf(str, end - str, "---------------------------------------------------------\r\n");
    }

    // rcmd latency and length
    for (i = 1; i < iMiuClientNum; i++)
    {
        if ((pstMiuRcmdClient + i)->bw_enabled && (pstMiuRcmdClient + i)->bw_rsvd == 0)
        {
            id = (pstMiuRcmdClient + i)->bw_client_id;
            // bandwidth report for overall
            id = id & 0x1FF;

            halBWMeasureTrigger(MIU_BW_RD_LATENCY, id, MIU_CH_RCMD);
            msleep(iMonitorDuration);
            if (iMonitorOutputKmsg)
            {
                halBWGetMeasureStat(0, measure_header, 128, MIU_BW_RD_LATENCY, id, MIU_CH_RCMD);
                printk("%d:%s\t%s\n", (short)i, (pstMiuRcmdClient + i)->name, measure_header);
                memset(measure_header, 0, sizeof(measure_header));
                halBWGetMeasureStat(0, measure_header, 128, MIU_BW_RD_LENGTH, id, MIU_CH_RCMD);
                printk("%d:%s\t%s\n", (short)i, (pstMiuRcmdClient + i)->name, measure_header);
                memset(measure_header, 0, sizeof(measure_header));
            }
            else
            {
                halBWGetMeasureStat(0, measure_header, 128, MIU_BW_RD_LATENCY, id, MIU_CH_RCMD);
                str += scnprintf(str, end - str, "%d:%s\t%s\n", (short)i, (pstMiuRcmdClient + i)->name, measure_header);
                memset(measure_header, 0, sizeof(measure_header));
                halBWGetMeasureStat(0, measure_header, 128, MIU_BW_RD_LENGTH, id, MIU_CH_RCMD);
                str += scnprintf(str, end - str, "%d:%s\t%s\n", (short)i, (pstMiuRcmdClient + i)->name, measure_header);
                memset(measure_header, 0, sizeof(measure_header));
            }
        }
    }

    if (iMonitorDumpToFile)
    {
        if (pstOutFd)
        {
            miu_bw_write_file(pstOutFd, u8Buf, u8Ptr - u8Buf);
            miu_bw_close_file(pstOutFd);
        }
        kfree(u8Buf);
    }

    if (str > buf)
        str--;

    str += scnprintf(str, end - str, "\n");
    return (str - buf);
}

static ssize_t measure_bw_overall_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    int   i   = 0;
    int   id;
    int   iMiuClientNum      = 0;
    int   iMonitorDuration   = 0;
    int   iMonitorOutputKmsg = 1;
    int   iMonitorDumpToFile = 0;
    int   iMiuBankAddr       = 0;
    // int   output_bw_ch0[3][2];
#if (MIU_BW_BANK_NUM > 1)
    int iMiuBankAddr_1 = 0;
    // int output_bw_ch1[3][2];
#endif
    struct miu_client *pstMiuClient     = NULL;
    struct miu_client *pstMiuRcmdClient = NULL;
    char *             u8Buf            = NULL;
    char *             u8Ptr            = NULL;
    char *             u8PtrEnd         = NULL;
    struct file *      pstOutFd         = NULL;
    char               measure_header[128];

    memset(measure_header, 0, sizeof(measure_header));

    if ('0' == (dev->kobj.name[6]))
    {
        iMiuClientNum      = MIU0_CLIENT_WR_NUM;
        iMonitorDuration   = gmonitor_duration[0];
        iMonitorOutputKmsg = gmonitor_output_kmsg[0];
        iMiuBankAddr       = BASE_REG_MIU_MISC_CH0;
#if (MIU_BW_BANK_NUM > 1)
        iMiuBankAddr_1 = BASE_REG_MIU_MISC_CH1;
#endif
        pstMiuClient     = &miu0_wr_clients[0];
        pstMiuRcmdClient = &miu0_rd_clients[0];
    }
    else
    {
        return 0;
    }

    if (iMonitorDumpToFile)
    {
        u8Buf    = kmalloc(DUMP_FILE_TEMP_BUF_SZ, GFP_KERNEL);
        u8Ptr    = u8Buf;
        u8PtrEnd = u8Buf + (DUMP_FILE_TEMP_BUF_SZ);

        pstOutFd = miu_bw_open_file(m_bOutputFilePath, O_WRONLY | O_CREAT, 0644);
    }

    if (iMonitorOutputKmsg)
    {
        halBWGetMeasureHeader(measure_header, MIU_BW_HDR_DDRINFO, MIU_CH_NONE);
        printk("%s\r\n", measure_header);
        printk("---------------------------------------------------------\r\n");
    }
    else
    {
        halBWGetMeasureHeader(measure_header, MIU_BW_HDR_DDRINFO, MIU_CH_NONE);
        str += scnprintf(str, end - str, "%s\r\n", measure_header);
        str += scnprintf(str, end - str, "---------------------------------------------------------\r\n");
    }

    for (i = 0; i < 1; i++)
    {
        if ((pstMiuClient + i)->bw_enabled && (pstMiuClient + i)->bw_rsvd == 0)
        {
            id = (pstMiuClient + i)->bw_client_id;
            // bandwidth report for overall
            id = id & 0x1FF;

            halBWMeasureTrigger(MIU_BW_HDR_DDRINFO, id, MIU_CH_NONE);

            msleep(iMonitorDuration);

            if (iMonitorOutputKmsg)
            {
                halBWGetMeasureStat(0, measure_header, 128, MIU_BW_HDR_DDRINFO, id, MIU_CH_NONE);
                printk("0:OVERALL\t%s\n", measure_header);
                memset(measure_header, 0, sizeof(measure_header));
#if (MIU_BW_BANK_NUM > 1)
                halBWGetMeasureStat(1, measure_header, 128, MIU_BW_HDR_DDRINFO, id, MIU_CH_NONE);
                printk("0:OVERALL\t%s\n", measure_header);
#endif
            }
            else
            {
                halBWGetMeasureStat(0, measure_header, 128, MIU_BW_HDR_DDRINFO, id, MIU_CH_NONE);
                str += scnprintf(str, end - str, "0:OVERALL\t%s\n", measure_header);
                memset(measure_header, 0, sizeof(measure_header));
#if (MIU_BW_BANK_NUM > 1)
                halBWGetMeasureStat(1, measure_header, 128, MIU_BW_HDR_DDRINFO, id, MIU_CH_NONE);
                str += scnprintf(str, end - str, "0:OVERALL\t%s\n", measure_header);
#endif
            }

            halBWResetFunc(iMiuBankAddr, MIU_CH_NONE); // reset all
#if (MIU_BW_BANK_NUM > 1)
            halBWResetFunc(iMiuBankAddr_1, MIU_CH_NONE); // reset all
#endif
        }
    }

    if (iMonitorDumpToFile)
    {
        if (pstOutFd)
        {
            miu_bw_write_file(pstOutFd, u8Buf, u8Ptr - u8Buf);
            miu_bw_close_file(pstOutFd);
        }
        kfree(u8Buf);
    }

    if (str > buf)
        str--;

    str += scnprintf(str, end - str, "\n");
    return (str - buf);
}

static ssize_t measure_ba_report_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    int   i   = 0;
    int   id;
    int   iMiuClientNum      = 0;
    int   iMonitorDuration   = 0;
    int   iMonitorOutputKmsg = 1;
    int   iMonitorDumpToFile = 0;
    int   iMiuBankAddr       = 0;
    // int   output_bw_ch0[3][2];
#if (MIU_BW_BANK_NUM > 1)
    int iMiuBankAddr_1 = 0;
    // int output_bw_ch1[3][2];
#endif
    struct miu_client *pstMiuClient     = NULL;
    struct miu_client *pstMiuRcmdClient = NULL;
    char *             u8Buf            = NULL;
    char *             u8Ptr            = NULL;
    char *             u8PtrEnd         = NULL;
    struct file *      pstOutFd         = NULL;
    char               measure_header[128];

    memset(measure_header, 0, sizeof(measure_header));

    if ('0' == (dev->kobj.name[6]))
    {
        iMiuClientNum      = MIU0_CLIENT_WR_NUM;
        iMonitorDuration   = gmonitor_duration[0];
        iMonitorOutputKmsg = gmonitor_output_kmsg[0];
        iMiuBankAddr       = BASE_REG_MIU_MISC_CH0;
#if (MIU_BW_BANK_NUM > 1)
        iMiuBankAddr_1 = BASE_REG_MIU_MISC_CH1;
#endif
        pstMiuClient     = &miu0_wr_clients[0];
        pstMiuRcmdClient = &miu0_rd_clients[0];
    }
    else
    {
        return 0;
    }

    if (iMonitorDumpToFile)
    {
        u8Buf    = kmalloc(DUMP_FILE_TEMP_BUF_SZ, GFP_KERNEL);
        u8Ptr    = u8Buf;
        u8PtrEnd = u8Buf + (DUMP_FILE_TEMP_BUF_SZ);

        pstOutFd = miu_bw_open_file(m_bOutputFilePath, O_WRONLY | O_CREAT, 0644);
    }

    if (iMonitorOutputKmsg)
    {
        halBWGetMeasureHeader(measure_header, MIU_BW_HDR_BA_REPORT, MIU_CH_NONE);
        printk("%s\r\n", measure_header);
        printk("---------------------------------------------------------\r\n");
    }
    else
    {
        halBWGetMeasureHeader(measure_header, MIU_BW_HDR_BA_REPORT, MIU_CH_NONE);
        str += scnprintf(str, end - str, "%s\r\n", measure_header);
        str += scnprintf(str, end - str, "---------------------------------------------------------\r\n");
    }

    // wcmd
    for (i = 1; i < iMiuClientNum; i++)
    {
        if ((pstMiuClient + i)->bw_enabled && (pstMiuClient + i)->bw_rsvd == 0)
        {
            id = (pstMiuClient + i)->bw_client_id;
            // bandwidth report for overall
            id = id & 0x1FF;

            halBWMeasureTrigger(MIU_BW_HDR_BA_REPORT, id, MIU_CH_WCMD);

            msleep(iMonitorDuration);

            if (iMonitorOutputKmsg)
            {
                halBWGetMeasureStat(0, measure_header, 128, MIU_BW_HDR_BA_REPORT, id, MIU_CH_WCMD);
                printk("%d:%s\t%s\n", (short)i, (pstMiuClient + i)->name, measure_header);
                memset(measure_header, 0, sizeof(measure_header));
#if (MIU_BW_BANK_NUM > 1)
                halBWGetMeasureStat(1, measure_header, 128, MIU_BW_HDR_BA_REPORT, id, MIU_CH_WCMD);
                printk("%d:%s\t%s\n", (short)i, (pstMiuClient + i)->name, measure_header);
#endif
            }
            else
            {
                halBWGetMeasureStat(0, measure_header, 128, MIU_BW_HDR_BA_REPORT, id, MIU_CH_WCMD);
                str += scnprintf(str, end - str, "%d:%s\t%s\n", (short)i, (pstMiuClient + i)->name, measure_header);
                memset(measure_header, 0, sizeof(measure_header));
#if (MIU_BW_BANK_NUM > 1)
                halBWGetMeasureStat(1, measure_header, 128, MIU_BW_HDR_BA_REPORT, id, MIU_CH_WCMD);
                str += scnprintf(str, end - str, "%d:%s\t%s\n", (short)i, (pstMiuClient + i)->name, measure_header);
#endif
            }

            halBWResetFunc(iMiuBankAddr, MIU_CH_NONE); // reset all
#if (MIU_BW_BANK_NUM > 1)
            halBWResetFunc(iMiuBankAddr_1, MIU_CH_NONE); // reset all
#endif
        }
    }

    // rcmd
    for (i = 1; i < iMiuClientNum; i++)
    {
        if ((pstMiuRcmdClient + i)->bw_enabled && (pstMiuRcmdClient + i)->bw_rsvd == 0)
        {
            id = (pstMiuRcmdClient + i)->bw_client_id;
            // bandwidth report for overall
            id = id & 0x1FF;

            halBWMeasureTrigger(MIU_BW_HDR_BA_REPORT, id, MIU_CH_RCMD);

            msleep(iMonitorDuration);

            if (iMonitorOutputKmsg)
            {
                halBWGetMeasureStat(0, measure_header, 128, MIU_BW_HDR_BA_REPORT, id, MIU_CH_RCMD);
                printk("%d:%s\t%s\n", (short)i, (pstMiuRcmdClient + i)->name, measure_header);
                memset(measure_header, 0, sizeof(measure_header));
#if (MIU_BW_BANK_NUM > 1)
                halBWGetMeasureStat(1, measure_header, 128, MIU_BW_HDR_BA_REPORT, id, MIU_CH_RCMD);
                printk("%d:%s\t%s\n", (short)i, (pstMiuRcmdClient + i)->name, measure_header);
#endif
            }
            else
            {
                halBWGetMeasureStat(0, measure_header, 128, MIU_BW_HDR_BA_REPORT, id, MIU_CH_RCMD);
                str += scnprintf(str, end - str, "%d:%s\t%s\n", (short)i, (pstMiuRcmdClient + i)->name, measure_header);
                memset(measure_header, 0, sizeof(measure_header));
#if (MIU_BW_BANK_NUM > 1)
                halBWGetMeasureStat(1, measure_header, 128, MIU_BW_HDR_BA_REPORT, id, MIU_CH_RCMD);
                str += scnprintf(str, end - str, "%d:%s\t%s\n", (short)i, (pstMiuRcmdClient + i)->name, measure_header);
#endif
            }

            halBWResetFunc(iMiuBankAddr, MIU_CH_NONE); // reset all
#if (MIU_BW_BANK_NUM > 1)
            halBWResetFunc(iMiuBankAddr_1, MIU_CH_NONE); // reset all
#endif
        }
    }

    if (iMonitorDumpToFile)
    {
        if (pstOutFd)
        {
            miu_bw_write_file(pstOutFd, u8Buf, u8Ptr - u8Buf);
            miu_bw_close_file(pstOutFd);
        }
        kfree(u8Buf);
    }

    if (str > buf)
        str--;

    str += scnprintf(str, end - str, "\n");
    return (str - buf);
}

static ssize_t measure_all_hw_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    int   i   = 0;
    int   id;
    int   iMiuClientNum      = 0;
    int   iMonitorDuration   = 0;
    int   iMonitorOutputKmsg = 1;
    int   iMonitorDumpToFile = 0;
    int   iMiuBankAddr       = 0;
#if (MIU_BW_BANK_NUM > 1)
    int iMiuBankAddr_1 = 0;
#endif
    struct miu_client *pstMiuClient     = NULL;
    struct miu_client *pstMiuRcmdClient = NULL;
    char *             u8Buf            = NULL;
    char *             u8Ptr            = NULL;
    char *             u8PtrEnd         = NULL;
    struct file *      pstOutFd         = NULL;
    char               measure_header[128];

    memset(measure_header, 0, sizeof(measure_header));

    if ('0' == (dev->kobj.name[6]))
    {
        iMiuClientNum      = MIU0_CLIENT_WR_NUM;
        iMonitorDuration   = gmonitor_duration[0];
        iMonitorOutputKmsg = gmonitor_output_kmsg[0];
        iMiuBankAddr       = BASE_REG_MIU_MISC_CH0;
#if (MIU_BW_BANK_NUM > 1)
        iMiuBankAddr_1 = BASE_REG_MIU_MISC_CH1;
#endif
        pstMiuClient     = &miu0_wr_clients[0];
        pstMiuRcmdClient = &miu0_rd_clients[0];
    }
    else
    {
        return 0;
    }

    for (i = 0; i < iMiuClientNum; i++)
    {
        (pstMiuClient + i)->effi_min = 0x3FF;
        (pstMiuClient + i)->effi_avg = 0;
        (pstMiuClient + i)->effi_max = 0;
        (pstMiuClient + i)->bw_avg   = 0;
        (pstMiuClient + i)->bw_max   = 0;
        (pstMiuClient + i)->bw_min   = 0x3FF;

        if ((pstMiuClient + i)->bw_dump_en)
        {
            iMonitorDumpToFile = 1;
        }
    }

    if (iMonitorDumpToFile)
    {
        u8Buf    = kmalloc(DUMP_FILE_TEMP_BUF_SZ, GFP_KERNEL);
        u8Ptr    = u8Buf;
        u8PtrEnd = u8Buf + (DUMP_FILE_TEMP_BUF_SZ);

        pstOutFd = miu_bw_open_file(m_bOutputFilePath, O_WRONLY | O_CREAT, 0644);
    }

    if (iMonitorOutputKmsg)
    {
        // printk("Num:client\tBA_rp0\tBWavg0\tBWmax0\tBA_rp1\tBWavg1\tBWmax1");
        halBWGetMeasureHeader(measure_header, MIU_BW_HDR_BABW, MIU_CH_NONE);
        printk("%s\r\n", measure_header);
        printk("---------------------------------------------------------\r\n");
    }
    else
    {
        halBWGetMeasureHeader(measure_header, MIU_BW_HDR_BABW, MIU_CH_NONE);
        str += scnprintf(str, end - str, "%s\r\n", measure_header);
        str += scnprintf(str, end - str, "---------------------------------------------------------\r\n");
    }

    for (i = 0; i < iMiuClientNum; i++)
    {
        // bandwidth for wcmd
        if ((pstMiuClient + i)->bw_enabled && (pstMiuClient + i)->bw_rsvd == 0)
        {
            id = (pstMiuClient + i)->bw_client_id;
            id = id & 0x1FF;

            halBWInit(iMiuBankAddr, id, MIU_CH_WCMD);
#if (MIU_BW_BANK_NUM > 1)
            halBWInit(iMiuBankAddr_1, id, MIU_CH_WCMD);
#endif

            halBWEffiMinPerConfig(iMiuBankAddr, id, MIU_CH_WCMD);
#if (MIU_BW_BANK_NUM > 1)
            halBWEffiMinPerConfig(iMiuBankAddr_1, id, MIU_CH_WCMD);
#endif
            msleep(iMonitorDuration);

            if (iMonitorOutputKmsg)
            {
                halBWGetMeasureStat(0, measure_header, 128, MIU_BW_HDR_BABW, id, MIU_CH_WCMD);
                printk("%d:%s\t%s\n", (short)(pstMiuClient + i)->bw_client_id, (pstMiuClient + i)->name,
                       measure_header);
            }
            else
            {
                halBWGetMeasureStat(0, measure_header, 128, MIU_BW_HDR_BABW, id, MIU_CH_WCMD);
                str += scnprintf(str, end - str, "%d:%s\t%s\n", (short)i, (pstMiuClient + i)->name, measure_header);
            }

            halBWResetFunc(iMiuBankAddr, MIU_CH_WCMD); // reset all
#if (MIU_BW_BANK_NUM > 1)
            halBWResetFunc(iMiuBankAddr_1, MIU_CH_WCMD); // reset all
#endif
        }

        // bandwidth for rcmd
        if ((pstMiuRcmdClient + i)->bw_enabled && (pstMiuRcmdClient + i)->bw_rsvd == 0)
        {
            id = (pstMiuRcmdClient + i)->bw_client_id;
            id = id & 0x1FF;

            halBWInit(iMiuBankAddr, id, MIU_CH_RCMD);
#if (MIU_BW_BANK_NUM > 1)
            halBWInit(iMiuBankAddr_1, id, MIU_CH_RCMD);
#endif

            halBWEffiMinPerConfig(iMiuBankAddr, id, MIU_CH_RCMD);
#if (MIU_BW_BANK_NUM > 1)
            halBWEffiMinPerConfig(iMiuBankAddr_1, id, MIU_CH_RCMD);
#endif
            msleep(iMonitorDuration);

            if (iMonitorOutputKmsg)
            {
                halBWGetMeasureStat(0, measure_header, 128, MIU_BW_HDR_BABW, id, MIU_CH_RCMD);
                printk("%d:%s\t%s\n", (short)(pstMiuRcmdClient + i)->bw_client_id, (pstMiuRcmdClient + i)->name,
                       measure_header);
            }
            else
            {
                halBWGetMeasureStat(0, measure_header, 128, MIU_BW_HDR_BABW, id, MIU_CH_RCMD);
                str += scnprintf(str, end - str, "%d:%s\t%s\n", (short)i, (pstMiuRcmdClient + i)->name, measure_header);
            }

            halBWResetFunc(iMiuBankAddr, MIU_CH_RCMD); // reset all
#if (MIU_BW_BANK_NUM > 1)
            halBWResetFunc(iMiuBankAddr_1, MIU_CH_RCMD); // reset all
#endif
        }
    }

    if (iMonitorDumpToFile)
    {
        if (pstOutFd)
        {
            miu_bw_write_file(pstOutFd, u8Buf, u8Ptr - u8Buf);
            miu_bw_close_file(pstOutFd);
        }
        kfree(u8Buf);
    }

    if (str > buf)
        str--;

    str += scnprintf(str, end - str, "\n");
    return (str - buf);
}
#else
static ssize_t measure_bw_overall_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str     = buf;
    char *end     = buf + PAGE_SIZE;
    int   bwch_en = MIU_CH_NONE;
    int   i       = 0;
    int   id;
    int   iMiuClientNum      = 0;
    int   iMonitorDuration   = 0;
    int   iMonitorOutputKmsg = 1;
    int   iMonitorDumpToFile = 0;
    int   iMiuBankAddr       = 0;
    // int   output_bw_ch0[3][2];
#if (MIU_BW_BANK_NUM > 1)
    int iMiuBankAddr_1 = 0;
    // int output_bw_ch1[3][2];
#endif
    struct miu_client *pstMiuClient     = NULL;
    struct miu_client *pstMiuRcmdClient = NULL;
    char *             u8Buf            = NULL;
    char *             u8Ptr            = NULL;
    char *             u8PtrEnd         = NULL;
    struct file *      pstOutFd         = NULL;
    char               measure_header[128];

    memset(measure_header, 0, sizeof(measure_header));

    if ('0' == (dev->kobj.name[6]))
    {
        iMiuClientNum      = MIU0_CLIENT_WR_NUM;
        iMonitorDuration   = gmonitor_duration[0];
        iMonitorOutputKmsg = gmonitor_output_kmsg[0];
        iMiuBankAddr       = BASE_REG_MIU_MISC_CH0;
#if (MIU_BW_BANK_NUM > 1)
        iMiuBankAddr_1     = BASE_REG_MIU_MISC_CH1;
#endif
        pstMiuClient       = &miu0_wr_clients[0];
        pstMiuRcmdClient   = &miu0_rd_clients[0];
    }
    else
    {
        return 0;
    }

    if (iMonitorDumpToFile)
    {
        u8Buf    = kmalloc(DUMP_FILE_TEMP_BUF_SZ, GFP_KERNEL);
        u8Ptr    = u8Buf;
        u8PtrEnd = u8Buf + (DUMP_FILE_TEMP_BUF_SZ);

        pstOutFd = miu_bw_open_file(m_bOutputFilePath, O_WRONLY | O_CREAT, 0644);
    }

    if (iMonitorOutputKmsg)
    {
        halBWGetMeasureHeader(measure_header, MIU_BW_HDR_DDRINFO, MIU_CH_NONE);
        printk("%s\r\n", measure_header);
        printk("---------------------------------------------------------\r\n");
    }
    else
    {
        halBWGetMeasureHeader(measure_header, MIU_BW_HDR_DDRINFO, MIU_CH_NONE);
        str += scnprintf(str, end - str, "%s\r\n", measure_header);
        str += scnprintf(str, end - str, "---------------------------------------------------------\r\n");
    }

    for (i = 0; i < iMiuClientNum; i++)
    {
        // printk("%s %d i=%d, wr%d, rd%d\r\n", __FUNCTION__, __LINE__, i, (pstMiuClient + i)->bw_enabled,
        // (pstMiuRcmdClient + i)->bw_enabled);
        if (((pstMiuClient + i)->bw_enabled && ((pstMiuClient + i)->bw_rsvd == 0))
            || ((pstMiuRcmdClient + i)->bw_enabled && ((pstMiuRcmdClient + i)->bw_rsvd == 0)))
        {
            if (i > 0)
            {
                if (((pstMiuClient + i)->bw_enabled && ((pstMiuClient + i)->bw_rsvd == 0))
                    && ((pstMiuRcmdClient + i)->bw_enabled && ((pstMiuRcmdClient + i)->bw_rsvd == 0)))
                {
                    id      = (pstMiuClient + i)->bw_client_id;
                    bwch_en = MIU_CH_RWCMD;
                }
                else if ((pstMiuClient + i)->bw_enabled && ((pstMiuClient + i)->bw_rsvd == 0))
                {
                    id      = (pstMiuClient + i)->bw_client_id;
                    bwch_en = MIU_CH_WCMD;
                }
                else
                {
                    id      = (pstMiuRcmdClient + i)->bw_client_id;
                    bwch_en = MIU_CH_RCMD;
                }
            }
            else
            {
                id      = (pstMiuClient + i)->bw_client_id;
                bwch_en = MIU_CH_RWCMD;
            }
            // bandwidth report for overall
            id = id & 0x1FF;

            halBWMeasureTrigger(MIU_BW_HDR_DDRINFO, id, MIU_CH_NONE);

            msleep(iMonitorDuration);

            if (iMonitorOutputKmsg)
            {
                halBWGetMeasureStat(0, measure_header, 128, MIU_BW_HDR_DDRINFO, id, MIU_CH_NONE);
                if (!id)
                    printk("0:OVERALL_RW\t%s\n", measure_header);
                else
                {
                    if ((bwch_en == MIU_CH_RWCMD) || (bwch_en == MIU_CH_WCMD))
                        printk("%d:%s\t%s\n", id, (pstMiuClient + i)->name, measure_header);
                    if ((bwch_en == MIU_CH_RWCMD) || (bwch_en == MIU_CH_RCMD))
                        printk("%d:%s\t%s\n", id, (pstMiuRcmdClient + i)->name, measure_header);
                }
                memset(measure_header, 0, sizeof(measure_header));
#if (MIU_BW_BANK_NUM > 1)
                halBWGetMeasureStat(1, measure_header, 128, MIU_BW_HDR_DDRINFO, id, MIU_CH_NONE);
                if (!id)
                    printk("0:OVERALL_RW\t%s\n", measure_header);
                else
                {
                    if ((bwch_en == MIU_CH_RWCMD) || (bwch_en == MIU_CH_WCMD))
                        printk("%d:%s\t%s\n", id, (pstMiuClient + i)->name, measure_header);
                    if ((bwch_en == MIU_CH_RWCMD) || (bwch_en == MIU_CH_RCMD))
                        printk("%d:%s\t%s\n", id, (pstMiuRcmdClient + i)->name, measure_header);
                }
#endif
            }
            else
            {
                halBWGetMeasureStat(0, measure_header, 128, MIU_BW_HDR_DDRINFO, id, MIU_CH_NONE);
                if (!id)
                    str += scnprintf(str, end - str, "0:OVERALL_RW\t%s\n", measure_header);
                else
                {
                    if ((bwch_en == MIU_CH_RWCMD) || (bwch_en == MIU_CH_WCMD))
                        str += scnprintf(str, end - str, "%d:%s\t%s\n", id, (pstMiuClient + i)->name, measure_header);
                    if ((bwch_en == MIU_CH_RWCMD) || (bwch_en == MIU_CH_WCMD))
                        str +=
                            scnprintf(str, end - str, "%d:%s\t%s\n", id, (pstMiuRcmdClient + i)->name, measure_header);
                }
                memset(measure_header, 0, sizeof(measure_header));
#if (MIU_BW_BANK_NUM > 1)
                halBWGetMeasureStat(1, measure_header, 128, MIU_BW_HDR_DDRINFO, id, MIU_CH_NONE);
                if (!id)
                    str += scnprintf(str, end - str, "0:OVERALL_RW\t%s\n", measure_header);
                else
                {
                    if ((bwch_en == MIU_CH_RWCMD) || (bwch_en == MIU_CH_WCMD))
                        str += scnprintf(str, end - str, "%d:%s\t%s\n", id, (pstMiuClient + i)->name, measure_header);
                    if ((bwch_en == MIU_CH_RWCMD) || (bwch_en == MIU_CH_WCMD))
                        str +=
                            scnprintf(str, end - str, "%d:%s\t%s\n", id, (pstMiuRcmdClient + i)->name, measure_header);
                }
#endif
            }

            halBWResetFunc(iMiuBankAddr, MIU_CH_NONE);   // reset all
#if (MIU_BW_BANK_NUM > 1)
            halBWResetFunc(iMiuBankAddr_1, MIU_CH_NONE); // reset all
#endif
        }
    }

    if (iMonitorDumpToFile)
    {
        if (pstOutFd)
        {
            miu_bw_write_file(pstOutFd, u8Buf, u8Ptr - u8Buf);
            miu_bw_close_file(pstOutFd);
        }
        kfree(u8Buf);
    }

    if (str > buf)
        str--;

    str += scnprintf(str, end - str, "\n");
    return (str - buf);
}

static ssize_t measure_all_hw_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *             str = buf;
    char *             end = buf + PAGE_SIZE;
    int                i   = 0;
    int                id;
    int                iMiuClientNum      = 0;
    int                iMonitorDuration   = 0;
    int                iMonitorOutputKmsg = 1;
    int                iMonitorDumpToFile = 0;
    int                iMiuBankAddr       = 0;
    struct miu_client *pstMiuClient       = NULL;
    // short effi_last = 0;
    struct miu_client *pstMiuRcmdClient = NULL;
    // short rcmd_effi_last = 0;
    char *       u8Buf    = NULL;
    char *       u8Ptr    = NULL;
    char *       u8PtrEnd = NULL;
    struct file *pstOutFd = NULL;

    if ('0' == (dev->kobj.name[6]))
    {
        iMiuClientNum      = MIU0_CLIENT_WR_NUM;
        iMonitorDuration   = gmonitor_duration[0];
        iMonitorOutputKmsg = gmonitor_output_kmsg[0];
        iMiuBankAddr       = BASE_REG_MIU_PA;
        pstMiuClient       = &miu0_wr_clients[0];
        pstMiuRcmdClient   = &miu0_rd_clients[0];
    }
    else
    {
        return 0;
    }

    for (i = 0; i < iMiuClientNum; i++)
    {
        (pstMiuClient + i)->effi_min = 0x3FF;
        (pstMiuClient + i)->effi_avg = 0;
        (pstMiuClient + i)->effi_max = 0;
        (pstMiuClient + i)->bw_avg   = 0;
        (pstMiuClient + i)->bw_max   = 0;
        (pstMiuClient + i)->bw_min   = 0x3FF;

        if ((pstMiuClient + i)->bw_dump_en)
        {
            iMonitorDumpToFile = 1;
        }
    }

    if (iMonitorDumpToFile)
    {
        u8Buf    = kmalloc(DUMP_FILE_TEMP_BUF_SZ, GFP_KERNEL);
        u8Ptr    = u8Buf;
        u8PtrEnd = u8Buf + (DUMP_FILE_TEMP_BUF_SZ);

        pstOutFd = miu_bw_open_file(m_bOutputFilePath, O_WRONLY | O_CREAT, 0644);
    }

    // calculate overall EFFI/BW
    if (iMonitorOutputKmsg)
    {
        printk("Num:client\t\tEFFI\tBWavg\tBWmax\tBWavg/E\tBWmax/E\n");
        printk("---------------------------------------------------------\n");
    }
    else
    {
        str += scnprintf(str, end - str, "Num:client\t\tEFFI\tBWavg\tBWmax\tBWavg/E\tBWmax/E\n");
        str += scnprintf(str, end - str, "---------------------------------------------------------\n");
    }
    id = 0;
    if (((pstMiuClient + id)->bw_enabled) && ((pstMiuClient + id)->bw_rsvd == 0))
    {
        halBWInit(iMiuBankAddr, id, MIU_CH_WCMD);

        halBWEffiMinPerConfig(iMiuBankAddr, id, MIU_CH_WCMD);

        msleep(iMonitorDuration);
        (pstMiuClient + id)->effi_avg = halBWReadBus(iMiuBankAddr, MIU_BW_WCMD_EFFI, id);

        halBWEffiAvgPerConfig(iMiuBankAddr, id, MIU_CH_WCMD);
        // msleep(iMonitorDuration);
        (pstMiuClient + id)->bw_avg = halBWReadBus(iMiuBankAddr, MIU_BW_WCMD_AVG, id);

        halBWEffiMaxPerConfig(iMiuBankAddr, id, MIU_CH_WCMD);
        // msleep(iMonitorDuration);
        (pstMiuClient + id)->bw_max = halBWReadBus(iMiuBankAddr, MIU_BW_WCMD_MAX, id);

        halBWOCCRealPerConfig(iMiuBankAddr, id, MIU_CH_WCMD);
        // msleep(iMonitorDuration);
        (pstMiuClient + id)->bw_avg_div_effi = halBWReadBus(iMiuBankAddr, MIU_BW_WCMD_AVG_EFFI, id);

        halBWOCCMaxPerConfig(iMiuBankAddr, id, MIU_CH_WCMD);
        // msleep(iMonitorDuration);
        (pstMiuClient + id)->bw_max_div_effi = halBWReadBus(iMiuBankAddr, MIU_BW_WCMD_MAX_EFFI, id);
        // halBWResetFunc(iMiuBankAddr, MIU_CH_WCMD); // reset all

        if (iMonitorOutputKmsg)
        {
            printk("%d:%s\t%2d.%02d%%\t%02d.%02d%%\t%02d.%02d%%\t%02d.%02d%%\t%02d.%02d%%\n", (short)id,
                   (pstMiuClient + id)->name,
                   halBWReadPercentValue(iMiuBankAddr, MIU_BW_WCMD_EFFI, (pstMiuClient + id)->effi_avg, 0),
                   halBWReadPercentValue(iMiuBankAddr, MIU_BW_WCMD_EFFI, (pstMiuClient + id)->effi_avg, 1),
                   halBWReadPercentValue(iMiuBankAddr, MIU_BW_WCMD_AVG, (pstMiuClient + id)->bw_avg, 0),
                   halBWReadPercentValue(iMiuBankAddr, MIU_BW_WCMD_AVG, (pstMiuClient + id)->bw_avg, 1),
                   halBWReadPercentValue(iMiuBankAddr, MIU_BW_WCMD_MAX, (pstMiuClient + id)->bw_max, 0),
                   halBWReadPercentValue(iMiuBankAddr, MIU_BW_WCMD_MAX, (pstMiuClient + id)->bw_max, 1),
                   halBWReadPercentValue(iMiuBankAddr, MIU_BW_WCMD_AVG_EFFI, (pstMiuClient + id)->bw_avg_div_effi, 0),
                   halBWReadPercentValue(iMiuBankAddr, MIU_BW_WCMD_AVG_EFFI, (pstMiuClient + id)->bw_avg_div_effi, 1),
                   halBWReadPercentValue(iMiuBankAddr, MIU_BW_WCMD_MAX_EFFI, (pstMiuClient + id)->bw_max_div_effi, 0),
                   halBWReadPercentValue(iMiuBankAddr, MIU_BW_WCMD_MAX_EFFI, (pstMiuClient + id)->bw_max_div_effi, 1));
        }
        else
        {
            str += scnprintf(
                str, end - str, "%d:%s\t%2d.%02d%%\t%02d.%02d%%\t%02d.%02d%%\t%02d.%02d%%\t%02d.%02d%%\n", (short)id,
                (pstMiuClient + id)->name,
                halBWReadPercentValue(iMiuBankAddr, MIU_BW_WCMD_EFFI, (pstMiuClient + id)->effi_avg, 0),
                halBWReadPercentValue(iMiuBankAddr, MIU_BW_WCMD_EFFI, (pstMiuClient + id)->effi_avg, 1),
                halBWReadPercentValue(iMiuBankAddr, MIU_BW_WCMD_AVG, (pstMiuClient + id)->bw_avg, 0),
                halBWReadPercentValue(iMiuBankAddr, MIU_BW_WCMD_AVG, (pstMiuClient + id)->bw_avg, 1),
                halBWReadPercentValue(iMiuBankAddr, MIU_BW_WCMD_MAX, (pstMiuClient + id)->bw_max, 0),
                halBWReadPercentValue(iMiuBankAddr, MIU_BW_WCMD_MAX, (pstMiuClient + id)->bw_max, 1),
                halBWReadPercentValue(iMiuBankAddr, MIU_BW_WCMD_AVG_EFFI, (pstMiuClient + id)->bw_avg_div_effi, 0),
                halBWReadPercentValue(iMiuBankAddr, MIU_BW_WCMD_AVG_EFFI, (pstMiuClient + id)->bw_avg_div_effi, 1),
                halBWReadPercentValue(iMiuBankAddr, MIU_BW_WCMD_MAX_EFFI, (pstMiuClient + id)->bw_max_div_effi, 0),
                halBWReadPercentValue(iMiuBankAddr, MIU_BW_WCMD_MAX_EFFI, (pstMiuClient + id)->bw_max_div_effi, 1));
        }
        halBWResetFunc(iMiuBankAddr, MIU_CH_WCMD); // reset all
    }

    if (((pstMiuRcmdClient + id)->bw_enabled) && ((pstMiuRcmdClient + id)->bw_rsvd == 0))
    {
        halBWInit(iMiuBankAddr, id, MIU_CH_RCMD);

        halBWEffiMinPerConfig(iMiuBankAddr, id, MIU_CH_RCMD);

        msleep(iMonitorDuration);
        (pstMiuRcmdClient + id)->effi_avg = halBWReadBus(iMiuBankAddr, MIU_BW_RCMD_EFFI, id);

        halBWEffiAvgPerConfig(iMiuBankAddr, id, MIU_CH_RCMD);
        // msleep(iMonitorDuration);
        (pstMiuRcmdClient + id)->bw_avg = halBWReadBus(iMiuBankAddr, MIU_BW_RCMD_AVG, id);

        halBWEffiMaxPerConfig(iMiuBankAddr, id, MIU_CH_RCMD);
        // msleep(iMonitorDuration);
        (pstMiuRcmdClient + id)->bw_max = halBWReadBus(iMiuBankAddr, MIU_BW_RCMD_MAX, id);

        halBWOCCRealPerConfig(iMiuBankAddr, id, MIU_CH_RCMD);
        // msleep(iMonitorDuration);
        (pstMiuRcmdClient + id)->bw_avg_div_effi = halBWReadBus(iMiuBankAddr, MIU_BW_RCMD_AVG_EFFI, id);

        halBWOCCMaxPerConfig(iMiuBankAddr, id, MIU_CH_RCMD);
        // msleep(iMonitorDuration);
        (pstMiuRcmdClient + id)->bw_max_div_effi = halBWReadBus(iMiuBankAddr, MIU_BW_RCMD_MAX_EFFI, id);
        // halBWResetFunc(iMiuBankAddr, MIU_CH_WCMD); // reset all

        if (iMonitorOutputKmsg)
        {
            printk(
                "%d:%s\t%2d.%02d%%\t%02d.%02d%%\t%02d.%02d%%\t%02d.%02d%%\t%02d.%02d%%\n", (short)id,
                (pstMiuRcmdClient + id)->name,
                halBWReadPercentValue(iMiuBankAddr, MIU_BW_RCMD_EFFI, (pstMiuRcmdClient + id)->effi_avg, 0),
                halBWReadPercentValue(iMiuBankAddr, MIU_BW_RCMD_EFFI, (pstMiuRcmdClient + id)->effi_avg, 1),
                halBWReadPercentValue(iMiuBankAddr, MIU_BW_RCMD_AVG, (pstMiuRcmdClient + id)->bw_avg, 0),
                halBWReadPercentValue(iMiuBankAddr, MIU_BW_RCMD_AVG, (pstMiuRcmdClient + id)->bw_avg, 1),
                halBWReadPercentValue(iMiuBankAddr, MIU_BW_RCMD_MAX, (pstMiuRcmdClient + id)->bw_max, 0),
                halBWReadPercentValue(iMiuBankAddr, MIU_BW_RCMD_MAX, (pstMiuRcmdClient + id)->bw_max, 1),
                halBWReadPercentValue(iMiuBankAddr, MIU_BW_RCMD_AVG_EFFI, (pstMiuRcmdClient + id)->bw_avg_div_effi, 0),
                halBWReadPercentValue(iMiuBankAddr, MIU_BW_RCMD_AVG_EFFI, (pstMiuRcmdClient + id)->bw_avg_div_effi, 1),
                halBWReadPercentValue(iMiuBankAddr, MIU_BW_RCMD_MAX_EFFI, (pstMiuRcmdClient + id)->bw_max_div_effi, 0),
                halBWReadPercentValue(iMiuBankAddr, MIU_BW_RCMD_MAX_EFFI, (pstMiuRcmdClient + id)->bw_max_div_effi, 1));
        }
        else
        {
            str += scnprintf(
                str, end - str, "%d:%s\t%2d.%02d%%\t%02d.%02d%%\t%02d.%02d%%\t%02d.%02d%%\t%02d.%02d%%\n", (short)id,
                (pstMiuRcmdClient + id)->name,
                halBWReadPercentValue(iMiuBankAddr, MIU_BW_RCMD_EFFI, (pstMiuRcmdClient + id)->effi_avg, 0),
                halBWReadPercentValue(iMiuBankAddr, MIU_BW_RCMD_EFFI, (pstMiuRcmdClient + id)->effi_avg, 1),
                halBWReadPercentValue(iMiuBankAddr, MIU_BW_RCMD_AVG, (pstMiuRcmdClient + id)->bw_avg, 0),
                halBWReadPercentValue(iMiuBankAddr, MIU_BW_RCMD_AVG, (pstMiuRcmdClient + id)->bw_avg, 1),
                halBWReadPercentValue(iMiuBankAddr, MIU_BW_RCMD_MAX, (pstMiuRcmdClient + id)->bw_max, 0),
                halBWReadPercentValue(iMiuBankAddr, MIU_BW_RCMD_MAX, (pstMiuRcmdClient + id)->bw_max, 1),
                halBWReadPercentValue(iMiuBankAddr, MIU_BW_RCMD_AVG_EFFI, (pstMiuRcmdClient + id)->bw_avg_div_effi, 0),
                halBWReadPercentValue(iMiuBankAddr, MIU_BW_RCMD_AVG_EFFI, (pstMiuRcmdClient + id)->bw_avg_div_effi, 1),
                halBWReadPercentValue(iMiuBankAddr, MIU_BW_RCMD_MAX_EFFI, (pstMiuRcmdClient + id)->bw_max_div_effi, 0),
                halBWReadPercentValue(iMiuBankAddr, MIU_BW_RCMD_MAX_EFFI, (pstMiuRcmdClient + id)->bw_max_div_effi, 1));
        }
        halBWResetFunc(iMiuBankAddr, MIU_CH_RCMD); // reset all
    }

    // calculate bandwidth access report
    if (iMonitorOutputKmsg)
    {
        printk("Num:client\tCMD\tACT\tR/W2P\tRWT\tBWavg\tBWmax\n");
        printk("---------------------------------------------------------\n");
    }
    else
    {
        str += scnprintf(str, end - str, "Num:client\tCMD\tACT\tR/W2P\tRWT\tBWavg\tBWmax\n");
        str += scnprintf(str, end - str, "---------------------------------------------------------\n");
    }

    for (i = 0; i < iMiuClientNum; i++)
    {
        // printk("%d, name=%s %d, %d\r\n", __LINE__, (pstMiuClient + i)->name, (pstMiuClient + i)->bw_enabled,
        // (pstMiuClient + i)->bw_rsvd);
        if ((pstMiuClient + i)->bw_enabled && (pstMiuClient + i)->bw_rsvd == 0)
        {
            id = (pstMiuClient + i)->bw_client_id;
            halBWInit(iMiuBankAddr, id, MIU_CH_WCMD);
            // bandwidth for wcmd
            id = id & 0x1FF;

            halBWEffiMinPerConfig(iMiuBankAddr, id, MIU_CH_WCMD);

            msleep(iMonitorDuration);
#if 0
                for (j=0x60; j<0x80; j++) {
                    printk("%d %X: %X\r\n", __LINE__, j, INREG16(BASE_REG_MIU_MISC_CH0+(j << 2)));
                }
#endif

            if (iMonitorOutputKmsg)
            {
                printk("%d:%s\t%02d\t%02d\t%02d\t%02d\t%02d.%02d%%\t%02d.%02d%%\n",
                       (short)(pstMiuClient + i)->bw_client_id, (pstMiuClient + i)->name,
                       halBWReadPercentValue(iMiuBankAddr, MIU_BW_WCMD_BW_CMD, 0, 0),
                       halBWReadPercentValue(iMiuBankAddr, MIU_BW_WCMD_BW_ACT, 0, 0),
                       halBWReadPercentValue(iMiuBankAddr, MIU_BW_WCMD_BW_W2P, 0, 0),
                       halBWReadPercentValue(iMiuBankAddr, MIU_BW_WCMD_BW_RWT, 0, 0),
                       halBWReadPercentValue(iMiuBankAddr, MIU_BW_WCMD_BW_AVG, 0, 0),
                       halBWReadPercentValue(iMiuBankAddr, MIU_BW_WCMD_BW_AVG, 0, 1),
                       halBWReadPercentValue(iMiuBankAddr, MIU_BW_WCMD_BW_MAX, 0, 0),
                       halBWReadPercentValue(iMiuBankAddr, MIU_BW_WCMD_BW_MAX, 0, 1));
            }
            else
            {
                str += scnprintf(str, end - str, "%d:%s\t%02d\t%02d\t%02d\t%02d\t%02d.%02d%%\t%02d.%02d%%\n",
                                 (short)(pstMiuClient + i)->bw_client_id, (pstMiuClient + i)->name,
                                 halBWReadPercentValue(iMiuBankAddr, MIU_BW_WCMD_BW_CMD, 0, 0),
                                 halBWReadPercentValue(iMiuBankAddr, MIU_BW_WCMD_BW_ACT, 0, 0),
                                 halBWReadPercentValue(iMiuBankAddr, MIU_BW_WCMD_BW_W2P, 0, 0),
                                 halBWReadPercentValue(iMiuBankAddr, MIU_BW_WCMD_BW_RWT, 0, 0),
                                 halBWReadPercentValue(iMiuBankAddr, MIU_BW_WCMD_BW_AVG, 0, 0),
                                 halBWReadPercentValue(iMiuBankAddr, MIU_BW_WCMD_BW_AVG, 0, 1),
                                 halBWReadPercentValue(iMiuBankAddr, MIU_BW_WCMD_BW_MAX, 0, 0),
                                 halBWReadPercentValue(iMiuBankAddr, MIU_BW_WCMD_BW_MAX, 0, 1));
            }
            halBWResetFunc(iMiuBankAddr, MIU_CH_WCMD); // reset all
        }

        // bandwidth for rcmd
        // printk("%d, name=%s %d, %d\r\n", __LINE__, (pstMiuRcmdClient + i)->name, (pstMiuRcmdClient + i)->bw_enabled,
        // (pstMiuRcmdClient + i)->bw_rsvd);
        if ((pstMiuRcmdClient + i)->bw_enabled && (pstMiuRcmdClient + i)->bw_rsvd == 0)
        {
            id = (pstMiuRcmdClient + i)->bw_client_id;
            halBWInit(iMiuBankAddr, id, MIU_CH_RCMD);
            id = id & 0x1FF;

            halBWEffiMinPerConfig(iMiuBankAddr, id, MIU_CH_RCMD);

            msleep(iMonitorDuration);
#if 0
                for (j=0x60; j<0x80; j++) {
                    printk("%d %X: %X\r\n", __LINE__, j, INREG16(BASE_REG_MIU_MISC_CH0+(j << 2)));
                }
#endif

            if (iMonitorOutputKmsg)
            {
                printk("%d:%s\t%02d\t%02d\t%02d\t%02d\t%02d.%02d%%\t%02d.%02d%%\n",
                       (short)(pstMiuRcmdClient + i)->bw_client_id, (pstMiuRcmdClient + i)->name,
                       halBWReadPercentValue(iMiuBankAddr, MIU_BW_RCMD_BW_CMD, 0, 0),
                       halBWReadPercentValue(iMiuBankAddr, MIU_BW_RCMD_BW_ACT, 0, 0),
                       halBWReadPercentValue(iMiuBankAddr, MIU_BW_RCMD_BW_R2P, 0, 0),
                       halBWReadPercentValue(iMiuBankAddr, MIU_BW_RCMD_BW_RWT, 0, 0),
                       halBWReadPercentValue(iMiuBankAddr, MIU_BW_RCMD_BW_AVG, 0, 0),
                       halBWReadPercentValue(iMiuBankAddr, MIU_BW_RCMD_BW_AVG, 0, 1),
                       halBWReadPercentValue(iMiuBankAddr, MIU_BW_RCMD_BW_MAX, 0, 0),
                       halBWReadPercentValue(iMiuBankAddr, MIU_BW_RCMD_BW_MAX, 0, 1));
            }
            else
            {
                str += scnprintf(str, end - str, "%d:%s\t%02d\t%02d\t%02d\t%02d\t%02d.%02d%%\t%02d.%02d%%\n",
                                 (short)(pstMiuRcmdClient + i)->bw_client_id, (pstMiuRcmdClient + i)->name,
                                 halBWReadPercentValue(iMiuBankAddr, MIU_BW_RCMD_BW_CMD, 0, 0),
                                 halBWReadPercentValue(iMiuBankAddr, MIU_BW_RCMD_BW_ACT, 0, 0),
                                 halBWReadPercentValue(iMiuBankAddr, MIU_BW_RCMD_BW_R2P, 0, 0),
                                 halBWReadPercentValue(iMiuBankAddr, MIU_BW_RCMD_BW_RWT, 0, 0),
                                 halBWReadPercentValue(iMiuBankAddr, MIU_BW_RCMD_BW_AVG, 0, 0),
                                 halBWReadPercentValue(iMiuBankAddr, MIU_BW_RCMD_BW_AVG, 0, 1),
                                 halBWReadPercentValue(iMiuBankAddr, MIU_BW_RCMD_BW_MAX, 0, 0),
                                 halBWReadPercentValue(iMiuBankAddr, MIU_BW_RCMD_BW_MAX, 0, 1));
            }
            halBWResetFunc(iMiuBankAddr, MIU_CH_RCMD); // reset all
        }
    }

    if (iMonitorDumpToFile)
    {
        if (pstOutFd)
        {
            miu_bw_write_file(pstOutFd, u8Buf, u8Ptr - u8Buf);
            miu_bw_close_file(pstOutFd);
        }
        kfree(u8Buf);
    }

    if (str > buf)
        str--;

    str += scnprintf(str, end - str, "\n");
    return (str - buf);
}
#endif

//***ToDo-----I6 Diff*/
#ifdef CONFIG_CAM_CLK
static ssize_t dram_info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *       str = buf;
    char *       end = buf + PAGE_SIZE;
    unsigned int dram_freq, miupll_freq;

    dram_freq   = CamClkRateGet(CAMCLK_ddrpll_clk) / 1000000;
    miupll_freq = CamClkRateGet(CAMCLK_miupll_clk) / 1000000;

    return halBWGetDramInfo(str, end, dram_freq, miupll_freq);
}
#else // CONFIG_CAM_CLK
static ssize_t dram_info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    if ('0' != (dev->kobj.name[6]))
    {
        return 0;
    }

    return halBWGetDramInfo(str, end);
}
#endif

#ifdef CONFIG_PM_SLEEP
int miu_subsys_suspend(struct device *dev)
{
    if (dev == &miu0.dev)
    {
        // keep dram size setting
        miu0.reg_dram_size = INREGMSK16(BASE_REG_MIU_PA + REG_ID_69, 0xF000);
        pr_debug("miu subsys suspend %d\n", miu0.reg_dram_size);
    }

    pr_debug("miu subsys suspend %s\n", dev->kobj.name);
    return 0;
}

int miu_subsys_resume(struct device *dev)
{
    if (dev == &miu0.dev)
    {
        // restore dram size setting
        OUTREGMSK16(BASE_REG_MIU_PA + REG_ID_69, miu0.reg_dram_size, 0xF000);
        pr_debug("restore dram size setting\n");
    }
    else if (strncmp(dev->kobj.name, "miu_arb0", 9) == 0)
    {
        pr_debug("miu_arb_resume \n");
        miu_arb_resume();
    }

    pr_debug("miu subsys resume %s\n", dev->kobj.name);
    return 0;
}
#endif

DEVICE_ATTR(monitor_wcmd_client_enable, 0644, monitor_wcmd_client_enable_show, monitor_wcmd_client_enable_store);
DEVICE_ATTR(monitor_wcmd_client_disable, 0644, monitor_wcmd_client_disable_show, monitor_wcmd_client_disable_store);
DEVICE_ATTR(monitor_rcmd_client_enable, 0644, monitor_rcmd_client_enable_show, monitor_rcmd_client_enable_store);
DEVICE_ATTR(monitor_rcmd_client_disable, 0644, monitor_rcmd_client_disable_show, monitor_rcmd_client_disable_store);
DEVICE_ATTR(monitor_set_duration_ms, 0644, monitor_set_counts_avg_show, monitor_set_counts_avg_store);
DEVICE_ATTR(measure_all_hw, 0644, measure_all_hw_show, measure_all_store);
DEVICE_ATTR(measure_bw_overall, 0644, measure_bw_overall_show, measure_bw_overall_store);
#ifdef MIU_BW_SYNOPSYS
DEVICE_ATTR(measure_ba_report, 0444, measure_ba_report_show, NULL);
DEVICE_ATTR(measure_rd_latency, 0444, measure_rd_latency_show, NULL);
#endif
DEVICE_ATTR(dram_info, 0444, dram_info_show, NULL);
#ifdef CONFIG_MIU_ARBITRATION
extern void create_miu_arb_node(struct bus_type *miu_subsys);
#endif
#ifdef CONFIG_MIU_PROTECT_SYSFS
extern void create_miu_protect_node(struct bus_type *miu_subsys);
#endif
extern void create_miu_synopsys_node(struct bus_type *miu_subsys);
void        mstar_create_MIU_node(void)
{
    int ret = 0;

    miu0.index         = 0;
    miu0.dev.kobj.name = "miu_bw0";
    miu0.dev.bus       = &miu_subsys;

    miu0_wr_clients[0].bw_enabled = 1;
    miu0_rd_clients[0].bw_enabled = 1;

    ret = subsys_system_register(&miu_subsys, NULL);
    if (ret)
    {
        printk(KERN_ERR "Failed to register miu sub system!! %d\n", ret);
        return;
    }

    ret = device_register(&miu0.dev);
    if (ret)
    {
        printk(KERN_ERR "Failed to register miu0 device!! %d\n", ret);
        return;
    }
#ifdef MIU_ARBITRATION
    create_miu_arb_node(&miu_subsys);
#endif
#ifdef CONFIG_MIU_PROTECT_SYSFS
    create_miu_protect_node(&miu_subsys);
#endif
    create_miu_synopsys_node(&miu_subsys);

    device_create_file(&miu0.dev, &dev_attr_monitor_wcmd_client_enable);
    device_create_file(&miu0.dev, &dev_attr_monitor_wcmd_client_disable);
    device_create_file(&miu0.dev, &dev_attr_monitor_rcmd_client_enable);
    device_create_file(&miu0.dev, &dev_attr_monitor_rcmd_client_disable);

    device_create_file(&miu0.dev, &dev_attr_monitor_set_duration_ms);
    device_create_file(&miu0.dev, &dev_attr_measure_all_hw);
    device_create_file(&miu0.dev, &dev_attr_measure_bw_overall);
#ifdef MIU_BW_SYNOPSYS
    device_create_file(&miu0.dev, &dev_attr_measure_ba_report);
    device_create_file(&miu0.dev, &dev_attr_measure_rd_latency);
#endif
    device_create_file(&miu0.dev, &dev_attr_dram_info);
}
