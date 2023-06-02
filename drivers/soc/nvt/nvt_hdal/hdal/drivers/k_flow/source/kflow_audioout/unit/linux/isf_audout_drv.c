#include "../include/isf_audout_drv.h"
#include "../include/isf_audout_ioctl.h"
#include "../isf_audout_int.h"
//#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "../include/isf_audout_dbg.h"
#include "kflow_audiocapture/wavstudio_tsk.h"
//#include "../../../kflow_audiocapture/unit/wavstudio/include/dxsound.h"

/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/
int isf_audout_drv_ioctl(unsigned char if_id, ISF_AUDOUT_INFO *p_module_info, unsigned int cmd, unsigned long arg);
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
int isf_audout_drv_open(PISF_AUDOUT_INFO p_module_info, unsigned char if_id)
{
	DBG_IND("%d\n", if_id);

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}


int isf_audout_drv_release(PISF_AUDOUT_INFO p_module_info, unsigned char if_id)
{
	DBG_IND("%d\n", if_id);

	/* Add HW Moduel release operation here when device file closed */

	return 0;
}

int isf_audout_drv_init(ISF_AUDOUT_INFO *p_module_info)
{
	/* Add HW Module initialization here when driver loaded */

	//isf_audout_install_id();
	isf_reg_unit(ISF_UNIT_ID(AUDOUT,0), &isf_audout0);
	isf_reg_unit(ISF_UNIT_ID(AUDOUT,1), &isf_audout1);

	return 0;
}

int isf_audout_drv_remove(ISF_AUDOUT_INFO *p_module_info)
{
	/* Add HW Moduel release operation here*/

	//isf_audout_uninstall_id();
	return 0;
}

int isf_audout_drv_suspend(ISF_AUDOUT_INFO *p_module_info)
{
	DBG_IND("\n");

	/* Add suspend operation here*/

	return 0;
}

int isf_audout_drv_resume(ISF_AUDOUT_INFO *p_module_info)
{
	DBG_IND("\n");
	/* Add resume operation here*/

	return 0;
}

int isf_audout_drv_ioctl(unsigned char if_id, ISF_AUDOUT_INFO *p_module_info, unsigned int cmd, unsigned long argc)
{
	int err = 0;

	DBG_IND("IF-%d cmd:%x\n", if_id, cmd);



	switch (cmd) {
	case ISF_AUDOUT_IOC_START:
		/*call someone to start operation*/
		break;

	case ISF_AUDOUT_IOC_STOP:
		/*call someone to stop operation*/
		break;
	case ISF_AUDOUT_IOC_READ_REG:
	case ISF_AUDOUT_IOC_WRITE_REG:
	case ISF_AUDOUT_IOC_READ_REG_LIST:
	case ISF_AUDOUT_IOC_WRITE_REG_LIST:
		break;

		/* Add other operations here */
	}

	return err;
}
