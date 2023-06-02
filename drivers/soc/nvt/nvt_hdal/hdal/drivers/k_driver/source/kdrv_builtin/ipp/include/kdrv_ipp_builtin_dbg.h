#ifndef __BUILTIN_IPP_DBG_H_
#define __BUILTIN_IPP_DBG_H_

#define __MODULE__	builtin_ipp
#define __DBGLVL__	8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER

#if defined(__LINUX)
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/string.h>
#elif defined(__FREERTOS)
#include <stdio.h>
#include <string.h>
#include "kwrap/error_no.h"
#ifndef unlikely
#define unlikely(x) (x)
#endif

#else
//#error Not supported OS
#endif

#include "kwrap/debug.h"
#include "kwrap/perf.h"
#include "comm/hwclock.h"

extern unsigned int builtin_ipp_debug_level;

#define KDRV_IPP_BUILTIN_DBG_EN (1)
#define KDRV_IPP_BUILTIN_INPUT_PATH_MAX  (6)
#define KDRV_IPP_BUILTIN_OUTPUT_PATH_MAX (6)

typedef struct{

	BOOL by_pass_algo_en;     /* To exam IQ/3A cause broken image or not */
	BOOL pause_ipp_output_en; /* For frame dump by cpu */
	BOOL stamp_dbg_mode_en;   /* To exam IPP cause broken image or not */

} KDRV_IPP_BUILTIN_DBG_CTRL;


typedef enum {
	DBG_BY_PASS_ALGO = 0,
	DBG_STAMP_PASTE,
	DBG_STREAM_INPUT_SWITCH,
	DBG_RESTORE_LAST_FRAME_SAVE,
	DBG_MAX
} KDRV_IPP_BUILTIN_DBG_METHOD;


UINT32 kdrv_ipp_builtin_dbg(void *p_hdl, UINT32 param_id, void *p_data);

#endif

