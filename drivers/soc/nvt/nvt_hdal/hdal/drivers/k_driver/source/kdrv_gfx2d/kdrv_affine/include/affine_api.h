#ifndef __module_api_h_
#define __module_api_h_
#include "affine_drv.h"

int nvt_affine_api_auto_test(PMODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
//int nvt_affine_api_write_reg(PMODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_affine_api_write_pattern(PMODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
//int nvt_affine_api_read_reg(PMODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);

#endif
