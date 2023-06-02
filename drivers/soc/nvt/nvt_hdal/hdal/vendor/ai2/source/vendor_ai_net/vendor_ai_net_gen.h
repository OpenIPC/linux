/**
	@brief Header file of definition of vendor net generation sample.

	@file net_gen_sample.h

	@ingroup net_gen_sample

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _NET_GEN_SAMPLE_H_
#define _NET_GEN_SAMPLE_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include "hd_type.h"
#include "kwrap/type.h"
#include "vendor_ai_dla/vendor_ai_dla.h"
#include "kflow_ai_net/kflow_ai_net.h"
#include "vendor_ai.h"

/********************************************************************
 MACRO CONSTANT DEFINITIONS
********************************************************************/
#define AI_DRV_IOCTL_INIT       0x1
#define AI_FLOW_IOCTL_INIT      0x2
#define AI_KERL_MEM_INIT        0x4
//#define AI_PREPROC_INIT         0x8

/********************************************************************
	TYPE DEFINITION
********************************************************************/
typedef struct _VENDOR_AIS_IMG_PARM {
	UINT32 pa;
    UINT32 va;
	HD_VIDEO_PXLFMT fmt;                ///< image format
	UINT16 width;                       ///< image width
	UINT16 height;                      ///< image height
	UINT16 channel;                     ///< image channels
	UINT16 batch_num;                   ///< batch processing number
	UINT32 line_ofs;                    ///< line offset
	UINT32 channel_ofs;                 ///< channel offset
	UINT32 batch_ofs;                   ///< batch offset
#if AI_SUPPORT_MULTI_FMT
	UINT32 fmt_type;					///< format type
#endif
} VENDOR_AIS_IMG_PARM;

typedef struct _VENDOR_AIS_IMG_PARM_V2 {
	UINT32 img_ch_num;
	VENDOR_AIS_IMG_PARM* img_ch;
} VENDOR_AIS_IMG_PARM_V2;


/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
extern HD_RESULT vendor_ais_net_gen_init(VENDOR_AIS_FLOW_MAP_MEM_PARM map_mem, UINT32 net_id);
extern HD_RESULT vendor_ais_net_gen_uninit(UINT32 net_id);
extern UINT32 vendor_ais_user_parm_va2pa(UINT32 addr, UINT32 net_id);
extern UINT32 vendor_ais_user_parm_pa2va(UINT32 addr, UINT32 net_id);
extern UINT32 vendor_ais_user_model_va2pa(UINT32 addr, UINT32 net_id);
extern UINT32 vendor_ais_user_model_pa2va(UINT32 addr, UINT32 net_id);
extern UINT32 vendor_ais_user_buff_va2pa(UINT32 addr, UINT32 net_id);
extern UINT32 vendor_ais_user_buff_pa2va(UINT32 addr, UINT32 net_id);
extern UINT32 vendor_ais_kerl_parm_va2pa(UINT32 addr, UINT32 net_id);
extern UINT32 vendor_ais_kerl_parm_pa2va(UINT32 addr, UINT32 net_id);
extern ER vendor_ais_cal_size(UINT32 s_addr, UINT32 *p_parm_sz, UINT32 *p_model_sz, UINT32 *p_buf_sz);
extern UINT32 vendor_ais_auto_alloc_mem(VENDOR_AIS_FLOW_MEM_PARM *p_mem, VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager);
extern UINT32 vendor_ais_get_max_buf_sz_layer(VENDOR_AIS_FLOW_MEM_PARM *p_mem);
extern HD_RESULT vendor_ais_net_input_init(VENDOR_AIS_IMG_PARM *p_input_info, UINT32 net_id);
extern HD_RESULT vendor_ais_net_input_init_v2(VENDOR_AIS_IMG_PARM_V2 *p_input_info, UINT32 net_id);
extern HD_RESULT vendor_ais_net_input_uninit(UINT32 net_id);
extern HD_RESULT vendor_ais_net_engine_open(VENDOR_AI_ENG engine, UINT32 net_id);
extern HD_RESULT vendor_ais_net_engine_close(VENDOR_AI_ENG engine, UINT32 net_id);
extern VOID vendor_ais_net_gen_en_init(BOOL enable, UINT32 init, UINT32 net_id);
extern HD_RESULT vendor_ais_net_gen_chk_vers(VENDOR_AIS_FLOW_MEM_PARM *p_model, UINT32 net_id);
extern INT32 vendor_ais_net_gen_get_id_list_size(VENDOR_AIS_FLOW_MEM_PARM *p_model);
extern HD_RESULT vendor_ai_net_gen_context_init(VOID);
extern HD_RESULT vendor_ai_net_gen_context_uninit(VOID);
#if CNN_MULTI_INPUT
extern UINT32 vendor_ais_get_net_is_batch(UINT32 net_id);
#endif

#endif  /* _NET_GEN_SAMPLE_H_ */
