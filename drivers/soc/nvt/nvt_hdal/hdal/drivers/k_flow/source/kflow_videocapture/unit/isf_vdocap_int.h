/*
    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.

    @file       isf_vdocap_int.h

    @brief      isf_vdocap internal header file

    @version    V1.00.000
    @author     Novatek FW Team
    @date       2018/06/04
*/
#ifndef _ISF_VDOCAP_INT_H
#define _ISF_VDOCAP_INT_H
#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "kwrap/task.h"
#include "kwrap/spinlock.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "kflow_common/nvtmpp.h"
#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_core.h"
#include "kflow_videocapture/isf_vdocap.h"
#include "kflow_videocapture/ctl_sie.h"
#include "kflow_videocapture/ctl_sie_utility.h"
#include "kflow_videocapture/ctl_sen.h"
#include "plat/top.h"
#define debug_msg 			vk_printk

#define DELAY_M_SEC(x)              vos_task_delay_ms(x)
#define DELAY_U_SEC(x)              vos_task_delay_us(x)

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
#include <malloc.h>
#define module_param_named(a, b, c, d)
#define MODULE_PARM_DESC(a, b)
#include <string.h>
#define msecs_to_jiffies(x)	0
#else
#include "comm/nvtmem.h"
#endif

#define ISF_VDOCAP_IN_NUM       1
#define ISF_VDOCAP_OUT_NUM      1
#define ISF_VDOCAP_PATH_NUM    16

#define ISF_VDOCAP_OUTQ_MAX 	4

#define VDOCAP_OUT_DEPTH_MAX    (4+ISF_VDOCAP_OUTQ_MAX)

#if defined(_BSP_NA51000_)
#define SHDR_MAX_FRAME_NUM     4
#else
#define SHDR_MAX_FRAME_NUM     2
#endif

#if defined(_BSP_NA51089_)
#define _DVS_FUNC_
#endif


#define	VDOCAP_DBG_TS_MAXNUM	  30

#define SHDR_QUEUE_DEBUG        DISABLE

#define NA51084_CSI0_SPT_SIE_MAP 0x1B //(VCAP4|VCAP3|VCAP1|VCAP0)
#define NA51084_CSI1_SPT_SIE_MAP 0x1A //(VCAP4|VCAP3|VCAP1)

#define NA51055_CSI0_SPT_SIE_MAP 0x3 //(VCAP1|VCAP0)

typedef enum  {
	VDOCAP_SHDR_SET1        = 0,
	VDOCAP_SHDR_SET2,
	VDOCAP_SHDR_SET_MAX_NUM,
	ENUM_DUMMY4WORD(VDOCAP_SHDR_SET)
} VDOCAP_SHDR_SET;

typedef enum _VDOCAP_ALG_CTRLFUNC {
	VDOCAP_ALG_FUNC_AE   = 0x00000100, ///< enable AE
	VDOCAP_ALG_FUNC_AWB  = 0x00000200, ///< enable AWB
	VDOCAP_ALG_FUNC_AF   = 0x00000400, ///< enable AF
	VDOCAP_ALG_FUNC_WDR  = 0x00000800, ///< enable WDR effect (single frame)
	VDOCAP_ALG_FUNC_SHDR = 0x00001000, ///< enable Sensor HDR effect (multi frame)
	VDOCAP_ALG_FUNC_ETH  = 0x00002000, ///< enable ETH
	ENUM_DUMMY4WORD(VDOCAP_ALG_CTRLFUNC)
} VDOCAP_ALG_CTRLFUNC;

typedef enum VDOCAP_SEN_HDR_MAP {
	VDOCAP_SEN_HDR_NONE        = 0,     ///< for not using sensor HDR
	VDOCAP_SEN_HDR_SET1_MAIN,           ///< indicate this videocapture connect to the main path of sensor HDR set1
	VDOCAP_SEN_HDR_SET1_SUB1,           ///< indicate this videocapture connect to the sub path 1 of sensor HDR set1
	VDOCAP_SEN_HDR_SET1_SUB2,           ///< indicate this videocapture connect to the sub path 2 of sensor HDR set1
	VDOCAP_SEN_HDR_SET1_SUB3,           ///< indicate this videocapture connect to the sub path 3 of sensor HDR set1
	VDOCAP_SEN_HDR_SET2_MAIN,           ///< indicate this videocapture connect to the main path of sensor HDR set2
	VDOCAP_SEN_HDR_SET2_SUB1,           ///< indicate this videocapture connect to the sub path 1 of sensor HDR set2
	ENUM_DUMMY4WORD(VDOCAP_SEN_HDR_MAP)
} VDOCAP_SEN_HDR_MAP;


typedef struct _VDOCAP_OUT_QUEUE {
	UINT32 output_en[ISF_VDOCAP_OUT_NUM];
	UINT32 *output_cur_en;
	UINT32 output_connecttype[ISF_VDOCAP_OUT_NUM];
	UINT32 force_onebuffer[ISF_VDOCAP_OUT_NUM];
	UINT32 force_onesize[ISF_VDOCAP_OUT_NUM];
	UINT32 force_onej[ISF_VDOCAP_OUT_NUM];
//output queue
	ISF_DATA output_data[ISF_VDOCAP_OUT_NUM][VDOCAP_OUT_DEPTH_MAX];
	UINT32 output_used[ISF_VDOCAP_OUT_NUM][VDOCAP_OUT_DEPTH_MAX];
	UINT32 output_max[ISF_VDOCAP_OUT_NUM];
} VDOCAP_OUT_QUEUE;

typedef struct _VDOCAP_PULL_QUEUE {
	ISF_DATA 		data[ISF_VDOCAP_OUT_NUM][ISF_VDOCAP_OUTQ_MAX];
	ISF_DATA_QUEUE	output[ISF_VDOCAP_OUT_NUM];
	UINT32          num[ISF_VDOCAP_OUT_NUM];
} VDOCAP_PULL_QUEUE;

typedef struct _VDOCAP_OUT_DBG {
	//UINT32 dbg_cnt[ISF_VDOCAP_OUT_NUM][VDOCAP_DBG_TS_MAXNUM];
	UINT64 t[ISF_VDOCAP_OUT_NUM][VDOCAP_DBG_TS_MAXNUM];
	UINT32 idx;
} VDOCAP_OUT_DBG;

typedef struct _VDOCAP_CSI_STATUS {
//	CTL_SEN_MIPI_CB cb;
	UINT32 cb_status;
	UINT32 ok_cnt;
	BOOL error;
} VDOCAP_CSI_STATUS;

typedef struct _VDOCAP_COMMON_MEM {
	ISF_DATA  memblk;
	MEM_RANGE unit_buf;
	MEM_RANGE ctl_sie_buf;
	MEM_RANGE ctl_sen_buf;
	MEM_RANGE total_buf;
} VDOCAP_COMMON_MEM;

typedef struct _VDOCAP_CONTEXT {
	SEM_HANDLE ISF_VDOCAP_OUTQ_SEM_ID[ISF_VDOCAP_OUT_NUM];
	SEM_HANDLE ISF_VDOCAP_OUT_SEM_ID[ISF_VDOCAP_OUT_NUM];
	UINT32 id;
	CTL_SIE_EVENT_FP sie_isr_cb;
	CTL_SIE_EVENT_FP buf_io_cb;
	CTL_SIE_EVENT_FP ipp_dir_cb;
	UINT64 vd_count;
	UINT64 vd_count_prev;
	USIZE  sen_dim;
	VDOCAP_CSI_STATUS csi;
	NVTMPP_DDR ddr; //for common buf (output)
	/*-------------------------------*/
	UINT32 sie_hdl;
	CTL_SIE_ID sie_id;
	UINT32 started;
	UINT32 dev_trigger_open; //before open
	UINT32 dev_trigger_close; //before close
	UINT32 dev_ready; //enable after open, disable before close
	VDOCAP_ALG_CTRLFUNC alg_func;
	UINT32 sen_option_en;
//	CTL_SEN_MAP_IF sen_map_if;
	UINT32 sen_timeout_ms;
	UINT32 tge_sync_id;
	VDOCAP_SEN_HDR_MAP shdr_map;
	CTL_SIE_FLOW_TYPE flow_type;
	CTL_SIE_PAG_GEN_INFO pat_gen_info;
	CTL_SIE_CHGSENMODE_INFO chgsenmode_info;
	CTL_SIE_IO_SIZE_INFO io_size;
	CTL_SIE_OUT_DEST out_dest;
	//CTL_SIE_DATAFORMAT data_fmt;
	CTL_SIE_FLIP_TYPE flip;
	UINT32 chn_lofs[CTL_SIE_CH_MAX];
	BOOL raw_compress;
	UINT32 ad_map;
	UINT32 ad_type;
	CTL_SIE_FIELD_SEL ccir_field_sel;
	UINT32 mux_data_index;
	VDOCAP_OUT_QUEUE outq;
	UINT32 out_buf_size[ISF_VDOCAP_OUT_NUM];
	VDOCAP_PULL_QUEUE pullq;
	VDOCAP_OUT_DBG out_dbg;
	BOOL one_buf;
	BOOL builtin_hdr;
	CTL_SEN_DATALANE data_lane;
	struct _ISF_UNIT *p_destunit;
	UINT32 mclksrc_sync;
	UINT32 pdaf_map;
	BOOL count_vd_by_sensor;
	UINT32 enc_rate;
	VDOCAP_AE_PRESET ae_preset;
#if defined(_DVS_FUNC_)
	VDOCAP_DVS_INFO dvs_info;
#endif
} VDOCAP_CONTEXT;

typedef struct _VDOCAP_SHDR_OUT_QUEUE {
	ISF_DATA output_data[VDOCAP_OUT_DEPTH_MAX][SHDR_MAX_FRAME_NUM];
	UINT32 output_used[VDOCAP_OUT_DEPTH_MAX];
	UINT32 push_cnt[VDOCAP_OUT_DEPTH_MAX];
	UINT32 frame_cnt[VDOCAP_OUT_DEPTH_MAX];
	UINT32 addr[VDOCAP_OUT_DEPTH_MAX][SHDR_MAX_FRAME_NUM]; //SHDR_MAIN block addr
	//UINT32 size[VDOCAP_OUT_DEPTH_MAX]; //SHDR whole block size
	UINT32 vdo_frm_addr[VDOCAP_OUT_DEPTH_MAX][SHDR_MAX_FRAME_NUM]; //SHDR_MAIN block addr
	UINT32 force_onebuffer[VDOCAP_OUT_DEPTH_MAX][SHDR_MAX_FRAME_NUM];
	UINT32 force_onesize[VDOCAP_OUT_DEPTH_MAX];
	UINT32 force_onej[VDOCAP_OUT_DEPTH_MAX];
	UINT32 force_drop[VDOCAP_OUT_DEPTH_MAX];
	#if SHDR_QUEUE_DEBUG
	UINT32 new_ok[SHDR_MAX_FRAME_NUM];
	UINT32 do_new[SHDR_MAX_FRAME_NUM];
	UINT32 new_release[SHDR_MAX_FRAME_NUM];
	UINT32 lock[SHDR_MAX_FRAME_NUM];
	UINT32 unlock[SHDR_MAX_FRAME_NUM];
	UINT32 unlock_release[SHDR_MAX_FRAME_NUM];
	UINT32 push[SHDR_MAX_FRAME_NUM];
	UINT32 push_collect[SHDR_MAX_FRAME_NUM];
	UINT32 do_push[SHDR_MAX_FRAME_NUM];
	UINT32 push_release[SHDR_MAX_FRAME_NUM];
	UINT32 push_drop[SHDR_MAX_FRAME_NUM];
	#endif
} VDOCAP_SHDR_OUT_QUEUE;

extern UINT32 _vdocap_max_count;
extern UINT32 _vdocap_active_list;
extern ISF_UNIT *g_vdocap_list[VDOCAP_MAX_NUM]; //list of all videocap device
#define DEV_UNIT(did)	g_vdocap_list[(did)]

extern ISF_RV _isf_vdocap_do_command(UINT32 cmd, UINT32 p0, UINT32 p1, UINT32 p2);

extern UINT32 _vdocap_shdr_frm_num[VDOCAP_SHDR_SET_MAX_NUM];
extern ISF_UNIT *_vdocap_shdr_main_unit[VDOCAP_SHDR_SET_MAX_NUM];
extern VDOCAP_SHDR_OUT_QUEUE _vdocap_shdr_queue[VDOCAP_SHDR_SET_MAX_NUM];
extern UINT32 _vdocap_shdr_oport_releasedata(VDOCAP_SHDR_OUT_QUEUE *p_outq, UINT32 j);
extern BOOL output_rate_update_pause;

extern void isf_vdocap_install_id(void) _SECTION(".kercfg_text");
extern void isf_vdocap_uninstall_id(void) _SECTION(".kercfg_text");

extern SEM_HANDLE _SECTION(".kercfg_data") ISF_VDOCAP_PROC_SEM_ID;

extern ISF_RV _isf_vdocap_bindouput(ISF_UNIT *p_thisunit, UINT32 oport, ISF_UNIT *p_destunit, UINT32 iport);
extern ISF_RV _isf_vdocap_do_setportparam(ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param, UINT32 value);
extern UINT32 _isf_vdocap_do_getportparam(ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param);
extern ISF_RV _isf_vdocap_do_setportstruct(ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param, UINT32 *p_struct, UINT32 size);
extern ISF_RV _isf_vdocap_do_getportstruct(ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param, UINT32 *p_struct, UINT32 size);

extern ISF_RV _isf_vdocap_updateport(ISF_UNIT *p_thisunit, UINT32 oport, ISF_PORT_CMD cmd);
extern INT32 _vdocap_sie_isr_cb(ISF_UNIT *p_thisunit, UINT32 msg, void *p_in, void *p_out);
extern void _vdocap_oport_initqueue(ISF_UNIT *p_thisunit);
extern void _vdocap_oport_block_check(ISF_UNIT *p_thisunit, UINT32 oport);
extern void _isf_vdocap_oport_do_new(ISF_UNIT *p_thisunit, UINT32 oport, UINT32 buf_size, UINT32 ddr, void *p_header_info);
extern void _isf_vdocap_oport_do_push(ISF_UNIT *p_thisunit, UINT32 oport, void *p_header_info);
extern void _isf_vdocap_oport_do_lock(ISF_UNIT *p_thisunit, UINT32 oport, void *p_header_info, UINT32 lock);
extern void _vdocap_oport_set_enable(ISF_UNIT *p_thisunit, UINT32 out_path, ISF_PORT *p_dest, UINT32 en);


extern void _vdocap_shdr_oport_initqueue(VDOCAP_SHDR_SET shdr_set);
extern void _vdocap_shdr_oport_close_check(ISF_UNIT *p_thisunit, UINT32 oport, VDOCAP_SHDR_OUT_QUEUE *p_outq);
extern VDOCAP_SHDR_SET _vdocap_shdr_map_to_set(VDOCAP_SEN_HDR_MAP shdr_map);
extern UINT32 _vdocap_shdr_map_to_seq(VDOCAP_SEN_HDR_MAP shdr_map);
extern BOOL _vdocap_is_shdr_mode(VDOCAP_SEN_HDR_MAP shdr_map);
extern BOOL _vdocap_is_direct_flow(VDOCAP_CONTEXT *p_ctx);
extern BOOL _vdocap_is_shdr_main_path(VDOCAP_SEN_HDR_MAP shdr_map);
extern void _isf_vdocap_direct_unlock_cb(ISF_UNIT *p_thisunit, UINT32 oport, void *p_header_info);
extern void _isf_vdocap_shdr_oport_do_new(ISF_UNIT *p_thisunit, UINT32 oport, UINT32 buf_size, UINT32 ddr, void *p_header_info, VDOCAP_SEN_HDR_MAP shdr_map);
extern void _isf_vdocap_shdr_oport_do_push(ISF_UNIT *p_thisunit, UINT32 oport, void *p_header_info, VDOCAP_SEN_HDR_MAP shdr_map);
extern void _isf_vdocap_shdr_oport_do_lock(ISF_UNIT *p_thisunit, UINT32 oport, void *p_header_info, UINT32 lock, VDOCAP_SEN_HDR_MAP shdr_map);


//pull data
extern void _isf_vdocap_oqueue_do_open(ISF_UNIT *p_thisunit, UINT32 oport);
extern void _isf_vdocap_oqueue_do_start(ISF_UNIT *p_thisunit, UINT32 oport);
extern void _isf_vdocap_oqueue_force_stop(ISF_UNIT *p_thisunit, UINT32 oport);
extern void _isf_vdocap_oqueue_do_stop(ISF_UNIT *p_thisunit, UINT32 oport);
extern void _isf_vdocap_oqueue_do_close(ISF_UNIT *p_thisunit, UINT32 oport);
extern ISF_RV _isf_vdocap_oqueue_do_push(ISF_UNIT *p_thisunit, UINT32 oport, ISF_DATA *p_data);
extern ISF_RV _isf_vdocap_oqueue_do_pull(ISF_UNIT *p_thisunit, UINT32 oport, ISF_DATA *p_data, INT32 wait_ms);

//debug
extern void isf_vdocap_dump_status(int (*dump)(const char *fmt, ...), ISF_UNIT *p_thisunit);
#endif

