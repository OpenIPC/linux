/* SPDX-License-Identifier: GPL-2.0
 ****************************************************************************
 *
 *    The MIT License (MIT)
 *
 *    Copyright 2020 Verisilicon(Beijing) Co.,Ltd. All Rights Reserved.
 *
 *    Permission is hereby granted, free of charge, to any person obtaining a
 *    copy of this software and associated documentation files (the "Software"),
 *    to deal in the Software without restriction, including without limitation
 *    the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *    and/or sell copies of the Software, and to permit persons to whom the
 *    Software is furnished to do so, subject to the following conditions:
 *
 *    The above copyright notice and this permission notice shall be included in
 *    all copies or substantial portions of the Software.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *    DEALINGS IN THE SOFTWARE.
 *
 *****************************************************************************
 *
 *    The GPL License (GPL)
 *
 *    Copyright 2020 Verisilicon(Beijing) Co.,Ltd. All Rights Reserved.
 *
 *    This program is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU General Public License
 *    as published by the Free Software Foundation; either version 2
 *    of the License, or (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software Foundation,
 *    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *****************************************************************************
 *
 *    Note: This software is released under dual MIT and GPL licenses. A
 *    recipient may use this file under the terms of either the MIT license or
 *    GPL License. If you wish to use only one license not the other, you can
 *    indicate your decision by deleting one of the above license notices in your
 *    version of this file.
 *
 *****************************************************************************
 */

#ifndef __VC8000_VCMD_CFG_H__
#define __VC8000_VCMD_CFG_H__

#include "vc8000_driver.h"

/* Configure information with CMD, fill according to System Memory Map*/
/* Assume default video subsystem io-address base is 0x50100000 of win2030 */

/* Defines for Sub-System 0 */
#define VCMD_VENC_IO_ADDR_0                (0x10000) /* offset in win2030 video sub-system */
#define VCMD_VENC_IO_SIZE_0                (ASIC_VCMD_SWREG_AMOUNT * 4)
#define VCMD_VENC_INT_PIN_0                (-1)
#define VCMD_VENC_MODULE_TYPE_0            (0) /* vc8000e */
#define VCMD_VENC_MODULE_MAIN_ADDR_0       (0x1000)
#define VCMD_VENC_MODULE_DEC400_ADDR_0     (0XFFFF) /*0xffff means no such kind of submodule*/
#define VCMD_VENC_MODULE_L2CACHE_ADDR_0    (0XFFFF)
#define VCMD_VENC_MODULE_MMU0_ADDR_0       (0XFFFF)
#define VCMD_VENC_MODULE_MMU1_ADDR_0       (0XFFFF)
#define VCMD_VENC_MODULE_AXIFE0_ADDR_0     (0x2000)
#define VCMD_VENC_MODULE_AXIFE1_ADDR_0     (0XFFFF)

/* Defines for Sub-System 1 */
#define VCMD_JENC_IO_ADDR_1                (0x30000) /* offset in win2030 video sub-system */
#define VCMD_JENC_IO_SIZE_1                (ASIC_VCMD_SWREG_AMOUNT * 4)
#define VCMD_JENC_INT_PIN_1                (-1)
#define VCMD_JENC_MODULE_TYPE_1            (3) /* jpege */
#define VCMD_JENC_MODULE_MAIN_ADDR_1       (0x1000)
#define VCMD_JENC_MODULE_DEC400_ADDR_1     (0XFFFF)
#define VCMD_JENC_MODULE_L2CACHE_ADDR_1    (0XFFFF)
#define VCMD_JENC_MODULE_MMU0_ADDR_1       (0XFFFF)
#define VCMD_JENC_MODULE_MMU1_ADDR_1       (0XFFFF)
#define VCMD_JENC_MODULE_AXIFE0_ADDR_1     (0X2000)
#define VCMD_JENC_MODULE_AXIFE1_ADDR_1     (0XFFFF)

struct vcmd_config vc8000e_vcmd_core_array[] = {
	/* Sub-System 0 */
	{ VCMD_VENC_IO_ADDR_0,
	  VCMD_VENC_IO_SIZE_0,
	  VCMD_VENC_INT_PIN_0,
	  VCMD_VENC_MODULE_TYPE_0,
	  VCMD_VENC_MODULE_MAIN_ADDR_0,
	  VCMD_VENC_MODULE_DEC400_ADDR_0,
	  VCMD_VENC_MODULE_L2CACHE_ADDR_0,
	  { VCMD_VENC_MODULE_MMU0_ADDR_0,
	  VCMD_VENC_MODULE_MMU1_ADDR_0 },
	  { VCMD_VENC_MODULE_AXIFE0_ADDR_0,
	  VCMD_VENC_MODULE_AXIFE1_ADDR_0 }
	},
	/* Sub-System 1 */
	{ VCMD_JENC_IO_ADDR_1,
	  VCMD_JENC_IO_SIZE_1,
	  VCMD_JENC_INT_PIN_1,
	  VCMD_JENC_MODULE_TYPE_1,
	  VCMD_JENC_MODULE_MAIN_ADDR_1,
	  VCMD_JENC_MODULE_DEC400_ADDR_1,
	  VCMD_JENC_MODULE_L2CACHE_ADDR_1,
	  { VCMD_JENC_MODULE_MMU0_ADDR_1,
	  VCMD_JENC_MODULE_MMU1_ADDR_1 },
	  { VCMD_JENC_MODULE_AXIFE0_ADDR_1,
	  VCMD_JENC_MODULE_AXIFE1_ADDR_1 }
	},
	{ VCMD_VENC_IO_ADDR_0,
	  VCMD_VENC_IO_SIZE_0,
	  VCMD_VENC_INT_PIN_0,
	  VCMD_VENC_MODULE_TYPE_0,
	  VCMD_VENC_MODULE_MAIN_ADDR_0,
	  VCMD_VENC_MODULE_DEC400_ADDR_0,
	  VCMD_VENC_MODULE_L2CACHE_ADDR_0,
	  { VCMD_VENC_MODULE_MMU0_ADDR_0,
	  VCMD_VENC_MODULE_MMU1_ADDR_0 },
	  { VCMD_VENC_MODULE_AXIFE0_ADDR_0,
	  VCMD_VENC_MODULE_AXIFE1_ADDR_0 }
	},
	{ VCMD_JENC_IO_ADDR_1,
	  VCMD_JENC_IO_SIZE_1,
	  VCMD_JENC_INT_PIN_1,
	  VCMD_JENC_MODULE_TYPE_1,
	  VCMD_JENC_MODULE_MAIN_ADDR_1,
	  VCMD_JENC_MODULE_DEC400_ADDR_1,
	  VCMD_JENC_MODULE_L2CACHE_ADDR_1,
	  { VCMD_JENC_MODULE_MMU0_ADDR_1,
	  VCMD_JENC_MODULE_MMU1_ADDR_1 },
	  { VCMD_JENC_MODULE_AXIFE0_ADDR_1,
	  VCMD_JENC_MODULE_AXIFE1_ADDR_1 }
	},

};

#endif /*__VC8000_VCMD_CFG_H__ */
