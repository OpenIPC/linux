/**
 * @file kdrv_videodec.h
 * @brief type definition of KDRV API.
 * @author ALG2
 * @date in the year 2018
 */

#ifndef __KDRV_VIDEODEC_H__
#define __KDRV_VIDEODEC_H__

#include "kwrap/type.h"
#include "kdrv_type.h"

typedef enum {
	KDRV_VDODEC_ID_1,							///< video decoder ID 1
	KDRV_VDODEC_ID_MAX,              			///< video decoder ID maximum
	ENUM_DUMMY4WORD(KDRV_VDODEC_ID)
} KDRV_VDODEC_ID;

typedef enum {
	VDODEC_TYPE_JPEG = 0,
	VDODEC_TYPE_H264,
	VDODEC_TYPE_H265,
	ENUM_DUMMY4WORD(KDRV_VDODEC_TYPE)
} KDRV_VDODEC_TYPE;

typedef enum {
	KDRV_VDODEC_YUV420 = 0,
	KDRV_VDODEC_YUV422,
	ENUM_DUMMY4WORD(KDRV_VDODEC_YUVFMT)
} KDRV_VDODEC_YUVFMT;

/*********** decode init ***********/
typedef struct {
	UINT32 width;								///<[w] decode image width
	UINT32 height;								///<[w] decode image heght
	UINT32 desc_addr;							///<[w] decode header address (sps + pps)
	UINT32 desc_size;							///<[w] decode header size
	UINT32 buf_addr;							///<[w] decoder internal buffer starting address
	UINT32 buf_size;							///<[w] decoder internal buffer size
	UINT32 display_width;						///<[w] display source width, e.g. 720 x 480, uiWidth = 736, uiDisplayWidth = 720
	UINT32 uiYLineoffset;						///<[w] decode Y line offset
	UINT32 uiUVLineoffset;						///<[w] decode UV line offset
} KDRV_VDODEC_INIT;

/*********** decode one frame ***********/
typedef struct {
	UINT32 bs_addr;								///<[w] bit-stream address
	UINT32 bs_size;								///<[w] bit-stream size
	UINT32 cur_bs_size;							///<[w] current bit-stream size
	UINT32 y_addr;								///<[w] raw y address
	UINT32 c_addr;								///<[w] raw uv address
	UINT32 jpeg_hdr_addr;						///<[w] bit-stream header address (JPEG)
	UINT32 uiWidth;								///<[r] return decoded image width
	UINT32 uiHeight;							///<[r] return decoded image height
	UINT32 yuv_fmt;								///<[r] return decoded image format
	UINT32 errorcode;							///<[r] return error code
	UINT32 interrupt;							///<[r] return interrupt
	KDRV_VDODEC_REFFRMCB vRefFrmCb;
} KDRV_VDODEC_PARAM;

typedef struct {
	UINT32 hdr_bs_addr;		///< [input]  : decode sps/pps heeader biststream addr
	UINT32 hdr_bs_len;		///< [input]  : decode sps/pps heeader biststream size
	UINT32 width;			///< [output] : return width
	UINT32 height;			///< [output] : return height
	UINT32 size;			///< [output] : return memory requirement size
} KDRV_VDODEC_MEM_INFO;

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
} KDRV_VDODEC_VUI_INFO;

typedef struct {
	UINT32 bs_addr;
	UINT32 bs_size;
	UINT32 total_bs_size;
} KDRV_VDODEC_NXT_BS_INFO;

typedef struct {
	UINT32 addr;
	UINT32 size;
} KDRV_VDODEC_LL_MEM;

typedef struct {
	UINT32 size;
} KDRV_VDODEC_LL_MEM_INFO;

typedef struct {
	UINT16 align_w;								///< [r/w] reconstruct yuv align width
	UINT16 align_h;								///< [r/w] reconstruct yuv align height
} KDRV_VDODEC_RECYUV_WH;

typedef enum {
	VDODEC_SET_INIT,							///< set init decode info
	VDODEC_SET_CODEC,						    ///< set channel codec type
	VDODEC_SET_JPEG_FREQ,						///< set JPEG engine frequency
	VDODEC_SET_CLOSE,							///< set video decode channe close
	VDODEC_SET_NXT_BS,							///< set next bitstream buffer
	VDODEC_SET_LL_MEM,							///< set link-list command buffer address
	ENUM_DUMMY4WORD(KDRV_VDODEC_SET_PARAM_ID)
} KDRV_VDODEC_SET_PARAM_ID;

typedef enum {
	VDODEC_GET_JPEG_FREQ, 						///< get JPEG engine frequency
	VDODEC_GET_MEM_SIZE,						///< get video memory size
	VDODEC_GET_VUI_INFO,
	VDODEC_GET_JPEG_RECYUV_WH,					///< get jpeg align width/height
	VDODEC_GET_H26X_INTERRUPT,
	VDODEC_GET_LL_MEM_SIZE,						///< get link-list command buffer size
	VDODEC_GET_RECYUV_WH,						///< get reconstruct yuv align width/height
	ENUM_DUMMY4WORD(KDRV_VDODEC_GET_PARAM_ID)
}KDRV_VDODEC_GET_PARAM_ID;

/*!
 * @fn INT32 kdrv_videodec_open(UINT32 chip, UINT32 engine)
 * @brief open hardware engine
 * @param chip		the chip id of hardware
 * @param engine	the engine id of hardware
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_videodec_open(UINT32 chip, UINT32 engine);

/*!
 * @fn INT32 kdrv_videodec_close(UINT32 chip, UINT32 engine)
 * @brief close hardware engine
 * @param chip		the chip id of hardware
 * @param engine	the engine id of hardware
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_videodec_close(UINT32 chip, UINT32 engine);

/*!
 * @fn INT32 kdrv_videodec_trigger(KDRV_VDODEC_ID handler, KDRV_VDODEC_PARAM *p_dec_param,
								  USR_CALLBACK_FUNC *p_cb_func,
								  VOID *user_data);
 * @brief trigger hardware engine
 * @param handler 				the handler of hardware
 * @param p_enc_info		 	the decode one frame settings
 * @param p_cb_func 			the callback function
 * @param user_data 			the private user data
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_videodec_trigger(UINT32 id, KDRV_VDODEC_PARAM *p_dec_param,
						  KDRV_CALLBACK_FUNC *p_cb_func,
						  VOID *p_user_data);

/*!
 * @fn INT32 kdrv_videodec_get(UINT32 handler, DEC_ID id, VOID *param)
 * @brief set parameters to hardware engine
 * @param handler	the handler of hardware
 * @param id 		the id of parameters
 * @param param 	the parameters
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_videodec_get(UINT32 id, KDRV_VDODEC_GET_PARAM_ID parm_id, VOID *param);

/*!
 * @fn INT32 kdrv_videodec_set(KDRV_VDODEC_ID handler, DEC_ID id, VOID *param)
 * @brief get parameters to hardware engine
 * @param handler	the handler of hardware
 * @param id 		the id of parameters
 * @param param 	the parameters
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_videodec_set(UINT32 id, KDRV_VDODEC_SET_PARAM_ID parm_id, VOID *param);


#endif
