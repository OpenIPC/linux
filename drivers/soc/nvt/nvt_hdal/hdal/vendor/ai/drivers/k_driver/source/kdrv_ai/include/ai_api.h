#ifndef __AI_API_h_
#define __AI_API_h_

#include "ai_drv.h"

#define KDRV_AI_SINGLE_LAYER_CYCLE 1
#if KDRV_AI_SINGLE_LAYER_CYCLE
#include "kdrv_ai.h"
int kdrv_ai_set_layer_cycle(KDRV_AI_ENG eng, UINT32 cycle);
UINT32 kdrv_ai_drv_get_net_supported_num(VOID);
#endif
int nvt_ai_api_write_reg(PAI_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_ai_api_write_pattern(PAI_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_kdrv_ai_api_test(PAI_INFO pmodule_info, unsigned char argc, char** pargv);
int nvt_ai_api_read_reg(PAI_INFO pmodule_info, unsigned char ucargc, char **pucargv);
int nvt_kdrv_ai_module_test(PAI_INFO pmodule_info, unsigned char argc, char **pargv);
int nvt_kdrv_ai_reset(PAI_INFO pmodule_info, unsigned char argc, char **pargv);

#endif
