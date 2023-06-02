#ifndef _ISP_PROC_H_
#define _ISP_PROC_H_

//=============================================================================
// struct definition
//=============================================================================
#define MAX_CMDLINE_LENGTH 256
#define MAX_CMD_ARGUMENTS 64
#define MAX_CMD_LENGTH 32
#define MAX_CMD_TEXT_LENGTH 256

typedef enum _ISP_PROC_R_ITEM {
	ISP_PROC_R_ITEM_NONE,
	ISP_PROC_R_ITEM_PARAM,
	ENUM_DUMMY4WORD(ISP_PROC_R_ITEM)
} ISP_PROC_R_ITEM;

typedef enum _ISP_PROC_R_PARAM {
	ISP_PROC_R_PARAM_CA,
	ISP_PROC_R_PARAM_CA_ACC_CNT,
	ISP_PROC_R_PARAM_LA,
	ISP_PROC_R_PARAM_VA,
	ISP_PROC_R_PARAM_HISTO,
	ENUM_DUMMY4WORD(ISP_PROC_R_PARAM)
} ISP_PROC_R_PARAM;

typedef struct _ISP_PROC_CMD {
	CHAR cmd[MAX_CMD_LENGTH];

	INT32 (*execute)(ISP_DRV_INFO *pdrv, INT32 argc, CHAR **argv);
	CHAR text[MAX_CMD_TEXT_LENGTH];
} ISP_PROC_CMD;

typedef struct _ISP_PROC_MSG_BUF {
	UINT8 *buf;
	UINT32 size;
	UINT32 count;
} ISP_PROC_MSG_BUF;

//=============================================================================
// extern functions
//=============================================================================
extern INT32 isp_proc_init(ISP_DRV_INFO *pdrv_info);
extern void isp_proc_remove(ISP_DRV_INFO *pdrv_info);

#endif
