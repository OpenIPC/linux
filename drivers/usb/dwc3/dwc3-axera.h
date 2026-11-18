/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AXERA_USB_SUPPORT__
#define __AXERA_USB_SUPPORT__

#include "core.h"

extern void axera_usb_global_init(struct dwc3 *dwc);
extern void axera_usb_host_init(struct dwc3 *dwc);
extern void axera_usb_device_init(struct dwc3 *dwc);

#ifdef CONFIG_TYPEC_SGM7220
#include <linux/list.h>

extern void put_dwc3_into_list(struct dwc3 *dwc);
extern struct list_head *get_dwc3_list(void);

struct dwc3_dev_list{
	struct list_head list_node;
	struct dwc3 *dwc;
};
#endif

#endif
