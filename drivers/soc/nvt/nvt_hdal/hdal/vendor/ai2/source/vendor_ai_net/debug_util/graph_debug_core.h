/**
	@brief Header file of graph debug core.

	@file graph_debug_core.h

	@ingroup graph_debug

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _GRAPH_DEBUG_CORE_H_
#define _GRAPH_DEBUG_CORE_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include "kwrap/nvt_type.h"
#include "kwrap/file.h"

/********************************************************************
	TYPE DEFINITION
********************************************************************/

typedef  VOS_FILE     GRAPH_DEBUG_CORE_HANDLER;       ///< graph_debug_core handler

typedef struct _GRAPH_DEBUG_CORE_OPEN_CFG {
	UINT32        top_margin;            ///< top margin,        default : 100
	UINT32        bottom_margin;         ///< bottom margin,     default : 100
	UINT32        left_margin;           ///< left margin,       default : 100
	UINT32        right_margin;          ///< right margin,      default : 100
	UINT32        target_min_w;          ///< target min width,  default : 50
	UINT32        target_min_h;          ///< target min height, default : 50
	UINT32        reverse_y;             ///< reverse Y-axis,    default : 1    ( 0:false, 1:true)
	UINT32        show_detail_range_x;   ///< show detail x range, default : 0  ( 0:false, 1:true)
	UINT32        show_detail_range_y;   ///< show detail y range, default : 0  ( 0:false, 1:true)
	UINT32        show_draw_line;        ///< show draw line     , default : 1  ( 0:false, 1:true)
	UINT32        max_canvas_width;      ///< max canvas width,  default : 32000  ( this depends on browser, if you can't display, modify to smaller value)
	UINT32        max_canvas_height;     ///< max canvas height, default : 32000  ( this depends on browser, if you can't display, modify to smaller value)
} GRAPH_DEBUG_CORE_OPEN_CFG;

typedef struct _AXIS_CFG {
	CHAR          *axis_name;
	UINT32        max_axis_name_len;
	BOOL          b_show_range;
} AXIS_CFG;

typedef struct _GRAPH_DEBUG_CORE_CLOSE_CFG {
	AXIS_CFG      x_axis;
	AXIS_CFG      y_axis;
} GRAPH_DEBUG_CORE_CLOSE_CFG;


/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
extern INT graph_debug_core_set_default_open_cfg(GRAPH_DEBUG_CORE_OPEN_CFG *open_cfg);
extern INT graph_debug_core_open(GRAPH_DEBUG_CORE_OPEN_CFG *open_cfg, CHAR *filepath, GRAPH_DEBUG_CORE_HANDLER *handler);
extern INT graph_debug_core_add_block(GRAPH_DEBUG_CORE_HANDLER handler, UINT32 x1, UINT32 x2, UINT32 y1, UINT32 y2, CHAR *block_name, UINT max_block_name_len, CHAR *color_name, UINT max_color_name_len);
extern INT graph_debug_core_set_var(GRAPH_DEBUG_CORE_HANDLER handler, CHAR *var_name, UINT max_var_name_len, UINT32 value);
extern INT graph_debug_core_add_block_with_var_x1(GRAPH_DEBUG_CORE_HANDLER handler, CHAR *var_name, UINT max_var_name_len, UINT32 x2, UINT32 y1, UINT32 y2, CHAR *block_name, UINT max_block_name_len, CHAR *color_name, UINT max_color_name_len);
extern INT graph_debug_core_add_block_with_var_y1(GRAPH_DEBUG_CORE_HANDLER handler, UINT32 x1, UINT32 x2, CHAR *var_name, UINT max_var_name_len, UINT32 y2, CHAR *block_name, UINT max_block_name_len, CHAR *color_name, UINT max_color_name_len);
extern INT graph_debug_core_add_line(GRAPH_DEBUG_CORE_HANDLER handler, UINT32 x1, UINT32 y1, UINT32 x2, UINT32 y2, CHAR *color_name, UINT max_color_name_len);
extern INT graph_debug_core_close(GRAPH_DEBUG_CORE_HANDLER handler, GRAPH_DEBUG_CORE_CLOSE_CFG *close_cfg) ;

#endif  /* _GRAPH_DEBUG_CORE_H_ */
