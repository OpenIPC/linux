/*
    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.

    @file       kdrv_vdocdc_main.h

    @brief      kdrv_vdocdc main header file

    @version    V1.00.000
    @author     Novatek FW Team
    @date       2019/01/21
*/
/*
0.2.48: support difference maq
*/
#ifndef _NVT_VDOCDC_MAIN_H
#define _NVT_VDOCDC_MAIN_H

#include <linux/cdev.h>

#include "nvt_vdocdc_drv.h"

#define MODULE_NAME			"nvt_h26x"
#define MODULE_MINOR_ID		0
#define MODULE_MINOR_COUNT	1

typedef struct nvt_vdocdc_drv_info {
	MODULE_INFO module_info;

	struct class *pmodule_class;
	struct device *pdevice[MODULE_MINOR_COUNT];
	struct resource* presource[MODULE_REG_NUM];
	struct cdev cdev;
	dev_t dev_id;
} NVT_VDOCDC_DRV_INFO;

int nvt_vdocdc_module_init(void);
void nvt_vdocdc_module_exit(void);

#endif	// _NVT_VDOCDC_MAIN_H

