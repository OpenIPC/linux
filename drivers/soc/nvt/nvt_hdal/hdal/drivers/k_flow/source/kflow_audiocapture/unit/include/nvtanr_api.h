#ifndef _NVTANRAPI_H
#define _NVTANRAPI_H

#if defined __UITRON || defined __ECOS
#include "Type.h"
#else
#ifdef __KERNEL__
#include "kwrap/type.h"
#include "kwrap/platform.h"
#else
#include "kwrap/type.h"
#include "kwrap/platform.h"
#endif
#endif

typedef enum {
	NVTANR_CONFIG_ID_NRDB,      		///< The maximum suppression level of noise. Value range is 3~35 dB.
	NVTANR_CONFIG_ID_HPF_FREQ,    		///< The Cut-Off frequency of HPF pre-filtering input PCM.
	NVTANR_CONFIG_ID_BIAS_SENSITIVE,    ///< Sensitive. Value range is 1~9.
	NVTANR_CONFIG_ID_TONE_MIN_TIME,     ///< Allowable time length of Tone noise.(Unit is 0.1 second)
	ENUM_DUMMY4WORD(NVTANR_CONFIG_ID)
} NVTANR_CONFIG_ID;


extern ER nvtanr_open(INT32 samplerate, INT32 channelnum);
extern ER nvtanr_close(void);
extern ER nvtanr_apply(UINT32 inaddr ,UINT32 insize);
extern void nvtanr_enable(BOOL enable);
extern void nvtanr_setconfig(NVTANR_CONFIG_ID config_id, INT32 value);
extern INT32 nvtanr_getconfig(NVTANR_CONFIG_ID config_id);
extern UINT32 nvtanr_getmem(INT32 samplerate, INT32 channelnum);
extern void nvtanr_setmem(UINT32 addr, UINT32 size);


#endif
