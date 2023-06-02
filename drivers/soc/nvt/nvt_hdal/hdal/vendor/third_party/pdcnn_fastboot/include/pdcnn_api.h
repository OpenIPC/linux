/**
	@brief Header file of definition of Retina PD postprocessing API.

	@file pdcnn_api.h

	@ingroup pdcnn

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2020.  All rights reserved.
*/
#ifndef _PDCNN_API_H_
#define _PDCNN_API_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include "kwrap/type.h"

/********************************************************************
	TYPE DEFINITION
********************************************************************/
typedef struct _PD_MEM_PARM {
	UINT32 pa;
	UINT32 va;
	UINT32 size;
	UINT32 blk;
} PD_MEM_PARM;

typedef struct _PD_MEM {
	PD_MEM_PARM pps_result;
	INT32 out_num;
	PD_MEM_PARM out_result;
	PD_MEM_PARM backbone_output;
} PD_MEM;

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

/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
UINT32 pd_get_mem_size(VOID);
VOID pd_set_mem(PD_MEM *p_pd_mem, UINT32 va);
ER get_pdcnn_postpara(PD_MEM *p_pd_mem, UINT32 in_va[]);
VOID pd_print_results(PD_MEM *p_pd_mem, UINT32 vdo_width, UINT32 vdo_height);

#endif  /* _PDCNN_API_H_ */
