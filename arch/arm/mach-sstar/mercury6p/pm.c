/*
* pm.c- Sigmastar
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

#include <linux/suspend.h>
#include <linux/io.h>
#include <asm/suspend.h>
#include <asm/fncpy.h>
#include <asm/cacheflush.h>
#include "ms_platform.h"
#include <linux/delay.h>
#include <asm/secure_cntvoff.h>


#define FIN     printk(KERN_ERR"[%s]+++\n",__FUNCTION__)
#define FOUT    printk(KERN_ERR"[%s]---\n",__FUNCTION__)
#define HERE    printk(KERN_ERR"%s: %d\n",__FILE__,__LINE__)
#define SUSPEND_WAKEUP 0
#define SUSPEND_SLEEP  1
#define SUSPEND_MAGIC  0xBABE
#define STR_PASSWORD   0x5A5A55AA

typedef struct {
    char magic[8];
    unsigned int resume_entry;
    unsigned int count;
    unsigned int status;
    unsigned int password;
} suspend_keep_info;

extern void sram_suspend_imi(void);
static void (*mstar_suspend_imi_fn)(void);
static void __iomem *suspend_imi_vbase;
static suspend_keep_info *pStr_info;
int suspend_status = SUSPEND_WAKEUP;

static void mstar_rtc_magic_set(u16 magic)
{
    u8 timeout = 20;

    OUTREG16(0x1F0068D4, 0x0001);   //enable autogen
    OUTREG16(0x1F006800, 0x0200);   //write sw2 cmd
    OUTREG16(0x1F006810, magic);

retry:
    // Turn-on ISO flow
    OUTREG8(0x1F0068C0, 0x01);
    while((!(INREG8(0x1F0068C4) & 0x01)) && (--timeout)) {
        udelay(100);
    }
    if(timeout == 0)
        goto retry;

    timeout = 20;

    OUTREG8(0x1F0068C8, 0x01);
    while((INREG8(0x1F0068C4) & 0x01) && (--timeout)) {
        udelay(100);
    }
    if(timeout == 0)
        goto retry;

    OUTREG16(0x1F006800, 0x0000);   //reset sw2 cmd
}

static int mstar_suspend_ready(unsigned long ret)
{
    mstar_suspend_imi_fn = fncpy(suspend_imi_vbase, (void*)&sram_suspend_imi, 0x1000);
    suspend_status = SUSPEND_SLEEP;

    mstar_rtc_magic_set(SUSPEND_MAGIC);

    //resume info
    if (pStr_info) {
        pStr_info->count++;
        pStr_info->status = SUSPEND_SLEEP;
        pStr_info->password = STR_PASSWORD;
    }
    //flush cache to ensure memory is updated before self-refresh
    __cpuc_flush_kern_all();
    //flush L3 cache
    Chip_Flush_MIU_Pipe();
    //flush tlb to ensure following translation is all in tlb
    local_flush_tlb_all();
    mstar_suspend_imi_fn();
    return 0;
}

static int mstar_suspend_enter(suspend_state_t state)
{
    //FIN;
    switch (state)
    {
        case PM_SUSPEND_MEM:
            printk(KERN_INFO "state = PM_SUSPEND_MEM\n");
            cpu_suspend(0, mstar_suspend_ready);
            #ifdef CONFIG_SMP
            //secure_cntvoff_init();
            #endif
            break;
        default:
            return -EINVAL;
    }

    return 0;
}

static void mstar_suspend_wake(void)
{
    if (pStr_info) {
        pStr_info->status = SUSPEND_WAKEUP;
        pStr_info->password = 0;
    }
    suspend_status = SUSPEND_WAKEUP;
    mstar_rtc_magic_set(SUSPEND_WAKEUP);
}

struct platform_suspend_ops mstar_suspend_ops = {
    .enter    = mstar_suspend_enter,
    .wake     = mstar_suspend_wake,
    .valid    = suspend_valid_only_mem,
};

int __init mstar_pm_init(void)
{
    unsigned int resume_pbase = virt_to_phys(cpu_resume);
    suspend_imi_vbase = __arm_ioremap_exec(0xA0010000, 0x1000, false);  //put suspend code at IMI offset 64K;

    pStr_info = (suspend_keep_info *)__va(0x20000000);
    memset(pStr_info, 0, sizeof(suspend_keep_info));
    strcpy(pStr_info->magic, "SIG_STR");
    pStr_info->resume_entry = resume_pbase;

    suspend_set_ops(&mstar_suspend_ops);

    printk(KERN_INFO "[%s] resume_pbase=0x%08X, suspend_imi_vbase=0x%08X\n", __func__, (unsigned int)resume_pbase, (unsigned int)suspend_imi_vbase);
    return 0;
}
