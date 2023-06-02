#include "isf_auddec_drv.h"
#include "isf_auddec_ioctl.h"
#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_core.h"
#include "kflow_audiodec/isf_auddec.h"
#include "nmediaplay_api.h"

//#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "isf_auddec_dbg.h"

///#include "../isf_auddec_int.h"

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int isf_auddec_drv_open(ISF_AUDDEC_INFO *pmodule_info, unsigned char minor)
{
	DBG_IND("%d\n", minor);

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}

int isf_auddec_drv_release(ISF_AUDDEC_INFO *pmodule_info, unsigned char minor)
{
	DBG_IND("%d\n", minor);

	/* Add HW Moduel release operation here when device file closed */

	return 0;
}

int isf_auddec_drv_init(ISF_AUDDEC_INFO *pmodule_info)
{
	int er = 0;
#if 0
	ISF_AUDDEC_INIT init = {0};

	isf_auddec_install_id();

	init.api_ver = ISF_AUDDEC_API_VERSION;
	if (!isf_auddec_init(&init)) {
		DBG_ERR("auddec_init() Failed!\r\n");
		return -ENODEV;
	}
	isf_auddec_regbi_adj();
#endif

	//isf_auddec_install_id();
	//nmp_auddec_install_id();

    isf_reg_unit(ISF_UNIT_ID(AUDDEC,0), &isf_auddec);

	return er;

#if 0
FAIL_FREE_IRQ:

	free_irq(pmodule_info->iinterrupt_id[0], pmodule_info);

	/* Add error handler here */

	return er;
#endif
}

int isf_auddec_drv_remove(ISF_AUDDEC_INFO *pmodule_info)
{
#if 0
	isf_auddec_exit();
#endif
	//isf_auddec_uninstall_id();
	//nmp_auddec_uninstall_id();
	return 0;
}

int isf_auddec_drv_suspend(ISF_AUDDEC_INFO *pmodule_info)
{
	DBG_IND("\n");

	/* Add suspend operation here*/

	return 0;
}

int isf_auddec_drv_resume(ISF_AUDDEC_INFO *pmodule_info)
{
	DBG_IND("\n");
	/* Add resume operation here*/

	return 0;
}

int isf_auddec_drv_ioctl(unsigned char minor, ISF_AUDDEC_INFO *pmodule_info, unsigned int cmd_id, unsigned long arg)
{
	int er = 0;

	DBG_IND("IF-%d cmd:%x\n", minor, cmd_id);

	switch (cmd_id) {
	case ISF_AUDDEC_IOC_CMD: {
			#if 0
			ISF_AUDDEC_CTRL *p_ctrl = isf_auddec_get_ctrl();
			ISF_AUDDEC_ICMD *p_cmd = &p_ctrl->ipc.p_cfg->cmd;
			((ISF_AUDDEC_IAPI)p_cmd->api_addr)(p_cmd);
			#endif
		}
		break;
	}

	return er;
}
