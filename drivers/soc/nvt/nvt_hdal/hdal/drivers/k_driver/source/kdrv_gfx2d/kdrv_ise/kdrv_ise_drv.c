#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include "plat/nvt-sramctl.h"
#include "kdrv_type.h"
#include "kdrv_ise_int_drv.h"
#include "kdrv_ise_int_dbg.h"
#include "kdrv_ise_int.h"
#include "ise_eng.h"


/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/
int nvt_kdrv_ise_drv_wait_cmd_complete(PNVT_KDRV_ISE_INFO pnvt_kdrv_ise_info);
//int nvt_kdrv_ise_drv_ioctl(unsigned char ucIF, NVT_KDRV_ISE_INFO *pnvt_kdrv_ise_info, unsigned int cmd, unsigned long arg);
void nvt_kdrv_ise_drv_do_tasklet(unsigned long data);
irqreturn_t nvt_kdrv_ise_drv_isr(int irq, void *devid);
/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/
typedef irqreturn_t (*irq_handler_t)(int, void *);

/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/


/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int nvt_kdrv_ise_drv_open(PNVT_KDRV_ISE_INFO pnvt_kdrv_ise_info, unsigned char ucIF)
{
	nvt_dbg(IND, "%d\n", ucIF);
	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}


int nvt_kdrv_ise_drv_release(PNVT_KDRV_ISE_INFO pnvt_kdrv_ise_info, unsigned char ucIF)
{
	nvt_dbg(IND, "%d\n", ucIF);

	/* for backward compatible */
	kdrv_ise_uninit();

	/* Add HW Moduel release operation here when device file closed */
	ise_eng_release(NULL);
	kdrv_ise_sys_uninit(NULL);

	return 0;
}

int nvt_kdrv_ise_drv_init(NVT_KDRV_ISE_INFO *pnvt_kdrv_ise_info)
{
	ISE_ENG_HANDLE *p_eng_handle;
	ISE_ENG_HANDLE m_eng;
	ISE_ENG_CTL eng_ctl;
	KDRV_ISE_HANDLE m_kdrv_hdl;
	KDRV_ISE_CTL kdrv_ctl;
	KDRV_ISE_CTX_BUF_CFG init_buf_cfg;
	int iRet = 0;
	int i = 0;

	/* temporay, init one ise */
	m_eng.reg_io_base = (UINT32)pnvt_kdrv_ise_info->io_addr[0];
	m_eng.pclk = pnvt_kdrv_ise_info->pclk[0];
	m_eng.clock_rate = 480;
	m_eng.sram_id = ISE_SD;
	m_eng.isr_cb = NULL;
	m_eng.chip_id = KDRV_CHIP0;
	m_eng.eng_id = KDRV_GFX2D_ISE0;
	eng_ctl.p_eng = &m_eng;
	eng_ctl.chip_num = 1;
	eng_ctl.eng_num = 1;
	eng_ctl.total_ch = 1;
	ise_eng_init(&eng_ctl);

	p_eng_handle = ise_eng_get_handle(m_eng.chip_id, m_eng.eng_id);

	m_kdrv_hdl.chip_id = m_eng.chip_id;
	m_kdrv_hdl.eng_id = m_eng.eng_id;
	kdrv_ctl.p_hdl = &m_kdrv_hdl;
	kdrv_ctl.chip_num = eng_ctl.chip_num;
	kdrv_ctl.eng_num = eng_ctl.eng_num;
	kdrv_ctl.total_ch = eng_ctl.total_ch;
	kdrv_ise_sys_init(&kdrv_ctl);

	/* register IRQ here*/
	for (i = 0; i < NVT_KDRV_ISE_IRQ_NUM; i++) {
		if (request_irq(pnvt_kdrv_ise_info->iinterrupt_id[i], nvt_kdrv_ise_drv_isr, IRQF_TRIGGER_HIGH, "ISE_INT", p_eng_handle)) {
			nvt_dbg(ERR, "failed to register an IRQ Int:%d\n", (int)pnvt_kdrv_ise_info->iinterrupt_id[i]);
			iRet = -ENODEV;

			free_irq(pnvt_kdrv_ise_info->iinterrupt_id[i], pnvt_kdrv_ise_info);

			goto FAIL_FREE_IRQ;
		}
	}

	/* for backward compatible */
	init_buf_cfg.reserved = 0;
	kdrv_ise_init(init_buf_cfg, 0, 0);

	nvt_dbg(IND, "ise initialization...\r\n");

	return iRet;

FAIL_FREE_IRQ:
	/* Add error handler here */

	return iRet;
}

int nvt_kdrv_ise_drv_remove(NVT_KDRV_ISE_INFO *pnvt_kdrv_ise_info)
{
	int i = 0;

	/* Free IRQ */
	for (i = 0; i < NVT_KDRV_ISE_REG_NUM; i++) {
		free_irq(pnvt_kdrv_ise_info->iinterrupt_id[i], pnvt_kdrv_ise_info);
	}

	return 0;
}

int nvt_kdrv_ise_drv_suspend(NVT_KDRV_ISE_INFO *pnvt_kdrv_ise_info)
{
	return 0;
}

int nvt_kdrv_ise_drv_resume(NVT_KDRV_ISE_INFO *pnvt_kdrv_ise_info)
{
	return 0;
}

#if 0
int nvt_kdrv_ise_drv_ioctl(unsigned char ucIF, NVT_KDRV_ISE_INFO *pnvt_kdrv_ise_info, unsigned int uiCmd, unsigned long ulArg)
{
	REG_INFO reg_info;
	REG_INFO_LIST reg_info_list;
	int iLoop;
	int iRet = 0;

	//nvt_dbg(IND, "IF-%d cmd:%x\n", ucIF, uiCmd);



	switch (uiCmd) {
	case ISE_IOC_START:
		/*call someone to start operation*/
		break;

	case ISE_IOC_STOP:
		/*call someone to stop operation*/
		break;

	case ISE_IOC_READ_REG:
		iRet = copy_from_user(&reg_info, (void __user *)ulArg, sizeof(REG_INFO));
		if (!iRet) {
			reg_info.uiValue = READ_REG(pnvt_kdrv_ise_info->io_addr[ucIF] + reg_info.uiAddr);
			iRet = copy_to_user((void __user *)ulArg, &reg_info, sizeof(REG_INFO));
		}
		break;

	case ISE_IOC_WRITE_REG:
		iRet = copy_from_user(&reg_info, (void __user *)ulArg, sizeof(REG_INFO));
		if (!iRet) {
			WRITE_REG(reg_info.uiValue, pnvt_kdrv_ise_info->io_addr[ucIF] + reg_info.uiAddr);
		}
		break;

	case ISE_IOC_READ_REG_LIST:
		iRet = copy_from_user(&reg_info_list, (void __user *)ulArg, sizeof(REG_INFO_LIST));
		if (!iRet) {
			if (reg_info_list.uiCount <= ISE_REG_LIST_NUM) {
				for (iLoop = 0 ; iLoop < reg_info_list.uiCount; iLoop++) {
					reg_info_list.RegList[iLoop].uiValue = READ_REG(pnvt_kdrv_ise_info->io_addr[ucIF] + reg_info_list.RegList[iLoop].uiAddr);
				}
			} else {
				DBG_ERR("Loop bound error!\r\n");
			}

			iRet = copy_to_user((void __user *)ulArg, &reg_info_list, sizeof(REG_INFO_LIST));
		}
		break;
	case ISE_IOC_WRITE_REG_LIST:
		iRet = copy_from_user(&reg_info_list, (void __user *)ulArg, sizeof(REG_INFO_LIST));
		if (!iRet) {
			if (reg_info_list.uiCount <= ISE_REG_LIST_NUM) {
				for (iLoop = 0 ; iLoop < reg_info_list.uiCount ; iLoop++) {
					WRITE_REG(reg_info_list.RegList[iLoop].uiValue, pnvt_kdrv_ise_info->io_addr[ucIF] + reg_info_list.RegList[iLoop].uiAddr);
				}
			} else {
				DBG_ERR("Loop bound error!\r\n");
			}
		}
		break;

		/* Add other operations here */
	}

	return iRet;
}
#endif

irqreturn_t nvt_kdrv_ise_drv_isr(int irq, void *p_eng_handle)
{
	ise_eng_isr(p_eng_handle);
	return IRQ_HANDLED;
}

