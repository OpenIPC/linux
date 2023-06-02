/**
	@brief Header file of vendor SDK debug.

	@file vendor_ai_net_debug.h

	@ingroup vendor_ai_net_debug

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _VENDOR_AI_NET_DEBUG_H_
#define _VENDOR_AI_NET_DEBUG_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include "hd_type.h"
#include "kflow_ai_net/kflow_ai_net.h"

/********************************************************************
	TYPE DEFINITION
********************************************************************/
#ifndef ENUM_DUMMY4WORD
#define ENUM_DUMMY4WORD(name)   E_##name = 0x10000000
#endif

typedef enum {
	NET_DBG_SPACE_USER = 0,
	NET_DBG_SPACE_KERL,
	ENUM_DUMMY4WORD(NET_DBG_SPACE)
} NET_DBG_SPACE;

typedef enum {
	NET_DBG_ITEM_IOMEM = 0,
	NET_DBG_ITEM_MCTRL,
	NET_DBG_ITEM_AIPARM,
	NET_DBG_ITEM_LLCMD,
	NET_DBG_ITEM_MCTRL_ENTRY,
	NET_DBG_ITEM_GROUP,
	NET_DBG_ITEM_MEM_ENTRY,
	NET_DBG_ITEM_CRASH,
	NET_DBG_ITEM_DOT_BUF_NODE = 100,
	NET_DBG_ITEM_DOT_LAYER_NODE,
	NET_DBG_ITEM_DOT_GROUP_NODE,
	ENUM_DUMMY4WORD(NET_DBG_ITEM)
} NET_DBG_ITEM;

typedef enum {
	NET_DBG_LAYER_FULL = 0,
	NET_DBG_LAYER_OUTPUT,
	ENUM_DUMMY4WORD(NET_DBG_LAYER)
} NET_DBG_LAYER;

/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#define debug_vprintf 		vprintf
#define _ATOI(str,pint)  	do { *(int*)(pint) = atoi((char*)str); } while (0)
#define _ATOUL(str,puint32)  	do { *(int*)(puint32) = atoi((char*)str); } while (0)
#define _ATOU32(str,puint32)  	do { *(int*)(puint32) = atoi((char*)str); } while (0)
#define _ATOU16(str,puint16)  	do { *(int*)(puint16) = atoi((char*)str); } while (0)
#define _ATOU8(str,puint8)  	do { *(int*)(puint8) = atoi((char*)str); } while (0)

#define AI_FLOW	0x00000001  //init, set, open, start, proc, stop, close, uninit
#define AI_GRP	0x00000010
#define AI_JOB	0x00000100
#define AI_BUF	0x00001000

#define AI_PERF	0x10000000	//performance
#define AI_RES	0x20000000	//resource
	
extern void vendor_ai_net_trace(UINT32 proc_id, UINT32 class_bits, const char *fmtstr, ...);
extern void vendor_ai_net_set_trace_class(UINT32 proc_id, UINT32 class_mask);
extern HD_RESULT vendor_ai_net_debug_dump(UINT32 proc_id, NET_DBG_SPACE space, NET_DBG_ITEM item, VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager, CHAR *filepath);
extern HD_RESULT vendor_ai_net_debug_layer(UINT32 proc_id, NET_DBG_LAYER layer_opt, VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager, CHAR *filedir);
extern HD_RESULT vendor_ai_net_debug_performance(UINT32 proc_id, CHAR *p_model_name, UINT64 *p_proc_time, DOUBLE p_cpu_loading);
extern HD_RESULT vendor_ai_net_debug_init(VOID);
extern HD_RESULT vendor_ai_net_debug_uninit(VOID);

#endif  /* _VENDOR_AI_NET_DEBUG_H_ */
