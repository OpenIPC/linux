/*
* hal_ive_simulate.h- Sigmastar
*
* Copyright (C) 2018 Sigmastar Technology Corp.
*
* Author: chris.luo <chris.luo@sigmastar.com.tw>
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/
#ifndef _HAL_IVE_SIMULATE_H_
#define _HAL_IVE_SIMULATE_H_

#include <linux/kernel.h>
#include "hal_ive.h"

void ive_hal_run_simulate(ive_hal_handle *handle);

#endif // _HAL_IVE_SIMULATE_H_