/*
 * dsp_ut_datatype.h- Sigmastar
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

#ifndef _dsp_ut_datatype_h_
#define _dsp_ut_datatype_h_
#include <linux/ioctl.h>

typedef enum
{
    E_DSP_UT_ADDRESS_SET0 = 0,
    E_DSP_UT_ADDRESS_SET1,
    E_DSP_UT_ADDRESS_SET2,
    E_DSP_UT_ADDRESS_SET_NUM
} dsp_ut_addrmap_set;

typedef struct
{
    dsp_ut_addrmap_set eSet;
    unsigned int       u32DspPhy;
    unsigned int       u32Len;
    unsigned long      cpuPhy;
} dsp_ut_addrmap_t;

typedef union
{
    int              core;
    dsp_ut_addrmap_t addrMap[E_DSP_UT_ADDRESS_SET_NUM];
    int              jtag;
    int              resetVector;
    int              prid;
    int              dfs;
    char             fwPath[256];
} dsp_ut_data;

#define DSP_UT_DEV_0 "dsp_ut_ioctl0"
#define DSP_UT_DEV_1 "dsp_ut_ioctl1"
#define DSP_UT_DEV_2 "dsp_ut_ioctl2"
#define DSP_UT_DEV_3 "dsp_ut_ioctl3"

#define DSP_UT_IOC_MAGIC 'D'

#define DSP_UT_SET_CORE     _IOW(DSP_UT_IOC_MAGIC, DSP_UT_IOC_SET_CORE, int)
#define DSP_UT_SET_FIRMWARE _IOW(DSP_UT_IOC_MAGIC, DSP_UT_IOC_SET_FIRMWARE, dsp_ut_data)
#define DSP_UT_SET_ADDRESS_MAP \
    _IOW(DSP_UT_IOC_MAGIC, DSP_UT_IOC_SET_ADDRESS_MAP, dsp_ut_addrmap_t[E_DSP_UT_ADDRESS_SET_NUM])
#define DSP_UT_SET_PRID           _IOW(DSP_UT_IOC_MAGIC, DSP_UT_IOC_SET_PRID, int)
#define DSP_UT_SET_RESET_VECTOR   _IOW(DSP_UT_IOC_MAGIC, DSP_UT_IOC_SET_RESET_VECTOR, int)
#define DSP_UT_SET_DFS            _IOW(DSP_UT_IOC_MAGIC, DSP_UT_IOC_SET_DFS, int)
#define DSP_UT_SET_JTAG           _IOW(DSP_UT_IOC_MAGIC, DSP_UT_IOC_SET_JTAG, int)
#define DSP_UT_TEST_BOOT_FIRMWARE _IO(DSP_UT_IOC_MAGIC, DSP_UT_IOC_TEST_BOOT_FIRMWARE)
#define DSP_UT_TEST_GPI           _IO(DSP_UT_IOC_MAGIC, DSP_UT_IOC_TEST_GPI)
#define DSP_UT_TEST_GPO           _IO(DSP_UT_IOC_MAGIC, DSP_UT_IOC_TEST_GPO)
#define DSP_UT_TEST_APB           _IO(DSP_UT_IOC_MAGIC, DSP_UT_IOC_TEST_APB)
#define DSP_UT_TEST_IRQ           _IO(DSP_UT_IOC_MAGIC, DSP_UT_IOC_TEST_IRQ)

#endif //_dsp_ut_datatype_h_
