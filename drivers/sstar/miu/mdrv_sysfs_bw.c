/*
 * mdrv_sysfs_bw.c- Sigmastar
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

//#define CONFIG_MIU_BW_SW_MODE
//#define CONFIG_MIU_BW_DUMP_TO_FILE

static struct miu_device miu0;
#if MIU_NUM > 1
static struct miu_device miu1;
#endif
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

static int gmonitor_interval[MIU_NUM] = {INTERVAL};
static int gmonitor_duration[MIU_NUM] = {DURATION};
#ifdef CONFIG_MIU_BW_TO_KMSG_OR_SYSFS
static int gmonitor_output_kmsg[MIU_NUM] = {KMSG};
#endif
#ifdef CONFIG_MIU_BW_DUMP_TO_FILE
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
#endif

static int set_miu_client_enable(struct device *dev, const char *buf, size_t n, int enabled)
{
    long idx = -1;

    if ('0' == (dev->kobj.name[6]))
    {
        if (kstrtol(buf, 10, &idx) != 0 || idx < 0 || idx >= MIU0_CLIENT_NUM)
        {
            return -EINVAL;
        }

        if (miu0_clients[idx].bw_rsvd == 0)
            miu0_clients[idx].bw_enabled = enabled;
    }
#if MIU_NUM > 1
    else if ('1' == (dev->kobj.name[6]))
    {
        if (kstrtol(buf, 10, &idx) != 0 || idx < 0 || idx >= MIU1_CLIENT_NUM)
        {
            return -EINVAL;
        }

        if (miu1_clients[idx].bw_rsvd == 0)
            miu1_clients[idx].bw_enabled = enabled;
    }
#endif

    return n;
}

#ifdef CONFIG_MIU_BW_DUMP_TO_FILE
static int set_miu_client_dump_enable(struct device *dev, const char *buf, size_t n, int enabled)
{
    long idx = -1;

    if ('0' == (dev->kobj.name[6]))
    {
        if (kstrtol(buf, 10, &idx) != 0 || idx < 0 || idx >= MIU0_CLIENT_NUM)
        {
            return -EINVAL;
        }

        if (miu0_clients[idx].bw_rsvd == 0)
            miu0_clients[idx].bw_dump_en = enabled;
    }
#if MIU_NUM > 1
    else if ('1' == (dev->kobj.name[6]))
    {
        if (kstrtol(buf, 10, &idx) != 0 || idx < 0 || idx >= MIU1_CLIENT_NUM)
        {
            return -EINVAL;
        }

        if (miu1_clients[idx].bw_rsvd == 0)
            miu1_clients[idx].bw_dump_en = enabled;
    }
#endif

    return n;
}
#endif

static int set_miu_client_filter_enable(struct device *dev, const char *buf, size_t n, int enabled)
{
    long idx = -1;

    if ('0' == (dev->kobj.name[6]))
    {
        if (kstrtol(buf, 10, &idx) != 0 || idx < 0 || idx >= MIU0_CLIENT_NUM)
        {
            return -EINVAL;
        }

        if (miu0_clients[idx].bw_rsvd == 0)
            miu0_clients[idx].bw_filter_en = enabled;
    }
#if MIU_NUM > 1
    else if ('1' == (dev->kobj.name[6]))
    {
        if (kstrtol(buf, 10, &idx) != 0 || idx < 0 || idx >= MIU1_CLIENT_NUM)
        {
            return -EINVAL;
        }

        if (miu1_clients[idx].bw_rsvd == 0)
            miu1_clients[idx].bw_filter_en = enabled;
    }
#endif

    return n;
}

static ssize_t monitor_client_enable_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    int   i;
    char *pt;
    char *opt;

    if (!strncmp(buf, "all", strlen("all")))
    {
        if ('0' == (dev->kobj.name[6]))
        {
            for (i = 0; i < MIU0_CLIENT_NUM; i++)
            {
                if (miu0_clients[i].bw_rsvd == 0)
                    miu0_clients[i].bw_enabled = 1;
            }
        }
#if MIU_NUM > 1
        else if ('1' == (dev->kobj.name[6]))
        {
            for (i = 0; i < MIU1_CLIENT_NUM; i++)
            {
                if (miu1_clients[i].bw_rsvd == 0)
                    miu1_clients[i].bw_enabled = 1;
            }
        }
#endif

        return n;
    }

    pt = kmalloc(strlen(buf) + 1, GFP_KERNEL);
    strcpy(pt, buf);
    while ((opt = strsep(&pt, ";, ")) != NULL)
    {
        set_miu_client_enable(dev, opt, n, 1);
    }
    kfree(pt);

    return n;
}

static ssize_t monitor_client_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    int   i   = 0;

    if ('0' == (dev->kobj.name[6]))
    {
        str += scnprintf(str, end - str, "Num:IP_name\t[BW_Idx][Enable(1)/Disable(0)]\n");

        for (i = 0; i < MIU0_CLIENT_NUM; i++)
        {
            // if (miu0_clients[i].bw_enabled && !miu0_clients[i].bw_rsvd)
            {
                str += scnprintf(str, end - str, "%3d:%s\t[0x%02X][%d]\n", (short)i, miu0_clients[i].name,
                                 (short)miu0_clients[i].bw_client_id, (char)miu0_clients[i].bw_enabled);
            }
        }
    }
#if MIU_NUM > 1
    else if ('1' == (dev->kobj.name[6]))
    {
        str += scnprintf(str, end - str, "Num:IP_name\t[BW_Idx][Enable(1)/Disable(0)]\n");

        for (i = 0; i < MIU1_CLIENT_NUM; i++)
        {
            // if (miu1_clients[i].bw_enabled && !miu1_clients[i].bw_rsvd)
            {
                str += scnprintf(str, end - str, "%3d:%s\t[0x%02X][%d]\n", (short)i, miu1_clients[i].name,
                                 (short)miu1_clients[i].bw_client_id, (char)miu1_clients[i].bw_enabled);
            }
        }
    }
#endif

    if (str > buf)
        str--;

    str += scnprintf(str, end - str, "\n");

    return (str - buf);
}

static ssize_t monitor_client_disable_store(struct device *dev, struct device_attribute *attr, const char *buf,
                                            size_t n)
{
    int   i;
    char *pt;
    char *opt;

    if (!strncmp(buf, "all", strlen("all")))
    {
        if ('0' == (dev->kobj.name[6]))
        {
            for (i = 0; i < MIU0_CLIENT_NUM; i++)
            {
                if (miu0_clients[i].bw_rsvd == 0)
                    miu0_clients[i].bw_enabled = 0;
            }
        }
#if MIU_NUM > 1
        else if ('1' == (dev->kobj.name[6]))
        {
            for (i = 0; i < MIU1_CLIENT_NUM; i++)
            {
                if (miu1_clients[i].bw_rsvd == 0)
                    miu1_clients[i].bw_enabled = 0;
            }
        }
#endif

        return n;
    }

    pt = kmalloc(strlen(buf) + 1, GFP_KERNEL);
    strcpy(pt, buf);
    while ((opt = strsep(&pt, ";, ")) != NULL)
    {
        set_miu_client_enable(dev, opt, n, 0);
    }
    kfree(pt);

    return n;
}

static ssize_t monitor_client_disable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    int   i   = 0;

    if ('0' == (dev->kobj.name[6]))
    {
        str += scnprintf(str, end - str, "Num:IP_name\t[BW_Idx][Enable(1)/Disable(0)]\n");

        for (i = 0; i < MIU0_CLIENT_NUM; i++)
        {
            str += scnprintf(str, end - str, "%3d:%s\t[0x%02X][%d]\n", (short)i, miu0_clients[i].name,
                             (short)miu0_clients[i].bw_client_id, (char)miu0_clients[i].bw_enabled);
        }
    }
#if MIU_NUM > 1
    else if ('1' == (dev->kobj.name[6]))
    {
        str += scnprintf(str, end - str, "Num:IP_name\t[BW_Idx][Enable(1)/Disable(0)]\n");

        for (i = 0; i < MIU1_CLIENT_NUM; i++)
        {
            str += scnprintf(str, end - str, "%3d:%s\t[0x%02X][%d]\n", (short)i, miu1_clients[i].name,
                             (short)miu1_clients[i].bw_client_id, (char)miu1_clients[i].bw_enabled);
        }
    }
#endif

    if (str > buf)
        str--;

    str += scnprintf(str, end - str, "\n");

    return (str - buf);
}

static ssize_t monitor_set_interval_avg_store(struct device *dev, struct device_attribute *attr, const char *buf,
                                              size_t count)
{
    u32 input = 0;

    input = simple_strtoul(buf, NULL, 10);

    if ('0' == (dev->kobj.name[6]))
        gmonitor_interval[0] = input;
#if MIU_NUM > 1
    else if ('1' == (dev->kobj.name[6]))
        gmonitor_interval[1] = input;
#endif

    return count;
}

static ssize_t monitor_set_interval_avg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    if ('0' == (dev->kobj.name[6]))
        return sprintf(buf, "%d\n", gmonitor_interval[0]);
#if MIU_NUM > 1
    else if ('1' == (dev->kobj.name[6]))
        return sprintf(buf, "%d\n", gmonitor_interval[1]);
#endif
    else
        return 0;
}

static ssize_t monitor_set_counts_avg_store(struct device *dev, struct device_attribute *attr, const char *buf,
                                            size_t count)
{
    u32 input = 0;

    input = simple_strtoul(buf, NULL, 10);

    if ('0' == (dev->kobj.name[6]))
        gmonitor_duration[0] = input;
#if MIU_NUM > 1
    else if ('1' == (dev->kobj.name[6]))
        gmonitor_duration[1] = input;
#endif

    return count;
}

static ssize_t monitor_set_counts_avg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    if ('0' == (dev->kobj.name[6]))
        return sprintf(buf, "%d\n", gmonitor_duration[0]);
#if MIU_NUM > 1
    else if ('1' == (dev->kobj.name[6]))
        return sprintf(buf, "%d\n", gmonitor_duration[1]);
#endif
    else
        return 0;
}

#ifdef CONFIG_MIU_BW_DUMP_TO_FILE
static ssize_t monitor_client_dump_enable_store(struct device *dev, struct device_attribute *attr, const char *buf,
                                                size_t n)
{
    int   i;
    char *pt;
    char *opt;

    if (!strncmp(buf, "all", strlen("all")))
    {
        if ('0' == (dev->kobj.name[6]))
        {
            for (i = 0; i < MIU0_CLIENT_NUM; i++)
            {
                if (miu0_clients[i].bw_rsvd == 0)
                    miu0_clients[i].bw_dump_en = 1;
            }
        }
#if MIU_NUM > 1
        else if ('1' == (dev->kobj.name[6]))
        {
            for (i = 0; i < MIU1_CLIENT_NUM; i++)
            {
                if (miu1_clients[i].bw_rsvd == 0)
                    miu1_clients[i].bw_dump_en = 1;
            }
        }
#endif

        return n;
    }

    pt = kmalloc(strlen(buf) + 1, GFP_KERNEL);
    strcpy(pt, buf);
    while ((opt = strsep(&pt, ";, ")) != NULL)
    {
        set_miu_client_dump_enable(dev, opt, n, 1);
    }
    kfree(pt);

    return n;
}

static ssize_t monitor_client_dump_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    int   i   = 0;

    if ('0' == (dev->kobj.name[6]))
    {
        str += scnprintf(str, end - str, "Num:IP_name\t[BW_Idx][Dump Enable(1)/Disable(0)]\n");

        for (i = 0; i < MIU0_CLIENT_NUM; i++)
        {
            // if (miu0_clients[i].bw_enabled && !miu0_clients[i].bw_rsvd)
            {
                str += scnprintf(str, end - str, "%3d:%s\t[0x%02X][%d]\n", (short)i, miu0_clients[i].name,
                                 (short)miu0_clients[i].bw_client_id, (char)miu0_clients[i].bw_dump_en);
            }
        }
    }
#if MIU_NUM > 1
    else if ('1' == (dev->kobj.name[6]))
    {
        str += scnprintf(str, end - str, "Num:IP_name\t[BW_Idx][Dump Enable(1)/Disable(0)]\n");

        for (i = 0; i < MIU1_CLIENT_NUM; i++)
        {
            // if (miu1_clients[i].bw_enabled && !miu1_clients[i].bw_rsvd)
            {
                str += scnprintf(str, end - str, "%3d:%s\t[0x%02X][%d]\n", (short)i, miu1_clients[i].name,
                                 (short)miu1_clients[i].bw_client_id, (char)miu1_clients[i].bw_dump_en);
            }
        }
    }
#endif

    if (str > buf)
        str--;

    str += scnprintf(str, end - str, "\n");

    return (str - buf);
}
#endif

static ssize_t monitor_filter_abnormal_value_store(struct device *dev, struct device_attribute *attr, const char *buf,
                                                   size_t n)
{
    int   i;
    char *pt;
    char *opt;

    if (!strncmp(buf, "all", strlen("all")))
    {
        if ('0' == (dev->kobj.name[6]))
        {
            for (i = 0; i < MIU0_CLIENT_NUM; i++)
            {
                if (miu0_clients[i].bw_rsvd == 0)
                    miu0_clients[i].bw_filter_en = 1;
            }
        }
#if MIU_NUM > 1
        else if ('1' == (dev->kobj.name[6]))
        {
            for (i = 0; i < MIU1_CLIENT_NUM; i++)
            {
                if (miu1_clients[i].bw_rsvd == 0)
                    miu1_clients[i].bw_filter_en = 1;
            }
        }
#endif

        return n;
    }

    pt = kmalloc(strlen(buf) + 1, GFP_KERNEL);
    strcpy(pt, buf);
    while ((opt = strsep(&pt, ";, ")) != NULL)
    {
        set_miu_client_filter_enable(dev, opt, n, 1);
    }
    kfree(pt);

    return n;
}

static ssize_t monitor_filter_abnormal_value_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    int   i   = 0;

    if ('0' == (dev->kobj.name[6]))
    {
        str += scnprintf(str, end - str, "Num:IP_name\t[BW_Idx][Filter Enable(1)/Disable(0)]\n");

        for (i = 0; i < MIU0_CLIENT_NUM; i++)
        {
            // if (miu0_clients[i].bw_enabled && !miu0_clients[i].bw_rsvd)
            {
                str += scnprintf(str, end - str, "%3d:%s\t[0x%02X][%d]\n", (short)i, miu0_clients[i].name,
                                 (short)miu0_clients[i].bw_client_id, (char)miu0_clients[i].bw_filter_en);
            }
        }
    }
#if MIU_NUM > 1
    else if ('1' == (dev->kobj.name[6]))
    {
        str += scnprintf(str, end - str, "Num:IP_name\t[BW_Idx][Filter Enable(1)/Disable(0)]\n");

        for (i = 0; i < MIU1_CLIENT_NUM; i++)
        {
            // if (miu1_clients[i].bw_enabled && !miu1_clients[i].bw_rsvd)
            {
                str += scnprintf(str, end - str, "%3d:%s\t[0x%02X][%d]\n", (short)i, miu1_clients[i].name,
                                 (short)miu1_clients[i].bw_client_id, (char)miu1_clients[i].bw_filter_en);
            }
        }
    }
#endif

    if (str > buf)
        str--;

    str += scnprintf(str, end - str, "\n");

    return (str - buf);
}

#ifdef CONFIG_MIU_BW_TO_KMSG_OR_SYSFS
static ssize_t measure_all_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    u32 input = 0;

    input = simple_strtoul(buf, NULL, 10);

    if ('0' == (dev->kobj.name[6]))
        gmonitor_output_kmsg[0] = input;
#if MIU_NUM > 1
    else if ('1' == (dev->kobj.name[6]))
        gmonitor_output_kmsg[1] = input;
#endif

    return count;
}
#endif

#ifdef CONFIG_MIU_BW_SW_MODE
static ssize_t measure_all_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    int   i = 0, temp_loop_time = 0;
    int   id;
    int   iMiuClientNum    = 0;
    int   iMonitorInterval = 0;
    int   iMonitorDuration = 0;
#if defined(CONFIG_MIU_BW_TO_SYSFS)
    int iMonitorOutputKmsg = 0;
#else
    int iMonitorOutputKmsg = 1;
#endif
#ifdef CONFIG_MIU_BW_DUMP_TO_FILE
    int iMonitorDumpToFile = 0;
#endif
    int                iMiuBankAddr = 0;
    struct miu_client *pstMiuClient = NULL;

    short         temp_val = 0;
    unsigned long total_temp;
    unsigned long deadline;

#ifdef CONFIG_MIU_BW_DUMP_TO_FILE
    char *       u8Buf    = NULL;
    char *       u8Ptr    = NULL;
    char *       u8PtrEnd = NULL;
    struct file *pstOutFd = NULL;
#endif

    if ('0' == (dev->kobj.name[6]))
    {
        iMiuClientNum    = MIU0_CLIENT_NUM;
        iMonitorInterval = gmonitor_interval[0];
        iMonitorDuration = gmonitor_duration[0];
#ifdef CONFIG_MIU_BW_TO_KMSG_OR_SYSFS
        iMonitorOutputKmsg = gmonitor_output_kmsg[0];
#endif
        iMiuBankAddr = BASE_REG_MIU_DIG;
        pstMiuClient = &miu0_clients[0];
    }
#if MIU_NUM > 1
    else if ('1' == (dev->kobj.name[6]))
    {
        iMiuClientNum    = MIU1_CLIENT_NUM;
        iMonitorInterval = gmonitor_interval[1];
        iMonitorDuration = gmonitor_duration[1];
#ifdef CONFIG_MIU_BW_TO_KMSG_OR_SYSFS
        iMonitorOutputKmsg = gmonitor_output_kmsg[1];
#endif
        iMiuBankAddr = BASE_REG_MIU1_PA;
        pstMiuClient = &miu1_clients[0];
    }
#endif
    else
    {
        printk("%d\r\n", __LINE__);
        return 0;
    }

    for (i = 0; i < iMiuClientNum; i++)
    {
        (pstMiuClient + i)->effi_min = 0x3FF;
        (pstMiuClient + i)->effi_avg = 0;
        (pstMiuClient + i)->bw_avg   = 0;
        (pstMiuClient + i)->bw_max   = 0;

#ifdef CONFIG_MIU_BW_DUMP_TO_FILE
        if ((pstMiuClient + i)->bw_dump_en)
        {
            iMonitorDumpToFile = 1;
        }
#endif
    }

#ifdef CONFIG_MIU_BW_DUMP_TO_FILE
    if (iMonitorDumpToFile)
    {
        u8Buf    = kmalloc(DUMP_FILE_TEMP_BUF_SZ, GFP_KERNEL);
        u8Ptr    = u8Buf;
        u8PtrEnd = u8Buf + (DUMP_FILE_TEMP_BUF_SZ);
        pstOutFd = miu_bw_open_file(m_bOutputFilePath, O_WRONLY | O_CREAT, 0644);
    }
#endif

    if (iMonitorOutputKmsg)
    {
#if defined(CONFIG_MIU_BW_TO_KMSG) || defined(CONFIG_MIU_BW_TO_KMSG_OR_SYSFS)
        printk("Num:client\tEFFI\tBWavg\tBWmax\tBWavg/E\tBWmax/E\n");
        printk("---------------------------------------------------------\n");
#endif
    }
    else
    {
#if defined(CONFIG_MIU_BW_TO_SYSFS) || defined(CONFIG_MIU_BW_TO_KMSG_OR_SYSFS)
        str += scnprintf(str, end - str, "Num:client\tEFFI\tBWavg\tBWmax\tBWavg/E\tBWmax/E\n");
        str += scnprintf(str, end - str, "---------------------------------------------------------\n");
#endif
    }

    for (i = 0; i < iMiuClientNum; i++)
    {
        if ((pstMiuClient + i)->bw_enabled && (pstMiuClient + i)->bw_rsvd == 0)
        {
            unsigned long diff = 0;
            short         last;
            total_temp     = 0;
            temp_loop_time = 0;

            id = (pstMiuClient + i)->bw_client_id;
            halBWInit(iMiuBankAddr, id);
            id = id & 0x3F;
            halBWEffiMinConfig(iMiuBankAddr, id);
            deadline = jiffies + iMonitorDuration * HZ / 1000;

            do
            {
                if (iMonitorInterval > 10)
                    msleep(iMonitorInterval);
                else
                    mdelay(iMonitorInterval);

                temp_val = halBWReadBus(iMiuBankAddr);

                if ((pstMiuClient + i)->bw_filter_en)
                {
                    if (temp_val)
                    {
                        total_temp += temp_val;
                    }
                }
                else
                {
                    total_temp += temp_val;
                }

                if (temp_loop_time)
                {
                    diff += (temp_val > last) ? temp_val - last : last - temp_val;
                }
                last = temp_val;

#ifdef CONFIG_MIU_BW_DUMP_TO_FILE
                if ((pstMiuClient + i)->bw_dump_en)
                {
                    if (temp_loop_time == 0)
                    {
                        u8Ptr += scnprintf(u8Ptr, u8PtrEnd - u8Ptr,
                                           "\n----------------------------------------------------------------\n");
                        u8Ptr += scnprintf(u8Ptr, u8PtrEnd - u8Ptr, "ClientId[%d] Name[%s] Efficiency\n", (short)i,
                                           (pstMiuClient + i)->name);
                        u8Ptr +=
                            scnprintf(u8Ptr, u8PtrEnd - u8Ptr, "\t0x00\t0x01\t0x02\t0x03\t0x04\t0x05\t0x06\t0x07\n");
                        u8Ptr += scnprintf(u8Ptr, u8PtrEnd - u8Ptr,
                                           "----------------------------------------------------------------\n");
                    }

                    u8Ptr += scnprintf(u8Ptr, u8PtrEnd - u8Ptr, "\t%2d.%02d%%", temp_val * 100 / 1024,
                                       (temp_val * 10000 / 1024) % 100);
                    if ((temp_loop_time + 1) % 0x8 == 0)
                    {
                        u8Ptr += scnprintf(u8Ptr, u8PtrEnd - u8Ptr, "\n");
                    }
                }
#endif
                if ((pstMiuClient + i)->effi_min > temp_val)
                {
                    (pstMiuClient + i)->effi_min = temp_val;
                }

                if ((pstMiuClient + i)->bw_filter_en)
                {
                    if (temp_val)
                    {
                        temp_loop_time++;
                    }
                }
                else
                {
                    temp_loop_time++;
                }

            } while (!time_after_eq(jiffies, deadline));

            halBWResetFunc(iMiuBankAddr); // reset all
            (pstMiuClient + i)->effi_avg = total_temp / temp_loop_time;

            total_temp     = 0;
            temp_loop_time = 0;
            halBWEffiRealConfig(iMiuBankAddr, id);

            deadline = jiffies + iMonitorDuration * HZ / 1000;

            do
            {
                if (iMonitorInterval > 10)
                    msleep(iMonitorInterval);
                else
                    mdelay(iMonitorInterval);

                temp_val = halBWReadBus(iMiuBankAddr);

                if ((pstMiuClient + i)->bw_filter_en)
                {
                    if (temp_val)
                    {
                        total_temp += temp_val;
                    }
                }
                else
                {
                    total_temp += temp_val;
                }

#ifdef CONFIG_MIU_BW_DUMP_TO_FILE
                if (iMonitorDumpToFile && (pstMiuClient + i)->bw_dump_en)
                {
                    if (temp_loop_time == 0)
                    {
                        u8Ptr += scnprintf(u8Ptr, u8PtrEnd - u8Ptr,
                                           "\n----------------------------------------------------------------\n");
                        u8Ptr += scnprintf(u8Ptr, u8PtrEnd - u8Ptr, "ClientId[%d] Name[%s] BandWidth\n", (short)i,
                                           (pstMiuClient + i)->name);
                        u8Ptr +=
                            scnprintf(u8Ptr, u8PtrEnd - u8Ptr, "\t0x00\t0x01\t0x02\t0x03\t0x04\t0x05\t0x06\t0x07\n");
                        u8Ptr += scnprintf(u8Ptr, u8PtrEnd - u8Ptr,
                                           "----------------------------------------------------------------\n");
                    }

                    u8Ptr += scnprintf(u8Ptr, u8PtrEnd - u8Ptr, "\t%2d.%02d%%", temp_val * 100 / 1024,
                                       (temp_val * 10000 / 1024) % 100);
                    if ((temp_loop_time + 1) % 0x8 == 0)
                    {
                        u8Ptr += scnprintf(u8Ptr, u8PtrEnd - u8Ptr, "\n");
                    }
                }
#endif
                if ((pstMiuClient + i)->bw_max < temp_val)
                {
                    (pstMiuClient + i)->bw_max = temp_val;
                }

                if ((pstMiuClient + i)->bw_filter_en)
                {
                    if (temp_val)
                    {
                        temp_loop_time++;
                    }
                }
                else
                {
                    temp_loop_time++;
                }

            } while (!time_after_eq(jiffies, deadline));

            halBWResetFunc(iMiuBankAddr); // reset all
            (pstMiuClient + i)->bw_avg = total_temp / temp_loop_time;

            // client effiency never changes and total BW is zero
            if ((diff == 0) && (total_temp == 0))
            {
                (pstMiuClient + i)->effi_avg = 0x3FF;
            }

            if (iMonitorOutputKmsg)
            {
#if defined(CONFIG_MIU_BW_TO_KMSG) || defined(CONFIG_MIU_BW_TO_KMSG_OR_SYSFS)
                printk("%3d:%s\t%2d.%02d%%\t%02d.%02d%%\t%02d.%02d%%\t%02d.%02d%%\t%02d.%02d%%\n",
                       (short)(pstMiuClient + i)->bw_client_id, (pstMiuClient + i)->name,
                       (pstMiuClient + i)->effi_avg * 100 / 1024, ((pstMiuClient + i)->effi_avg * 10000 / 1024) % 100,
                       (pstMiuClient + i)->bw_avg * 100 / 1024, ((pstMiuClient + i)->bw_avg * 10000 / 1024) % 100,
                       (pstMiuClient + i)->bw_max * 100 / 1024, ((pstMiuClient + i)->bw_max * 10000 / 1024) % 100,
                       (pstMiuClient + i)->bw_avg * 100 / (pstMiuClient + i)->effi_avg,
                       ((pstMiuClient + i)->bw_avg * 10000 / (pstMiuClient + i)->effi_avg) % 100,
                       (pstMiuClient + i)->bw_max * 100 / (pstMiuClient + i)->effi_avg,
                       ((pstMiuClient + i)->bw_max * 10000 / (pstMiuClient + i)->effi_avg) % 100);
#endif
            }
            else
            {
#if defined(CONFIG_MIU_BW_TO_SYSFS) || defined(CONFIG_MIU_BW_TO_KMSG_OR_SYSFS)
                str += scnprintf(
                    str, end - str, "%3d:%s\t%2d.%02d%%\t%02d.%02d%%\t%02d.%02d%%\t%02d.%02d%%\t%02d.%02d%%\n",
                    (short)i, (pstMiuClient + i)->name, (pstMiuClient + i)->effi_avg * 100 / 1024,
                    ((pstMiuClient + i)->effi_avg * 10000 / 1024) % 100, (pstMiuClient + i)->bw_avg * 100 / 1024,
                    ((pstMiuClient + i)->bw_avg * 10000 / 1024) % 100, (pstMiuClient + i)->bw_max * 100 / 1024,
                    ((pstMiuClient + i)->bw_max * 10000 / 1024) % 100,
                    (pstMiuClient + i)->bw_avg * 100 / (pstMiuClient + i)->effi_avg,
                    ((pstMiuClient + i)->bw_avg * 10000 / (pstMiuClient + i)->effi_avg) % 100,
                    (pstMiuClient + i)->bw_max * 100 / (pstMiuClient + i)->effi_avg,
                    ((pstMiuClient + i)->bw_max * 10000 / (pstMiuClient + i)->effi_avg) % 100);
#endif
            }
        }
    }

#ifdef CONFIG_MIU_BW_DUMP_TO_FILE
    if (iMonitorDumpToFile)
    {
        if (pstOutFd)
        {
            miu_bw_write_file(pstOutFd, u8Buf, u8Ptr - u8Buf);
            miu_bw_close_file(pstOutFd);
        }
        kfree(u8Buf);
    }
#endif

    if (str > buf)
        str--;
    str += scnprintf(str, end - str, "\n");
    return (str - buf);
}
#endif

static ssize_t measure_all_hw_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    int   i   = 0;
    int   id;
    int   iMiuClientNum    = 0;
    int   iMonitorDuration = 0;
#if defined(CONFIG_MIU_BW_TO_SYSFS)
    int iMonitorOutputKmsg = 0;
#else
    int iMonitorOutputKmsg = 1;
#endif
    int                iMiuBankAddr = 0;
    struct miu_client *pstMiuClient = NULL;
    short              effi_last    = 0;

    if ('0' == (dev->kobj.name[6]))
    {
        iMiuClientNum    = MIU0_CLIENT_NUM;
        iMonitorDuration = gmonitor_duration[0];
#ifdef CONFIG_MIU_BW_TO_KMSG_OR_SYSFS
        iMonitorOutputKmsg = gmonitor_output_kmsg[0];
#endif
        iMiuBankAddr = BASE_REG_MIU_DIG;
        pstMiuClient = &miu0_clients[0];
    }
#if MIU_NUM > 1
    else if ('1' == (dev->kobj.name[6]))
    {
        iMiuClientNum    = MIU1_CLIENT_NUM;
        iMonitorDuration = gmonitor_duration[1];
#ifdef CONFIG_MIU_BW_TO_KMSG_OR_SYSFS
        iMonitorOutputKmsg = gmonitor_output_kmsg[1];
#endif
        iMiuBankAddr = BASE_REG_MIU1_PA;
        pstMiuClient = &miu1_clients[0];
    }
#endif
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
    }

    if (iMonitorOutputKmsg)
    {
#if defined(CONFIG_MIU_BW_TO_KMSG) || defined(CONFIG_MIU_BW_TO_KMSG_OR_SYSFS)
        printk("ID:client\tEFFI\tBWavg\tBWmax\tBWavg/E\tBWmax/E\n");
        printk("---------------------------------------------------------\n");
#endif
    }
    else
    {
#if defined(CONFIG_MIU_BW_TO_SYSFS) || defined(CONFIG_MIU_BW_TO_KMSG_OR_SYSFS)
        str += scnprintf(str, end - str, "ID:client\tEFFI\tBWavg\tBWmax\tBWavg/E\tBWmax/E\n");
        str += scnprintf(str, end - str, "---------------------------------------------------------\n");
#endif
    }

    for (i = 0; i < iMiuClientNum; i++)
    {
        if ((pstMiuClient + i)->bw_enabled && (pstMiuClient + i)->bw_rsvd == 0)
        {
            id = (pstMiuClient + i)->bw_client_id;
            halBWInit(iMiuBankAddr, id);
            id = id & 0x3F;
            halBWEffiMinPerConfig(iMiuBankAddr, id); // RESET
            msleep(iMonitorDuration);
            (pstMiuClient + i)->effi_avg = halBWReadBus(iMiuBankAddr);
            halBWEffiAvgPerConfig(iMiuBankAddr, id);
            //            msleep(iMonitorDuration);
            (pstMiuClient + i)->bw_avg = halBWReadBus(iMiuBankAddr);
            halBWEffiMaxPerConfig(iMiuBankAddr, id);
            //            msleep(iMonitorDuration);
            (pstMiuClient + i)->bw_max = halBWReadBus(iMiuBankAddr);
            halBWOCCRealPerConfig(iMiuBankAddr, id);
            //            msleep(iMonitorDuration);
            (pstMiuClient + i)->bw_avg_div_effi = halBWReadBus(iMiuBankAddr);
            halBWOCCMaxPerConfig(iMiuBankAddr, id);
            //            msleep(iMonitorDuration);
            (pstMiuClient + i)->bw_max_div_effi = halBWReadBus(iMiuBankAddr);
            halBWResetFunc(iMiuBankAddr); // reset all

            // client effiency never changes and total BW is zero
            // all measured BW are all zero, set effi to 99.9%
            if ((pstMiuClient + i)->bw_avg == 0 && (pstMiuClient + i)->bw_max == 0
                && (pstMiuClient + i)->bw_avg_div_effi == 0 && (pstMiuClient + i)->bw_max_div_effi == 0)
            {
                //                printk("all measured BW are all zero !\r\n");
                (pstMiuClient + i)->effi_avg = 0x3FF;
            }

            if (iMonitorOutputKmsg)
            {
#if defined(CONFIG_MIU_BW_TO_KMSG) || defined(CONFIG_MIU_BW_TO_KMSG_OR_SYSFS)
                if ((pstMiuClient + i)->effi_avg != effi_last)
                {
                    printk("%3d:%s\t%2d.%02d%%\t%02d.%02d%%\t%02d.%02d%%\t%02d.%02d%%\t%02d.%02d%%\n",
                           (short)(pstMiuClient + i)->bw_client_id, (pstMiuClient + i)->name,
                           (pstMiuClient + i)->effi_avg * 100 / 1024,
                           ((pstMiuClient + i)->effi_avg * 10000 / 1024) % 100, (pstMiuClient + i)->bw_avg * 100 / 1024,
                           ((pstMiuClient + i)->bw_avg * 10000 / 1024) % 100, (pstMiuClient + i)->bw_max * 100 / 1024,
                           ((pstMiuClient + i)->bw_max * 10000 / 1024) % 100,
                           (pstMiuClient + i)->bw_avg_div_effi * 100 / 1024,
                           ((pstMiuClient + i)->bw_avg_div_effi * 10000 / 1024) % 100,
                           (pstMiuClient + i)->bw_max_div_effi * 100 / 1024,
                           ((pstMiuClient + i)->bw_max_div_effi * 10000 / 1024) % 100);
                }
                else
                {
                    printk("%3d:%s\t" ASCII_COLOR_RED "%2d.%02d%%" ASCII_COLOR_END
                           "\t%02d.%02d%%\t%02d.%02d%%\t%02d.%02d%%\t%02d.%02d%%\n",
                           (short)(pstMiuClient + i)->bw_client_id, (pstMiuClient + i)->name,
                           (pstMiuClient + i)->effi_avg * 100 / 1024,
                           ((pstMiuClient + i)->effi_avg * 10000 / 1024) % 100, (pstMiuClient + i)->bw_avg * 100 / 1024,
                           ((pstMiuClient + i)->bw_avg * 10000 / 1024) % 100, (pstMiuClient + i)->bw_max * 100 / 1024,
                           ((pstMiuClient + i)->bw_max * 10000 / 1024) % 100,
                           (pstMiuClient + i)->bw_avg_div_effi * 100 / 1024,
                           ((pstMiuClient + i)->bw_avg_div_effi * 10000 / 1024) % 100,
                           (pstMiuClient + i)->bw_max_div_effi * 100 / 1024,
                           ((pstMiuClient + i)->bw_max_div_effi * 10000 / 1024) % 100);
                }
#endif
            }
            else
            {
#if defined(CONFIG_MIU_BW_TO_SYSFS) || defined(CONFIG_MIU_BW_TO_KMSG_OR_SYSFS)
                if ((pstMiuClient + i)->effi_avg != effi_last)
                {
                    str += scnprintf(
                        str, end - str, "%3d:%s\t%2d.%02d%%\t%02d.%02d%%\t%02d.%02d%%\t%02d.%02d%%\t%02d.%02d%%\n",
                        (short)i, (pstMiuClient + i)->name, (pstMiuClient + i)->effi_avg * 100 / 1024,
                        ((pstMiuClient + i)->effi_avg * 10000 / 1024) % 100, (pstMiuClient + i)->bw_avg * 100 / 1024,
                        ((pstMiuClient + i)->bw_avg * 10000 / 1024) % 100, (pstMiuClient + i)->bw_max * 100 / 1024,
                        ((pstMiuClient + i)->bw_max * 10000 / 1024) % 100,
                        (pstMiuClient + i)->bw_avg_div_effi * 100 / 1024,
                        ((pstMiuClient + i)->bw_avg_div_effi * 10000 / 1024) % 100,
                        (pstMiuClient + i)->bw_max_div_effi * 100 / 1024,
                        ((pstMiuClient + i)->bw_max_div_effi * 10000 / 1024) % 100);
                }
                else
                {
                    str += scnprintf(
                        str, end - str,
                        "%3d:%s\t" ASCII_COLOR_RED "%2d.%02d%%" ASCII_COLOR_RED "\t%02d.%02d%%" ASCII_COLOR_RED
                        "\t%02d.%02d%%\t%02d.%02d%%\t%02d.%02d%%\n",
                        (short)i, (pstMiuClient + i)->name, (pstMiuClient + i)->effi_avg * 100 / 1024,
                        ((pstMiuClient + i)->effi_avg * 10000 / 1024) % 100, (pstMiuClient + i)->bw_avg * 100 / 1024,
                        ((pstMiuClient + i)->bw_avg * 10000 / 1024) % 100, (pstMiuClient + i)->bw_max * 100 / 1024,
                        ((pstMiuClient + i)->bw_max * 10000 / 1024) % 100,
                        (pstMiuClient + i)->bw_avg_div_effi * 100 / 1024,
                        ((pstMiuClient + i)->bw_avg_div_effi * 10000 / 1024) % 100,
                        (pstMiuClient + i)->bw_max_div_effi * 100 / 1024,
                        ((pstMiuClient + i)->bw_max_div_effi * 10000 / 1024) % 100);
                }
#endif
            }
        }
        if ((pstMiuClient + i)->effi_avg != 0x3FF)
        {
            effi_last = (pstMiuClient + i)->effi_avg;
        }
    }

    if (str > buf)
        str--;

    str += scnprintf(str, end - str, "\n");
    return (str - buf);
}

//***ToDo-----I6 Diff*/
#ifdef CONFIG_CAM_CLK
static ssize_t dram_info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *       str           = buf;
    char *       end           = buf + PAGE_SIZE;
    unsigned int iMiuBankAddr  = 0;
    unsigned int iAtopBankAddr = 0;
    unsigned int dram_type     = 0;

    if ('0' == (dev->kobj.name[6]))
    {
        iMiuBankAddr  = BASE_REG_MIU_PA;
        iAtopBankAddr = BASE_REG_ATOP_PA;
    }
#if MIU_NUM > 1
    else if ('1' == (dev->kobj.name[6]))
    {
        iMiuBankAddr  = BASE_REG_MIU1_PA;
        iAtopBankAddr = BASE_REG_ATOP_PA;
    }
#endif
    else
    {
        return 0;
    }

    dram_type = INREGMSK16(iMiuBankAddr + REG_ID_01, 0x0003);

    str += scnprintf(str, end - str, "DRAM Type:   %s\n",
                     (dram_type == 3)   ? "DDR3"
                     : (dram_type == 2) ? "DDR2"
                                        : "Unknown");
    str += scnprintf(str, end - str, "DRAM Size:   %dMB\n", 1 << (INREGMSK16(iMiuBankAddr + REG_ID_69, 0xF000) >> 12));
    str += scnprintf(str, end - str, "DRAM Freq:   %dMHz\n", CamClkRateGet(CAMCLK_ddrpll_clk) / 1000000);
    str += scnprintf(str, end - str, "MIUPLL Freq: %dMHz\n", CamClkRateGet(CAMCLK_miupll_clk) / 1000000);
    str +=
        scnprintf(str, end - str, "Data Rate:   %dx Mode\n", 1 << (INREGMSK16(iMiuBankAddr + REG_ID_01, 0x0300) >> 8));
    str += scnprintf(str, end - str, "Bus Width:   %dbit\n", 16 << (INREGMSK16(iMiuBankAddr + REG_ID_01, 0x000C) >> 2));
    str += scnprintf(str, end - str, "SSC:         %s\n",
                     (INREGMSK16(iAtopBankAddr + REG_ID_14, 0xC000) == 0x8000) ? "OFF" : "ON");

    if (str > buf)
        str--;
    str += scnprintf(str, end - str, "\n");
    return (str - buf);
}
#else // CONFIG_CAM_CLK
static ssize_t dram_info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    unsigned int iMiuBankAddr = 0;
    unsigned int iAtopBankAddr = 0;
    unsigned int iMiupllBankAddr = 0;
    unsigned int dram_type = 0;
    unsigned int ddfset = 0;
    unsigned int dram_freq = 0;
    unsigned int miupll_freq = 0;

    if ('0' == (dev->kobj.name[6]))
    {
        iMiuBankAddr = BASE_REG_MIU_PA;
        iAtopBankAddr = BASE_REG_ATOP_PA;
        iMiupllBankAddr = BASE_REG_MIUPLL_PA;
    }
#if MIU_NUM > 1
    else if ('1' == (dev->kobj.name[6]))
    {
        iMiuBankAddr = BASE_REG_MIU1_PA;
        iAtopBankAddr = BASE_REG_ATOP_PA;
        iMiupllBankAddr = BASE_REG_MIUPLL_PA;
    }
#endif
    else
    {
        return 0;
    }
    dram_type = INREGMSK16(iMiuBankAddr + REG_ID_01, 0x0003);
    ddfset = (INREGMSK16(iAtopBankAddr + REG_ID_19, 0x00FF) << 16) + INREGMSK16(iAtopBankAddr + REG_ID_18, 0xFFFF);
    dram_freq = ((432 * 4 * 4) << 19) / ddfset;
    miupll_freq = 24 * INREGMSK16(iMiupllBankAddr + REG_ID_03, 0x00FF)
                  / ((INREGMSK16(iMiupllBankAddr + REG_ID_03, 0x0700) >> 8) + 2);
    str += scnprintf(str, end - str, "DRAM Type:   %s\n",
                     (dram_type == 3) ? "DDR3"
                     : (dram_type == 2) ? "DDR2"
                                        : "Unknown");
    str += scnprintf(str, end - str, "DRAM Size:   %dMB\n", 1 << (INREGMSK16(iMiuBankAddr + REG_ID_69, 0xF000) >> 12));
    str += scnprintf(str, end - str, "DRAM Freq:   %dMHz\n", dram_freq);
    str += scnprintf(str, end - str, "MIUPLL Freq: %dMHz\n", miupll_freq);
    str +=
        scnprintf(str, end - str, "Data Rate:   %dx Mode\n", 1 << (INREGMSK16(iMiuBankAddr + REG_ID_01, 0x0300) >> 8));
    str += scnprintf(str, end - str, "Bus Width:   %dbit\n", 16 << (INREGMSK16(iMiuBankAddr + REG_ID_01, 0x000C) >> 2));
    str += scnprintf(str, end - str, "SSC:         %s\n",
                     (INREGMSK16(iAtopBankAddr + REG_ID_14, 0xC000) == 0x8000) ? "OFF" : "ON");

    if (str > buf)
        str--;
    str += scnprintf(str, end - str, "\n");
    return (str - buf);
}
#endif

#ifdef CONFIG_PM_SLEEP
int miu_subsys_suspend(struct device *dev)
{
    if (dev == &miu0.dev)
    {
        // keep dram size setting
        // miu0.reg_dram_size = INREGMSK16(BASE_REG_MIU_PA + REG_ID_69, 0xF000);
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
        // OUTREGMSK16(BASE_REG_MIU_PA + REG_ID_69, miu0.reg_dram_size, 0xF000);
        pr_debug("restore dram size setting\n");
    }
    else if (strncmp(dev->kobj.name, "miu_arb0", 9) == 0)
    {
        pr_debug("miu_arb_resume \n");
        // miu_arb_resume();
    }

    pr_debug("miu subsys resume %s\n", dev->kobj.name);
    return 0;
}
#endif

DEVICE_ATTR(monitor_client_enable, 0644, monitor_client_enable_show, monitor_client_enable_store);
DEVICE_ATTR(monitor_client_disable, 0644, monitor_client_disable_show, monitor_client_disable_store);
DEVICE_ATTR(monitor_set_interval_ms, 0644, monitor_set_interval_avg_show, monitor_set_interval_avg_store);
DEVICE_ATTR(monitor_set_duration_ms, 0644, monitor_set_counts_avg_show, monitor_set_counts_avg_store);
#ifdef CONFIG_MIU_BW_DUMP_TO_FILE
DEVICE_ATTR(monitor_client_dump_enable, 0644, monitor_client_dump_enable_show, monitor_client_dump_enable_store);
#endif
DEVICE_ATTR(monitor_client_filter_enable, 0644, monitor_filter_abnormal_value_show,
            monitor_filter_abnormal_value_store);
#ifdef CONFIG_MIU_BW_SW_MODE
#ifdef CONFIG_MIU_BW_TO_KMSG_OR_SYSFS
DEVICE_ATTR(measure_all, 0644, measure_all_show, measure_all_store);
#else
DEVICE_ATTR(measure_all, 0444, measure_all_show, NULL);
#endif
#endif
#ifdef CONFIG_MIU_BW_TO_KMSG_OR_SYSFS
DEVICE_ATTR(measure_all_hw, 0644, measure_all_hw_show, measure_all_store);
#else
DEVICE_ATTR(measure_all_hw, 0444, measure_all_hw_show, NULL);
#endif
DEVICE_ATTR(dram_info, 0444, dram_info_show, NULL);
#ifdef CONFIG_MIU_ARBITRATION
extern void create_miu_arb_node(struct bus_type *miu_subsys);
#endif
#ifdef CONFIG_MIU_PROTECT_SYSFS
extern void create_miu_protect_node(struct bus_type *miu_subsys);
#endif
#ifdef CONFIG_MSYS_MIU_UT
extern void create_miu_ut_node(struct bus_type *miu_subsys);
#endif

void mstar_create_MIU_node(void)
{
    int ret = 0;

    miu0.index         = 0;
    miu0.dev.kobj.name = "miu_bw0";
    miu0.dev.bus       = &miu_subsys;

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
#ifdef CONFIG_MIU_ARBITRATION
    create_miu_arb_node(&miu_subsys);
#endif
#ifdef CONFIG_MIU_PROTECT_SYSFS
    create_miu_protect_node(&miu_subsys);
#endif
#ifdef CONFIG_MSYS_MIU_UT
    create_miu_ut_node(&miu_subsys);
#endif
    device_create_file(&miu0.dev, &dev_attr_monitor_client_enable);
    device_create_file(&miu0.dev, &dev_attr_monitor_client_disable);
    device_create_file(&miu0.dev, &dev_attr_monitor_set_interval_ms);
    device_create_file(&miu0.dev, &dev_attr_monitor_set_duration_ms);
#ifdef CONFIG_MIU_BW_DUMP_TO_FILE
    device_create_file(&miu0.dev, &dev_attr_monitor_client_dump_enable);
#endif
    device_create_file(&miu0.dev, &dev_attr_monitor_client_filter_enable);
#ifdef CONFIG_MIU_BW_SW_MODE
    device_create_file(&miu0.dev, &dev_attr_measure_all);
#endif
    device_create_file(&miu0.dev, &dev_attr_measure_all_hw);
    device_create_file(&miu0.dev, &dev_attr_dram_info);

#if MIU_NUM > 1
    miu1.index         = 0;
    miu1.dev.kobj.name = "miu_bw1";
    miu1.dev.bus       = &miu_subsys;

#if 0 // Don't register again
    ret = subsys_system_register(&miu_subsys, NULL);
    if (ret) {
        printk(KERN_ERR "Failed to register miu sub system!! %d\n",ret);
        return;
    }
#endif

    ret = device_register(&miu1.dev);
    if (ret)
    {
        printk(KERN_ERR "Failed to register miu1 device!! %d\n", ret);
        return;
    }

    device_create_file(&miu1.dev, &dev_attr_monitor_client_enable);
    device_create_file(&miu1.dev, &dev_attr_monitor_client_disable);
    device_create_file(&miu1.dev, &dev_attr_monitor_set_interval_ms);
    device_create_file(&miu1.dev, &dev_attr_monitor_set_duration_ms);
    device_create_file(&miu1.dev, &dev_attr_monitor_client_dump_enable);
    device_create_file(&miu1.dev, &dev_attr_monitor_client_filter_enable);
    device_create_file(&miu1.dev, &dev_attr_measure_all);
    device_create_file(&miu1.dev, &dev_attr_measure_all_hw);
    device_create_file(&miu1.dev, &dev_attr_dram_info);
#endif
}
