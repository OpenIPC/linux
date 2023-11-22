/*
 * memory.h- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
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

#ifndef _MEMORY_H_
#define _MEMORY_H_

#define SOC_LOW_PHYS_START 0x20000000ULL
#define SOC_LOW_PHYS_SIZE  0x80000000ULL /* 2G */
#define SOC_LOW_PHYS_END   (SOC_LOW_PHYS_START + SOC_LOW_PHYS_SIZE - 1)

#define SOC_HIGH_PHYS_START 0x1000000000ULL
#define SOC_HIGH_PHYS_SIZE  0x800000000ULL /* 32G */
#define SOC_HIGH_PHYS_END   (SOC_HIGH_PHYS_START + SOC_HIGH_PHYS_SIZE - 1)

#endif /* _MEMORY_H_ */
