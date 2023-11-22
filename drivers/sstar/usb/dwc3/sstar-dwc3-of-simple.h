/*
 * sstar-dwc3-of-simple.h- Sigmastar
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

#ifndef __SSTAR_DWC3_OF_SIMPLE_H__
#define __SSTAR_DWC3_OF_SIMPLE_H__

struct dwc3_of_simple
{
    struct device*        dev;
    struct clk_bulk_data* clks;
    int                   num_clocks;
    struct dwc3*          dwc;
    struct dentry*        root;
    spinlock_t            lock;
};

#endif
