/*
 * uvc_ait_xu.h- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */

#ifndef _UVC_AIT_XU_H_
#define _UVC_AIT_XU_H_

#include <linux/usb/video.h>
#include "u_uvc.h"

/*************************************************************/
/*															 */
/*						  MACROS							 */
/*															 */
/*************************************************************/
/* ----------------------
* AIT XU descriptor GUID.
*/
#define USE_AIT_XU

#define UUID_LE_AIT(a, b, c, d0, d1, d2, d3, d4, d5, d6, d7)                   \
	{                                                                      \
		(a) & 0xff, ((a) >> 8) & 0xff, ((a) >> 16) & 0xff,             \
			((a) >> 24) & 0xff, (b)&0xff, ((b) >> 8) & 0xff,       \
			(c)&0xff, ((c) >> 8) & 0xff, (d0), (d1), (d2), (d3),   \
			(d4), (d5), (d6), (d7)                                 \
	}

#define UUID_BE_AIT(a, b, c, d0, d1, d2, d3, d4, d5, d6, d7)                   \
	{                                                                      \
		((a) >> 24) & 0xff, ((a) >> 16) & 0xff, ((a) >> 8) & 0xff,     \
			(a)&0xff, ((b) >> 8) & 0xff, (b)&0xff,                 \
			((c) >> 8) & 0xff, (c)&0xff, (d0), (d1), (d2), (d3),   \
			(d4), (d5), (d6), (d7)                                 \
	}

#define UVC_AIT_EU1_GUID                                                       \
	UUID_LE_AIT(0x23E49ED0, 0x1178, 0x4f31, 0xAE, 0x52, 0xD2, 0xFB, 0x8A,  \
		    0x8D, 0x3B, 0x48)
#define UVC_CUS_EU2_GUID                                                       \
	UUID_LE_AIT(0x2C49D16A, 0x32B8, 0x4485, 0x3E, 0xA8, 0x64, 0x3A, 0x15,  \
		    0x23, 0x62, 0xF2)
#define UVC_MS_EU3_GUID                                                        \
	UUID_LE_AIT(0x0F3F95DC, 0x2632, 0x4C4E, 0x92, 0xC9, 0xA0, 0x47, 0x82,  \
		    0xF4, 0x3B, 0xC8)

typedef struct tagKSCAMERA_METADATA_ITEMHEADER {
	uint32_t MetadataId;
	uint32_t Size; // Size of this header + metadata payload following
} KSCAMERA_METADATA_ITEMHEADER, *PKSCAMERA_METADATA_ITEMHEADER;

typedef enum {
	MetadataId_Standard_Start = 1,
	MetadataId_PhotoConfirmation = MetadataId_Standard_Start,
	MetadataId_UsbVideoHeader,
	MetadataId_CaptureStats,
	MetadataId_CameraExtrinsics,
	MetadataId_CameraIntrinsics,
	MetadataId_FrameIllumination,
	MetadataId_Standard_End = MetadataId_FrameIllumination,
	MetadataId_Custom_Start = 0x80000000,
} KSCAMERA_MetadataId;

typedef struct tagKSCAMERA_METADATA_FRAMEILLUMINATION {
	KSCAMERA_METADATA_ITEMHEADER Header;
	uint32_t Flags;
	uint32_t Reserved;
} KSCAMERA_METADATA_FRAMEILLUMINATION, *PKSCAMERA_METADATA_FRAMEILLUMINATION;

#define KSCAMERA_METADATA_FRAMEILLUMINATION_FLAG_ON 0x00000001
#define KSCAMERA_METADATA_FRAMEILLUMINATION_FLAG_OFF 0x00000000

#endif //_UVC_AIT_XU_H_
