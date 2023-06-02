#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include "kwrap/type.h"
#include "dis_drv.h"
#include "dis_ioctl.h"
#include "dis_dbg.h"
#include "dis_lib.h"
#include "kdrv_dis_int.h"
#include "kdrv_eth.h"


/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/
int nvt_dis_drv_wait_cmd_complete(PDIS_MODULE_INFO pmodule_info);
int nvt_dis_drv_ioctl(unsigned char ucIF, DIS_MODULE_INFO *p_module_info, unsigned int cmd, unsigned long arg);
void nvt_dis_drv_do_tasklet(unsigned long data);
irqreturn_t nvt_dis_drv_isr(int irq, void *p_devid);

/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/
typedef irqreturn_t (*irq_handler_t)(int, void *);

/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/
extern void dis_isr(void);


/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int nvt_dis_drv_open(PDIS_MODULE_INFO pmodule_info, unsigned char ucIF)
{

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}


int nvt_dis_drv_release(PDIS_MODULE_INFO pmodule_info, unsigned char ucIF)
{

	/* Add HW Moduel release operation here when device file closed */
	
	return 0;
}

int nvt_dis_drv_init(DIS_MODULE_INFO *p_module_info)
{
	int i = 0, iRet = 0;

	if (p_module_info == NULL) {		
		nvt_dbg(ERR, "invalid input\r\n");		
		return -EINVAL;	
	}

	init_waitqueue_head(&p_module_info->dis_wait_queue);
	vk_spin_lock_init(&p_module_info->dis_spinlock);
	vk_sema_init(&p_module_info->dis_sem, 1);
	init_completion(&p_module_info->dis_completion);
	tasklet_init(&p_module_info->dis_tasklet, nvt_dis_drv_do_tasklet, (unsigned long)p_module_info);

	/* initial clock here */
	/*for (i = 0; i < MODULE_CLK_NUM; i++) {
		if (IS_ERR(p_module_info->p_clk[i])) {
			DBG_IND("====dis%d-clk_err...\r\n", i);
		} else {
			//clk_prepare(p_module_info->p_clk[i]);
			//clk_enable(p_module_info->p_clk[i]);
			clk_prepare(dis_clk[i]);
			//clk_enable(dis_clk[i]);
			//DBG_IND("clk %d is enable\r\n", i);
		}
	}*/
	

	/* register IRQ here*/
	for (i = 0; i < MODULE_IRQ_NUM; i++) {
		if (request_irq(p_module_info->iinterrupt_id[i], nvt_dis_drv_isr, IRQF_TRIGGER_HIGH, "DIS_INT", p_module_info)) {
			nvt_dbg(ERR, "failed to register an IRQ Int:%d\n", p_module_info->iinterrupt_id[i]);
			iRet = -ENODEV;
			goto FAIL_FREE_IRQ;
		}
	}

	/* Add HW Module initialization here when driver loaded */
	dis_setBaseAddr((UINT32)p_module_info->io_addr[0]);
	dis_create_resource();


	kdrv_dis_install_id();
	kdrv_dis_init();


	return iRet;

FAIL_FREE_IRQ:

	free_irq(p_module_info->iinterrupt_id[0], p_module_info);

	/* Add error handler here */

	return iRet;
}

int nvt_dis_drv_remove(DIS_MODULE_INFO *p_module_info)
{
	UINT32 i = 0;
	if (p_module_info == NULL) {
		nvt_dbg(ERR, "invalid input\r\n");
		return -EINVAL;
	}

	//Free IRQ
	for (i = 0; i < MODULE_IRQ_NUM; i++) {
		free_irq(p_module_info->iinterrupt_id[0], p_module_info);
	}

	/* Add HW Moduel release operation here*/
	dis_release_resource();

	kdrv_dis_uninstall_id();

	return 0;
}

int nvt_dis_drv_suspend(DIS_MODULE_INFO *p_module_info)
{
	if (p_module_info == NULL) {
		nvt_dbg(ERR, "invalid input\r\n");
		return -EINVAL;
	}

	/* Add suspend operation here*/

	return 0;
}

int nvt_dis_drv_resume(DIS_MODULE_INFO *p_module_info)
{
	if (p_module_info == NULL) {
		nvt_dbg(ERR, "invalid input\r\n");
		return -EINVAL;
	}
	/* Add resume operation here*/

	return 0;
}
int nvt_dis_drv_ioctl(unsigned char ucIF, DIS_MODULE_INFO *p_module_info, unsigned int uiCmd, unsigned long ulArg)
{
	REG_INFO reg_info;
	REG_INFO_LIST reg_info_list;
	int iLoop;
	int iRet = 0;
	KDRV_DIS_OPENCFG                  dis_cfg_data            = {0};
	KDRV_DIS_IN_IMG_INFO              dis_imginfo_data        = {0};
	KDRV_DIS_IN_DMA_INFO              dis_input_addr          = {0};
	KDRV_DIS_OUT_DMA_INFO             dis_output_addr         = {0};
	KDRV_MV_OUT_DMA_INFO              dis_out_mv              = {0};
	UINT32                            dis_int_en              = 0;
	KDRV_DIS_TRIGGER_PARAM            dis_trigger_param       = {0};
	KDRV_MDS_DIM                      dis_mdsdim              = {0};
	KDRV_BLOCKS_DIM                   dis_blksz               = KDRV_DIS_BLKSZ_MAX;

	KDRV_ETH_IN_PARAM                 dis_eth_param           = {0};
	KDRV_ETH_IN_BUFFER_INFO           dis_eth_dma             = {0};
	KDRV_ETH_OUT_PARAM                dis_eth_param_out       = {0};
	DIS_ABORT					   dis_dma_abort           = DIS_DMA_ENABLE;

	nvt_dbg(IND, "IF-%d cmd:%x\n", ucIF, uiCmd);

	if (p_module_info == NULL) {
		nvt_dbg(ERR, "invalid input\r\n");
		return -EINVAL;
	}

	switch (uiCmd) {
	case DIS_IOC_START:
		/*call someone to start operation*/
		break;

	case DIS_IOC_STOP:
		/*call someone to stop operation*/
		break;

	case DIS_IOC_READ_REG:
		iRet = copy_from_user(&reg_info, (void __user *)ulArg, sizeof(REG_INFO));
		if (!iRet) {
			reg_info.uiValue = READ_REG(p_module_info->io_addr[ucIF] + reg_info.uiAddr);
			iRet = copy_to_user((void __user *)ulArg, &reg_info, sizeof(REG_INFO));
		}
		break;

	case DIS_IOC_WRITE_REG:
		iRet = copy_from_user(&reg_info, (void __user *)ulArg, sizeof(REG_INFO));
		if (!iRet) {
			WRITE_REG(reg_info.uiValue, p_module_info->io_addr[ucIF] + reg_info.uiAddr);
		}
		break;

	case DIS_IOC_READ_REG_LIST:
		iRet = copy_from_user(&reg_info_list, (void __user *)ulArg, sizeof(REG_INFO_LIST));
		if (!iRet) {
			if (reg_info_list.uiCount < MODULE_REG_LIST_NUM) {
				for (iLoop = 0 ; iLoop < reg_info_list.uiCount; iLoop++) 
					reg_info_list.RegList[iLoop].uiValue = READ_REG(p_module_info->io_addr[ucIF] + reg_info_list.RegList[iLoop].uiAddr);
			}
			iRet = copy_to_user((void __user *)ulArg, &reg_info_list, sizeof(REG_INFO_LIST));
		}
		break;
	case DIS_IOC_WRITE_REG_LIST:
		iRet = copy_from_user(&reg_info_list, (void __user *)ulArg, sizeof(REG_INFO_LIST));
		if (!iRet) {
			if (reg_info_list.uiCount <= MODULE_REG_LIST_NUM) {
				for(iLoop = 0 ; iLoop < reg_info_list.uiCount ; iLoop++)
					WRITE_REG(reg_info_list.RegList[iLoop].uiValue, p_module_info->io_addr[ucIF] + reg_info_list.RegList[iLoop].uiAddr);
			}
		}
		break;
		
	/* Add other operations here */
		
	// OPEN
	case DIS_IOC_OPEN:
		kdrv_dis_open(0, 0);
		break;
	// CLOSE
	case DIS_IOC_CLOSE:
		kdrv_dis_close(0, 0);
		break;
	// OPENCFG
	case DIS_IOC_OPENCFG:
		if (copy_from_user(&dis_cfg_data, (void __user *)ulArg, sizeof(dis_cfg_data))) {
			iRet = -EFAULT;
			goto exit;
		}
		kdrv_dis_set(0, KDRV_DIS_PARAM_OPENCFG, &dis_cfg_data);
		break;
	// INPUT IMG INFO
	case DIS_IOC_SET_IMG_INFO:
		if (copy_from_user(&dis_imginfo_data, (void __user *)ulArg, sizeof(dis_imginfo_data))) {
			iRet = -EFAULT;
			goto exit;
		}
		kdrv_dis_set(0, KDRV_DIS_PARAM_IN_IMG, &dis_imginfo_data);
		break;
	case DIS_IOC_GET_IMG_INFO:
		kdrv_dis_get(0, KDRV_DIS_PARAM_IN_IMG, &dis_imginfo_data);
		iRet = (copy_to_user((void __user *)ulArg, &dis_imginfo_data, sizeof(dis_imginfo_data))) ? (-EFAULT) : 0;
		break;
	// DMA IN
	case DIS_IOC_SET_IMG_DMA_IN:
		if (copy_from_user(&dis_input_addr, (void __user *)ulArg, sizeof(dis_input_addr))) {
			iRet = -EFAULT;
			goto exit;
		}
		kdrv_dis_set(0, KDRV_DIS_PARAM_DMA_IN, &dis_input_addr);
		break;
	case DIS_IOC_GET_IMG_DMA_IN:
		kdrv_dis_get(0, KDRV_DIS_PARAM_DMA_IN, &dis_input_addr);
		iRet = (copy_to_user((void __user *)ulArg, &dis_input_addr, sizeof(dis_input_addr))) ? (-EFAULT) : 0;
		break;
	// DMA_OUT
	case DIS_IOC_SET_IMG_DMA_OUT:
		if (copy_from_user(&dis_output_addr, (void __user *)ulArg, sizeof(dis_output_addr))) {
			iRet = -EFAULT;
			goto exit;
		}
		kdrv_dis_set(0, KDRV_DIS_PARAM_DMA_OUT, &dis_output_addr);
		break;
	case DIS_IOC_GET_IMG_DMA_OUT:
		kdrv_dis_get(0, KDRV_DIS_PARAM_DMA_OUT, &dis_output_addr);
		iRet = (copy_to_user((void __user *)ulArg, &dis_output_addr, sizeof(dis_output_addr))) ? (-EFAULT) : 0;
		break;
	// INT_EN_IN
	case DIS_IOC_SET_INT_EN:
		if (copy_from_user(&dis_int_en, (void __user *)ulArg, sizeof(dis_int_en))) {
			iRet = -EFAULT;
			goto exit;
		}
		kdrv_dis_set(0, KDRV_DIS_PARAM_INT_EN, &dis_int_en);
		break;
	case DIS_IOC_GET_INT_EN:
		kdrv_dis_get(0, KDRV_DIS_PARAM_INT_EN, &dis_int_en);
		iRet = (copy_to_user((void __user *)ulArg, &dis_int_en, sizeof(dis_int_en))) ? (-EFAULT) : 0;
		break;
	// MV_OUT
	case DIS_IOC_GET_MV_MAP_OUT:
		if (copy_from_user(&dis_out_mv, (void __user *)ulArg, sizeof(dis_out_mv))) {
			iRet = -EFAULT;
			goto exit;
		}
		kdrv_dis_get(0, KDRV_DIS_PARAM_MV_OUT, &dis_out_mv);
		break;	
	//MDS_DIM
	case DIS_IOC_GET_MV_MDS_DIM:
		kdrv_dis_get(0, KDRV_DIS_PARAM_MV_DIM, &dis_mdsdim);
		iRet = (copy_to_user((void __user *)ulArg, &dis_mdsdim, sizeof(dis_mdsdim))) ? (-EFAULT) : 0;
		break;	
	//BLOCK_DIM
	case DIS_IOC_SET_MV_BLOCKS_DIM:
		if (copy_from_user(&dis_blksz, (void __user *)ulArg, sizeof(dis_blksz))) {
			iRet = -EFAULT;
			goto exit;
		}
		kdrv_dis_set(0, KDRV_DIS_PARAM_BLOCK_DIM, &dis_blksz);
		break;
	case DIS_IOC_GET_MV_BLOCKS_DIM:
		kdrv_dis_get(0, KDRV_DIS_PARAM_BLOCK_DIM, &dis_blksz);
		iRet = (copy_to_user((void __user *)ulArg, &dis_blksz, sizeof(dis_blksz))) ? (-EFAULT) : 0;
		break;
	//trigger engine
	case DIS_IOC_TRIGGER:
		if (copy_from_user(&dis_trigger_param, (void __user *)ulArg, sizeof(dis_trigger_param))) {
			iRet = -EFAULT;
			goto exit;
		}
		kdrv_dis_trigger(0, &dis_trigger_param, NULL, NULL);
		break;

	//ETH IN_PARAM
	case ETH_IOC_SET_ETH_PARAM_IN:
		if (copy_from_user(&dis_eth_param, (void __user *)ulArg, sizeof(dis_eth_param))) {
			iRet = -EFAULT;
			goto exit;
		}
		kdrv_dis_set(0, KDRV_DIS_ETH_PARAM_IN, &dis_eth_param);
		break;	
	//ETH IN_PARAM
	case ETH_IOC_GET_ETH_PARAM_IN:
		kdrv_dis_get(0, KDRV_DIS_ETH_PARAM_IN, &dis_eth_param);
		iRet = (copy_to_user((void __user *)ulArg, &dis_eth_param, sizeof(dis_eth_param))) ? (-EFAULT) : 0;
		break;
	//ETH IN_BUFFER
	case ETH_IOC_SET_ETH_BUFFER:
		if (copy_from_user(&dis_eth_dma, (void __user *)ulArg, sizeof(dis_eth_dma))) {
			iRet = -EFAULT;
			goto exit;
		}
		kdrv_dis_set(0, KDRV_DIS_ETH_BUFFER_IN, &dis_eth_dma);
		break;	
	//ETH IN_BUFFER
	case ETH_IOC_GET_ETH_BUFFER:
		kdrv_dis_get(0, KDRV_DIS_ETH_BUFFER_IN, &dis_eth_dma);
		iRet = (copy_to_user((void __user *)ulArg, &dis_eth_dma, sizeof(dis_eth_dma))) ? (-EFAULT) : 0;
		break;
	//ETH OUT_PARAM
	case ETH_IOC_GET_ETH_PARAM_OUT:
		kdrv_dis_get(0, KDRV_DIS_ETH_PARAM_OUT, &dis_eth_param_out);
		iRet = (copy_to_user((void __user *)ulArg, &dis_eth_param_out, sizeof(dis_eth_param_out))) ? (-EFAULT) : 0;
		break;
	//DMA DISABLE
	case DIS_IOC_SET_DMA_ABORT:
		//printk("DIS_IOC_SET_DMA_ABORT\n\r");
		if (copy_from_user(&dis_dma_abort, (void __user *)ulArg, sizeof(dis_dma_abort))) {
			iRet = -EFAULT;
			goto exit;
		}
		kdrv_dis_set(0, KDRV_DIS_DMA_ABORT, &dis_dma_abort);
		break;
	case DIS_IOC_GET_DMA_ABORT:
		kdrv_dis_get(0, KDRV_DIS_DMA_ABORT, &dis_dma_abort);
		//printk("DIS_IOC_GET_DMA_ABORT, %d\n\r", dis_dma_abort);
		iRet = (copy_to_user((void __user *)ulArg, &dis_dma_abort, sizeof(dis_dma_abort))) ? (-EFAULT) : 0;
		break;
	default :
		break;
	}
exit:
	return iRet;
}

irqreturn_t nvt_dis_drv_isr(int irq, void *p_devid)
{
	PDIS_MODULE_INFO pmodule_info = (PDIS_MODULE_INFO)p_devid;

	if (p_devid == NULL) {
		nvt_dbg(ERR, "invalid devid\r\n");
		return IRQ_NONE;
	}

	/* simple triggle and response mechanism*/
	complete(&pmodule_info->dis_completion);


	/*  Tasklet for bottom half mechanism */
	tasklet_schedule(&pmodule_info->dis_tasklet);

	if (irq == pmodule_info->iinterrupt_id[0]) {
		dis_isr();
	}

	return IRQ_HANDLED;
}


int nvt_dis_drv_wait_cmd_complete(PDIS_MODULE_INFO pmodule_info)
{
	if (pmodule_info == NULL) {
		nvt_dbg(ERR, "invalid pmodule_info\r\n");
		return -EINVAL;
	}

	wait_for_completion(&pmodule_info->dis_completion);
	return 0;
}

int nvt_dis_drv_write_reg(PDIS_MODULE_INFO pmodule_info, unsigned long addr, unsigned long value)
{
	if (pmodule_info == NULL) {
		nvt_dbg(ERR, "invalid pmodule_info\r\n");
		return -EINVAL;
	}

	WRITE_REG(value, pmodule_info->io_addr[0] + addr);
	return 0;
}

int nvt_dis_drv_read_reg(PDIS_MODULE_INFO pmodule_info, unsigned long addr)
{
	if (pmodule_info == NULL) {
		nvt_dbg(ERR, "invalid pmodule_info\r\n");
		return -EINVAL;
	}

	return READ_REG(pmodule_info->io_addr[0] + addr);
}

void nvt_dis_drv_do_tasklet(unsigned long data)
{
	PDIS_MODULE_INFO pmodule_info = (PDIS_MODULE_INFO)data;
	//nvt_dbg(INFO, "\n");

	/* do something you want*/
	complete(&pmodule_info->dis_completion);
}

