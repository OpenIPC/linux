
#ifndef _SIE_LIMIT_H_
#define _SIE_LIMIT_H_

//=====================================================
// input limitation
#define SIE_MAX_ENG_NUM                3
#define SIE1_MAX_OUTPUT_NUM            3 // main-out(Y-out), CA(uv-out), LA
#define SIE1_MAX_INPUT_NUM             2 // DPC, ECS
#define SIE2_MAX_OUTPUT_NUM            3 // main-out(Y-out), CA(uv-out), LA
#define SIE2_MAX_INPUT_NUM             2 // DPC, ECS
#define SIE3_MAX_OUTPUT_NUM            2 // Y-out, uv-out
#define SIE3_MAX_INPUT_NUM             0



#define SIE_CA_MAX_WIN_NUM            32
#define SIE_LA_MAX_WIN_NUM            32
#define SIE_LA_HISTO_BIN_NUM          64

#define SIE_PATGEN_SRC_WIN_W_ALIGN    2
#define SIE_PATGEN_SRC_WIN_H_ALIGN    2 // 1
#define SIE_PATGEN_SRC_WIN_MAX_W      16382  // 14bit
#define SIE_PATGEN_SRC_WIN_MAX_H      16382 //16383  // 14bit
#define SIE_PATGEN_SRC_WIN_MIN_W      2
#define SIE_PATGEN_SRC_WIN_MIN_H      2//1

#define SIE_ACT_WIN_W_ALIGN           2
#define SIE_ACT_WIN_H_ALIGN           2//1
#define SIE_ACT_WIN_X_ALIGN           2//1
#define SIE_ACT_WIN_Y_ALIGN           2//1
#define SIE_ACT_WIN_MAX_W             16382
#define SIE_ACT_WIN_MAX_H             16382//16383
#define SIE_ACT_WIN_MIN_W             2
#define SIE_ACT_WIN_MIN_H             2//1

#define SIE_CROP_WIN_W_ALIGN          2
#define SIE_CROP_WIN_H_ALIGN          2//1
#define SIE_CROP_WIN_X_ALIGN          2//1
#define SIE_CROP_WIN_Y_ALIGN          2//1
#define SIE_CROP_WIN_MAX_W            16382
#define SIE_CROP_WIN_MAX_H            16382//16383
#define SIE_CROP_WIN_MIN_W            2
#define SIE_CROP_WIN_MIN_H            2//1

#define SIE_SCALE_WIN_W_ALIGN         2
#define SIE_SCALE_WIN_H_ALIGN         2
#define SIE_SCALE_INPUT_WIN_MAX_W     4096
#define SIE_SCALE_INPUT_WIN_MAX_H     4096
#define SIE_SCALE_OUTPUT_WIN_MAX_W    2688
#define SIE_SCALE_OUTPUT_WIN_MAX_H    2688
#define SIE_SCALE_W_RATIO             8
#define SIE_SCALE_H_RATIO             8

#define SIE_CA_CROP_WIN_W_ALIGN       2
#define SIE_CA_CROP_WIN_H_ALIGN       2
#define SIE_CA_CROP_WIN_X_ALIGN       2//1
#define SIE_CA_CROP_WIN_Y_ALIGN       2//1
#define SIE_CA_CROP_WIN_MAX_W         10208 //(16383*(640/2-1) / 1024 + 1 )*2 = 10208
#define SIE_CA_CROP_WIN_MAX_H         8160 //(16383*(512/2-1) / 1024 + 1 )*2 = 8160

#define SIE_LA_CROP_WIN_W_ALIGN       2
#define SIE_LA_CROP_WIN_H_ALIGN       2
#define SIE_LA_CROP_WIN_X_ALIGN       2//1
#define SIE_LA_CROP_WIN_Y_ALIGN       2//1
#define SIE_LA_CROP_WIN_MAX_W         320 // scale down to 640, split to rgb, transfer to Y only
#define SIE_LA_CROP_WIN_MAX_H         256 // scale down to 512, split to rgb, transfer to Y only


#define SIE_OUT0_LINEOFFSET_ALIGN     4 // word align
#define SIE_OUT1_LINEOFFSET_ALIGN     4 // word align
#define SIE_OUT2_LINEOFFSET_ALIGN     4 // word align

#define SIE_RING_BUFFER_MAX_LEN       2047

typedef enum {
    SIE_SSDRV_FUNC_NONE          = 0x00000000, ///< none
    SIE_SSDRV_FUNC_DIRECT        = 0x00000001, ///< direct to ipp
    SIE_SSDRV_FUNC_PATGEN        = 0x00000002, ///< pattern gen
    SIE_SSDRV_FUNC_DVI           = 0x00000004, ///< yuv(ccir) sensor
    SIE_SSDRV_FUNC_OB_AVG        = 0x00000008, ///< ob average
    SIE_SSDRV_FUNC_OB_BYPASS     = 0x00000020, ///< ob bypass
    SIE_SSDRV_FUNC_CGAIN         = 0x00000200, ///< color gain
    SIE_SSDRV_FUNC_DVS           = 0x00000400, ///< dvs data parser
    SIE_SSDRV_FUNC_DPC           = 0x00000800, ///< defect pixel compensation
    SIE_SSDRV_FUNC_MD            = 0x00001000, ///< motion detection
    SIE_SSDRV_FUNC_ECS           = 0x00008000, ///< ecs
    SIE_SSDRV_FUNC_DGAIN         = 0x00010000, ///< digital gain
    SIE_SSDRV_FUNC_BS_H          = 0x00020000, ///< horizontal bayer scale
    SIE_SSDRV_FUNC_BS_V          = 0x00040000, ///< vertical bayer scale
    SIE_SSDRV_FUNC_RAWENC        = 0x00080000, ///< raw compression
    SIE_SSDRV_FUNC_LA_HISTO      = 0x00200000, ///< luminance accumulation histogram output
    SIE_SSDRV_FUNC_LA            = 0x00800000, ///< luminance accumulation output
    SIE_SSDRV_FUNC_CA            = 0x01000000, ///< color accumulation output
    SIE_SSDRV_FUNC_COMPANDING    = 0x10000000, ///< companding function
    SIE_SSDRV_FUNC_RGGB_FMT_SEL  = 0x20000000, ///< RGGB format Selection
    SIE_SSDRV_FUNC_RGBIR_FMT_SEL = 0x40000000, ///< RGBIR 4x4 format Selection
    SIE_SSDRV_FUNC_FLIP_H        = 0x00100000, ///< mirror
    SIE_SSDRV_FUNC_FLIP_V        = 0x00400000, ///< fliP
    SIE_SSDRV_FUNC_SINGLE_OUT    = 0x02000000, ///< single out
    SIE_SSDRV_FUNC_RING_BUF      = 0x04000000, ///< ring buffer, Note: SIE1/2 support ring-buffer
    SIE_SSDRV_FUNC_VDHD_DLY		 = 0x08000000, ///< vdhd delay, Note: SIE1/2/3 support vdhd_delay

} SIE_SSDRV_FUNC_SUPPORT;


#define SIE1_SUPPORT_FUNC (SIE_SSDRV_FUNC_DIRECT|SIE_SSDRV_FUNC_PATGEN|SIE_SSDRV_FUNC_DVI|SIE_SSDRV_FUNC_OB_AVG|SIE_SSDRV_FUNC_DVS| \
                           SIE_SSDRV_FUNC_OB_BYPASS|SIE_SSDRV_FUNC_CGAIN|SIE_SSDRV_FUNC_DPC|SIE_SSDRV_FUNC_MD|SIE_SSDRV_FUNC_ECS| \
                           SIE_SSDRV_FUNC_DGAIN|SIE_SSDRV_FUNC_BS_H|SIE_SSDRV_FUNC_BS_V| \
                           SIE_SSDRV_FUNC_LA_HISTO|SIE_SSDRV_FUNC_LA|SIE_SSDRV_FUNC_CA|SIE_SSDRV_FUNC_COMPANDING| \
                           SIE_SSDRV_FUNC_RGGB_FMT_SEL|SIE_SSDRV_FUNC_RGBIR_FMT_SEL|SIE_SSDRV_FUNC_FLIP_H|SIE_SSDRV_FUNC_FLIP_V| \
                           SIE_SSDRV_FUNC_SINGLE_OUT                       |SIE_SSDRV_FUNC_VDHD_DLY)

#define SIE2_SUPPORT_FUNC (                      SIE_SSDRV_FUNC_PATGEN|SIE_SSDRV_FUNC_DVI|SIE_SSDRV_FUNC_OB_AVG|SIE_SSDRV_FUNC_DVS| \
                           SIE_SSDRV_FUNC_OB_BYPASS|SIE_SSDRV_FUNC_CGAIN|SIE_SSDRV_FUNC_DPC                  |SIE_SSDRV_FUNC_ECS| \
                           SIE_SSDRV_FUNC_DGAIN|SIE_SSDRV_FUNC_BS_H|SIE_SSDRV_FUNC_BS_V|SIE_SSDRV_FUNC_RAWENC| \
                           SIE_SSDRV_FUNC_LA_HISTO|SIE_SSDRV_FUNC_LA|SIE_SSDRV_FUNC_CA|SIE_SSDRV_FUNC_COMPANDING| \
                           SIE_SSDRV_FUNC_RGGB_FMT_SEL|SIE_SSDRV_FUNC_RGBIR_FMT_SEL|SIE_SSDRV_FUNC_FLIP_H|SIE_SSDRV_FUNC_FLIP_V| \
                           SIE_SSDRV_FUNC_SINGLE_OUT|SIE_SSDRV_FUNC_RING_BUF)

#define SIE3_SUPPORT_FUNC (SIE_SSDRV_FUNC_PATGEN|SIE_SSDRV_FUNC_DVI|SIE_SSDRV_FUNC_CGAIN|SIE_SSDRV_FUNC_SINGLE_OUT|SIE_SSDRV_FUNC_DVS| \
						   SIE_SSDRV_FUNC_DGAIN|SIE_SSDRV_FUNC_FLIP_H|SIE_SSDRV_FUNC_FLIP_V)





#endif

