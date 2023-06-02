/**
	@brief Header file of graph debug schedule.

	@file graph_debug_buffer.h

	@ingroup graph_debug

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _GRAPH_DEBUG_SCHEDULE_H_
#define _GRAPH_DEBUG_SCHEDULE_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include "kwrap/nvt_type.h"
#include "kwrap/file.h"

/********************************************************************
	TYPE DEFINITION
********************************************************************/
typedef  VOS_FILE     GRAPH_DEBUG_SCHEDULE_HANDLER;       ///< graph_debug handler

/**
	Parameters of buffer optimize
*/

typedef enum _SCHE_ENG{
	SCHE_ENG_NUE,
	SCHE_ENG_NUE2,
	SCHE_ENG_CNN,
	SCHE_ENG_CNN2,
	SCHE_ENG_DSP,
	SCHE_ENG_CPU,
} SCHE_ENG;

/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/

extern INT graph_debug_schedule_open(CHAR *filepath, GRAPH_DEBUG_SCHEDULE_HANDLER *handler);
extern INT graph_debug_schedule_add_block(GRAPH_DEBUG_SCHEDULE_HANDLER handler, SCHE_ENG eng, UINT32 start_t, UINT32 end_t, CHAR *block_name, UINT max_block_name_len);

extern INT graph_debug_schedule_set_var_start_t(GRAPH_DEBUG_SCHEDULE_HANDLER handler, CHAR *var_name, UINT max_var_name_len, UINT32 value);
extern INT graph_debug_schedule_add_block_with_var_start_t(GRAPH_DEBUG_SCHEDULE_HANDLER handler, SCHE_ENG eng, CHAR *var_name, UINT max_var_name_len, UINT32 end_t, CHAR *block_name, UINT max_block_name_len);

extern INT graph_debug_schedule_add_connection(GRAPH_DEBUG_SCHEDULE_HANDLER handler, SCHE_ENG eng_src, UINT32 end_t, SCHE_ENG eng_dst, UINT32 start_t);

extern INT graph_debug_schedule_close(GRAPH_DEBUG_SCHEDULE_HANDLER handler);


#endif  /* _GRAPH_DEBUG_SCHEDULE_H_ */
