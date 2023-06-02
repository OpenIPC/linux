/**
	@brief Header file of vendor ai net command.

	@file vendor_ai_net_cmd.h

	@ingroup vendor_ai_net

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _VENDOR_AI_NET_CMD_H_
#define _VENDOR_AI_NET_CMD_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include "hd_type.h"
#include "kflow_ai_net/kflow_ai_net.h"

/********************************************************************
	TYPE DEFINITION
********************************************************************/


/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
#define CMD_MAX_PATH_LEN	100

// group
#define VENDOR_AI_NET_CMD_DOT_GROUP		(1 << 1)
#define VENDOR_AI_NET_CMD_MCTRL_ENTRY	(1 << 2)
#define VENDOR_AI_NET_CMD_GROUP			(1 << 3)
#define VENDOR_AI_NET_CMD_MEM_LIST		(1 << 4)

// iomem
#define VENDOR_AI_NET_CMD_IOMEM_DUMP_DEBUG      (1 << 1)
#define VENDOR_AI_NET_CMD_IOMEM_CHK_OVERLAP     (1 << 2)
#define VENDOR_AI_NET_CMD_IOMEM_REORDER_DEBUG   (1 << 3)
#define VENDOR_AI_NET_CMD_IOMEM_SIM_AI1_BUG     (1 << 4)
#define VENDOR_AI_NET_CMD_IOMEM_SHRINK_DEBUG    (1 << 5)
#define VENDOR_AI_NET_CMD_IOMEM_NETINFO_DUMP    (1 << 6)
#define VENDOR_AI_NET_CMD_IOMEM_CLEAR_IOBUF     (1 << 7)

HD_RESULT vendor_ai_cmd_get_group_dump(unsigned int proc_id);
HD_RESULT vendor_ai_cmd_get_iomem_debug(unsigned int proc_id);
HD_RESULT vendor_ai_cmd_init(UINT32 net_id);
HD_RESULT vendor_ai_cmd_uninit(UINT32 net_id);
#endif  /* _VENDOR_AI_NET_CMD_H_ */
