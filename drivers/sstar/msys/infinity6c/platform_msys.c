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
#include "ms_platform.h"
#include "gpio.h"
#include "registers.h"
#include "mdrv_msys_io_st.h"
#include "../platform_msys.h"

#define MSYS_ERROR(fmt, args...) printk(KERN_ERR "MSYS: " fmt, ##args)
#define MSYS_WARN(fmt, args...)  printk(KERN_WARNING "MSYS: " fmt, ##args)

int msys_miu_check_cmd(const char *buf)
{
    char name[32];

    sscanf(buf, "%s", name);

    if (strcmp(name, "MIU_BIST_ALL") == 0)
        return 1;
    else if (strcmp(name, "MIU_BIST_ONESHOT") == 0)
        return 1;
    else if (strcmp(name, "MIU_BIST1_ONESHOT") == 0)
        return 1;
    else if (strcmp(name, "MIU_BIST2_ONESHOT") == 0)
        return 1;
    else if (strcmp(name, "MIU_GC0_BIST") == 0)
        return 1;
    else if (strcmp(name, "MIU_GC1_BIST") == 0)
        return 1;
    else if (strcmp(name, "MIU_GC2_BIST") == 0)
        return 1;
    else if (strcmp(name, "MIU_TEST") == 0)
        return 1;

    return 0;
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

static unsigned int               MIU_BANK[3] = {BASE_REG_MIU_GRP_SC0, BASE_REG_MIU_GRP_SC1, BASE_REG_MIU_GRP_ISP0};
extern unsigned long long notrace sched_clock(void);

// mode: 0-> r/w, 1-> r
void miu_bist(u64 start_addr, u64 length, u16 mode, u32 grp_base, u32 effi_base)
{
    unsigned int timer0, timer1;
    printk("Set bist start %llx\r\n", start_addr);
    OUTREG16(effi_base + (0x50 << 2), 0x00); // clear bist
    // read mode loop
    if (mode == 1)
    {
        // set 1 to access BIST
        OUTREGMSK16(grp_base + (0x59 << 2), BIT15, BIT15 | BIT14);
        OUTREG16(effi_base + (0x50 << 2), 0x0C30);
        //        OUTREG16(effi_base + (0x50 << 2), 0x0430);
    }
    else if (mode == 2)
    {
        printk("one shot bist\r\n");
        OUTREGMSK16(grp_base + (0x59 << 2), BIT15 | BIT14, BIT15 | BIT14); // 0xF Bist mux
        OUTREG16(effi_base + (0x50 << 2), 0x0400);                         // one shot
    }
    else
    {
        // set 1 to access BIST
        OUTREGMSK16(grp_base + (0x59 << 2), BIT15 | BIT14, BIT15 | BIT14);
        OUTREG16(effi_base + (0x50 << 2), 0x0410);
    }
    // test data byte
    OUTREG16(effi_base + (0x51 << 2), 0xffff);

    // test start address
    OUTREG16(effi_base + (0x57 << 2), (start_addr >> 10) & 0xFFFF);
    OUTREG16(effi_base + (0x58 << 2), (start_addr >> 26) & 0xFFFF);

    // test start length
    OUTREG16(effi_base + (0x54 << 2), (length >> 4) & 0xFFFF);
    OUTREG16(effi_base + (0x55 << 2), (length >> 20) & 0xFFFF);

    // trigger bist
    SETREG16(effi_base + (0x50 << 2), BIT0);

    if (mode == 2)
    {
        timer0 = sched_clock();
        while (1)
        {
            if ((INREG16(effi_base + (0x59 << 2)) & 0xFF) == 0x14)
            {
                timer1 = sched_clock();
                break;
            }
        }

        timer1 = timer1 - timer0;
        printk("MIU_Bist time: %d(ns)\r\n", timer1);
    }
}

void msys_miu_ioctl(MSYS_DMEM_INFO mem_info, const char *buf)
{
    MSYS_WARN("%s %d\r\n", mem_info.name, __LINE__);
    if (strcmp(mem_info.name, "MIU_BIST_ALL") == 0)
    {
        printk("MIU BIST All\r\n");
        // grp_sc0
        miu_bist(mem_info.phys - 0x20000000, mem_info.length, 0, BASE_REG_MIU_GRP_SC0, BASE_REG_MIU_EFFI_SC0);
        // grp_sc1
        miu_bist(mem_info.phys - 0x20000000, mem_info.length, 0, BASE_REG_MIU_GRP_SC1, BASE_REG_MIU_EFFI_SC1);
        // grp_sc2
        miu_bist(mem_info.phys - 0x20000000, mem_info.length, 0, BASE_REG_MIU_GRP_ISP0, BASE_REG_MIU_EFFI_ISP0);
    }
    else if (strcmp(mem_info.name, "MIU_BIST_OFF") == 0)
    {
        OUTREG16(BASE_REG_MIU_EFFI_SC0 + (0x50 << 2), 0x0410);
        OUTREG16(BASE_REG_MIU_EFFI_SC1 + (0x50 << 2), 0x0410);
        OUTREG16(BASE_REG_MIU_EFFI_ISP0 + (0x50 << 2), 0x0410);
    }
    else if (strcmp(mem_info.name, "MIU_BIST_ONESHOT") == 0)
    {
        miu_bist(mem_info.phys - 0x20000000, mem_info.length, 2, BASE_REG_MIU_GRP_SC0, BASE_REG_MIU_EFFI_SC0);
    }
    else if (strcmp(mem_info.name, "MIU_BIST1_ONESHOT") == 0)
    {
        miu_bist(mem_info.phys - 0x20000000, mem_info.length, 2, BASE_REG_MIU_GRP_SC1, BASE_REG_MIU_EFFI_SC1);
    }
    else if (strcmp(mem_info.name, "MIU_BIST2_ONESHOT") == 0)
    {
        miu_bist(mem_info.phys - 0x20000000, mem_info.length, 2, BASE_REG_MIU_GRP_ISP0, BASE_REG_MIU_EFFI_ISP0);
    }
    else if (strcmp(mem_info.name, "MIU_GC0_BIST") == 0)
    {
        // grp_sc0
        miu_bist(mem_info.phys - 0x20000000, mem_info.length, 1, BASE_REG_MIU_GRP_SC0, BASE_REG_MIU_EFFI_SC0);
    }
    else if (strcmp(mem_info.name, "MIU_GC1_BIST") == 0)
    {
        // grp_sc1
        miu_bist(mem_info.phys - 0x20000000, mem_info.length, 1, BASE_REG_MIU_GRP_SC1, BASE_REG_MIU_EFFI_SC1);
    }
    else if (strcmp(mem_info.name, "MIU_GC2_BIST") == 0)
    {
        // grp_sc2
        miu_bist(mem_info.phys - 0x20000000, mem_info.length, 1, BASE_REG_MIU_GRP_ISP0, BASE_REG_MIU_EFFI_ISP0);
    }

    else if (strcmp(mem_info.name, "MIU_TEST") == 0)
    {
        // for MIU BIST only
        CLRREG16(BASE_REG_MIU_DIG + REG_ID_69, 0xFF);
        OUTREG16(BASE_REG_MIU_DIG + REG_ID_71, (mem_info.phys - 0x20000000) >> 13); // test base address[15:0]
        CLRREG16(BASE_REG_MIU_DIG + REG_ID_6F, BIT2 | BIT3);                        // test base address[17:16]
        OUTREG16(BASE_REG_MIU_DIG + REG_ID_72, (mem_info.length >> 4) & 0xFFFF);    // test length[15:0]
        OUTREG16(BASE_REG_MIU_DIG + REG_ID_73, (mem_info.length >> 20) & 0x0FFF);   // test length[27:16]
        // set CHIPTOP bist 's priority
        OUTREG16(BASE_REG_MIU_ARB + REG_ID_7C, 0xfc21);
        OUTREG16(BASE_REG_MIU_ARB + REG_ID_75, 0x0101);
        OUTREG16(BASE_REG_MIU_ARB + REG_ID_70, 0x0102);
        OUTREG16(BASE_REG_MIU_DIG + REG_ID_0F, 0x8);
        OUTREG16(BASE_REG_MIU_DIG + REG_ID_70, 0x99);
    }

    else if (strcmp(mem_info.name, "MIU_PA_SET_MAX_SERVICE") == 0)
    {
//        printk("ToBeCheck\r\n");
#if 1
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
            OUTREGMSK16(BASE_REG_MIU_PA + (0x01 << 2), cpu_trans & 0x00FF, 0x00FF);
            OUTREGMSK16(BASE_REG_MIU_PA + (0x21 << 2), cpu_trans & 0x00FF, 0x00FF);
        }
        else
        {
            printk("Wrong parameter, normal transaction(%d) > 255\r\n", cpu_trans);
        }

        if (dla_trans < 0x100)
        {
            OUTREGMSK16(BASE_REG_MIU_PA + (0x02 << 2), dla_trans & 0x00FF, 0x00FF);
            OUTREGMSK16(BASE_REG_MIU_PA + (0x22 << 2), dla_trans & 0x00FF, 0x00FF);
        }
        else
        {
            printk("Wrong parameter, normal transaction(%d) > 255\r\n", dla_trans);
        }

        SETREG16(BASE_REG_MIU_PA + (0x1F << 2), BIT0);
        SETREG16(BASE_REG_MIU_PA + (0x3F << 2), BIT0);
#endif
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
    else if (strcmp(mem_info.name, "MIU_GRBARB_SET_RCMD_WIN") == 0)
    {
        char name[32];
        // unsigned int size;
        unsigned int win_id, win_data, client_id, group, grp_base;
        sscanf(buf, "%s %x %x %x", name, &win_id, &win_data, &client_id);
        group    = (client_id & (0x30)) >> 4;
        grp_base = MIU_BANK[group];
        printk("parameters: %d %x %x\r\n", win_id, win_data, group);
        if (win_id < 16)
        {
            OUTREG16(grp_base + (win_id << 2), win_data & 0xFFFF);
            OUTREG16(grp_base + (0x29 << 2), 0x0100);
            printk("Disable RR\r\n");
            OUTREGMSK16(grp_base + (0x13 << 2), BIT2, BIT2);
        }
        else
            printk("Wrong parameter, window id >= 16\r\n");
    }
    else if (strcmp(mem_info.name, "MIU_GRBARB_SET_WCMD_WIN") == 0)
    {
        char name[32];
        // unsigned int size;
        unsigned int win_id, win_data, client_id, group, grp_base;
        sscanf(buf, "%s %x %x %x", name, &win_id, &win_data, &client_id);
        group    = (client_id & (0x30)) >> 4;
        grp_base = MIU_BANK[group];
        printk("parameters: %d %x %x\r\n", win_id, win_data, group);

        if (win_id < 16)
        {
            OUTREG16(grp_base + ((win_id + 0x30) << 2), win_data & 0xFFFF);
            OUTREG16(grp_base + (0x29 << 2), 0x0100);
            printk("Disable RR\r\n");
            OUTREGMSK16(grp_base + (0x43 << 2), BIT2, BIT2);
        }
        else
            printk("Wrong parameter, window id >= 16\r\n");
    }
    else
    {
        printk("Not support this command\r\n");
    }
}
