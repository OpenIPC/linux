/**
    Encode JPEG file.

    @file       jpg_enc.h
    @brief      JPEG encoder

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#ifndef _JPGENC_H
#define _JPGENC_H

#if defined (__UITRON)
#include "SysKer.h"
#include "JpgHeader.h"
#include "Type.h"

#elif defined __KERNEL__
#include "kwrap/type.h"
//#include "kwrap/platform.h"
#include "jpg_header.h"
//#include "dal_vdoenc.h"
//#include <linux/module.h>
//#include <linux/string.h>
//#include <linux/kernel.h>
//#include <linux/printk.h>
#define debug_msg        printk

#else
#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "jpg_header.h"
#include "dal_vdoenc.h"
#endif



/**
     @name The size of DQT(define quantization table)
*/
//@{
#define DQT_SIZE    64
//@}

/**
    JPEG header form.
*/
typedef enum _JPG_HEADER_FORM {
	JPG_HEADER_STD,                     ///< standard JPEG header
	JPG_HEADER_MOV,                     ///< MOV JPEG header
	JPG_HEADER_QV5,                     ///< QV5 JPEG header
	JPG_HEADER_QV5_NO_HUFFTABLE,        ///< QV5 JPEG header without huffman table
	ENUM_DUMMY4WORD(JPG_HEADER_FORM)
} JPG_HEADER_FORM;

/**
    JPEG encoding configuration.
*/
typedef enum _JPG_ENC_DATA_SET {
	JPG_BRC_STD_QTABLE_QUALITY,         ///< The quality of BRC table base. Default value is 50.
	JPG_TARGET_RATE,                    ///< The target rate of JPEG
	JPG_VBR_QUALITY,                    ///< The quality of VBR initial
	ENUM_DUMMY4WORD(JPG_ENC_DATA_SET)
} JPG_ENC_DATA_SET;


/**
    JPEG Encode configuration structure.
*/
typedef struct _JPG_ENC_CFG {
	UINT32    image_addr[2];                ///< Raw Image address. Ex:RawImageAddr[0] for Y, RawImageAddr[1] for UV
	UINT32    bs_addr;                      ///< Compressed data ouput location.
	UINT32    *p_bs_buf_size;                  ///< [in]available buffer size for output bitstream.[out]real output bitstream size.
	UINT32    width;                          ///< Raw Image width, This value should be 16-multiplier.
	UINT32    height;                         ///< Raw Image height, This value should be 8 or 16 multiplier, depends on output bs format.
	UINT32    lineoffset[2];                  ///< Raw Image line offset. Ex:lineoffset[0] for Y, lineoffset[1] for UV
	UINT32    interval;
	JPG_YUV_FORMAT  yuv_fmt;                ///< YUV format, refer to JPG_YUV_FORMAT
	BOOL    bs_out_dis;                         ///< If BSOutDis is TRUE, JPEG engine wiil NOT output bitstream data.
	BOOL    brc_en;                            ///< Bit Rate Control enable, TRUE for enable and FALSE for disable.
	JPG_HEADER_FORM  header_form;              ///< Header form, refer to JPG_HEADER_FORM.
	UINT32    rotation;                        ///< Rotation
} JPG_ENC_CFG, *PJPG_ENC_CFG;

/**
    Bit Rate Control configuration structure.
*/
typedef struct _JPG_BRC_CFG {
	UINT32          initial_qf;        ///< initial QF value in bit rate control
	UINT32          target_size;       ///< target compressed size when bit rate control is enabled
	UINT32          lowbound_size;     ///< bit stream lower boundary size(error tolerance)
	UINT32          upbound_size;      ///< bit stream upper boundary size(error tolerance)
	UINT32          max_retry;      ///< maxmum re-try count
	UINT32          width;            ///< image width
	UINT32          height;           ///< image height
} JPG_BRC_CFG, *PJPG_BRC_CFG;

/**
    Encode one image into JPEG bitstream.

    @note It's a synchronous API.
    @param[in] p_jpg_param Encoding configuration: #_JPG_ENC_CFG.
    @param quality This parameter is only valid in Non-BRC mode and the value should between 0 to 100.\n
                     100 is the best quality and 1 is the worst one. 0 means default quality.
    @return
        - @b ER_OK:  Succeed.
        - @b ER_OBJ:  Reach BRC limitation or retry count, the bitstream might be correct in BRC mode.
        - @b ER_SYS:  Failed.
        - @b E_NOMEM: Bitstream Buffer is too small.
*/
extern ER       jpg_enc_one_img(PJPG_ENC_CFG p_jpg_param, UINT32 quality);

/**
    Open JPEG engine in asynchronous encoding mode.

    @note It's an asynchronous API.
    @return
        - @b ER_OK:  Succeed.
        - @b ER_SYS:  Failed.
*/
extern ER       jpg_async_enc_open(void);

/**
    Set JPEG header in asynchronous encoding mode.

    @note It's an asynchronous API.
    @param[in] p_jpg_param Encoding configuration: #_JPG_ENC_CFG.
    @param[in] p_q_tbl_y Specific Q Table Y which could be made by jpg_async_set_qtable() or jpg_async_set_brc_qtable().
    @param[in] p_q_tbl_uv Specific Q Table UV which could be made by jpg_async_set_qtable() or jpg_async_set_brc_qtable().
    @param[in] rotation Specific rotation
    @return JPEG header size.
*/
//extern UINT32   jpg_async_enc_setheader(PJPG_ENC_CFG p_jpg_param, UINT8 *p_q_tbl_y, UINT8 *p_q_tbl_uv);
extern UINT32   jpg_async_enc_setheader(PJPG_ENC_CFG p_jpg_param, UINT8 *p_q_tbl_y, UINT8 *p_q_tbl_uv, UINT32 rotation);

/**
    Start to encode in asynchronous encoding mode.

    @note It's an asynchronous API.
    @param[in] bs_bufaddr The address of output bitstream (NOT including header).
    @param[in] bs_bufsize The available size for output bitstream (NOT including header).
    @param[in] img_y_addr Raw image address Y.
    @param[in] img_uv_addr Raw image address UV.
    @return
        - @b ER_OK:  Succeed.
        - @b ER_SYS:  Failed.
*/
extern ER       jpg_async_enc_start(UINT32 bs_bufaddr, UINT32 bs_bufsize, UINT32 img_y_addr, UINT32 img_uv_addr);

/**
    Wait for JPEG to finish in asynchronous encoding mode.

    @note It's an asynchronous API.
    @param[out] p_jpg_size JPG file size including JFIF header and bitsteam.
    @return
        - @b ER_OK:  Succeed.
        - @b ER_SYS:  Failed.
*/
extern ER       jpg_async_enc_waitdone(UINT32 *p_jpg_size);

/**
    Close JPEG engine in asynchronous encoding mode.

    @note It's an asynchronous API.
    @return
        - @b ER_OK:  Succeed.
        - @b ER_SYS:  Failed.
*/
extern ER       jpg_async_enc_close(void);

/**
    Set Q table in asynchronous BRC encoding mode.

    @note It's an asynchronous API.
    @param[in] qf Quality value in BRC mode. 1 is the best quality and 512 is the worst one.
    @param[out] p_q_tbl_y The corresponding Q table Y.
    @param[out] p_q_tbl_uv The corresponding Q table UV.
    @return
        - @b ER_OK:  Succeed.
        - @b ER_SYS:  Failed.
*/
extern void     jpg_async_set_brc_qtable(UINT32 qf, UINT8 *p_q_tbl_y, UINT8 *p_q_tbl_uv);

/**
    Set Q table in asynchronous Non-BRC encoding mode.

    @note It's an asynchronous API.
    @param[in] quality Quality value in Non-BRC mode. 100 is the best quality and 1 is the worst one.
    @param[out] p_q_tbl_y The corresponding Q table Y.
    @param[out] p_q_tbl_uv The corresponding Q table UV.
    @return
        - @b ER_OK:  Succeed.
        - @b ER_SYS:  Failed.
*/
extern void     jpg_async_set_qtable(UINT32 quality, UINT8 *p_q_tbl_y, UINT8 *p_q_tbl_uv);

/**
    Set bit rate control

    This API is used to setup bit rate control.

    @param[in] p_brc_cfg Bit rate control configuration: #_JPG_BRC_CFG
    @return
        - @b E_OK: Succeed.
        - @b E_PAR: Parameter error.
*/
extern ER jpg_set_bitrate_ctrl(PJPG_BRC_CFG p_brc_cfg);

/**
    Get BRC QF value.

    After encoding finished, use this API to get predicted QF value to encode again.

    @param[in] yuv_fmt JPEG BRC YUV type: #JPG_YUV_FORMAT.
    @param[in,out] p_qf QF value, [in]original QF, [out]predicted QF.
    @param[in] bs_size Bitstream size.

*/
extern void jpg_get_brc_qf(UINT32 id, JPG_YUV_FORMAT yuv_fmt, UINT32 *p_qf, UINT32 bs_size);

/**
    Set encoding data.

    In order to change some default setting, such as the quality of BRC table base.

    @param[in] attribute Encoding configuration: #_JPG_ENC_DATA_SET.
    @param[in] Value Value of the attribute.

*/
extern void jpg_enc_setdata(UINT32 id, JPG_ENC_DATA_SET attribute, UINT32 value);

//ER dal_jpegenc_init(DAL_VDOENC_ID id, DAL_VDOENC_INIT *pinit);
//ER dal_jpegenc_getinfo(DAL_VDOENC_ID id, DAL_VDOENC_GET_ITEM item, UINT32 *pvalue);
//ER dal_jpegenc_setinfo(DAL_VDOENC_ID id, DAL_VDOENC_SET_ITEM item, UINT32 value);
//ER dal_jpegenc_encodeone(DAL_VDOENC_ID id, DAL_VDOENC_PARAM *pparam);
//ER dal_jpegenc_close(DAL_VDOENC_ID id);


#endif // _JPGENC_H
