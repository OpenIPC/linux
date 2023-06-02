#ifndef _AF_UI_H_
#define _AF_UI_H_

#define AF_UI_INFOR_ERR   0xFFFFFFFF

typedef struct _AF_UI_INFO_TAB {
	UINT32 value;
	UINT32 max_value;
} AF_UI_INFO_TAB;

/**
	AF informatin selection index(af_ui_set_info() or af_ui_get_info())
*/
typedef enum _AF_UI_INFO {
	AF_UI_MODE,
	AF_UI_OPERATION,                ///<AF movie/photo, data type ##AF_OPERATION
	AF_UI_MAX_CNT,
	ENUM_DUMMY4WORD(AF_UI_INFO)
} AF_UI_INFO;

/**
	AF scene mode
*/
typedef enum _AF_SEARCH_MODE {
	AF_SEARCH_MODE_AUTO,
	AF_SEARCH_MODE_MACRO,
	AF_SEARCH_MODE_INIFINITE,
	AF_SEARCH_MODE_MANUAL,
	AF_SEARCH_MODE_MAX_CNT,
	ENUM_DUMMY4WORD(AF_SEARCH_MODE)
} AF_SEARCH_MODE;

/**
	AFmovie/photo
*/
typedef enum _AF_OPERATION {
	AF_OPERATION_MOVIE,
	AF_OPERATION_PHOTO,
	AF_OPERATION_CAPTURE,
	AF_OPERATION_MAX_CNT,
	ENUM_DUMMY4WORD(AF_OPERATION)
} AF_OPERATION;

#endif

