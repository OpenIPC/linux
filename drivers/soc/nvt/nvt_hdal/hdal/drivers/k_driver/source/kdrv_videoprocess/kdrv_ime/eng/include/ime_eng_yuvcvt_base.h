

#ifndef _IME_ENG_FUNCTION_BASE_H_
#define _IME_ENG_FUNCTION_BASE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "kwrap/type.h"
#include "ime_eng_int_comm.h"


#define IME_RNG_SEL_8   0
#define IME_RNG_SEL_16  1
#define IME_RNG_SEL_32  2
#define IME_RNG_SEL_64  3


#define IME_WT_SEL_8    0
#define IME_WT_SEL_16   1
#define IME_WT_SEL_32   2
#define IME_WT_SEL_64   3


/**
    IME enum - YUV converter selection
*/
typedef enum _IME_YUV_CVT_ {
	IME_YUV_CVT_BT601 = 0,  ///< BT.601
	IME_YUV_CVT_BT709 = 1,  ///< BT.709
	ENUM_DUMMY4WORD(IME_YUV_CVT)
} IME_YUV_CVT;


/**
    IME enum - YUV data type
*/
typedef enum _IME_YUV_TYPE_ {
	IME_YUV_TYPE_FULL  = 0,  ///< full range
	IME_YUV_TYPE_BT601 = 1,  ///< BT.601
	IME_YUV_TYPE_BT709 = 2,  ///< BT.709
	ENUM_DUMMY4WORD(IME_YUV_TYPE)
} IME_YUV_TYPE;




//-------------------------------------------------------------------
// YUV converter
extern VOID ime_eng_set_yuv_converter_enable_buf_reg(UINT32 set_en);
extern VOID ime_eng_set_yuv_converter_sel_buf_reg(UINT32 set_sel);

#if (IME_GET_API_EN == 1)
extern UINT32 ime_eng_get_yuv_converter_enable_buf_reg(VOID);
extern UINT32 ime_eng_get_yuv_converter_sel_buf_reg(VOID);
#endif


#ifdef __cplusplus
}
#endif


#endif


