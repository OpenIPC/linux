#ifndef _AF_ALG_H_
#define _AF_ALG_H_

#if defined(__KERNEL__) || defined(__FREERTOS)
#include "kwrap/type.h"
#endif

//=============================================================================
// struct & definition
//=============================================================================
#define AFALG_MIN(A, B)       (((A) <= (B)) ? (A) : (B))
#define AFALG_MAX(A, B)       (((A) >= (B)) ? (A) : (B))
#define AFALG_ABS(X)          (((X) < 0) ? -(X) : (X))
#define AFALG_DIFF(A, B)      (((A) >= (B)) ? ((A)-(B)) : ((B)-(A)))
#define AFALG_CLAMP(X, L, H)  (((X) >= (H)) ? (H) : (((X) <= (L)) ? (L) : (X)))

#define AF_WIN_HW_X        8
#define AF_WIN_HW_Y        8

#define AF_WIN_ALG_X       8
#define AF_WIN_ALG_Y       8

#define AF_2ND_ID_PARAM    1    // AF_ID_MAX_NUM == 2


/**
	AF id
*/
typedef enum _AF_ID {
	AF_ID_1 = 0,       ///< af id 1
	AF_ID_2,           ///< af id 2
	AF_ID_MAX_NUM,
	ENUM_DUMMY4WORD(AF_ID)
} AF_ID;

/**
	AF shot mode
*/
typedef enum _AFALG_MODE {
	AFALG_MODE_SINGLE = 0,
	AFALG_MODE_CONTINUOUS,
	AFALG_MODE_UNKNOWN,
	ENUM_DUMMY4WORD(AFALG_MODE)
} AFALG_MODE;

/**
	AF execute command
*/
typedef enum _AFALG_CMD {
	AFALG_CMD_STOP = 0,    ///< AF process cmd: stop - initial state
	AFALG_CMD_RUN,         ///< AF process cmd: run  - AF is running
	AFALG_CMD_END,         ///< AF process cmd: end  - AF is finished
	AFALG_CMD_QUIT,        ///< AF process cmd: quit by UI
	ENUM_DUMMY4WORD(AFALG_CMD)
} AFALG_CMD;

typedef enum _AFALG_STATUS_RESULT {
	AFALG_STS_INIT         = 0x00,    ///< AF result: initialize
	AFALG_STS_SUCCESS      = 0x01,    ///< AF result: success
	AFALG_STS_MULTI_TARGET = 0x02,    ///< AF result: multi target
	AFALG_STS_NO_TARGET    = 0x04,    ///< AF result: no target
	AFALG_STS_LOW_VA       = 0x08,    ///< AF result: low VA
	AFALG_STS_FLAT_CURVE   = 0x10,    ///< AF result: too flat
	AFALG_STS_TIME_OUT     = 0x20,    ///< AF result: time out
	AFALG_STS_NO_MOTOR     = 0x40,    ///< AF result: no motor driver exist
	AFALG_STS_FAILURE      = 0x80,    ///< AF result: failure
	ENUM_DUMMY4WORD(AFALG_STATUS_RESULT)
} AFALG_STATUS_RESULT;

typedef enum _AF_ALG_METHOD {
	AF_ALG_HILL_CLIMB = 0,
	AF_ALG_2ND_POLY_FIT,
	AF_ALG_HYBRID,
	AF_ALG_FULL_SEARCH,
	AF_ALG_BACK_FORWARD,
	ENUM_DUMMY4WORD(AF_ALG_METHOD)
} AF_ALG_METHOD;

typedef enum _AF_SHOT_MODE {
	AF_MODE_SINGLE = 0,
	AF_MODE_CONTINUOUS,
	ENUM_DUMMY4WORD(AF_SHOT_MODE)
} AF_SHOT_MODE;

typedef enum _AF_SEARCH_DIR {
	AF_DIR_2TELE = 0,
	AF_DIR_2WIDE,
	ENUM_DUMMY4WORD(AF_SEARCH_DIR)
} AF_SEARCH_DIR;

/**
	AF threshold
*/
typedef struct _AF_THRES {
	INT32 thres_rough;
	INT32 thres_fine;
	INT32 thres_final;
	INT32 thres_restart;
} AF_THRES;

/**
	AF step size
*/
typedef struct _AF_STEP_SIZE {
	INT32 step_rough;
	INT32 step_fine;
	INT32 step_final;
} AF_STEP_SIZE;

/**
	AF window weighting
*/
typedef struct _AF_WIN_WEIGHT {
	UINT8 wei[AF_WIN_ALG_Y][AF_WIN_ALG_X];
} AF_WIN_WEIGHT;

/**
	AF VA statistics
*/
typedef struct _AF_VA_STA {
	UINT32 acc[AF_WIN_HW_Y][AF_WIN_HW_X];
} AF_VA_STA;

typedef enum _AF_DBG_MSG {
	AF_DBG_NONE         = 0x00000000,
	AF_DBG_INFO         = 0x00000001,
	AF_DBG_VA           = 0x00000002,
	AF_DBG_PERF_MEASURE = 0x00000004,
	AF_DBG_DUMP_PROC    = 0x00010000,
	AF_DBG_DUMP_IOCTL   = 0x00020000,
	ENUM_DUMMY4WORD(AF_DBG_MSG)
} AF_DBG_MSG;

/**
	AF parameter
*/
typedef struct _AF_PARAM {
	BOOL          *func_en;
	AF_ALG_METHOD *alg_method;
	AF_SHOT_MODE  *shot_mode;
	AF_SEARCH_DIR *search_dir;
	UINT32        *skip_frame;
	AF_THRES      *thres;
	AF_STEP_SIZE  *step_size;
	UINT32        *max_count;
	AF_WIN_WEIGHT *win_weight;
	BOOL          *retrigger;
	AF_DBG_MSG    *dbg_msg;
} AF_PARAM;

typedef struct _AF_CTRL_OBJ {
	INT32 focus_pos;
	INT32 zoom_pos;
	UINT32 aperture_pos;
	UINT32 shutter_ste;
	BOOL ircut_ste;
} AF_CTRL_OBJ;

typedef enum _AF_VA_OPT {
	AF_VA_G1G2_SUM = 0,
	AF_VA_G1_ONLY,
	AF_VA_G2_ONLY,
	AF_VA_TYPE_MAX,
	ENUM_DUMMY4WORD(AF_VA_OPT)
} AF_VA_OPT;

typedef enum _LENS_STATE {
	LENS_STATE_IDLE            = 0x00,    // lens is in IDLE state
	LENS_STATE_ZOOM_INIT       = 0x01,    // lens is in ZOOM INIT state
	LENS_STATE_FOCUS_INIT      = 0x02,    // lens is in FOCUS INIT state
	LENS_STATE_ZOOM_MOVING     = 0x04,    // lens is in ZOOM moving state
	LENS_STATE_FOCUS_MOVING    = 0x08,    // lens is in FOCUS moving state
	LENS_STATE_APERTURE_MOVING = 0x10,    // lens is in APERTURE moving state
	LENS_STATE_SHUTTER_MOVING  = 0x20,    // lens is in SHUTTER moving state
	LENS_STATE_IRCUT_MOVING    = 0x40,    // lens is in IRCUT moving state
	LENS_STATE_RESET           = 0x80,    // lens is in RESET state
	LENS_STATE_POWER_OFF       = 0x100,   // lens is in POWER_OFF state
	LENS_STATE_ALL             = 0x1FF,
	ENUM_DUMMY4WORD(LENS_STATE)
} LENS_STATE;

#endif

