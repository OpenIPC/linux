/*
* miu_bw.c- Sigmastar
*
* Copyright (C) 2018 Sigmastar Technology Corp.
*
* Author: Karl.Xiao <Karl.Xiao@sigmastar.com.tw>
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
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/irqchip.h>
#include <linux/of_platform.h>
#include <linux/of_fdt.h>
#include <linux/sys_soc.h>
#include <linux/slab.h>
#include <linux/suspend.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/clocksource.h>
#include <linux/gpio.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/memory.h>
#include <asm/io.h>
#include <asm/mach/map.h>
#include "gpio.h"
#include "registers.h"
#include "mcm_id.h"
#include "ms_platform.h"
#include "ms_types.h"
#include "_ms_private.h"

#include <linux/module.h>
#include <linux/unistd.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/mm.h>
#include <linux/buffer_head.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#include "cam_os_wrapper.h"
#include "miu_bw.h"
#ifdef CONFIG_CAM_CLK
#include "camclk.h"
#include "drv_camclk_Api.h"
#endif


/*=============================================================*/
// Local variable
/*=============================================================*/

static struct miu_client miu_clients[MIU_CLIENT_NUM] =
{
    {"OVERALL  ",0x00,0},
    {"RSVD     ",0x01,1},
    {"VEN      ",0x02,0},
    {"USB30    ",0x03,0},
    {"JPE_R    ",0x04,0},
    {"JPE_W    ",0x05,0},
    {"BACH     ",0x06,0},
    {"AESDMA   ",0x07,0},
    {"USB20    ",0x08,0},
    {"EMAC     ",0x09,0},
    {"MCU51    ",0x0A,0},
    {"URDMA    ",0x0B,0},
    {"BDMA     ",0x0C,0},
    {"MOVDMA   ",0x0D,0},
    {"LDC      ",0x0E,0},
    {"RSVD     ",0x0F,1},
    {"CMDQ0_R  ",0x10,0},
    {"ISP_DMA_W",0x11,0},
    {"ISP_DMA_R",0x12,0},
    {"ISP_ROT_R",0x13,0},
    {"ISP_MLOAD",0x14,0},
    {"GOP      ",0x15,0},
    {"RSVD     ",0x16,1},
    {"DIP0_R   ",0x17,0},
    {"DIP0_W   ",0x18,0},
    {"SC0_FRM_W",0x19,0},
    {"SC0_FRM_R",0x1A,0},
    {"SC0_DBG_R",0x1B,0},
    {"SC1_FRM_W",0x1C,0},
    {"SC2_FRM_W",0x1D,0},
    {"SD30     ",0x1E,0},
    {"SDIO30   ",0x1F,0},
    {"RSVD     ",0x20,1},
    {"RSVD     ",0x21,1},
    {"CSI_TX_R ",0x22,0},
    {"ISP_DMAG ",0x23,0},
    {"GOP1_R   ",0x24,0},
    {"GOP2_R   ",0x25,0},
    {"USB20_H  ",0x26,0},
    {"MIIC2    ",0x27,0},
    {"MIIC1    ",0x28,0},
    {"3DNR0_W  ",0x29,0},
    {"3DNR0_R  ",0x2A,0},
    {"DLA      ",0x2B,0},
    {"RSVD     ",0x2C,1},
    {"RSVD     ",0x2D,1},
    {"MIIC0    ",0x2E,0},
    {"IVE      ",0x2F,0},
    {"CPU      ",CPU_CLIENT_ID,0},
    {"DLA_HIWAY",DLA_HIWAY_ID,0},
};

static char miu_devname[MIU_NUM][5];
static struct miu_client_bw miu_clients_bw[MIU_NUM][MIU_CLIENT_NUM];
static struct miu_device miu[MIU_NUM];
#ifdef CONFIG_PM_SLEEP
int miu_subsys_suspend(struct device *dev);
int miu_subsys_resume(struct device *dev);
static SIMPLE_DEV_PM_OPS(miu_pm_ops, miu_subsys_suspend, miu_subsys_resume);
#endif
static struct bus_type miu_subsys = {
    .name = "miu",
    .dev_name = "miu",
#ifdef CONFIG_PM_SLEEP
    .pm = &miu_pm_ops,
#endif
};

static int gmonitor_duration[MIU_NUM] = {600};//{2000};
static int gmonitor_output_kmsg[MIU_NUM] = {1};

/*=============================================================*/
// Local function
/*=============================================================*/
short miu_client_reserved(U16 id)
{
    if (id < MIU_CLIENT_NUM)
    {
        return miu_clients[id].rsvd;
    }
    else if (id == CPU_CLIENT_ID)
    {
        return miu_clients[MIU_CLIENT_NUM - 2].rsvd;
    }
    else if (id == DLA_HIWAY_ID)
    {
        return miu_clients[MIU_CLIENT_NUM - 1].rsvd;
    }
    return 1;
}

const char* miu_client_id_to_name(U16 id)
{
    if (id < MIU_CLIENT_NUM)
    {
        return miu_clients[id].name;
    }
    else if (id == CPU_CLIENT_ID)
    {
        return miu_clients[MIU_CLIENT_NUM - 2].name;
    }
    else if (id == DLA_HIWAY_ID)
    {
        return miu_clients[MIU_CLIENT_NUM - 1].name;
    }
    return NULL;
}

static int set_miu_client_enable(struct device *dev, const char *buf, size_t n, int enabled)
{
    long idx = -1;
    unsigned char m = MIU_IDX(dev->kobj.name[3]);

    if (kstrtol(buf, 10, &idx) != 0 ||
        idx < 0 ||
        idx >= MIU_CLIENT_NUM) {
        return -EINVAL;
    }
    if (miu_clients[idx].rsvd == 0)
    {
        miu_clients_bw[m][idx].enabled = enabled;
    }

    return n;
}

static ssize_t monitor_client_enable_store(struct device *dev,  struct device_attribute *attr, const char *buf, size_t n)
{
    int i;
    char* pt;
    char* opt;
    unsigned char m = MIU_IDX(dev->kobj.name[3]);

    if(!strncmp(buf, "all", strlen("all")))
    {
        for (i = 0; i < MIU_CLIENT_NUM; i++) {
            if (miu_clients[i].rsvd == 0)
            {
                miu_clients_bw[m][i].enabled = 1;
            }
        }
        return n;
    }

    pt = kmalloc(strlen(buf)+1, GFP_KERNEL);
    strcpy(pt, buf);
    while((opt = strsep(&pt, ";, ")) != NULL)
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
    int i = 0;
    unsigned char m = MIU_IDX(dev->kobj.name[3]);

    str += scnprintf(str, end - str, "Num:IP_name\t[ Idx][Enable(1)/Disable(0)]\n");
    for (i = 0; i < MIU_CLIENT_NUM; i++)
    {
        str += scnprintf(str, end - str, "%3d:%s\t[0x%02X][%d]\n",
                                        i,
                                        miu_clients[i].name,
                                        miu_clients[i].id,
                                        miu_clients_bw[m][i].enabled);
    }

    if (str > buf)
        str--;
    str += scnprintf(str, end - str, "\n");

    return (str - buf);
}

static ssize_t monitor_client_disable_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    int i;
    char* pt;
    char* opt;
    unsigned char m = MIU_IDX(dev->kobj.name[3]);

    if (!strncmp(buf, "all", strlen("all")))
    {
        for (i = 0; i < MIU_CLIENT_NUM; i++) {
            if (miu_clients[i].rsvd == 0)
            {
                miu_clients_bw[m][i].enabled = 0;
            }
        }
        return n;
    }

    pt = kmalloc(strlen(buf)+1, GFP_KERNEL);
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
    int i = 0;
    unsigned char m = MIU_IDX(dev->kobj.name[3]);

    str += scnprintf(str, end - str, "Num:IP_name\t[ Idx][Enable(1)/Disable(0)]\n");
    for (i = 0; i < MIU_CLIENT_NUM; i++)
    {
        str += scnprintf(str, end - str, "%3d:%s\t[0x%02X][%d]\n",
                                        i,
                                        miu_clients[i].name,
                                        miu_clients[i].id,
                                        miu_clients_bw[m][i].enabled);
    }

    if (str > buf)
        str--;
    str += scnprintf(str, end - str, "\n");

    return (str - buf);
}

static ssize_t monitor_set_counts_avg_store(struct device *dev, struct device_attribute *attr,const char *buf, size_t count)
{
    u32 input = 0;
    unsigned char m = MIU_IDX(dev->kobj.name[3]);

    input = simple_strtoul(buf, NULL, 10);
    gmonitor_duration[m] = input;

    return count;
}

static ssize_t monitor_set_counts_avg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    unsigned char m = MIU_IDX(dev->kobj.name[3]);

    return sprintf(buf, "%d\n", gmonitor_duration[m]);
}

static ssize_t measure_all_store(struct device *dev, struct device_attribute *attr,const char *buf, size_t count)
{
    u32 input = 0;
    unsigned char m = MIU_IDX(dev->kobj.name[3]);

    input = simple_strtoul(buf, NULL, 10);
    gmonitor_output_kmsg[m] = input;

    return count;
}

static ssize_t measure_all_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    int i = 0;
    int id;
    int iMiuBankAddr = 0;
    unsigned char m = MIU_IDX(dev->kobj.name[3]);
    short effi = 0, effi_last = 0, bw[4] = {0, 0, 0, 0}; /* BWavg, BWmax, BWavg/EFFI, BWmax/EFFI */

    if (m == 0) {
        iMiuBankAddr = BASE_REG_MIU_PA;
    }
#if MIU_NUM > 1
    else {
        printk("NOT support multiple MIUs\n");
        return 0;
    }
#endif

    if (gmonitor_output_kmsg[m]) {
        printk("Num:client\tEFFI\tBWavg\tBWmax\tBWavg/E\tBWmax/E\n");
        printk("---------------------------------------------------------\n");
    }
    else {
        str += scnprintf(str, end - str, "Num:client\tEFFI\tBWavg\tBWmax\tBWavg/E\tBWmax/E\n");
        str += scnprintf(str, end - str, "---------------------------------------------------------\n");
    }

    for (i = 0; i < MIU_CLIENT_NUM; i++)
    {
        if (miu_clients_bw[m][i].enabled && (miu_clients[i].rsvd == 0))
        {
            id = miu_clients[i].id;
            OUTREG16((iMiuBankAddr+REG_ID_15), (id & 0x40) ? 0x80 : 0x00);
            id = id & 0x3F;
            OUTREG16((iMiuBankAddr+REG_ID_29), 0x0404);

            /* client utilization */
            OUTREG16((iMiuBankAddr+REG_ID_0D), 0) ; // reset all
            OUTREG16((iMiuBankAddr+REG_ID_0D), ((id << 8) | 0x40)); // reset
            OUTREG16((iMiuBankAddr+REG_ID_0D), ((id << 8) | 0x41)); // set to read bandwidth
            msleep(gmonitor_duration[m]);
            bw[0] = INREG16((iMiuBankAddr+REG_ID_0E));

            /* client peak bandwidth */
            OUTREG16((iMiuBankAddr+REG_ID_0D), 0) ; // reset all
            OUTREG16((iMiuBankAddr+REG_ID_0D), ((id << 8) | 0x50)); // reset
            OUTREG16((iMiuBankAddr+REG_ID_0D), ((id << 8) | 0x51)); // set to read bandwidth
            msleep(gmonitor_duration[m]);
            bw[1] = INREG16((iMiuBankAddr+REG_ID_0E));

            /* client utilization / efficiency */
            OUTREG16((iMiuBankAddr+REG_ID_0D), 0) ; // reset all
            OUTREG16((iMiuBankAddr+REG_ID_0D), ((id << 8) | 0x20)); // reset
            OUTREG16((iMiuBankAddr+REG_ID_0D), ((id << 8) | 0x21)); // set to read bandwidth
            msleep(gmonitor_duration[m]);
            bw[2] = INREG16((iMiuBankAddr+REG_ID_0E));

            /* client peak bandwidth / efficiency */
            OUTREG16((iMiuBankAddr+REG_ID_0D), 0) ; // reset all
            OUTREG16((iMiuBankAddr+REG_ID_0D), ((id << 8) | 0x60)); // reset
            OUTREG16((iMiuBankAddr+REG_ID_0D), ((id << 8) | 0x61)); // set to read bandwidth
            msleep(gmonitor_duration[m]);
            bw[3] = INREG16((iMiuBankAddr+REG_ID_0E));

            /* client efficiency */
            // all measured BW are all zero, set effi to 99.9%
            if ((bw[0] + bw[1] + bw[2] + bw[3]) == 0) {
                effi = 0x3FF;
            }
            else
            {
                OUTREG16((iMiuBankAddr+REG_ID_29), 0x0444);
                OUTREG16((iMiuBankAddr+REG_ID_0D), 0) ; // reset all
                OUTREG16((iMiuBankAddr+REG_ID_0D), ((id << 8) | 0x30)); // reset
                OUTREG16((iMiuBankAddr+REG_ID_0D), ((id << 8) | 0x35));
                msleep(gmonitor_duration[m]);
                effi = INREG16((iMiuBankAddr+REG_ID_0E));
            }

            if (gmonitor_output_kmsg[m]) {
                if (effi_last != effi) {
                    printk("%3d:%s\t%2d.%02d%%\t%2d.%02d%%\t%2d.%02d%%\t%2d.%02d%%\t%2d.%02d%%\n",
                        i, miu_clients[i].name, effi*100/1024, (effi*10000/1024)%100,
                        bw[0]*100/1024, (bw[0]*10000/1024)%100, bw[1]*100/1024, (bw[1]*10000/1024)%100,
                        bw[2]*100/1024, (bw[2]*10000/1024)%100, bw[3]*100/1024, (bw[3]*10000/1024)%100);
                }
                else {
                    printk("%3d:%s\t"ASCII_COLOR_RED"%2d.%02d%%"ASCII_COLOR_END"\t%2d.%02d%%\t%2d.%02d%%\t%2d.%02d%%\t%2d.%02d%%\n",
                        i, miu_clients[i].name, effi*100/1024, (effi*10000/1024)%100,
                        bw[0]*100/1024, (bw[0]*10000/1024)%100, bw[1]*100/1024, (bw[1]*10000/1024)%100,
                        bw[2]*100/1024, (bw[2]*10000/1024)%100, bw[3]*100/1024, (bw[3]*10000/1024)%100);
                }
            }
            else {
                if (effi_last != effi) {
                    str +=  scnprintf(str, end - str, "%3d:%s\t%2d.%02d%%\t%2d.%02d%%\t%2d.%02d%%\t%2d.%02d%%\t%2d.%02d%%\n",
                        i, miu_clients[i].name, effi*100/1024, (effi*10000/1024)%100,
                        bw[0]*100/1024, (bw[0]*10000/1024)%100, bw[1]*100/1024, (bw[1]*10000/1024)%100,
                        bw[2]*100/1024, (bw[2]*10000/1024)%100, bw[3]*100/1024, (bw[3]*10000/1024)%100);
                }
                else {
                    str +=  scnprintf(str, end - str, "%3d:%s\t"ASCII_COLOR_RED"%2d.%02d%%"ASCII_COLOR_END"\t%2d.%02d%%\t%2d.%02d%%\t%2d.%02d%%\t%2d.%02d%%\n",
                        i, miu_clients[i].name, effi*100/1024, (effi*10000/1024)%100,
                        bw[0]*100/1024, (bw[0]*10000/1024)%100, bw[1]*100/1024, (bw[1]*10000/1024)%100,
                        bw[2]*100/1024, (bw[2]*10000/1024)%100, bw[3]*100/1024, (bw[3]*10000/1024)%100);
                }
            }
            if (effi != 0x3FF) {
                effi_last = effi;
            }
        }
    }

    if (str > buf)
        str--;
    str += scnprintf(str, end - str, "\n");
    return (str - buf);
}

#ifdef CONFIG_CAM_CLK
static ssize_t dram_info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    unsigned char m = MIU_IDX(dev->kobj.name[3]);
    unsigned int iMiuBankAddr = 0;
    unsigned int iAtopBankAddr = 0;
    unsigned int dram_type = 0;

    if (m == 0) {
        iMiuBankAddr = BASE_REG_MIU_PA;
        iAtopBankAddr = BASE_REG_ATOP_PA;
    }
#if MIU_NUM > 1
    else {
        printk("NOT support multiple MIUs\n");
        return 0;
    }
#endif

    dram_type = INREGMSK16(iMiuBankAddr + REG_ID_01, 0x0003);

    str += scnprintf(str, end - str, "DRAM Type:   %s\n", (dram_type==3)? "DDR3" : (dram_type==2)? "DDR2" : "Unknown");
    str += scnprintf(str, end - str, "DRAM Size:   %dMB\n", 1 << (INREGMSK16(iMiuBankAddr + REG_ID_69, 0xF000) >> 12));
    str += scnprintf(str, end - str, "DRAM Freq:   %dMHz\n", CamClkRateGet(CAMCLK_ddrpll_clk) / 1000000);
    str += scnprintf(str, end - str, "MIUPLL Freq: %dMHz\n", CamClkRateGet(CAMCLK_miupll_clk) / 1000000);
    str += scnprintf(str, end - str, "Data Rate:   %dx Mode\n", 1 << (INREGMSK16(iMiuBankAddr + REG_ID_01, 0x0300) >> 8));
    str += scnprintf(str, end - str, "Bus Width:   %dbit\n", 16 << (INREGMSK16(iMiuBankAddr + REG_ID_01, 0x000C) >> 2));
    str += scnprintf(str, end - str, "SSC:         %s\n", (INREGMSK16(iAtopBankAddr + REG_ID_14, 0x8000)==0x8000)? "OFF" : "ON");

    if (str > buf)
        str--;

    str += scnprintf(str, end - str, "\n");

    return (str - buf);
}

#else
static ssize_t dram_info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    unsigned char m = MIU_IDX(dev->kobj.name[3]);
    unsigned int iMiuBankAddr = 0;
    unsigned int iAtopBankAddr = 0;
    unsigned int iMiupllBankAddr = 0;
    unsigned int dram_type = 0;
    unsigned int ddfset = 0;
    unsigned int dram_freq = 0;
    unsigned int miupll_freq = 0;

    if (m == 0) {
        iMiuBankAddr = BASE_REG_MIU_PA;
        iAtopBankAddr = BASE_REG_ATOP_PA;
        iMiupllBankAddr = BASE_REG_MIUPLL_PA;
    }
#if MIU_NUM > 1
    else {
        printk("NOT support multiple MIUs\n");
        return 0;
    }
#endif

    dram_type = INREGMSK16(iMiuBankAddr + REG_ID_01, 0x0003);
    ddfset = (INREGMSK16(iAtopBankAddr + REG_ID_19, 0x00FF) << 16) + INREGMSK16(iAtopBankAddr + REG_ID_18, 0xFFFF);
    dram_freq = ((432 * 4 * 4) << 19) / ddfset;
    miupll_freq = 24 * INREGMSK16(iMiupllBankAddr + REG_ID_03, 0x00FF) / ((INREGMSK16(iMiupllBankAddr + REG_ID_03, 0x0700) >> 8) + 2);

    str += scnprintf(str, end - str, "DRAM Type:   %s\n", (dram_type==3)? "DDR3" : (dram_type==2)? "DDR2" : "Unknown");
    str += scnprintf(str, end - str, "DRAM Size:   %dMB\n", 1 << (INREGMSK16(iMiuBankAddr + REG_ID_69, 0xF000) >> 12));
    str += scnprintf(str, end - str, "DRAM Freq:   %dMHz\n", dram_freq);
    str += scnprintf(str, end - str, "MIUPLL Freq: %dMHz\n", miupll_freq);
    str += scnprintf(str, end - str, "Data Rate:   %dx Mode\n", 1 << (INREGMSK16(iMiuBankAddr + REG_ID_01, 0x0300) >> 8));
    str += scnprintf(str, end - str, "Bus Width:   %dbit\n", 16 << (INREGMSK16(iMiuBankAddr + REG_ID_01, 0x000C) >> 2));
    str += scnprintf(str, end - str, "SSC:         %s\n", (INREGMSK16(iAtopBankAddr + REG_ID_14, 0x8000)==0x8000)? "OFF" : "ON");

    if (str > buf)
        str--;

    str += scnprintf(str, end - str, "\n");

    return (str - buf);
}
#endif

DEVICE_ATTR(monitor_client_enable, 0644, monitor_client_enable_show, monitor_client_enable_store);
DEVICE_ATTR(monitor_client_disable, 0644, monitor_client_disable_show, monitor_client_disable_store);
DEVICE_ATTR(monitor_set_duration_ms, 0644, monitor_set_counts_avg_show, monitor_set_counts_avg_store);
DEVICE_ATTR(measure_all, 0644, measure_all_show, measure_all_store);
DEVICE_ATTR(dram_info, 0444, dram_info_show, NULL);

#ifdef CONFIG_PM_SLEEP
int miu_subsys_suspend(struct device *dev)
{
    if (dev == &miu[0].dev) {
        // keep dram size setting
        miu[0].reg_dram_size = INREGMSK16(BASE_REG_MIU_PA + REG_ID_69, 0xF000);
    }

    pr_debug("miu subsys suspend %s\n", dev->kobj.name);
    return 0;
}

int miu_subsys_resume(struct device *dev)
{
    if (dev == &miu[0].dev) {
        // restore dram size setting
        OUTREGMSK16(BASE_REG_MIU_PA + REG_ID_69, miu[0].reg_dram_size, 0xF000);
    }
    else if (strncmp(dev->kobj.name, "miu_arb", 7) == 0) {
        miu_arb_resume();
    }

    pr_debug("miu subsys resume %s\n", dev->kobj.name);
    return 0;
}
#endif

void mstar_create_MIU_node(void)
{
    int ret = 0, i;

    //initialize MIU client table
    memset(miu_clients_bw, 0, MIU_NUM*MIU_CLIENT_NUM);
    //OVERALL client monitor enable
    for(i = 0; i < MIU_NUM; i++)
    {
        miu_clients_bw[i][0].enabled = 1;
        miu_devname[i][0] = 'm'; miu_devname[i][1] = 'i'; miu_devname[i][2] = 'u'; miu_devname[i][3] = '0' + i;
    }

    ret = subsys_system_register(&miu_subsys, NULL);
    if (ret) {
        printk(KERN_ERR "Failed to register miu sub system!! %d\n",ret);
        return;
    }
    for(i = 0; i < MIU_NUM; i++)
    {
        miu[i].index = 0;
        miu[i].dev.kobj.name = (const char *)miu_devname[i];
        miu[i].dev.bus = &miu_subsys;

        ret = device_register(&miu[i].dev);
        if (ret) {
            printk(KERN_ERR "Failed to register %s device!! %d\n",miu[i].dev.kobj.name,ret);
            return;
        }

        device_create_file(&miu[i].dev, &dev_attr_monitor_client_enable);
        device_create_file(&miu[i].dev, &dev_attr_monitor_client_disable);
        device_create_file(&miu[i].dev, &dev_attr_monitor_set_duration_ms);
        device_create_file(&miu[i].dev, &dev_attr_measure_all);
        device_create_file(&miu[i].dev, &dev_attr_dram_info);
    }
#if CONFIG_SS_MIU_ARBITRATION
    create_miu_arb_node(&miu_subsys);
#endif
}
