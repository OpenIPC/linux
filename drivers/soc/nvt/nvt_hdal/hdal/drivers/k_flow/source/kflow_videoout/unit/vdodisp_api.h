/**
    vdodisp, APIs declare.

    @file       vdodisp_api.h
    @ingroup    mVDODISP
    @note       --

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#ifndef _VDODISPAPI_H
#define _VDODISPAPI_H
#include "kflow_common/type_vdo.h"

#define VDODISP_API_VERSION 0x19100801U

/**
   Error Code.

   Any error code occured will display on uart or return.
*/
typedef enum _VDODISP_ER {
	VDODISP_ER_OK                =   0, ///< no error
	VDODISP_ER_NOT_INIT          =  -1, ///< vdodisp_init() is not called or not success
	VDODISP_ER_SYS               =  -2, ///< unknown system error
	VDODISP_ER_LOCK              =  -3, ///< failed to lock API
	VDODISP_ER_UNLOCK            =  -4, ///< failed to unlock API
	VDODISP_ER_PARAM             =  -5, ///< wrong Parameters
	VDODISP_ER_API_VERSION       =  -6, ///< wrong API version
	VDODISP_ER_STA_TASK          =  -7, ///< failed to call sta_tsk
	VDODISP_ER_WAIT_IDLE         =  -8, ///< failed to call wai_flg for idle
	VDODISP_ER_SET_IDLE          =  -9, ///< failed to call set_flg to idle
	VDODISP_ER_NOT_IN_IDLE       = -10, ///< if prop.b_enter_only_idle is true, but system is not idle
	VDODISP_ER_STATE             = -11, ///< service state machine is not correctly.
	VDODISP_ER_INVALID_CALL      = -12, ///< invald to call this API
	VDODISP_ER_OUT_DATA_VOLATILE = -13, ///< the parameter VDODISP_CMD.out maybe volatile. (suggest to b_exit_cmd_finish=TRUE)
	VDODISP_ER_CMD_MAXDATA       = -14, ///< internal error. all cmd data structure have to declare in VDODISP_CMD_MAXDATA
	VDODISP_ER_INVALID_CMD_IDX   = -15, ///< invalid command index
	VDODISP_ER_CMD_MAP_NULL      = -16, ///< internal error. cmd function map to a null pointer
	VDODISP_ER_CMD_NOT_MATCH     = -17, ///< internal error. g_vdodispCallTbl is not match to VDODISP_CMD_IDX
	VDODISP_ER_CMD_IN_DATA       = -18, ///< wrong VDODISP_CMD.in Data
	VDODISP_ER_CMD_OUT_DATA      = -19, ///< wrong VDODISP_CMD.out Data
	VDODISP_ER_NOMEM      	     = -20, ///< no buffer.
	VDODISP_ER_WAIT_STOPED       = -21, ///< failed to call wai_flg for stoped
	ENUM_DUMMY4WORD(VDODISP_ER)
} VDODISP_ER;

/**
    Warning Code.

    The warning codes is only displayed via uart.
*/
typedef enum _VDODISP_WR {
	VDODISP_WR_OK                =   0, ///< no warning
	VDODISP_WR_INIT_TWICE        =  -1, ///< vdodisp_init() is called twice
	VDODISP_WR_ALREADY_CLOSED    =  -2, ///< service is already closed.
	VDODISP_WR_SUSPEND_TWICE     =  -3, ///< vdodisp_suspend maybe is called twice
	VDODISP_WR_NOT_IN_SUSPEND    =  -4, ///< call vdodisp_resume, but service isn't in suspend state before.
	VDODISP_WR_CMD_SKIP          =  -5, ///< existing a command is skipped
	ENUM_DUMMY4WORD(VDODISP_WR)
} VDODISP_WR;

/**
    State Machine.

    Indicate the task state.
*/
typedef enum _VDODISP_STATE {
	VDODISP_STATE_UNKNOWN    = 0,    ///< unknown. only existing service is not opened.
	VDODISP_STATE_OPEN_BEGIN,        ///< start to open a task
	VDODISP_STATE_OPEN_END,          ///< finished to do open flow
	VDODISP_STATE_CLOSE_BEGIN,       ///< start to close
	VDODISP_STATE_CLOSE_END,         ///< task is terminated.
	VDODISP_STATE_SUSPEND_BEGIN,     ///< start to enter suspend state
	VDODISP_STATE_SUSPEND_END,       ///< task is in suspend state
	VDODISP_STATE_RESUME_BEGIN,      ///< start to resume service
	VDODISP_STATE_RESUME_END,        ///< finished to resume, task start to recevice command
	VDODISP_STATE_CMD_BEGIN,         ///< start to do a command
	VDODISP_STATE_CMD_END,           ///< command finished
	VDODISP_STATE_CMD_CB_BEGIN,      ///< start to call a callback when command finished
	VDODISP_STATE_CMD_CB_END,        ///< exit to callback when command finished
	VDODISP_STATE_STATUS_CB_BEGIN,   ///< start to call a callback when status change
	VDODISP_STATE_STATUS_CB_END,     ///< exit to callback when status change
	VDODISP_STATE_ER_CB_BEGIN,       ///< start to call a callback when error occured
	VDODISP_STATE_ER_CB_END,         ///< exit to callback when error occured
	VDODISP_STATE_MAX_NUM,           ///< total state numbers
	ENUM_DUMMY4WORD(VDODISP_STATE)
} VDODISP_STATE;

/**
    Command Index.

    The command of vdodisp service support. Set this index to the VDODISP_CMD
    and call the vdodisp_cmd to do command when service is opened.
*/
typedef enum _VDODISP_CMD_IDX {
	VDODISP_CMD_IDX_UNKNOWN  = 0,    ///< unknown command. maybe forget to set VDODISP_CMD.Idx1
	VDODISP_CMD_IDX_SET_DISP,        ///< set display config (In: VDODISP_DISP_DESC, Out: NULL)
	VDODISP_CMD_IDX_GET_DISP,        ///< get display config (In: NULL, Out: VDODISP_DISP_DESC)
	VDODISP_CMD_IDX_SET_FLIP,        ///< flip a frame (In:VDO_FRAME*, Out: NULL)
	VDODISP_CMD_IDX_DUMP_FLIP,       ///< dump current displayed frame to A:\\disp_xxx_xxx.raw (In:NULL, Out: NULL)
	VDODISP_CMD_IDX_RELEASE,         ///< to release locked frame by using temp buffer to freeze a last frame or by setting ide disable (In:VDO_FRAME*, Out: NULL)
	VDODISP_CMD_IDX_SET_EVENT_CB,    ///< set event callback (In:VDODISP_EVENT_CB, Out: NULL)
	VDODISP_CMD_IDX_GET_EVENT_CB,    ///< set event callback (In:NULL, Out: VDODISP_EVENT_CB)
	VDODISP_CMD_IDX_MAX_NUM,         ///< total command numbers
	ENUM_DUMMY4WORD(VDODISP_CMD_IDX)
} VDODISP_CMD_IDX;


/**
     Rotation type

     for VDODISP_DISP_DESC
*/
typedef enum _VDODISP_ROTATE {
	VDODISP_ROTATE_0 = 0,       ///< No rotation
	VDODISP_ROTATE_180,         ///< rotate 180 degree
	VDODISP_ROTATE_HORZ,        ///< horzional flip
	VDODISP_ROTATE_VERT,        ///< vertical flip
	VDODISP_ROTATE_NO_HANDLE,   ///< user handle ide(n) output dir. dispsrv will not set output dir to ide(n) in cases of 0,180,horz,vert.
	VDODISP_ROTATE_MAX_NUM,     ///< Number of rotation item
	ENUM_DUMMY4WORD(VDODISP_ROTATE)
} VDODISP_ROTATE;

/**
     Device ID, mapping to ide[0], ide[1]
*/
typedef enum _VDODISP_DEVID {
	VDODISP_DEVID_0 = 0,       ///< ide[0]
#if defined(_BSP_NA51000_)
	VDODISP_DEVID_1 = 1,       ///< ide[1]
#endif
	VDODISP_DEVID_MAX_NUM,     ///< Number of device item
	ENUM_DUMMY4WORD(VDODISP_DEVID)
} VDODISP_DEVID;


typedef void (*VDODISP_FP_STATE_CB)(UINT32 device_id, VDODISP_STATE state); ///< State callback. If there's any state changed, the service will callback out.
typedef void (*VDODISP_FP_ERROR_CB)(UINT32 device_id, VDODISP_ER er); ///< Error callback. If there's any error occured, the service will callback out.
/**
    Initial Data.

    For system inital via vdodisp_init().
*/
typedef struct _VDODISP_INIT {
	UINT32               ui_api_ver;   ///< just assign to VDODISP_API_VERSION
	VDODISP_FP_STATE_CB  fp_state_cb;  ///< (Optional) system will call the callback when state change
	VDODISP_FP_ERROR_CB  fp_error_cb;  ///< (Optional) if there is error occured, callback will be performed for debug
} VDODISP_INIT, *PVDODISP_INIT;

/**
    black buffer for release.

    For system inital via vdodisp_init().
*/
typedef struct _VDODISP_BLACKBUF {
	UINT8 				 *m_y;   ///<  Y buffer for release clear screen black,alloc in init
	UINT8 				 *m_uv;  ///<  UV buffer for release clear screen black,alloc in init
} VDODISP_BLACKBUF, *PVDODISP_BLACKBUF;

/**
    Finish information.

    When VDODISP_PROP.fp_finish_cb assign a callback, the data will send to this
    callback after command finish.
*/
typedef struct _VDODISP_FINISH {
	VDODISP_CMD_IDX  idx;        ///< Finished command index
	VDODISP_ER       er;         ///< command finish result.
	VOID            *p_user_data;  ///< If VDODISP_PROP.p_user_data is assigned a userdata pointer. service will take it to VDODISP_PROP.fp_finish_cb
} VDODISP_FINISH, *PVDODISP_FINISH;

typedef void (*VDODISP_CB_FINISH)(const VDODISP_FINISH *p_info); ///< Command finish callback. When any command is finished, the service will callback out.
/**
    Command Property.

    Indicate how to run the current command.
*/
typedef struct _VDODISP_PROP {
	BOOL                b_enter_only_idle; ///< do job only system idle. if system is busy, the VDODISP_ER_NOT_IN_IDLE returned.
	BOOL                b_exit_cmd_finish; ///< exit API when job finished.
	VDODISP_CB_FINISH   fp_finish_cb;     ///< (Optional) system will call the callback when job is finished
	VOID               *p_user_data;      ///< (Optional) when CbFinish is called, assigned userdata can get from CbFinish parameter.
} VDODISP_PROP, *PVDODISP_PROP;

/**
    Command Input/Output Data.

    Including data pointer and data size depend on VDODISP_CMD_IDX.
*/
typedef struct _VDODISP_DATA {
	VOID   *p_data;      ///< input/output data pointer
	UINT32  ui_num_byte;  ///< input/output data size
} VDODISP_DATA, *PVDODISP_DATA;

/**
    Command Description.

    The necessary information for running a command via vdodisp_cmd().
*/
typedef struct _VDODISP_CMD {
	VDODISP_DEVID    device_id; ///< device id
	VDODISP_CMD_IDX  idx;    ///< index
	VDODISP_PROP     prop;   ///< property
	VDODISP_DATA     in;     ///< input data description (depend on idx)
	VDODISP_DATA     out;    ///< output data description (depend on idx)
} VDODISP_CMD, *PVDODISP_CMD;

/**
    Display description

    Input data to config display description
*/
typedef struct _VDODISP_DISP_DESC {
	VDODISP_ROTATE          degree;      ///< rotation degree (no 90 or 270 degree, to do so needs image unit.
	URECT                   wnd;         ///< display window, normally is same as buffer
	USIZE                   aspect_ratio;///< display aspect ratio, (eg. disp buffer is 960x240, but the aspect ratio is 4:3
	USIZE                   device_size; ///< device size to meet HSync and VSync for calculating best buffer size.(the value normally are 960x480,960x240,640x480,640x240 for LCD)
} VDODISP_DISP_DESC, *PVDODISP_DISP_DESC;


/**
    Event callback function table
*/
typedef void (*VDODISP_FP_STABLE_CB)(UINT32 device_id); ///< notify first frame is finished after changing setting.
typedef void (*VDODISP_FP_RELEASE_CB)(UINT32 device_id, VDO_FRAME *img_buf); ///< notify this frame is released.
typedef void (*VDODISP_FP_FRM_END_CB)(UINT32 device_id); ///< notify each ide frame end is finished.

typedef struct _VDODISP_EVENT_CB {
	VDODISP_FP_STABLE_CB fp_stable;
	VDODISP_FP_RELEASE_CB fp_release;
	VDODISP_FP_FRM_END_CB fp_frm_end;
} VDODISP_EVENT_CB, *PVDODISP_EVENT_CB;


/**
     Install vdodisp task,flag and semaphore id

     This is default install API. User can create own task id, semphore id,
     flag id to adjust own stack size and task priority as following:

     @code
     {
        #define PRI_VDODISP             20
        #define STKSIZE_VDODISP         2048

        UINT32 VDODISP_TSK_ID = 0;
        UINT32 VDODISP_FLG_ID = 0;
        UINT32 VDODISP_SEM_ID = 0;

        void vdodisp_install_id(void)
        {
            OS_CONFIG_TASK(VDODISP_TSK_ID,PRI_VDODISP,STKSIZE_VDODISP,vdodisp_tsk);
            OS_CONFIG_FLAG(VDODISP_FLG_ID);
            OS_CONFIG_SEMPHORE(VDODISP_SEM_ID,0,1,1);
        }
     }
     @endcode
*/
extern ER vdodisp_install_id(VDODISP_DEVID id);

/**
     Uninstall vdodisp task flag
*/
extern ER vdodisp_uninstall_id(VDODISP_DEVID id);

/**
    Initialize vdodisp service.

    For using the vdodisp service, the task id, flag id, semphore id have to
    prepare by project layer. Assign these ids to the VDODISP_INIT strcuture
    when system starup (just set once).

    @param[in] p_Init necessary inital data.
    @return
        - @b VDODISP_ER_OK: inital success.
        - @b VDODISP_ER_API_VERSION: vdodisp_api.h and Appvdodisp.a not match.
*/
extern VDODISP_ER vdodisp_init(VDODISP_DEVID id,const VDODISP_INIT *p_Init);

/**
    Start to open the vdodisp service.

    After vdodisp service is initialized, call vdodisp_open to start the
    service.

    @return
        - @b VDODISP_ER_OK: open success.
        - @b VDODISP_ER_STATE: may vdodisp_init does not be called yet.
        - @b VDODISP_ER_STA_TASK: failed to start task(sta_tsk failed).
*/
extern VDODISP_ER vdodisp_open(VDODISP_DEVID id);

/**
    Stop to open the vdodisp service.

    When vdodisp service is running, call vdodisp_close to terminate the task
    service.

    @note: Don't call vdodisp_close in any vdodisp service callback. In
    callback, the vdodisp_async_close is allowed to call to stop recevice receive
    data but task is not terminated. The vdodisp_close need to be called outside.

    @return
        - @b VDODISP_ER_OK: close success.
        - @b VDODISP_ER_INVALID_CALL: call vdodisp_close in callback.
        - @b VDODISP_ER_STA_TASK: failed to start task(sta_tsk failed).
*/
extern VDODISP_ER vdodisp_close(VDODISP_DEVID id);

/**
    Suspend the service task.

    When vdodisp service is running, call vdodisp_suspend to stop recevice
    command. Any command will be skiped by service task. The function exit after
    task finish his suspend.

    @return
        - @b VDODISP_ER_OK: suspend success.
        - @b VDODISP_ER_STATE: may vdodisp_open does not be opened yet.
*/
extern VDODISP_ER vdodisp_suspend(VDODISP_DEVID id);

/**
    Resume the service task.

    When vdodisp service is suspended, call vdodisp_resume to start to
    recevice command. The function exit after task finish his resume.

    @return
        - @b VDODISP_ER_OK: suspend success.
        - @b VDODISP_ER_STATE: may vdodisp_open does not be opened yet.
*/
extern VDODISP_ER vdodisp_resume(VDODISP_DEVID id);

/**
    Set a command action to vdodisp service.

    Configure VDODISP_CMD and send it to the service to do some action via
    vdodisp_cmd.

    @param[in] p_cmd command description.
    @return
        - @b VDODISP_ER_OK: send command success.
        - @b VDODISP_ER_CMD_NOT_MATCH: internal error. Please contact to module
             maintenance staff.
        - @b VDODISP_ER_CMD_IN_DATA: the VDODISP_CMD.in maybe assing wrong object
             / structure
        - @b VDODISP_ER_CMD_IN_DATA: the VDODISP_CMD.out maybe assing wrong object
             / structure
        - @b VDODISP_ER_STATE: may vdodisp_open does not be opened yet.
        - @b VDODISP_ER_NOT_IN_IDLE: If VDODISP_CMD.VDODISP_PROP.b_enter_only_idle set
             to TRUE and service is not in idle. Notice you this command is
             skipped.
        - @b VDODISP_ER_OUT_DATA_VOLATILE: If the command include the output data
             and set VDODISP_CMD.VDODISP_PROP.b_exit_cmd_finish to FALSE, the finish
             callback have to plug-in a callback function to VDODISP_CMD
             .VDODISP_PROP.fp_finish_cb and VDODISP_CMD.out have to point to a
             global data structure.
        - @b VDODISP_ER_CMD_MAXDATA: internal error. Please contact to module
             maintenance staff.
*/
extern VDODISP_ER vdodisp_cmd(const VDODISP_CMD *p_cmd);

/**
    Suspend task service, but exit this function without finish suspend.

    This function is for advance. If you don't want to wait vdodisp_suspend()
    finish too long and you want to pipe some flow, you can do the flow that
    vdodisp_async_suspend() -> Do Something -> vdodisp_suspend();

    @note: vdodisp_suspend() is also called necessary to confirm the suspend is
           finished.

    @return
        - @b VDODISP_ER_OK: suspend success.
        - @b VDODISP_ER_STATE: may vdodisp_open does not be opened yet.
*/
extern VDODISP_ER vdodisp_async_suspend(VOID);

/**
    Stop task service, but exit this function without finish close.

    This function is for advance. If you don't want to wait vdodisp_close()
    finish too long and you want to pipe some flow, you can do the flow that
    vdodisp_async_close() -> Do Something -> vdodisp_close();

    @note: vdodisp_async_close() is also called necessary to confirm the suspend is
           finished.

    @return
        - @b VDODISP_ER_OK: suspend success.
        - @b VDODISP_ER_STATE: may vdodisp_open does not be opened yet.
*/
extern VDODISP_ER vdodisp_async_close(VOID);

/**
    Dump vdodisp application library information.

    The information includes version, build date, last error code, last warning
    code, last command, last state and check internal data.

    @return
        - @b VDODISP_ER_OK: suspend success.
*/
extern VOID vdodisp_dump_info(int (*dump)(const char *fmt, ...));
#endif

