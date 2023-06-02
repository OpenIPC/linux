/*
 * arch/arm/plat-ambarella/include/plat/wdt.h
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

#ifndef __PLAT_AMBARELLA_WDT_H__
#define __PLAT_AMBARELLA_WDT_H__

/* ==========================================================================*/
#define WDOG_OFFSET			0xC000
#define WDOG_BASE			(APB_BASE + WDOG_OFFSET)
#define WDOG_REG(x)			(WDOG_BASE + (x))

/* ==========================================================================*/
#define WDOG_STATUS_OFFSET		0x00
#define WDOG_RELOAD_OFFSET		0x04
#define WDOG_RESTART_OFFSET		0x08
#define WDOG_CONTROL_OFFSET		0x0c
#define WDOG_TIMEOUT_OFFSET		0x10
#define WDOG_CLR_TMO_OFFSET		0x14
#define WDOG_RST_WD_OFFSET		0x18

#define WDOG_STATUS_REG			WDOG_REG(WDOG_STATUS_OFFSET)
#define WDOG_RELOAD_REG			WDOG_REG(WDOG_RELOAD_OFFSET)
#define WDOG_RESTART_REG		WDOG_REG(WDOG_RESTART_OFFSET)
#define WDOG_CONTROL_REG		WDOG_REG(WDOG_CONTROL_OFFSET)
#define WDOG_TIMEOUT_REG		WDOG_REG(WDOG_TIMEOUT_OFFSET)
#define WDOG_CLR_TMO_REG		WDOG_REG(WDOG_CLR_TMO_OFFSET)
#define WDOG_RST_WD_REG			WDOG_REG(WDOG_RST_WD_OFFSET)

#define WDOG_CTR_INT_EN			0x00000004
#define WDOG_CTR_RST_EN			0x00000002
#define WDOG_CTR_EN			0x00000001

/* WDOG_RESTART_REG only works with magic 0x4755.
 * Set this value would transferred the value in
 * WDOG_RELOAD_REG into WDOG_STATUS_REG and would
 * not trigger the underflow event. */
#define WDT_RESTART_VAL			0x4755

/* ==========================================================================*/

#endif /* __PLAT_AMBARELLA_WDT_H__ */

