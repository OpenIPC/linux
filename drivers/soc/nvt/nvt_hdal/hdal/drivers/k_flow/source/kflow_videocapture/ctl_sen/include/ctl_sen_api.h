#ifndef __CTL_SEN_API_h_
#define __CTL_SEN_API_h_
#include "ctl_sen_drv.h"
#include "kwrap/sxcmd.h"
#include "kflow_videocapture/ctl_sen.h"

#define CTL_SEN_PROC_NAME "ctl_sen"
#define CTL_SEN_CEIL_64(n) ((((n) + 63) >> 6) << 6) // for memory 64 byte alignment

#define CTL_SEN_TEST DISABLE

#if defined(__FREERTOS) || defined(__ECOS) || defined(__UITRON)
#else
int ctl_sen_cmd_execute(unsigned char argc, char **argv);
#endif

#endif
