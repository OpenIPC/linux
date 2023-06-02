/**
    @brief Header file of hdal videosprite module.\n
    This file contains the functions which is related to hdal videosprite in the chip.

    @file hd_videosprite.h

    @ingroup videosprite

    @note Nothing.

    Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#ifndef _HD_VIDEOSPRITE_H_
#define _HD_VIDEOSPRITE_H_

#include "hd_type.h"
#include "videosprite/videosprite.h"

typedef enum {
	VDS_PARAM_ID_NULL,
	VDS_PARAM_ID_STAMP_BUF,
	VDS_PARAM_ID_STAMP_IMG,
	VDS_PARAM_ID_STAMP_ATTR,
	VDS_PARAM_ID_MASK_ATTR,
	VDS_PARAM_ID_MOSAIC_ATTR,
	VDS_PARAM_ID_MAX,
	ENUM_DUMMY4WORD(VDS_PARAM_ID)
} VDS_PARAM_ID;

int vds_enc_start(VDS_PHASE phase, UINT32 rgn, UINT32 vid);
int vds_enc_stop(VDS_PHASE phase, UINT32 rgn, UINT32 vid);
int vds_enc_get(VDS_PHASE phase, UINT32 rgn, UINT32 vid, HD_VIDEOENC_PARAM_ID id, VOID *p_param);
int vds_enc_set(VDS_PHASE phase, UINT32 rgn, UINT32 vid, HD_VIDEOENC_PARAM_ID id, VOID *p_param);
int vds_enc_close(VDS_PHASE phase, UINT32 rgn, UINT32 vid);

int vds_vp_start(VDS_PHASE phase, UINT32 rgn, UINT32 vid);
int vds_vp_stop(VDS_PHASE phase, UINT32 rgn, UINT32 vid);
int vds_vp_get(VDS_PHASE phase, UINT32 rgn, UINT32 vid, HD_VIDEOPROC_PARAM_ID id, VOID *param);
int vds_vp_set(VDS_PHASE phase, UINT32 rgn, UINT32 vid, HD_VIDEOPROC_PARAM_ID id, VOID *param);
int vds_vp_close(VDS_PHASE phase, UINT32 rgn, UINT32 vid);

#endif //_HD_VIDEOSPRITE_H_