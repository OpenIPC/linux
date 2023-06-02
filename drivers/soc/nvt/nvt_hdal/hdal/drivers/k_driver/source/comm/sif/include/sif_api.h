#ifndef __sif_api_h_
#define __sif_api_h_
#include "sif_drv.h"

#define NVT_SIF_TEST_CMD 0

int nvt_xxx_api_write_reg(PMODULE_INFO pmodule_info, unsigned char ucargc, char** pucargv);
int nvt_xxx_api_write_pattern(PMODULE_INFO pmodule_info, unsigned char ucargc, char** pucargv);
int nvt_xxx_api_read_reg(PMODULE_INFO pmodule_info, unsigned char ucargc, char** pucargv);

#endif
