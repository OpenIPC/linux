/*
 * prof.c- Sigmastar
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
#include <linux/kernel.h>
#include <linux/types.h>
#include "ms_platform.h"
#include "ms_types.h"
#include <linux/io.h>
#include "registers.h"
#include <asm/arch_timer.h>
#include "cam_os_wrapper.h"
#ifdef CONFIG_SS_DUALOS
#include "cam_inter_os.h"
#endif

#ifdef CONFIG_SS_DUALOS
#define INTEROS_SC_L2R_BOOTING_TIME_GET 0xF1030000
#define INTEROS_SC_R2L_BOOTING_TIME_GET 0xF1030001
#endif

#ifdef CONFIG_PM
#define RECORD_STR_ADDR (0xF9028400) // vad_str.h
#endif

#define RECORD_IPL_ADDR (IMI_VIRT | 0xF000UL) // IPL
#define RECORD_BL_ADDR  (IMI_VIRT | 0xF400UL) // BL
#define MAX_RECORD      25                    /*max:(0xF800-0xF400) or (0xF400-0xF000)/40~=25*/
#define MAX_LANGTH      32
#define MAX_RECORD_SIZE (MAX_RECORD * sizeof(struct timestamp))

struct timestamp
{
    unsigned int  timestamp_us;     /* 4                 */
    unsigned int  mark;             /* 4, ex:__line__    */
    unsigned char name[MAX_LANGTH]; /*32, ex:__function__*/
};

struct timecrecord
{
    U32              count;
    struct timestamp tt[MAX_RECORD];
};

int                g_record_inited = 0;
struct timecrecord tp_record_ipl   = {0};
struct timecrecord tp_record_bl    = {0};
struct timecrecord tp_record_linux = {0};

void recode_timestamp_init(void)
{
    g_record_inited = 0;
}

unsigned int read_timestamp(void)
{
    u64 cycles  = __arch_counter_get_cntpct();
    u64 usticks = div64_u64(cycles, 6 /*us_ticks_factor*/);
    return (unsigned int)usticks;
}

void recode_timestamp(int mark, const char *name)
{
    struct timecrecord *tc;

    tc = &tp_record_linux;
    if (!g_record_inited)
    {
        tc->count       = 0;
        g_record_inited = 1;
    }

    if (tc->count < MAX_RECORD)
    {
        tc->tt[tc->count].timestamp_us = read_timestamp();
        tc->tt[tc->count].mark         = mark;
        strncpy(tc->tt[tc->count].name, name, MAX_LANGTH - 1);
        tc->count++;
    }
}
EXPORT_SYMBOL(recode_timestamp);

static int _BootTimestampPrintInfo(struct timecrecord *ptTimeRec, char *buf, int bufLen)
{
    int i      = 0;
    int strLen = 0;

    if (ptTimeRec && (ptTimeRec->count <= MAX_RECORD) && (ptTimeRec->count > 0))
    {
        for (i = 0; i < ptTimeRec->count; i++)
        {
            ptTimeRec->tt[i].name[MAX_LANGTH - 1] = '\0';

            if (buf && bufLen)
            {
                strLen += scnprintf(buf + strLen, bufLen - strLen, "%03d time:%8u, diff:%8u, %s, %d\n", i,
                                    ptTimeRec->tt[i].timestamp_us,
                                    (ptTimeRec->tt[i].timestamp_us > ptTimeRec->tt[i ? i - 1 : i].timestamp_us)
                                        ? (ptTimeRec->tt[i].timestamp_us - ptTimeRec->tt[i ? i - 1 : i].timestamp_us)
                                        : 0,
                                    ptTimeRec->tt[i].name, ptTimeRec->tt[i].mark);
            }
        }
        if (buf)
        {
            strLen += scnprintf(buf + strLen, bufLen - strLen, "Total cost:%8u(us)\n",
                                ptTimeRec->tt[ptTimeRec->count - 1].timestamp_us - ptTimeRec->tt[0].timestamp_us);
        }
    }

    return strLen;
}

int recode_show(char *buf)
{
    struct timecrecord *tc;
    int                 strLen = 0;
#ifdef CONFIG_SS_DUALOS
#define LOG_FROM_RTOS_LEN 2048
    void *va = NULL;

    va = kzalloc(LOG_FROM_RTOS_LEN, GFP_KERNEL);
    CamOsMemInvalidate(va, LOG_FROM_RTOS_LEN);
    CamInterOsSignal(INTEROS_SC_L2R_BOOTING_TIME_GET, CamOsMemVirtToPhys(va), LOG_FROM_RTOS_LEN, 0);
    CamOsMemInvalidate(va, LOG_FROM_RTOS_LEN);
    strLen += scnprintf(buf + strLen, PAGE_SIZE - strLen, "%s", (char *)va);
    kfree(va);
#else

    tc = &tp_record_ipl;

    if (tc->count <= MAX_RECORD && tc->count > 0)
    {
        strLen += scnprintf(buf + strLen, PAGE_SIZE - strLen, "IPL (%d records)\n", tc->count);
        strLen += _BootTimestampPrintInfo(tc, buf + strLen, PAGE_SIZE - strLen);
    }

    tc = &tp_record_bl;
    if (tc->count <= MAX_RECORD && tc->count > 0)
    {
        strLen += scnprintf(buf + strLen, PAGE_SIZE - strLen, "BL (%d records)\n", tc->count);
        strLen += _BootTimestampPrintInfo(tc, buf + strLen, PAGE_SIZE - strLen);
    }
#endif

#ifdef CONFIG_PM
    tc = (struct timecrecord *)RECORD_STR_ADDR;

    if (tc->count <= MAX_RECORD && tc->count > 0)
    {
        strLen += scnprintf(buf + strLen, PAGE_SIZE - strLen, "STR (%d records)\n", tc->count);
        strLen += _BootTimestampPrintInfo(tc, buf + strLen, PAGE_SIZE - strLen);
    }
#endif

    tc = &tp_record_linux;
    if (tc->count <= MAX_RECORD && tc->count > 0)
    {
        strLen += scnprintf(buf + strLen, PAGE_SIZE - strLen, "LINUX (%d records)\n", tc->count);
        strLen += _BootTimestampPrintInfo(tc, buf + strLen, PAGE_SIZE - strLen);
    }

    return strLen;
}

#ifdef CONFIG_SS_DUALOS
static u32 boot_timestamp_interos_get_log(u32 arg0, u32 bufPa, u32 bufLen, u32 arg3)
{
    char *              bufVa  = NULL;
    u32                 strLen = 0;
    struct timecrecord *tc     = NULL;

    bufVa = (char *)CamOsMemMap(bufPa, bufLen, TRUE);

    tc = &tp_record_linux;
    if (tc->count <= MAX_RECORD && tc->count > 0)
    {
        strLen += scnprintf(bufVa + strLen, bufLen - strLen, "LINUX (%d records)\n", tc->count);
        strLen += _BootTimestampPrintInfo(tc, bufVa + strLen, bufLen - strLen);
    }

    CamOsMemFlush(bufVa, strLen);
    CamOsMemUnmap(bufVa, bufLen);

    return strLen;
}
#endif

static int __init boot_timestamp_init(void)
{
#ifdef CONFIG_SS_DUALOS
    CamInterOsSignalReg(INTEROS_SC_R2L_BOOTING_TIME_GET, boot_timestamp_interos_get_log, "booting_time");
#else
    struct timecrecord *volatile tc;
    U32 size;

    /*
     * Copy IPL and uboot stage's timestamp data from SRAM section to DRAM before image pipe initial,
     * because some hardware IP use all SRAM section.
     */
    tc   = (struct timecrecord *)RECORD_IPL_ADDR;
    size = tc->count * sizeof(struct timestamp);
    if (size > MAX_RECORD_SIZE)
        size = MAX_RECORD_SIZE;
    size += sizeof(tc->count);
    memcpy(&tp_record_ipl, tc, size);
    memset_io(tc, 0, MAX_RECORD_SIZE);

    tc   = (struct timecrecord *)RECORD_BL_ADDR;
    size = tc->count * sizeof(struct timestamp);
    if (size > MAX_RECORD_SIZE)
        size = MAX_RECORD_SIZE;
    size += sizeof(tc->count);
    memcpy(&tp_record_bl, tc, size);
    memset_io(tc, 0, MAX_RECORD_SIZE);

#ifdef CONFIG_PM
    memset_io((struct timecrecord *)RECORD_STR_ADDR, 0, 0x1000);
#endif

#endif

    return 0;
}
late_initcall(boot_timestamp_init);
