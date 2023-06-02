#include "isf_vdoout_drv.h"
#include "isf_vdoout_ioctl.h"
#include "../isf_vdoout_int.h"
#include "../isf_vdoout_dbg.h"

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int isf_vdoout_drv_open(ISF_VDOOUT_INFO *pmodule_info, unsigned char minor)
{
	DBG_IND("%d\n", minor);

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}

int isf_vdoout_drv_release(ISF_VDOOUT_INFO *pmodule_info, unsigned char minor)
{
	DBG_IND("%d\n", minor);

	/* Add HW Moduel release operation here when device file closed */

	return 0;
}

int isf_vdoout_drv_init(ISF_VDOOUT_INFO *pmodule_info)
{
	int er = 0;
	isf_vdoout_install_id();

	isf_reg_unit(ISF_UNIT_ID(VDOOUT, 0), &isf_vdoout0);
#if (DEV_ID_MAX > 1)
	isf_reg_unit(ISF_UNIT_ID(VDOOUT, 1), &isf_vdoout1);
#endif
	return er;

}

int isf_vdoout_drv_remove(ISF_VDOOUT_INFO *pmodule_info)
{
	isf_vdoout_uninstall_id();
	return 0;
}

int isf_vdoout_drv_suspend(ISF_VDOOUT_INFO *pmodule_info)
{
	DBG_IND("\n");

	/* Add suspend operation here*/

	return 0;
}

int isf_vdoout_drv_resume(ISF_VDOOUT_INFO *pmodule_info)
{
	DBG_IND("\n");
	/* Add resume operation here*/

	return 0;
}

int isf_vdoout_drv_ioctl(unsigned char minor, ISF_VDOOUT_INFO *pmodule_info, unsigned int cmd_id, unsigned long arg)
{
	int er = 0;

	DBG_IND("IF-%d cmd:%x\n", minor, cmd_id);

	switch (cmd_id) {
	case ISF_VDOOUT_IOC_CMD: {
		}
		break;
	}

	return er;
}
