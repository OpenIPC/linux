/*
 * voltage_ctrl.h- Sigmastar
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
#ifndef __VOLTAGE_CTRL_H
#define __VOLTAGE_CTRL_H

#include "voltage_ctrl_demander.h"

#ifdef CONFIG_SS_DUALOS
#define INTEROS_SC_L2R_CORE_VOLTAGE_SET  0xF1020000
#define INTEROS_SC_L2R_CORE_VOLTAGE_GET  0xF1020001
#define INTEROS_SC_L2R_CORE_VOLTAGE_SYNC 0xF1020002
#endif

#define VOLTAGE_CORE_INIT 0
#define VOLTAGE_CORE_860  860
#define VOLTAGE_CORE_890  890
#define VOLTAGE_CORE_920  920
#define VOLTAGE_CORE_980  980
#define VOLTAGE_CORE_940  940
#define VOLTAGE_CORE_950  950
#define VOLTAGE_CORE_1000 1000
#define VOLTAGE_CORE_1020 1020

#ifdef CONFIG_SS_VOLTAGE_CTRL_WITH_OSC
int sync_core_voltage_with_TEMP(const char *name, int useTT);
#endif

#ifdef CONFIG_SS_VOLTAGE_IDAC_CTRL
int get_lv_voltage(const char *name);
int get_tt_voltage(const char *name);
int sync_voltage_with_OSCandTemp(const char *name, int useTT);
#endif

int get_voltage_width(const char *name, unsigned int *num);
int set_core_voltage(const char *name, VOLTAGE_DEMANDER_E demander, int mV);
int get_core_voltage(const char *name, int *mV);
int get_core_lt_voltage(const char *name, int *mV);
int core_voltage_available(const char *name, unsigned int **voltages, unsigned int *num);
int core_voltage_pin(const char *name, unsigned int **pins, unsigned int *num);

#endif //__VOLTAGE_CTRL_H
