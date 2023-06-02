#ifndef __LVDS_API_H_
#define __LVDS_API_H_
#include "lvds_drv.h"

int nvt_lvds_api_write_reg(PLVDS_MODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_lvds_api_write_pattern(PLVDS_MODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_lvds_api_read_reg(PLVDS_MODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);

#endif
