/**
    vdodisp, Internal function declare

    @file       vdodisp_int.h
    @ingroup    mVDODISP

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#ifndef _VDODISPINT_H
#define _VDODISPINT_H

#define JANICE_TODO    (0)

#include "kflow_common/type_vdo.h"
#include <gximage/gximage.h>
#include "../vdodisp_api.h"
#include "vdodisp_cmd.h"
#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#include "kwrap/task.h"
#include "kwrap/stdio.h"
#include "kwrap/sxcmd.h"
#include "kwrap/util.h"
#define Delay_DelayMs(ms) vos_util_delay_ms(ms)

#define IDE_TASK          DISABLE

#define THIS_DBGLVL         NVT_DBG_WRN
#define __MODULE__          vdodisp
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "kwrap/debug.h"
extern unsigned int vdodisp_debug_level;

/**
*   Internal configuration
*/
//@{
#define CFG_VDODISP_INIT_KEY     MAKEFOURCC('D','I','S','P') ///< a key value 'N','O','V','A' for indicating system initial.
#define BLACK_BUF_SIZE	    8
//@}

#if defined(_BSP_NA51000_)
#define DEVICE_COUNT	2
#else
#define DEVICE_COUNT	1
#endif

/**
    Flag Pattern
*/
//@{
//Task Operation
#define FLGVDODISP_OPEN      FLGPTN_BIT(0)   //not used, currently.
#define FLGVDODISP_CLOSE     FLGPTN_BIT(1)   //notify task don't do any command for task close
#define FLGVDODISP_SUSPEND   FLGPTN_BIT(2)   //notify task don't do any command for task suspend
#define FLGVDODISP_RESUME    FLGPTN_BIT(3)   //notify task start to receive command
#define FLGVDODISP_CMD       FLGPTN_BIT(4)   //notify task there's command coming
#define FLGVDODISP_IDLE      FLGPTN_BIT(5)   //task is idle or just finish his job
#define FLGVDODISP_POLLING   FLGPTN_BIT(6)   //safe polling enter idle
//Task Finish State
#define FLGVDODISP_OPENED    FLGPTN_BIT(7)  //indicate task can start to recevie command
#define FLGVDODISP_STOPPED   FLGPTN_BIT(8)  //indicate task start to skip all command to wait terminate task
#define FLGVDODISP_SUSPENDED FLGPTN_BIT(9)  //indicate task start to skip all command
#define FLGVDODISP_RESUMED   FLGPTN_BIT(10)  //task start to receive command
//@}

/**
*   Task condition
*/
typedef struct _VDODISP_TASK_CONDITION {
	UINT32 			ui_init_key;  ///< indicate module is initail
	BOOL   			b_opened;     ///< indicate task is open
	BOOL   			b_suspended;  ///< indicate task is suppend
	BOOL   			b_stopped;    ///< indicate task is close
	THREAD_HANDLE   task_id;      ///< given a task id form project layer creation (can set 0 to use system default)
	SEM_HANDLE     	*sem_id;       ///< given a semaphore id form project layer creation (can set 0 to use system default)
	ID     			flag_id;      ///< given a flag id form project layer creation (can set 0 to use system default)
	THREAD_HANDLE   ide_id;      ///< given a flag id form project layer creation (can set 0 to use system default)
} VDODISP_TASK_CONDITION;

/**
*   Debug information
*/
typedef struct _VDODISP_DBGINFO {
	VDODISP_ER last_er;                ///< last error code (if any success, this variable will not be clean)
	VDODISP_WR last_wr;                ///< last warning code (if any success, this variable will not be clean)
	VDODISP_CMD_IDX last_cmd_idx;      ///< last command index
	THREAD_HANDLE   last_caller_task;  ///< last command caller task id
	UINT32         last_caller_addr;  ///< last command caller address
} VDODISP_DBGINFO;

/**
    Maxium Data.
*/
typedef union _VDODISP_CMD_MAXDATA {
	union {
		UINT32              ui_reserve;  ///< just reserve. for no any other structure
		VDODISP_DISP_DESC   disp_desc;
		VDO_FRAME           img_buf;
		VDODISP_EVENT_CB    event_cb;
	} MAXDATA;
} VDODISP_CMD_MAXDATA;

/**
    Command controller
*/
typedef struct _VDODISP_CMD_CTRL {
	const VDODISP_CMD_DESC  *p_func_tbl;   ///< command function table
	UINT32                   ui_num_func;  ///< total items in command function table
	VDODISP_CMD              cmd;          ///< received vdodisp_cmd command data
	VDODISP_CMD_MAXDATA      max_in_buf;   ///< duplicated command input data
	VDODISP_ER               er_cmd;        ///< command error code after command finish
} VDODISP_CMD_CTRL;

/**
    Filp controller
*/
typedef struct _VDODISP_FLIP_CTRL {
	BOOL                is_need_update; ///< indicate GxDisplay or DispObj have to update
	VDO_FRAME           *p_img;         ///< current buffer to display
	VDODISP_DISP_DESC   disp_desc;      ///< for ide setting
	UINT32              frame_cnt;      ///< frame count for fps to evaluate
} VDODISP_FLIP_CTRL;

/**
    IDE frame-end controller
*/
typedef struct _VDODISP_IDE_CTRL {
	BOOL                bStop; 			///< indicate vdodisp_ide_0 loop
} VDODISP_IDE_CTRL;

/*
*   vdodisp Manager Control Object
*/
typedef struct _VDODISP_CTRL {
	VDODISP_INIT             init;       ///< duplicated initial data
	VDODISP_TASK_CONDITION   cond;       ///< task condition object
	VDODISP_STATE            state;      ///< task state machine
	VDODISP_DBGINFO          dbg_info;   ///< debug information
	VDODISP_CMD_CTRL         cmd_ctrl;   ///< command controllor data
	VDODISP_FLIP_CTRL        flip_ctrl;  ///< flip control
	VDODISP_EVENT_CB         event_cb;   ///< event callback
	VDODISP_IDE_CTRL         ide;        ///< ide frame end control
	VDODISP_BLACKBUF	     black_buf;
} VDODISP_CTRL;

/**
    vdodisp main task.

    Task body. Just set it into user_systasktabl in SysCfg.c. The stack size
    suggest to 4096KB. If there are many callbacks have plug-in, the stack
    size maybe need to more.
*/
extern THREAD_DECLARE(vdodisp_tsk_0, arglist);
extern THREAD_DECLARE(vdodisp_tsk_1, arglist);
extern THREAD_DECLARE(vdodisp_ide_0, arglist);
extern THREAD_DECLARE(vdodisp_ide_1, arglist);

VDODISP_CTRL    *x_vdodisp_get_ctrl(VDODISP_DEVID device_id);
VDODISP_ER       x_vdodisp_err(VDODISP_DEVID device_id, VDODISP_ER er);
VDODISP_WR       x_vdodisp_wrn(VDODISP_DEVID device_id, VDODISP_WR wr);
VDODISP_ER       x_vdodisp_lock(VDODISP_DEVID device_id);
VDODISP_ER       x_vdodisp_unlock(VDODISP_DEVID device_id);
BOOL             x_vdodisp_chk_idle(VDODISP_DEVID device_id);
VDODISP_ER       x_vdodisp_wait_idle(VDODISP_DEVID device_id);
VDODISP_ER       x_vdodisp_set_idle(VDODISP_DEVID device_id);
VDODISP_ER       x_vdodisp_set_state(VDODISP_DEVID device_id, VDODISP_STATE state);
VDODISP_STATE    x_vdodisp_get_state(VDODISP_DEVID device_id);

#endif
