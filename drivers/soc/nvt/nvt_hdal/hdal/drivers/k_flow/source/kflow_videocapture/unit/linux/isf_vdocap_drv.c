#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include "isf_vdocap_drv.h"
#include "isf_vdocap_ioctl.h"

#include "../isf_vdocap_dbg.h"
#include "../isf_vdocap_int.h"

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int isf_vdocap_drv_open(MODULE_INFO *pmodule_info, unsigned char minor)
{
	DBG_IND("%d\n", minor);

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}

int isf_vdocap_drv_release(MODULE_INFO *pmodule_info, unsigned char minor)
{
	DBG_IND("%d\n", minor);

	/* Add HW Moduel release operation here when device file closed */

	return 0;
}

int isf_vdocap_drv_init(MODULE_INFO *pmodule_info)
{
	int er = 0;
#if 0
	ISF_VDOCAP_INIT init = {0};

	isf_vdocap_install_id();

	init.api_ver = ISF_VDOCAP_API_VERSION;
	if (!isf_vdocap_init(&init)) {
		DBG_ERR("vdocap_init() Failed!\r\n");
		return -ENODEV;
	}
	isf_vdocap_regbi_adj();
#endif
	isf_reg_unit(ISF_UNIT_ID(VDOCAP,0), &isf_vdocap0);
	isf_reg_unit(ISF_UNIT_ID(VDOCAP,1), &isf_vdocap1);
	isf_reg_unit(ISF_UNIT_ID(VDOCAP,2), &isf_vdocap2);
	isf_reg_unit(ISF_UNIT_ID(VDOCAP,3), &isf_vdocap3);
	isf_reg_unit(ISF_UNIT_ID(VDOCAP,4), &isf_vdocap4);
#if defined(_BSP_NA51000_)
	isf_reg_unit(ISF_UNIT_ID(VDOCAP,5), &isf_vdocap5);
	isf_reg_unit(ISF_UNIT_ID(VDOCAP,6), &isf_vdocap6);
	isf_reg_unit(ISF_UNIT_ID(VDOCAP,7), &isf_vdocap7);
#endif
	//isf_vdocap_install_id();

	return er;

#if 0
FAIL_FREE_IRQ:

	free_irq(pmodule_info->iinterrupt_id[0], pmodule_info);

	/* Add error handler here */

	return er;
#endif
}

int isf_vdocap_drv_remove(MODULE_INFO *pmodule_info)
{
#if 0
	isf_vdocap_exit();
#endif
	//isf_vdocap_uninstall_id();
	return 0;
}

int isf_vdocap_drv_suspend(MODULE_INFO *pmodule_info)
{
	DBG_IND("\n");

	/* Add suspend operation here*/

	return 0;
}

int isf_vdocap_drv_resume(MODULE_INFO *pmodule_info)
{
	DBG_IND("\n");
	/* Add resume operation here*/

	return 0;
}

int isf_vdocap_drv_ioctl(unsigned char minor, MODULE_INFO *pmodule_info, unsigned int cmd_id, unsigned long arg)
{
	int er = 0;

	DBG_IND("IF-%d cmd:%x\n", minor, cmd_id);

	switch (cmd_id) {
	case ISF_VDOCAP_IOC_CMD: {
			#if 0
			ISF_VDOCAP_CTRL *p_ctrl = isf_vdocap_get_ctrl();
			ISF_VDOCAP_ICMD *p_cmd = &p_ctrl->ipc.p_cfg->cmd;
			((ISF_VDOCAP_IAPI)p_cmd->api_addr)(p_cmd);
			#endif
		}
		break;
	}

	return er;
}
