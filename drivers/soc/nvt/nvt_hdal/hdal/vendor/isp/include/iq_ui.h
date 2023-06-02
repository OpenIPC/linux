#ifndef _IQ_UI_H_
#define _IQ_UI_H_

#define IQ_UI_INFOR_ERR   0xFFFFFFFF

typedef struct _IQ_UI_INFO_TAB {
	UINT32 value;
	UINT32 max_value;
} IQ_UI_INFO_TAB;

/**
	IQ informatin selection index(iq_set_ui_info() or iq_get_ui_info())
*/
typedef enum _IQ_UI_ITEM {
	IQ_UI_ITEM_NR_LV,                   ///<NR level, data type #IQ_UI_NR_LV
	IQ_UI_ITEM_3DNR_LV,                 ///<3DNR level, data type #IQ_UI_3DNR_LV
	IQ_UI_ITEM_SHARPNESS_LV,            ///<Sharpness level, data type #IQ_UI_SHARPNESS_LV
	IQ_UI_ITEM_SATURATION_LV,           ///<Saturation level, data type #IQ_UI_SATURATION_LV
	IQ_UI_ITEM_CONTRAST_LV,             ///<Contrast level, data type #IQ_UI_CONTRAST_LV
	IQ_UI_ITEM_BRIGHTNESS_LV,           ///<Brightness level, data type #IQ_UI_BRIGHTNESS_LV
	IQ_UI_ITEM_NIGHT_MODE,              ///<Night mode on/off, data type #IQ_UI_NIGHT_MODE
	IQ_UI_ITEM_YCC_FORMAT,              ///<YCC format, data type #IQ_UI_YCC_FORMAT
	IQ_UI_ITEM_OPERATION,               ///<IQ movie/photo, data type ##IQ_UI_OPERATION
	IQ_UI_ITEM_IMAGEEFFECT,             ///<Color effect, data type #IQ_UI_IMAGEEFFECT
	IQ_UI_ITEM_CCID,                    ///<Image effect "selective color" index, data type #IQ_UI_CCID
	IQ_UI_ITEM_HUE_SHIFT,               ///<HUE, data type #IQ_UI_HUE_SHIFT
	IQ_UI_ITEM_SHDR_TONE_LV,            ///<Tone level for SHDR, data type #IQ_UI_SHDR_TONE_LV
	IQ_UI_ITEM_MAX_CNT,
	ENUM_DUMMY4WORD(IQ_UI_INFO)
} IQ_UI_ITEM;

/**
	2d noise reduction
*/
typedef enum _IQ_UI_NR_LV {
	IQ_UI_NR_LV_N1             = 50,     ///< NR level 25
	IQ_UI_NR_LV_NORMAL         = 100,    ///< NR level 50
	IQ_UI_NR_LV_P1             = 150,    ///< NR level 75
	IQ_UI_NR_LV_MAX_CNT        = 201,
	ENUM_DUMMY4WORD(IQ_UI_NR_LV)
} IQ_UI_NR_LV;

/**
	3d noise reduction
*/
typedef enum _IQ_UI_3DNR_LV {
	IQ_UI_3DNR_LV_N1           = 50,     ///< 3DNR level 25
	IQ_UI_3DNR_LV_NORMAL       = 100,    ///< 3DNR level 50
	IQ_UI_3DNR_LV_P1           = 150,    ///< 3DNR level 75
	IQ_UI_3DNR_LV_MAX_CNT      = 201,
	ENUM_DUMMY4WORD(IQ_UI_3DNR_LV)
} IQ_UI_3DNR_LV;


/**
	Sharpness
*/
typedef enum _IQ_UI_SHARPNESS_LV {
	IQ_UI_SHARPNESS_LV_N1      = 50,     ///< Sharpness 25
	IQ_UI_SHARPNESS_LV_NORMAL  = 100,    ///< Sharpness 50
	IQ_UI_SHARPNESS_LV_P1      = 150,    ///< Sharpness 75
	IQ_UI_SHARPNESS_LV_MAX_CNT = 201,
	ENUM_DUMMY4WORD(IQ_UI_SHARPNESS_LV)
} IQ_UI_SHARPNESS_LV;

/**
	Saturation
*/
typedef enum _IQ_UI_SATURATION_LV {
	IQ_UI_SATURATION_LV_N1      = 50,    ///< Saturation 25
	IQ_UI_SATURATION_LV_NORMAL  = 100,   ///< Saturation 50
	IQ_UI_SATURATION_LV_P1      = 150,   ///< Saturation 75
	IQ_UI_SATURATION_LV_MAX_CNT = 201,
	ENUM_DUMMY4WORD(IQ_UI_SATURATION_LV)
} IQ_UI_SATURATION_LV;

/**
	Contrast
*/
typedef enum _IQ_UI_CONTRAST_LV {
	IQ_UI_CONTRAST_LV_N1       = 50,     ///< Contrast 25
	IQ_UI_CONTRAST_LV_NORMAL   = 100,    ///< Contrast 50
	IQ_UI_CONTRAST_LV_P1       = 150,    ///< Contrast 75
	IQ_UI_CONTRAST_LV_MAX_CNT  = 201,
	ENUM_DUMMY4WORD(IQ_UI_CONTRAST_LV)
} IQ_UI_CONTRAST_LV;

/**
	Brightness
*/
typedef enum _IQ_UI_BRIGHTNESS_LV {
	IQ_UI_BRIGHTNESS_LV_N1       = 50,   ///< Brightness 25
	IQ_UI_BRIGHTNESS_LV_NORMAL   = 100,  ///< Brightness 50
	IQ_UI_BRIGHTNESS_LV_P1       = 150,  ///< Brightness 75
	IQ_UI_BRIGHTNESS_LV_MAX_CNT  = 201,
	ENUM_DUMMY4WORD(IQ_UI_BRIGHTNESS_LV)
} IQ_UI_BRIGHTNESS_LV;

/**
	Night mode on/off
*/
typedef enum _IQ_UI_NIGHT_MODE {
	IQ_UI_NIGHT_MODE_OFF,
	IQ_UI_NIGHT_MODE_ON,
	IQ_UI_NIGHT_MODE_MAX_CNT,
	ENUM_DUMMY4WORD(IQ_UI_NIGHT_MODE)
} IQ_UI_NIGHT_MODE;

/**
	YCC_FORMAT
*/
typedef enum _IQ_UI_YCC_FORMAT {
	IQ_UI_YCC_OUT_FULL      = 0,
	IQ_UI_YCC_OUT_BT601,
	IQ_UI_YCC_OUT_BT709,
	IQ_UI_YCC_OUT_MAX_CNT,
	ENUM_DUMMY4WORD(IQ_UI_YCC_FORMAT)
} IQ_UI_YCC_FORMAT;

/**
	IQ movie/photo
*/
typedef enum _IQ_UI_OPERATION {
	IQ_UI_OPERATION_MOVIE,
	IQ_UI_OPERATION_PHOTO,
	IQ_UI_OPERATION_CAPTURE,
	IQ_UI_OPERATION_MAX_CNT,
	ENUM_DUMMY4WORD(IQ_UI_OPERATION)
} IQ_UI_OPERATION;

/**
	Image effect
*/
typedef enum _IQ_UI_IMAGEEFFECT {
	IQ_UI_IMAGEEFFECT_OFF,
	IQ_UI_IMAGEEFFECT_BW,
	IQ_UI_IMAGEEFFECT_SEPIA,
	IQ_UI_IMAGEEFFECT_VIVID,
	IQ_UI_IMAGEEFFECT_ROCK,
	IQ_UI_IMAGEEFFECT_COOLGREEN,
	IQ_UI_IMAGEEFFECT_WARMYELLOW,
	IQ_UI_IMAGEEFFECT_SKETCH,
	IQ_UI_IMAGEEFFECT_COLORPENCIL,
	IQ_UI_IMAGEEFFECT_FISHEYE,
	IQ_UI_IMAGEEFFECT_NEGATIVE_DEFOG,
	IQ_UI_IMAGEEFFECT_CCID,
	IQ_UI_IMAGEEFFECT_MAX_CNT,
	ENUM_DUMMY4WORD(IQ_UI_IMAGEEFFECT)
} IQ_UI_IMAGEEFFECT;

/**
	Image effect: select color id
*/
typedef enum _IQ_UI_CCID_ {
	IQ_UI_CCID_NO_EFFECT,
	IQ_UI_CCID_ENHANCE_RED,
	IQ_UI_CCID_ENHANCE_YELLOW,
	IQ_UI_CCID_ENHANCE_GREEN,
	IQ_UI_CCID_ENHANCE_BLUE,
	IQ_UI_CCID_ENHANCE_CYAN,
	IQ_UI_CCID_ENHANCE_ORANGE,
	IQ_UI_CCID_ENHANCE_MAGENTA,
	IQ_UI_CCID_REMOVAL_RED,
	IQ_UI_CCID_REMOVAL_YELLOW,
	IQ_UI_CCID_REMOVAL_GREEN,
	IQ_UI_CCID_REMOVAL_BLUE,
	IQ_UI_CCID_REMOVAL_CYAN,
	IQ_UI_CCID_REMOVAL_ORANGE,
	IQ_UI_CCID_REMOVAL_MAGENTA,
	IQ_UI_CCID_MAX_CNT,
	ENUM_DUMMY4WORD(IQ_UI_CCID)
} IQ_UI_CCID;

/**
	HUE
*/
typedef enum _IQ_UI_HUE_SHIFT {
	IQ_UI_HUE_SHIFT_0       = 0,    ///< HUE Normal
	IQ_UI_HUE_SHIFT_MAX_CNT = 361,
	ENUM_DUMMY4WORD(IQ_UI_HUE_SHIFT)
} IQ_UI_HUE_SHIFT;

/**
	SHDR Tone
*/
typedef enum _IQ_UI_SHDR_TONE_LV {
	IQ_UI_SHDR_TONE_LV_N1       = 25,     ///< SHDR TONE 25
	IQ_UI_SHDR_TONE_LV_NORMAL   = 50,     ///< SHDR TONE 50
	IQ_UI_SHDR_TONE_LV_P1       = 75,     ///< SHDR TONE 75
	IQ_UI_SHDR_TONE_LV_MAX_CNT  = 101,
	ENUM_DUMMY4WORD(IQ_UI_SHDR_TONE_LV)
} IQ_UI_SHDR_TONE_LV;

#endif

