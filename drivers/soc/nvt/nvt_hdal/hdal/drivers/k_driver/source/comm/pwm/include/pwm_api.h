#ifndef __pwm_api_h_
#define __pwm_api_h_
#include "pwm_drv.h"

int nvt_pwm_api_write_reg(PMODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_pwm_api_write_pattern(PMODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_pwm_api_read_reg(PMODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_pwm_api_open_pwm(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv);
int nvt_pwm_api_close_pwm(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv);
int nvt_pwm_api_open_mstep(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv);
int nvt_pwm_api_close_mstep(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv);

#endif
