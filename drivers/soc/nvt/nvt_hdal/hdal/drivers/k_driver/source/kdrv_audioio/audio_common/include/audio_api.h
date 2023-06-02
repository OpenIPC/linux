#ifndef __audio_api_h_
#define __audio_api_h_
#include "audio_drv.h"

#define NVT_AUDIO_TEST_CMD 0

int nvt_audio_api_write_reg(PMODULE_INFO pmodule_info, unsigned char ucargc, char** pucargv);
int nvt_audio_api_write_pattern(PMODULE_INFO pmodule_info, unsigned char ucargc, char** pucargv);
int nvt_audio_api_read_reg(PMODULE_INFO pmodule_info, unsigned char ucargc, char** pucargv);
int nvt_audio_api_write_playpattern(PMODULE_INFO pmodule_info, unsigned char argc, char** pargv);
int nvt_audio_api_dumpinfo(PMODULE_INFO pmodule_info, unsigned char argc, char** pargv);
int nvt_audio_api_enable_dai_dbg(PMODULE_INFO pmodule_info, unsigned char argc, char** pargv);
int nvt_audio_api_dumpinfo_obj(PMODULE_INFO pmodule_info, unsigned char argc, char** pargv);


#endif
