#ifndef __dsi_api_h_
#define __dsi_api_h_
#include "dsi_drv.h"

int nvt_dsi_api_write_reg(PMODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_dsi_api_write_pattern(PMODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_dsi_api_read_reg(PMODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);

#endif
