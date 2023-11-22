/*
 * drv_ca7.c- Sigmastar
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

#include <linux/module.h>
#include <linux/delay.h>

#include "registers.h"
#include "ms_platform.h"
#include "interos_call.h"
#include "drv_ca7.h"

#define RTOS_VECTOR_OFFSET 0x00008000

#if defined(CONFIG_ARCH_INFINITY7) && (defined(CONFIG_ENABLE_CA7) || defined(CONFIG_ENABLE_CA7_MODULE))
static int ca7_freq = 1000;

static unsigned int ca7_rom[] = {
    0xe321f0d3, // msr CPSR_c, #Mode_SVC|IBIT|FBIT
    0xee110f10, // mrc 15, 0, r0, cr1, cr0, {0}
    0xe3800a01, // orr r0, r0, #4096			;Enable I-Cache
    0xe3c00005, // bic r0, r0, #5			;Disable D-Cache/MMU
    0xee010f10, // mcr 15, 0, r0, cr1, cr0, {0}
    0xee110f30, // mrc 15, 0, r0, cr1, cr0, {1}
    0xe3800040, // orr r0, r0, #64 			;Enable SMP
    0xee010f30, // mcr 15, 0, r0, cr1, cr0, {1}
    0xe3a00902, // mov r0, #32768  			;offset 0x8000
    0xe12fff30, // blx r0
};

void ca7_set_freq(int freq)
{
    ca7_freq = freq;
}

static int ca7_file_read(char *path, void *pdst)
{
    struct file *fp;
    int          ret = 0;

    fp = filp_open(path, O_RDWR, 0755);
    if (IS_ERR(fp))
    {
        ret = PTR_ERR(fp);
        pr_err("Open file %s error ret=%d\n", path, ret);
    }
    else
    {
        loff_t       pos = 0;
        struct kstat stat;

        ret = vfs_getattr(&fp->f_path, &stat, STATX_INO, AT_STATX_SYNC_AS_STAT);
        if (ret < 0)
            pr_err("Error reading %s file stat\n", path);

        ret = kernel_read(fp, pdst, stat.size, &pos);
        if (ret < 0)
            printk("Error reading %s file data\n", path);

        filp_close(fp, NULL);
    }

    return ret;
}

int __init ca7_start(void)
{
#ifdef CONFIG_SSTAR_CA7_VIRTIO
    interos_call_mbox_args_t *ptr_mbox_args;
#endif
    void *vptr;
    int   ret = 0;
    int   pll = 0;

    if (ca7_freq < 600 || ca7_freq > 1200)
    {
        return -EINVAL;
    }

    // ARM PLL PD (Power-down)
    //  0: power-on
    //  1: power-down (Default)
    // reg_ckg_mips_syn
    // reg[103223]#0
    OUTREG8(GET_REG_ADDR8(BASE_REG_CA7PLL_PA, 0x23), 0x01);

    /*
    formula:
    PLL default setting is synthesizer frequency * (24/2)
    If target = 1200MHz , synthesizer = 1200 / 12 = 100MHz
    Synthesizer setting = 432 / 100 * 2^19 = 0x228F5C
    transfer {0x61, 0x60} = 32!|h228F
    Scenario :
    1200 MHz : 432*512*1024*12/ 1200 = 0x228F5C
    1000 MHz : 432*512*1024*12/ 1000 = 0x2978D4
    800 MHz : 432*512*1024*12/ 800 = 0x33D70A
    600 MHz : 432*512*1024*12/ 600 = 0x451EB8
    */
    pll = (432 * 512 * 1024 * 12UL) / ca7_freq;
    OUTREG8(GET_REG_ADDR8(BASE_REG_CA7PLL_PA, 0xC2), (pll >> 16) & 0xFFFF);
    OUTREG16(GET_REG_ADDR8(BASE_REG_CA7PLL_PA, 0xC0), pll & 0xFFFF);

    OUTREG8(GET_REG_ADDR8(BASE_REG_CA7PLL_PA, 0xC4), 0x01); // update ARM frequency synthesizer N.f setting
    OUTREG16(GET_REG_ADDR8(BASE_REG_CA7PLL_PA, 0x22), 0x0080);
    udelay(200);

#ifdef CONFIG_SSTAR_CA7_VIRTIO
    ptr_mbox_args         = (interos_call_mbox_args_t *)(BASE_REG_MAILBOX_PA + BK_REG(0x50) + IO_OFFSET);
    ptr_mbox_args->arg0_l = 0;
#endif

    vptr = ioremap_wc(CONFIG_CA7_MEM_PHY_ADDR, CONFIG_CA7_MEM_PHY_SIZE);
    if (vptr == NULL)
    {
        return -ENOMEM;
    }

    memset(vptr, 0, CONFIG_CA7_MEM_PHY_SIZE);

    memcpy(vptr, ca7_rom, sizeof(ca7_rom));

    ret = ca7_file_read(CONFIG_CA7_RTOS_PATH, vptr + RTOS_VECTOR_OFFSET);
    if (ret < 0)
        goto error;

    // boot offset of CA7
    OUTREG16(GET_REG_ADDR(BASE_REG_CHIPTOP_PA, 0x67), (CONFIG_CA7_MEM_PHY_ADDR / 0x400) & 0xffff);
    OUTREG16(GET_REG_ADDR(BASE_REG_CHIPTOP_PA, 0x68), (CONFIG_CA7_MEM_PHY_ADDR / 0x400) >> 16);

    // dram offset of CA7
    OUTREG16(GET_REG_ADDR(BASE_REG_CHIPTOP_PA, 0x5C), ((CONFIG_CA7_MEM_PHY_ADDR - MIU0_LOW_BASE) / 0x400) & 0xffff);
    OUTREG16(GET_REG_ADDR(BASE_REG_CHIPTOP_PA, 0x5D), ((CONFIG_CA7_MEM_PHY_ADDR - MIU0_LOW_BASE) / 0x400) >> 16);

    OUTREG16(GET_REG_ADDR(BASE_REG_CHIPTOP_PA, 0x14), 0x0000); // bring up CA7
    OUTREG16(GET_REG_ADDR(BASE_REG_CHIPTOP_PA, 0x16), 0x0000);
    OUTREG16(GET_REG_ADDR(BASE_REG_CHIPTOP_PA, 0x15), 0x0000);
    OUTREG16(GET_REG_ADDR(BASE_REG_CA7_PA, 0x01), 0x0480);
    OUTREG16(GET_REG_ADDR(BASE_REG_CA7_PA, 0x02), 0x4000);

    OUTREG8(GET_REG_ADDR8(BASE_REG_CA7PLL_PA, 0x3F), 0x40);    // add for U02
    udelay(10);                                                // wait 10us for stable
    OUTREG8(GET_REG_ADDR8(BASE_REG_CA7_HEMCU_PA, 0xF8), 0x01); // select PLL

    OUTREG16(GET_REG_ADDR(BASE_REG_CHIPTOP_PA, 0x13), 0x0000);

    pr_notice("CA7 start\n");

#ifdef CONFIG_SSTAR_CA7_VIRTIO
    while (1)
    {
        if (ptr_mbox_args->arg0_l == 0xBABE)
        {
            ptr_mbox_args->arg0_l = 0;
            break;
        }
        udelay(1000);
    }
#endif

error:
    iounmap(vptr);

    if (ret >= 0)
        ret = 0;

    return ret;
}

void __exit ca7_stop(void)
{
    OUTREG16(GET_REG_ADDR(BASE_REG_CHIPTOP_PA, 0x13), 0x0001);
    OUTREG16(GET_REG_ADDR(BASE_REG_CHIPTOP_PA, 0x14), 0x0002);
    OUTREG16(GET_REG_ADDR(BASE_REG_CHIPTOP_PA, 0x15), 0x0004);
    OUTREG16(GET_REG_ADDR(BASE_REG_CHIPTOP_PA, 0x16), 0x0008);
    OUTREG16(GET_REG_ADDR(BASE_REG_CA7_PA, 0x01), 0x0404);
    OUTREG16(GET_REG_ADDR(BASE_REG_CA7_PA, 0x02), 0x0800);

    pr_notice("CA7 stop\n");
}

#ifdef CONFIG_ENABLE_CA7_MODULE
module_init(ca7_start);
module_exit(ca7_stop);
module_param(ca7_freq, int, 0);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SigmaStar");
MODULE_DESCRIPTION("CA7 driver");
MODULE_VERSION("1.0");
#endif
#endif
