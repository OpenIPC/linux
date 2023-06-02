#ifndef _AWB_UI_H_
#define _AWB_UI_H_

#define AWB_UI_INFOR_ERR   0xFFFFFFFF

typedef struct _AWB_UI_INFO_TAB {
	UINT32 value;
	UINT32 max_value;
} AWB_UI_INFO_TAB;

/**
	AWB informatin selection index(awb_ui_set_info() or awb_ui_get_info())
*/
typedef enum _AWB_UI_INFO {
	AWB_UI_SCENE,        ///<WB mode, data type #AWB_WB
	AWB_UI_WB_R_RATIO,   ///<WB R ratio, data type #AWB_WB
	AWB_UI_WB_B_RATIO,   ///<WB B ratio, data type #AWB_WB
	AWB_UI_OPERATION,    ///<AWB movie/photo, data type ##AWB_OPERATION
	AWB_UI_STITCH_ID,    ///<AWB stitch id, data type #AWB_STITCH_ID
	AWB_UI_MAX_CNT,
	ENUM_DUMMY4WORD(AWB_UI_INFO)
} AWB_UI_INFO;

/**
	AWB scene mode
*/
typedef enum _AWB_SCENE {
	AWB_SCENE_AUTO,            ///<Auto mode
	AWB_SCENE_DAYLIGHT,        ///<Daylight mode
	AWB_SCENE_CLOUDY,          ///<Cloudy mode
	AWB_SCENE_TUNGSTEN,        ///<Tugsten mode
	AWB_SCENE_SUNSET,          ///<Sunset mode
	AWB_SCENE_CUSTOMER1,       ///<-
	AWB_SCENE_CUSTOMER2,       ///<-
	AWB_SCENE_CUSTOMER3,       ///<-
	AWB_SCENE_CUSTOMER4,       ///<-
	AWB_SCENE_CUSTOMER5,       ///<-
	AWB_SCENE_NIGHT_MODE,      ///<-
	AWB_SCENE_MGAIN,           ///<-Manually R/G/B gain
	AWB_SCENE_MAX_CNT,
	ENUM_DUMMY4WORD(AWB_SCENE)
} AWB_SCENE;

/**
	AWB ui R ratio
*/
typedef enum _AWB_R_RATIO {
	AWB_R_RATIO_N1          = 50,     ///< R ratio 50
	AWB_R_RATIO_NORMAL      = 100,    ///< R ratio 100
	AWB_R_RATIO_P1          = 200,    ///< R ratio 200
	AWB_R_RATIO_MAX_CNT     = 401,
	ENUM_DUMMY4WORD(AWB_R_RATIO)
} AWB_R_RATIO;

/**
	AWB ui B ratio
*/
typedef enum _AWB_B_RATIO {
	AWB_B_RATIO_N1          = 50,     ///< B ratio 50
	AWB_B_RATIO_NORMAL      = 100,    ///< B ratio 100
	AWB_B_RATIO_P1          = 200,    ///< B ratio 200
	AWB_B_RATIO_MAX_CNT     = 401,
	ENUM_DUMMY4WORD(AWB_B_RATIO)
} AWB_B_RATIO;

/**
	AWB movie/photo
*/
typedef enum _AWB_OPERATION {
	AWB_OPERATION_MOVIE,
	AWB_OPERATION_PHOTO,
	AWB_OPERATION_CAPTURE,
	AWB_OPERATION_MAX_CNT,
	ENUM_DUMMY4WORD(AWB_OPERATION)
} AWB_OPERATION;

/**
	AWB stitch id
*/
typedef enum _AWB_STITCH_ID {
	AWB_STITCH_ID_NORMAL = 0,
	AWB_STITCH_ID_MAX    = 0xFFFFFFFF,
	ENUM_DUMMY4WORD(AWB_STITCH_ID)
} AWB_STITCH_ID;

#endif

