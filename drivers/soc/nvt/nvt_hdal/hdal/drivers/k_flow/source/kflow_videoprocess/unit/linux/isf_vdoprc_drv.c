#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include "kflow_videoprocess/isf_vdoprc.h"
#include "isf_vdoprc_drv.h"
#include "isf_vdoprc_ioctl.h"
//#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "../isf_vdoprc_dbg.h"
#include "../isf_vdoprc_int.h"

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int isf_vdoprc_drv_open(PMODULE_INFO pmodule_info, unsigned char minor)
{
	DBG_IND("%d\n", minor);

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}

int isf_vdoprc_drv_release(PMODULE_INFO pmodule_info, unsigned char minor)
{
	DBG_IND("%d\n", minor);

	/* Add HW Moduel release operation here when device file closed */

	return 0;
}

int isf_vdoprc_drv_init(PMODULE_INFO pmodule_info)
{
	int er = 0;
#if 0
	ISF_VDOPRC_INIT init = {0};
	init.api_ver = ISF_VDOPRC_API_VERSION;
	if (!isf_vdoprc_init(&init)) {
		DBG_ERR("vdoprc_init() Failed!\r\n");
		return -ENODEV;
	}
	isf_vdoprc_regbi_adj();
#endif
#if (VDOPRC_MAX_NUM > 0)
	isf_reg_unit(ISF_UNIT_ID(VDOPRC,0), &isf_vdoprc0);
#endif
#if (VDOPRC_MAX_NUM > 1)
	isf_reg_unit(ISF_UNIT_ID(VDOPRC,1), &isf_vdoprc1);
#endif
#if (VDOPRC_MAX_NUM > 2)
	isf_reg_unit(ISF_UNIT_ID(VDOPRC,2), &isf_vdoprc2);
#endif
#if (VDOPRC_MAX_NUM > 3)
	isf_reg_unit(ISF_UNIT_ID(VDOPRC,3), &isf_vdoprc3);
#endif
#if (VDOPRC_MAX_NUM > 4)
	isf_reg_unit(ISF_UNIT_ID(VDOPRC,4), &isf_vdoprc4);
#endif
#if (VDOPRC_MAX_NUM > 5)
	isf_reg_unit(ISF_UNIT_ID(VDOPRC,5), &isf_vdoprc5);
#endif
#if (VDOPRC_MAX_NUM > 6)
	isf_reg_unit(ISF_UNIT_ID(VDOPRC,6), &isf_vdoprc6);
#endif
#if (VDOPRC_MAX_NUM > 7)
	isf_reg_unit(ISF_UNIT_ID(VDOPRC,7), &isf_vdoprc7);
#endif
#if (VDOPRC_MAX_NUM > 8)
	isf_reg_unit(ISF_UNIT_ID(VDOPRC,8), &isf_vdoprc8);
#endif
#if (VDOPRC_MAX_NUM > 9)
	isf_reg_unit(ISF_UNIT_ID(VDOPRC,9), &isf_vdoprc9);
#endif
#if (VDOPRC_MAX_NUM > 10)
	isf_reg_unit(ISF_UNIT_ID(VDOPRC,10), &isf_vdoprc10);
#endif
#if (VDOPRC_MAX_NUM > 11)
	isf_reg_unit(ISF_UNIT_ID(VDOPRC,11), &isf_vdoprc11);
#endif
#if (VDOPRC_MAX_NUM > 12)
	isf_reg_unit(ISF_UNIT_ID(VDOPRC,12), &isf_vdoprc12);
#endif
#if (VDOPRC_MAX_NUM > 13)
	isf_reg_unit(ISF_UNIT_ID(VDOPRC,13), &isf_vdoprc13);
#endif
#if (VDOPRC_MAX_NUM > 14)
	isf_reg_unit(ISF_UNIT_ID(VDOPRC,14), &isf_vdoprc14);
#endif
#if (VDOPRC_MAX_NUM > 15)
	isf_reg_unit(ISF_UNIT_ID(VDOPRC,15), &isf_vdoprc15);
#endif
	//isf_vdoprc_install_id();

	return er;

#if 0
FAIL_FREE_IRQ:

	free_irq(pmodule_info->iinterrupt_id[0], pmodule_info);

	/* Add error handler here */

	return er;
#endif
}

int isf_vdoprc_drv_remove(PMODULE_INFO pmodule_info)
{
#if 0
	isf_vdoprc_exit();
#endif
	//isf_vdoprc_uninstall_id();
	return 0;
}

int isf_vdoprc_drv_suspend(PMODULE_INFO pmodule_info)
{
	DBG_IND("\n");

	/* Add suspend operation here*/

	return 0;
}

int isf_vdoprc_drv_resume(PMODULE_INFO pmodule_info)
{
	DBG_IND("\n");
	/* Add resume operation here*/

	return 0;
}

int isf_vdoprc_drv_ioctl(unsigned char minor, PMODULE_INFO pmodule_info, unsigned int cmd_id, unsigned long arg)
{
	int er = 0;

	DBG_IND("IF-%d cmd:%x\n", minor, cmd_id);

	switch (cmd_id) {
	case ISF_VDOPRC_IOC_CMD: {
			#if 0
			ISF_VDOPRC_CTRL *p_ctrl = isf_vdoprc_get_ctrl();
			ISF_VDOPRC_ICMD *p_cmd = &p_ctrl->ipc.p_cfg->cmd;
			((ISF_VDOPRC_IAPI)p_cmd->api_addr)(p_cmd);
			#endif
		}
		break;
	}

	return er;
}
