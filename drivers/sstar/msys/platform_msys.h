/*
 * platform_msys.h- Sigmastar
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
#ifndef _PLATFORM_MSYS_H_
#define _PLATFORM_MSYS_H_

typedef enum
{
    MSYS_MIU_CLIENT_SW_BIST,
    MSYS_MIU_CLIENT_SW_TVTOOL,
} msys_miu_client_sw_e;

int msys_miu_check_cmd(const char *buf);
int msys_miu_set_bist(U8 enable);
int msys_miu_client_switch(msys_miu_client_sw_e sw);

int  msys_request_freq(MSYS_FREQGEN_INFO *freq_info);
void msys_miu_ioctl(MSYS_DMEM_INFO mem_info, const char *buf);

#endif
