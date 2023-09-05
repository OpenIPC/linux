/*
* voltage_ctrl.h- Sigmastar
*
* Copyright (C) 2018 Sigmastar Technology Corp.
*
* Author: karl.xiao <karl.xiao@sigmastar.com.tw>
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
#ifndef __VOLTAGE_CTRL_H
#define __VOLTAGE_CTRL_H

#include "../vcore_defs.h"

typedef enum {
    VOLTAGE_DEMANDER_CPUFREQ = 0,
    VOLTAGE_DEMANDER_TEMPERATURE,
    VOLTAGE_DEMANDER_VENC,
    VOLTAGE_DEMANDER_MAX,
    VOLTAGE_DEMANDER_INIT,
} VOLTAGE_DEMANDER_E;

typedef struct {
    u32 vcore;
    u32 gpio_ctrls[VID_MAX_GPIO_CNT];
} stVcoreCtrlTableEntry;

typedef struct {
    u8 gpio_num;
    u8 gpios[VID_MAX_GPIO_CNT];
    stVcoreCtrlTableEntry table[(1<<VID_MAX_GPIO_CNT)+1];
} stVcoreCtrlTable;

//void set_core_voltage(VOLTAGE_DEMANDER_E factor, int vcore);

#endif  //__VOLTAGE_CTRL_H
