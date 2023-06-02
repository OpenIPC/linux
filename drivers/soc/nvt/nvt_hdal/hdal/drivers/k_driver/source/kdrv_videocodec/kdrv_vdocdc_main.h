/*
    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.

    @file       kdrv_vdocdc_main.h

    @brief      kdrv_vdocdc main header file

    @version    V1.00.000
    @author     Novatek FW Team
    @date       2019/01/21
*/
#ifndef _KDRV_VDOCDC_MAIN_H
#define _KDRV_VDOCDC_MAIN_H

#include <linux/cdev.h>

//#define H26XE_VER_STR			"0.2.45"	/*add initial pq parameters*/
//#define H26XE_VER_STR			"0.3.0"		/*support motion ratio & jnd*/
//#define H26XE_VER_STR			"0.3.1"		/*(2020/05/12): update tile settings. remove unused condition.
//									correct search range setting for h264
//									correct h/w limitation definition for 520 and add h/w limitation definition for 528
//									update register map to 528 format
//									support osg chroma alpha for 528 */
//#define H26XE_VER_STR			"0.3.2"		/*(2020/05/19): update h/w limitation description
//									support 528 feature */
//#define H26XE_VER_STR			"0.3.3"	/*(2020/05/27): set rrc ndqp and rrc stop = 0.05 */
//#define H26XE_VER_STR			"0.3.5"	/*(2020/06/08): support dynamic change gop */
//#define H26XE_VER_STR			"0.3.7"	/*(2021/02/02): fix hevc lpm init */
#define H26XE_VER_STR			"0.3.8"	/*(2021/02/19): add proc for debug */

#define MODULE_NAME			"kdrv_h26x"
#define MODULE_MINOR_ID		0
#define MODULE_MINOR_COUNT	1

typedef struct kdrv_vdocdc_drv_info {
	struct class *pmodule_class;
	struct device *pdevice[MODULE_MINOR_COUNT];
	struct cdev cdev;
	dev_t dev_id;
} KDRV_VDOCDC_DRV_INFO;

#endif	// _KDRV_VDOCDC_MAIN_H

