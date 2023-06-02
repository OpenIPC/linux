/**
    Object Detection and Tracking library.

    @file       odt_lib.h
    @ingroup    mILibODT

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/
#ifndef NVT_ODT_H
#define NVT_ODT_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include "hdal.h"
#include "nvt_dis.h"

#ifndef DISABLE
#define DISABLE 0           ///< Feature is disabled
#endif

#ifndef ENABLE
#define ENABLE 1           ///< Feature is enabled
#endif


#define ODT_NUM_MAX                 64  ///< max obj number in ODT
#define ODT_MAX_DISRSLTBUF_NUM 		30
#define ODT_PROCESS_NUM_MAX         10  ///< max ODT process number

#define ODT_OFFLINE_TEST            DISABLE
#define ODT_TEST					DISABLE

#define ODT_MAX_FEATURE_OUT         1024


/**
     @name ODT return status
*/
//@{
#define ODT_STA_OK                 (0)                   ///<  the status is ok
#define ODT_STA_ERROR             (-1)                   ///<  some error occurred
//@}

typedef enum
{
	// - NvtODT functions -------------------------------------------------------
	NVTODT_FUN_BASE,
	// add new function below
	NVT_ODT_FDCNN,
    NVT_ODT_PDCNN,

	NVTODT_FUN_MAX,
	// - End Detection functions ---------------------------------------------------

	ENUM_DUMMY4WORD(NVTODT_FUN)
} NVTODT_FUN;

/**
    ODT IPC init buffer.
*/
typedef struct {
	UINT32 addr;                             /* IPC buffer addr */
	UINT32 size;                             /* IPC buffer size */
} ODT_IPC_INIT;


typedef struct _odt_rslt_loc
{
	UINT32 timestamp_sec;
	UINT32 timestamp_usec;
}odt_rslt_loc;

typedef struct _odt_od_result
{
    UINT32	class;
    INT32 	score;
    UINT32 	x;
    UINT32 	y;
    UINT32 	w;
    UINT32 	h;
	UINT32  feature_addr;
	BOOL    valid;
}odt_od_result;


typedef struct _odt_obj
{
    HD_URECT	obj_rect;          ///< result start x, start y, width, height
    UINT32		id;                ///< result id
	INT32  		score;             ///< result obj quality reference score
    INT32  		class;             ///< result obj class
    UINT32 	    feature_addr;
    INT32  		type;              ///< result type: 0--the current detected target, other--estimated value of previous target .
    INT32  		life1;
	INT32  		life2;
	BOOL        valid;
    HD_GFX_IMG_BUF *	p_frame;   ///< result obj corresponding frame image
    odt_rslt_loc timestamp;
} odt_obj;

typedef struct _odt_rslt_inf
{
	INT32	flag;
	UINT32 	num;
	INT32 	index_max[ODT_NUM_MAX];
	INT32 	index_shift_x[ODT_NUM_MAX];
	INT32 	index_shift_y[ODT_NUM_MAX];
	odt_obj	odt_obj_inf[ODT_NUM_MAX];
}odt_rslt_inf;


typedef enum
{
	odt_low = 0,
	odt_normal,
	odt_high
} odt_value;

typedef struct
{
	odt_value sensitivity;
	float     sensitivity_w;
	float     sensitivity_b;
	UINT32    ui_num_h;
	UINT32    ui_num_v;
	DIS_BLKSZ blksz;
}odt_track_param;


/**
     Config ID.

*/
typedef enum
{
    odt_cfg_max_obj_life1,       ///< config max obj life1
	odt_cfg_max_obj_life2,       ///< config max obj life2
	odt_cfg_outobj_context,      ///< config output obj context range, can be set to 0~100, e.g. set 15 means output obj box with 1.5 times the original size and context range.
	odt_cfg_scale_for_match,     ///< config scale factor for match
	odt_cfg_sensitivity,         ///< config odt sensitivity
	odt_cfg_target_min_w,
	odt_cfg_target_min_h,
	ENUM_DUMMY4WORD(odt_cfg)
} odt_cfg;


typedef struct _ODT_MOTION_INFOR {
	INT32   ix;                  ///< x component
	INT32   iy;                  ///< y component
	BOOL    bvalid;              ///< reliability of MV
} ODT_MOTION_INFOR;


/**
     DIS results buffer.
*/
typedef struct _odt_dis_rsltbuf
{
	ODT_MOTION_INFOR odt_dis_mvrslt[DIS_MVNUMMAX];
	UINT64           odt_dis_timestamp;
}odt_dis_rsltbuf;


typedef struct _odt_process_buf_param
{
	UINT32	process_id;
	UINT32 	detect_rslt_addr;
	UINT32 	odt_od_rslt_buffer_addr;
	UINT32 	odt_rslt_buffer_addr;
}odt_process_buf_param;

typedef struct _odt_process_param
{
	INT32	        flag;
	UINT32 	        process_id;

	UINT32 	        detect_loc;
	UINT32 	        detect_loc_pre;
	UINT32 	        detect_num;
	UINT32 	        detect_rslt_addr;

	UINT32 	        odt_od_rslt_loc;
	UINT32      	odt_od_rslt_loc_pre;
	UINT32       	odt_od_rslt_buffer_addr;

	UINT32          odt_rslt_loc_after_match;

	UINT32 	        odt_rslt_loc;
	UINT32 	        odt_rslt_buffer_addr;

	UINT32 	        odt_getdisrsltloc;

	UINT32 	        odt_obj_id;
	INT32 	        odt_max_objlife1;
	INT32 	        odt_max_objlife2;
	INT32 	        odt_outobjcontext;
    INT32	        odt_scale_for_match;
	UINT32          odt_target_size_min_w;
	UINT32          odt_target_size_min_h;
	odt_track_param	odt_ot_param;

}odt_process_param;



/**
    Install required system resource.
*/
//extern void odt_installid(void) _SECTION(".kercfg_text");
extern INT32  odt_regist(odt_process_buf_param *p_param, NVTODT_FUN func);
extern INT32  odt_logoff_func(NVTODT_FUN func);
extern INT32  odt_regist_state(NVTODT_FUN func);

/**
     Config some settings of ODT.

     @param[in] config_id: configure ID.
     @param[in] value: setting value.

     Example:
     @code
     {
         odt_config(odt_cfg_max_obj_life1, NVT_ODT_OD);
     }
     @endcode
*/
extern void   odt_config(odt_cfg config_id, INT32 value, NVTODT_FUN func);

/**
     Get some configurations of ODT.

     @param[in] config_id: configure ID.

     @return the setting value.
     @return odt_sta_error:    Some error occurred.

     Example:
     @code
     {
         UINT32 value;
         value  = odt_getconfig(odt_cfg_max_obj_life1, NVT_ODT_OD);
     }
     @endcode
*/
extern UINT32 odt_getconfig(odt_cfg config_id, NVTODT_FUN func);
/**
     Init ODT function.

     This function is used to initialize ODT buffer and set some initial setting.

     @param[in] p_buf: the work buffer for ODT process. the required buffer size can be get by odt_calcbuffsize().
     @param[in] cachebuf: the cache work buffer for ODT process. the required cache buffer size can be get by odt_calccachebuffsize().
     @return
         - @b odt_sta_ok:       Success.
         - @b odt_sta_error:    Some error occurred.

     Example:
     @code
     {
         ODT_IPC_INIT buf;

         buf.addr = pbuf;
         buf.size = odt_calcbuffsize();

         // Init ODT buffer
         odt_init(&buf);
     }
     @endcode
*/
extern INT32  odt_init(ODT_IPC_INIT *p_buf);

/**
     Reset ODT function.

     The API will reset the ODT result number and params.
     @return
         - @b odt_sta_ok:       Success.
         - @b odt_sta_error:    Some error occurred.
*/
extern INT32  odt_reset(void);


/**
     UnInit ODT function.

     The API will reset the ODT result number and release buffer.
     @return
         - @b odt_sta_ok:       Success.
         - @b odt_sta_error:    Some error occurred.
*/
extern INT32  odt_uninit(void);


/**
     Process the ODT function.

     This API include obj detection and obj tracking

     Obj detection will scale the input image to temp buffer by image ratio.\n
     If Novatek OD lib return TRUE, then we treat this candidate obj as a real obj, and give it a id number.\n

     Obj tracking will tracking the reference real obj in present frame use DIS process.\n
     If DIS process return TRUE, then we treat this obj and the reference real obj having same id number.\n

*/
extern void   odt_od_process(void);
extern BOOL   odt_ot_process(void);

/**
     Calculate ODT required buffer size.

     The working buffer is used for obj detection and tracking.

     @return the required buffer size.
*/
extern UINT32 odt_calcbuffsize(void);

/**
     Lock/Unlock ODT.

     @param[in] bLock: lock or unlock.

     Example:
     @code
     {
         // lock the ODT
         odt_lock(NVT_ODT_OD, TRUE);
         // doing something such as capture
         ...
         // end of capture
         // unlock the ODT
         odt_lock(NVT_ODT_OD, FALSE);
     }
     @endcode
*/
extern INT32  odt_lock(NVTODT_FUN func, BOOL block);

/**
     Get the result objs of ODT.

     This API will return the result objs by different coordinate.\n

     @param[out] p_obj: the result obj info.
     @param[in]  process_id: the detection process id in ODT.
     @param[in]  p_targetCoord: the target window coordinate.

     @return The number of objs in present frame.

     Example:
     @code
     {
        odt_obj   odt_info[ODT_NUM_MAX];
        HD_URECT  odt_disp_coord={0,0,640,480};
        UINT32    obj_number;

        obj_number = odt_getrsltobj_for_draw(&odt_info, 1, &odt_disp_coord);
     }
     @endcode
*/
extern UINT32 odt_getrsltobj_for_draw(odt_obj *p_obj, UINT32 process_id, HD_URECT* p_targetcoord);

/**
     Get the after match result objs of ODT.

     This API will return the result objs by different coordinate.\n

     @param[out] p_obj: the result obj info.
     @param[in]  func: the detection func in ODT.
     @param[in]  p_targetCoord: the target window coordinate.
     @param[out] p_loc: the target location.

     @return The number of objs in present frame.

     Example:
     @code
     {
        odt_obj   odt_info[ODT_NUM_MAX];
        HD_URECT  odt_disp_coord={0,0,640,480};
        UINT32    obj_number;
        UINT32    loc;

        obj_number = odt_getrsltobj_after_match(&odt_info, NVT_ODT_OD, &odt_disp_coord, &loc);
     }
     @endcode
*/
extern UINT32 odt_getrsltobj_after_match(odt_obj *p_obj, NVTODT_FUN func, HD_URECT* p_targetcoord, UINT32 *p_loc);

/**
     Get the result objs of ODT.

     This API will return the result objs by different coordinate.\n

     @param[out] p_obj: the result obj info.
     @param[in]  func: the detection func in ODT.
     @param[in]  p_targetCoord: the target window coordinate.
     @param[out] p_loc: the target location.

     @return The number of objs in present frame.

     Example:
     @code
     {
        odt_obj   odt_info[ODT_NUM_MAX];
        HD_URECT  odt_disp_coord={0,0,640,480};
        UINT32    obj_number;
        UINT32    loc;

        obj_number = odt_getrsltobj_after_match(&odt_info, NVT_ODT_FDCNN, &odt_disp_coord, &loc);
     }
     @endcode
*/
extern UINT32 odt_getrsltobj_for_func(odt_obj *p_obj, NVTODT_FUN func, HD_URECT* p_targetcoord, UINT32 *p_loc);


/**
     Map obj IDs to continuous IDs.

     @param[in] p_obj: ODT obj detection results.
     @param[in] objnumber: ODT obj number.

     @return void.

     Example:
     @code
     {
        odt_obj   odt_info[ODT_NUM_MAX];
        HD_URECT  odt_disp_coord={0,0,640,480};
        UINT32    obj_number;

        obj_number = odt_getrsltobj(&odt_info, 1, &odt_disp_coord);
		odt_idmap(odt_info, obj_number);
     }
     @endcode

*/
extern void   odt_idmap(odt_obj* p_obj, UINT32 objnumber);

/**
     Get the number of buffer which used to save newest dis result.
*/
extern UINT32 odt_getsavedisrsltloc(void);

/**
     Get the number of buffer which used to save newest dis result and timestamp.
*/
extern UINT32 odt_getsavedisrsltloc_timestamp(UINT32 rslt_loc);

/**
     Set the newest detector result to ODT, rslt_loc for location in ODT.
*/
extern INT32  odt_setodrslt(UINT32 num, odt_od_result *p_rslt, UINT32 rslt_loc, NVTODT_FUN func);

/**
     Set the newest detector result to ODT, rslt_loc for timestamp addr.
*/
extern INT32  odt_setodrslt_timestamp(UINT32 num, odt_od_result *p_rslt, UINT32 rslt_loc, NVTODT_FUN func);

/**
     Save one DIS motion vecter result.
*/
extern void   odt_savedisrslt(void);


/**
     Get DIS init state.
*/
extern BOOL   odt_getinitstate(void);

/**
     Process DIS motion vecter offline when ODT_OFFLINE_TEST = ENABLE.
*/
extern INT32  odt_process_offline(UINT32 cur_egmap_addr, UINT32 ref_egmap_addr);


#endif
