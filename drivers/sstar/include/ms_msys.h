/*
* ms_msys.h- Sigmastar
*
* Copyright (C) 2018 Sigmastar Technology Corp.
*
* Author: karl.xiao <karl.xiao@sigmastar.com.tw>
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/
#ifndef _MS_MSYS_H_
#define _MS_MSYS_H_

#include "mdrv_msys_io_st.h"
#include "linux/proc_fs.h"

extern int msys_release_dmem(MSYS_DMEM_INFO *mem_info);
extern int msys_request_dmem(MSYS_DMEM_INFO *mem_info);
extern struct proc_dir_entry* msys_get_proc_zen(void);
extern struct proc_dir_entry* msys_get_proc_zen_kernel(void);
extern struct proc_dir_entry* msys_get_proc_zen_mi(void);
extern struct proc_dir_entry* msys_get_proc_zen_omx(void);
extern struct class *msys_get_sysfs_class(void);
void msys_prints(const char *string, int length);
extern int msys_read_uuid(unsigned long long* udid);
extern int msys_dma_blit(MSYS_DMA_BLIT *cfg);
extern int msys_dma_fill(MSYS_DMA_FILL *cfg);
extern int msys_dma_copy(MSYS_DMA_COPY *cfg);
extern int ssys_get_HZ(void);

#endif
