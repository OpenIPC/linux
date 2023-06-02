/* *
 * Copyright (C) 2009 Texas Instruments Inc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef DM365_A3_HW_H
#define DM365_A3_HW_H

#ifdef __KERNEL__
#include <asm/io.h>
#include <linux/device.h>
#include <mach/hardware.h>
#include "dm365_aew.h"
#include "dm365_af.h"

/* AF/AE/AWB Base address and range */
#define DM365_A3_HW_ADDR	0x1c71400
#define DM365_A3_HW_ADDR_SIZE	128
/* AF Register Offsets */

/* Peripheral Revision and Class Information */
#define AFPID			0x0
/* Peripheral Control Register */
#define AFPCR			0x4
/* Setup for the Paxel Configuration */
#define AFPAX1			0x8
/* Setup for the Paxel Configuration */
#define AFPAX2			0xc
/* Start Position for AF Engine Paxels */
#define AFPAXSTART		0x10
/* Start Position for IIRSH */
#define AFIIRSH			0x14
/* SDRAM/DDRAM Start address */
#define AFBUFST			0x18
/* IIR filter coefficient data for SET 0 */
#define AFCOEF010		0x1c
/* IIR filter coefficient data for SET 0 */
#define AFCOEF032		0x20
/* IIR filter coefficient data for SET 0 */
#define AFCOEF054		0x24
/* IIR filter coefficient data for SET 0 */
#define AFCOEF076		0x28
/* IIR filter coefficient data for SET 0 */
#define AFCOEF098		0x2c
/* IIR filter coefficient data for SET 0 */
#define AFCOEF0010		0x30
/* IIR filter coefficient data for SET 1 */
#define AFCOEF110		0x34
/* IIR filter coefficient data for SET 1 */
#define AFCOEF132		0x38
/* IIR filter coefficient data for SET 1 */
#define AFCOEF154		0x3c
/* IIR filter coefficient data for SET 1 */
#define AFCOEF176		0x40
/* IIR filter coefficient data for SET 1 */
#define AFCOEF198		0x44
/* IIR filter coefficient data for SET 1 */
#define AFCOEF1010		0x48

/* Vertical Focus vlaue configuration 1 */
#define AF_VFV_CFG1		0x68
/* Vertical Focus vlaue configuration 2 */
#define AF_VFV_CFG2		0x6c
/* Vertical Focus vlaue configuration 3 */
#define AF_VFV_CFG3		0x70
/* Vertical Focus vlaue configuration 4 */
#define AF_VFV_CFG4		0x74
/* Horizontal Focus vlaue Threshold */
#define AF_HFV_THR		0x78
/* COEFFICIENT BASE ADDRESS */

#define AF_OFFSET	0x00000004

/* AEW Register offsets */
#define AEWPID			AFPID
/* Peripheral Control Register */
#define AEWPCR			AFPCR
/* Configuration for AE/AWB Windows */
#define AEWWIN1			0x4c
/* Start position for AE/AWB Windows */
#define AEWINSTART		0x50
/* Start position and height for black linr of AE/AWB Windows */
#define AEWINBLK		0x54
/* Configuration for subsampled data in AE/AWB windows */
#define AEWSUBWIN		0x58
/* SDRAM/DDRAM Start address for AEW Engine */
#define AEWBUFST       		0x5c
/* Line start */
#define LINE_START		0x64

/* AEW Engine configuration */
#define AEW_CFG			0x60

/* PID fields */
#define PID_MINOR			(0x3f << 0)
#define PID_MAJOR			(7 << 8)
#define PID_RTL				(0x1f << 11)
#define PID_FUNC			(0xFFF << 16)
#define PID_SCHEME			(3 << 30)

/* PCR FIELDS */

/*Saturation Limit */
#define AVE2LMT			(0x3ff << 22)
#define AF_VF_EN		(1 << 20)
#define AEW_MED_EN		(1 << 19)
/* Busy bit for AEW */
#define AEW_BUSYAEWB		(1 << 18)
/* Alaw Enable/Disable Bit */
#define AEW_ALAW_EN		(1 << 17)
/* AEW Engine Enable/Disable bit */
#define AEW_EN			(1 << 16)
/* Busy Bit for AF */
#define AF_BUSYAF		(1 << 15)
#define FVMODE			(1 << 14)
#define RGBPOS			(7 << 11)
#define MED_TH			(0xFF << 3)
#define AF_MED_EN		(1 << 2)
#define AF_ALAW_EN		(1 << 1)
#define AF_EN			(1 << 0)

/*
 * AFPAX1 fields
 */
#define PAXW			(0xFF << 16)
#define PAXH			(0xFF)

/*
 * AFPAX2 fields
 */
#define  AFINCH			(0xF << 17)
#define  AFINCV			(0xF << 13)
#define  PAXVC			(0x7F << 6)
#define  PAXHC			(0x3F)

/*
 * AFPAXSTART fields
 */
#define  PAXSH			(0xFFF << 16)
#define  PAXSV			(0xFFF)

/*
 * IIR COEFFICIENT MASKS
 */
#define COEF_MASK0		(0xFFF)
#define COEF_MASK1		(0xFFF << 16)

/*
 * VFV_CFGX COEFFICIENT MASKS
 */
#define VFV_COEF_MASK0		(0xFF)
#define VFV_COEF_MASK1		(0xFF << 8)
#define VFV_COEF_MASK2		(0xFF << 16)
#define VFV_COEF_MASK3		(0xFF << 24)

/* HFV THR MASKS */
#define HFV_THR0_MASK		(0xFFFF)
#define HFV_THR2_SHIFT		(16)
#define HFV_THR2_MASK		(0xFFFF << HFV_THR2_SHIFT)

/* VFV THR MASKS */
#define VFV_THR_SHIFT		(16)
#define VFV_THR_MASK		(0xFFFF << VFV_THR_SHIFT)

/* BIT SHIFTS */
#define AF_BUSYAF_SHIFT			15
#define AEW_EN_SHIFT			16
#define AEW_BUSYAEW_SHIFT		18
#define AF_RGBPOS_SHIFT			11
#define AF_MED_TH_SHIFT			3
#define AF_PAXW_SHIFT			16
#define AF_LINE_INCR_SHIFT		13
#define AF_COLUMN_INCR_SHIFT		17
#define AF_VT_COUNT_SHIFT		6
#define AF_HZ_START_SHIFT		16
#define AF_COEF_SHIFT			16

/* AEWWIN1 fields */
/* Window Height */
#define WINH			(0x7F << 24)
/* Window Width */
#define WINW			(0x7f << 13)
/* Window vertical Count */
#define WINVC			(0x7f << 6)
/* Window Horizontal Count */
#define WINHC			(0x3f)

/* AEWWINSTART fields */
/* Window Vertical Start */
#define WINSV			(0xfff << 16)
/* Window Horizontal start */
#define WINSH			(0xfff)

/* AEWWINBLK fields
 * Black Window Vertical Start
 */
#define BLKWINSV		(0xfff << 16)
/* Black Window height */
#define BLKWINH			(0x7f)

/* AEWSUBWIN fields
 * Vertical Lime Increment
 */
#define AEWINCV			(0xf << 8)
/* Horizontal Line Increment */
#define AEWINCH			(0xf)

/* BIT POSITIONS */
#define AEW_AVE2LMT_SHIFT		22

#define AEW_WINH_SHIFT			24
#define AEW_WINW_SHIFT              	13
#define AEW_VT_COUNT_SHIFT		6
#define AEW_VT_START_SHIFT		16
#define AEW_LINE_INCR_SHIFT		8
#define AEW_BLKWIN_VT_START_SHIFT   	16
#define AEW_EN_SHIFT                	16
#define AEW_BUSYAEWB_SHIFT          	18

#define AEFMT_SHFT			8
#define AEFMT_MASK			(3 << AEFMT_SHFT)
#define AEW_SUMSHFT_MASK		(0xf)

#define SET_VAL(x)		(((x) / 2) - 1)
#define NOT_EVEN		1
#define CHECK_EVEN(x)		((x) % 2)

/* Function declaration for af */
int af_register_setup(struct device *, struct af_device *);
void af_engine_setup(struct device *, int);
void af_set_address(struct device *, unsigned long);
/* Function Declaration for aew */
int aew_register_setup(struct device *, struct aew_device *);
void aew_engine_setup(struct device *, int);
void aew_set_address(struct device *, unsigned long);
u32 af_get_hw_state(void);
u32 aew_get_hw_state(void);
u32 af_get_enable(void);
u32 aew_get_enable(void);

#endif				/*end of #ifdef __KERNEL__ */
#endif				/*end of #ifdef __DAVINCI_A3_HW_H */
