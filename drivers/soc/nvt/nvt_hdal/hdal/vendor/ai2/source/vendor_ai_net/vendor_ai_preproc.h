#ifndef _NET_PREPROC_H_
#define _NET_PREPROC_H_

#include "hd_type.h"

#define NET_PRE_SCALE_EN            0x00000001      ///< preprocessing scaling enable
#define NET_PRE_SUB_IMG_EN          0x00000002      ///< preprocessing substraction enable
#define NET_PRE_CROP_EN             0x00000004      ///< preprocessing cropping enable
#define NET_PRE_PAD_EN              0x00000008      ///< preprocessing padding enable
#define NET_PRE_ALL_EN              0x0000000f      ///< preprocessing all function enable

/**
    preprocessing image flip
*/
//@{
typedef enum
{
    NET_IMG_FLIP_NO         = 0,
    NET_IMG_FLIP_X			= 1,
    NET_IMG_FLIP_Y,
    NET_IMG_FLIP_XY,
    ENUM_DUMMY4WORD(NET_IMG_FLIP)
} NET_IMG_FLIP;
//@}

/**
    preprocessing image rotate
*/
//@{
typedef enum
{
    NET_IMG_ROTATE0          = 0,
    NET_IMG_ROTATE90         = 1,
    NET_IMG_ROTATE180,
    NET_IMG_ROTATE270,
    ENUM_DUMMY4WORD(NET_IMG_ROTATE)
} NET_IMG_ROTATE;
//@}

/**
    select preprocessing input image format
*/
//@{
typedef enum
{
	NET_IMG_RGB_PLANE	= 0,
    NET_IMG_BGR_PLANE	= 1,
    NET_IMG_YUV420,
    NET_IMG_YUV422,
    NET_IMG_YONLY,
    ENUM_DUMMY4WORD(NET_IMG_FMT)
} NET_IMG_FMT;
//@}

/**
    scaling up or down selection
*/
//@{
typedef enum {
	NET_SCALE_DN = 0,                ///< scaling down operation
	NET_SCALE_UP = 1,                ///< scaling up operation
	ENUM_DUMMY4WORD(NET_SCALE_UD)
} NET_SCALE_UD;
//@}

/**
    image scaling mode selection
*/
//@{
typedef enum {
	NET_SCALE_BILINEAR    = 1,       ///< bilinear interpolation
	ENUM_DUMMY4WORD(NET_SCALE_MODE)
} NET_SCALE_MODE;
//@}

/**
    select image substraction mode
*/
//@{
typedef enum {
	NET_SUB_IMG_DC       = 0,        ///< substract image with DC (constant value)
    NET_SUB_IMG_PLANE    = 1,        ///< substract image with plane
	ENUM_DUMMY4WORD(NET_SUB_IMG_MODE)
} NET_SUB_IMG_MODE;
//@}

/**
    select image plane scaling mode
*/
//@{
typedef enum {
	NET_PLANE_SCALE_DUP  = 0,        ///< plane duplicate scaling
	ENUM_DUMMY4WORD(NET_PLANE_SCALE_MODE)
} NET_PLANE_SCALE_MODE;
//@}

/**
    scaling kernel parameters configuration
*/
//@{
typedef struct _NET_SCALE_KERPARM
{
    NET_SCALE_MODE mode;        ///< image scaling mode selection
    UINT16 scl_out_width;           ///< output scale width
    UINT16 scl_out_height;          ///< output scale height
} NET_SCALE_KERPARM;
//@}

/**
    image substraction kernel parameters configuration
*/
//@{
typedef struct _NET_SUB_IMG_KERPARM
{
    NET_SUB_IMG_MODE sub_mode;			///< substract image mode (DC, plane)
    NET_PLANE_SCALE_MODE scl_mode;		///< plane scaling mode (duplicate)
    INT32 dc_val[3];                    ///< dc value
    UINT32 plane_addr;                  ///< plane iamge address
    UINT16 plane_w;                     ///< plane image width
    UINT16 plane_h;                     ///< plane image height
    INT8 sub_img_shf;                   ///< shift after substracting image; right shift(>0), left shift(<0)
} NET_SUB_IMG_KERPARM;
//@}

/**
    image cropping kernel parameters configuration
*/
//@{
typedef struct _NET_CROP_KERPARM
{
    UINT16 crop_x;                  ///< image crop x
    UINT16 crop_y;                  ///< image crop y
    UINT16 crop_w;                  ///< image crop width
    UINT16 crop_h;                  ///< image crop height
} NET_CROP_KERPARM;
//@}

/**
    image padding kernel parameters configuration
*/
//@{
typedef struct _NET_PAD_KERPARM
{
    UINT8 top_pad_num;              ///< number of padding top pixels
    UINT8 bot_pad_num;              ///< number of padding bottom pixels
    UINT8 left_pad_num;             ///< number of padding left pixels
    UINT8 right_pad_num;            ///< number of padding right pixels
    INT32 pad_val;                  ///< padding value	0x0BGR
} NET_PAD_KERPARM;
//@}

/*
    Preprocessing parameters
    scaling -> substraction -> crop -> padding -> rotate
*/
//@{
typedef struct _NN_PRE_PARM
{
    UINT32 func_en;                     ///< default: NET_PRE_ALL_EN
    NET_IMG_FMT in_fmt;					///< input image format
    NET_IMG_FMT out_fmt;				///< output image format
	INT8 out_data_sign;					///< output data sign (s8/u8)
    UINT16 width;                       ///< input image width
    UINT16 height;                      ///< input image height
    UINT32 in_addr;                     ///< input image address
    UINT32 out_addr;                    ///< output image address
    UINT32 interm_addr;					///< intermediate address
    UINT32 in_lofs;                     ///< input line offset
    UINT32 out_lofs;                    ///< output line offset
	NET_IMG_FLIP flip;					///< image flip
    NET_IMG_ROTATE rotate;				///< image rotation angle
    NET_SCALE_KERPARM scale;			///< image scaling parameters
    NET_SUB_IMG_KERPARM sub_img;		///< image subtraction parameters
    NET_CROP_KERPARM crop;				///< image cropping parameters
    NET_PAD_KERPARM pad;				///< image padding parameters
} NN_PRE_PARM;
//@}

#endif