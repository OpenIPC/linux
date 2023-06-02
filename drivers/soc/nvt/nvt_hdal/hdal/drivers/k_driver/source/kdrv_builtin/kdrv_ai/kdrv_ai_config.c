/*
    KDRV AI configuration file.

    KDRV AI configuration file. Define semaphore ID, flag ID, etc.

    @file       KDRV_AI_config.c
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/



//#include <linux/module.h>
//#include <linux/export.h>
#if defined(__FREERTOS)
#include <rtosfdt.h>
#include <stdio.h>
#include <libfdt.h>
#include <compiler.h>
#endif

#include "kwrap/flag.h"

#include "ai_lib.h"
#include "kdrv_ai.h"
#include "kdrv_ai_dbg.h"
#include "ai_drv.h"

#if defined(__FREERTOS)
ID     	  FLG_ID_KDRV_AI[AI_ENG_TOTAL] = {0};
#else
FLGPTN     FLG_ID_KDRV_AI[AI_ENG_TOTAL] = {0};
#endif

SEM_HANDLE SEMID_KDRV_AI[AI_ENG_TOTAL] = {0};
#if KDRV_AI_FLG_IMPROVE
SEM_HANDLE ai_sem_id[AI_ENG_TOTAL][KDRV_AI_HANDLE_MAX_NUM] ={0};
SEM_HANDLE ai_sem_param_id[AI_ENG_TOTAL] ={0};
SEM_HANDLE ai_sem_ch_id[KDRV_AI_HANDLE_MAX_NUM] ={0};
SEM_HANDLE ai_handle_sem_id[AI_ENG_TOTAL] ={0};
SEM_HANDLE ai_ioctl_sem_id = 0;

#if defined(__FREERTOS)
static ID     	  flg_ai_sem_id[AI_ENG_TOTAL][KDRV_AI_HANDLE_MAX_NUM] = {0};
static ID     	  flg_ai_sem_param_id[AI_ENG_TOTAL] = {0};
static ID     	  flg_ai_sem_ch_id[KDRV_AI_HANDLE_MAX_NUM] = {0};
static ID     	  flg_ai_handle_sem_id[AI_ENG_TOTAL] = {0};
static ID     	  flg_ai_ioctl_sem_id = 0;
#else
static FLGPTN     	  flg_ai_sem_id[AI_ENG_TOTAL][KDRV_AI_HANDLE_MAX_NUM] = {0};
static FLGPTN     	  flg_ai_sem_param_id[AI_ENG_TOTAL] = {0};
static FLGPTN     	  flg_ai_sem_ch_id[KDRV_AI_HANDLE_MAX_NUM] = {0};
static FLGPTN     	  flg_ai_handle_sem_id[AI_ENG_TOTAL] = {0};
static FLGPTN     	  flg_ai_ioctl_sem_id = 0;
#endif

#endif

#if defined(__FREERTOS)
unsigned int ai_clock_freq[MODULE_CLK_NUM];
#if defined(CONFIG_NVT_SMALL_HDAL)
unsigned int ai_net_supported_num=1;
#else
unsigned int ai_net_supported_num=AI_SUPPORT_NET_MAX;
#endif
#else
extern unsigned int ai_net_supported_num;
#endif

#if defined(__FREERTOS)
UINT32 kdrv_ai_drv_get_clock_freq(UINT8 clk_idx)
{
	unsigned char *fdt_addr = (unsigned char *)fdt_get_base();
	char path[64] = {0};
	INT nodeoffset;
	UINT32 *cell = NULL;
	UINT32 idx;

	for(idx = 0; idx < MODULE_CLK_NUM; idx++) {
		ai_clock_freq[idx] = 0;
	}
	sprintf(path,"/ai@f0c60000");
	nodeoffset = fdt_path_offset((const void*)fdt_addr, path);

	cell = (UINT32 *)fdt_getprop((const void*)fdt_addr, nodeoffset, "clock-frequency", NULL);
	if (cell) {
		for (idx = 0; idx < MODULE_CLK_NUM; idx++) {
			ai_clock_freq[idx] = be32_to_cpu(cell[idx]);
		}
	}

	//DBG_ERR("AI_RTOS: ai_clock(%d %d %d %d)\r\n", ai_clock_freq[0], ai_clock_freq[1], ai_clock_freq[2], ai_clock_freq[3]);	

	return ai_clock_freq[clk_idx];
}
#endif

UINT32 kdrv_ai_drv_get_net_supported_num(VOID)
{
	UINT32 ai_max_net_num_now;

	ai_max_net_num_now = (UINT32) ai_net_supported_num;

	if (ai_max_net_num_now > AI_SUPPORT_NET_MAX) {
        DBG_ERR("Error, to get net supported num = %d > %d, force to %d\n",
                    ai_max_net_num_now, AI_SUPPORT_NET_MAX, AI_SUPPORT_NET_MAX);
        ai_max_net_num_now = AI_SUPPORT_NET_MAX;
    }

	return (UINT32) ai_max_net_num_now;
}

void kdrv_ai_install_id(void)
{
	UINT32 eng = 0;
#if KDRV_AI_FLG_IMPROVE
	UINT32 handle = 0;
#endif
	for (eng = 0; eng < (UINT32)AI_ENG_TOTAL; eng++) {
		OS_CONFIG_FLAG(FLG_ID_KDRV_AI[eng]);
		SEM_CREATE(SEMID_KDRV_AI[eng], 1);
	}
#if KDRV_AI_FLG_IMPROVE
	for (eng = 0; eng < (UINT32)AI_ENG_TOTAL; eng++) {
		OS_CONFIG_FLAG(flg_ai_sem_param_id[eng]);
		SEM_CREATE(ai_sem_param_id[eng], 1);
		OS_CONFIG_FLAG(flg_ai_handle_sem_id[eng]);
		SEM_CREATE(ai_handle_sem_id[eng], 1);
		for (handle = 0; handle < KDRV_AI_HANDLE_MAX_NUM; handle++) {
			OS_CONFIG_FLAG(flg_ai_sem_id[eng][handle]);
			SEM_CREATE(ai_sem_id[eng][handle], 1);
		}
	}
	for (handle = 0; handle < KDRV_AI_HANDLE_MAX_NUM; handle++) {
		OS_CONFIG_FLAG(flg_ai_sem_ch_id[handle]);
		SEM_CREATE(ai_sem_ch_id[handle], 1);
	}
	OS_CONFIG_FLAG(flg_ai_ioctl_sem_id);
	SEM_CREATE(ai_ioctl_sem_id, 1);
#endif
}
void kdrv_ai_init(void)
{
#if (KDRV_AI_FLG_IMPROVE == 0)
	UINT32 eng = 0;
	for (eng = 0; eng < (UINT32)AI_ENG_TOTAL; eng++) {
		set_flg(FLG_ID_KDRV_AI[eng], KDRV_AI_INIT_BITS);
	}
#endif
}
void kdrv_ai_uninstall_id(void)
{
	UINT32 eng = 0;
	UINT32 handle = 0;
	for (eng = 0; eng < (UINT32)AI_ENG_TOTAL; eng++) {
		rel_flg(FLG_ID_KDRV_AI[eng]);
		SEM_DESTROY(SEMID_KDRV_AI[eng]);
		rel_flg(flg_ai_sem_param_id[eng]);
		SEM_DESTROY(ai_sem_param_id[eng]);
		rel_flg(flg_ai_handle_sem_id[eng]);
		SEM_DESTROY(ai_handle_sem_id[eng]);
		for (handle = 0; handle < KDRV_AI_HANDLE_MAX_NUM; handle++) {
			rel_flg(flg_ai_sem_id[eng][handle]);
			SEM_DESTROY(ai_sem_id[eng][handle]);
		}
	}
	for (handle = 0; handle < KDRV_AI_HANDLE_MAX_NUM; handle++) {
		rel_flg(flg_ai_sem_ch_id[handle]);
		SEM_DESTROY(ai_sem_ch_id[handle]);
	}
	rel_flg(flg_ai_ioctl_sem_id);
	SEM_DESTROY(ai_ioctl_sem_id);
}
#ifdef __KERNEL__

EXPORT_SYMBOL(kdrv_ai_install_id);
EXPORT_SYMBOL(kdrv_ai_uninstall_id);
EXPORT_SYMBOL(kdrv_ai_init);
EXPORT_SYMBOL(kdrv_ai_drv_get_net_supported_num);

#endif
