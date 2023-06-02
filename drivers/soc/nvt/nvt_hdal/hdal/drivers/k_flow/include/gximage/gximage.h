/**
    Image operation module.

    This module include the image operation like fill image,copy image, scale image, rotate image ...

    @file       gximage.h
    @ingroup    imgtrans

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _GX_IMAGE_H_
#define _GX_IMAGE_H_

#include "kwrap/type.h"
#include "kflow_common/type_vdo.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
    @addtogroup imgtrans
*/

/**
     @name Lineoffset alignment pattern
*/
//@{
#define GXIMG_LINEOFFSET_PATTERN     0x80000000                          ///<   lineoffset base pattern
#define GXIMG_LINEOFFSET_ALIGN(n)    (GXIMG_LINEOFFSET_PATTERN | (n))  ///<   n bytes alignment lineoffset
//@}

/**
     @name Image buffer pixel format for 650
*/
//@{
#define GXIMG_PIXEL_FMT_YUV422     GXIMG_PIXEL_FMT_YUV422_PACKED   ///<   for 650 paltform , YUV 422 means YUV 422 packed
#define GXIMG_PIXEL_FMT_YUV420     GXIMG_PIXEL_FMT_YUV420_PACKED   ///<   for 650 paltform , YUV 420 means YUV 420 packed
//@}


/**
     @name Image buffer plane number maximum value
*/
//@{
#define GXIMG_MAX_PLANE_NUM                 3            ///<  3 if for YUV planner
//@}

/**
     @name Image region default value
*/
//@{
#define GXIMG_REGION_MATCH_IMG              NULL         ///<  use in region w,h,x,y match pImgBuf
//@}

/**
     macros to extra old IMG_BUF fields from VDO_FRAME.
*/
#define GXIMG_GET_SCALE_RATIO(vf)   (&(((PVDO_COORD)(vf->reserved))->scale))
#define GXIMG_GET_VIRT_COORD(vf)    (&(((PVDO_COORD)(vf->reserved))->coord))
#define GXIMG_GET_REAL_COORD_XY(vf) (&(((PVDO_COORD)(vf->reserved))->shift))
#define GXIMG_GET_REAL_COORD_WH(vf) (&(vf->size))
#define GXIMG_GET_META_DATA(vf)     ((*MEM_RANGE)(&(vf->meta)))

/**
     Structure of image cover description.
*/
typedef struct {
	IPOINT     top_left;            ///< point 0 (left/top)
	IPOINT     top_right;           ///< point 1 (right/top)
	IPOINT     bottom_right;        ///< point 2 (right/bottom)
	IPOINT     bottom_left;         ///< point 3 (left/bottom)
	UINT32     yuva;                  ///< Alpha[31..24], V[23..16], U[15..8], Y[7..0], Please refer to COLOR_MAKE_YUVA macro
	///< If this value is not zero, mosaic setting will be omitted
	ISIZE      mosaic_blk_size;            ///< mosaic block size (only support 8*8, 16*16, 32*32, 64*64)
	PVDO_FRAME p_mosaic_img;            ///< mosaic image buffer
} GXIMG_COVER_DESC, *PGXIMG_COVER_DESC;

/**
     Video Display layer ID.
*/
typedef enum _GXIMG_DISP_ID {
	GXIMG_DISP_VIDEO1 = 1,      ///< video 1
	GXIMG_DISP_VIDEO2 = 2,      ///< video 2
	ENUM_DUMMY4WORD(GXIMG_DISP_ID)
} GXIMG_DISP_ID;


/**
     Rotation Engine ID.
*/
typedef enum _GXIMG_RT_ENG {
	GXIMG_ROTATE_ENG1 = 0,      ///< Rotation Engine 1
	GXIMG_ROTATE_ENG2 ,         ///< Rotation Engine 2
	GXIMG_ROTATE_RESERVE,       ///< Rotation Engine reserved
	ENUM_DUMMY4WORD(GXIMG_RT_ENG)
} GXIMG_RT_ENG;

/**
     Fill Engine ID.
*/
typedef enum _GXIMG_FILL_ENG {
	GXIMG_FILL_ENG1 = 0,      ///< Fill Engine 1
	GXIMG_FILL_ENG2,          ///< Fill Engine 2
	ENUM_DUMMY4WORD(GXIMG_FILL_ENG)
} GXIMG_FILL_ENG;


/**
     Copy Engine ID.
*/
typedef enum _GXIMG_CP_ENG {
	GXIMG_CP_ENG1 = 0,					///< Copy Engine 1
	GXIMG_CP_ENG2,						///< Copy Engine 2
	GXIMG_CP_ENG_MAX,					///< Copy Engine MAX
	GXIMG_CP_ENG_DFT = GXIMG_CP_ENG1,	///< Copy Engine Default
	ENUM_DUMMY4WORD(GXIMG_CP_ENG)
} GXIMG_CP_ENG;

/**
     Scale Engine ID.
*/
typedef enum _GXIMG_SC_ENG {
	GXIMG_SC_ENG1 = 0,      ///< Scale Engine 1
	GXIMG_SC_ENG2,          ///< Scale Engine 2
	ENUM_DUMMY4WORD(GXIMG_SC_ENG)
} GXIMG_SC_ENG;

/**
     Encode information.

*/
typedef struct _GXIMG_ENCODE_INFO {
	PVDO_FRAME p_src_img;          ///< the source raw image buffer
	UINT32     out_buf_addr;       ///< Out bitstream buffer starting address
	UINT32     out_buf_size;       ///< Out bitstream buffer size
	void      *param[4];         ///< param[0] is Q-table addr Y, param[1] is Q-table addr UV , param[2] & param[3] are reserved.
} GXIMG_ENCODE_INFO, *PGXIMG_ENCODE_INFO;

/**
    Parameter ID.

*/
typedef enum _GXIMG_PARM_ID {
	GXIMG_PARM_SCALE_METHOD = 0,        ///< set scale method for scale function, the setting value please reference GXIMG_SCALE_METHOD.
	GXIMG_PARM_FILTER_P2I = 1,          ///< set IME scale filter for enable progressive to interlaced operation, the setting value is 1 or 0.
	ENUM_DUMMY4WORD(GXIMG_PARM_ID)
} GXIMG_PARM_ID;


/**
    Scale method.

    Used when set parameter GXIMG_PARM_SCALE_METHOD. The default scale method for scale function is GXIMG_SCALE_AUTO.

*/
typedef enum _GXIMG_SCALE_METHOD {
	GXIMG_SCALE_BICUBIC = 0,        ///< bicubic interpolation
	GXIMG_SCALE_BILINEAR,           ///< bilinear interpolation
	GXIMG_SCALE_NEAREST,            ///< nearest interpolation
	GXIMG_SCALE_INTEGRATION,        ///< integration interpolation
	GXIMG_SCALE_AUTO,               ///< auto choose scale method above, this is the default value.
	GXIMG_SCALE_MAX_ID,             ///< total scale method numbers
	ENUM_DUMMY4WORD(GXIMG_SCALE_METHOD)
} GXIMG_SCALE_METHOD;


//------------------------------------------------------------------------------
// GxImage Basic
//------------------------------------------------------------------------------
/**
     Calculate required buffer size.

     User can firstly call this API to get required buffer size before calling gximg_init_buf().

     @param[in] width: The image width.
     @param[in] height: The image height.
     @param[in] pxlfmt: The image pixel format.
     @param[in] lineoff: The image pixel lineoffset. GXIMG_LINEOFFSET_ALIGN(n) means the lineoffset is the input image width align n.\n
                         0 means the lineoffset is input image width align 4.\n
     @return required buffer size.

*/
extern UINT32 gximg_calc_require_size(UINT32 width, UINT32 height, VDO_PXLFMT pxlfmt, UINT32 lineoff);

extern UINT32 gximg_calc_require_size_h_align(UINT32 width, UINT32 height, UINT32 halign, VDO_PXLFMT pxlfmt, UINT32 lineoff);


/**
     Init Image Buffer.

     When user use this API to init buffer, it is assumed that for the Y UVpacked image, the width, lineoffset of Y & UV are the same.\n
     And the Y UV data are put in sequence and continuous in the buffer address.

     @param[out] p_img_buf: The image buffer.
     @param[in] width: The image width.
     @param[in] height: The image Height.
     @param[in] pxlfmt: The image pixel format.
     @param[in] lineoff: The image pixel lineoffset. GXIMG_LINEOFFSET_ALIGN(n) means the lineoffset is the input image width align n.\n
                         0 means the lineoffset is input image width align 4.\n
     @param[in] addr: The image raw buffer address.
     @param[in] available_size: The image raw buffer available size.
     @return
         - @b E_OK:    Init Image buffer ok.
         - @b E_SYS:   Init Image buffer fail, input buffer size not enough.
         - @b E_PAR:   Init Image buffer fail, input parameter error.

     Example:
     @code
     {
         UINT32      uiPool,uiPoolSize;
         ISIZE       DeviceSize;
         VDO_FRAME   DispImg;

         // Get display size
         DeviceSize = GxVideo_GetDeviceSize(DOUT1);
         // Init Display image buffer as pixel format 422
         gximg_init_buf(&DispImg,DeviceSize.w,DeviceSize.h,VDO_PXLFMT_YUV422, GXIMG_LINEOFFSET_ALIGN(16),uiPool,uiPoolSize);
     }
     @endcode
*/
extern ER gximg_init_buf(PVDO_FRAME p_img_buf, UINT32 width, UINT32 height, VDO_PXLFMT pxlfmt, UINT32 lineoff, UINT32 addr, UINT32 available_size);

extern ER gximg_init_buf_h_align(PVDO_FRAME p_img_buf, UINT32 width, UINT32 height, UINT32 halign, VDO_PXLFMT pxlfmt, UINT32 lineoff, UINT32 addr, UINT32 available_size);

/**
     Init Image Buffer extension API.

     User can use this API when the Y UV data address & lineoffset is already known in other image buffer structure.
     The Y UV data address may be not in sequence or continuous.

     @param[out] p_img_buf: The image buffer.
     @param[in] width: The image width.
     @param[in] height: The image height.
     @param[in] pxlfmt: The image pixel format.
     @param[in] lineoff: The image pixel lineoffset of each plane.
     @param[in] pxladdr: The image raw buffer address of each plane.
     @return
         - @b E_OK:    Init Image buffer ok.
         - @b E_PAR:   Init Image buffer fail, input parameter error.

     Example:
     @code
     {
         uiLineOffset[0] = StampBuffer.uiYLineOffset;
         uiLineOffset[1] = StampBuffer.uiUVLineOffset;
         uiPxlAddr[0]    = StampBuffer.uiYAddr;
         uiPxlAddr[1]    = StampBuffer.uiUAddr;
         gximg_init_buf_ex(&SrcImg, uiWidth, uiStampDataHeight,VDO_PXLFMT_YUV420, uiLineOffset, uiPxlAddr);
     }
     @endcode
*/
extern ER gximg_init_buf_ex(PVDO_FRAME p_img_buf, UINT32 width, UINT32 height, VDO_PXLFMT pxlfmt, UINT32 lineoff[GXIMG_MAX_PLANE_NUM], UINT32 pxladdr[GXIMG_MAX_PLANE_NUM]);

/**
     Get image buffer start & End address.

     User can use this API to get start & end address when the image buffer is initialized by gximg_init_buf(),\n.
     because the Y UV data is in sequence & continuous.
     User can't use this API to get start & end address when the image buffer is initialized by gximg_init_buf_ex(),\n
     because the Y UV data may be not in sequence or continuous.

     @param[in] p_img_buf: The image buffer.
     @param[out] p_start_addr: The image buffer start address.
     @param[out] p_end_addr: The image buffer end address.
     @return
         - @b TRUE:    Get buffer address ok.
         - @b FALSE:   Get buffer address fail, input parameter error.

*/
extern ER gximg_get_buf_addr(PVDO_FRAME p_img_buf, UINT32 *p_start_addr, UINT32 *p_end_addr);

//------------------------------------------------------------------------------
// GxImage Process
//------------------------------------------------------------------------------


/**
     Fill image buffer with specific YCbCr/rgb565/argb1555/argb4444 color.

     @param[in] p_dst_img: The image buffer.
     @param[in] p_dst_region: Fill region of image buffer. GXIMG_REGION_MATCH_IMG means the region matches image buffer.
     @param[in] color : for yuv image, it's ycbcr: The fill color value, format is 0x00CrCbYY.
	                    for (a)rgb image, it's (a)rgb
     @return
         - @b E_OK:    Fill color success.
         - @b E_PAR:   Input parameter error.
*/
extern ER gximg_fill_data(PVDO_FRAME p_dst_img, IRECT *p_dst_region, UINT32 color);

extern ER gximg_fill_data_no_flush(PVDO_FRAME p_dst_img, IRECT *p_dst_region, UINT32 color);

/**
     Fill image buffer with specific YCbCr/rgb565/argb1555/argb4444 color.

     @param[in] p_dst_img: The image buffer.
     @param[in] p_dst_region: Fill region of image buffer. GXIMG_REGION_MATCH_IMG means the region matches image buffer.
     @param[in] color : for yuv image, it's ycbcr: The fill color value, format is 0x00CrCbYY.
	                    for (a)rgb image, it's (a)rgb
     @param[in] engine: The fill data engine.
     @return
         - @b E_OK:    Fill color success.
         - @b E_PAR:   Input parameter error.
*/
extern ER gximg_fill_data_ex(PVDO_FRAME p_dst_img, IRECT *p_dst_region, UINT32 color, GXIMG_FILL_ENG engine);

/**
     Copy image data.

     Copy image data of specific region from source buffer to specific location of destionation buff.

     @note Naturely, the source region should not over the range of source image. So if the input soucre region is over the source image.\n
     This fuction will auto calculate clipped rectangle for copying. The region has limitation of minimum width value is 2, height value is 2.

     @param[in] p_src_img: The source image buffer.
     @param[in] p_src_region: The source buffer region for copying. GXIMG_REGION_MATCH_IMG means the region matches source image buffer.
     @param[in] p_dst_img: The destionation image buffer.
     @param[in] p_dst_location: The destionation image buffer location to copy to.
     @return
         - @b E_OK:    Copy data success.
         - @b E_PAR:   Input parameter error.
*/
extern ER gximg_copy_data(PVDO_FRAME p_src_img, IRECT *p_src_region, PVDO_FRAME p_dst_img, IPOINT *p_dst_location, GXIMG_CP_ENG engine);

extern ER gximg_copy_data_no_flush(PVDO_FRAME p_src_img, IRECT *p_src_region, PVDO_FRAME p_dst_img, IPOINT *p_dst_location, GXIMG_CP_ENG engine);

/**
     Copy image data.

     Copy image data of specific region from source buffer to specific location of destionation buff.

     @note Naturely, the source region should not over the range of source image. So if the input soucre region is over the source image.\n
     This fuction will auto calculate clipped rectangle for copying. The region has limitation of minimum width value is 2, height value is 2.

     @param[in] p_src_img: The source image buffer.
     @param[in] p_src_region: The source buffer region for copying. GXIMG_REGION_MATCH_IMG means the region matches source image buffer.
     @param[in] p_dst_img: The destionation image buffer.
     @param[in] p_dst_location: The destionation image buffer location to copy to.
     @param[in] engine: The copy data engine.
     @return
         - @b E_OK:    Copy data success.
         - @b E_PAR:   Input parameter error.
*/
extern ER gximg_copy_data_ex(PVDO_FRAME p_src_img, IRECT *p_src_region, PVDO_FRAME p_dst_img, IPOINT *p_dst_location, GXIMG_CP_ENG engine);

/**
     Copy image data with colorkey effect.

     This API will do the color key (=) copy effect. \n
     A: is source image. B: is key image. It means if (B == colorkey) ? A:B -> destination buffer.

     @param[in] p_src_img: The source image buffer.
     @param[in] p_copy_region: The buffer region for copying. GXIMG_REGION_MATCH_IMG means the region matches source image buffer.
     @param[in] p_key_img: The color key image buffer.
     @param[in] p_key_location: The key image buffer location to copy to.
     @param[in] color_key: The color key (0x00VVUUYY).
     @param[in] is_copy_to_key_img: If the destination buffer is p_key_img, else the destination buffer will be p_src_img.
     @param[in] engine: The copy data engine.
     @return
         - @b E_OK:    Copy data success.
         - @b E_PAR:   Input parameter error.
*/
extern ER gximg_copy_color_key_data(PVDO_FRAME p_src_img, IRECT *p_copy_region, PVDO_FRAME p_key_img, IPOINT *p_key_location, UINT32 color_key, BOOL is_copy_to_key_img, GXIMG_CP_ENG engine);

/**
     Copy image data with alpha blending effect.

     This API will do the alpha blending effect. \n
     A: is source image. B: is destion image. The copy blending weight is (alpha/256)*A + ((256-alpha)/256)*B.

     @param[in] p_src_img: The source image buffer.
     @param[in] p_src_region: The source buffer region for copying. GXIMG_REGION_MATCH_IMG means the region matches source image buffer.
     @param[in] p_dst_img: The destionation image buffer.
     @param[in] p_dst_location: The destionation image buffer location to copy to.
     @param[in] alpha: The alpha blending value, range is from 0 ~ 256, all the image use the same constant alpha.
     @param[in] engine: The copy data engine.
     @return
         - @b E_OK:    Copy data success.
         - @b E_PAR:   Input parameter error.
*/
extern ER gximg_copy_blend_data(PVDO_FRAME p_src_img, IRECT *p_src_region, PVDO_FRAME p_dst_img, IPOINT *p_dst_location, UINT32 alpha, GXIMG_CP_ENG engine);

extern ER gximg_copy_blend_data_no_flush(PVDO_FRAME p_src_img, IRECT *p_src_region, PVDO_FRAME p_dst_img, IPOINT *p_dst_location, UINT32 alpha, GXIMG_CP_ENG engine);

/**
     Copy image data with alpha blending effect.

     This API will do the alpha blending effect. \n
     A: is source image. B: is alpha plane. C: is destion image. The copy blending weight is (B/256)*A + ((256-B)/256)*C.

     @param[in] p_src_img: The source image buffer.
     @param[in] p_src_region: The source buffer region for copying. GXIMG_REGION_MATCH_IMG means the region matches source image buffer.
     @param[in] p_dst_img: The destionation image buffer.
     @param[in] p_dst_location: The destionation image buffer location to copy to.
     @param[in] p_alpha_plane: The alpha plane. The width, height and lineoffset is the same with source image.
     @return
         - @b E_OK:    Copy data success.
         - @b E_PAR:   Input parameter error.
*/
extern ER gximg_copy_blend_data_ex(PVDO_FRAME p_src_img, IRECT *p_src_region, PVDO_FRAME p_dst_img, IPOINT *p_dst_location, UINT8 *p_alpha_plane);

extern ER gximg_copy_blend_data_ex_no_flush(PVDO_FRAME p_src_img, IRECT *p_src_region, PVDO_FRAME p_dst_img, IPOINT *p_dst_location, UINT8 *p_alpha_plane);

/**
     Set parameter of GxIamge.

     @param[in] parm_id: The parameter ID.
     @param[in] value: The parameter value.
     @return
         - @b E_OK:    Setting success.
         - @b E_PAR:   Input parameter error.
*/
extern ER gximg_set_parm(GXIMG_PARM_ID parm_id, UINT32 value);

/**
     Get parameter of GxIamge.

     @param[in] parm_id: The parameter ID.
     @return
         - @b The parameter value
*/
extern UINT32 gximg_get_parm(GXIMG_PARM_ID parm_id);

/**
     Scaling up/down image data.

     This API will scale up/down image data, the max scale up/down factor is 16.\n
     This API have no start address word alignment limitation or width/height word alignment limitation.\n
     The scaling method supports all that defined in GXIMG_SCALE_METHOD.

     @note If the scaling up/down factor over the HW limitation, it will auto change to FW scaling.

     @param[in] p_src_img: The source image buffer.
     @param[in] p_src_region: The source buffer region for scaling. GXIMG_REGION_MATCH_IMG means the region matches source image buffer.
     @param[in] p_dst_img: The destionation image buffer.
     @param[in] p_dst_region: The destionation image buffer region to scaling to. GXIMG_REGION_MATCH_IMG means the region matches destionation image buffer.
     @return
         - @b E_OK:    Scaling data success.
         - @b E_PAR:   Input parameter error.
*/
extern ER gximg_scale_data(PVDO_FRAME p_src_img, IRECT *p_src_region, PVDO_FRAME p_dst_img, IRECT *p_dst_region, GXIMG_SCALE_METHOD method);

extern ER gximg_scale_data_no_flush(PVDO_FRAME p_src_img, IRECT *p_src_region, PVDO_FRAME p_dst_img, IRECT *p_dst_region, GXIMG_SCALE_METHOD method);

/**
     Scaling up/down image data.

     This API will scale up/down image data, the max scale up/down factor is 16.\n
     This API have no start address word alignment limitation or width/height word alignment limitation.\n
     The scaling method supports all that defined in GXIMG_SCALE_METHOD.

     @note If the scaling up/down factor over the HW limitation, it will auto change to FW scaling.

     @param[in] p_src_img: The source image buffer.
     @param[in] p_src_region: The source buffer region for scaling. GXIMG_REGION_MATCH_IMG means the region matches source image buffer.
     @param[in] p_dst_img: The destionation image buffer.
     @param[in] p_dst_region: The destionation image buffer region to scaling to. GXIMG_REGION_MATCH_IMG means the region matches destionation image buffer.
     @param[in] engine: The scale data engine.
     @return
         - @b E_OK:    Scaling data success.
         - @b E_PAR:   Input parameter error.
*/
extern ER gximg_scale_data_ex(PVDO_FRAME p_src_img, IRECT *p_src_region, PVDO_FRAME p_dst_img, IRECT *p_dst_region, GXIMG_SC_ENG engine, GXIMG_SCALE_METHOD method);

/**
     Scaling up/down image data with high quality.

     This API will scale up/down image data, the max scale up factor is 32, the max scale down factor is 16.\n
     This API have start address word alignment limitation & width/height word alignment limitation.\n
     The scaling method supports all that defined in GXIMG_SCALE_METHOD.

     @note If the scaling up/down factor over the HW limitation, or the address alignment not meet the HW limitation,\n
     it wll auto change to FW scaling.

     @param[in] p_src_img: The source image buffer.
     @param[in] p_src_region: The source buffer region for scaling.GXIMG_REGION_MATCH_IMG means the region matches source image buffer.
     @param[in] p_dst_img: The destionation image buffer.
     @param[in] p_dst_region: The destionation image buffer region to scaling to. GXIMG_REGION_MATCH_IMG means the region matches destionation image buffer.
     @return
         - @b E_OK:    Scale data success.
         - @b E_PAR:   Input parameter error.
*/
extern ER gximg_scale_data_fine(PVDO_FRAME p_src_img, IRECT *p_src_region, PVDO_FRAME p_dst_img, IRECT *p_dst_region);


/**
     Scaling down Y image data only.

     This API will scale down Y image data only, The scaling method is bilinear.

     @note If the scaling up/down factor over the HW limitation, or the address alignment not meet the HW limitation,\n
     it wll auto change to FW scaling.

     @param[in] p_src_img: The source image buffer.
     @param[in] p_src_region: The source buffer region for scaling.GXIMG_REGION_MATCH_IMG means the region matches source image buffer.
     @param[in] p_dst_img: The destionation image buffer.
     @param[in] p_dst_region: The destionation image buffer region to scaling to. GXIMG_REGION_MATCH_IMG means the region matches destionation image buffer.
     @return
         - @b E_OK:    Scale data success.
         - @b E_PAR:   Input parameter error.
*/
extern ER gximg_scale_data_down_y_only(PVDO_FRAME p_src_img, IRECT *p_src_region, PVDO_FRAME p_dst_img, IRECT *p_dst_region);


/**
     Rotate image data.

     This API will rotate a image data and put the rotated image to dstBuff, the rotated image info will be put in p_out_img_buf.\n
     The needed destination buffer size is align16(w)*align8(h)*3/2;

     @note If the source image is 422 format, the roated image will be translated to 420 format, because the rotate operation need 420 foramt.

     @param[in] p_img_buf: The source image buffer.
     @param[in] dst_buff: The destination buffer for putting rotated image.
     @param[in] dst_buf_size: The destionation buffer size.
     @param[in] rotate_dir: The rotation direction.
     @param[out] p_out_img_buf: The rotated image buffer.
     @return
         - @b E_OK:    Rotate data success.
         - @b E_PAR:   Input parameter error or destination buffer too small.
*/
extern ER gximg_rotate_data(PVDO_FRAME p_img_buf, UINT32 dst_buff, UINT32 dst_buf_size, UINT32 rotate_dir, PVDO_FRAME p_out_img_buf);

extern ER gximg_rotate_data_no_flush(PVDO_FRAME p_img_buf, UINT32 dst_buff, UINT32 dst_buf_size, UINT32 rotate_dir, PVDO_FRAME p_out_img_buf);

/**
     Rotate image data with specific rotation engine.

     This API will rotate a image data and put the rotated image to dstBuff, the rotated image info will be put in p_out_img_buf.\n
     The needed destination buffer size is align16(w)*align8(h)*3/2;

     @note If the source image is 422 format, the roated image will be translated to 420 format, because the rotate operation need 420 foramt.

     @param[in] p_img_buf: The source image buffer.
     @param[in] dst_buff: The destination buffer for putting rotated image.
     @param[in] dst_buf_size: The destionation buffer size.
     @param[in] rotate_dir: The rotation direction.
     @param[out] p_out_img_buf: The rotated image buffer.
     @param[in] engine: The rotation engine.
     @return
         - @b E_OK:    Rotate data success.
         - @b E_PAR:   Input parameter error or destination buffer too small.
*/
extern ER gximg_rotate_data_ex(PVDO_FRAME p_img_buf, UINT32 dst_buff, UINT32 dst_buf_size, UINT32 rotate_dir, PVDO_FRAME p_out_img_buf, GXIMG_RT_ENG engine);


/**
     Rotate image data with specific source region to specific postion of destination image.

     @note The source image & destination image should be 420 format, because the rotate operation need 420 foramt.

     @param[in] p_src_img: The source image buffer.
     @param[in] p_src_region: The source buffer region for scaling.GXIMG_REGION_MATCH_IMG means the region matches source image buffer.
     @param[in] p_dst_img: The destionation image buffer.
     @param[in] p_dst_location: The destionation image buffer location to rotate to.
     @param[in] rotate_dir: The rotation direction.
     @param[in] engine: The rotation engine.
     @return
         - @b E_OK:    Rotate data success.
         - @b E_PAR:   Input parameter error or destination buffer too small.
*/
extern ER gximg_rotate_paste_data(PVDO_FRAME p_src_img, IRECT *p_src_region, PVDO_FRAME p_dst_img, IPOINT *p_dst_location, UINT32 rotate_dir, GXIMG_RT_ENG engine);

extern ER gximg_rotate_paste_data_no_flush(PVDO_FRAME p_src_img, IRECT *p_src_region, PVDO_FRAME p_dst_img, IPOINT *p_dst_location, UINT32 rotate_dir, GXIMG_RT_ENG engine);

/**
     Self rotate image data.

     This API will rotate a image data and put the rotated image to original image buffer address.\n
     So the rotated image buffer size should not over the original image buffer size.

     @note If the source image is 422 format, the roated image will be translated to 420 format, because the rotate operation need 420 foramt.

     @param[in,out] p_img_buf: The image buffer.
     @param[in] tmp_buff: The temp buffer for roate operaion.
     @param[in] tmp_buf_size: The temp buffer size (need about dstLineoff * dstHeight).
     @param[in] rotate_dir: The rotation direction.
     @return
         - @b E_OK:    Rotate data success.
         - @b E_PAR:   Input parameter error or temp buffer too small.
*/
extern ER gximg_self_rotate(PVDO_FRAME p_img_buf, UINT32 tmp_buff, UINT32 tmp_buf_size, UINT32 rotate_dir);

/**
     Color Transform between YUV and RGB.

     This API support the color transform of 4 cases.
     1. YUV 420 packed/planar -> RGB888 planar
     2. RGB888 planar -> YUV packed
     3. ARGB888 packed -> YUV packed (need tmpBuff W*ALIGN_8(H)*7)
     4. YUV 420 planar -> YUV 420 packed

     @param[in] p_src_img: The source image buffer.
     @param[in] p_dst_img: The destionation image buffer.
     @param[in] tmp_buff: The temp buffer for color transform.
     @param[in] tmp_buf_size: The temp buffer size.


*/
extern ER gximg_color_transform(PVDO_FRAME p_src_img, PVDO_FRAME p_dst_img, UINT32 tmp_buff, UINT32 tmp_buf_size);

/**
     Transform a Gray(8bits-Y) image to YUV packed.

     @param[in] p_src_img: The source image buffer.
     @param[in] p_dst_img: The destionation image buffer.
     @param[in] lookup_table: The color transform lookup table.

*/
extern ER gximg_gray_to_yuv(PVDO_FRAME p_src_img, PVDO_FRAME p_dst_img, UINT32 lookup_table[GXIMG_MAX_PLANE_NUM]);

/**
     Transform ARGB to YUV with alpha value by graphic engine.

     Supported input formats: \n
     GXIMG_PXLFMT_ARGB1555 (Alpha0: uiAlpha[3..0], Alpha1: uiAlpha[7..4])
     GXIMG_PXLFMT_ARGB4444
     GXIMG_PXLFMT_RGB565 (Alpha: uiAlpha)

     @param[in] p_src_img: The source image buffer.
     @param[in] p_src_region: The source buffer region for copying. GXIMG_REGION_MATCH_IMG means the region matches source image buffer.
     @param[in] p_dst_img: The destionation image buffer.
     @param[in] p_dst_location: The destionation image buffer location.
     @param[in] alpha: The alpha blending value.
     @return
         - @b E_OK:    Operation success.
         - @b E_PAR:   Input parameter error.
         - @b E_SYS:   HW engine or system error
*/
extern ER gximg_argb_to_yuv_blend(PVDO_FRAME p_src_img, IRECT *p_src_region, PVDO_FRAME p_dst_img, IPOINT *p_dst_location, UINT32 alpha, void *palette);

extern ER gximg_argb_to_yuv_blend_no_flush(PVDO_FRAME p_src_img, IRECT *p_src_region, PVDO_FRAME p_dst_img, IPOINT *p_dst_location, UINT32 alpha, void *palette);

/**
     Transform RGB to YUV with colorkey and alpha by graphic engine.

     Supported input formats: \n
     GXIMG_PXLFMT_RGB565

     @param[in] p_src_img: The source image buffer.
     @param[in] p_src_region: The source buffer region for copying. GXIMG_REGION_MATCH_IMG means the region matches source image buffer.
     @param[in] p_dst_img: The destionation image buffer.
     @param[in] p_dst_location: The destionation image buffer location.
     @param[in] alpha: The alpha blending value. (Applied only when the Src pixel is not ColorKey)
     @param[in] color_key: The RGB color key. (0x00RRGGBB)
     @return
         - @b E_OK:    Operation success.
         - @b E_PAR:   Input parameter error.
         - @b E_SYS:   HW engine or system error
*/
extern ER gximg_rgb_to_yuv_color_key(PVDO_FRAME p_src_img, IRECT *p_src_region, PVDO_FRAME p_dst_img, IPOINT *p_dst_location, UINT32 alpha, UINT32 color_key);

/**
     Set a convex quadrilateral cover to the image.

     Supported input formats: \n
     GXIMG_PXLFMT_YUV422_PLANAR
     GXIMG_PXLFMT_YUV420_PLANAR
     GXIMG_PXLFMT_YUV422
     GXIMG_PXLFMT_YUV420
     GXIMG_PXLFMT_Y8

     @note The size of pMosaicImg is limited to SrcW/BlkSize and SrcH/BlkSize

     @param[in] p_cover: The image cover description.
     @param[in] p_dst_img: The destionation image buffer.
     @param[in] p_dst_region: The destionation image region to cover.
     @return
         - @b E_OK:    Operation success.
         - @b E_PAR:   Input parameter error.
         - @b E_SYS:   HW engine or system error
*/
extern ER gximg_quad_cover(GXIMG_COVER_DESC *p_cover, PVDO_FRAME p_dst_img, IRECT *p_dst_region, UINT32 hollow, UINT32 thickness);

extern ER gximg_quad_cover_no_flush(GXIMG_COVER_DESC *p_cover, PVDO_FRAME p_dst_img, IRECT *p_dst_region, UINT32 hollow, UINT32 thickness);

extern ER gximg_quad_cover_with_flush(GXIMG_COVER_DESC *p_cover, PVDO_FRAME p_dst_img, IRECT *p_dst_region, UINT32 hollow, UINT32 thickness, int no_flush);
//------------------------------------------------------------------------------
// GxImage Virtual coordinate
//------------------------------------------------------------------------------
/**
     Set virtual coordinate for one image buffer.

     When user set the virtual coordinate rectangle differ with real coordinate rectangle.\n
     The image buffer will work in the virtual coordinate view.

     @param[in] p_img_buf: The image buffer.
     @param[in] p_virt_coord: The virtual coordinate rectangle.
     @param[in] p_real_coord: The real coordinate rectangle. NULL means pRealCoord = image buff rectangle.
     @return
         - @b E_OK:    Set ok.
         - @b E_PAR:   Input parameter error.
*/
extern ER gximg_set_coord(PVDO_FRAME p_img_buf, IRECT *p_virt_coord, IRECT *p_real_coord);

/**
     Get virtual coordinate of one image buffer.

     @param[in] p_img_buf: The image buffer.
     @return the virtual coordinate rectangle value.

*/
extern IRECT  gximg_get_virtual_coord(PVDO_FRAME p_img_buf);

/**
     Get real coordinate of one image buffer.

     @param[in] p_img_buf: The image buffer.
     @return the real coordinate rectangle value.

*/
extern IRECT  gximg_get_real_coord(PVDO_FRAME p_img_buf);

extern ER     gximg_raw_graphic(UINT32 engine, void *p_request);

#ifdef __cplusplus
} //extern "C"
#endif

#endif //_GX_IMAGE_H_
