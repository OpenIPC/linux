/*
 * fh interrupt controller definitions
 *
 * Copyright (C) 2014 Fullhan Microelectronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef __ASM_ARCH_IRQS_H
#define __ASM_ARCH_IRQS_H

#include <mach/chip.h>

#define		REG_IRQ_EN_LOW				(INTC_REG_BASE + 0x0000)
#define		REG_IRQ_EN_HIGH				(INTC_REG_BASE + 0x0004)
#define		REG_IRQ_IRQMASK_LOW			(INTC_REG_BASE + 0x0008)
#define		REG_IRQ_IRQMASK_HIGH		(INTC_REG_BASE + 0x000C)
#define		REG_IRQ_IRQFORCE_LOW		(INTC_REG_BASE + 0x0010)
#define		REG_IRQ_IRQFORCE_HIGH		(INTC_REG_BASE + 0x0014)
#define		REG_IRQ_RAWSTATUS_LOW		(INTC_REG_BASE + 0x0018)
#define		REG_IRQ_RAWSTATUS_HIGH		(INTC_REG_BASE + 0x001C)
#define		REG_IRQ_STATUS_LOW			(INTC_REG_BASE + 0x0020)
#define		REG_IRQ_STATUS_HIGH			(INTC_REG_BASE + 0x0024)
#define		REG_IRQ_MASKSTATUS_LOW		(INTC_REG_BASE + 0x0028)
#define		REG_IRQ_MASKSTATUS_HIGH		(INTC_REG_BASE + 0x002C)
#define		REG_IRQ_FINSTATUS_LOW		(INTC_REG_BASE + 0x0030)
#define		REG_IRQ_FINSTATUS_HIGH		(INTC_REG_BASE + 0x0034)
#define		REG_FIQ_EN_LOW				(INTC_REG_BASE + 0x02C0)
#define		REG_FIQ_EN_HIGH				(INTC_REG_BASE + 0x02C4)
#define		REG_FIQ_FIQMASK_LOW			(INTC_REG_BASE + 0x02C8)
#define		REG_FIQ_FIQMASK_HIGH		(INTC_REG_BASE + 0x02CC)
#define		REG_FIQ_FIQFORCE_LOW		(INTC_REG_BASE + 0x02D0)
#define		REG_FIQ_FIQFORCE_HIGH		(INTC_REG_BASE + 0x02D4)
#define		REG_FIQ_RAWSTATUS_LOW		(INTC_REG_BASE + 0x02D8)
#define		REG_FIQ_RAWSTATUS_HIGH		(INTC_REG_BASE + 0x02DC)
#define		REG_FIQ_STATUS_LOW			(INTC_REG_BASE + 0x02E0)
#define		REG_FIQ_STATUS_HIGH			(INTC_REG_BASE + 0x02E4)
#define		REG_FIQ_FINSTATUS_LOW		(INTC_REG_BASE + 0x02E8)
#define		REG_FIQ_FINSTATUS_HIGH		(INTC_REG_BASE + 0x02EC)

#endif /* __ASM_ARCH_IRQS_H */
