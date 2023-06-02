/**
 * @file kdrv_vdoout_lmt.h
 * @brief parameter limitation of KDRV VDOOUT.
 * @author ESW
 * @date in the year 2019
 */

#ifndef __KDRV_VDOOUT_LIMIT_H__
#define __KDRV_VDOOUT_LIMIT_H__

#include "comm/drv_lmt.h"
#include "kdrv_type.h"

#define KDRV_VDOOUT_MAX_WIDTH                                  (4096)
#define KDRV_VDOOUT_MAX_HIGH                                   (2048)

// Video 1 window limitation
#define KDRV_VDOOUT_V1_WIN_WIDTH_ALIGN					0x00000001					// unit: pixel
#define KDRV_VDOOUT_V1_WIN_HIGH_ALIGN					0x00000001					// unit: line

// Video 2 window limitation
#define KDRV_VDOOUT_V2_WIN_WIDTH_ALIGN					0x00000001					// unit: pixel
#define KDRV_VDOOUT_V2_WIN_HIGH_ALIGN					0x00000001					// unit: line

// OSD 1 window limitation
#define KDRV_VDOOUT_O1_WIN_WIDTH_ALIGN					0x00000001					// unit: pixel
#define KDRV_VDOOUT_O1_WIN_HIGH_ALIGN					0x00000001					// unit: line


// Video 1 buffer limitation
#define KDRV_VDOOUT_V1_YUV422_BUF_WIDTH_ALIGN			0x00000002					// unit: pixel
#define KDRV_VDOOUT_V1_YUV422_BUF_HIGH_ALIGN			0x00000001					// unit: line
#define KDRV_VDOOUT_V1_YUV420_BUF_WIDTH_ALIGN			0x00000002					// unit: pixel
#define KDRV_VDOOUT_V1_YUV420_BUF_HIGH_ALIGN			0x00000002					// unit: line

#define KDRV_VDOOUT_V1_BUF_LOFF_ALIGN					DRV_LIMIT_ALIGN_WORD		// unit: byte

#define KDRV_VDOOUT_V1_ADDR_ALIGN						DRV_LIMIT_ALIGN_BYTE

// Video 2 buffer limitation
#define KDRV_VDOOUT_V2_YUV422_BUF_WIDTH_ALIGN			0x00000002					// unit: pixel
#define KDRV_VDOOUT_V2_YUV422_BUF_HIGH_ALIGN			0x00000001					// unit: line
#define KDRV_VDOOUT_V2_YUV420_BUF_WIDTH_ALIGN			0x00000002					// unit: pixel
#define KDRV_VDOOUT_V2_YUV420_BUF_HIGH_ALIGN			0x00000002					// unit: line

#define KDRV_VDOOUT_V2_BUF_LOFF_ALIGN					DRV_LIMIT_ALIGN_WORD		// unit: byte

#define KDRV_VDOOUT_V2_ADDR_ALIGN						DRV_LIMIT_ALIGN_BYTE

// OSD buffer limitation
#define KDRV_VDOOUT_O1_BUF_WIDTH_ALIGN					0x00000001					// unit: pixel
#define KDRV_VDOOUT_O1_BUF_HIGH_ALIGN					0x00000001					// unit: line
#define KDRV_VDOOUT_O1_BUF_LOFF_ALIGN					DRV_LIMIT_ALIGN_WORD		// unit: byte

#define KDRV_VDOOUT_O1_ADDR_ALIGN						DRV_LIMIT_ALIGN_4WORD

#endif
