/**
 * @file kdrv_audio_lmt.h
 * @brief parameter limitation of KDRV AUDIO.
 * @author ESW
 * @date in the year 2019
 */

#ifndef __KDRV_AUDIO_LIMIT_H__
#define __KDRV_AUDIO_LIMIT_H__

#include "comm/drv_lmt.h"
#include "kdrv_type.h"

#define AUDIO_BUF_SIZE_ALIGN  (DRV_LIMIT_ALIGN_WORD)                  // unit: byte
#define AUDIO_IN_ADDR_ALIGN  (DRV_LIMIT_ALIGN_2WORD)                  // unit: byte
#define AUDIO_OUT_ADDR_ALIGN  (DRV_LIMIT_ALIGN_2WORD)                  // unit: byte



#endif
