/*
 * ms_msys.h- Sigmastar
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
#ifndef _MS_MSYS_H_
#define _MS_MSYS_H_

#include "mdrv_msys_io_st.h"
#include "linux/proc_fs.h"

enum knwon_boot_reason_types
{
    ANDR_BOOT_REASON_COLD,
    ANDR_BOOT_REASON_HARD,
    ANDR_BOOT_REASON_WARM,
    ANDR_BOOT_REASON_WATCHDOG,
    ANDR_BOOT_REASON_REBOOT,
    ANDR_BOOT_REASON_TYPES,
};

extern int                    msys_release_dmem(MSYS_DMEM_INFO *mem_info);
extern int                    msys_request_dmem(MSYS_DMEM_INFO *mem_info);
extern void                   msys_set_rebootType(const char *cmd);
extern struct proc_dir_entry *msys_get_proc_zen(void);
extern struct proc_dir_entry *msys_get_proc_zen_kernel(void);
extern struct proc_dir_entry *msys_get_proc_zen_mi(void);
extern struct proc_dir_entry *msys_get_proc_zen_omx(void);
extern struct class *         msys_get_sysfs_class(void);
extern struct class *         msys_get_sysfs_ut_class(void);

extern void msys_set_miubist(const char *cmd, unsigned long long phy_addr, unsigned int length);
void        msys_print(const char *fmt, ...);
void        msys_prints(const char *string, int length);
void        msys_prints(const char *string, int length);
extern int  msys_read_uuid(unsigned long long *udid);
extern int  msys_dma_blit(MSYS_DMA_BLIT *cfg);
extern int  msys_dma_fill(MSYS_DMA_FILL *cfg);
extern int  msys_dma_copy(MSYS_DMA_COPY *cfg);
#if defined(CONFIG_MS_BDMA_LINE_OFFSET_ON)
extern int msys_dma_fill_lineoffset(MSYS_DMA_FILL_BILT *pstDmaCfg);
extern int msys_dma_copy_lineoffset(MSYS_DMA_BLIT *cfg);
#endif
extern int ssys_get_HZ(void);

#ifdef CONFIG_SSTAR_IRQ_DEBUG_TRACE
extern int sirq_head_initialized;
void       sstar_irq_disable_debug(void);
void       sstar_irq_enable_debug(void);
void       sstar_irq_debug_ignore(void);
void       sstar_irq_records_count(int irq_num);
void       sstar_irq_dump_count(void);
void       sstar_irq_dump_latency_info(void);
void       sstar_irq_record_large_latency_in_top(SSTAR_IRQ_INFO *irq_info);
void       sstar_irq_records(SSTAR_IRQ_INFO *irq_info);
void       sstar_irq_dump_info(void);
void       sstar_sirq_records(SSTAR_IRQ_INFO *irq_info);
void       sstar_sirq_dump_info(void);
void       sstar_irqoff_records(SSTAR_IRQ_INFO *irq_info);
void       sstar_irqoff_dump_info(struct seq_file *s);
#endif // CONFIG_SSTAR_IRQ_DEBUG_TRACE

#endif
