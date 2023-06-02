#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kwrap/spinlock.h"
#include "ive_drv.h"
#include "ive_ioctl.h"
#include "ive_lib.h"
#include "ive_dbg.h"
#include "kdrv_ive_int.h"
#include "kdrv_ive.h"
#include "kdrv_ive_version.h"


/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/
int nvt_ive_drv_wait_cmd_complete(PMODULE_INFO pmodule_info);
int nvt_ive_drv_ioctl(unsigned char ucIF, MODULE_INFO* pmodule_info, unsigned int cmd, unsigned long arg);
void nvt_ive_drv_do_tasklet(unsigned long data);
irqreturn_t nvt_ive_drv_isr(int irq, void *devid);
/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/
typedef irqreturn_t (*irq_handler_t)(int, void *);

/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/
int iEventFlag = 0;

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int nvt_ive_drv_open(PMODULE_INFO pmodule_info, unsigned char ucIF)
{
	DBG_IND("%d\n", ucIF);

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}


int nvt_ive_drv_release(PMODULE_INFO pmodule_info, unsigned char ucIF)
{
	DBG_IND("%d\n", ucIF);

	/* Add HW Moduel release operation here when device file closed */

	return 0;
}

int nvt_ive_drv_init(MODULE_INFO* pmodule_info)
{
	int iRet = 0;
    struct clk *pclk;
	DBG_IND("1\r\n");

	init_waitqueue_head(&pmodule_info->ive_wait_queue);
	vk_spin_lock_init(&pmodule_info->ive_spinlock);
	vk_sema_init(&pmodule_info->ive_sem, 1);
	init_completion(&pmodule_info->ive_completion);
	tasklet_init(&pmodule_info->ive_tasklet, nvt_ive_drv_do_tasklet, (unsigned long)pmodule_info);

	/* initial clock here */
	//clk_prepare(pmodule_info->pclk[0]);
	//clk_enable(pmodule_info->pclk[0]);
	//clk_set_rate(pmodule_info->pclk[0], 240000000);


	pclk = clk_get(NULL, "fix480m");
	if (IS_ERR(pclk)) {
		printk("%s: get source pll13 fail\r\n", __func__);
	}

	clk_set_parent(pmodule_info->pclk[0], pclk);
	//clk_prepare(pmodule_info->pclk[0]);
	//clk_enable(pmodule_info->pclk[0]);
	clk_put(pclk);
	//clk_set_rate(pime_info->pclk[0], 480000000);
	//ive_clk[0] = module_info->pclk[0];


    //printk("get source pclk : %d  \r\n", pmodule_info->pclk[0]);
    //printk("get source pclk : %d  \r\n", pclk->rate);



    DBG_IND("2\r\n");

	/* register IRQ here*/
	if(request_irq(pmodule_info->iinterrupt_id[0], nvt_ive_drv_isr, IRQF_TRIGGER_HIGH, "IVE_INT", pmodule_info)) {
		DBG_ERR("failed to register an IRQ Int:%d\n", pmodule_info->iinterrupt_id[0]);
		iRet = -ENODEV;
		goto FAIL_FREE_IRQ;
	}

	ive_set_base_addr((UINT32)pmodule_info->io_addr[0]);
	ive_create_resource((void *)pmodule_info->pclk[0], ive_freq_from_dtsi[0]);

	DBG_IND("3\r\n");

	kdrv_ive_install_id();
	kdrv_ive_init();

	/* Add HW Module initialization here when driver loaded */

	return iRet;

FAIL_FREE_IRQ:

	free_irq(pmodule_info->iinterrupt_id[0], pmodule_info);

	/* Add error handler here */

	return iRet;
}

int nvt_ive_drv_remove(MODULE_INFO* pmodule_info)
{

	//Free IRQ
	free_irq(pmodule_info->iinterrupt_id[0], pmodule_info);

	/* Add HW Moduel release operation here*/
    ive_release_resource((void *)pmodule_info->pclk[0]);
	kdrv_ive_uninstall_id();

	return 0;
}

int nvt_ive_drv_suspend(MODULE_INFO* pmodule_info)
{
	DBG_IND("\n");

	/* Add suspend operation here*/

	return 0;
}

int nvt_ive_drv_resume(MODULE_INFO* pmodule_info)
{
	DBG_IND("\n");
	/* Add resume operation here*/

	return 0;
}

int nvt_ive_drv_ioctl(unsigned char ucIF, MODULE_INFO* pmodule_info, unsigned int uiCmd, unsigned long ulArg)
{
	REG_INFO reg_info;
	REG_INFO_LIST reg_info_list;
	int iLoop;
	int iRet = 0;
	KDRV_IVE_OPENCFG                  ive_cfg_data            = {0};
	KDRV_IVE_IN_IMG_INFO              ive_imginfo_data        = {0};
	KDRV_IVE_IMG_IN_DMA_INFO          ive_input_addr          = {0};
	KDRV_IVE_IMG_OUT_DMA_INFO         ive_output_addr         = {0};
	KDRV_IVE_GENERAL_FILTER_PARAM     ive_general_filter_data = {0};
	KDRV_IVE_MEDIAN_FILTER_PARAM      ive_median_filter_data  = {0};
	KDRV_IVE_EDGE_FILTER_PARAM        ive_edge_filter_data    = {0};
	KDRV_IVE_NON_MAX_SUP_PARAM        ive_non_max_sup_data    = {0};
	KDRV_IVE_THRES_LUT_PARAM          ive_thres_lut_data      = {0};
	KDRV_IVE_MORPH_FILTER_PARAM       ive_morph_filter_data   = {0};
	KDRV_IVE_INTEGRAL_IMG_PARAM       ive_integral_img_data   = {0};
	KDRV_IVE_IRV_PARAM                ive_irv_data            = {0};
	KDRV_IVE_DMA_ABORT_PARAM		  ive_dma_abort_data      = {0};
	KDRV_IVE_FLOW_CT_PARAM            ive_flowct_data         = {0};
	CHAR                              version_info[32]        = KDRV_IVE_IMPL_VERSION;
	UINT32                            ive_waitdone_timeout    = 0;
	//KDRV_IVE_ITER_REG_VOTE_PARAM      ive_iter_reg_vote_data  = {0};
	//
	KDRV_IVE_OUTSEL_PARAM             ive_outsel_data         = {0};
	KDRV_IVE_TRIGGER_PARAM            ive_trigger_param       = {0};
	unsigned int id = 0;
	DBG_IND("IF-%d cmd:%x\n", ucIF, uiCmd);



	switch(uiCmd) {
		case IVE_IOC_START:
			/*call someone to start operation*/
			break;

		case IVE_IOC_STOP:
			/*call someone to stop operation*/
			break;

		case IVE_IOC_READ_REG:
			iRet = copy_from_user(&reg_info, (void __user *)ulArg, sizeof(REG_INFO));
	        if(!iRet) {
		        reg_info.uiValue = READ_REG(pmodule_info->io_addr[ucIF] + reg_info.uiAddr);
        	 	iRet = copy_to_user((void __user *)ulArg, &reg_info, sizeof(REG_INFO));
			}
			break;

		case IVE_IOC_WRITE_REG:
			iRet = copy_from_user(&reg_info, (void __user *)ulArg, sizeof(REG_INFO));
	        if(!iRet)
				WRITE_REG(reg_info.uiValue, pmodule_info->io_addr[ucIF] + reg_info.uiAddr);
			break;

		case IVE_IOC_READ_REG_LIST:
			iRet = copy_from_user(&reg_info_list, (void __user *)ulArg, sizeof(REG_INFO_LIST));
	        if(!iRet) {
				if (reg_info_list.uiCount < MODULE_REG_LIST_NUM) {
					for(iLoop = 0 ; iLoop < reg_info_list.uiCount; iLoop++)
						reg_info_list.RegList[iLoop].uiValue = READ_REG(pmodule_info->io_addr[ucIF] + reg_info_list.RegList[iLoop].uiAddr);
				}

        	 	iRet = copy_to_user((void __user *)ulArg, &reg_info_list, sizeof(REG_INFO_LIST));
			}
			break;
		case IVE_IOC_WRITE_REG_LIST:
			iRet = copy_from_user(&reg_info_list, (void __user *)ulArg, sizeof(REG_INFO_LIST));
	        if(!iRet) {
				if (reg_info_list.uiCount < MODULE_REG_LIST_NUM) {
					for(iLoop = 0 ; iLoop < reg_info_list.uiCount ; iLoop++)
						WRITE_REG(reg_info_list.RegList[iLoop].uiValue, pmodule_info->io_addr[ucIF] + reg_info_list.RegList[iLoop].uiAddr);
				}
			}
			break;

		/* Add other operations here */
		// OPEN
		case IVE_IOC_OPEN:
			kdrv_ive_open(0, 0);
			break;
		// CLOSE
		case IVE_IOC_CLOSE:
			kdrv_ive_close(0, 0);
			break;
		// OPENCFG
		case IVE_IOC_OPENCFG:
			if (copy_from_user(&ive_cfg_data, (void __user *)ulArg, sizeof(ive_cfg_data))) {
				iRet = -EFAULT;
				goto exit;
			}
			kdrv_ive_set(0, KDRV_IVE_PARAM_IPL_OPENCFG, &ive_cfg_data);
			break;
		// INPUT IMG INFO
		case IVE_IOC_SET_IMG_INFO:
			if (copy_from_user(&ive_imginfo_data, (void __user *)ulArg, sizeof(ive_imginfo_data))) {
				iRet = -EFAULT;
				goto exit;
			}
			if (copy_from_user(&id, (void __user *)(ulArg+sizeof(ive_imginfo_data)), sizeof(unsigned int))) {
				iRet = -EFAULT;
				goto exit;
			}
			kdrv_ive_set(id, KDRV_IVE_PARAM_IPL_IN_IMG, &ive_imginfo_data);
			break;
		case IVE_IOC_GET_IMG_INFO:
			if (copy_from_user(&id, (void __user *)ulArg, sizeof(unsigned int))) {
				iRet = -EFAULT;
				goto exit;
			}
			kdrv_ive_get(id, KDRV_IVE_PARAM_IPL_IN_IMG, &ive_imginfo_data);
			iRet = (copy_to_user((void __user *)ulArg, &ive_imginfo_data, sizeof(ive_imginfo_data))) ? (-EFAULT) : 0;
			break;
		// DMA IN
		case IVE_IOC_SET_IMG_DMA_IN:
			if (copy_from_user(&ive_input_addr, (void __user *)ulArg, sizeof(ive_input_addr))) {
				iRet = -EFAULT;
				goto exit;
			}
			if (copy_from_user(&id, (void __user *)(ulArg+sizeof(ive_input_addr)), sizeof(unsigned int))) {
				iRet = -EFAULT;
				goto exit;
			}
			kdrv_ive_set(id, KDRV_IVE_PARAM_IPL_IMG_DMA_IN, &ive_input_addr);
			break;
		case IVE_IOC_GET_IMG_DMA_IN:
			if (copy_from_user(&id, (void __user *)ulArg, sizeof(unsigned int))) {
				iRet = -EFAULT;
				goto exit;
			}
			kdrv_ive_get(id, KDRV_IVE_PARAM_IPL_IMG_DMA_IN, &ive_input_addr);
			iRet = (copy_to_user((void __user *)ulArg, &ive_input_addr, sizeof(ive_input_addr))) ? (-EFAULT) : 0;
			break;
		// DMA_OUT
		case IVE_IOC_SET_IMG_DMA_OUT:
			if (copy_from_user(&ive_output_addr, (void __user *)ulArg, sizeof(ive_output_addr))) {
				iRet = -EFAULT;
				goto exit;
			}
			if (copy_from_user(&id, (void __user *)(ulArg+sizeof(ive_output_addr)), sizeof(unsigned int))) {
				iRet = -EFAULT;
				goto exit;
			}
			kdrv_ive_set(id, KDRV_IVE_PARAM_IPL_IMG_DMA_OUT, &ive_output_addr);
			break;
		case IVE_IOC_GET_IMG_DMA_OUT:
			if (copy_from_user(&id, (void __user *)ulArg, sizeof(unsigned int))) {
				iRet = -EFAULT;
				goto exit;
			}
			kdrv_ive_get(id, KDRV_IVE_PARAM_IPL_IMG_DMA_OUT, &ive_output_addr);
			iRet = (copy_to_user((void __user *)ulArg, &ive_output_addr, sizeof(ive_output_addr))) ? (-EFAULT) : 0;
			break;
		// GENERAL FILTER
		case IVE_IOC_SET_GENERAL_FILTER:
			if (copy_from_user(&ive_general_filter_data, (void __user *)ulArg, sizeof(ive_general_filter_data))) {
				iRet = -EFAULT;
				goto exit;
			}
			if (copy_from_user(&id, (void __user *)(ulArg+sizeof(ive_general_filter_data)), sizeof(unsigned int))) {
				iRet = -EFAULT;
				goto exit;
			}
			kdrv_ive_set(id, KDRV_IVE_PARAM_IQ_GENERAL_FILTER, &ive_general_filter_data);
			break;
		case IVE_IOC_GET_GENERAL_FILTER:
			if (copy_from_user(&id, (void __user *)ulArg, sizeof(unsigned int))) {
				iRet = -EFAULT;
				goto exit;
			}
			kdrv_ive_get(id, KDRV_IVE_PARAM_IQ_GENERAL_FILTER, &ive_general_filter_data);
			iRet = (copy_to_user((void __user *)ulArg, &ive_general_filter_data, sizeof(ive_general_filter_data))) ? (-EFAULT) : 0;
			break;
		// MEDIAN FILTER
		case IVE_IOC_SET_MEDIAN_FILTER:
			if (copy_from_user(&ive_median_filter_data, (void __user *)ulArg, sizeof(ive_median_filter_data))) {
				iRet = -EFAULT;
				goto exit;
			}
			if (copy_from_user(&id, (void __user *)(ulArg+sizeof(ive_median_filter_data)), sizeof(unsigned int))) {
				iRet = -EFAULT;
				goto exit;
			}
			kdrv_ive_set(id, KDRV_IVE_PARAM_IQ_MEDIAN_FILTER, &ive_median_filter_data);
			break;
		case IVE_IOC_GET_MEDIAN_FILTER:
			if (copy_from_user(&id, (void __user *)ulArg, sizeof(unsigned int))) {
				iRet = -EFAULT;
				goto exit;
			}
			kdrv_ive_get(id, KDRV_IVE_PARAM_IQ_MEDIAN_FILTER, &ive_median_filter_data);
			iRet = (copy_to_user((void __user *)ulArg, &ive_median_filter_data, sizeof(ive_median_filter_data))) ? (-EFAULT) : 0;
			break;
		// EDGE FILTER
		case IVE_IOC_SET_EDGE_FILTER:
			if (copy_from_user(&ive_edge_filter_data, (void __user *)ulArg, sizeof(ive_edge_filter_data))) {
				iRet = -EFAULT;
				goto exit;
			}
			if (copy_from_user(&id, (void __user *)(ulArg+sizeof(ive_edge_filter_data)), sizeof(unsigned int))) {
				iRet = -EFAULT;
				goto exit;
			}
			kdrv_ive_set(id, KDRV_IVE_PARAM_IQ_EDGE_FILTER, &ive_edge_filter_data);
			break;
		case IVE_IOC_GET_EDGE_FILTER:
			if (copy_from_user(&id, (void __user *)ulArg, sizeof(unsigned int))) {
				iRet = -EFAULT;
				goto exit;
			}
			kdrv_ive_get(id, KDRV_IVE_PARAM_IQ_EDGE_FILTER, &ive_edge_filter_data);
			iRet = (copy_to_user((void __user *)ulArg, &ive_edge_filter_data, sizeof(ive_edge_filter_data))) ? (-EFAULT) : 0;
			break;
		// NON_MAX_SUP
		case IVE_IOC_SET_NON_MAX_SUP:
			if (copy_from_user(&ive_non_max_sup_data, (void __user *)ulArg, sizeof(ive_non_max_sup_data))) {
				iRet = -EFAULT;
				goto exit;
			}
			if (copy_from_user(&id, (void __user *)(ulArg+sizeof(ive_non_max_sup_data)), sizeof(unsigned int))) {
				iRet = -EFAULT;
				goto exit;
			}
			kdrv_ive_set(id, KDRV_IVE_PARAM_IQ_NON_MAX_SUP, &ive_non_max_sup_data);
			break;
		case IVE_IOC_GET_NON_MAX_SUP:
			if (copy_from_user(&id, (void __user *)ulArg, sizeof(unsigned int))) {
				iRet = -EFAULT;
				goto exit;
			}
			kdrv_ive_get(id, KDRV_IVE_PARAM_IQ_NON_MAX_SUP, &ive_non_max_sup_data);
			iRet = (copy_to_user((void __user *)ulArg, &ive_non_max_sup_data, sizeof(ive_non_max_sup_data))) ? (-EFAULT) : 0;
			break;
		// THRES_LUT
		case IVE_IOC_SET_THRES_LUT:
			if (copy_from_user(&ive_thres_lut_data, (void __user *)ulArg, sizeof(ive_thres_lut_data))) {
				iRet = -EFAULT;
				goto exit;
			}
			if (copy_from_user(&id, (void __user *)(ulArg+sizeof(ive_thres_lut_data)), sizeof(unsigned int))) {
				iRet = -EFAULT;
				goto exit;
			}
			kdrv_ive_set(id, KDRV_IVE_PARAM_IQ_THRES_LUT, &ive_thres_lut_data);
			break;
		case IVE_IOC_GET_THRES_LUT:
			if (copy_from_user(&id, (void __user *)ulArg, sizeof(unsigned int))) {
				iRet = -EFAULT;
				goto exit;
			}
			kdrv_ive_get(id, KDRV_IVE_PARAM_IQ_THRES_LUT, &ive_thres_lut_data);
			iRet = (copy_to_user((void __user *)ulArg, &ive_thres_lut_data, sizeof(ive_thres_lut_data))) ? (-EFAULT) : 0;
			break;
		// MORPH FILTER
		case IVE_IOC_SET_MORPH_FILTER:
			if (copy_from_user(&ive_morph_filter_data, (void __user *)ulArg, sizeof(ive_morph_filter_data))) {
				iRet = -EFAULT;
				goto exit;
			}
			if (copy_from_user(&id, (void __user *)(ulArg+sizeof(ive_morph_filter_data)), sizeof(unsigned int))) {
				iRet = -EFAULT;
				goto exit;
			}
			kdrv_ive_set(id, KDRV_IVE_PARAM_IQ_MORPH_FILTER, &ive_morph_filter_data);
			break;
		case IVE_IOC_GET_MORPH_FILTER:
			if (copy_from_user(&id, (void __user *)ulArg, sizeof(unsigned int))) {
				iRet = -EFAULT;
				goto exit;
			}
			kdrv_ive_get(id, KDRV_IVE_PARAM_IQ_MORPH_FILTER, &ive_morph_filter_data);
			iRet = (copy_to_user((void __user *)ulArg, &ive_morph_filter_data, sizeof(ive_morph_filter_data))) ? (-EFAULT) : 0;
			break;
		// INTEGRAL IMG
		case IVE_IOC_SET_INTEGRAL_IMG:
			if (copy_from_user(&ive_integral_img_data, (void __user *)ulArg, sizeof(ive_integral_img_data))) {
				iRet = -EFAULT;
				goto exit;
			}
			if (copy_from_user(&id, (void __user *)(ulArg+sizeof(ive_integral_img_data)), sizeof(unsigned int))) {
				iRet = -EFAULT;
				goto exit;
			}
			kdrv_ive_set(id, KDRV_IVE_PARAM_IQ_INTEGRAL_IMG, &ive_integral_img_data);
			break;
		case IVE_IOC_GET_INTEGRAL_IMG:
			if (copy_from_user(&id, (void __user *)ulArg, sizeof(unsigned int))) {
				iRet = -EFAULT;
				goto exit;
			}
			kdrv_ive_get(id, KDRV_IVE_PARAM_IQ_INTEGRAL_IMG, &ive_integral_img_data);
			iRet = (copy_to_user((void __user *)ulArg, &ive_integral_img_data, sizeof(ive_integral_img_data))) ? (-EFAULT) : 0;
			break;
		// ITER REGION VOTE
		/*
		case IVE_IOC_SET_ITER_REGION_VOTE:
			if (copy_from_user(&ive_iter_reg_vote_data, (void __user *)ulArg, sizeof(ive_iter_reg_vote_data))) {
				iRet = -EFAULT;
				goto exit;
			}
			if (copy_from_user(&id, (void __user *)(ulArg+sizeof(ive_iter_reg_vote_data)), sizeof(unsigned int))) {
				iRet = -EFAULT;
				goto exit;
			}
			kdrv_ive_set(id, KDRV_IVE_PARAM_IQ_ITER_REGION_VOTE, &ive_iter_reg_vote_data);
			break;
		case IVE_IOC_GET_ITER_REGION_VOTE:
			if (copy_from_user(&id, (void __user *)ulArg, sizeof(unsigned int))) {
				iRet = -EFAULT;
				goto exit;
			}
			kdrv_ive_get(id, KDRV_IVE_PARAM_IQ_ITER_REGION_VOTE, &ive_iter_reg_vote_data);
			iRet = (copy_to_user((void __user *)ulArg, &ive_iter_reg_vote_data, sizeof(ive_iter_reg_vote_data))) ? (-EFAULT) : 0;
			break;
			*/
		// trigger engine
		case IVE_IOC_TRIGGER:
			if (copy_from_user(&ive_trigger_param, (void __user *)ulArg, sizeof(ive_trigger_param))) {
				iRet = -EFAULT;
				goto exit;
			}
			if (copy_from_user(&id, (void __user *)(ulArg+sizeof(ive_trigger_param)), sizeof(unsigned int))) {
				iRet = -EFAULT;
				goto exit;
			}
			kdrv_ive_trigger(id, &ive_trigger_param, NULL, NULL);
			break;
		// trigger engine
		case IVE_IOC_TRIGGER_NONBLOCK:
			if (copy_from_user(&id, (void __user *)(ulArg), sizeof(unsigned int))) {
				iRet = -EFAULT;
				goto exit;
			}
			iRet = kdrv_ive_trigger_nonblock(id);
			if (iRet != 0) {
				iRet = IVE_MSG_ERR;
			}
			break;
		//520-add
		case IVE_IOC_SET_OUTSEL:
		    if (copy_from_user(&ive_outsel_data, (void __user *)ulArg, sizeof(ive_outsel_data))) {
				iRet = -EFAULT;
				goto exit;
			}
			if (copy_from_user(&id, (void __user *)(ulArg+sizeof(ive_outsel_data)), sizeof(unsigned int))) {
				iRet = -EFAULT;
				goto exit;
			}
			kdrv_ive_set(id, KDRV_IVE_PARAM_IQ_OUTSEL, &ive_outsel_data);
            break;
        case IVE_IOC_GET_OUTSEL:
            if (copy_from_user(&id, (void __user *)ulArg, sizeof(unsigned int))) {
				iRet = -EFAULT;
				goto exit;
			}
			kdrv_ive_get(id, KDRV_IVE_PARAM_IQ_OUTSEL, &ive_outsel_data);
			iRet = (copy_to_user((void __user *)ulArg, &ive_outsel_data, sizeof(ive_outsel_data))) ? (-EFAULT) : 0;
            break;
		// IRV
		case IVE_IOC_SET_IRV:
			if (copy_from_user(&ive_irv_data, (void __user *)ulArg, sizeof(ive_irv_data))) {
				iRet = -EFAULT;
				goto exit;
			}
			if (copy_from_user(&id, (void __user *)(ulArg+sizeof(ive_irv_data)), sizeof(unsigned int))) {
				iRet = -EFAULT;
				goto exit;
			}
			kdrv_ive_set(id, KDRV_IVE_PARAM_IRV, &ive_irv_data);
			break;
		case IVE_IOC_GET_IRV:
			if (copy_from_user(&id, (void __user *)ulArg, sizeof(unsigned int))) {
				iRet = -EFAULT;
				goto exit;
			}
			kdrv_ive_get(id, KDRV_IVE_PARAM_IRV, &ive_irv_data);
			iRet = (copy_to_user((void __user *)ulArg, &ive_irv_data, sizeof(ive_irv_data))) ? (-EFAULT) : 0;
			break;

		case IVE_IOC_SET_DMA_ABORT:
			if (copy_from_user(&ive_dma_abort_data, (void __user *)ulArg, sizeof(ive_dma_abort_data))) {
				iRet = -EFAULT;
				goto exit;
			}
			if (copy_from_user(&id, (void __user *)(ulArg + sizeof(ive_dma_abort_data)), sizeof(unsigned int))) {
				iRet = -EFAULT;
				goto exit;
			}
			kdrv_ive_set(id, KDRV_IVE_PARAM_DMA_ABORT, &ive_dma_abort_data);
			break;
		case IVE_IOC_SET_FLOWCT:
			if (copy_from_user(&ive_flowct_data, (void __user *)ulArg, sizeof(ive_flowct_data))) {
				iRet = -EFAULT;
				goto exit;
			}
			if (copy_from_user(&id, (void __user *)(ulArg+sizeof(ive_flowct_data)), sizeof(unsigned int))) {
				iRet = -EFAULT;
				goto exit;
			}
			kdrv_ive_set(id, KDRV_IVE_PARAM_FLOWCT, &ive_flowct_data);
			break;
		case IVE_IOC_GET_FLOWCT:
			if (copy_from_user(&id, (void __user *)ulArg, sizeof(unsigned int))) {
				iRet = -EFAULT;
				goto exit;
			}
			kdrv_ive_get(id, KDRV_IVE_PARAM_FLOWCT, &ive_flowct_data);
			iRet = (copy_to_user((void __user *)ulArg, &ive_flowct_data, sizeof(ive_flowct_data))) ? (-EFAULT) : 0;
			break;

		case IVE_IOC_GET_VERSION:
			if (copy_from_user(&id, (void __user *)ulArg, sizeof(unsigned int))) {
				iRet = -EFAULT;
				goto exit;
			}
			iRet = (copy_to_user((void __user *)ulArg, version_info, sizeof(KDRV_IVE_IMPL_VERSION))) ? (-EFAULT) : 0;
			break;
		// wait engine done
		case IVE_IOC_WAITDONE_NONBLOCK:
			if (copy_from_user(&ive_waitdone_timeout, (void __user *)ulArg, sizeof(UINT32))) {
				iRet = -EFAULT;
				goto exit;
			}
			if (copy_from_user(&id, (void __user *)(ulArg+sizeof(UINT32)), sizeof(unsigned int))) {
				iRet = -EFAULT;
				goto exit;
			}
			iRet = kdrv_ive_waitdone_nonblock(id, &ive_waitdone_timeout);
			if (iRet != 0) {
				if (IVE_MSG_TIMEOUT != iRet) {
					iRet = IVE_MSG_ERR;
				}
			}
			break;

		default :
			break;
	}

exit:
	return iRet;
}

irqreturn_t nvt_ive_drv_isr(int irq, void *devid)
{
	PMODULE_INFO pmodule_info = (PMODULE_INFO)devid;

	/* simple triggle and response mechanism*/
	complete(&pmodule_info->ive_completion);


	/*  Tasklet for bottom half mechanism */
	tasklet_schedule(&pmodule_info->ive_tasklet);

	ive_isr();

    return IRQ_HANDLED;
}

int nvt_ive_drv_wait_cmd_complete(PMODULE_INFO pmodule_info)
{
	wait_for_completion(&pmodule_info->ive_completion);
	return 0;
}

int nvt_ive_drv_write_reg(PMODULE_INFO pmodule_info, unsigned long addr, unsigned long value)
{
	WRITE_REG(value, pmodule_info->io_addr[0] + addr);
	return 0;
}

int nvt_ive_drv_read_reg(PMODULE_INFO pmodule_info, unsigned long addr)
{
	return READ_REG(pmodule_info->io_addr[0] + addr);
}

void nvt_ive_drv_do_tasklet(unsigned long data)
{
	PMODULE_INFO pmodule_info = (PMODULE_INFO)data;
	//DBG_IND("\n");

	/* do something you want*/
	complete(&pmodule_info->ive_completion);
}
