/*
    Header file for JPEG module

    This file is the header file that define the API for JPEG module.

    @file       jpeg.h
    @ingroup    mIDrvCodec_JPEG
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#ifndef _JPEG_H
#define _JPEG_H

#include "kwrap/type.h"
#include "kdrv_videoenc/kdrv_videoenc.h"
#include "kdrv_videodec/kdrv_videodec.h"
#include "kdrv_jpeg_queue.h"

/**
    @addtogroup mIDrvCodec_JPEG
*/
//@{

/**
    JPEG block format

    @note For JPG_HW_YUV_FORMAT
*/
typedef enum {
	JPEG_BLK_Y_2X    = 0x0800,      ///< Y's block width is 16
	JPEG_BLK_Y_1X    = 0x0400,      ///< Y's block width is 8
	JPEG_BLK_Y_2Y    = 0x0200,      ///< Y's block height is 16
	JPEG_BLK_Y_1Y    = 0x0100,      ///< Y's block height is 8
	JPEG_BLK_U_2X    = 0x0080,      ///< U's block width is 16
	JPEG_BLK_U_1X    = 0x0040,      ///< U's block width is 8
	JPEG_BLK_U_2Y    = 0x0020,      ///< U's block height is 16
	JPEG_BLK_U_1Y    = 0x0010,      ///< U's block height is 8
	JPEG_BLK_V_2X    = 0x0008,      ///< V's block width is 16
	JPEG_BLK_V_1X    = 0x0004,      ///< V's block width is 8
	JPEG_BLK_V_2Y    = 0x0002,      ///< V's block height is 16
	JPEG_BLK_V_1Y    = 0x0001,      ///< V's block height is 8
	ENUM_DUMMY4WORD(JPEG_BLK_FORMAT)
} JPEG_BLK_FORMAT;

/**
    JPEG YUV format

    @note For jpeg_set_format()
*/
typedef enum {
	JPEG_YUV_FORMAT_100     = (JPEG_BLK_Y_1X | JPEG_BLK_Y_1Y),                                                                  ///< YUV 100 (Y plane only)
	JPEG_YUV_FORMAT_111     = (JPEG_BLK_Y_1X | JPEG_BLK_Y_1Y | JPEG_BLK_U_1X | JPEG_BLK_U_1Y | JPEG_BLK_V_1X | JPEG_BLK_V_1Y),  ///< YUV 111
	JPEG_YUV_FORMAT_211     = (JPEG_BLK_Y_2X | JPEG_BLK_Y_1Y | JPEG_BLK_U_1X | JPEG_BLK_U_1Y | JPEG_BLK_V_1X | JPEG_BLK_V_1Y),  ///< YUV 2h11 (4:2:2 in Exif)
	JPEG_YUV_FORMAT_211V    = (JPEG_BLK_Y_1X | JPEG_BLK_Y_2Y | JPEG_BLK_U_1X | JPEG_BLK_U_1Y | JPEG_BLK_V_1X | JPEG_BLK_V_1Y),  ///< YUV 2v11
	JPEG_YUV_FORMAT_222     = (JPEG_BLK_Y_2X | JPEG_BLK_Y_1Y | JPEG_BLK_U_2X | JPEG_BLK_U_1Y | JPEG_BLK_V_2X | JPEG_BLK_V_1Y),  ///< YUV 222h
	JPEG_YUV_FORMAT_222V    = (JPEG_BLK_Y_1X | JPEG_BLK_Y_2Y | JPEG_BLK_U_1X | JPEG_BLK_U_2Y | JPEG_BLK_V_1X | JPEG_BLK_V_2Y),  ///< YUV 111v
	JPEG_YUV_FORMAT_411     = (JPEG_BLK_Y_2X | JPEG_BLK_Y_2Y | JPEG_BLK_U_1X | JPEG_BLK_U_1Y | JPEG_BLK_V_1X | JPEG_BLK_V_1Y),  ///< YUV 411  (4:2:0 in Exif)
	JPEG_YUV_FORMAT_422     = (JPEG_BLK_Y_2X | JPEG_BLK_Y_2Y | JPEG_BLK_U_2X | JPEG_BLK_U_1Y | JPEG_BLK_V_2X | JPEG_BLK_V_1Y),  ///< YUV 422h
	JPEG_YUV_FORMAT_422V    = (JPEG_BLK_Y_2X | JPEG_BLK_Y_2Y | JPEG_BLK_U_1X | JPEG_BLK_U_2Y | JPEG_BLK_V_1X | JPEG_BLK_V_2Y),  ///< YUV 422v
	ENUM_DUMMY4WORD(JPEG_YUV_FORMAT)
} JPEG_YUV_FORMAT;


/**
    JPEG CODEC Mode

    @note For jpeg_add_queue()
*/
typedef enum {
	JPEG_CODEC_MODE_ENC,          ///< JPEG encode mode
	JPEG_CODEC_MODE_DEC,          ///< JPEG decode mode

	ENUM_DUMMY4WORD(JPEG_CODEC_MODE)
} JPEG_CODEC_MODE;

/**
    JPEG interrupt status

    @note For jpeg_get_status(), jpeg_set_enableint(), jpeg_set_disableint() and jpeg_waitdone().
*/
typedef enum {
	JPEG_INT_FRAMEEND   = 0x01,         ///< Frame end. Encoding/decoding a frame is done.
	///< @note  When bit-stream buffer end and frame end are both happened,
	///<        only bit-stream buffer end interrupt will be issued.
	///<        You have to set the bit-stream buffer again to issue the frame end interrupt.
	JPEG_INT_SLICEDONE  = 0x02,         ///< Slice Done. (Not support in NT96660)
	JPEG_INT_DECERR     = 0x04,         ///< Decode error. Encounter error in JPEG decode.
	JPEG_INT_BUFEND     = 0x08,         ///< Bit-stream buffer end. Bit-stream length exceed specified length.
	JPEG_INT_DECMP_ERR  = 0x10,			///< source decompress error
	JPEG_INT_ABORTDONE	= 0x20,			///< dma abort done
	JPEG_INT_ALL        = 0x3F,         ///< All interrupts of above

	ENUM_DUMMY4WORD(JPEG_INT)
} JPEG_INT;

/**
    JPEG Decode Scale Ratio

    @note For jpeg_set_scaleratio().
*/
typedef enum {
	JPEG_DECODE_RATIO_WIDTH_1_2,        ///< 1/2 of Width Only
	JPEG_DECODE_RATIO_BOTH_1_2,         ///< 1/2 of Width/Height
	JPEG_DECODE_RATIO_BOTH_1_4,         ///< 1/4 of Width/Height
	JPEG_DECODE_RATIO_BOTH_1_8,         ///< 1/8 of Width/Height

	ENUM_DUMMY4WORD(JPEG_DECODE_RATIO)
} JPEG_DECODE_RATIO;

/**
    JPEG DC Output Scaling ratio

    JPEG DC Output scaling ratio.
*/
typedef enum {
	JPEG_DC_OUT_RATIO_1_2 = 1,          ///< DC output scaled to 1/2
	JPEG_DC_OUT_RATIO_1_4,              ///< DC output scaled to 1/4
	JPEG_DC_OUT_RATIO_1_8,              ///< DC output scaled to 1/8

	ENUM_DUMMY4WORD(JPEG_DC_OUT_RATIO)
} JPEG_DC_OUT_RATIO;

/**
    JPEG DC Output Configuration structure

    @note For jpeg_set_dcout().
*/
typedef struct {
	UINT32              dc_enable;       ///< DC output enable
	///< - @b TRUE  : Enable DC output function
	///< - @b FALSE : Disable DC output function
	JPEG_DC_OUT_RATIO   dc_xratio;       ///< DC output width (horizontal) ratio
	JPEG_DC_OUT_RATIO   dc_yratio;       ///< DC output height (vertical) ratio
	UINT32              dc_yaddr;        ///< DC output Y DRAM address
	UINT32              dc_uaddr;        ///< DC output UV packed DRAM address
	UINT32              dc_vaddr;        ///< Obsoleted member
	UINT32              dc_ylineoffset;  ///< DC output Y line offset
	UINT32              dc_ulineoffset;  ///< DC output UV packed line offset
	UINT32              dc_vlineoffset;  ///< Obsoleted member
} JPEG_DC_OUT_CFG, *PJPEG_DC_OUT_CFG;


/**    JPEG Bit stream DMA burst length    JPEG BS dma burst length.*/
typedef enum
{	JPEG_BS_DMA_BURST_32W,          ///< BS DMA burst 32W
    JPEG_BS_DMA_BURST_64W,          ///< BS DMA burst 64W
    ENUM_DUMMY4WORD(JPEG_BS_DMA_BURST)
} JPEG_BS_DMA_BURST;

/**
    JPEG Rotate mode

    @note For jpeg_set_rotate().
*/
typedef enum
{
    JPG_ROTATE_DISABLE = 0,             ///< Not rotate
    JPG_ROTATE_CCW      = 1,            ///< Rotate counter-clockwise
    JPG_ROTATE_CW     = 2,              ///< Rotate clockwise
    JPG_ROTATE_180     = 3,             ///< Rotate 180 degree clockwise

    ENUM_DUMMY4WORD(JPG_HW_ROTATE_MODE)
} JPG_HW_ROTATE_MODE;

/**
    JPEG Bit Rate Control Information

    @note For jpeg_get_brcinfo().
*/
typedef struct {
	UINT32 brcinfo1;                  ///< Rho Q/8
	UINT32 brcinfo2;                  ///< Rho Q/4
	UINT32 brcinfo3;                  ///< Rho Q/2
	UINT32 brcinfo4;                  ///< Rho Q
	UINT32 brcinfo5;                  ///< Rho 2Q
	UINT32 brcinfo6;                  ///< Rho 4Q
	UINT32 brcinfo7;                  ///< Rho 8Q
} JPEG_BRC_INFO, *PJPEG_BRC_INFO;

/**
    JPEG configuration identifier

    @note For jpeg_set_config()
*/
typedef enum {
	JPEG_CONFIG_ID_CHECK_DEC_ERR,           ///< Enable / Disable checking decode error
	///< (Default is DISABLED)
	///< Context can be:
	///< - @b TRUE  : JPEG driver will check decode error
	///< - @b FALSE : JPEG driver will NOT check decode error

	JPEG_CONFIG_ID_FREQ,                    ///< JPEG module clock (Unit: MHz), will be active on succeeding jpeg_open().
	///< Context can be one of the following:
	///< - @b 192   : 192 MHz (Default value)
	///< - @b 240   : 240 MHz
	///< - @b 250   : 250 MHz

	ENUM_DUMMY4WORD(JPEG_CONFIG_ID)
} JPEG_CONFIG_ID;


typedef void (*JPEG_TRI_FUNC)(JPEG_CODEC_MODE codec_mode, void *p_param, KDRV_CALLBACK_FUNC *p_cb_func);


typedef struct _KDRV_JPEG_TRIG_INFO {
	KDRV_CALLBACK_FUNC     *cb;
	VOID                   *user_data;
	//UINT32                  flg_ptn;
	BOOL                    is_busy;
	KDRV_JPEG_QUEUE_INFO    *p_queue;
	JPEG_TRI_FUNC           tri_func;
	KDRV_VDODEC_PARAM       *jpeg_dec_param;
	KDRV_VDOENC_PARAM       *jpeg_enc_param;	
} KDRV_JPEG_TRIG_INFO;

#define MAX_JPEG_OSG_NUM		10
#define MAX_JPEG_OSG_PAL_NUM	16

typedef struct {
	UINT8	ucRgb2Yuv[3][3];
} JPEG_OSG_RGB_CFG;

typedef struct {
	UINT8 ucAlpha;
	UINT8 ucRed;
	UINT8 ucGreen;
	UINT8 ucBlue;
} JPEG_OSG_PAL_CFG;

typedef struct {
	UINT8  ucType;
	UINT16 usWidth;
	UINT16 usHeight;
	UINT16 usLofs;
	UINT32 uiAddr;
} JPEG_OSG_GRAP_CFG;

typedef struct {
	UINT8  ucMode;
	UINT16 usXStr;
	UINT16 usYStr;
	UINT8  ucBgAlpha;
	UINT8  ucFgAlpha;
	UINT8  ucMaskType;
	UINT8  ucMaskBdSize;
	UINT8  ucMaskY[2];
	UINT8  ucMaskCb;
	UINT8  ucMaskCr;
} JPEG_OSG_DISP_CFG;

typedef struct {
	BOOL bEnable;
	BOOL bAlphaEn;
	UINT8 ucAlpha;
	UINT8 ucRed;
	UINT8 ucGreen;
	UINT8 ucBlue;
} JPEG_OSG_COLORKEY_CFG;

typedef struct {
	BOOL	bEnable;
	JPEG_OSG_GRAP_CFG	stGrap;
	JPEG_OSG_DISP_CFG	stDisp;
	JPEG_OSG_COLORKEY_CFG	stKey;
} JPEG_OSG_WIN_CFG;

// -------------------------------------------------
// The API for the JPEG codec driver
// -------------------------------------------------
extern ER       jpeg_open(void);
extern ER       jpeg_close(void);
extern BOOL   jpeg_is_opened(void);
extern ID       jpeg_getlock_status(void);

extern ER       jpeg_set_config(JPEG_CONFIG_ID cfg_id, UINT32 cfg);
extern UINT32   jpeg_get_config(JPEG_CONFIG_ID cfg_id);

extern UINT32   jpeg_get_status(void);
extern UINT32   jpeg_get_activestatus(void);
extern UINT32   jpeg_get_bssize(void);
extern void     jpeg_set_swreset(void);

extern void jpeg_set_startencode(UINT32 addr , UINT32 size);
extern void     jpeg_set_startdecode(void);

extern void     jpeg_set_endencode(void);
extern void     jpeg_set_enddecode(void);

extern UINT32   jpeg_waitdone(void);
extern BOOL     jpeg_waitdone_polling(void);

extern ER       jpeg_set_enableint(UINT32 interrupt);
extern void     jpeg_set_disableint(UINT32 interrupt);

extern void     jpeg_set_imgstartaddr(UINT32 yaddr, UINT32 uaddr, UINT32 vaddr);
extern void     jpeg_set_imgstartaddr_oft(UINT32 yaddr, UINT32 uaddr, UINT32 vaddr, INT32 y_offset, INT32 c_offset);
extern void     jpeg_get_imgstartaddr(UINT32 *yaddr, UINT32 *uaddr, UINT32 *vaddr);

extern ER       jpeg_set_imglineoffset(UINT32 YLOFS, UINT32 ULOFS, UINT32 VLOFS);
extern UINT32   jpeg_get_imglineoffsety(void);
extern UINT32   jpeg_get_imglineoffsetu(void);
extern UINT32   jpeg_get_imglineoffsetv(void);

extern ER       jpeg_set_format(UINT32 imgwidth, UINT32 imgheight, JPEG_YUV_FORMAT fmt);

// Bit-stream control
extern ER       jpeg_set_bsstartaddr(UINT32 bs_addr, UINT32 buf_size);
extern UINT32   jpeg_get_bsstartaddr(void);
extern UINT32   jpeg_get_bscurraddr(void);
extern void     jpeg_set_bsoutput(BOOL en);

// Restart marker
extern ER       jpeg_set_restartinterval(UINT32 mcu_num);
extern ER       jpeg_set_restartenable(BOOL en);
extern UINT32   jpeg_set_restarteof(void);
extern void     jpeg_set_restartpatch(BOOL en);

// Encode format transform (420 -> 422)
extern void     jpeg_set_fmttransenable(void);
extern void     jpeg_set_fmttransdisable(void);

// Encode DC output
extern ER       jpeg_set_dcout(PJPEG_DC_OUT_CFG pdcout_cfg);

// Decode crop
extern ER       jpeg_set_crop(UINT32 sta_x, UINT32 sta_y, UINT32 width, UINT32 height);
extern void     jpeg_set_cropdisable(void);
extern void     jpeg_set_cropenable(void);

// Decode scaling
extern void     jpeg_set_scaleenable(void);
extern void     jpeg_set_scaledisable(void);
extern ER       jpeg_set_scaleratio(JPEG_DECODE_RATIO scale_ratio);

// Rotate
extern ER       jpeg_set_rotate(JPG_HW_ROTATE_MODE RotateMode);// IMGDMA
extern ER       jpeg_set_newimgdma(BOOL en);// BSDMA
extern ER       jpeg_set_burstlen(JPEG_BS_DMA_BURST bslen);

// Table access
extern ER       jpeg_set_hwqtable(UINT8 *pqtab_y, UINT8 *pqtab_uv);
extern void     jpeg_enc_set_hufftable(UINT16 *phufftablumac, UINT16 *phufftablumdc, UINT16 *phufftabchrac, UINT16 *phufftabchrdc);
extern void     jpeg_set_decode_hufftabhw(UINT8 *phuffdc0th, UINT8 *phuffdc1th, UINT8 *phuffac0th, UINT8 *phuffac1th);

// Profiling
extern UINT32   jpeg_get_cyclecnt(void);

// BRC control
extern void     jpeg_get_brcinfo(PJPEG_BRC_INFO p_brcinfo);

extern ER jpeg_trigger(JPEG_CODEC_MODE codec_mode, void *p_param, KDRV_CALLBACK_FUNC *p_cb_func);
extern ER jpeg_add_queue(UINT32 id, JPEG_CODEC_MODE codec_mode, void *p_param, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
extern void jpeg_set_property(KDRV_JPEG_INFO *p_jpeg_info);
extern KDRV_JPEG_TRIG_INFO *kdrv_jpeg_get_triginfo_by_coreid(void);

// set rotate //
extern ER jpeg_set_rotate_cfg(UINT8 ucId, UINT32 uiRotate);
extern ER jpeg_set_rotate_reg(UINT8 ucId);
extern ER jpeg_set_rotate(JPG_HW_ROTATE_MODE RotateMode);
extern ER jpeg_set_newimgdma(BOOL en);

extern UINT32 jpeg_get_DCQvalue(void);

// dma abort //
extern ER jpeg_set_dma_abort(void);
extern UINT32 jpeg_get_dma_idle(void);
// pad reg //
extern ER jpeg_set_pad_reg(UINT32 pad_right, UINT32 pad_bottom);
extern ER jpeg_set_pad_info(UINT32 width, UINT32 height, UINT32 rotation);

extern void jpeg_flush_bsbuf(UINT32 addr, UINT32 size);
extern void jpeg_frame_window_count(UINT32 id, UINT32 size);
// ycc api //
extern ER jpeg_set_init_sde(void);
extern ER jpeg_set_sde_enable(BOOL enable);
// osg kdriver api //
extern ER jpeg_set_osg_win_cfg(UINT8 ucId, UINT8 ucWinIdx, JPEG_OSG_WIN_CFG *pOsgWin);
extern ER jpeg_set_osg_reg(UINT8 ucId, UINT32 uiImgW, UINT32 uiImgH);
extern ER jpeg_clean_osg(UINT8 ucId);
extern BOOL jpeg_check_osg_enable(UINT8 ucId);

extern ER jpeg_set_osg_rgb_cfg(JPEG_OSG_RGB_CFG *pOsgRgb);
extern ER jpeg_set_osg_rgb_reg(void);
extern ER jpeg_set_osg_pal_cfg(UINT8 ucIdx, JPEG_OSG_PAL_CFG *pOsgPal);
extern ER jpeg_set_osg_pal_reg(void);
//@}

extern ER jpeg_set_uv_swap(BOOL uv_swap);
extern void jpeg_dump_reg(void);
extern void jpeg_dump_osg_reg(void);

#endif
