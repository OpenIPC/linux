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
    return 1;
}

int msys_miu_set_bist(U8 enable)
{
    // 1012 0x69 disable miu protect
    CLRREG16(BASE_REG_MIU_PA + REG_ID_69, 0x0FFF);
    OUTREGMSK16(BASE_REG_MIU_PA + REG_ID_69, 0x3FFF, 0xF000);
    // 1011 0x71 = 0x001e, enable high way,normal way cutted length
    OUTREG16(BASE_REG_MIU_ARB + REG_ID_71, 0x001E);
    // 1011 0x75 = 0x4040, set normal way cutted length to 512
    OUTREG16(BASE_REG_MIU_ARB + REG_ID_75, 0x8080);
    // 1011 0x76 = 0x0808, set high way cutted length to 256
    OUTREG16(BASE_REG_MIU_ARB + REG_ID_76, 0x4040);
    // 1011 0x77 = 0x0808, set high way cutted length to 32
    OUTREG16(BASE_REG_MIU_ARB + REG_ID_77, 0x0808);
    // 1011 0x7c = 0xffc0, set normal way high priority than high way
    OUTREG16(BASE_REG_MIU_ARB + REG_ID_7C, 0xFFC0);
    // 1012 0x5F = 0x4040, enable second bist mode,it is coupled with the setting of 1012 0x72
    OUTREG16(BASE_REG_MIU_PA + REG_ID_5F, 0x4040);
    // 1012 0x72 = 0x0100, burst length, adjust this affect the BW occupied
    OUTREG16(BASE_REG_MIU_PA + REG_ID_72, 0x0100);
    // 1012 0x71,bist addr 0x7e00
    OUTREG16(BASE_REG_MIU_PA + REG_ID_71, 0x7E00);
    // 1012 0x73 bist dram size 1M
    OUTREG16(BASE_REG_MIU_PA + REG_ID_73, 0x0080);
    // 1012 0x70, enable test mode
    OUTREG16(BASE_REG_MIU_PA + REG_ID_70, 0x0011);
    // 1012 0x20, disable round robin
    CLRREG16(BASE_REG_MIU_PA + REG_ID_20, 0x0001);

    OUTREGMSK16(BASE_REG_MIU_PA + REG_ID_20, 0x0002, 0x0002);
    // 1012 0x28, set group0 bist priority 0
    OUTREG16(BASE_REG_MIU_PA + REG_ID_25, 0x301F);
    OUTREG16(BASE_REG_MIU_PA + REG_ID_28, 0x2EDC);
    CLRREG16(BASE_REG_MIU_PA + REG_ID_24, 0xFFFB);

    if (enable)
    {
        msys_miu_client_switch(MSYS_MIU_CLIENT_SW_BIST);
    }
    else
    {
        msys_miu_client_switch(MSYS_MIU_CLIENT_SW_TVTOOL);
    }

    return 0;
}

int msys_miu_client_switch(msys_miu_client_sw_e sw)
{
    switch (sw)
    {
        case MSYS_MIU_CLIENT_SW_BIST:
            CLRREG16(BASE_REG_MIU_PA + REG_ID_7F, BIT11);
            break;
        case MSYS_MIU_CLIENT_SW_TVTOOL:
            SETREG16(BASE_REG_MIU_PA + REG_ID_7F, BIT11);
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

void msys_miu_ioctl(MSYS_DMEM_INFO mem_info, const char *buf)
{
    if (strcmp(mem_info.name, "MIU_TEST") == 0)
    {
        CLRREG16(BASE_REG_MIU_PA + REG_ID_69, 0xFF);
        OUTREG16(BASE_REG_MIU_PA + REG_ID_71, (mem_info.phys - 0x20000000) >> 13); // test base address[15:0]
        CLRREG16(BASE_REG_MIU_PA + REG_ID_6F, BIT2 | BIT3);                        // test base address[17:16]
        OUTREG16(BASE_REG_MIU_PA + REG_ID_72, (mem_info.length >> 4) & 0xFFFF);    // test length[15:0]
        OUTREG16(BASE_REG_MIU_PA + REG_ID_73, (mem_info.length >> 20) & 0x0FFF);   // test length[27:16]
        msys_miu_client_switch(MSYS_MIU_CLIENT_SW_BIST);
    }
}
