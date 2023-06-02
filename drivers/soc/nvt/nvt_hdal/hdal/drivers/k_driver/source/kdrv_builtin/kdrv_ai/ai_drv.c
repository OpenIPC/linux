#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include <linux/vmalloc.h>
#include "kwrap/type.h"
#include "ai_drv.h"
#include "ai_lib.h"
#include "ai_ioctl.h"
#if defined(__FREERTOS)
#include "kwrap/debug.h"
#else
#include "kdrv_ai_dbg.h"
#endif
#include "kdrv_ai.h"
#include "mach/fmem.h"
#include <kwrap/verinfo.h>
#if defined(_BSP_NA51055_)
#include <plat-na51055/nvt-sramctl.h>
#endif
#if defined(_BSP_NA51089_)
#include <plat-na51089/nvt-sramctl.h>
#endif
#include "kdrv_builtin.h"

#include "kdrv_ai_int.h"
/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/
int nvt_ai_drv_wait_cmd_complete(PAI_INFO pmodule_info);
#if (KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
int nvt_ai_drv_ioctl(unsigned char if_id, AI_INFO *pmodule_info, unsigned int cmd, unsigned long arg);
#endif
void nvt_ai_drv_do_tasklet(unsigned long data);
irqreturn_t nvt_ai_drv_isr(int irq, void *devid);
/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/

typedef irqreturn_t (*irq_handler_t)(int, void *);

/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/
AI_DRV_MAP_MEM g_ai_drv_map_mem[AI_SUPPORT_NET_MAX] = {0};

#if LL_SUPPORT_ROI


#if LL_BUF_FROM_USR
void *g_buf_handle[AI_SUPPORT_NET_MAX] = {NULL};
struct nvt_fmem_mem_info_t g_buf_info = {0};
UINT32 net_ll_size[AI_SUPPORT_NET_MAX] = {0};
UINT32 net_ll_va[AI_SUPPORT_NET_MAX] = {0};
UINT32 net_ll_pa[AI_SUPPORT_NET_MAX] = {0};
#else
void *g_buf_handle = NULL;
struct nvt_fmem_mem_info_t g_buf_info = {0};
#define MAX_LL_BUF_SIZE 0x2000000
#define AI_SINGLE_NET_LL_SIZE (MAX_LL_BUF_SIZE/AI_SUPPORT_NET_MAX)
#endif
#define ROI_PARAM_GLOBAL 1
#if ROI_PARAM_GLOBAL
AI_DRV_LL_USR_INFO  ai_ll_usr_info = {0};
AI_DRV_LL_BUF_INFO  ai_ll_buf_info = {0};
#endif

#endif
extern VOID cnn_isr(BOOL cnn_id);
extern VOID nue_isr(VOID);
extern VOID nue2_isr(VOID);

KDRV_AI_PREPROC_PARM g_nue2_input_parm = {0};
NUE2_PARM g_nue2_parm = {0};

typedef void (* kdrv_ai_nue_isr_t)(VOID);
typedef void (* kdrv_ai_nue2_isr_t)(VOID);
typedef void (* kdrv_ai_cnn_isr_t)(BOOL);

kdrv_ai_nue_isr_t  kdrv_ai_nue_isr=NULL;
kdrv_ai_nue2_isr_t  kdrv_ai_nue2_isr=NULL;
kdrv_ai_cnn_isr_t  kdrv_ai_cnn_isr=NULL;

extern UINT32 is_ai_enable_dtsi;
extern UINT32 do_ai_init;

VOID kdrv_ai_reg_nue_isr_cb(kdrv_ai_nue_isr_t nue_reg_isr)
{
	if (nue_reg_isr) {
		kdrv_ai_nue_isr = nue_reg_isr;
	}
}

VOID kdrv_ai_reg_nue2_isr_cb(kdrv_ai_nue2_isr_t nue2_reg_isr)
{
	if (nue2_reg_isr) {
		kdrv_ai_nue2_isr = nue2_reg_isr;
	}
}

VOID kdrv_ai_reg_cnn_isr_cb(kdrv_ai_cnn_isr_t cnn_reg_isr)
{
	if (cnn_reg_isr) {
		kdrv_ai_cnn_isr = cnn_reg_isr;
	}
}

UINT8 * kdrv_ai_vmalloc(UINT32 v_size)
{
	char *v_buff;

	v_buff = (char *) vmalloc(v_size);

	return (UINT8 *) v_buff;
}
EXPORT_SYMBOL(kdrv_ai_vmalloc);

VOID kdrv_ai_vfree(UINT8 *v_buff)
{
	if (v_buff) {
		vfree((char *)v_buff);
	}

	return;
}
EXPORT_SYMBOL(kdrv_ai_vfree);

#if LL_SUPPORT_ROI
UINT32 kdrv_ai_get_drv_base_va(UINT32 net_id)
{
#if LL_BUF_FROM_USR
	if (net_ll_va[net_id] == 0) {
		printk("ll cmd buffer is NULL!\n");
		return 0;
	}
	return net_ll_va[net_id];
#else
	if (g_buf_info.vaddr == 0) {
		printk("ll cmd buffer is NULL!\n");
		return 0;
	}
	return (UINT32)(g_buf_info.vaddr + net_id*AI_SINGLE_NET_LL_SIZE);
#endif
}

UINT32 kdrv_ai_get_drv_base_pa(UINT32 net_id)
{
#if LL_BUF_FROM_USR
	if (net_ll_pa[net_id] == 0) {
		printk("ll cmd buffer is NULL!\n");
		return 0;
	}
	return net_ll_pa[net_id];
#else
	if (g_buf_info.paddr == 0) {
		printk("ll cmd buffer is NULL!\n");
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

#if (KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
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

static bool nvt_ai_drv_is_mem_map(UINT32 net_id)
{
	AI_DRV_MAP_MEM *p_mem;
	net_id = nvt_ai_chk_net_id(net_id);
	p_mem = &g_ai_drv_map_mem[net_id];

	if ((p_mem->kerl_pa == 0) || (p_mem->kerl_va == 0) || (p_mem->user_pa == 0) || (p_mem->user_va == 0)) {
		nvt_dbg(ERR, "map mem user va=%08x, user pa=%08x, kernel va=%08x, kernel pa=%08x\r\n"
				, (int)p_mem->user_va, (int)p_mem->user_pa, (int)p_mem->kerl_va, (int)p_mem->kerl_pa);
		return false;
	} else {
		return true;
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
#endif //#if (KDRV_AI_MINI_FOR_FASTBOOT == 1)

int nvt_ai_drv_open(PAI_INFO pmodule_info, unsigned char if_id)
{
	/* Add HW Module initial operation here when the device file opened*/

	return 0;
}


int nvt_ai_drv_release(PAI_INFO pmodule_info, unsigned char if_id)
{
	/* Add HW Module release operation here when device file closed */

	return 0;
}

int nvt_ai_drv_init(AI_INFO *pmodule_info)
{
	int err = 0, i = 0;
#if LL_SUPPORT_ROI
#if (LL_BUF_FROM_USR == 0)
	int ret = 0;
#endif
#endif	
	if (pmodule_info == NULL) {
		nvt_dbg(ERR, "invalid input\r\n");
		return -EINVAL;
	}

	init_waitqueue_head(&pmodule_info->ai_wait_queue);
	vk_spin_lock_init(&pmodule_info->ai_spinlock);
	vk_sema_init(&pmodule_info->ai_sem, 1);
	init_completion(&pmodule_info->ai_completion);
	tasklet_init(&pmodule_info->ai_tasklet, nvt_ai_drv_do_tasklet, (unsigned long)pmodule_info);

#if 0
	/* initial clock here */
	for (i = 0; i < MODULE_CLK_NUM; i++) {
		if (IS_ERR(pmodule_info->pclk[i])) {
			DBG_IND("====ise%d-clk_err...\r\n", i);
		} else {
			//clk_prepare(pmodule_info->pclk[i]);
			//clk_enable(pmodule_info->pclk[i]);
			//clk_prepare(ai_clk[i]);
			//clk_enable(ai_clk[i]);
			nvt_ai_set_reset_and_clear(i);
			nvt_ai_set_default_clk_off(i);
			DBG_IND("clk %d is enable\r\n", i);
		}
	}
#endif

#if 0
	nvt_enable_sram_shutdown(NUE_SD);
	nvt_enable_sram_shutdown(CNN_SD);
	nvt_enable_sram_shutdown(NUE2_SD);
#endif

	if (is_ai_enable_dtsi == 1) {
		if (do_ai_init == 1) {
			/* register IRQ here*/
			for (i = 0; i < MODULE_IRQ_NUM; i++) {
				if (request_irq(pmodule_info->iinterrupt_id[i], nvt_ai_drv_isr, IRQF_TRIGGER_HIGH, "AI_INT", pmodule_info)) {
					nvt_dbg(ERR, "failed to register an IRQ Int:%d\n", pmodule_info->iinterrupt_id[i]);
					err = -ENODEV;
					goto FAIL_FREE_IRQ;
				}
			}
		}
	} else {
		if (kdrv_builtin_is_fastboot()) {
#if (KDRV_AI_MINI_FOR_FASTBOOT == 1)
			/* register IRQ here*/
			for (i = 0; i < MODULE_IRQ_NUM; i++) {
				if (request_irq(pmodule_info->iinterrupt_id[i], nvt_ai_drv_isr, IRQF_TRIGGER_HIGH, "AI_INT", pmodule_info)) {
					nvt_dbg(ERR, "failed to register an IRQ Int:%d\n", pmodule_info->iinterrupt_id[i]);
					err = -ENODEV;
					goto FAIL_FREE_IRQ;
				}
			}
#else
#endif //#if (KDRV_AI_MINI_FOR_FASTBOOT == 1)
		} else {
			/* register IRQ here*/
			for (i = 0; i < MODULE_IRQ_NUM; i++) {
				if (request_irq(pmodule_info->iinterrupt_id[i], nvt_ai_drv_isr, IRQF_TRIGGER_HIGH, "AI_INT", pmodule_info)) {
					nvt_dbg(ERR, "failed to register an IRQ Int:%d\n", pmodule_info->iinterrupt_id[i]);
					err = -ENODEV;
					goto FAIL_FREE_IRQ;
				}
			}
		}
	}

	/* Add HW Module initialization here when driver loaded */
	//nvt_dbg(IND, "ai set engine resource...\r\n");
	//cnn_set_base_addr((UINT32)pmodule_info->io_addr[0]);
	//nvt_dbg(IND, "ai set NUE  addr = 0x%08X\r\n", (UINT32)pmodule_info->io_addr[0]);
	nue_set_base_addr((UINT32)pmodule_info->io_addr[0]); 
	//nvt_dbg(IND, "ai set NUE2 addr = 0x%08X\r\n", (UINT32)pmodule_info->io_addr[1]);
	nue2_set_base_addr((UINT32)pmodule_info->io_addr[1]);
#if defined(_BSP_NA51089_)
	//nvt_dbg(IND, "ai set CNN1  addr = 0x%08X\r\n", (UINT32)pmodule_info->io_addr[2]);
	cnn_set_base_addr(0, (UINT32)pmodule_info->io_addr[2]);
#else
	//nvt_dbg(IND, "ai set CNN2  addr = 0x%08X\r\n", (UINT32)pmodule_info->io_addr[2]);
	cnn_set_base_addr(1, (UINT32)pmodule_info->io_addr[2]);
	//nvt_dbg(IND, "ai set CNN1  addr = 0x%08X\r\n", (UINT32)pmodule_info->io_addr[3]);
	cnn_set_base_addr(0, (UINT32)pmodule_info->io_addr[3]);
#endif

	nue_create_resource(ai_clk[0], ai_freq_from_dtsi[0]);
	nue2_create_resource((VOID *) ai_clk[1], ai_freq_from_dtsi[1]);
#if defined(_BSP_NA51089_)
	cnn_create_resource(0, ai_clk[2], ai_freq_from_dtsi[2]);
#else
	cnn_create_resource(1, ai_clk[2], ai_freq_from_dtsi[2]);
	cnn_create_resource(0, ai_clk[3], ai_freq_from_dtsi[3]);
#endif

	kdrv_ai_install_id();
	kdrv_ai_init();
	

#if LL_SUPPORT_ROI
#if (LL_BUF_FROM_USR == 0)
	ret = nvt_fmem_mem_info_init(&g_buf_info, NVT_FMEM_ALLOC_NONCACHE, MAX_LL_BUF_SIZE, NULL);
    if (ret >= 0) {
		g_buf_handle = fmem_alloc_from_cma(&g_buf_info, 0);
		if (g_buf_handle == NULL) {
			nvt_dbg(ERR, "get buffer fail\n");
			return -EINVAL;
		}	
	} else {
		nvt_dbg(ERR, "init buffer fail\n");
		return -EINVAL;
	}
#endif
#endif
	//========
	//nvt_dbg(IND, "ai set engine resource...done\r\n");
	return err;

FAIL_FREE_IRQ:

	if (is_ai_enable_dtsi == 1) {
		if (do_ai_init == 1) {	
			free_irq(pmodule_info->iinterrupt_id[i], pmodule_info);
		}
	} else {
		if (kdrv_builtin_is_fastboot()) {
#if (KDRV_AI_MINI_FOR_FASTBOOT == 1)
			free_irq(pmodule_info->iinterrupt_id[i], pmodule_info);
#else
#endif //#if (KDRV_AI_MINI_FOR_FASTBOOT == 1)
		} else {
			free_irq(pmodule_info->iinterrupt_id[i], pmodule_info);
		}
	}

	/* Add error handler here */

	return err;
}

int nvt_ai_drv_remove(AI_INFO *pmodule_info)
{
	UINT32 i = 0;
	if (pmodule_info == NULL) {
		nvt_dbg(ERR, "invalid input\r\n");
		return -EINVAL;
	}

	if (is_ai_enable_dtsi == 1) {
		if (do_ai_init == 1) {
			for (i = 0; i < MODULE_IRQ_NUM; i++) {
				free_irq(pmodule_info->iinterrupt_id[i], pmodule_info);
			}
		}
	} else {
		//Free IRQ
		if (kdrv_builtin_is_fastboot()) {
#if (KDRV_AI_MINI_FOR_FASTBOOT == 1)
			for (i = 0; i < MODULE_IRQ_NUM; i++) {
				free_irq(pmodule_info->iinterrupt_id[i], pmodule_info);
			}
#else
#endif //#if (KDRV_AI_MINI_FOR_FASTBOOT == 1)
		} else {
			for (i = 0; i < MODULE_IRQ_NUM; i++) {
				free_irq(pmodule_info->iinterrupt_id[i], pmodule_info);
			}
		}
	}

#if 0
	/* release to reset keep here */
	for (i = 0; i < MODULE_CLK_NUM; i++) {
		nvt_ai_set_reset_keep(i);
	}
#endif

	/* Add HW Module release operation here*/
#if LL_SUPPORT_ROI
#if (LL_BUF_FROM_USR == 0)
	if (g_buf_handle == NULL) {
		nvt_dbg(ERR, "free buf: handle is null!\n");
		return -1;
	} else {
		int ret = fmem_release_from_cma(g_buf_handle, 0);
		if (ret < 0) {
			nvt_dbg(ERR, "failed in release buffer\n");
			return -1;
		}
	}
#endif
#endif
	nue_release_resource();
	nue2_release_resource();
#if defined(_BSP_NA51089_)
	cnn_release_resource(0);
#else
	cnn_release_resource(0);
	cnn_release_resource(1);
#endif
	kdrv_ai_uninstall_id();
	
	return 0;
}

int nvt_ai_drv_suspend(AI_INFO *pmodule_info)
{
	if (pmodule_info == NULL) {
		nvt_dbg(ERR, "invalid input\r\n");
		return -EINVAL;
	}

	/* Add suspend operation here*/

	return 0;
}

int nvt_ai_drv_resume(AI_INFO *pmodule_info)
{
	if (pmodule_info == NULL) {
		nvt_dbg(ERR, "invalid input\r\n");
		return -EINVAL;
	}

	/* Add resume operation here*/

	return 0;
}

#if (KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
int nvt_ai_drv_ioctl(unsigned char if_id, AI_INFO *pmodule_info, unsigned int cmd, unsigned long argc)
{
	ER er_return;
	REG_INFO reg_info;
	REG_INFO_LIST reg_info_list;
	int loop_count;
	int err = 0;
	AI_DRV_OPENCFG      ai_cfg_data = {0};
	AI_DRV_MODEINFO     ai_mode;
	AI_DRV_APPINFO      ai_app_info = {0};
	AI_DRV_LLINFO       ai_ll_info = {0};
	AI_DRV_TRIGINFO   	ai_triginfo = {0};
	AI_DRV_MAP_MEMINFO  ai_map_mem_info = {0};
	KDRV_AI_ENG ai_eng = 0;
	const UINT32 chip = 0, channel = 0;
	UINT32 id = 0;
#if LL_SUPPORT_ROI
#if (ROI_PARAM_GLOBAL == 0)
	AI_DRV_LL_USR_INFO  ai_ll_usr_info = {0};
#endif
#endif

	if (pmodule_info == NULL) {
		nvt_dbg(ERR, "invalid input\r\n");
		return -EINVAL;
	}

	switch (cmd) {
	case AI_IOC_START:
		/*call someone to start operation*/
		break;

	case AI_IOC_STOP:
		/*call someone to stop operation*/
		break;

	case AI_IOC_READ_REG:
		err = copy_from_user(&reg_info, (void __user *)argc, sizeof(REG_INFO));
		if (!err) {
			reg_info.reg_value = READ_REG(pmodule_info->io_addr[if_id] + reg_info.reg_addr);
			err = copy_to_user((void __user *)argc, &reg_info, sizeof(REG_INFO));
		}
		break;

	case AI_IOC_WRITE_REG:
		err = copy_from_user(&reg_info, (void __user *)argc, sizeof(REG_INFO));
		if (!err) {
			WRITE_REG(reg_info.reg_value, pmodule_info->io_addr[if_id] + reg_info.reg_addr);
		}
		break;

	case AI_IOC_READ_REG_LIST:
		err = copy_from_user(&reg_info_list, (void __user *)argc, sizeof(REG_INFO_LIST));
		if (!err) {
			if (reg_info_list.reg_cnt <= MODULE_REG_LIST_NUM) {
				for (loop_count = 0 ; loop_count < reg_info_list.reg_cnt ; loop_count++) {
					reg_info_list.reg_list[loop_count].reg_value = READ_REG(pmodule_info->io_addr[if_id] + reg_info_list.reg_list[loop_count].reg_addr);
				}
			} else {
				DBG_ERR("Loop bound error!\r\n");
			}
			err = copy_to_user((void __user *)argc, &reg_info_list, sizeof(REG_INFO_LIST));
		}
		break;
	case AI_IOC_WRITE_REG_LIST:
		err = copy_from_user(&reg_info_list, (void __user *)argc, sizeof(REG_INFO_LIST));
		if (!err) {
			if (reg_info_list.reg_cnt <= MODULE_REG_LIST_NUM) {
				for (loop_count = 0 ; loop_count < reg_info_list.reg_cnt ; loop_count++) {
					WRITE_REG(reg_info_list.reg_list[loop_count].reg_value, pmodule_info->io_addr[if_id] + reg_info_list.reg_list[loop_count].reg_addr);
				}
			} else {
				DBG_ERR("Loop bound error!\r\n");
			}
		}
		break;

	/* Add other operations here */
	// engine init
	case AI_IOC_ENG_INIT:
		err = nue_init();
		if (err != 0) {
            err = -EFAULT;
            nvt_dbg(ERR, "AI_DRV: Error, nue_init failed.\r\n");
            goto AI_IOC_ENG_INIT_EXIT;
        }
		err = cnn_init(0);
		if (err != 0) {
            err = -EFAULT;
            nvt_dbg(ERR, "AI_DRV: Error, cnn_init failed.\r\n");
            goto AI_IOC_ENG_INIT_EXIT;
        }
		err = nue2_init();
		if (err != 0) {
            err = -EFAULT;
            nvt_dbg(ERR, "AI_DRV: Error, nue2_init failed.\r\n");
            goto AI_IOC_ENG_INIT_EXIT;
        }

AI_IOC_ENG_INIT_EXIT:

		break;
		
	case AI_IOC_ENG_UNINIT:
		err = nue_uninit();
		if (err != 0) {
            err = -EFAULT;
            nvt_dbg(ERR, "AI_DRV: Error, nue_uninit failed.\r\n");
            goto AI_IOC_ENG_UNINIT_EXIT;
        }
		err = cnn_uninit(0);
		if (err != 0) {
            err = -EFAULT;
            nvt_dbg(ERR, "AI_DRV: Error, cnn_uninit failed.\r\n");
            goto AI_IOC_ENG_UNINIT_EXIT;
        }
		err = nue2_uninit();
		if (err != 0) {
            err = -EFAULT;
            nvt_dbg(ERR, "AI_DRV: Error, nue2_uninit failed.\r\n");
            goto AI_IOC_ENG_UNINIT_EXIT;
        }

AI_IOC_ENG_UNINIT_EXIT:

		break;
		// engine init
	case AI_IOC_CNN2_INIT:
		err = cnn_init(1);
		if (err != 0) {
            err = -EFAULT;
            nvt_dbg(ERR, "AI_DRV: Error, cnn_init failed.\r\n");
            goto AI_IOC_CNN2_INIT_EXIT;
        }

AI_IOC_CNN2_INIT_EXIT:

		break;

	case AI_IOC_CNN2_UNINIT:
		err = cnn_uninit(1);
		if (err != 0) {
            err = -EFAULT;
            nvt_dbg(ERR, "AI_DRV: Error, cnn_uninit failed.\r\n");
            goto AI_IOC_CNN2_UNINIT_EXIT;
        }

AI_IOC_CNN2_UNINIT_EXIT:

		break;
	// OPEN
	case AI_IOC_OPEN:
		if (copy_from_user(&ai_eng, (void __user *)argc, sizeof(KDRV_AI_ENG))) {
			err = -EFAULT;
			goto exit;
		}
		er_return = kdrv_ai_open(chip, kdrv_ai_trans_eng2id(ai_eng));
		if (er_return != E_OK) {
            err = -EFAULT;
            nvt_dbg(ERR, "AI_DRV: Error, kdrv_ai_open failed.\r\n");
            goto AI_IOC_OPEN_EXIT;
        }

AI_IOC_OPEN_EXIT:

		break;

	// CLOSE
	case AI_IOC_CLOSE:
		if (copy_from_user(&ai_eng, (void __user *)argc, sizeof(KDRV_AI_ENG))) {
			err = -EFAULT;
			goto exit;
		}
		er_return = kdrv_ai_close(chip, kdrv_ai_trans_eng2id(ai_eng));
		if (er_return != E_OK) {
            err = -EFAULT;
            nvt_dbg(ERR, "AI_DRV: Error, kdrv_ai_close failed.\r\n");
			goto AI_IOC_CLOSE_EXIT;
        }

AI_IOC_CLOSE_EXIT:

		break;

	// OPENCFG
	case AI_IOC_OPENCFG:
		if (copy_from_user(&ai_cfg_data, (void __user *)argc, sizeof(ai_cfg_data))) {
			err = -EFAULT;
			goto exit;
		}
		id = KDRV_DEV_ID(chip, kdrv_ai_trans_eng2id(ai_cfg_data.engine), channel);
		er_return = kdrv_ai_set(id, KDRV_AI_PARAM_OPENCFG, &ai_cfg_data.opencfg);
		if (er_return != E_OK) {
            err = -EFAULT;
            nvt_dbg(ERR, "AI_DRV: Error, kdrv_ai_set failed.\r\n");
            goto AI_IOC_OPENCFG_EXIT;
        }

AI_IOC_OPENCFG_EXIT:

		break;

	// SET PARM MODE
	case AI_IOC_SET_MODE:
		if (copy_from_user(&ai_mode, (void __user *)argc, sizeof(ai_mode))) {
			err = -EFAULT;
			goto exit;
		}
		id = KDRV_DEV_ID(chip, kdrv_ai_trans_eng2id(ai_mode.engine), channel);
		er_return = kdrv_ai_set(id, KDRV_AI_PARAM_MODE_INFO, &ai_mode.mode);
		if (er_return != E_OK) {
            err = -EFAULT;
            nvt_dbg(ERR, "AI_DRV: Error, kdrv_ai_set failed.\r\n");
            goto AI_IOC_SET_MODE_EXIT;
        }

AI_IOC_SET_MODE_EXIT:

		break;

	// SET APP MODE
	case AI_IOC_SET_APP:
		if (copy_from_user(&ai_app_info, (void __user *)argc, sizeof(ai_app_info))) {
			err = -EFAULT;
			goto exit;
		}
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
            err = -EFAULT;
            nvt_dbg(ERR, "AI_DRV: Error, kdrv_ai_set failed.\r\n");
            goto AI_IOC_SET_APP_EXIT;
        }

AI_IOC_SET_APP_EXIT:

		break;

	// SET LL MODE
	case AI_IOC_SET_LL:
		if (copy_from_user(&ai_ll_info, (void __user *)argc, sizeof(ai_ll_info))) {
			err = -EFAULT;
			goto exit;
		}
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
            err = -EFAULT;
            nvt_dbg(ERR, "AI_DRV: Error, kdrv_ai_set failed.\r\n");
            goto AI_IOC_SET_LL_EXIT;
        }

AI_IOC_SET_LL_EXIT:

		break;

	// GET PARM MODE
	case AI_IOC_GET_MODE:
		if (copy_from_user(&ai_mode, (void __user *)argc, sizeof(ai_mode))) {
			err = -EFAULT;
			goto exit;
		}
		id = KDRV_DEV_ID(chip, kdrv_ai_trans_eng2id(ai_mode.engine), channel);

		er_return = kdrv_ai_get(id, KDRV_AI_PARAM_MODE_INFO, &ai_mode.mode);
		if (er_return != E_OK) {
            nvt_dbg(ERR, "AI_DRV: Error, kdrv_ai_get failed.\r\n");
            goto AI_IOC_GET_MODE_EXIT;
        }

AI_IOC_GET_MODE_EXIT:

		err = (copy_to_user((void __user *)argc, &ai_mode, sizeof(ai_mode))) ? (-EFAULT) : 0;
		break;

	// GET APP MODE
	case AI_IOC_GET_APP:
		if (copy_from_user(&ai_app_info, (void __user *)argc, sizeof(ai_app_info))) {
			err = -EFAULT;
			goto exit;
		}

		if (!nvt_ai_drv_is_mem_map(ai_app_info.net_id)) {
			nvt_dbg(ERR, "memory should be mapped before app parms get\r\n");
			goto exit;
		}
		id = KDRV_DEV_ID(chip, kdrv_ai_trans_eng2id(ai_app_info.engine), channel);
		er_return = kdrv_ai_get(id, KDRV_AI_PARAM_APP_INFO, &ai_app_info);
		if (er_return != E_OK) {
            nvt_dbg(ERR, "AI_DRV: Error, kdrv_ai_get failed.\r\n");
            goto AI_IOC_GET_APP_EXIT;
        }

		ai_app_info.info.p_head = (KDRV_AI_APP_HEAD*)nvt_ai_drv_kerl2user_va((unsigned int)ai_app_info.info.p_head, ai_app_info.net_id);
		if ((int)ai_app_info.info.p_head == 0) {
			goto exit;
		}

AI_IOC_GET_APP_EXIT:

		err = (copy_to_user((void __user *)argc, &ai_app_info, sizeof(ai_app_info))) ? (-EFAULT) : 0;
		break;

	// GET LL MODE
	case AI_IOC_GET_LL:
		if (copy_from_user(&ai_ll_info, (void __user *)argc, sizeof(ai_ll_info))) {
			err = -EFAULT;
			goto exit;
		}

		if (!nvt_ai_drv_is_mem_map(ai_ll_info.net_id)) {
			nvt_dbg(ERR, "memory should be mapped before ll parms get\r\n");
			goto exit;
		}
		id = KDRV_DEV_ID(chip, kdrv_ai_trans_eng2id(ai_ll_info.engine), channel);
		er_return = kdrv_ai_get(id, KDRV_AI_PARAM_LL_INFO, &ai_ll_info);
		if (er_return != E_OK) {
            nvt_dbg(ERR, "AI_DRV: Error, kdrv_ai_get failed.\r\n");
            goto AI_IOC_GET_LL_EXIT;
        }

		ai_ll_info.info.p_head = (KDRV_AI_LL_HEAD*)nvt_ai_drv_kerl2user_va((unsigned int)ai_ll_info.info.p_head, ai_ll_info.net_id);
		if ((int)ai_ll_info.info.p_head == 0) {
			goto exit;
		}

AI_IOC_GET_LL_EXIT:

		err = (copy_to_user((void __user *)argc, &ai_ll_info, sizeof(ai_ll_info))) ? (-EFAULT) : 0;
		break;

	// trigger engine
	case AI_IOC_TRIGGER:
		if (copy_from_user(&ai_triginfo, (void __user *)argc, sizeof(ai_triginfo))) {
			err = -EFAULT;
			goto exit;
		}
		id = KDRV_DEV_ID(chip, kdrv_ai_trans_eng2id(ai_triginfo.engine), channel);
#if LL_SUPPORT_ROI
		er_return = kdrv_ai_trigger(id, &ai_triginfo.trig_parm, NULL, &ai_triginfo);
#else
		er_return = kdrv_ai_trigger(id, &ai_triginfo.trig_parm, NULL, NULL);
#endif
		if (er_return != E_OK) {
            err = -EFAULT;
            nvt_dbg(ERR, "AI_DRV: Error, kdrv_ai_trigger failed.\r\n");
            goto AI_IOC_TRIGGER_EXIT;
        }

AI_IOC_TRIGGER_EXIT:

		break;

	// wait engine done
	case AI_IOC_WAITDONE:
		if (copy_from_user(&ai_triginfo, (void __user *)argc, sizeof(ai_triginfo))) {
			err = -EFAULT;
			goto exit;
		}
		id = KDRV_DEV_ID(chip, kdrv_ai_trans_eng2id(ai_triginfo.engine), channel);
		er_return = kdrv_ai_waitdone(id, &ai_triginfo.trig_parm, NULL, NULL);
		if (er_return != E_OK) {
            err = -EFAULT;
            nvt_dbg(ERR, "AI_DRV: Error, kdrv_ai_waitdone failed.\r\n");
            goto AI_IOC_WAITDONE_EXIT;
        }

AI_IOC_WAITDONE_EXIT:

		break;

	// reset engine
	case AI_IOC_RESET:
		if (copy_from_user(&ai_triginfo, (void __user *)argc, sizeof(ai_triginfo))) {
			err = -EFAULT;
			goto exit;
		}
		id = KDRV_DEV_ID(chip, kdrv_ai_trans_eng2id(ai_triginfo.engine), channel);
		er_return = kdrv_ai_reset(id, &ai_triginfo.trig_parm);
		if (er_return != E_OK) {
            err = -EFAULT;
            nvt_dbg(ERR, "AI_DRV: Error, kdrv_ai_reset failed.\r\n");
            goto AI_IOC_RESET_EXIT;
        }

AI_IOC_RESET_EXIT:

		break;

	case AI_IOC_SET_MAP_ADDR:
		if (copy_from_user(&ai_map_mem_info, (void __user *)argc, sizeof(AI_DRV_MAP_MEMINFO))) {
			err = -EFAULT;
			goto exit;
		}
		memcpy(&g_ai_drv_map_mem[nvt_ai_chk_net_id(ai_map_mem_info.net_id)], &ai_map_mem_info.mem, sizeof(AI_DRV_MAP_MEM));
		break;

	case AI_IOC_GET_MAP_ADDR:
		if (copy_from_user(&ai_map_mem_info, (void __user *)argc, sizeof(AI_DRV_MAP_MEMINFO))) {
			err = -EFAULT;
			goto exit;
		}
		memcpy(&ai_map_mem_info.mem, &g_ai_drv_map_mem[nvt_ai_chk_net_id(ai_map_mem_info.net_id)], sizeof(AI_DRV_MAP_MEM));
		err = (copy_to_user((void __user *)argc, &ai_map_mem_info, sizeof(AI_DRV_MAP_MEMINFO))) ? (-EFAULT) : 0;
		break;
#if LL_SUPPORT_ROI
	case AI_IOC_SET_LL_USR_INFO: {	
		#if ROI_PARAM_GLOBAL
		SEM_WAIT(ai_ioctl_sem_id);
		#endif
		if (copy_from_user(&ai_ll_usr_info, (void __user *)argc, sizeof(AI_DRV_LL_USR_INFO))) {
			err = -EFAULT;
			goto exit;
		}
		er_return = kdrv_ai_link_ll_update_addr(&ai_ll_usr_info.layer_info, ai_ll_usr_info.net_id);
		if (er_return != E_OK) {
            err = -EFAULT;
            nvt_dbg(ERR, "AI_DRV: Error, kdrv_ai_link_ll_update_addr failed.\r\n");
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
		if (copy_from_user(&net_id, (void __user *)argc, sizeof(UINT32))) {
			err = -EFAULT;
			goto exit;
		}
		er_return = kdrv_ai_link_uninit(net_id);
		if (er_return != E_OK) {
            err = -EFAULT;
            nvt_dbg(ERR, "AI_DRV: Error, kdrv_ai_link_uninit failed.\r\n");
            goto AI_IOC_UNINIT_LINK_INFO_EXIT;
        }

AI_IOC_UNINIT_LINK_INFO_EXIT:

	}
		break;
#if LL_BUF_FROM_USR
	case AI_IOC_INIT_LL_BUF: {
		int ret = 0;
		#if ROI_PARAM_GLOBAL
		SEM_WAIT(ai_ioctl_sem_id);
		#endif
		if (copy_from_user(&ai_ll_buf_info, (void __user *)argc, sizeof(AI_DRV_LL_BUF_INFO))) {
			err = -EFAULT;
			printk("copy fail\n");
			goto exit;
		}
		// size, net id
		ret = nvt_fmem_mem_info_init(&g_buf_info, NVT_FMEM_ALLOC_NONCACHE, ai_ll_buf_info.size, NULL);
		if (ret >= 0) {
			g_buf_handle[ai_ll_buf_info.net_id] = fmem_alloc_from_cma(&g_buf_info, 0);
			if (g_buf_handle[ai_ll_buf_info.net_id] == NULL) {
				nvt_dbg(ERR, "get buffer fail\n");
				return -EINVAL;
			}
			net_ll_pa[ai_ll_buf_info.net_id] = (UINT32)g_buf_info.paddr; 
			net_ll_va[ai_ll_buf_info.net_id] = (UINT32)g_buf_info.vaddr; 
			net_ll_size[ai_ll_buf_info.net_id] = ai_ll_buf_info.size;
		} else {
			nvt_dbg(ERR, "init buffer fail\n");
			return -EINVAL;
		}
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
		if (copy_from_user(&net_id, (void __user *)argc, sizeof(UINT32))) {
			err = -EFAULT;
			goto exit;
		}
		if (g_buf_handle[net_id] == NULL) {
			nvt_dbg(ERR, "free buf: handle %d is null!\n", (int)net_id);
			return -1;
		} else {
			int ret = fmem_release_from_cma(g_buf_handle[net_id], 0);
			if (ret < 0) {
				nvt_dbg(ERR, "failed in release buffer\n");
				return -1;
			}
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
		KDRV_AI_FC_PARM input_parm = {0};
		NUE_PARM nue_parm = {0};
		UINT32 run_idx = 0;
		UINT32 run_time = 0;
		UINT32 tmp_h = 0;
#if AI_IOREMAP_IN_KERNEL
		UINT32 ioremap_size = 0;
#endif		
		if (copy_from_user(&input_parm, (void __user *)argc, sizeof(KDRV_AI_FC_PARM))) {
			err = -EFAULT;
			goto exit;
		}
		
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
		
#if AI_IOREMAP_IN_KERNEL
		// map va
		// input
		ioremap_size = input_parm.fc_ker.weight_w;
		nue_parm.dmaio_addr.addrin0 = (UINT32)ioremap(nue_parm.dmaio_addr.addrin0, PAGE_ALIGN(ioremap_size));
		// output
		ioremap_size = 4*input_parm.fc_ker.weight_h;
		input_parm.out_interm_addr = (UINT32)ioremap(input_parm.out_interm_addr, PAGE_ALIGN(ioremap_size));
		// weight
		ioremap_size = input_parm.fc_ker.weight_w*input_parm.fc_ker.weight_h;
		input_parm.fc_ker.weight_addr = (UINT32)ioremap(input_parm.fc_ker.weight_addr, PAGE_ALIGN(ioremap_size));
#endif		
		run_time = (nue_parm.insvm_size.sv_h / 256) + (((nue_parm.insvm_size.sv_h % 256) > 0)?1:0);
		tmp_h    = input_parm.fc_ker.weight_h;
		er_return = nue_open(NULL);
		if (er_return != E_OK) {
			err = -EFAULT;
			nvt_dbg(ERR, "AI_DRV: Error, nue_open failed.\r\n");
            goto NUE_IOC_RUN_FC_EXIT;
		}
		for (run_idx = 0; run_idx < run_time; run_idx++) {
			nue_parm.insvm_size.sv_h = (tmp_h < 256)?tmp_h:256;
			nue_parm.dmaio_addr.addr_out  = input_parm.out_interm_addr + run_idx*4*256;
			nue_parm.dmaio_addr.addrin_sv = input_parm.fc_ker.weight_addr + run_idx*nue_parm.insvm_size.sv_w*256;
			nue_parm.svm_parm.fcd_parm.fcd_enc_bit_length = 256*nue_parm.insvm_size.sv_h*8;
#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
			nvt_dbg(ERR, "AI_FASTBOOT: Error, NUE_IOC_RUN_FC can't be called at fastboot.\n");
#else
			if (nue_setmode(NUE_OPMODE_USERDEFINE, &nue_parm) == E_OK) {
				er_return = nue_start();
				if (er_return != E_OK) {
					err = -EFAULT;
					nvt_dbg(ERR, "AI_DRV: Error, nue_start failed.\r\n");
					goto NUE_IOC_RUN_FC_1_EXIT;
				}
				nue_wait_frameend(FALSE);
			}
			er_return = nue_pause();
			if (er_return != E_OK) {
				err = -EFAULT;
				nvt_dbg(ERR, "AI_DRV: Error, nue_pause failed.\r\n");
				goto NUE_IOC_RUN_FC_1_EXIT;
			}
#endif
			tmp_h -= nue_parm.insvm_size.sv_h;
		}

#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
NUE_IOC_RUN_FC_1_EXIT:
#endif
		er_return = nue_close();
		if (er_return != E_OK) {
            err = -EFAULT;
            nvt_dbg(ERR, "AI_DRV: Error, nue_close failed.\r\n");
            goto NUE_IOC_RUN_FC_EXIT;
        }
	
NUE_IOC_RUN_FC_EXIT:	
#if AI_IOREMAP_IN_KERNEL
		// unmap va
		// input
		iounmap((VOID *)nue_parm.dmaio_addr.addrin0);
		// output
		iounmap((VOID *)input_parm.out_interm_addr);
		// weight
		iounmap((VOID *)input_parm.fc_ker.weight_addr);
#endif	
	}
		break;
	case NUE_IOC_INIT:
		err = nue_init();
		if (err != 0) {
            err = -EFAULT;
            nvt_dbg(ERR, "AI_DRV: Error, nue_init failed.\r\n");
            goto NUE_IOC_INIT_EXIT;
        }

NUE_IOC_INIT_EXIT:

		break;
	
	case NUE_IOC_UNINIT:
		err = nue_uninit();
		if (err != 0) {
            err = -EFAULT;
            nvt_dbg(ERR, "AI_DRV: Error, nue_uninit failed.\r\n");
            goto NUE_IOC_UNINIT_EXIT;
        }

NUE_IOC_UNINIT_EXIT:

		break;
	
	case NUE2_IOC_RUN: {
		
		UINT32 func_en = 0;
		UINT32 func_idx = 0;

		if (copy_from_user(&g_nue2_input_parm, (void __user *)argc, sizeof(KDRV_AI_PREPROC_PARM))) {
			err = -EFAULT;
			goto exit;
		}
		
		for (func_idx = 0; func_idx < KDRV_AI_PREPROC_FUNC_CNT; func_idx++) {
			if (g_nue2_input_parm.func_list[func_idx] == 0) {
				break;
			}
			func_en |= g_nue2_input_parm.func_list[func_idx];
		}
		
		er_return = kdrv_ai_tran_nue2_preproc_parm(&g_nue2_input_parm, &g_nue2_parm, func_en);
		if (er_return != E_OK) {
            err = -EFAULT;
            nvt_dbg(ERR, "AI_DRV: Error, kdrv_ai_tran_nue2_preproc_parm failed.\r\n");
            goto NUE2_IOC_RUN_EXIT;
        }
		g_nue2_parm.dmaio_addr.dma_do_not_sync = 1;
		g_nue2_parm.dmaio_addr.is_pa = 1;
		er_return = nue2_open(NULL);
		if (er_return != E_OK) {
            err = -EFAULT;
            nvt_dbg(ERR, "AI_DRV: Error, nue2_open failed.\r\n");
            goto NUE2_IOC_RUN_EXIT;
        }
#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
		nvt_dbg(ERR, "AI_FASTBOOT: Error, NUE2_IOC_RUN can't be called at fastboot.\n");
#else
		if (nue2_setmode(NUE2_OPMODE_USERDEFINE, &g_nue2_parm) == E_OK) {
			er_return = nue2_start();
			if (er_return != E_OK) {
				err = -EFAULT;
				nvt_dbg(ERR, "AI_DRV: Error, nue2_start failed.\r\n");
				goto NUE2_IOC_RUN_EXIT;
			}
			//nue2_wait_frameend(FALSE);
		}
#endif
		//nue2_pause();
		
		//nue2_close();
	}

NUE2_IOC_RUN_EXIT:

		break;
	
	case NUE2_IOC_DONE: 
		nue2_wait_frameend(FALSE);
		er_return = nue2_pause();
		if (er_return != E_OK) {
            err = -EFAULT;
            nvt_dbg(ERR, "AI_DRV: Error, nue2_pause failed.\r\n");
            goto NUE2_IOC_DONE_EXIT;
        }
		er_return = nue2_close();
		if (er_return != E_OK) {
            err = -EFAULT;
            nvt_dbg(ERR, "AI_DRV: Error, nue2_close failed.\r\n");
            goto NUE2_IOC_DONE_EXIT;
        }

NUE2_IOC_DONE_EXIT:

		break;
	
	case NUE2_IOC_INIT:
		err = nue2_init();
		if (err != 0) {
			err = -EFAULT;
            nvt_dbg(ERR, "AI_DRV: Error, nue2_init failed.\r\n");
            goto NUE2_IOC_INIT_EXIT;
		}

NUE2_IOC_INIT_EXIT:

		break;
	
	case NUE2_IOC_UNINIT:
		err = nue2_uninit();
		if (err != 0) {
            err = -EFAULT;
            nvt_dbg(ERR, "AI_DRV: Error, nue2_uninit failed.\r\n");
            goto NUE2_IOC_UNINIT_EXIT;
        }

NUE2_IOC_UNINIT_EXIT:

		break;

	case AI_IOC_DMA_ABORT:
		if (copy_from_user(&ai_eng, (void __user *)argc, sizeof(KDRV_AI_ENG))) {
			err = -EFAULT;
			goto exit;
		}
		er_return = kdrv_ai_dma_abort(chip, AI_ENG_TOTAL);
		if (er_return != E_OK) {
            err = -EFAULT;
            nvt_dbg(ERR, "AI_DRV: Error, kdrv_ai_dma_abort failed.\r\n");
            goto AI_IOC_DMA_ABORT_EXIT;
        }

AI_IOC_DMA_ABORT_EXIT:

		break;
	
	default :
		nvt_dbg(ERR, "unknown ai ioctl cmd: %08x\r\n", cmd);
		break;
	}
exit:
	return err;
}
#endif //#if (KDRV_AI_MINI_FOR_FASTBOOT == 1)

irqreturn_t nvt_ai_drv_isr(int irq, void *devid)
{
	PAI_INFO pmodule_info = (PAI_INFO)devid;
	if (devid == NULL) {
		nvt_dbg(ERR, "invalid devid\r\n");
		return IRQ_NONE;
	}
	/* simple triggle and response mechanism*/
	complete(&pmodule_info->ai_completion);


	/*  Tasklet for bottom half mechanism */
	tasklet_schedule(&pmodule_info->ai_tasklet);

	if (irq == pmodule_info->iinterrupt_id[0]) {
		//DBG_IND("int-id: %d\r\n", pmodule_info->iinterrupt_id[0]);
#if (KDRV_AI_MINI_FOR_FASTBOOT == 1)
		if(kdrv_ai_nue_isr) {
			kdrv_ai_nue_isr();
		}
		nue_isr();
#else
		nue_isr();
#endif
	}
	
	if (irq == pmodule_info->iinterrupt_id[1]) {
		//DBG_IND("int-id: %d\r\n", pmodule_info->iinterrupt_id[1]);
#if (KDRV_AI_MINI_FOR_FASTBOOT == 1)
		if(kdrv_ai_nue2_isr) {
			kdrv_ai_nue2_isr();
		}
		nue2_isr();
#else
		nue2_isr();
#endif
	}

#if defined(_BSP_NA51089_)
	if (irq == pmodule_info->iinterrupt_id[2]) {
		//DBG_IND("int-id: %d\r\n", pmodule_info->iinterrupt_id[3]);
#if (KDRV_AI_MINI_FOR_FASTBOOT == 1)
		if(kdrv_ai_cnn_isr) {
			kdrv_ai_cnn_isr(0);
		}
		cnn_isr(0);
#else
		cnn_isr(0);
#endif
	}
#else	
	if (irq == pmodule_info->iinterrupt_id[2]) {
		//DBG_IND("int-id: %d\r\n", pmodule_info->iinterrupt_id[2]);
#if (KDRV_AI_MINI_FOR_FASTBOOT == 1)
		if(kdrv_ai_cnn_isr) {
			kdrv_ai_cnn_isr(1);
		}
		cnn_isr(1);
#else
		cnn_isr(1);
#endif
	}
	
	if (irq == pmodule_info->iinterrupt_id[3]) {
		//DBG_IND("int-id: %d\r\n", pmodule_info->iinterrupt_id[3]);
#if (KDRV_AI_MINI_FOR_FASTBOOT == 1)
		if(kdrv_ai_cnn_isr) {
			kdrv_ai_cnn_isr(0);
		}
		cnn_isr(0);
#else
		cnn_isr(0);
#endif
	}
#endif

	return IRQ_HANDLED;
}

int nvt_ai_drv_wait_cmd_complete(PAI_INFO pmodule_info)
{
	if (pmodule_info == NULL) {
		nvt_dbg(ERR, "invalid pmodule_info\r\n");
		return -EINVAL;
	}
	
	wait_for_completion(&pmodule_info->ai_completion);
	return 0;
}

int nvt_ai_drv_write_reg(PAI_INFO pmodule_info, unsigned long addr, unsigned long value)
{
	if (pmodule_info == NULL) {
		nvt_dbg(ERR, "invalid pmodule_info\r\n");
		return -EINVAL;
	}
	
	WRITE_REG(value, pmodule_info->io_addr[0] + addr);
	return 0;
}

int nvt_ai_drv_read_reg(PAI_INFO pmodule_info, unsigned long addr)
{
	if (pmodule_info == NULL) {
		nvt_dbg(ERR, "invalid pmodule_info\r\n");
		return -EINVAL;
	}
	
	return READ_REG(pmodule_info->io_addr[0] + addr);
}

void nvt_ai_drv_do_tasklet(unsigned long data)
{
	PAI_INFO pmodule_info = (PAI_INFO)data;

	if (pmodule_info == NULL) {
		nvt_dbg(ERR, "invalid pmodule_info\r\n");
		return;
	}
	//nvt_dbg(IND, "\n");

	/* do something you want*/
	complete(&pmodule_info->ai_completion);
}

INT32 nvt_ai_is_do_ai_init(VOID)
{
    int ret = 0;

    if (is_ai_enable_dtsi == 1) {
        if (do_ai_init == 1) {
            ret = 1;
        } else {
            ret = 0;
        }
    } else {
#if (KDRV_AI_MINI_FOR_FASTBOOT == 1)
		if (kdrv_builtin_is_fastboot()) {
        	ret = 1;
		} else {
			ret = 0;
		}
#else
		if (kdrv_builtin_is_fastboot()) {
        	ret = 0;
		} else {
			ret = 1;
		}
#endif
    }

    return ret;
}

EXPORT_SYMBOL(nvt_ai_is_do_ai_init);
#if (KDRV_AI_MINI_FOR_FASTBOOT == 1)
EXPORT_SYMBOL(kdrv_ai_reg_nue_isr_cb);
EXPORT_SYMBOL(kdrv_ai_reg_nue2_isr_cb);
EXPORT_SYMBOL(kdrv_ai_reg_cnn_isr_cb);
#endif
