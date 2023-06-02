#include "kflow_videocapture/isf_vdocap.h"
#include "isf_vdocap_drv.h"
//#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "../isf_vdocap_dbg.h"
#include "../isf_vdocap_int.h"

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/

int kflow_videocap_init(void)
{
	isf_reg_unit(ISF_UNIT(VDOCAP,0), &isf_vdocap0);
	isf_reg_unit(ISF_UNIT(VDOCAP,1), &isf_vdocap1);
	isf_reg_unit(ISF_UNIT(VDOCAP,2), &isf_vdocap2);

	return 0;
}
