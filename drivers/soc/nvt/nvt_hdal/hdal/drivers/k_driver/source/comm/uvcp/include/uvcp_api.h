#ifndef __UVCP_API_H_
#define __UVCP_API_H_
#include "uvcp_drv.h"

int nvt_uvcp_api_write_reg(PUVCP_MODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_uvcp_api_write_pattern(PUVCP_MODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_uvcp_api_read_reg(PUVCP_MODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);

#endif
