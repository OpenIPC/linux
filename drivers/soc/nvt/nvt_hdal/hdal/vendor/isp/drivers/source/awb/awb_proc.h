#ifndef _AWB_PROC_H_
#define _AWB_PROC_H_

//=============================================================================
// struct & enum definition
//=============================================================================
#define PROC_MSG_BUFSIZE        256
#define MAX_CMDLINE_LENGTH      256
#define MAX_CMD_ARGUMENTS       64
#define MAX_CMD_LENGTH          32
#define MAX_CMD_TEXT_LENGTH     256

typedef struct _AWB_PROC_CMD {
	CHAR cmd[MAX_CMD_LENGTH];

	INT32 (*execute)(ISP_MODULE *pdrv, INT32 argc, CHAR **argv);
	CHAR text[MAX_CMD_TEXT_LENGTH];
} AWB_PROC_CMD;

typedef struct _AWB_MSG_BUF {
	INT8 *buf;
	UINT32 size;
	UINT32 count;
} AWB_MSG_BUF;

typedef struct _AWBALG_GAIN {
	UINT32 r_gain;
	UINT32 g_gain;
	UINT32 b_gain;
} AWBALG_GAIN;


//=============================================================================
// external functions
//=============================================================================
extern INT32 awb_proc_create(void);
extern void awb_proc_remove(void);

#endif

