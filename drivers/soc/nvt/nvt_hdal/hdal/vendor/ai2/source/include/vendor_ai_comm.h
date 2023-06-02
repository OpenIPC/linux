/**
	@brief Header file of definition of vendor user-space net flow sample.

	@file net_flow_user_sample.h

	@ingroup net_flow_user_sample

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _VENDOR_AI_COMM_H_
#define _VENDOR_AI_COMM_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include "hd_type.h"
#include "kflow_ai_net/kflow_ai_net.h"
#include "kflow_ai_net/nn_parm.h"
#include "vendor_ai.h"
#include "vendor_ai_plugin.h"

#include "kdrv_ai.h" //for NEW_AI_FLOW

/********************************************************************
	MACRO CONSTANT DEFINITIONS
********************************************************************/
#define NN_SUPPORT_NET_MAX		128

/********************************************************************
	TYPE DEFINITION
********************************************************************/
/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions												   */
/*-----------------------------------------------------------------------------*/
#define DEFAULT_PROC_CNT   4

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations 												   */
/*-----------------------------------------------------------------------------*/

#if CNN_AI_FASTBOOT
#define PD_MAX_OUTNUM                  100         // refer to pdcnn_postproc.h
#define FBOOT_RSLT_NUM                 30
#define VENDOR_AI_FBOOT_RSLT           0Xafb00000  // detection result
#define VENDOR_AI_FBOOT_PROC_PERIOD    0Xafb00001  // proc trigger period

typedef struct _PD_RESULT {
	INT32 category;
	FLOAT score;
	FLOAT x1;
	FLOAT y1;
	FLOAT x2;
	FLOAT y2;
} PD_RESULT;

typedef struct _PD_RESULT_INFO {
	UINT32 sign;          ///< signature = MAKEFOURCC('R','T','P','D')    pdcnn
	UINT32 chunk_size;    ///< sizeof(all buffer) - 8;
	UINT32 result_num;
	PD_RESULT *p_result;
} PD_RESULT_INFO;
#endif

#if CNN_MULTI_INPUT
typedef struct _SRC_LARER_INFO {
	INT32  cnt;           ///< counter for set multiple input
	UINT32 in_buf_num;    ///< number of input buffers (if larger than 1 means multiple input)
} SRC_LARER_INFO;
#endif

typedef struct _VENDOR_AI_NET_PRIV {
	VENDOR_AIS_FLOW_MEM_PARM     init_buf;
	VENDOR_AIS_FLOW_MEM_PARM     work_buf; // whole work buf, including rslt_buf and io_buf
	VENDOR_AIS_FLOW_MEM_PARM     rslt_buf; // rslt_buf
	VENDOR_AIS_FLOW_MEM_PARM     io_buf;   // io_buf
	VENDOR_AIS_FLOW_MEM_PARM     group_buf;
	VENDOR_AI_PROC_STATE         status;
#if CNN_MULTI_INPUT
	SRC_LARER_INFO               src_layer;
#endif
	INT                          io_buf_alloc;
	INT                          b_simplify_bin;
	INT                          fastboot_dump;
	INT                          b_is_used;
} VENDOR_AI_NET_PRIV;

typedef struct _VENDOR_AI_DIFF_MODEL_RESINFO {
	UINT32 curr_id;							///< current model id
	HD_DIM curr_dim;						///< current resolution
	UINT32 new_id;							///< model id to be updated
	HD_DIM new_dim;							///< resolution to be updated
	VENDOR_AI_NET_CFG_MODEL diff_model;		///< different model bin
} VENDOR_AI_DIFF_MODEL_RESINFO;

typedef struct _VENDOR_AI_COMMON_INFO {
	VENDOR_AI_PROC_SCHD           schd;
	VENDOR_AI_ENGINE_PLUGIN	  	  engine_plug[4];
	UINT32						  chk_interval;
} VENDOR_AI_COMMON_INFO;

typedef struct _VENDOR_AI_NET_INFO_PROC {
	VENDOR_AI_NET_INFO            info;
	VENDOR_AI_NET_CFG_MODEL 	  cfg_model;       // user_parm  only
	VENDOR_AI_NET_CFG_MODEL 	  cfg_share_model; // user_model only
	VENDOR_AI_BUF				  input_img;
	VENDOR_AI_BUF				  input2_img;
    VENDOR_AI_BUF				  input3_img;
	VENDOR_AI_BUF				  output_img;
	VENDOR_AI_BUF				  output2_img;
    VENDOR_AI_BUF				  output3_img;
	VENDOR_AIS_FLOW_MAP_MEM_PARM  mem_manager;
	VENDOR_AI_NET_CFG_JOB_OPT	  job_opt;
	VENDOR_AI_NET_CFG_BUF_OPT	  buf_opt;
	VENDOR_AI_DIFF_MODEL_RESINFO  diff_resinfo;
	VENDOR_AI_NET_CFG_WORKBUF 	  workbuf;         // record whole working buffer, which is allocated by user(VENDOR_AI_NET_PARAM_CFG_WORKBUF) or hdal(_vendor_ai_net_work_buf_alloc).
	VENDOR_AI_NET_CFG_WORKBUF 	  rsltbuf;         // result buffer
	VENDOR_AI_NET_CFG_WORKBUF 	  iobuf;           // io buffer
	VENDOR_AI_ENGINE_PLUGIN	  	  user_postproc;
	
	//private data
	VENDOR_AI_NET_PRIV		 priv;
} VENDOR_AI_NET_INFO_PROC;

typedef struct _VENDOR_AI_INFO {
	VENDOR_AI_NET_INFO_PROC  *proc;
} VENDOR_AI_INFO;


typedef enum {
	GET_PORT_TYPE_IN,
	GET_PORT_TYPE_OUT
} GET_PORT_TYPE;

#if defined(_BSP_NA51068_)
typedef struct _VENDOR_AI_NET_MEM_INFO {
	UINT32 start_addr;
	UINT32 size;
	UINT32 offset;
	UINT32 init_done;
} VENDOR_AI_NET_MEM_INFO;
#endif

/*-----------------------------------------------------------------------------*/
/* Local Macros Declarations												   */
/*-----------------------------------------------------------------------------*/
/*
#define DBG_ERR(fmtstr, args...) printf("\033[0;31mERR:%s() \033[0m" fmtstr, __func__, ##args)
#define DBG_WRN(fmtstr, args...) printf("\033[0;33mWRN:%s() \033[0m" fmtstr, __func__, ##args)
#define DBG_DUMP(fmtstr, args...) printf(fmtstr, ##args)
*/
	
#define VENDOR_AI_GET_LAYER(id)                   ((id) & 0x0000ffff)
#define VENDOR_AI_GET_IN(id)                      (((id) & 0x0fff0000) >> 16)
#define VENDOR_AI_GET_OUT(id)                     (((id) & 0x0fff0000) >> 16)
	
#define VENDOR_AI_PARAM_TYPE_LAYER                0x80000000
#define VENDOR_AI_PARAM_TYPE_IN                   0x90000000
#define VENDOR_AI_PARAM_TYPE_OUT                  0xa0000000
#define VENDOR_AI_GET_PARAM_TYPE(id)              ((id) & 0xf0000000)



/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/

extern void _vendor_ai_cfg_max(UINT32 maxproc);
extern BOOL _vendor_ai_is_init(VOID);
extern HD_RESULT _vendor_ai_validate(UINT32 proc_id);
extern VENDOR_AI_COMMON_INFO* _vendor_ai_common_info(void);
extern VENDOR_AI_NET_INFO_PROC* _vendor_ai_info(UINT32 proc_id);
extern HD_RESULT vendor_ai_comm_lock(void);
extern HD_RESULT vendor_ai_comm_unlock(void);

#endif  /* _VENDOR_AI_COMM_H_ */
