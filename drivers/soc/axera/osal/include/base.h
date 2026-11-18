/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AXDEV_BASE_H_
#define _AXDEV_BASE_H_

#include "axdev.h"

int axdev_bus_init(void);
void axdev_bus_exit(void);

int axdev_device_register(struct axdev_device *pdev);
void axdev_device_unregister(struct axdev_device *pdev);

struct axdev_driver *axdev_driver_register(const char *name, struct module *owner, struct axdev_ops *ops);

void axdev_driver_unregister(struct axdev_driver *pdrv);

#endif
