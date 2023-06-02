/**
    Header file of vdodec library

    Exported header file of vdodec library.

    @file       dal_vdodec.h
    @ingroup    mIAVCODEC
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2009.  All rights reserved.
*/
#ifndef _DAL_VDODEC_H
#define _DAL_VDODEC_H

#if defined __UITRON || defined __ECOS
#include <stdio.h>
#include <string.h>
#include "Type.h"
#include "kernel.h"
#elif defined __KERNEL__
#include "kwrap/type.h"
#include "kwrap/platform.h"
//#include <linux/module.h>
#include <linux/string.h>
#include <linux/kernel.h>

#include <linux/printk.h>
#define debug_msg        printk
#else
#include "kwrap/type.h"
#include "kwrap/platform.h"
#endif
#include "jpg_header.h"

#define DAL_VDODEC_SHOW_MSG								0
#define DAL_VDODEC_PREDEC_FRMNUM						3

/**
    Video Decoder ID
*/
typedef enum {
	DAL_VDODEC_ID_1,                			        ///< video decoder ID 1
	DAL_VDODEC_ID_MAX,              			        ///< video decoder ID maximum
	ENUM_DUMMY4WORD(DAL_VDODEC_ID)
} DAL_VDODEC_ID;

/**
    Type of getting information from video decoding codec library.
*/
typedef enum {
	ENUM_DUMMY4WORD(DAL_VDODEC_GET_ITEM)
} DAL_VDODEC_GET_ITEM;

/**
    Type of setting information to video decoding codec library.
*/
typedef enum {
	ENUM_DUMMY4WORD(DAL_VDODEC_SET_ITEM)
} DAL_VDODEC_SET_ITEM;

typedef struct {
	void (*VdoDec_RefFrmDo)(UINT32 pathID, UINT32 uiYAddr, BOOL bIsRef);
	UINT32 id;
} DAL_VDODEC_REFFRMCB;

/**
    Init structure
*/
typedef struct {
	UINT32 uiWidth;										///<[out] decode image width
	UINT32 uiHeight;									///<[out] decode image heght
	UINT32 uiDescAddr;									///<[in] description addr
	UINT32 uiDescSize;									///<[in] description size
	UINT32 uiDecBufAddr;								///<[in] decoder internal buffer starting address
	UINT32 uiDecBufSize;								///<[in] decoder internal buffer size
	UINT32 uiDisplayWidth;								///<[out] display source width, e.g. 720 x 480, uiWidth = 736, uiDisplayWidth = 720
} DAL_VDODEC_INIT;

/**
    Decode structure
*/
typedef struct {
	UINT32 uiBsAddr;									///<[in] bit-stream address
	UINT32 uiBsSize;									///<[in] bit-stream size
	UINT32 uiCurBsSize;									///<[in] current bit-stream size
	UINT32 uiRawYAddr;									///<[in] raw y address
	UINT32 uiRawUVAddr;									///<[in] raw uv address
	UINT32 uiJpegHdrAddr;                                                     /// <[in] jpeg hdr address
	UINT32 uiWidth;										///<[out] image width
	UINT32 uiHeight;									///<[out] image heght
	UINT32 uiYuvFmt;									///<[out] yuv format
	ER     dec_er;										///<[out] decoded error code
	UINT32 uiInterrupt;									///<[out] hw interrupt
	DAL_VDODEC_REFFRMCB vRefFrmCb;
} DAL_VDODEC_PARAM;

typedef struct {
	BOOL   present_flag;													///< VUI present flag, 0 : vui not encoded, 1 : vui encoded and parameter return as below.
	UINT32 sar_width;                                                       ///< Horizontal size of the sample aspect ratio. default: 0, range: 0~65535
	UINT32 sar_height;                                                      ///< Vertical size of the sample aspect rat. default: 0, range: 0~65535
	UINT8 matrix_coef;                                                      ///< Matrix coefficients are used to derive the luma and Chroma signals from green, blue, and red primaries. default: 2, range: 0~255
	UINT8 transfer_characteristics;                                         ///< The opto-electronic transfers characteristic of the source pictures. default: 2, range: 0~255
	UINT8 colour_primaries;                                                 ///< Chromaticity coordinates the source primaries. default: 2, range: 0~255
	UINT8 video_format;                                                     ///< Indicate the representation of pictures. default: 5, range: 0~7
	UINT8 color_range;                                                      ///< Indicate the black level and range of the luma and Chroma signals. default: 0, range: 0~1 (0: Not full range, 1: Full range)
	BOOL timing_present_flag;                                               ///< timing info present flag. default: 0, range: 0~1 (0: disable, 1: enable)
} DAL_VDODEC_VUI_INFO;

typedef struct {
	//INT32 (* p_jpg_scalar_handler)(PJPGHEAD_DEC_CFG, UINT32 *);              ///< [in]The callback function for JPEG scale handling.
	UINT8               *p_src_addr;               ///< [in]Source bitstream address.
	UINT8               *p_dst_addr;               ///< [out]Destination YUV raw image buffer address.
	UINT32              jpg_file_size;             ///< [in]Source bitstream size.
	PJPGHEAD_DEC_CFG    p_dec_cfg;                 ///< [out]JPEG decode configuration parameters.
	UINT32              out_addr_y;              ///< [out]Output raw image Y buffer address in p_dst_addr.
	UINT32              out_addr_uv;             ///< [out]Output raw image UV buffer address in p_dst_addr.
	JPG_DEC_TYPE        decode_type;              ///< [in]Decoding type, refer to JPG_DEC_TYPE.
	BOOL                speed_up_sn;            ///< [in]Speed up Screennail.
	BOOL                enable_timeout;          ///< [in]Enable time out mechanism.
	void (*timer_start_cb)(void);                ///< [in]Timer start callback function.
	void (*timer_pause_cb)(void);                ///< [in]Timer pause callback function.
	void (*timer_wait_cb)(void);                 ///< [in]Timer wait callback function.
	PARSE_EXIF_CB parse_exif_cb;				 ///< [in]If decode_type is DEC_THUMBNAIL, this callback is needed to handle exif parser.
} DAL_VDODEC_JPGDECINFO;

typedef struct {
	UINT32 uiHdrBsAddr;		///< [input]  : decode sps/pps heeader biststream addr
	UINT32 uiHdrBsLen;		///< [input]  : decode sps/pps heeader biststream size
	UINT32 uiWidth;			///< [output] : return width
	UINT32 uiHeight;		///< [output] : return height
	UINT32 uiMemSize;		///< [output] : return memory requirement size
} DAL_VDODEC_MEMINFO;

#endif
