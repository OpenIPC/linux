/*
 * platform_msys.c- Sigmastar
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
//#include <asm/uaccess.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include "ms_platform.h"
#include "gpio.h"
#include "registers.h"
#include "mdrv_msys_io_st.h"
#include "../platform_msys.h"

#define MSYS_ERROR(fmt, args...) printk(KERN_ERR "MSYS: " fmt, ##args)
#define MSYS_WARN(fmt, args...)  printk(KERN_WARNING "MSYS: " fmt, ##args)

#define BIST_READ_ONLY  BIT5
#define BIST_WRITE_ONLY BIT6

#define BIST_SINGLE_MODE 0x0000
#define BIST_LOOP_MODE   0x0010

#define BIST_CMDLEN_1  0x0000
#define BIST_CMDLEN_8  0x0400
#define BIST_CMDLEN_16 0x0800
#define BIST_CMDLEN_64 0x0C00

#define BIST_BURSTLEN_16  0x0000
#define BIST_BURSTLEN_32  0x0100
#define BIST_BURSTLEN_64  0x0200
#define BIST_BURSTLEN_128 0x0300

U16 burst_len = BIST_CMDLEN_64 | BIST_BURSTLEN_128;

int msys_miu_check_cmd(const char *buf)
{
    char name[32];

    sscanf(buf, "%s", name);

    if (strcmp(name, "MIU_BIST_OFF") == 0)
        return 0;
    else if (strcmp(name, "MIU_BIST_BURST_LEN") == 0)
        return 0;

    return 1;
}

int msys_miu_set_bist(U8 enable)
{
    MSYS_WARN("Not support this command\r\n");
    return 0;
}

int msys_miu_client_switch(msys_miu_client_sw_e sw)
{
    switch (sw)
    {
        case MSYS_MIU_CLIENT_SW_BIST:
            break;
        case MSYS_MIU_CLIENT_SW_TVTOOL:
            break;
        default:
            return -1;
    }

    return 0;
}

int msys_request_freq(MSYS_FREQGEN_INFO *freq_info)
{
    if (freq_info->padnum != PAD_CLOCK_OUT)
    {
        MSYS_ERROR("Not implement PAD: %d for this platform\n", freq_info->padnum);
        return -EINVAL;
    }

    if (freq_info->bEnable)
    {
        if (freq_info->freq != FREQ_24MHZ)
        {
            MSYS_ERROR("Not implement FREQ: %d for this platform\n", freq_info->freq);
            return -EINVAL;
        }

        if (freq_info->bInvert != false)
            MSYS_WARN("Not support invert clock in this platform");

        switch (freq_info->padnum)
        {
            case PAD_CLOCK_OUT:
                // reg_ext_xtali_se_enb[1]=0
                CLRREG16(BASE_REG_PMSLEEP_PA + REG_ID_30, BIT1); // enable clock
                break;
            default:
                MSYS_ERROR("Not implement PAD: %d for this platform\n", freq_info->padnum);
                break;
        }
    }
    else // disable clk
    {
        switch (freq_info->padnum)
        {
            case PAD_CLOCK_OUT:
                // reg_ext_xtali_se_enb[1]=1
                SETREG16(BASE_REG_PMSLEEP_PA + REG_ID_30, BIT1); // disable clk
                break;
            default:
                MSYS_ERROR("Not implement PAD: %d for this platform\n", freq_info->padnum);
                break;
        }
    }
    return 0;
}

// mode: 0-> r/w, 1-> r
static void miu_bist(u64 start_addr, u64 length, u16 mode, u32 grp_base, u32 effi_base)
{
    U16 is_restart = 0, loop = 0;
    U16 cmd;

    is_restart = INREG16(effi_base + (0x50 << 2)) & 0x0001;

    cmd = burst_len;
    // read mode/one-shoot
    if (mode)
    {
        // set 1 to access BIST
        SETREG16(grp_base + (0x59 << 2), BIT15);
        cmd |= BIST_READ_ONLY | BIST_LOOP_MODE;
        OUTREG16(effi_base + (0x50 << 2), cmd);
    }
    else
    {
        // set 1 to access BIST
        SETREG16(grp_base + (0x59 << 2), BIT15 | BIT14);
        cmd |= BIST_LOOP_MODE;
        OUTREG16(effi_base + (0x50 << 2), cmd);
    }

    // check bist enabled, need to wait finish done
    if (is_restart)
    {
        while ((INREG16(effi_base + (0x59 << 2)) & BIT2) == 0)
        {
            msleep_interruptible(100);
            loop++;
            if (loop > 20)
            {
                printk("warning, miu bist can't wait finish bit over 2s.\r\n");
                break;
            }
        }
    }

    // test data byte
    OUTREG16(effi_base + (0x51 << 2), 0xffff);

    OUTREG16(effi_base + (0x52 << 2), 0x0000);
    OUTREG16(effi_base + (0x56 << 2), 0x0000);

    // test start address
    OUTREG16(effi_base + (0x57 << 2), (start_addr >> 10) & 0xFFFF);
    OUTREG16(effi_base + (0x58 << 2), (start_addr >> 26) & 0xFFFF);

    // test start length
    OUTREG16(effi_base + (0x54 << 2), (length >> 4) & 0xFFFF);
    OUTREG16(effi_base + (0x55 << 2), (length >> 20) & 0xFFFF);

    // trigger bist
    SETREG16(effi_base + (0x50 << 2), BIT0);
}

static void miu_bist_off(u32 grp_base, u32 effi_base)
{
    U16 loop = 0;
    U16 bank;

    if ((INREG16(effi_base + (0x50 << 2)) & BIT0) == 0)
        return;

    bank = (grp_base >> 9) & 0x1FFF;
    // printk("bist off: 0x%X\r\n", bank);

    CLRREG16(effi_base + (0x50 << 2), BIT0);
    while ((INREG16(effi_base + (0x59 << 2)) & BIT2) == 0)
    {
        msleep_interruptible(100);
        loop++;
        if (loop > 20)
        {
            printk("warning, miu bist(0x%X) can't wait finish bit over 2s.\r\n", bank);
            break;
        }
    }

    CLRREG16(grp_base + (0x59 << 2), BIT15 | BIT14);
}

void msys_miu_ioctl(MSYS_DMEM_INFO mem_info, const char *buf)
{
    if (strcmp(mem_info.name, "MIU_BIST_ALL") == 0)
    {
        printk("MIU BIST All!!\r\n");
        // grp_sc0
        miu_bist(mem_info.phys - CONFIG_MIU0_BUS_BASE, mem_info.length, 0, BASE_REG_MIU_GRP_SC0, BASE_REG_MIU_EFFI_SC0);
        // grp_sc1
        miu_bist(mem_info.phys - CONFIG_MIU0_BUS_BASE, mem_info.length, 0, BASE_REG_MIU_GRP_SC1, BASE_REG_MIU_EFFI_SC1);
        // grp_sc2
        miu_bist(mem_info.phys - CONFIG_MIU0_BUS_BASE, mem_info.length, 0, BASE_REG_MIU_GRP_SC2, BASE_REG_MIU_EFFI_SC2);
        // grp_isp0
        miu_bist(mem_info.phys - CONFIG_MIU0_BUS_BASE, mem_info.length, 0, BASE_REG_MIU_GRP_ISP0,
                 BASE_REG_MIU_EFFI_ISP0);
        // grp_isp1
        miu_bist(mem_info.phys - CONFIG_MIU0_BUS_BASE, mem_info.length, 0, BASE_REG_MIU_GRP_ISP1,
                 BASE_REG_MIU_EFFI_ISP1);
        // grp_disp0
        miu_bist(mem_info.phys - CONFIG_MIU0_BUS_BASE, mem_info.length, 0, BASE_REG_MIU_GRP_DISP0,
                 BASE_REG_MIU_EFFI_DISP0);
        // grp_disp1
        miu_bist(mem_info.phys - CONFIG_MIU0_BUS_BASE, mem_info.length, 1, BASE_REG_MIU_GRP_DISP1,
                 BASE_REG_MIU_EFFI_DISP1);
    }
    else if (strcmp(mem_info.name, "MIU_BIST_VIDEO_PIPE") == 0)
    {
        printk("MIU BIST for Video-Pipe!!\r\n");
        // grp_sc2
        miu_bist(mem_info.phys - CONFIG_MIU0_BUS_BASE, mem_info.length, 0, BASE_REG_MIU_GRP_SC2, BASE_REG_MIU_EFFI_SC2);
        // grp_isp0
        miu_bist(mem_info.phys - CONFIG_MIU0_BUS_BASE, mem_info.length, 0, BASE_REG_MIU_GRP_ISP0,
                 BASE_REG_MIU_EFFI_ISP0);
        // grp_isp1
        miu_bist(mem_info.phys - CONFIG_MIU0_BUS_BASE, mem_info.length, 0, BASE_REG_MIU_GRP_ISP1,
                 BASE_REG_MIU_EFFI_ISP1);
    }
    else if (strcmp(mem_info.name, "MIU_BIST_IVE") == 0)
    {
        printk("MIU BIST for IVE!!\r\n");
        // grp_sc0
        miu_bist(mem_info.phys - CONFIG_MIU0_BUS_BASE, mem_info.length, 0, BASE_REG_MIU_GRP_SC0, BASE_REG_MIU_EFFI_SC0);
        // grp_sc2
        miu_bist(mem_info.phys - CONFIG_MIU0_BUS_BASE, mem_info.length, 0, BASE_REG_MIU_GRP_SC2, BASE_REG_MIU_EFFI_SC2);
        // grp_isp0
        miu_bist(mem_info.phys - CONFIG_MIU0_BUS_BASE, mem_info.length, 0, BASE_REG_MIU_GRP_ISP0,
                 BASE_REG_MIU_EFFI_ISP0);
        // grp_isp1
        miu_bist(mem_info.phys - CONFIG_MIU0_BUS_BASE, mem_info.length, 0, BASE_REG_MIU_GRP_ISP1,
                 BASE_REG_MIU_EFFI_ISP1);
        // grp_disp0
        miu_bist(mem_info.phys - CONFIG_MIU0_BUS_BASE, mem_info.length, 0, BASE_REG_MIU_GRP_DISP0,
                 BASE_REG_MIU_EFFI_DISP0);
        // grp_disp1
        miu_bist(mem_info.phys - CONFIG_MIU0_BUS_BASE, mem_info.length, 1, BASE_REG_MIU_GRP_DISP1,
                 BASE_REG_MIU_EFFI_DISP1);
    }
    else if (strcmp(mem_info.name, "MIU_BIST_SC_GOP1") == 0)
    {
        printk("MIU BIST for SC-GOP1!!\r\n");
        // grp_sc1
        miu_bist(mem_info.phys - CONFIG_MIU0_BUS_BASE, mem_info.length, 0, BASE_REG_MIU_GRP_SC1, BASE_REG_MIU_EFFI_SC1);
        // grp_sc2
        miu_bist(mem_info.phys - CONFIG_MIU0_BUS_BASE, mem_info.length, 0, BASE_REG_MIU_GRP_SC2, BASE_REG_MIU_EFFI_SC2);
        // grp_isp0
        miu_bist(mem_info.phys - CONFIG_MIU0_BUS_BASE, mem_info.length, 0, BASE_REG_MIU_GRP_ISP0,
                 BASE_REG_MIU_EFFI_ISP0);
        // grp_isp1
        miu_bist(mem_info.phys - CONFIG_MIU0_BUS_BASE, mem_info.length, 0, BASE_REG_MIU_GRP_ISP1,
                 BASE_REG_MIU_EFFI_ISP1);
        // grp_disp0
        miu_bist(mem_info.phys - CONFIG_MIU0_BUS_BASE, mem_info.length, 0, BASE_REG_MIU_GRP_DISP0,
                 BASE_REG_MIU_EFFI_DISP0);
        // grp_disp1
        miu_bist(mem_info.phys - CONFIG_MIU0_BUS_BASE, mem_info.length, 1, BASE_REG_MIU_GRP_DISP1,
                 BASE_REG_MIU_EFFI_DISP1);
    }
    else if ((strcmp(mem_info.name, "MIU_BIST_GE") == 0) || (strcmp(mem_info.name, "MIU_BIST_GMAC1") == 0))
    {
        printk("MIU BIST for GE/GMAC1!!\r\n");
        // grp_sc0
        miu_bist(mem_info.phys - CONFIG_MIU0_BUS_BASE, mem_info.length, 0, BASE_REG_MIU_GRP_SC0, BASE_REG_MIU_EFFI_SC0);
        // grp_sc1
        miu_bist(mem_info.phys - CONFIG_MIU0_BUS_BASE, mem_info.length, 0, BASE_REG_MIU_GRP_SC1, BASE_REG_MIU_EFFI_SC1);
        // grp_sc2
        miu_bist(mem_info.phys - CONFIG_MIU0_BUS_BASE, mem_info.length, 0, BASE_REG_MIU_GRP_SC2, BASE_REG_MIU_EFFI_SC2);
        // grp_isp0
        miu_bist(mem_info.phys - CONFIG_MIU0_BUS_BASE, mem_info.length, 0, BASE_REG_MIU_GRP_ISP0,
                 BASE_REG_MIU_EFFI_ISP0);
        // grp_isp1
        miu_bist(mem_info.phys - CONFIG_MIU0_BUS_BASE, mem_info.length, 0, BASE_REG_MIU_GRP_ISP1,
                 BASE_REG_MIU_EFFI_ISP1);
        // grp_disp1
        miu_bist(mem_info.phys - CONFIG_MIU0_BUS_BASE, mem_info.length, 1, BASE_REG_MIU_GRP_DISP1,
                 BASE_REG_MIU_EFFI_DISP1);
    }
    else if (strcmp(mem_info.name, "MIU_BIST_BURST_LEN") == 0)
    {
        char name[32];
        U32  size;

        printk("MIU BIST SET BURST LEN!!\r\n");
        sscanf(buf, "%s %x", name, &size);
        burst_len = size << 8;
    }
    else if (strcmp(mem_info.name, "MIU_BIST_OFF") == 0)
    {
        printk("MIU_BIST_OFF!!\r\n");
        miu_bist_off(BASE_REG_MIU_GRP_SC0, BASE_REG_MIU_EFFI_SC0);
        miu_bist_off(BASE_REG_MIU_GRP_SC1, BASE_REG_MIU_EFFI_SC1);
        miu_bist_off(BASE_REG_MIU_GRP_SC2, BASE_REG_MIU_EFFI_SC2);
        miu_bist_off(BASE_REG_MIU_GRP_ISP0, BASE_REG_MIU_EFFI_ISP0);
        miu_bist_off(BASE_REG_MIU_GRP_ISP1, BASE_REG_MIU_EFFI_ISP1);
        miu_bist_off(BASE_REG_MIU_GRP_DISP0, BASE_REG_MIU_EFFI_DISP0);
        miu_bist_off(BASE_REG_MIU_GRP_DISP1, BASE_REG_MIU_EFFI_DISP1);
    }
    else
    {
        printk("Warning!  No support this command!!\r\n");
    }
    printk("done\r\n");
}
