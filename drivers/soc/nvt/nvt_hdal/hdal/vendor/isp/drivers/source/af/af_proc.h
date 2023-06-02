#ifndef _AF_PROC_H_
#define _AF_PROC_H_

//=============================================================================
// struct & enum definition
//=============================================================================
#define PROC_MSG_BUFSIZE       256
#define MAX_CMDLINE_LENGTH     256
#define MAX_CMD_ARGUMENTS      64
#define MAX_CMD_LENGTH         32
#define MAX_CMD_TEXT_LENGTH    256

typedef struct _AF_PROC_CMD {
	CHAR cmd[MAX_CMD_LENGTH];
	INT32 (*execute)(ISP_MODULE *pdrv, INT32 argc, CHAR **argv);
	CHAR text[MAX_CMD_TEXT_LENGTH];
} AF_PROC_CMD;

typedef struct _AF_MSG_BUF {
	INT8 *buf;
	UINT32 size;
	UINT32 count;
} AF_MSG_BUF;


//=============================================================================
// external functions
//=============================================================================
extern INT32 af_proc_create(void);
extern void af_proc_remove(void);

#endif

