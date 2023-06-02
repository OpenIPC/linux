#ifndef _IQ_PROC_H_
#define _IQ_PROC_H_

//=============================================================================
// struct & enum definition
//=============================================================================
#define PROC_MSG_BUFSIZE        256
#define MAX_CMDLINE_LENGTH      256
#define MAX_CMD_ARGUMENTS       64
#define MAX_CMD_LENGTH          32
#define MAX_CMD_TEXT_LENGTH     256

typedef enum {
	IQ_PROC_R_ITEM_NONE,
	IQ_PROC_R_ITEM_BUFFER,
	IQ_PROC_R_ITEM_PARAM,
	IQ_PROC_R_ITEM_UI_PARAM,
	IQ_PROC_R_ITEM_CFG_DATA,
	ENUM_DUMMY4WORD(IQ_PROC_R_ITEM)
} IQ_PROC_R_ITEM;

typedef struct _IQ_PROC_CMD {
	CHAR cmd[MAX_CMD_LENGTH];

	INT32 (*execute)(ISP_MODULE *pdrv, INT32 argc, CHAR **argv);
	CHAR text[MAX_CMD_TEXT_LENGTH];
} IQ_PROC_CMD;

typedef struct _IQ_PROC_MSG_BUF {
	INT8 *buf;
	UINT32 size;
	UINT32 count;
} IQ_PROC_MSG_BUF;

//=============================================================================
// external functions
//=============================================================================
extern INT32 iq_proc_create(void);
extern void iq_proc_remove(void);

#endif

