#ifndef __USB_CHRGDET_API_H_
#define __USB_CHRGDET_API_H_
#include "usb_chrgdet_drv.h"

int nvt_usb_chrgdet_api_write_reg(PUSB_CHRGDET_MODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_usb_chrgdet_api_write_pattern(PUSB_CHRGDET_MODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_usb_chrgdet_api_read_reg(PUSB_CHRGDET_MODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);

#endif
