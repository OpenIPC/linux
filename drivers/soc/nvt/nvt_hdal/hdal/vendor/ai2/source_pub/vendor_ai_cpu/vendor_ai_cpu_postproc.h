/**
	@brief Header file of definition of cpu postproc layer.

	@file vendor_ai_cpu_postproc.h

	@ingroup vendor_ai_cpu_postproc

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
#ifndef _VENDOR_AI_CPU_POSTPROC_H_
#define _VENDOR_AI_CPU_POSTPROC_H_

/********************************************************************
 MACRO CONSTANT DEFINITIONS
********************************************************************/
#define VENDOR_AIS_LBL_LEN      256     ///< maximal length of class label
#define MAX_CLASS_NUM           1000
#define NN_POSTPROC_TOP_N       5

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
    Result information.
*/
typedef struct _VENDOR_AI_POSTPROC_RESULT {
	INT32 no[NN_POSTPROC_TOP_N];						///< class number
	FLOAT score[NN_POSTPROC_TOP_N];						///< class score
	//UINT16 x;
    //UINT16 y;
	//UINT16 w;
    //UINT16 h;
} VENDOR_AI_POSTPROC_RESULT;

typedef struct _VENDOR_AI_POSTPROC_RESULT_INFO {
	UINT32 sign;          ///< signature = MAKEFOURCC('A','C','0','1')    C:Classify 01:Accuracy
	UINT32 chunk_size;    ///< sizeof(all buffer) - 8;
	UINT32 result_num;
	VENDOR_AI_POSTPROC_RESULT *p_result;
} VENDOR_AI_POSTPROC_RESULT_INFO;

/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
extern HD_RESULT vendor_ais_postproc(UINT32 proc_id, UINT32 parm_addr);
extern UINT32 vendor_ai_cpu_postproc_calcbuffersize(VOID);
extern HD_RESULT vendor_ai_cpu_postproc_setbuffer(UINT32 proc_id, UINT32 p_out_addr, UINT32 out_size);
extern VENDOR_AI_POSTPROC_RESULT_INFO *vendor_ai_cpu_postproc_getresult(UINT32 proc_id);
extern HD_RESULT vendor_ai_cpu_postproc_get_labelnum(UINT32 proc_id, UINT32 *p_label_num);

#endif  /* _VENDOR_AI_CPU_POSTPROC_H_ */
