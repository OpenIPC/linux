/*
 * sstar-dwc3-debugfs.h- Sigmastar
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
#ifndef __STAR_DWC3_DEBUGFS_H__
#define __STAR_DWC3_DEBUGFS_H__
#include "sstar-dwc3-of-simple.h"

void sstar_dwc3_debugfs_init(struct dwc3_of_simple *of_simple);
void sstar_dwc3_debugfs_exit(struct dwc3_of_simple *of_simple);
#endif
