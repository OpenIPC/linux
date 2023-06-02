/*
 * arch/arm/plat-ambarella/include/plat/pinctrl.h
 *
 * Author: Cao Rongrong <rrcao@ambarella.com>
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
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

#ifndef __PLAT_AMBARELLA_PINCTRL_H__
#define __PLAT_AMBARELLA_PINCTRL_H__

#ifndef __ASSEMBLER__

#define PINID_TO_BANK(p)	((p) >> 5)
#define PINID_TO_OFFSET(p)	((p) & 0x1f)

/* pins alternate function */
enum amb_pin_altfunc {
	AMB_ALTFUNC_GPIO = 0,
	AMB_ALTFUNC_HW_1,
	AMB_ALTFUNC_HW_2,
	AMB_ALTFUNC_HW_3,
	AMB_ALTFUNC_HW_4,
	AMB_ALTFUNC_HW_5,
};

#endif /* __ASSEMBLER__ */

#endif

