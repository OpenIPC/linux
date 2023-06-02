/**
	@brief Header file of graph debug log.

	@file graph_debug_log.h

	@ingroup graph_debug

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _GRAPH_DEBUG_LOG_H_
#define _GRAPH_DEBUG_LOG_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/

/********************************************************************
	TYPE DEFINITION
********************************************************************/

/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
extern int graph_debug_log_open(int buffer_size);
extern int graph_debug_log_close(void);
extern int graph_debug_log_print(const char *fmt, ...);
extern int graph_debug_log_dump(void);

#endif  /* _GRAPH_DEBUG_CORE_H_ */
