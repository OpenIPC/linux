#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include "kwrap/type.h"
#include "kwrap/debug.h"
#include "sie_drv.h"
#include "sie_lib.h"
#include "sie_int.h"
#include "sie_platform.h"

/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/
int nvt_sie_drv_wait_cmd_complete(PSIE_MODULE_INFO pmodule_info);
void nvt_sie_drv_do_tasklet(unsigned long data);
irqreturn_t nvt_sie_drv_isr(int irq, void *devid);
irqreturn_t nvt_sie_drv_isr2(int irq, void *devid);
irqreturn_t nvt_sie_drv_isr3(int irq, void *devid);
/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/
typedef irqreturn_t (*irq_handler_t)(int, void *);

/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/
//int iEventFlag = 0;

//static UINT32 uiTmpPhyBufAddr = 0x08000000, uiTmpBufSz = 16000000, uiGbl_non_cache_base;
/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int nvt_sie_drv_open(PSIE_MODULE_INFO pmodule_info, unsigned char ucIF)
{
	/* Add HW Moduel initial operation here when the device file opened*/
	return E_OK;
}


int nvt_sie_drv_release(PSIE_MODULE_INFO pmodule_info, unsigned char ucIF)
{
	sie_platform_release_resource();
	/* Add HW Moduel release operation here when device file closed */
	return E_OK;
}

int nvt_sie_drv_init(SIE_MODULE_INFO* pmodule_info, unsigned char ucIF)
{
	int iRet = 0;

	init_waitqueue_head(&pmodule_info->sie_wait_queue);
	vk_spin_lock_init(&pmodule_info->sie_spinlock);
	vk_sema_init(&pmodule_info->sie_sem, 1);
	init_completion(&pmodule_info->sie_completion);
	tasklet_init(&pmodule_info->sie_tasklet, nvt_sie_drv_do_tasklet, (unsigned long)pmodule_info);

	/* register IRQ here*/
	if(request_irq(pmodule_info->iinterrupt_id[0], nvt_sie_drv_isr, IRQF_TRIGGER_HIGH, "SIE_INT", pmodule_info)) {
		nvt_dbg(ERR, "failed to register an IRQ IF:%d Int:%d\n", 0, pmodule_info->iinterrupt_id[0]);
		iRet = -ENODEV;
		goto FAIL_FREE_IRQ;
	}
	if(request_irq(pmodule_info->iinterrupt_id[1], nvt_sie_drv_isr2, IRQF_TRIGGER_HIGH, "SIE2_INT", pmodule_info)) {
		nvt_dbg(ERR, "failed to register an IRQ IF:%d Int:%d\n", 1, pmodule_info->iinterrupt_id[1]);
		iRet = -ENODEV;
		goto FAIL_FREE_IRQ;
	}
	if(request_irq(pmodule_info->iinterrupt_id[2], nvt_sie_drv_isr3, IRQF_TRIGGER_HIGH, "SIE3_INT", pmodule_info)) {
		nvt_dbg(ERR, "failed to register an IRQ IF:%d Int:%d\n", 2, pmodule_info->iinterrupt_id[2]);
		iRet = -ENODEV;
		goto FAIL_FREE_IRQ;
	}

	sie_platform_create_resource(pmodule_info);
    sie_platform_prepare_clk(SIE_ENGINE_ID_1);
	sie_platform_prepare_clk(SIE_ENGINE_ID_2);
	sie_platform_prepare_clk(SIE_ENGINE_ID_3);

	return iRet;

FAIL_FREE_IRQ:

	free_irq(pmodule_info->iinterrupt_id[0], pmodule_info);

	/* Add error handler here */

	return iRet;
}
int nvt_sie_drv_remove(SIE_MODULE_INFO* pmodule_info)
{

	//Free IRQ
	free_irq(pmodule_info->iinterrupt_id[0], pmodule_info);

	/* Add HW Moduel release operation here*/

	return E_OK;
}

int nvt_sie_drv_suspend(SIE_MODULE_INFO* pmodule_info)
{
	/* Add suspend operation here*/

	return E_OK;
}

int nvt_sie_drv_resume(SIE_MODULE_INFO* pmodule_info)
{
	/* Add resume operation here*/

	return E_OK;
}

irqreturn_t nvt_sie_drv_isr(int irq, void *devid)
{
	PSIE_MODULE_INFO pmodule_info = (PSIE_MODULE_INFO)devid;

	/* simple triggle and response mechanism*/
	complete(&pmodule_info->sie_completion);


	/*  Tasklet for bottom half mechanism */
	tasklet_schedule(&pmodule_info->sie_tasklet);

	sie_isr(SIE_ENGINE_ID_1);
    return IRQ_HANDLED;
}
irqreturn_t nvt_sie_drv_isr2(int irq, void *devid)
{
	sie_isr(SIE_ENGINE_ID_2);
    return IRQ_HANDLED;
}
irqreturn_t nvt_sie_drv_isr3(int irq, void *devid)
{
	sie_isr(SIE_ENGINE_ID_3);
    return IRQ_HANDLED;
}
int nvt_sie_drv_wait_cmd_complete(PSIE_MODULE_INFO pmodule_info)
{
	wait_for_completion(&pmodule_info->sie_completion);
	return 0;
}

int nvt_sie_drv_write_reg(PSIE_MODULE_INFO pmodule_info, unsigned long addr, unsigned long value)
{
	WRITE_REG(value, pmodule_info->io_addr[0] + addr);
	return E_OK;
}

int nvt_sie_drv_read_reg(PSIE_MODULE_INFO pmodule_info, unsigned long addr)
{
	return READ_REG(pmodule_info->io_addr[0] + addr);
}

void nvt_sie_drv_do_tasklet(unsigned long data)
{
	PSIE_MODULE_INFO pmodule_info = (PSIE_MODULE_INFO)data;
	//nvt_dbg(INFO, "\n");

	/* do something you want*/
	complete(&pmodule_info->sie_completion);
}
