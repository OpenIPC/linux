/*
 * mdrv_sysfs_synopsys.c- Sigmastar
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
#include <linux/printk.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/device.h>
#include "MsTypes.h"
#include "registers.h"
#include "ms_platform.h"
#include "cam_os_wrapper.h"
#include "mhal_synopsys.h"
#include "mdrv_miu.h"

static struct miu_device          synopsys_dev;
static struct miu_synopsys_handle synopsys_handle;

/*=============================================================
 * Local function
 *=============================================================*/
static ssize_t synopsys_reg_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    char *pt;
    char *opt;
    int   cnt = 0, i = 0;
    u32   value = 0;

    pt = kmalloc(strlen(buf) + 1, GFP_KERNEL);
    strcpy(pt, buf);
    while ((opt = strsep(&pt, ":")) != NULL)
    {
        if (cnt == 0)
        {
            for (i = 0; i < MIU_SYNOPSYS_REG_NUM; i++)
            {
                if (!strcmp(synopsys_reg[i].name, opt))
                {
                    break;
                }
            }
            if (i == MIU_SYNOPSYS_REG_NUM)
            {
                printk(KERN_ERR "no support synopsys reg : %s\n", opt);
                goto OUT;
            }
        }
        else if (cnt == 1)
        {
            value = simple_strtoul(opt, NULL, 0);
        }
        cnt++;
    }
    halSynopsysSetRegValue(&synopsys_reg[i], value);
    if (value == halSynopsysGetRegValue(&synopsys_reg[i]))
    {
        printk(KERN_ERR "set synopsys reg : %s, value : 0x%08x success\n", synopsys_reg[i].name, value);
    }
    else
    {
        printk(KERN_ERR "set synopsys reg : %s, value : 0x%08x fail\n", synopsys_reg[i].name, value);
    }

OUT:
    kfree(pt);

    return count;
}

static ssize_t synopsys_reg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    int   i   = 0;

    str += scnprintf(str, end - str, "reg_name : value\n");

    for (i = 0; i < MIU_SYNOPSYS_REG_NUM; i++)
    {
        str +=
            scnprintf(str, end - str, "%s : 0x%08x\n", synopsys_reg[i].name, halSynopsysGetRegValue(&synopsys_reg[i]));
    }

    return (str - buf);
}

DEVICE_ATTR(synopsys_reg, 0644, synopsys_reg_show, synopsys_reg_store);

static ssize_t synopsys_cam_para_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    char *pt;
    char *opt;
    int   cnt = 0, i = 0, j = 0;
    u32   value = 0, reg_value = 0;

    pt = kmalloc(strlen(buf) + 1, GFP_KERNEL);
    strcpy(pt, buf);
    while ((opt = strsep(&pt, ":")) != NULL)
    {
        if (cnt == 0)
        {
            for (i = 0; i < MIU_SYNOPSYS_CAM_PARA_NUM; i++)
            {
                if (!strcmp(synopsys_cam_para[i].name, opt))
                {
                    break;
                }
            }
            if (i == MIU_SYNOPSYS_CAM_PARA_NUM)
            {
                printk(KERN_ERR "no support synopsys cam para : %s\n", opt);
                goto OUT;
            }
        }
        else if (cnt == 1)
        {
            value = simple_strtoul(opt, NULL, 0);
            if (value > synopsys_cam_para[i].mask >> synopsys_cam_para[i].bit_shift)
            {
                printk(KERN_ERR "input value is too large !!! limit : 0x%04x\n",
                       synopsys_cam_para[i].mask >> synopsys_cam_para[i].bit_shift);
                goto OUT;
            }
        }
        cnt++;
    }

    for (j = 0; j < MIU_SYNOPSYS_REG_NUM; j++)
    {
        if (!strcmp(synopsys_reg[j].name, synopsys_cam_para[i].reg_name))
        {
            break;
        }
    }
    if (j == MIU_SYNOPSYS_REG_NUM)
    {
        printk(KERN_ERR "set synopsys cam para : %s, value : 0x%04x failed for reg %s is not find\n",
               synopsys_cam_para[i].name, value, synopsys_cam_para[i].reg_name);
        goto OUT;
    }
    else
    {
        reg_value = halSynopsysGetRegValue(&synopsys_reg[j]);
        reg_value = (reg_value & ~synopsys_cam_para[i].mask) | value << synopsys_cam_para[i].bit_shift;
        halSynopsysSetRegValue(&synopsys_reg[j], reg_value);
    }

    if (reg_value == halSynopsysGetRegValue(&synopsys_reg[j]))
    {
        printk(KERN_ERR "set synopsys cam para : %s, value : 0x%04x success\n", synopsys_cam_para[i].name, value);
    }
    else
    {
        printk(KERN_ERR "set synopsys cam para : %s, value : 0x%04x fail\n", synopsys_cam_para[i].name, value);
    }

OUT:
    kfree(pt);

    return count;
}

static ssize_t synopsys_cam_para_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    int   i = 0, j = 0;

    str += scnprintf(str, end - str, "para_name : value\n");

    for (i = 0; i < MIU_SYNOPSYS_CAM_PARA_NUM; i++)
    {
        for (j = 0; j < MIU_SYNOPSYS_REG_NUM; j++)
        {
            if (!strcmp(synopsys_reg[j].name, synopsys_cam_para[i].reg_name))
            {
                break;
            }
        }
        if (j == MIU_SYNOPSYS_REG_NUM)
        {
            str += scnprintf(str, end - str, "%s : reg %s is not find\n", synopsys_cam_para[i].name,
                             synopsys_cam_para[i].reg_name);
        }
        else
        {
            str += scnprintf(str, end - str, "%s : 0x%04x\n", synopsys_cam_para[i].name,
                             (halSynopsysGetRegValue(&synopsys_reg[j]) & synopsys_cam_para[i].mask)
                                 >> synopsys_cam_para[i].bit_shift);
        }
    }

    return (str - buf);
}

DEVICE_ATTR(synopsys_cam_para, 0644, synopsys_cam_para_show, synopsys_cam_para_store);

void create_miu_synopsys_node(struct bus_type *miu_subsys)
{
    int ret = 0;

    strcpy(synopsys_handle.name, "miu_synopsys");

    synopsys_dev.index         = 0;
    synopsys_dev.dev.kobj.name = (const char *)synopsys_handle.name;
    synopsys_dev.dev.bus       = miu_subsys;

    ret = device_register(&synopsys_dev.dev);
    if (ret)
    {
        printk(KERN_ERR "Failed to register %s device!! %d\n", synopsys_dev.dev.kobj.name, ret);
        return;
    }

    device_create_file(&synopsys_dev.dev, &dev_attr_synopsys_reg);
    device_create_file(&synopsys_dev.dev, &dev_attr_synopsys_cam_para);
}
