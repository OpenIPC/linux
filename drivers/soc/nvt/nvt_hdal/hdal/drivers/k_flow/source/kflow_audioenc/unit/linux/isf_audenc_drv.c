#include "isf_audenc_drv.h"
#include "isf_audenc_ioctl.h"
#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_core.h"
#include "kflow_audioenc/isf_audenc.h"
#include "nmediarec_api.h"

//#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "isf_audenc_dbg.h"

///#include "../isf_audenc_int.h"

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int isf_audenc_drv_open(ISF_AUDENC_INFO *pmodule_info, unsigned char minor)
{
	DBG_IND("%d\n", minor);

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}

int isf_audenc_drv_release(ISF_AUDENC_INFO *pmodule_info, unsigned char minor)
{
	DBG_IND("%d\n", minor);

	/* Add HW Moduel release operation here when device file closed */

	return 0;
}

int isf_audenc_drv_init(ISF_AUDENC_INFO *pmodule_info)
{
	int er = 0;
#if 0
	ISF_AUDENC_INIT init = {0};

	isf_audenc_install_id();

	init.api_ver = ISF_AUDENC_API_VERSION;
	if (!isf_audenc_init(&init)) {
		DBG_ERR("audenc_init() Failed!\r\n");
		return -ENODEV;
	}
	isf_audenc_regbi_adj();
#endif

	//isf_audenc_install_id();
	//nmr_audenc_install_id();

    isf_reg_unit(ISF_UNIT_ID(AUDENC,0), &isf_audenc);

	return er;

#if 0
FAIL_FREE_IRQ:

	free_irq(pmodule_info->iinterrupt_id[0], pmodule_info);

	/* Add error handler here */

	return er;
#endif
}

int isf_audenc_drv_remove(ISF_AUDENC_INFO *pmodule_info)
{
#if 0
	isf_audenc_exit();
#endif
	//isf_audenc_uninstall_id();
	//nmr_audenc_uninstall_id();
	return 0;
}

int isf_audenc_drv_suspend(ISF_AUDENC_INFO *pmodule_info)
{
	DBG_IND("\n");

	/* Add suspend operation here*/

	return 0;
}

int isf_audenc_drv_resume(ISF_AUDENC_INFO *pmodule_info)
{
	DBG_IND("\n");
	/* Add resume operation here*/

	return 0;
}

int isf_audenc_drv_ioctl(unsigned char minor, ISF_AUDENC_INFO *pmodule_info, unsigned int cmd_id, unsigned long arg)
{
	int er = 0;

	DBG_IND("IF-%d cmd:%x\n", minor, cmd_id);

	switch (cmd_id) {
	case ISF_AUDENC_IOC_CMD: {
			#if 0
			ISF_AUDENC_CTRL *p_ctrl = isf_audenc_get_ctrl();
			ISF_AUDENC_ICMD *p_cmd = &p_ctrl->ipc.p_cfg->cmd;
			((ISF_AUDENC_IAPI)p_cmd->api_addr)(p_cmd);
			#endif
		}
		break;
	}

	return er;
}
