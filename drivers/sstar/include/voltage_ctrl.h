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

#include "voltage_ctrl_demander.h"

#define VOLTAGE_CORE_850   850
#define VOLTAGE_CORE_900   900
#define VOLTAGE_CORE_950   950
#define VOLTAGE_CORE_1000 1000

#ifdef CONFIG_SS_VOLTAGE_CTRL_WITH_SIDD
int sync_core_voltage_with_SIDD_and_TEMP(int useTT);
#endif
void set_core_voltage(VOLTAGE_DEMANDER_E demander, int mV);
int  get_core_voltage(void);
int  core_voltage_available(unsigned int **voltages, unsigned int *num);
int  core_voltage_pin(unsigned int **pins, unsigned int *num);

#endif  //__VOLTAGE_CTRL_H
