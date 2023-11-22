/*
 * io.h- Sigmastar
 *
 * Copyright (c) [2019~2021] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */

#ifndef __IO_H__
#define __IO_H__
#include <linux/io.h>

#define INREG16(x)     readw((x))
#define OUTREG16(x, y) writew((y), (x))
#define CLRREG16(x, y) OUTREG16((x), INREG16(x) & ~(y))
#define SETREG16(x, y) OUTREG16((x), INREG16(x) | (y))

#define INREG8(x)     readb((x))
#define OUTREG8(x, y) writeb((y), (x))
#define CLRREG8(x, y) OUTREG8((x), INREG8(x) & ~(y))
#define SETREG8(x, y) OUTREG8((x), INREG8(x) | (y))

#define GET_REG8_ADDR(x, y) ((x) + ((y) << 1) - ((y)&1))

#endif
