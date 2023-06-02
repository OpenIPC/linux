#ifndef __module_api_h_
#define __module_api_h_
#include "ssenif_drv.h"

int nvt_dal_ssenif_api_write_reg(PSSENIF_MODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_dal_ssenif_api_write_pattern(PSSENIF_MODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_dal_ssenif_api_read_info(PSSENIF_MODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_dal_ssenif_api_read_reg(PSSENIF_MODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);

#endif
