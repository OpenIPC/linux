#ifndef __csi_api_h_
#define __csi_api_h_
#include "csi_drv.h"

int nvt_csi_api_write_reg(PMODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_csi_api_write_pattern(PMODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_csi_api_read_reg(PMODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);

#endif
