#ifndef __dispobj_api_h_
#define __dispobj_api_h_
#include "display_obj_drv.h"

int nvt_dispobj_api_write_reg(PMODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_dispobj_api_write_pattern(PMODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_dispobj_api_read_reg(PMODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_dispobj_api_lcd_open(PMODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_dispobj_api_tv_open(PMODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_dispobj_api_fill_background(PMODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_dispobj_api_fill_video(PMODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_dispobj_api_fill_video2(PMODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_dispobj_api_fill_osd(PMODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_dispobj_api_v1_scaling(PMODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_dispobj_api_v2_scaling(PMODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_dispobj_api_osd_scaling(PMODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
#endif
