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
#include <asm/uaccess.h>
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
    else if (strcmp(name, "MIU_BIST256_OCCUPY") == 0)
        return 0;
    else if (strcmp(name, "MIU_BIST256_STOP") == 0)
        return 0;
    else if (strcmp(name, "MIU_PA_SET_MAX_SERVICE") == 0)
        return 0;
    else if (strcmp(name, "MIU_PREARB_SET_RCMD_WIN") == 0)
        return 0;
    else if (strcmp(name, "MIU_PREARB_SET_WCMD_WIN") == 0)
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
                CLRREG16(BASE_REG_XTAL_ATOP_PA + REG_ID_06, BIT0); // enable clock
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
                SETREG16(BASE_REG_XTAL_ATOP_PA + REG_ID_06, BIT1); // disable clk
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
    MSYS_WARN("%s %d\r\n", mem_info.name, __LINE__);
    if (strcmp(mem_info.name, "MIU_BIST_ALL") == 0)
    {
        printk("MIU BIST All!!\r\n");
        // grp_sc0
        miu_bist(mem_info.phys - CONFIG_MIU0_BUS_BASE, mem_info.length, 0, BASE_REG_MIU_GRP_SC0, BASE_REG_MIU_EFFI_SC0);
        // grp_sc1
        miu_bist(mem_info.phys - CONFIG_MIU0_BUS_BASE, mem_info.length, 0, BASE_REG_MIU_GRP_SC1, BASE_REG_MIU_EFFI_SC1);
        // grp_isp0
        miu_bist(mem_info.phys - CONFIG_MIU0_BUS_BASE, mem_info.length, 0, BASE_REG_MIU_GRP_ISP0,
                 BASE_REG_MIU_EFFI_ISP0);
        // grp_disp0
        miu_bist(mem_info.phys - CONFIG_MIU0_BUS_BASE, mem_info.length, 0, BASE_REG_MIU_GRP_DISP0,
                 BASE_REG_MIU_EFFI_DISP0);
        // grp_net0
        miu_bist(mem_info.phys - CONFIG_MIU0_BUS_BASE, mem_info.length, 0, BASE_REG_MIU_GRP_NET0,
                 BASE_REG_MIU_EFFI_NET0);
    }
    else if (strcmp(mem_info.name, "MIU_BIST_VENC0_GOP") == 0)
    {
        printk("MIU BIST VENC0 GOP!!\r\n");
        // grp_sc0
        miu_bist(mem_info.phys - CONFIG_MIU0_BUS_BASE, mem_info.length, 0, BASE_REG_MIU_GRP_SC0, BASE_REG_MIU_EFFI_SC0);
        // grp_sc1
        miu_bist(mem_info.phys - CONFIG_MIU0_BUS_BASE, mem_info.length, 0, BASE_REG_MIU_GRP_SC1, BASE_REG_MIU_EFFI_SC1);
        // grp_isp0
        miu_bist(mem_info.phys - CONFIG_MIU0_BUS_BASE, mem_info.length, 0, BASE_REG_MIU_GRP_ISP0,
                 BASE_REG_MIU_EFFI_ISP0);
        // grp_net0
        miu_bist(mem_info.phys - CONFIG_MIU0_BUS_BASE, mem_info.length, 0, BASE_REG_MIU_GRP_NET0,
                 BASE_REG_MIU_EFFI_NET0);
    }
    else if (strcmp(mem_info.name, "MIU_BIST_OFF") == 0)
    {
        printk("MIU_BIST_OFF!!\r\n");
        miu_bist_off(BASE_REG_MIU_GRP_SC0, BASE_REG_MIU_EFFI_SC0);
        miu_bist_off(BASE_REG_MIU_GRP_SC1, BASE_REG_MIU_EFFI_SC1);
        miu_bist_off(BASE_REG_MIU_GRP_ISP0, BASE_REG_MIU_EFFI_ISP0);
        miu_bist_off(BASE_REG_MIU_GRP_DISP0, BASE_REG_MIU_EFFI_DISP0);
        miu_bist_off(BASE_REG_MIU_GRP_NET0, BASE_REG_MIU_EFFI_NET0);
    }
    else if (strcmp(mem_info.name, "MIU_BIST256_OCCUPY") == 0)
    {
        OUTREG16(BASE_REG_MIU_PRE_ARB + (0x62 << 2), 0x0010);
        OUTREG16(BASE_REG_MIU_PRE_ARB + (0x6e << 2), 0x1007);
        OUTREG16(BASE_REG_MIU_PRE_ARB + (0x62 << 2), 0x0011);
    }
    else if (strcmp(mem_info.name, "MIU_BIST256_STOP") == 0)
    {
        OUTREG16(BASE_REG_MIU_PRE_ARB + (0x6e << 2), 0x1006);
        OUTREG16(BASE_REG_MIU_PRE_ARB + (0x62 << 2), 0x0010);
    }
    else if (strcmp(mem_info.name, "MIU_PA_SET_MAX_SERVICE") == 0)
    {
        char name[32];
        // unsigned int size;
        unsigned int normal_trans, cpu_trans, dla_trans;

        // sscanf(buf, "%s %d %x %x %x", name, &size, &normal_trans, &cpu_trans, &dla_trans);
        sscanf(buf, "%s %x %x %x", name, &normal_trans, &cpu_trans, &dla_trans);
        printk("parameters: %x %x %x\r\n", normal_trans, cpu_trans, dla_trans);
        if (normal_trans < 0x100)
        {
            OUTREGMSK16(BASE_REG_MIU_PA + (0x00 << 2), normal_trans & 0x00FF, 0x00FF);
            OUTREGMSK16(BASE_REG_MIU_PA + (0x20 << 2), normal_trans & 0x00FF, 0x00FF);
        }
        else
        {
            printk("Wrong parameter, normal transaction(%d) > 255\r\n", normal_trans);
        }

        if (cpu_trans < 0x100)
        {
            OUTREGMSK16(BASE_REG_MIU_PA + (0x02 << 2), cpu_trans & 0x00FF, 0x00FF);
            OUTREGMSK16(BASE_REG_MIU_PA + (0x21 << 2), cpu_trans & 0x00FF, 0x00FF);
        }
        else
        {
            printk("Wrong parameter, normal transaction(%d) > 255\r\n", cpu_trans);
        }

        if (dla_trans < 0x100)
        {
            OUTREGMSK16(BASE_REG_MIU_PA + (0x01 << 2), dla_trans & 0x00FF, 0x00FF);
            OUTREGMSK16(BASE_REG_MIU_PA + (0x22 << 2), dla_trans & 0x00FF, 0x00FF);
        }
        else
        {
            printk("Wrong parameter, normal transaction(%d) > 255\r\n", dla_trans);
        }

        SETREG16(BASE_REG_MIU_PA + (0x1F << 2), BIT0);
        SETREG16(BASE_REG_MIU_PA + (0x3F << 2), BIT0);
    }
    else if (strcmp(mem_info.name, "MIU_PREARB_SET_RCMD_WIN") == 0)
    {
        char name[32];
        // unsigned int size;
        unsigned int win_id, win_data;

        // sscanf(buf, "%s %d %x %x", name, &size, &win_id, &win_data);
        sscanf(buf, "%s %x %x", name, &win_id, &win_data);
        printk("parameters: %d %x \r\n", win_id, win_data);
        if (win_id < 16)
        {
            OUTREG16(BASE_REG_MIU_PRE_ARB + (win_id << 2), win_data & 0xFFFF);
            OUTREGMSK16(BASE_REG_MIU_PRE_ARB + (0x13 << 2), BIT2, BIT2);
        }
        else
            printk("Wrong parameter, window id >= 16\r\n");
    }
    else if (strcmp(mem_info.name, "MIU_PREARB_SET_WCMD_WIN") == 0)
    {
        char name[32];
        // unsigned int size;
        unsigned int win_id, win_data;

        // sscanf(buf, "%s %d %x %x", name, &size, &win_id, &win_data);
        sscanf(buf, "%s %x %x", name, &win_id, &win_data);
        printk("parameters: %d %x \r\n", win_id, win_data);
        if (win_id < 16)
        {
            OUTREG16(BASE_REG_MIU_PRE_ARB + ((win_id + 0x30) << 2), win_data & 0xFFFF);
            OUTREGMSK16(BASE_REG_MIU_PRE_ARB + (0x43 << 2), BIT2, BIT2);
        }
        else
            printk("Wrong parameter, window id >= 16\r\n");
    }
    else
    {
        printk("Not support this command\r\n");
    }
}
