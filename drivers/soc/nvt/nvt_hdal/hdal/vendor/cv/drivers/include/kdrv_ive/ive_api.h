#ifndef __IVE_API_h_
#define __IVE_API_h_

#include "ive_drv.h"

int nvt_ive_api_write_reg(PMODULE_INFO pmodule_info, unsigned char ucargc, char** pucargv);
int nvt_ive_api_write_pattern(PMODULE_INFO pmodule_info, unsigned char ucargc, char** pucargv);
int nvt_ive_api_read_reg(PMODULE_INFO pmodule_info, unsigned char ucargc, char** pucargv);
int nvt_kdrv_ipp_api_ive_test(PMODULE_INFO pmodule_info, unsigned char argc, char** pargv);

#endif
