#ifndef _AFT_API_H_
#define _AFT_API_H_

#if defined(__KERNEL__) || defined(__FREERTOS)
#include "af_alg.h"
#include "af_ui.h"
#endif

#define AF_CFG_NAME_LENGTH     256
#define AF_DTSI_NAME_LENGTH    256

/**
	AF tuning item
*/
typedef enum _AFT_ITEM {
	AFT_ITEM_VERSION             = 0,///< data_type: [Get]     UINT32
	AFT_ITEM_SIZE_TAB,               ///< data_type: [Get]     AFT_INFO
	AFT_ITEM_RLD_CONFIG,             ///< data_type: [Set]     AFT_CFG_INFO
	AFT_ITEM_RLD_DTSI,
	AFT_ITEM_RESERVE_04,
	AFT_ITEM_RESERVE_05,
	AFT_ITEM_RESERVE_06,
	AFT_ITEM_RESERVE_07,
	AFT_ITEM_RESERVE_08,
	AFT_ITEM_RESERVE_09,
	// NOTE: Start of vendor API usage
	AFT_ITEM_OPERATION,              ///< data_type: [Set/Get] AFT_OPERATION
	AFT_ITEM_RESERVE_11,
	AFT_ITEM_RESERVE_12,
	AFT_ITEM_RESERVE_13,
	AFT_ITEM_RESERVE_14,
	AFT_ITEM_RESERVE_15,
	AFT_ITEM_RESERVE_16,
	AFT_ITEM_RESERVE_17,
	AFT_ITEM_RESERVE_18,
	AFT_ITEM_RESERVE_19,
	AFT_ITEM_RESERVE_20,
	AFT_ITEM_RESERVE_21,
	AFT_ITEM_RESERVE_22,
	AFT_ITEM_RESERVE_23,
	AFT_ITEM_RESERVE_24,
	AFT_ITEM_RESERVE_25,
	AFT_ITEM_RESERVE_26,
	AFT_ITEM_RESERVE_27,
	AFT_ITEM_RESERVE_28,
	AFT_ITEM_RESERVE_29,
	AFT_ITEM_RESERVE_30,
	// NOTE: End of vendor API usage
	AFT_ITEM_ENABLE             = 31,///< data_type: [Set/Get] AFT_ENABLE
	AFT_ITEM_ALG_METHOD,             ///< data_type: [Set/Get] AFT_ALG_METHOD
	AFT_ITEM_SHOT_MODE,              ///< data_type: [Set/Get] AFT_SHOT_MODE
	AFT_ITEM_SEARCH_DIR,             ///< data_type: [Set/Get] AFT_SEARCH_DIR
	AFT_ITEM_SKIP_FRAME,        // 35,///< data_type: [Set/Get] AFT_SKIP_FRAME
	AFT_ITEM_THRES,                  ///< data_type: [Set/Get] AFT_THRES
	AFT_ITEM_STEP_SIZE,              ///< data_type: [Set/Get] AFT_STEP_SIZE
	AFT_ITEM_MAX_COUNT,              ///< data_type: [Set/Get] AFT_MAX_COUNT
	AFT_ITEM_WIN_WEIGHT,             ///< data_type: [Set/Get] AFT_WIN_WEIGHT
	AFT_ITEM_VA_STA,            // 40,///< data_type: [Get] AFT_VA_STA
	AFT_ITEM_RETRIGGER,              ///< data_type: [Set] AFT_RETRIGGER
	AFT_ITEM_DBG_MSG,                ///< data_type: [Set/Get] AFT_DBG_MSG
	AFT_ITEM_EXEC_STATUS,            ///< data_type: [Get] AFT_EXEC_STS
	AFT_ITEM_SEN_RESOLUTION,         ///< data_type: [Get] AFT_SEN_RESO
	AFT_ITEM_NIGHT_MODE,        // 45,///< data_type: [Get] AFT_NIGHT_MODE
	AFT_ITEM_ALG_CMD,                ///< data_type: [Set/Get] AFT_ALG_CMD
	AFT_ITEM_VA_OPT,                 ///< data_type: [Set/Get] AFT_VA_OPT
	AFT_ITEM_RESERVE_48,
	AFT_ITEM_RESERVE_49,
	AFT_ITEM_MAX                = 50,
	ENUM_DUMMY4WORD(AFT_ITEM)
} AFT_ITEM;

typedef struct AFT_INFO {
	UINT32 size_tab[AFT_ITEM_MAX][2];
} AFT_INFO;

typedef struct _AFT_CFG_INFO {
	AF_ID id;
	CHAR path[AF_CFG_NAME_LENGTH];
} AFT_CFG_INFO;

typedef struct _AFT_DTSI_INFO {
	AF_ID id;
	CHAR node_path[AF_DTSI_NAME_LENGTH];
	CHAR file_path[AF_DTSI_NAME_LENGTH];
	UINT8 *buf_addr;
} AFT_DTSI_INFO;

typedef struct _AFT_OPERATION {
	UINT32 id;
	AF_OPERATION operation;
} AFT_OPERATION;

typedef struct _AFT_ENABLE {
	AF_ID id;
	BOOL func_en;
} AFT_ENABLE;

typedef struct _AFT_ALG_METHOD {
	AF_ID id;
	AF_ALG_METHOD alg_method;
} AFT_ALG_METHOD;

typedef struct _AFT_SHOT_MODE {
	AF_ID id;
	AF_SHOT_MODE shot_mode;
} AFT_SHOT_MODE;

typedef struct _AFT_SEARCH_DIR {
	AF_ID id;
	AF_SEARCH_DIR search_dir;
} AFT_SEARCH_DIR;

typedef struct _AFT_SKIP_FRAME {
	AF_ID id;
	UINT32 skip_frame;
} AFT_SKIP_FRAME;

typedef struct _AFT_THRES {
	AF_ID id;
	AF_THRES thres;
} AFT_THRES;

typedef struct _AFT_STEP_SIZE {
	AF_ID id;
	AF_STEP_SIZE step_size;
} AFT_STEP_SIZE;

typedef struct _AFT_MAX_COUNT {
	AF_ID id;
	UINT32 max_count;
} AFT_MAX_COUNT;

typedef struct _AFT_WIN_WEIGHT {
	AF_ID id;
	AF_WIN_WEIGHT win_weight;
} AFT_WIN_WEIGHT;

typedef struct _AFT_VA_STA {
	AF_ID id;
	AF_VA_STA va_sta;
} AFT_VA_STA;

typedef struct _AFT_RETRIGGER {
	AF_ID id;
	BOOL retrigger;
} AFT_RETRIGGER;

typedef struct _AFT_DBG_MSG {
	AF_ID id;
	AF_DBG_MSG dbg_msg;
} AFT_DBG_MSG;

typedef struct _AFT_EXEC_STS {
	AF_ID id;
	AFALG_STATUS_RESULT exec_rslt;
	UINT32 reserve[3];
} AFT_EXEC_STS;

typedef struct _AFT_SEN_RESO {
	AF_ID id;
	UINT32 sen_reso;
} AFT_SEN_RESO;

typedef struct _AFT_NIGHT_MODE {
	AF_ID id;
	BOOL night_mode;
} AFT_NIGHT_MODE;

typedef struct _AFT_ALG_CMD {
	AF_ID id;
	AFALG_CMD cmd;
} AFT_ALG_CMD;

typedef struct _AFT_VA_OPT {
	AF_ID id;
	AF_VA_OPT opt;
} AFT_VA_OPT;

#endif

