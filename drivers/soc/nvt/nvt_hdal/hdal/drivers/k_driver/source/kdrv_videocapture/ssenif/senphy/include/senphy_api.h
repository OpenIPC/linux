#ifndef __SENPHY_API_H_
#define __SENPHY_API_H_
#include "senphy_drv.h"

int nvt_senphy_api_write_reg(PSENPHY_MODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_senphy_api_write_pattern(PSENPHY_MODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_senphy_api_read_reg(PSENPHY_MODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);

#endif
