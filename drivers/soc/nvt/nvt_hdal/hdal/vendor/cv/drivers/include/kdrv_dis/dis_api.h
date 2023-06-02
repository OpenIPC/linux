#ifndef __DIS_API_H_
#define __DIS_API_H_
#include "dis_drv.h"

//#define DEBUG_KDRV_DIS


#ifdef DEBUG_KDRV_DIS
int nvt_dis_api_write_pattern(PDIS_MODULE_INFO pmodule_info, unsigned char ucargc, char **p_ucargv);
int nvt_kdrv_ipp_api_dis_test(PDIS_MODULE_INFO pmodule_info, unsigned char argc, char **p_argv);
int nvt_kdrv_ipp_api_dis_test2(PDIS_MODULE_INFO pmodule_info, unsigned char argc, char **p_argv);
#endif

int nvt_dis_api_write_reg(PDIS_MODULE_INFO pmodule_info, unsigned char ucargc, char **p_ucargv);
int nvt_dis_api_read_reg(PDIS_MODULE_INFO pmodule_info, unsigned char ucargc, char **p_ucargv);



#endif
