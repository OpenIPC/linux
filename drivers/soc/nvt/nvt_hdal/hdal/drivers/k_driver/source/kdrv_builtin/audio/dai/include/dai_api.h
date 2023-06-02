#ifndef __dai_api_h_
#define __dai_api_h_
#include "dai_drv.h"

#define NVT_DAI_TEST_CMD 0

int nvt_dai_api_write_reg(PDAI_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_dai_api_write_pattern(PDAI_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_dai_api_read_reg(PDAI_INFO pmodule_info, unsigned char ucargc, char **pucargv);

#endif
