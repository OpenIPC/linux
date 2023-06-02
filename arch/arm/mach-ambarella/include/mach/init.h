/*
 * arch/arm/plat-ambarella/include/mach/init.h
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

#ifndef __ASM_ARCH_INIT_H
#define __ASM_ARCH_INIT_H

/* ==========================================================================*/
#include <plat/gpio.h>
#include <plat/adc.h>
#include <plat/audio.h>
#include <plat/crypto.h>
#include <plat/dma.h>
#include <plat/eth.h>
#include <plat/event.h>
#include <plat/fio.h>
#include <plat/idc.h>
#include <plat/ir.h>
#include <plat/irq.h>
#include <plat/nand.h>
#include <plat/clk.h>
#include <plat/pm.h>
#include <plat/pwm.h>
#include <plat/rtc.h>
#include <plat/sd.h>
#include <plat/spi.h>
#include <plat/timer.h>
#include <plat/uart.h>
#include <plat/udc.h>
#include <plat/ahci.h>
#include <plat/wdt.h>
#include <plat/pinctrl.h>

/* ==========================================================================*/
#ifndef __ASSEMBLER__

#define AMBARELLA_BOARD_TYPE_AUTO		(0)
#define AMBARELLA_BOARD_TYPE_BUB		(1)
#define AMBARELLA_BOARD_TYPE_EVK		(2)
#define AMBARELLA_BOARD_TYPE_IPCAM		(3)
#define AMBARELLA_BOARD_TYPE_VENDOR		(4)
#define AMBARELLA_BOARD_TYPE_ATB		(5)

#define AMBARELLA_BOARD_VERSION(c,t,r)		(((c) << 16) + ((t) << 12) + (r))
#define AMBARELLA_BOARD_CHIP(v)			(((v) >> 16) & 0xFFFF)
#define AMBARELLA_BOARD_TYPE(v)			(((v) >> 12) & 0xF)
#define AMBARELLA_BOARD_REV(v)			(((v) >> 0) & 0xFFF)

/* ==========================================================================*/
extern void ambarella_init_early(void);
extern void ambarella_init_machine(void);
extern void ambarella_map_io(void);
extern void ambarella_restart_machine(char mode, const char *cmd);

/* ==========================================================================*/
extern int ambarella_init_fb(void);

/* ==========================================================================*/
extern u32 ambarella_phys_to_virt(u32 paddr);
extern u32 ambarella_virt_to_phys(u32 vaddr);

extern u32 get_ambarella_iavmem_phys(void);
extern u32 get_ambarella_iavmem_size(void);

extern u32 get_ambarella_fbmem_phys(void);
extern u32 get_ambarella_fbmem_size(void);

extern u32 get_ambarella_ppm_phys(void);
extern u32 get_ambarella_ppm_virt(void);
extern u32 get_ambarella_ppm_size(void);

extern u32 get_ambarella_ahb_phys(void);
extern u32 get_ambarella_ahb_virt(void);
extern u32 get_ambarella_ahb_size(void);

extern u32 get_ambarella_apb_phys(void);
extern u32 get_ambarella_apb_virt(void);
extern u32 get_ambarella_apb_size(void);

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif

