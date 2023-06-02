#ifndef __ide_api_h_
#define __ide_api_h_
#include "ide_drv.h"

int nvt_ide_api_write_reg(PMODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_ide_api_write_pattern(PMODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_ide_api_read_reg(PMODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_ide_api_dump(PMODULE_INFO pmodule_info, unsigned char ucargc, char **pucargv);

#endif
