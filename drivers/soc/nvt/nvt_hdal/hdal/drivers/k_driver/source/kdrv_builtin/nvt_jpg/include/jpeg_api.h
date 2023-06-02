#ifndef __jpeg_api_h_
#define __jpeg_api_h_
#include "jpeg_drv.h"

int nvt_jpg_api_write_reg(PJPG_MODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_jpg_api_write_pattern(PJPG_MODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_jpg_api_read_reg(PJPG_MODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);

#endif
