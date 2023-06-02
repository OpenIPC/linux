#ifndef __md_api_h_
#define __md_api_h_
#include "md_drv.h"

int nvt_md_api_write_reg(PMD_MODULE_INFO pmodule_info, unsigned char ucargc, char** pucargv);
int nvt_md_api_write_pattern(PMD_MODULE_INFO pmodule_info, unsigned char ucargc, char** pucargv);
int nvt_kdrv_md_api_test(PMD_MODULE_INFO pmodule_info, unsigned char argc, char** pargv);
int nvt_md_api_read_reg(PMD_MODULE_INFO pmodule_info, unsigned char ucargc, char** pucargv);

#endif
