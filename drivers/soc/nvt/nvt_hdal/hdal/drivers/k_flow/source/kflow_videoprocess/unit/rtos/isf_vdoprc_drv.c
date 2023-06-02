#include "kflow_videoprocess/isf_vdoprc.h"
#include "isf_vdoprc_drv.h"
//#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "../isf_vdoprc_dbg.h"
#include "../isf_vdoprc_int.h"

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/

int kflow_videoproc_init(void)
{
#if (VDOPRC_MAX_NUM > 0)
	isf_reg_unit(ISF_UNIT(VDOPRC,0), &isf_vdoprc0);
#endif
#if (VDOPRC_MAX_NUM > 1)
	isf_reg_unit(ISF_UNIT(VDOPRC,1), &isf_vdoprc1);
#endif
#if (VDOPRC_MAX_NUM > 2)
	isf_reg_unit(ISF_UNIT(VDOPRC,2), &isf_vdoprc2);
#endif
#if (VDOPRC_MAX_NUM > 3)
	isf_reg_unit(ISF_UNIT(VDOPRC,3), &isf_vdoprc3);
#endif
#if (VDOPRC_MAX_NUM > 4)
	isf_reg_unit(ISF_UNIT(VDOPRC,4), &isf_vdoprc4);
#endif
#if (VDOPRC_MAX_NUM > 5)
	isf_reg_unit(ISF_UNIT(VDOPRC,5), &isf_vdoprc5);
#endif
#if (VDOPRC_MAX_NUM > 6)
	isf_reg_unit(ISF_UNIT(VDOPRC,6), &isf_vdoprc6);
#endif
#if (VDOPRC_MAX_NUM > 7)
	isf_reg_unit(ISF_UNIT(VDOPRC,7), &isf_vdoprc7);
#endif
#if (VDOPRC_MAX_NUM > 8)
	isf_reg_unit(ISF_UNIT(VDOPRC,8), &isf_vdoprc8);
#endif
#if (VDOPRC_MAX_NUM > 9)
	isf_reg_unit(ISF_UNIT(VDOPRC,9), &isf_vdoprc9);
#endif
#if (VDOPRC_MAX_NUM > 10)
	isf_reg_unit(ISF_UNIT(VDOPRC,10), &isf_vdoprc10);
#endif
#if (VDOPRC_MAX_NUM > 11)
	isf_reg_unit(ISF_UNIT(VDOPRC,11), &isf_vdoprc11);
#endif
#if (VDOPRC_MAX_NUM > 12)
	isf_reg_unit(ISF_UNIT(VDOPRC,12), &isf_vdoprc12);
#endif
#if (VDOPRC_MAX_NUM > 13)
	isf_reg_unit(ISF_UNIT(VDOPRC,13), &isf_vdoprc13);
#endif
#if (VDOPRC_MAX_NUM > 14)
	isf_reg_unit(ISF_UNIT(VDOPRC,14), &isf_vdoprc14);
#endif
#if (VDOPRC_MAX_NUM > 15)
	isf_reg_unit(ISF_UNIT(VDOPRC,15), &isf_vdoprc15);
#endif
	//isf_vdoprc_install_id();

	return 0;
}
