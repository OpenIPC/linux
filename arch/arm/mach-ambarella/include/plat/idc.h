/*
 * arch/arm/plat-ambarella/include/plat/idc.h
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2010, Ambarella, Inc.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#ifndef __PLAT_AMBARELLA_IDC_H__
#define __PLAT_AMBARELLA_IDC_H__

/* ==========================================================================*/
#if (CHIP_REV == A5S) || (CHIP_REV == A7L)
#define IDC_INSTANCES			2
#define IDC_SUPPORT_INTERNAL_MUX	1
#define IDC3_BUS_MUX			GPIO(36)
#else
#define IDC_INSTANCES			3
#define IDC_SUPPORT_INTERNAL_MUX	0
#endif

/* ==========================================================================*/
#define IDC_OFFSET			0x3000
#define IDC_BASE			(APB_BASE + IDC_OFFSET)
#define IDC_REG(x)			(IDC_BASE + (x))

#if (CHIP_REV == S2L) || (CHIP_REV == S3) || (CHIP_REV == S3L)
#define IDC2_OFFSET			0x1000
#else
#define IDC2_OFFSET			0x7000
#endif
#define IDC2_BASE			(APB_BASE + IDC2_OFFSET)
#define IDC2_REG(x)			(IDC2_BASE + (x))

#if (CHIP_REV == S2) || (CHIP_REV == S2E)
#define IDC3_OFFSET			0x13000
#elif (CHIP_REV == A8)
#define IDC3_OFFSET			0xE000
#elif (CHIP_REV == S2L) || (CHIP_REV == S3) || (CHIP_REV == S3L)
#define IDC3_OFFSET			0x7000
#endif
#define IDC3_BASE			(APB_BASE + IDC3_OFFSET)
#define IDC3_REG(x)			(IDC3_BASE + (x))

/* ==========================================================================*/
#define IDC_ENR_OFFSET			0x00
#define IDC_CTRL_OFFSET			0x04
#define IDC_DATA_OFFSET			0x08
#define IDC_STS_OFFSET			0x0c
#define IDC_PSLL_OFFSET			0x10
#define IDC_PSLH_OFFSET			0x14
#define IDC_FMCTRL_OFFSET		0x18
#define IDC_FMDATA_OFFSET		0x1c
#define IDC_DUTYCYCLE_OFFSET	0x24
#define IDC_STRETCHSCL_OFFSET	0x28

#define IDC_ENR_REG_ENABLE		(0x01)
#define IDC_ENR_REG_DISABLE		(0x00)

#define IDC_CTRL_STOP			(0x08)
#define IDC_CTRL_START			(0x04)
#define IDC_CTRL_IF			(0x02)
#define IDC_CTRL_ACK			(0x01)
#define IDC_CTRL_CLS			(0x00)

#define IDC_STS_FIFO_EMP		(0x04)
#define IDC_STS_FIFO_FUL		(0x02)

#define IDC_FIFO_BUF_SIZE		(63)

#define IDC_FMCTRL_STOP			(0x08)
#define IDC_FMCTRL_START		(0x04)
#define IDC_FMCTRL_IF			(0x02)

/* ==========================================================================*/

#endif /* __PLAT_AMBARELLA_IDC_H__ */

