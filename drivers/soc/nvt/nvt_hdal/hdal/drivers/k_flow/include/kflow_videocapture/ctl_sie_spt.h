/**
    Public header file for SIE limit

    This file is the header file that define SIE limit.

    @file      ctl_sie_spt.h
    @ingroup    mILibSIECom
    @note      Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2020.  All rights reserved.
*/
#ifndef _CTL_SIE_SPT_H_
#define _CTL_SIE_SPT_H_
#include "kflow_videocapture/ctl_sie_utility.h"

/*
    output format support
*/
typedef enum {
	CTL_SIE_SPT_BAYER_8         = (1 << CTL_SIE_BAYER_8),       // 0x1
	CTL_SIE_SPT_BAYER_10        = (1 << CTL_SIE_BAYER_10),      // 0x2
	CTL_SIE_SPT_BAYER_12        = (1 << CTL_SIE_BAYER_12),      // 0x4
	CTL_SIE_SPT_BAYER_16        = (1 << CTL_SIE_BAYER_16),      // 0x8
	CTL_SIE_SPT_BAYER_16_MSB    = (1 << CTL_SIE_BAYER_16_MSB),  // 0x8
	CTL_SIE_SPT_BAYER_16_LSB    = (1 << CTL_SIE_BAYER_16_LSB),  // 0x10
	CTL_SIE_SPT_YUV_422_SPT     = (1 << CTL_SIE_YUV_422_SPT),   // 0x20
	CTL_SIE_SPT_YUV_422_NOSPT   = (1 << CTL_SIE_YUV_422_NOSPT), // 0x40
	CTL_SIE_SPT_YUV_420_SPT     = (1 << CTL_SIE_YUV_420_SPT),   // 0x80
	CTL_SIE_SPT_Y_8             = (1 << CTL_SIE_Y_8),           // 0x100
	CTL_SIE_SPT_DATAFORMAT_ALL  = (CTL_SIE_SPT_BAYER_8 | CTL_SIE_SPT_BAYER_10 | CTL_SIE_SPT_BAYER_12 | CTL_SIE_SPT_BAYER_16 | CTL_SIE_SPT_BAYER_16_LSB | CTL_SIE_SPT_YUV_422_SPT | CTL_SIE_SPT_YUV_422_NOSPT | CTL_SIE_YUV_420_SPT | CTL_SIE_SPT_YUV_420_SPT | CTL_SIE_SPT_Y_8),
	ENUM_DUMMY4WORD(CTL_SIE_DATAFORMAT_SPT)
} CTL_SIE_DATAFORMAT_SPT;

/*
    function support
*/
typedef enum {
	CTL_SIE_FUNC_SPT_NONE           = 0x00000000, ///< none
	CTL_SIE_FUNC_SPT_DIRECT         = 0x00000001, ///< direct to ipp
	CTL_SIE_FUNC_SPT_PATGEN         = 0x00000002, ///< pattern gen
	CTL_SIE_FUNC_SPT_DVI            = 0x00000004, ///< dvi sensor
	CTL_SIE_FUNC_SPT_OB_AVG         = 0x00000008, ///< ob average
	CTL_SIE_FUNC_SPT_OB_BYPASS      = 0x00000010, ///< ob bypass
	CTL_SIE_FUNC_SPT_OB_FRAME_AVG   = 0x00000020, ///< ob frame average
	CTL_SIE_FUNC_SPT_OB_PLANE_SUB   = 0x00000040, ///< ob plane sub
	CTL_SIE_FUNC_SPT_YOUT           = 0x00000080, ///< y out
	CTL_SIE_FUNC_SPT_CGAIN          = 0x00000100, ///< color gain
	CTL_SIE_FUNC_SPT_GRID_LINE      = 0x00000200, ///< grid line
	CTL_SIE_FUNC_SPT_DPC            = 0x00000400, ///< defect pixel compensation
	CTL_SIE_FUNC_SPT_PFPC           = 0x00000800, ///< period fixed pattern compensation
	CTL_SIE_FUNC_SPT_ECS            = 0x00001000, ///< ecs
	CTL_SIE_FUNC_SPT_DGAIN          = 0x00002000, ///< digital gain
	CTL_SIE_FUNC_SPT_BS_H           = 0x00004000, ///< horizontal bayer scale
	CTL_SIE_FUNC_SPT_BS_V           = 0x00008000, ///< vertical bayer scale
	CTL_SIE_FUNC_SPT_FLIP_H         = 0x00010000, ///< horizontal flip
	CTL_SIE_FUNC_SPT_FLIP_V         = 0x00020000, ///< vertical flip
	CTL_SIE_FUNC_SPT_RAWENC         = 0x00040000, ///< raw compress
	CTL_SIE_FUNC_SPT_LA             = 0x00080000, ///< luminance accumulation output
	CTL_SIE_FUNC_SPT_LA_HISTO       = 0x00100000, ///< luminance accumulation histogram output
	CTL_SIE_FUNC_SPT_CA             = 0x00200000, ///< color accumulation output
	CTL_SIE_FUNC_SPT_VA             = 0x00400000, ///< variation accumulation output
	CTL_SIE_FUNC_SPT_ETH            = 0x00800000, ///< edgh threshold output
	CTL_SIE_FUNC_SPT_COMPANDING     = 0x01000000, ///< companding function
	CTL_SIE_FUNC_SPT_RGBIR_FMT_SEL  = 0x02000000, ///< RGBIR format Selection
	CTL_SIE_FUNC_SPT_RCCB_FMT_SEL   = 0x04000000, ///< RCCB format Selection
	CTL_SIE_FUNC_SPT_RGGB_FMT_SEL   = 0x08000000, ///< RGGB format Selection
	CTL_SIE_FUNC_SPT_SINGLE_OUT     = 0x10000000, ///< single out
	CTL_SIE_FUNC_SPT_RING_BUF       = 0x20000000, ///< ring buffer
	CTL_SIE_FUNC_SPT_MD             = 0x40000000, ///< md
	CTL_SIE_FUNC_SPT_DVS            = 0x80000000, ///< dvs data parser
	CTL_SIE_FUNC_SPT_VD_DLY         = 0x100000000LLU, ///< vd delay
} CTL_SIE_FUNC_SPT;


/*
    in/out size support
*/
typedef struct {
	/* active */
	URECT act_align;
	USIZE act_min;
	USIZE act_max;

	/* crop */
	URECT crp_align;
	USIZE crp_min;
	USIZE crp_max;

	/* scale output */
	USIZE scl_out_align;
	USIZE scl_out_min;
	USIZE scl_out_max;
	USIZE scl_ratio_max; // scale ratio = scale_output_size / crp_size

} CTL_SIE_IOSIZE_SPT;

typedef enum {
	CTL_SIE_SPT_ITEM_DATAFORMAT,        // data_type: CTL_SIE_DATAFORMAT_SPT
	CTL_SIE_SPT_ITEM_DATAFORMAT_RAWENC, // data_type: CTL_SIE_DATAFORMAT_SPT
	CTL_SIE_SPT_ITEM_FUNC,              // data_type: CTL_SIE_FUNC_SPT
	CTL_SIE_SPT_ITEM_IOSIZE,            // data_type: CTL_SIE_IOSIZE_SPT
	ENUM_DUMMY4WORD(CTL_SIE_SPT_ITEM)
} CTL_SIE_SPT_ITEM;

/*
    ctl_sie support function or parameters

    input :
        CTL_SIE_ID id
        CTL_SIE_SPT_ITEM item
    output :
        void *spt
*/
void ctl_sie_spt(CTL_SIE_ID id, CTL_SIE_SPT_ITEM item, void *spt);

#endif //_CTL_SIE_SPT_H_
