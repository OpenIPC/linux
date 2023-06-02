#ifndef __module_api_h_
#define __module_api_h_
#include "ddr_arb_drv.h"

int nvt_ddr_arb_api_auto_test(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv);
//int nvt_ddr_arb_api_write_reg(PMODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_ddr_arb_api_write_pattern(PMODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
//int nvt_ddr_arb_api_read_reg(PMODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);

#endif
