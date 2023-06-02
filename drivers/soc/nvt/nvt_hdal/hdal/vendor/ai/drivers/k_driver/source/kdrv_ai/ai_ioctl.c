#include "ai_lib.h"
#include "ai_ioctl.h"
#include <kwrap/verinfo.h> 
#include "kdrv_ai_int.h"
#include "interrupt.h"

#include <string.h>
#include "kwrap/semaphore.h"
#include "kwrap/debug.h"
#include "kwrap/flag.h"

#if defined(__FREERTOS)
#include "hdal.h"
#include "hd_debug.h"
#include <malloc.h>
#include "kdrv_ai.h"


#if LL_SUPPORT_ROI


#if LL_BUF_FROM_USR
void *g_buf_handle[AI_SUPPORT_NET_MAX] = {NULL};
//struct nvt_fmem_mem_info_t g_buf_info = {0};
UINT32 g_buf_info_pa[AI_SUPPORT_NET_MAX] ={0};
UINT32 g_buf_info_va[AI_SUPPORT_NET_MAX] ={0};
UINT32 net_ll_size[AI_SUPPORT_NET_MAX] = {0};
UINT32 net_ll_va[AI_SUPPORT_NET_MAX] = {0};
UINT32 net_ll_pa[AI_SUPPORT_NET_MAX] = {0};
#else
void *g_buf_handle = NULL;
//struct nvt_fmem_mem_info_t g_buf_info = {0};
UINT32 g_buf_info_pa =0;
UINT32 g_buf_info_va =0;
#define MAX_LL_BUF_SIZE 0x2000000
#define AI_SINGLE_NET_LL_SIZE (MAX_LL_BUF_SIZE/AI_SUPPORT_NET_MAX)
#endif
#define ROI_PARAM_GLOBAL 1
#if ROI_PARAM_GLOBAL
AI_DRV_LL_USR_INFO  ai_ll_usr_info = {0};
AI_DRV_LL_BUF_INFO  ai_ll_buf_info = {0};
#endif

#endif

AI_DRV_MAP_MEM g_ai_drv_map_mem[AI_SUPPORT_NET_MAX] = {0};


UINT8 * kdrv_ai_vmalloc(UINT32 v_size)
{
    char *v_buff;
    v_buff = (char *) malloc(v_size);
    return (UINT8 *) v_buff;
}

VOID kdrv_ai_vfree(UINT8 *v_buff)
{
    if (v_buff) {
        free((char *)v_buff);
    }
    return;
}

#if LL_SUPPORT_ROI
UINT32 kdrv_ai_get_drv_base_va(UINT32 net_id)
{
#if LL_BUF_FROM_USR
	if (net_ll_va[net_id] == 0) {
		nvt_dbg(ERR, "ll cmd buffer is NULL!\n");
		return 0;
	}
	return net_ll_va[net_id];
#else
	if (g_buf_info.vaddr == 0) {
		nvt_dbg(ERR, "ll cmd buffer is NULL!\n");
		return 0;
	}
	return (UINT32)(g_buf_info.vaddr + net_id*AI_SINGLE_NET_LL_SIZE);
#endif
}

UINT32 kdrv_ai_get_drv_base_pa(UINT32 net_id)
{
#if LL_BUF_FROM_USR
	if (net_ll_pa[net_id] == 0) {
		nvt_dbg(ERR, "ll cmd buffer is NULL!\n");
		return 0;
	}
	return net_ll_pa[net_id];
#else
	if (g_buf_info.paddr == 0) {
		nvt_dbg(ERR, "ll cmd buffer is NULL!\n");
		return 0;
	}
	return (UINT32)(g_buf_info.paddr + net_id*AI_SINGLE_NET_LL_SIZE);
#endif
}

UINT32 kdrv_ai_get_drv_io_buf_size(UINT32 net_id)
{
#if LL_BUF_FROM_USR
	return net_ll_size[net_id];
#else
	return AI_SINGLE_NET_LL_SIZE;
#endif
}
#endif


static UINT32 kdrv_ai_trans_eng2id(KDRV_AI_ENG ai_eng)
{
	UINT32 eng_id = KDRV_AI_ENGINE_CNN;
	if (ai_eng == AI_ENG_CNN) {
		eng_id = KDRV_AI_ENGINE_CNN;
	} else if (ai_eng == AI_ENG_NUE) {
		eng_id = KDRV_AI_ENGINE_NUE;
	} else if (ai_eng == AI_ENG_NUE2) {
		eng_id = KDRV_AI_ENGINE_NUE2;
	} else if (ai_eng == AI_ENG_CNN2) {
		eng_id = KDRV_AI_ENGINE_CNN2;
	} else {
		nvt_dbg(ERR, "not support engine: %d\r\n", ai_eng);
	}
	return eng_id;
}

static UINT32 nvt_ai_chk_net_id(UINT32 net_id) {
	const UINT32 net_id_upb = AI_SUPPORT_NET_MAX - 1;
	if (net_id > net_id_upb) {
		nvt_dbg(WRN, "network id is overflow, clamp -> %d\r\n", (int)net_id_upb);
		net_id = net_id_upb;
	}
	return net_id;
}

static BOOL nvt_ai_drv_is_mem_map(UINT32 net_id)
{
	AI_DRV_MAP_MEM *p_mem;
	net_id = nvt_ai_chk_net_id(net_id);
	p_mem = &g_ai_drv_map_mem[net_id];

	if ((p_mem->kerl_pa == 0) || (p_mem->kerl_va == 0) || (p_mem->user_pa == 0) || (p_mem->user_va == 0)) {
		nvt_dbg(ERR, "map mem user va=%08x, user pa=%08x, kernel va=%08x, kernel pa=%08x\r\n"
				, (int)p_mem->user_va, (int)p_mem->user_pa, (int)p_mem->kerl_va, (int)p_mem->kerl_pa);
		return FALSE;
	} else {
		return TRUE;
	}
}

static unsigned int nvt_ai_drv_user2kerl_va(unsigned int addr, UINT32 net_id)
{
	AI_DRV_MAP_MEM *p_mem;
	net_id = nvt_ai_chk_net_id(net_id);
	p_mem = &g_ai_drv_map_mem[net_id];

	if (addr < p_mem->user_va) {
		nvt_dbg(ERR, "user va %08x should > %08x\r\n", (int)addr, (int)p_mem->user_va);
		return addr;
	}
	return addr - p_mem->user_va + p_mem->kerl_va;
}

static unsigned int nvt_ai_drv_kerl2user_va(unsigned int addr, UINT32 net_id)
{
	AI_DRV_MAP_MEM *p_mem;
	net_id = nvt_ai_chk_net_id(net_id);
	p_mem = &g_ai_drv_map_mem[net_id];

	if (addr < p_mem->kerl_va) {
		nvt_dbg(ERR, "kernel va %08x should > %08x\r\n", (int)addr, (int)p_mem->kerl_va);
		return addr;
	}
	return addr - p_mem->kerl_va + p_mem->user_va;
}

void nvt_ai_drv_init_rtos(void)
{
	//int iRet;
	UINT32 cnn_clk_freq;
#if defined(_BSP_NA51089_)
#else
	UINT32 cnn2_clk_freq;
#endif
	UINT32 nue2_clk_freq;	
	UINT32 nue_clk_freq;

#if defined(_BSP_NA51089_)
	cnn_clk_freq = kdrv_ai_drv_get_clock_freq(2);
#else
	cnn_clk_freq = kdrv_ai_drv_get_clock_freq(3);
	cnn2_clk_freq = kdrv_ai_drv_get_clock_freq(2);
#endif
	nue2_clk_freq = kdrv_ai_drv_get_clock_freq(1);
	nue_clk_freq = kdrv_ai_drv_get_clock_freq(0);

	nue_set_base_addr((UINT32)(0xF0C60000)); 
	nue2_set_base_addr((UINT32)(0xF0D50000));
	cnn_set_base_addr(0, (UINT32)(0xF0CB0000));
#if defined(_BSP_NA51089_)
#else
	cnn_set_base_addr(1, (UINT32)(0xf0d60000));
#endif

	nue_create_resource(NULL, nue_clk_freq);
	nue2_create_resource(NULL, nue2_clk_freq);
	cnn_create_resource(0, cnn_clk_freq);
#if defined(_BSP_NA51089_)
#else
	cnn_create_resource(1, cnn2_clk_freq);
#endif
	kdrv_ai_install_id();
	kdrv_ai_init();
	/*iRet = kdrv_ai_do_vmalloc_for_netflowsample();
	if (iRet < 0) {
		nvt_dbg(ERR, "Error, to do vmalloc for global array.\r\n");
	}*/
}

int nvt_ai_ioctl(int fd, unsigned int uiCmd, void *p_arg)
{
	ER er_return;
	int iRet = 0;
	//REG_INFO reg_info;
	//REG_INFO_LIST reg_info_list;
	//int loop_count;
	//int err = 0;
	AI_DRV_OPENCFG      ai_cfg_data = {0};
	AI_DRV_MODEINFO     ai_mode;
	AI_DRV_APPINFO      ai_app_info = {0};
	AI_DRV_LLINFO       ai_ll_info = {0};
	AI_DRV_TRIGINFO   	ai_triginfo = {0};
	AI_DRV_MAP_MEMINFO  ai_map_mem_info = {0};
	KDRV_AI_ENG ai_eng = 0;
	const UINT32 chip = 0, channel = 0;
	UINT32 id = 0;
	UINT32 net_id;
//#if LL_SUPPORT_ROI
//#if (ROI_PARAM_GLOBAL == 0)
//	AI_DRV_LL_USR_INFO  ai_ll_usr_info = {0};
//#endif
//#endif


	nvt_dbg(IND, "cmd:%x\n", uiCmd);

	switch (uiCmd) {
	/* Add other operations here */
	// engine init
	case AI_IOC_ENG_INIT:
		iRet = nue_init();
		if (iRet != 0) {
            nvt_dbg(ERR, "AI_IOCTL: Error, nue_init failed.\r\n");
            goto AI_IOC_ENG_INIT_EXIT;
		}
		iRet = cnn_init(0);
		if (iRet != 0) {
            nvt_dbg(ERR, "AI_IOCTL: Error, cnn_init failed.\r\n");
            goto AI_IOC_ENG_INIT_EXIT;
		}
		iRet = nue2_init();
		if (iRet != 0) {
            nvt_dbg(ERR, "AI_IOCTL: Error, nue2_init failed.\r\n");
            goto AI_IOC_ENG_INIT_EXIT;
		}

AI_IOC_ENG_INIT_EXIT:

		break;
	
	case AI_IOC_ENG_UNINIT:
		iRet = nue_uninit();
		if (iRet != 0) {
            nvt_dbg(ERR, "AI_IOCTL: Error, nue_uninit failed.\r\n");
            goto AI_IOC_ENG_UNINIT_EXIT;
        }
		iRet = cnn_uninit(0);
		if (iRet != 0) {
            nvt_dbg(ERR, "AI_IOCTL: Error, cnn_uninit failed.\r\n");
            goto AI_IOC_ENG_UNINIT_EXIT;
        }
		iRet = nue2_uninit();
		if (iRet != 0) {
            nvt_dbg(ERR, "AI_IOCTL: Error, nue2_uninit failed.\r\n");
            goto AI_IOC_ENG_UNINIT_EXIT;
        }	

AI_IOC_ENG_UNINIT_EXIT:

		break;
	
	case AI_IOC_CNN2_INIT:
		iRet = cnn_init(1);
		if (iRet != 0) {
            nvt_dbg(ERR, "AI_IOCTL: Error, cnn2_init failed.\r\n");
            goto AI_IOC_CNN2_INIT_EXIT;
        }

AI_IOC_CNN2_INIT_EXIT:

		break;
	
	case AI_IOC_CNN2_UNINIT:
		iRet = cnn_uninit(1);
		if (iRet != 0) {
            nvt_dbg(ERR, "AI_IOCTL: Error, cnn1_uninit failed.\r\n");
            goto AI_IOC_CNN2_UNINIT_EXIT;
        }

AI_IOC_CNN2_UNINIT_EXIT:

		break;
	// OPEN
	case AI_IOC_OPEN:
		memcpy(&ai_eng, p_arg, sizeof(KDRV_AI_ENG));
		er_return = kdrv_ai_open(chip, kdrv_ai_trans_eng2id(ai_eng));
		if (er_return != E_OK) {
            nvt_dbg(ERR, "AI_IOCTL: Error, kdrv_ai_open failed.\r\n");
            goto AI_IOC_OPEN_EXIT;
        }

AI_IOC_OPEN_EXIT:

		break;

	// CLOSE
	case AI_IOC_CLOSE:
		memcpy(&ai_eng, p_arg, sizeof(KDRV_AI_ENG));	
		er_return = kdrv_ai_close(chip, kdrv_ai_trans_eng2id(ai_eng));
		if (er_return != E_OK) {
            nvt_dbg(ERR, "AI_IOCTL: Error, kdrv_ai_close failed.\r\n");
            goto AI_IOC_CLOSE_EXIT;
        }

AI_IOC_CLOSE_EXIT:

		break;

	// OPENCFG
	case AI_IOC_OPENCFG:
		memcpy(&ai_cfg_data, p_arg, sizeof(ai_cfg_data));	
		id = KDRV_DEV_ID(chip, kdrv_ai_trans_eng2id(ai_cfg_data.engine), channel);
		er_return = kdrv_ai_set(id, KDRV_AI_PARAM_OPENCFG, &ai_cfg_data.opencfg);
		if (er_return != E_OK) {
            nvt_dbg(ERR, "AI_IOCTL: Error, kdrv_ai_set failed.\r\n");
            goto AI_IOC_OPENCFG_EXIT;
        }

AI_IOC_OPENCFG_EXIT:

		break;

	// SET PARM MODE
	case AI_IOC_SET_MODE:
		memcpy(&ai_mode, p_arg, sizeof(ai_mode));	
		id = KDRV_DEV_ID(chip, kdrv_ai_trans_eng2id(ai_mode.engine), channel);
		er_return = kdrv_ai_set(id, KDRV_AI_PARAM_MODE_INFO, &ai_mode.mode);
		if (er_return != E_OK) {
            nvt_dbg(ERR, "AI_IOCTL: Error, kdrv_ai_set failed.\r\n");
            goto AI_IOC_SET_MODE_EXIT;
        }

AI_IOC_SET_MODE_EXIT:

		break;

	// SET APP MODE
	case AI_IOC_SET_APP:
		memcpy(&ai_app_info, p_arg, sizeof(ai_app_info));
		if (!nvt_ai_drv_is_mem_map(ai_app_info.net_id)) {
			nvt_dbg(ERR, "memory should be mapped before app parms set\r\n");
			goto exit;
		}
		ai_app_info.info.p_head = (KDRV_AI_APP_HEAD*)nvt_ai_drv_user2kerl_va((unsigned int)ai_app_info.info.p_head, ai_app_info.net_id);
		if ((int)ai_app_info.info.p_head == 0) {
			goto exit;
		}

		id = KDRV_DEV_ID(chip, kdrv_ai_trans_eng2id(ai_app_info.engine), channel);
		er_return = kdrv_ai_set(id, KDRV_AI_PARAM_APP_INFO, &ai_app_info.info);
		if (er_return != E_OK) {
            nvt_dbg(ERR, "AI_IOCTL: Error, kdrv_ai_set failed.\r\n");
            goto AI_IOC_SET_APP_EXIT;
        }

AI_IOC_SET_APP_EXIT:

		break;

	// SET LL MODE
	case AI_IOC_SET_LL:
		memcpy(&ai_ll_info, p_arg, sizeof(ai_ll_info));
		if (!nvt_ai_drv_is_mem_map(ai_ll_info.net_id)) {
			nvt_dbg(ERR, "memory should be mapped before ll parms set\r\n");
			goto exit;
		}
		ai_ll_info.info.p_head = (KDRV_AI_LL_HEAD*)nvt_ai_drv_user2kerl_va((unsigned int)ai_ll_info.info.p_head, ai_ll_info.net_id);
		if ((int)ai_ll_info.info.p_head == 0) {
			goto exit;
		}

		id = KDRV_DEV_ID(chip, kdrv_ai_trans_eng2id(ai_ll_info.engine), channel);
		er_return = kdrv_ai_set(id, KDRV_AI_PARAM_LL_INFO, &ai_ll_info.info);
		if (er_return != E_OK) {
            nvt_dbg(ERR, "AI_IOCTL: Error, kdrv_ai_set failed.\r\n");
            goto AI_IOC_SET_LL_EXIT;
        }

AI_IOC_SET_LL_EXIT:

		break;

	// GET PARM MODE
	case AI_IOC_GET_MODE:
		memcpy(&ai_mode, p_arg, sizeof(ai_mode));
		id = KDRV_DEV_ID(chip, kdrv_ai_trans_eng2id(ai_mode.engine), channel);

		er_return = kdrv_ai_get(id, KDRV_AI_PARAM_MODE_INFO, &ai_mode.mode);
		if (er_return != E_OK) {
            nvt_dbg(ERR, "AI_IOCTL: Error, kdrv_ai_get failed.\r\n");
            goto AI_IOC_GET_MODE_EXIT;
        }
		//err = (copy_to_user((void __user *)argc, &ai_mode, sizeof(ai_mode))) ? (-EFAULT) : 0;

AI_IOC_GET_MODE_EXIT:

		break;

	// GET APP MODE
	case AI_IOC_GET_APP:
		memcpy(&ai_app_info, p_arg, sizeof(ai_app_info));

		if (!nvt_ai_drv_is_mem_map(ai_app_info.net_id)) {
			nvt_dbg(ERR, "memory should be mapped before app parms get\r\n");
			goto exit;
		}
		id = KDRV_DEV_ID(chip, kdrv_ai_trans_eng2id(ai_app_info.engine), channel);
		er_return = kdrv_ai_get(id, KDRV_AI_PARAM_APP_INFO, &ai_app_info);
		if (er_return != E_OK) {
            nvt_dbg(ERR, "AI_IOCTL: Error, kdrv_ai_get failed.\r\n");
            goto AI_IOC_GET_APP_EXIT;
		}
		ai_app_info.info.p_head = (KDRV_AI_APP_HEAD*)nvt_ai_drv_kerl2user_va((unsigned int)ai_app_info.info.p_head, ai_app_info.net_id);
		if ((int)ai_app_info.info.p_head == 0) {
			goto exit;
		}
		//err = (copy_to_user((void __user *)argc, &ai_app_info, sizeof(ai_app_info))) ? (-EFAULT) : 0;

AI_IOC_GET_APP_EXIT:

		break;

	// GET LL MODE
	case AI_IOC_GET_LL:
		memcpy(&ai_ll_info, p_arg, sizeof(ai_ll_info));
		if (!nvt_ai_drv_is_mem_map(ai_ll_info.net_id)) {
			nvt_dbg(ERR, "memory should be mapped before ll parms get\r\n");
			goto exit;
		}
		id = KDRV_DEV_ID(chip, kdrv_ai_trans_eng2id(ai_ll_info.engine), channel);
		er_return = kdrv_ai_get(id, KDRV_AI_PARAM_LL_INFO, &ai_ll_info);
		if (er_return != E_OK) {
            nvt_dbg(ERR, "AI_IOCTL: Error, kdrv_ai_get failed.\r\n");
            goto AI_IOC_GET_LL_EXIT;
        }
		ai_ll_info.info.p_head = (KDRV_AI_LL_HEAD*)nvt_ai_drv_kerl2user_va((unsigned int)ai_ll_info.info.p_head, ai_ll_info.net_id);
		if ((int)ai_ll_info.info.p_head == 0) {
			goto exit;
		}
		//err = (copy_to_user((void __user *)argc, &ai_ll_info, sizeof(ai_ll_info))) ? (-EFAULT) : 0;

AI_IOC_GET_LL_EXIT:

		break;

	// trigger engine
	case AI_IOC_TRIGGER:
		memcpy(&ai_triginfo, p_arg, sizeof(ai_triginfo));
		id = KDRV_DEV_ID(chip, kdrv_ai_trans_eng2id(ai_triginfo.engine), channel);
#if LL_SUPPORT_ROI
		er_return = kdrv_ai_trigger(id, &ai_triginfo.trig_parm, NULL, &ai_triginfo);
#else
		er_return = kdrv_ai_trigger(id, &ai_triginfo.trig_parm, NULL, NULL);
#endif
		if (er_return != E_OK) {
            nvt_dbg(ERR, "AI_IOCTL: Error, kdrv_ai_trigger failed.\r\n");
            goto AI_IOC_TRIGGER_EXIT;
        }

AI_IOC_TRIGGER_EXIT:

		break;

	// wait engine done
	case AI_IOC_WAITDONE:
		memcpy(&ai_triginfo, p_arg, sizeof(ai_triginfo));
		id = KDRV_DEV_ID(chip, kdrv_ai_trans_eng2id(ai_triginfo.engine), channel);
		er_return = kdrv_ai_waitdone(id, &ai_triginfo.trig_parm, NULL, NULL);
		if (er_return != E_OK) {
            nvt_dbg(ERR, "AI_IOCTL: Error, kdrv_ai_waitdone failed.\r\n");
            goto AI_IOC_WAITDONE_EXIT;
        }

AI_IOC_WAITDONE_EXIT:

		break;

	// reset engine
	case AI_IOC_RESET:
		memcpy(&ai_triginfo, p_arg, sizeof(ai_triginfo));
		id = KDRV_DEV_ID(chip, kdrv_ai_trans_eng2id(ai_triginfo.engine), channel);
		er_return = kdrv_ai_reset(id, &ai_triginfo.trig_parm);
		if (er_return != E_OK) {
            nvt_dbg(ERR, "AI_IOCTL: Error, kdrv_ai_reset failed.\r\n");
            goto AI_IOC_RESET_EXIT;
        }

AI_IOC_RESET_EXIT:

		break;

	case AI_IOC_SET_MAP_ADDR:
		memcpy(&ai_map_mem_info, p_arg, sizeof(ai_map_mem_info));
		net_id = ai_map_mem_info.net_id; //don't use nvt_ai_chk_net_id here, cim defects
		if (net_id > (AI_SUPPORT_NET_MAX -1)) {
			nvt_dbg(WRN, "network id is overflow(%d), clamp -> %d\r\n", (int) net_id, (int) (AI_SUPPORT_NET_MAX -1));
			net_id = (AI_SUPPORT_NET_MAX -1);
		}
		memcpy(&g_ai_drv_map_mem[net_id], &ai_map_mem_info.mem, sizeof(AI_DRV_MAP_MEM));
		break;

	case AI_IOC_GET_MAP_ADDR:
		memcpy(&ai_map_mem_info, p_arg, sizeof(AI_DRV_MAP_MEMINFO));
		net_id = ai_map_mem_info.net_id; //don't use nvt_ai_chk_net_id here, cim defects
		if (net_id > (AI_SUPPORT_NET_MAX -1)) {
			nvt_dbg(WRN, "network id is overflow(%d), clamp -> %d\r\n", (int) net_id, (int) (AI_SUPPORT_NET_MAX -1));
			net_id = (AI_SUPPORT_NET_MAX -1);
		}
		memcpy(&ai_map_mem_info.mem, &g_ai_drv_map_mem[net_id], sizeof(AI_DRV_MAP_MEM));
		//err = (copy_to_user((void __user *)argc, &ai_map_mem_info, sizeof(AI_DRV_MAP_MEMINFO))) ? (-EFAULT) : 0;
		break;
#if LL_SUPPORT_ROI
	case AI_IOC_SET_LL_USR_INFO: {	
		#if ROI_PARAM_GLOBAL
		SEM_WAIT(ai_ioctl_sem_id);
		#endif
		memcpy(&ai_ll_usr_info, p_arg, sizeof(AI_DRV_LL_USR_INFO));
		er_return = kdrv_ai_link_ll_update_addr(&ai_ll_usr_info.layer_info, ai_ll_usr_info.net_id);
		if (er_return != E_OK) {
            nvt_dbg(ERR, "AI_IOCTL: Error, kdrv_ai_link_ll_update_addr failed.\r\n");
            goto AI_IOC_SET_LL_USR_INFO_EXIT;
        }

AI_IOC_SET_LL_USR_INFO_EXIT:

		#if ROI_PARAM_GLOBAL
		SEM_SIGNAL(ai_ioctl_sem_id);
		#endif
	}
		break;
	case AI_IOC_UNINIT_LINK_INFO: {
		UINT32 net_id = 0;
		memcpy(&net_id, p_arg, sizeof(UINT32));
		er_return = kdrv_ai_link_uninit(net_id);
		if (er_return != E_OK) {
            nvt_dbg(ERR, "AI_IOCTL: Error, kdrv_ai_link_uninit failed.\r\n");
            goto AI_IOC_UNINIT_LINK_INFO_EXIT;
        }
	}

AI_IOC_UNINIT_LINK_INFO_EXIT:

		break;
#if LL_BUF_FROM_USR
	case AI_IOC_INIT_LL_BUF: {
		int ret = 0;
		#if ROI_PARAM_GLOBAL
		SEM_WAIT(ai_ioctl_sem_id);
		#endif
		memcpy(&ai_ll_buf_info, p_arg, sizeof(AI_DRV_LL_BUF_INFO));
		// size, net id
		ret = hd_common_mem_alloc("ai_ll_ioctl", &g_buf_info_pa[ai_ll_buf_info.net_id], (void**)&g_buf_info_va[ai_ll_buf_info.net_id], ai_ll_buf_info.size, DDR_ID0);
		g_buf_handle[ai_ll_buf_info.net_id] = (void*)g_buf_info_pa[ai_ll_buf_info.net_id];
		if (HD_OK == ret) {
			printf("mem alloc ,virt_addr error handling fail\r\n");
			return HD_ERR_NG;
		}
		if (g_buf_handle[ai_ll_buf_info.net_id] == NULL) {
			nvt_dbg(ERR, "get buffer fail\n");
			goto exit;
		}
		net_ll_pa[ai_ll_buf_info.net_id] = (UINT32)g_buf_info_pa[ai_ll_buf_info.net_id]; 
		net_ll_va[ai_ll_buf_info.net_id] = (UINT32)g_buf_info_va[ai_ll_buf_info.net_id]; 
		net_ll_size[ai_ll_buf_info.net_id] = ai_ll_buf_info.size;
		
		#if ROI_PARAM_GLOBAL
		SEM_SIGNAL(ai_ioctl_sem_id);
		#endif

	}
		break;
	
	case AI_IOC_UNINIT_LL_BUF: {
		UINT32 net_id = 0;
		
		#if ROI_PARAM_GLOBAL
		SEM_WAIT(ai_ioctl_sem_id);
		#endif
		memcpy(&net_id, p_arg, sizeof(UINT32));
		if (g_buf_handle[net_id] == NULL) {
			nvt_dbg(ERR, "free buf: handle %d is null!\n", (int)net_id);
			return -1;
		} else {
			
			int ret =  hd_common_mem_free(g_buf_info_pa[net_id], (void *)g_buf_info_va[net_id]);
			if (ret!= HD_OK) {
				nvt_dbg(ERR, "failed in release buffer\n");
				return -1;
			}
			g_buf_handle[net_id] = NULL;
			net_ll_size[net_id] = 0;
		}
		#if ROI_PARAM_GLOBAL
		SEM_SIGNAL(ai_ioctl_sem_id);
		#endif
	}
		break;
#endif
#endif

	case NUE_IOC_RUN_FC: {
#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
		nvt_dbg(ERR, "AI_FASTBOOT: Error, NUE_IOC_RUN_FC can't be called at fastboot.\n");
#else
		KDRV_AI_FC_PARM input_parm = {0};
		NUE_PARM nue_parm = {0};
		UINT32 run_idx = 0;
		UINT32 run_time = 0;
		UINT32 tmp_h = 0;
		
		memcpy(&input_parm, p_arg, sizeof(KDRV_AI_FC_PARM));
		
		// init parameter
		input_parm.func_list[0] = KDRV_AI_FC_FULLY_EN;
		input_parm.func_list[1] = 0;
		input_parm.src_fmt  = AI_FC_SRC_FEAT;
		input_parm.out_type = AI_IO_INT8;
		input_parm.out_addr = 0;
		input_parm.size.width   = 1;
		input_parm.size.height  = 1;
		input_parm.size.channel = 256;
		input_parm.batch_num = 1;
		input_parm.in_interm_addr    = 0;
		input_parm.in_interm_dma_en  = 0;
		input_parm.out_interm_dma_en = 3;
		input_parm.in_interm_ofs.line_ofs = 1;              
		input_parm.in_interm_ofs.channel_ofs = 1;      
		input_parm.in_interm_ofs.batch_ofs = 256;
		input_parm.fc_ker.weight_w = 256;
		input_parm.fc_ker.sclshf.in_shift  = 0;
		input_parm.fc_ker.sclshf.out_shift = 0;
		input_parm.fc_ker.sclshf.in_scale  = 1;
		input_parm.fc_ker.sclshf.out_scale = 1;
		input_parm.fc_ker.bias_addr = 0;
		input_parm.act.mode = AI_ACT_RELU;
		input_parm.act.relu.leaky_val = 0;
		input_parm.act.relu.leaky_shf = 0;
		input_parm.act.neg_en = 0;
		input_parm.act.act_shf0 = 0;
		input_parm.act.act_shf1 = 0;
		input_parm.act.sclshf.in_shift  = 0;
		input_parm.act.sclshf.out_shift = 0;
		input_parm.act.sclshf.in_scale  = 1;
		input_parm.act.sclshf.out_scale = 1;
		input_parm.fcd.func_en = KDRV_AI_FCD_QUANTI_EN;
		input_parm.fcd.quanti_kmeans_addr = 0;
		input_parm.fcd.enc_bit_length = 0;
		input_parm.fcd.vlc_code_size = 0;
		input_parm.fcd.vlc_valid_size = 0;
		input_parm.fcd.vlc_ofs_size = 0;
		kdrv_ai_tran_nue_fc_parm(&input_parm, &nue_parm, KDRV_AI_FC_FULLY_EN);
		
		nue_parm.dmaio_addr.drv_dma_not_sync[0] = 1;
		nue_parm.dmaio_addr.drv_dma_not_sync[2] = 1;
		nue_parm.dmaio_addr.drv_dma_not_sync[3] = 1;
		nue_parm.dmaio_addr.drv_dma_not_sync[4] = 1;
		nue_parm.dmaio_addr.drv_dma_not_sync[6] = 1;
		
		
		run_time = (nue_parm.insvm_size.sv_h / 256) + (((nue_parm.insvm_size.sv_h % 256) > 0)?1:0);
		tmp_h    = input_parm.fc_ker.weight_h;
		er_return = nue_open(NULL);
		if (er_return != E_OK) {
            nvt_dbg(ERR, "AI_IOCTL: Error, nue_open failed.\r\n");
            goto NUE_IOC_RUN_FC_EXIT;
        }
		for (run_idx = 0; run_idx < run_time; run_idx++) {
			nue_parm.insvm_size.sv_h = (tmp_h < 256)?tmp_h:256;
			nue_parm.dmaio_addr.addr_out  = input_parm.out_interm_addr + run_idx*4*256;
			nue_parm.dmaio_addr.addrin_sv = input_parm.fc_ker.weight_addr + run_idx*nue_parm.insvm_size.sv_w*256;
			nue_parm.svm_parm.fcd_parm.fcd_enc_bit_length = 256*nue_parm.insvm_size.sv_h*8;
			if (nue_setmode(NUE_OPMODE_USERDEFINE, &nue_parm) == E_OK) {
				er_return = nue_start();
				if (er_return != E_OK) {
					nvt_dbg(ERR, "AI_IOCTL: Error, nue_start failed.\r\n");
					goto NUE_IOC_RUN_FC_1_EXIT;
				}
				nue_wait_frameend(FALSE);
			}
			er_return = nue_pause();
			if (er_return != E_OK) {
				nvt_dbg(ERR, "AI_IOCTL: Error, nue_pause failed.\r\n");
				goto NUE_IOC_RUN_FC_1_EXIT;
			}
			tmp_h -= nue_parm.insvm_size.sv_h;
		}

NUE_IOC_RUN_FC_1_EXIT:

		er_return = nue_close();
		if (er_return != E_OK) {
			nvt_dbg(ERR, "AI_IOCTL: Error, nue_close failed.\r\n");
			goto NUE_IOC_RUN_FC_EXIT;
		}
#endif //#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
	}

#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
NUE_IOC_RUN_FC_EXIT:
#endif

		break;
	case NUE_IOC_INIT:
		iRet = nue_init();
		if (iRet != 0) {
            nvt_dbg(ERR, "AI_IOCTL: Error, nue_init failed.\r\n");
            goto NUE_IOC_INIT_EXIT;
        }

NUE_IOC_INIT_EXIT:

		break;
	
	case NUE_IOC_UNINIT:
		iRet = nue_uninit();
		if (iRet != 0) {
            nvt_dbg(ERR, "AI_IOCTL: Error, nue_uninit failed.\r\n");
            goto NUE_IOC_UNINIT_EXIT;
        }

NUE_IOC_UNINIT_EXIT:

		break;

	case AI_IOC_DMA_ABORT:
		memcpy(&ai_eng, p_arg, sizeof(KDRV_AI_ENG));
        er_return = kdrv_ai_dma_abort(chip, AI_ENG_TOTAL);
        if (er_return != E_OK) {
            nvt_dbg(ERR, "AI_IOCTL: Error, kdrv_ai_dma_abort failed.\r\n");
            goto AI_IOC_DMA_ABORT_EXIT;
        }

AI_IOC_DMA_ABORT_EXIT:

        break;


	default:
		nvt_dbg(ERR, "unknown ai ioctl cmd: %08x\r\n", uiCmd);
		break;
	}
exit:
	return iRet;
}

#endif

