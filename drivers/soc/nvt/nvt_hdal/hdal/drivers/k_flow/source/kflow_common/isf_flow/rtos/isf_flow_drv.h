#ifndef __ISF_FLOW_DRV_H
#define __ISF_FLOW_DRV_H

#include "isf_flow_ioctl.h"

#define MODULE_IRQ_NUM          0
#define MODULE_REG_NUM          1
#define MODULE_CLK_NUM          0

typedef struct module_info {
	ISF_FLOW_IOCTL_OUT_LOG u_log_cmd;
} MODULE_INFO, *PMODULE_INFO;

#endif

