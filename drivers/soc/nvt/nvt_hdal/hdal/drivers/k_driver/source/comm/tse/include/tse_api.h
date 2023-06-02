#ifndef __TSE_API_H_
#define __TSE_API_H_
#include "tse_drv.h"

#define NVT_TSE_TEST_CMD 0

int nvt_tse_set_dbglevel(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv);
int nvt_tse_get_dbglevel(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv);

//self test command
int nvt_tse_reset(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv);
int nvt_tse_mem_set_test(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv);
int nvt_tse_mem_cpy_test(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv);
int nvt_tse_mux_demux_test(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv);

#endif //__TSE_API_H_
