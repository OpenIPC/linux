#ifndef __dispdev_api_h_
#define __dispdev_api_h_
#include "dispdev_drv.h"

int nvt_dispdev_api_write_reg(PMODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_dispdev_api_write_pattern(PMODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_dispdev_api_read_reg(PMODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);

#endif
