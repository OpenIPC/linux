/**
	@brief Header file of graph debug buffer.

	@file graph_debug_buffer.h

	@ingroup graph_debug

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _GRAPH_DEBUG_BUFFER_H_
#define _GRAPH_DEBUG_BUFFER_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include "kwrap/nvt_type.h"
#include "kwrap/file.h"

/********************************************************************
	TYPE DEFINITION
********************************************************************/
typedef  VOS_FILE     GRAPH_DEBUG_BUFFER_HANDLER;       ///< graph_debug handler

/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/

extern INT graph_debug_buffer_open(CHAR *filepath, GRAPH_DEBUG_BUFFER_HANDLER *handler);
extern INT graph_debug_buffer_add_block(GRAPH_DEBUG_BUFFER_HANDLER handler, UINT32 x1, UINT32 x2, UINT32 y1, UINT32 y2, CHAR *block_name, UINT max_block_name_len);
extern INT graph_debug_buffer_set_var(GRAPH_DEBUG_BUFFER_HANDLER handler, CHAR *var_name, UINT max_var_name_len, UINT32 value);
extern INT graph_debug_buffer_add_block_with_var_x1(GRAPH_DEBUG_BUFFER_HANDLER handler, CHAR *var_name, UINT max_var_name_len, UINT32 x2, UINT32 y1, UINT32 y2, CHAR *block_name, UINT max_block_name_len);
extern INT graph_debug_buffer_add_block_with_var_y1(GRAPH_DEBUG_BUFFER_HANDLER handler, UINT32 x1, UINT32 x2, CHAR *var_name, UINT max_var_name_len, UINT32 y2, CHAR *block_name, UINT max_block_name_len);
extern INT graph_debug_buffer_close(GRAPH_DEBUG_BUFFER_HANDLER handler);

#endif  /* _GRAPH_DEBUG_BUFFER_H_ */
