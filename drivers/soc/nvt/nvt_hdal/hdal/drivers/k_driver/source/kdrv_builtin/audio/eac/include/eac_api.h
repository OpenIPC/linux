#ifndef __EAC_API_H_
#define __EAC_API_H_
#include "eac_drv.h"

#define NVT_EAC_TEST_CMD 0

int nvt_eac_api_write_reg(PEAC_MODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_eac_api_write_pattern(PEAC_MODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_eac_api_read_reg(PEAC_MODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);

#endif
