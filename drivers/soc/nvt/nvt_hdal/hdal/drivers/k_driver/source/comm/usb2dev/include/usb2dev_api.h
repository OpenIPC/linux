#ifndef __USB2DEV_API_H_
#define __USB2DEV_API_H_
#include "usb2dev_drv.h"

int nvt_usb2dev_api_write_reg(PUSB2DEV_MODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_usb2dev_api_write_pattern(PUSB2DEV_MODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_usb2dev_api_read_reg(PUSB2DEV_MODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);

#endif
