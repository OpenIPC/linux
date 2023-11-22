
/******************** (C) COPYRIGHT 2016 Silan *********************************
*
* File Name          : sc7a20_acc.h
* Authors            : djq(dongjianqing@silan.com.cn)
* Version            : V 1.0.0
* Date               : 09/20/2016
*
********************************************************************************
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* THE PRESENT SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES
* OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, FOR THE SOLE
* PURPOSE TO SUPPORT YOUR APPLICATION DEVELOPMENT.
* AS A RESULT, SILAN SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
* THIS SOFTWARE IS SPECIFICALLY DESIGNED FOR EXCLUSIVE USE WITH ST PARTS.
*
*******************************************************************************/
/*******************************************************************************
Version History.

Revision 1-0-0 05/11/2016
First Release
*******************************************************************************/

#ifndef	__SC7A20_H__
#define	__SC7A20_H__

#include	<linux/ioctl.h>	/* For IOCTL macros */
#include	<linux/input.h>

#ifndef DEBUG
#define DEBUG
#endif

#define SAD0L			0x00
#define SAD0H			0x01

#define SC7A20_ACC_I2C_SADROOT	0x0c  

#define SC7A20_ACC_I2C_SAD_L	((SC7A20_ACC_I2C_SADROOT<<1)|SAD0L)     //0x18
#define SC7A20_ACC_I2C_SAD_H	((SC7A20_ACC_I2C_SADROOT<<1)|SAD0H)     //0x19
#define	SC7A20_ACC_DEV_NAME	    "sc7a20_acc"
// #define	SC7A20_ACC_DEV_NAME	    "lis3dh_acc"


/* SAO pad is connected to GND, set LSB of SAD '0' */
#define SC7A20_ACC_I2C_ADDR     SC7A20_ACC_I2C_SAD_H
#define SC7A20_ACC_I2C_NAME     "sc7a20_acc"

#define	SC7A20_ACC_IOCTL_BASE 77
/** The following define the IOCTL command values via the ioctl macros */
#define	SC7A20_ACC_IOCTL_SET_DELAY		    _IOW(SC7A20_ACC_IOCTL_BASE, 0, int)
#define	SC7A20_ACC_IOCTL_GET_DELAY		    _IOR(SC7A20_ACC_IOCTL_BASE, 1, int)
#define	SC7A20_ACC_IOCTL_SET_ENABLE		    _IOW(SC7A20_ACC_IOCTL_BASE, 2, int)
#define	SC7A20_ACC_IOCTL_GET_ENABLE		    _IOR(SC7A20_ACC_IOCTL_BASE, 3, int)
#define	SC7A20_ACC_IOCTL_SET_FULLSCALE	    _IOW(SC7A20_ACC_IOCTL_BASE, 4, int)
#define	SC7A20_ACC_IOCTL_SET_G_RANGE	    SC7A20_ACC_IOCTL_SET_FULLSCALE

#define	SC7A20_ACC_IOCTL_SET_CTRL_REG3		_IOW(SC7A20_ACC_IOCTL_BASE, 6, int)
#define	SC7A20_ACC_IOCTL_SET_CTRL_REG6		_IOW(SC7A20_ACC_IOCTL_BASE, 7, int)
#define	SC7A20_ACC_IOCTL_SET_DURATION1		_IOW(SC7A20_ACC_IOCTL_BASE, 8, int)
#define	SC7A20_ACC_IOCTL_SET_THRESHOLD1		_IOW(SC7A20_ACC_IOCTL_BASE, 9, int)
#define	SC7A20_ACC_IOCTL_SET_CONFIG1		_IOW(SC7A20_ACC_IOCTL_BASE, 10, int)

#define	SC7A20_ACC_IOCTL_SET_DURATION2		_IOW(SC7A20_ACC_IOCTL_BASE, 11, int)
#define	SC7A20_ACC_IOCTL_SET_THRESHOLD2		_IOW(SC7A20_ACC_IOCTL_BASE, 12, int)
#define	SC7A20_ACC_IOCTL_SET_CONFIG2		_IOW(SC7A20_ACC_IOCTL_BASE, 13, int)

#define	SC7A20_ACC_IOCTL_GET_SOURCE1	    _IOW(SC7A20_ACC_IOCTL_BASE, 14, int)
#define	SC7A20_ACC_IOCTL_GET_SOURCE2		_IOW(SC7A20_ACC_IOCTL_BASE, 15, int)

#define	SC7A20_ACC_IOCTL_GET_TAP_SOURCE		_IOW(SC7A20_ACC_IOCTL_BASE, 16, int)

#define	SC7A20_ACC_IOCTL_SET_TAP_TW		    _IOW(SC7A20_ACC_IOCTL_BASE, 17, int)
#define	SC7A20_ACC_IOCTL_SET_TAP_CFG		_IOW(SC7A20_ACC_IOCTL_BASE, 18, int)
#define	SC7A20_ACC_IOCTL_SET_TAP_TLIM		_IOW(SC7A20_ACC_IOCTL_BASE, 19, int)
#define	SC7A20_ACC_IOCTL_SET_TAP_THS		_IOW(SC7A20_ACC_IOCTL_BASE, 20, int)
#define	SC7A20_ACC_IOCTL_SET_TAP_TLAT		_IOW(SC7A20_ACC_IOCTL_BASE, 21, int)

#define SC7A20_ACC_IOCTL_GET_COOR_XYZ        _IOR(SC7A20_ACC_IOCTL_BASE, 22, int[3])

#define SC7A20_ACC_IOCTL_GET_CHIP_ID         _IOR(SC7A20_ACC_IOCTL_BASE, 255, char[32])

/************************************************/
/* 	Accelerometer defines section	 	*/
/************************************************/

/* Accelerometer Sensor Full Scale */
#define	SC7A20_ACC_FS_MASK		0x30
#define SC7A20_ACC_G_2G 		0x00
#define SC7A20_ACC_G_4G 		0x10
#define SC7A20_ACC_G_8G 		0x20
#define SC7A20_ACC_G_16G 	    0x30

/* Accelerometer Sensor Operating Mode */
#define SC7A20_ACC_ENABLE		0x01
#define SC7A20_ACC_DISABLE		0x00
//#define RES_8BIT	  	        0
//#define RES_12BIT				(1 << 6)
//#define SC7A20_ACC_FS_MASK	    0x30
//#define SC7A20_ACC_G_2G			0x00
//#define SC7A20_ACC_G_4G			0x10
//#define SC7A20_ACC_G_8G			0x20
//#define SC7A20_ACC_G_16G		    0x30

#ifdef	__KERNEL__
struct sc7a20_platform_data {
	int poll_interval;
	int min_interval;

	u32 g_range;

	u32 axis_map_x;
	u32 axis_map_y;
	u32 axis_map_z;

	u32 negate_x;
	u32 negate_y;
	u32 negate_z;

	int (*init)(void);
	void (*exit)(void);
	int (*power_on)(void);
	int (*power_off)(void);
	

	int gpio_int1;
	int gpio_int2;

};
#endif	/* __KERNEL__ */

#endif	/* __SC7A20_H__ */



