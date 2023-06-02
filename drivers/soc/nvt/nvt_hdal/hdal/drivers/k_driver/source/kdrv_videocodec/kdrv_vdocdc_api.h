#ifndef _KDRV_VDOCDC_API_H_
#define _KDRV_VDOCDC_API_H_

#include "kwrap/type.h"

int kdrv_vdocdc_api_wt_sim(unsigned char argc, char **argv);
int kdrv_vdocdc_api_wt_dbg(unsigned char argc, char **argv);
int kdrv_vdocdc_api_wt_perf(unsigned char argc, char **argv);
int kdrv_vdocdc_api_wt_int(unsigned char argc, char **argv);

#endif // _KDRV_VDOCDC_API_H_