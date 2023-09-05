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

#define VOLTAGE_CORE_850   850
#define VOLTAGE_CORE_900   900
#define VOLTAGE_CORE_950   950
#define VOLTAGE_CORE_1000 1000


#define FOREACH_DEMANDER(DEMANDER) \
            DEMANDER(VOLTAGE_DEMANDER_CPUFREQ)      \
            DEMANDER(VOLTAGE_DEMANDER_TEMPERATURE)  \
            DEMANDER(VOLTAGE_DEMANDER_VENC)         \
            DEMANDER(VOLTAGE_DEMANDER_MIU)          \
            DEMANDER(VOLTAGE_DEMANDER_USER)         \
            DEMANDER(VOLTAGE_DEMANDER_MAX)          \

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

typedef enum {
    FOREACH_DEMANDER(GENERATE_ENUM)
} VOLTAGE_DEMANDER_E;

void set_core_voltage(VOLTAGE_DEMANDER_E demander, int mV);
int  get_core_voltage(void);
int  core_voltage_available(unsigned int **voltages, unsigned int *num);
int  core_voltage_pin(unsigned int **pins, unsigned int *num);

#endif  //__VOLTAGE_CTRL_H
