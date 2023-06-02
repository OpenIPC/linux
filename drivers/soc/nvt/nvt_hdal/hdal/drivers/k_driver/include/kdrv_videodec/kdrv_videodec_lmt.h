/**
 * @file kdrv_videodec_lmt.h
 * @brief type definition of KDRV API.
 * @author ALG2
 * @date in the year 2019
 */
#ifndef __KDRV_VIDEODEC_LMT_H__
#define __KDRV_VIDEODEC_LMT_H__

#include "comm/drv_lmt.h"

#define H264D_WIDTH_MIN			(320)
#define H264D_HEIGHT_MIN		(240)
#define H264D_WIDTH_MAX			(2688)
#define H264D_HEIGHT_MAX		(2688)
#define H264D_WIDTH_ALIGN		(DRV_LIMIT_ALIGN_4WORD)
#define H264D_HEIGHT_ALIGN		(DRV_LIMIT_ALIGN_2WORD)

#define H265D_WIDTH_MIN			(320)
#define H265D_HEIGHT_MIN		(240)
#define H265D_WIDTH_MAX			(5120)
#define H265D_HEIGHT_MAX		(2688)
#define H265D_WIDTH_ALIGN		(DRV_LIMIT_ALIGN_4WORD)
#define H265D_HEIGHT_ALIGN		(DRV_LIMIT_ALIGN_2WORD)
#define H265D_TILE_MAX			(3)

#define H264D_WIDTH_MIN_528		(240)	//min decode width
#define H264D_HEIGHT_MIN_528		(240)	//min decode height
#define H264D_WIDTH_MAX_528		(5120)
#define H264D_HEIGHT_MAX_528		(5120)

#define H265D_WIDTH_MIN_528		(240)	//min decode width
#define H265D_HEIGHT_MIN_528		(240)	//min decode height
#define H265D_WIDTH_MAX_528		(5120)
#define H265D_HEIGHT_MAX_528		(5120)
#define H265D_TILE_MAX_528		(4)

#define H265D_MAX_WIDTH_WITHOUT_TILE_528	(2176)

#define H26XD_BUF_ADDR_ALIGN	(DRV_LIMIT_ALIGN_WORD)
#define H26XD_BS_BUF_ADDR_ALIGN	(DRV_LIMIT_ALIGN_WORD)

#endif	// __KDRV_VIDEODEC_LMT_H__
