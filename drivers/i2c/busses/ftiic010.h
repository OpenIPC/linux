/*
 * Faraday FTIIC010 I2C Controller
 *
 * (C) Copyright 2010 Faraday Technology
 * Po-Yu Chuang <ratbert@faraday-tech.com>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __FTIIC010_H
#define __FTIIC010_H

#define FTIIC010_OFFSET_CR	0x00
#define FTIIC010_OFFSET_SR	0x04
#define FTIIC010_OFFSET_CDR	0x08
#define FTIIC010_OFFSET_DR	0x0c
#define FTIIC010_OFFSET_SAR	0x10
#define FTIIC010_OFFSET_TGSR	0x14
#define FTIIC010_OFFSET_BMR	0x18
#define FTIIC010_OFFSET_SMCR	0x1c
#define FTIIC010_OFFSET_MAXTR	0x20
#define FTIIC010_OFFSET_MINTR	0x24
#define FTIIC010_OFFSET_METR	0x28
#define FTIIC010_OFFSET_SETR	0x2c
#define FTIIC010_OFFSET_REV	0x30
#define FTIIC010_OFFSET_FEAT	0x34

/*
 * Control Register
 */
#define FTIIC010_CR_I2C_RST	(1 << 0)
#define FTIIC010_CR_I2C_EN	(1 << 1)
#define FTIIC010_CR_SCL_EN	(1 << 2)
#define FTIIC010_CR_GC_EN	(1 << 3)
#define FTIIC010_CR_START	(1 << 4)
#define FTIIC010_CR_STOP	(1 << 5)
#define FTIIC010_CR_NAK		(1 << 6)
#define FTIIC010_CR_TB_EN	(1 << 7)
#define FTIIC010_CR_DTI_EN	(1 << 8)
#define FTIIC010_CR_DRI_EN	(1 << 9)
#define FTIIC010_CR_BERRI_EN	(1 << 10)
#define FTIIC010_CR_STOPI_EN	(1 << 11)
#define FTIIC010_CR_SAMI_EN	(1 << 12)
#define FTIIC010_CR_ALI_EN	(1 << 13)	/* arbitration lost */
#define FTIIC010_CR_STARTI_EN	(1 << 14)
#define FTIIC010_CR_SCL_LOW	(1 << 15)
#define FTIIC010_CR_SDA_LOW	(1 << 16)
#define FTIIC010_CR_TESTMODE	(1 << 17)

/*
 * Status Register
 */
#define FTIIC010_SR_RW		(1 << 0)
#define FTIIC010_SR_ACK		(1 << 1)
#define FTIIC010_SR_I2CBUSY	(1 << 2)
#define FTIIC010_SR_BUSBUSY	(1 << 3)
#define FTIIC010_SR_DT		(1 << 4)
#define FTIIC010_SR_DR		(1 << 5)
#define FTIIC010_SR_BERR	(1 << 6)
#define FTIIC010_SR_STOP	(1 << 7)
#define FTIIC010_SR_SAM		(1 << 8)
#define FTIIC010_SR_GC		(1 << 9)
#define FTIIC010_SR_AL		(1 << 10)
#define FTIIC010_SR_START	(1 << 11)
#define FTIIC010_SR_SEXT	(1 << 12)
#define FTIIC010_SR_MEXT	(1 << 13)
#define FTIIC010_SR_MINTIMEOUT	(1 << 14)
#define FTIIC010_SR_MAXTIMEOUT	(1 << 15)
#define FTIIC010_SR_ALERT	(1 << 16)
#define FTIIC010_SR_SUSPEND	(1 << 17)
#define FTIIC010_SR_RESUME	(1 << 18)
#define FTIIC010_SR_ARA		(1 << 19)
#define FTIIC010_SR_DDA		(1 << 20)
#define FTIIC010_SR_SAL		(1 << 21)

/*
 * Clock Division Register
 */
#define FTIIC010_CDR_MASK	0x3ffff

/*
 * Data Register
 */
#define FTIIC010_DR_MASK	0xff

/*
 * Slave Address Register
 */
#define FTIIC010_SAR_MASK	0x3ff
#define FTIIC010_SAR_EN10	(1 << 31)

/*
 * Set/Hold Time Glitch Suppression Setting Register
 */
#define FTIIC010_TGSR_TSR(x)	((x) & 0x3ff)	/* should not be zero */
#define FTIIC010_TGSR_GSR(x)	(((x) & 0x7) << 10)

/*
 * Bus Monitor Register
 */
#define FTIIC010_BMR_SDA_IN	(1 << 0)
#define FTIIC010_BMR_SCL_IN	(1 << 1)

/*
 * SM Control Register
 */
#define FTIIC010_SMCR_SEXT_EN	(1 << 0)
#define FTIIC010_SMCR_MEXT_EN	(1 << 1)
#define FTIIC010_SMCR_TOUT_EN	(1 << 2)
#define FTIIC010_SMCR_ALERT_EN	(1 << 3)
#define FTIIC010_SMCR_SUS_EN	(1 << 4)
#define FTIIC010_SMCR_RSM_EN	(1 << 5)
#define FTIIC010_SMCR_SAL_EN	(1 << 8)
#define FTIIC010_SMCR_ALERT_LOW	(1 << 9)
#define FTIIC010_SMCR_SUS_LOW	(1 << 10)
#define FTIIC010_SMCR_SUSOUT_EN	(1 << 11)

/*
 * SM Maximum Timeout Register
 */
#define FTIIC010_MAXTR_MASK	0x3fffff

/*
 * SM Minimum Timeout Register
 */
#define FTIIC010_MINTR_MASK	0x3fffff

/*
 * SM Master Extend Time Register
 */
#define FTIIC010_METR_MASK	0xfffff

/*
 * SM Slave Extend Time Register
 */
#define FTIIC010_SETR_MASK	0x1fffff

/*
 * Feature Register
 */
#define FTIIC010_FEAT_SMBUS	(1 << 0)

#endif /* __FTIIC010_H */
