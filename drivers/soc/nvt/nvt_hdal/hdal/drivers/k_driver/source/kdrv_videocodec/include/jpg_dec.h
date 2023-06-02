/**
    Decode JPEG file.

    @file       JpgDec.h
    @ingroup    mIUserJPEG
    @brief      Jpeg decoder

    Copyright   Novatek Microelectronics Corp. 2004.  All rights reserved.

*/

#ifndef _JPGDEC_H
#define _JPGDEC_H

#if defined __UITRON || defined __ECOS
#include "SysKer.h"
#include "JpgHeader.h"
#include "Type.h"
#elif defined __KERNEL__
#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "jpg_header.h"
#include "dal_vdodec.h"
#include <linux/module.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#define debug_msg        printk
#else
#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "jpg_header.h"
#include "dal_vdodec.h"

#endif

/**
    @addtogroup mIUserJPEG
*/
//@{

/**
     @name Needed data size for parsing JPEG header(including EXIF info)
*/
//@{
#define FST_READ_THUMB_BUF_SIZE         0x10000
//@}


/**
     @name Alignment for decoded image address
*/
//@{
#define JPGDEC_IMG_ADDR_ALIGN(a)           ALIGN_CEIL_4(a)
//@}


/**
    Jpeg decoder descriptor
*/
typedef struct _JPG_DEC_INFO {
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
} JPG_DEC_INFO, *PJPG_DEC_INFO;

/**
    Transform size to fit JPEG SPEC or limitation

    @param[in,out] p_width [in]Original image width, [out]output image width acording to the prarmeter fileformat and ScalarLevel.
    @param[in,out] p_height [in]Original image height, [out]output image height acording to the prarmeter fileformat and ScalarLevel.
    @param[in,out] p_loftY [in]Original image lineoffset Y, [out]output image lineoffse Y acording to the prarmeter fileformat and ScalarLevel.
    @param[in,out] p_loftuv [in]Original image lineoffset UV, [out]output image lineoffset UV acording to the prarmeter fileformat and ScalarLevel.
    @param[in] fileformat File format: #_JPG_YUV_FORMAT.
*/
extern void jpg_dec_trans_size(UINT16 *p_width, UINT16 *p_height, UINT16 *p_loftY, UINT16 *p_loftuv, JPG_YUV_FORMAT  fileformat);

/**
    Decode one JPEG bitstream to image raw data.

    @note It's a synchronous API.
    @param[in] p_dec_info Decoding info: #_JPG_DEC_INFO.
    @return
        - @b ER_OK:  Succeed.
        - @b ER_SYS:  Failed.
*/
extern ER jpg_dec_one(PJPG_DEC_INFO p_dec_info);

/**
    Open JPEG engine in asynchronous decoding mode.

    @note It's an asynchronous API.
    @return
        - @b ER_OK:  Succeed.
        - @b ER_SYS:  Failed.
*/
extern ER jpg_async_dec_open(void);

/**
    Start to decode in asynchronous decoding mode.

    @note It's an asynchronous API.
    @param[in] p_dec_info decoding info: #_JPG_DEC_INFO.
    @return
        - @b ER_OK:  Succeed.
        - @b ER_SYS:  Failed.
*/
extern ER jpg_async_dec_start(PJPG_DEC_INFO p_dec_info);

/**
    Wait for JPEG to finish in asynchronous decoding mode.

    @note It's an asynchronous API.
    @return
        - @b ER_OK:  Succeed.
        - @b ER_SYS:  Failed.
*/
extern ER jpg_async_dec_waitdone(void);

/**
    Close JPEG engine in asynchronous decoding mode.

    @note It's an asynchronous API.
    @return
        - @b ER_OK:  Succeed.
        - @b ER_SYS:  Failed.
*/
extern ER jpg_async_dec_close(void);


ER dal_jpegdec_init(DAL_VDODEC_ID id, DAL_VDODEC_INIT *pinit);
ER dal_jpegdec_getinfo(DAL_VDODEC_ID id, DAL_VDODEC_GET_ITEM item, UINT32 *pvalue);
ER dal_jpegdec_setinfo(DAL_VDODEC_ID id, DAL_VDODEC_SET_ITEM item, UINT32 value);
ER dal_jpegdec_decodeone(DAL_VDODEC_ID id, DAL_VDODEC_PARAM *pparam);
ER dal_jpegdec_close(DAL_VDODEC_ID id);

//@}
#endif // _JPGDEC_H
