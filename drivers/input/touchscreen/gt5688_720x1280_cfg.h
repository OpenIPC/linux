/* drivers/input/touchscreen/gt9xx.h
 *
 * 2010 - 2013 Goodix Technology.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be a reference
 * to you, when you are integrating the GOODiX's CTP IC into your system,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * */

#ifndef _GT5688_720X1280_CFG_H_
#define _GT5688_720X1280_CFG_H_

#include <linux/kernel.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <asm/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/interrupt.h>
#include <linux/io.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <mach/gpio.h>
#include <linux/gpio.h>
#include <linux/pinctrl/consumer.h>

//***************************PART1:ON/OFF define*******************************
#define GTP_CUSTOM_CFG        0
#define GTP_CHANGE_X2Y        0
#define GTP_DRIVER_SEND_CFG   0
#define GTP_HAVE_TOUCH_KEY    1
#define GTP_ICS_SLOT_REPORT   0

#define GTP_AUTO_UPDATE       0    // auto update fw by .bin file as default
#define GTP_HEADER_FW_UPDATE  0    // auto update fw by gtp_default_FW in gt9xx_firmware.h, function together with GTP_AUTO_UPDATE
#define GTP_AUTO_UPDATE_CFG   0    // auto update config by .cfg file, function together with GTP_AUTO_UPDATE

#define GTP_COMPATIBLE_MODE   0    // compatible with GT9XXF

#define GTP_CREATE_WR_NODE    1
#define GTP_ESD_PROTECT       0    // esd protection with a cycle of 2 seconds

#define GTP_WITH_PEN          0
#define GTP_PEN_HAVE_BUTTON   0    // active pen has buttons, function together with GTP_WITH_PEN

//#define GTP_GESTURE_WAKEUP    0    // gesture wakeup

//#if (!GTP_GESTURE_WAKEUP)
//#define GTP_POWER_CTRL_SLEEP  1
//#endif

#define GTP_DEBUG_ON          1
#define GTP_DEBUG_ARRAY_ON    0
#define GTP_DEBUG_FUNC_ON     0

#if GTP_COMPATIBLE_MODE
typedef enum {
	CHIP_TYPE_GT9  = 0,
	CHIP_TYPE_GT9F = 1,
} CHIP_TYPE_T;
#endif

extern u16 show_len;
extern u16 total_len;

extern struct ctp_config_info config_info;
extern void gtp_set_int_value(int status);
extern void gtp_set_io_int(void);
#define GTP_INT_PORT			(config_info.irq_gpio.gpio)
#define GTP_RST_PORT			(config_info.wakeup_gpio.gpio)
#define GTP_INT_IRQ			(gpio_to_irq(GTP_INT_PORT))

/* TODO: define your config for Sensor_ID == 1 here, if needed
 * for  gt5688,COF,720x1280 by sochip used at sc3840 */
#define CTP_CFG_GROUP1 {\
0x5A, 0xD0, 0x02, 0x00, 0x05, 0x0A, 0x05, 0x00, 0x00, 0x00,\
0x00, 0x0F, 0x55, 0x32, 0x53, 0x01, 0x00, 0x00, 0x00, 0x00,\
0x14, 0x16, 0x18, 0x1C, 0x0F, 0x04, 0x00, 0x00, 0x00, 0x00,\
0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
0x00, 0x00, 0x00, 0x64, 0x1E, 0x28, 0x8A, 0x2A, 0x0C, 0x37,\
0x35, 0x0C, 0x08, 0x20, 0x33, 0x03, 0x83, 0x03, 0x27, 0x00,\
0x00, 0x28, 0x50, 0x80, 0x14, 0x02, 0x00, 0x00, 0x54, 0xAC,\
0x2E, 0x9B, 0x35, 0x8B, 0x3B, 0x80, 0x42, 0x77, 0x49, 0x70,\
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x14, 0x14, 0x03,\
0x04, 0x00, 0x21, 0x64, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00,\
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
0x32, 0x20, 0x50, 0x3C, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00,\
0x01, 0x08, 0x02, 0x09, 0x03, 0x0A, 0x0D, 0x06, 0x0C, 0x05,\
0x0B, 0x04, 0x01, 0x00, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A,\
0x09, 0x08, 0x07, 0x06, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,\
0x13, 0x12, 0x11, 0x10, 0x14, 0x15, 0x16, 0x17, 0xFF, 0xFF,\
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x1E, 0x00, 0x02,\
0x2A, 0x1E, 0x19, 0x14, 0x02, 0x00, 0x03, 0x0A, 0x05, 0x00,\
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x84,\
0x33, 0x03, 0x0C, 0x08, 0x33, 0x00, 0x19, 0x00, 0x00, 0x78,\
0x46, 0x32, 0x50, 0x00, 0x00, 0x05, 0x96, 0x8A, 0x01}

/* TODO: define your config for Sensor_ID == 1 here, if needed
 * for  gt9271,COF, 1024x768 by sochip used at octopus-sc5803 */
#define CTP_CFG_GROUP2 {\
}

/* TODO: define your config for Sensor_ID == 2 here, if needed
 * for yuxin gt9271,COF */
#define CTP_CFG_GROUP3 {\
}

/* TODO: define your config for Sensor_ID == 3 here, if needed
 * for dushulang gt9110,COF */
#define CTP_CFG_GROUP4 {\
}
 /* TODO: define your config for Sensor_ID == 4 here, if needed */
#define CTP_CFG_GROUP5 {\
}

/* TODO: define your config for Sensor_ID == 5 here, if needed */
#define CTP_CFG_GROUP6 {\
}

#if GTP_CUSTOM_CFG
#define GTP_MAX_HEIGHT   800
#define GTP_MAX_WIDTH    480
#define GTP_INT_TRIGGER  0            // 0: Rising 1: Falling
#else
#define GTP_MAX_HEIGHT   4096
#define GTP_MAX_WIDTH    4096
#define GTP_INT_TRIGGER  1
#endif
#define GTP_MAX_TOUCH         10

/* STEP_4(optional): If keys are available and reported as keys, config your key info here */
#if GTP_HAVE_TOUCH_KEY
#define GTP_KEY_TAB  {KEY_BACK, KEY_HOME, KEY_MENU}
#endif

//***************************PART3:OTHER define*********************************
#define GTP_DRIVER_VERSION          "V2.2<2014/01/14>"
#define GTP_I2C_NAME                "Goodix-TS"
#define GT91XX_CONFIG_PROC_FILE     "gt9xx_config"
#define GTP_POLL_TIME         10
#define GTP_ADDR_LENGTH       2
#define GTP_CONFIG_MIN_LENGTH 186
#define GTP_CONFIG_MAX_LENGTH 240
#define FAIL                  0
#define SUCCESS               1
#define SWITCH_OFF            0
#define SWITCH_ON             1

//******************** For GT9XXF Start **********************//
#define GTP_REG_BAK_REF                 0x99D0
#define GTP_REG_MAIN_CLK                0x8020
#define GTP_REG_CHIP_TYPE               0x8000
#define GTP_REG_HAVE_KEY                0x804E
#define GTP_REG_MATRIX_DRVNUM           0x8069
#define GTP_REG_MATRIX_SENNUM           0x806A

#define GTP_FL_FW_BURN              0x00
#define GTP_FL_ESD_RECOVERY         0x01
#define GTP_FL_READ_REPAIR          0x02

#define GTP_BAK_REF_SEND                0
#define GTP_BAK_REF_STORE               1
#define CFG_LOC_DRVA_NUM                29
#define CFG_LOC_DRVB_NUM                30
#define CFG_LOC_SENS_NUM                31

#define GTP_CHK_FW_MAX                  40
#define GTP_CHK_FS_MNT_MAX              300
#define GTP_BAK_REF_PATH                "/data/gtp_ref.bin"
#define GTP_MAIN_CLK_PATH               "/data/gtp_clk.bin"
#define GTP_RQST_CONFIG                 0x01
#define GTP_RQST_BAK_REF                0x02
#define GTP_RQST_RESET                  0x03
#define GTP_RQST_MAIN_CLOCK             0x04
#define GTP_RQST_RESPONDED              0x00
#define GTP_RQST_IDLE                   0xFF

//******************** For GT9XXF End **********************//
/* Registers define */
#define GTP_READ_COOR_ADDR    0x814E
#define GTP_REG_SLEEP         0x8040
#define GTP_REG_SENSOR_ID     0x814A
#define GTP_REG_CONFIG_DATA   0x8050
#define GTP_REG_VERSION       0x8140

#define RESOLUTION_LOC        3
#define TRIGGER_LOC           8

#define CFG_GROUP_LEN(p_cfg_grp)  (sizeof(p_cfg_grp) / sizeof(p_cfg_grp[0]))
/* Log define */
/***********************Add by zhongjian for sunxi tp************************
  extern u32 debug_mask;

  enum{
  DEBUG_INIT = 1U << 0,
  DEBUG_SUSPEND = 1U << 1,
  DEBUG_INT_INFO = 1U << 2,
  DEBUG_X_Y_INFO = 1U << 3,
  DEBUG_KEY_INFO = 1U << 4,
  DEBUG_WAKEUP_INFO = 1U << 5,
  DEBUG_OTHERS_INFO = 1U << 6,
  };

#define dprintk(level_mask,fmt,arg...)    if(unlikely(debug_mask & level_mask)) \
printk("***CTP***"fmt, ## arg)
 ***************************************************************************/

#define GTP_INFO(fmt, arg...)           printk("<<-GTP-INFO->> "fmt"\n", ##arg)
#define GTP_ERROR(fmt, arg...)          printk("<<-GTP-ERROR->> "fmt"\n", ##arg)
#define GTP_DEBUG(fmt, arg...)          do {\
	if (GTP_DEBUG_ON)\
	printk("<<-GTP-DEBUG->> [%d]"fmt"\n", __LINE__, ##arg);\
} while (0)
#define GTP_DEBUG_ARRAY(array, num)    do { \
	s32 i; \
	u8 *a = array; \
	if (GTP_DEBUG_ARRAY_ON) { \
		printk("<<-GTP-DEBUG-ARRAY->>\n"); \
		for (i = 0; i < (num); i++) { \
			printk("%02x   ", (a)[i]); \
			if ((i + 1) % 10 == 0) { \
				printk("\n"); \
			} \
		} \
		printk("\n"); \
	} \
} while (0)

#define GTP_DEBUG_FUNC()               do { \
	if (GTP_DEBUG_FUNC_ON)\
	printk("<<-GTP-FUNC->> Func:%s@Line:%d\n", __func__, __LINE__); \
} while (0)
#define GTP_SWAP(x, y)                 do { \
	typeof(x) z = x;\
	x = y;\
	y = z;\
} while (0)

//*****************************End of Part III********************************

#endif/* _GOODIX_GT9XX_H_ */
