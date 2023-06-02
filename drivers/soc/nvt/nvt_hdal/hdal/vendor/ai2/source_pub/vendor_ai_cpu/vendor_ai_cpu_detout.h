/**
	@brief Header file of definition of cpu detout layer.

	@file vendor_ai_cpu_detout.h

	@ingroup vendor_ai_cpu_detout

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
#ifndef _VENDOR_AI_CPU_DETOUT_H_
#define _VENDOR_AI_CPU_DETOUT_H_

/********************************************************************
 MACRO CONSTANT DEFINITIONS
********************************************************************/
#define VENDOR_AIS_LBL_LEN      256     ///< maximal length of class label
#define MAX_CLASS_NUM           1000
#define NN_DETOUT_TOP_N         5

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include "hd_type.h"
#include "vendor_ai_net/nn_verinfo.h"
#include "vendor_ai_net/nn_net.h"
#include "vendor_ai_net/nn_parm.h"

/********************************************************************
	TYPE DEFINITION
********************************************************************/

/**
	Detection output information
*/
typedef struct _VENDOR_AI_DETOUT_RESULT {
	INT32 no[NN_DETOUT_TOP_N];                          ///< class number
	FLOAT score[NN_DETOUT_TOP_N];                       ///< class score
	FLOAT x;
    FLOAT y;
	FLOAT w;
    FLOAT h;
} VENDOR_AI_DETOUT_RESULT;

typedef struct _VENDOR_AI_DETOUT_RESULT_INFO {
	UINT32 sign;          ///< signature = MAKEFOURCC('A','D','0','1')    D:Detect 01=detection output
	UINT32 chunk_size;    ///< sizeof(all buffer) - 8;
	UINT32 result_num;
	VENDOR_AI_DETOUT_RESULT *p_result;
} VENDOR_AI_DETOUT_RESULT_INFO;

/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
extern HD_RESULT vendor_ais_detout(NN_DETOUT_PARM *p_parm, UINT32 net_id);
extern UINT32 vendor_ai_cpu_detout_calcbuffersize(UINT32 proc_id, UINT32 p_num_classes);
extern HD_RESULT vendor_ai_cpu_detout_setbuffer(UINT32 proc_id, UINT32 p_out_addr, UINT32 out_size, UINT32 p_num_classes);
extern VENDOR_AI_DETOUT_RESULT_INFO *vendor_ai_cpu_detout_getresult(UINT32 proc_id);
extern HD_RESULT vendor_ai_cpu_detout_get_labelnum(UINT32 proc_id, UINT32 *p_label_num);

#endif  /* _VENDOR_AI_CPU_DETOUT_H_ */
