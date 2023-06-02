#ifndef _REMOTE_API_H_
#define _REMOTE_API_H_
#include "remote_drv.h"

int nvt_remote_api_write_reg(PREMOTE_MODULE_INFO pmodule_info, unsigned char ucargc, char** pucargv);
int nvt_remote_api_write_receiver_test(PREMOTE_MODULE_INFO pmodule_info, unsigned char argc, char** pargv);
int nvt_remote_api_write_chg_clk_src(PREMOTE_MODULE_INFO pmodule_info, unsigned char argc, char** pargv);
int nvt_remote_api_write_pattern(PREMOTE_MODULE_INFO pmodule_info, unsigned char ucargc, char** pucargv);
int nvt_remote_api_read_reg(PREMOTE_MODULE_INFO pmodule_info, unsigned char ucargc, char** pucargv);

#endif