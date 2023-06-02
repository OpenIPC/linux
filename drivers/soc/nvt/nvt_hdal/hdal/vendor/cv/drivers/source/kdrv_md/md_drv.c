#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include "kwrap/type.h"
#include "md_drv.h"
#include "md_ioctl.h"
//#include "md_dbg.h"
#include "mdbc_lib.h"
#include "kdrv_md_int.h"
#include "md_platform.h"


/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/
int nvt_md_drv_wait_cmd_complete(PMD_MODULE_INFO pmodule_info);
int nvt_md_drv_ioctl(unsigned char ucIF, MD_MODULE_INFO* pmodule_info, unsigned int cmd, unsigned long arg);
void nvt_md_drv_do_tasklet(unsigned long data);
irqreturn_t nvt_md_drv_isr(int irq, void *devid);
/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/
typedef irqreturn_t (*irq_handler_t)(int, void *);

/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/
//int iEventFlag = 0;

extern void mdbc_isr(void);
//struct clk *mdbc_clk[MD_CLK_NUM];

//static UINT32 uiTmpPhyBufAddr = 0x08000000, uiTmpBufSz = 16000000, uiGbl_non_cache_base;
/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int nvt_md_drv_open(PMD_MODULE_INFO pmodule_info, unsigned char ucIF)
{
	//nvt_dbg(IND, "%d\n", ucIF);

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}


int nvt_md_drv_release(PMD_MODULE_INFO pmodule_info, unsigned char ucIF)
{
	//int i = 0;
	//nvt_dbg(IND, "%d\n", ucIF);

	/* Add HW Moduel release operation here when device file closed */

	return 0;
}

int nvt_md_drv_init(MD_MODULE_INFO* pmodule_info)
{
	int iRet = 0;
	int i = 0;
    //struct clk *pclk;

    //nvt_dbg(IND, "1\r\n");

	init_waitqueue_head(&pmodule_info->md_wait_queue);
	vk_spin_lock_init(&pmodule_info->md_spinlock);
	vk_sema_init(&pmodule_info->md_sem, 1);
	init_completion(&pmodule_info->md_completion);
	tasklet_init(&pmodule_info->md_tasklet, nvt_md_drv_do_tasklet, (unsigned long)pmodule_info);

    md_platform_create_resource(pmodule_info,md_freq_from_dtsi[0]);
    md_platform_prepare_clk();

	/* initial clock here */
    /*
	for (i = 0; i < MD_CLK_NUM; i++) {
        pclk = clk_get(NULL, "pll13");
    	if (IS_ERR(pclk)) {
    		printk("%s: get source pll13 fail\r\n", __func__);
    	}
	    clk_set_parent(pmodule_info->pclk[i], pclk);
		clk_prepare(pmodule_info->pclk[i]);
		clk_enable(pmodule_info->pclk[i]);
        clk_put(pclk);
        mdbc_clk[i] = pmodule_info->pclk[i];
	}
    */
	//clk_prepare(pmodule_info->pclk[0]);
	//clk_enable(pmodule_info->pclk[0]);
	//clk_set_rate(pmodule_info->pclk[0], 240000000);

	/* register IRQ here*/
	for (i = 0; i < MD_IRQ_NUM; i++) {
		if(request_irq(pmodule_info->iinterrupt_id[i], nvt_md_drv_isr, IRQF_TRIGGER_HIGH, "MD_INT", pmodule_info)) {
			nvt_dbg(ERR, "failed to register an IRQ Int:%d\n", pmodule_info->iinterrupt_id[i]);
			iRet = -ENODEV;
			free_irq(pmodule_info->iinterrupt_id[i], pmodule_info);
			goto FAIL_FREE_IRQ;
		}
	}

	//nvt_dbg(IND, "3\r\n");

	/* Add HW Module initialization here when driver loaded */
	//mdbc_setBaseAddr((UINT32)pmodule_info->io_addr[0]);
	//mdbc_create_resource();

	kdrv_md_install_id();
	kdrv_md_init();

	return iRet;

FAIL_FREE_IRQ:
	for (i = 0; i < MD_IRQ_NUM; i++) {
		free_irq(pmodule_info->iinterrupt_id[i], pmodule_info);
	}
	//free_irq(pmodule_info->iinterrupt_id[0], pmodule_info);

	/* Add error handler here */

	return iRet;
}
int nvt_md_drv_remove(MD_MODULE_INFO* pmodule_info)
{
	int i = 0;
	//Free IRQ
	for (i = 0; i < MD_IRQ_NUM; i++) {
		free_irq(pmodule_info->iinterrupt_id[i], pmodule_info);
	}
	/* Add HW Moduel release operation here*/
	md_platform_unprepare_clk();
	md_platform_release_resource();
	kdrv_md_uninstall_id();
	return 0;
}

int nvt_md_drv_suspend(MD_MODULE_INFO* pmodule_info)
{
	nvt_dbg(IND, "\n");

	/* Add suspend operation here*/

	return 0;
}

int nvt_md_drv_resume(MD_MODULE_INFO* pmodule_info)
{
	nvt_dbg(IND, "\n");
	/* Add resume operation here*/

	return 0;
}

int nvt_md_drv_ioctl(unsigned char ucIF, MD_MODULE_INFO* pmodule_info, unsigned int uiCmd, unsigned long ulArg)
{
	REG_INFO reg_info;
	REG_INFO_LIST reg_info_list;
	int iLoop;
	int iRet = 0;
	KDRV_MD_OPENCFG      md_cfg_data = {0};
	KDRV_MD_PARAM        md_param = {0};
    KDRV_MD_TRIGGER_PARAM md_trigger_param = {0};
	KDRV_MD_REG_DATA     md_reg = {0};
	KDRV_MDBC_ABORT			 md_dma_abort = KDRV_MDBC_DMA_ENABLE;
    const UINT32 chip = 0, engine = KDRV_CV_ENGINE_MD, channel = 0;
	static UINT32 id = 0;

	nvt_dbg(IND, "IF-%d cmd:%x\n", ucIF, uiCmd);

	switch(uiCmd) {
		case MD_IOC_START:
			/*call someone to start operation*/
			break;

		case MD_IOC_STOP:
			/*call someone to stop operation*/
			break;

		case MD_IOC_READ_REG:
			iRet = copy_from_user(&reg_info, (void __user *)ulArg, sizeof(REG_INFO));
	        if(!iRet) {
		        reg_info.uiValue = READ_REG(pmodule_info->io_addr[ucIF] + reg_info.uiAddr);
        	 	iRet = copy_to_user((void __user *)ulArg, &reg_info, sizeof(REG_INFO));
			}
			break;

		case MD_IOC_WRITE_REG:
			iRet = copy_from_user(&reg_info, (void __user *)ulArg, sizeof(REG_INFO));
	        if(!iRet)
				WRITE_REG(reg_info.uiValue, pmodule_info->io_addr[ucIF] + reg_info.uiAddr);
			break;

		case MD_IOC_READ_REG_LIST:
			iRet = copy_from_user(&reg_info_list, (void __user *)ulArg, sizeof(REG_INFO_LIST));
	        if(!iRet) {
                if (reg_info_list.uiCount < MODULE_REG_LIST_NUM) {
    				for(iLoop = 0 ; iLoop < reg_info_list.uiCount; iLoop++)
    			        reg_info_list.RegList[iLoop].uiValue = READ_REG(pmodule_info->io_addr[ucIF] + reg_info_list.RegList[iLoop].uiAddr);
                }
        	 	iRet = copy_to_user((void __user *)ulArg, &reg_info_list, sizeof(REG_INFO_LIST));
			}
			break;
		case MD_IOC_WRITE_REG_LIST:
			iRet = copy_from_user(&reg_info_list, (void __user *)ulArg, sizeof(REG_INFO_LIST));
	        if(!iRet)
	        {
                if (reg_info_list.uiCount < MODULE_REG_LIST_NUM) {
    				for(iLoop = 0 ; iLoop < reg_info_list.uiCount ; iLoop++)
    					WRITE_REG(reg_info_list.RegList[iLoop].uiValue, pmodule_info->io_addr[ucIF] + reg_info_list.RegList[iLoop].uiAddr);
                }
	        }
			break;

		/* Add other operations here */
		// OPEN
		case MD_IOC_OPEN:
			kdrv_md_open(chip, engine);
            id = KDRV_DEV_ID(chip, engine, channel);
			break;
		// CLOSE
		case MD_IOC_CLOSE:
			kdrv_md_close(chip, engine);
			break;
		// OPENCFG
		case MD_IOC_OPENCFG:
			iRet = copy_from_user(&md_cfg_data, (void __user *)ulArg, sizeof(md_cfg_data));
			if (!iRet) {
                kdrv_md_set(id, KDRV_MD_PARAM_OPENCFG, &md_cfg_data);
			}
			break;
		// SET MODE
		case MD_IOC_SET_PARAM:
			iRet = copy_from_user(&md_param, (void __user *)ulArg, sizeof(md_param));
			if (!iRet) {
				kdrv_md_set(id, KDRV_MD_PARAM_ALL, &md_param);
			}
			break;
		// GET MODE
		case MD_IOC_GET_PARAM:
			kdrv_md_get(id, KDRV_MD_PARAM_ALL, &md_param);
			iRet = (copy_to_user((void __user *)ulArg, &md_param, sizeof(md_param))) ? (-EFAULT) : 0;
			break;
		// trigger engine
		case MD_IOC_TRIGGER:
            iRet = copy_from_user(&md_trigger_param, (void __user *)ulArg, sizeof(md_trigger_param));
            if (!iRet) {
			    kdrv_md_trigger(id, &md_trigger_param, NULL, NULL);
            }
			break;
		case MD_IOC_GET_REG:
            kdrv_md_get(id, KDRV_MD_PARAM_GET_REG, &md_reg);
			iRet = (copy_to_user((void __user *)ulArg, &md_reg, sizeof(md_reg))) ? (-EFAULT) : 0;
			break;	
		//DMA DISABLE
		case MD_IOC_SET_DMA_ABORT:
			//printk("MD_IOC_SET_DMA_ABORT\n\r");
			if (copy_from_user(&md_dma_abort, (void __user *)ulArg, sizeof(md_dma_abort))) {
				iRet = -EFAULT;
			}
			kdrv_md_set(0, KDRV_MD_DMA_ABORT, &md_dma_abort);
			break;
		case MD_IOC_GET_DMA_ABORT:
			kdrv_md_get(0, KDRV_MD_DMA_ABORT, &md_dma_abort);
			//printk("MD_IOC_GET_DMA_ABORT, %d\n\r", md_dma_abort);
			iRet = (copy_to_user((void __user *)ulArg, &md_dma_abort, sizeof(md_dma_abort))) ? (-EFAULT) : 0;
			break;	
	}

	return iRet;
}

irqreturn_t nvt_md_drv_isr(int irq, void *devid)
{
	PMD_MODULE_INFO pmodule_info = (PMD_MODULE_INFO)devid;

	/* simple triggle and response mechanism*/
	complete(&pmodule_info->md_completion);


	/*  Tasklet for bottom half mechanism */
	tasklet_schedule(&pmodule_info->md_tasklet);

	if (irq == pmodule_info->iinterrupt_id[0]) {
		mdbc_isr();
	}

    return IRQ_HANDLED;
}

int nvt_md_drv_wait_cmd_complete(PMD_MODULE_INFO pmodule_info)
{
	wait_for_completion(&pmodule_info->md_completion);
	return 0;
}

int nvt_md_drv_write_reg(PMD_MODULE_INFO pmodule_info, unsigned long addr, unsigned long value)
{
	WRITE_REG(value, pmodule_info->io_addr[0] + addr);
	return 0;
}

int nvt_md_drv_read_reg(PMD_MODULE_INFO pmodule_info, unsigned long addr)
{
	return READ_REG(pmodule_info->io_addr[0] + addr);
}

void nvt_md_drv_do_tasklet(unsigned long data)
{
	PMD_MODULE_INFO pmodule_info = (PMD_MODULE_INFO)data;
	//nvt_dbg(INFO, "\n");

	/* do something you want*/
	complete(&pmodule_info->md_completion);
}
