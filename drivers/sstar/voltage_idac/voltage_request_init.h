/*
 * voltage_request_init.h- Sigmastar
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
#ifndef __VOLTAGE_REQUEST_INIT_H
#define __VOLTAGE_REQUEST_INIT_H

enum idac_component
{
    IDAC_CP_VOLTAGE_STEP,
    IDAC_CP_STEP_CURRENT,
    IDAC_CP_SINK_SOURCE,
};


int  idac_set_voltage(const char *name, int base_vol, int setvol);
int  idac_set_voltage_reg(const char *name, int type, u16 value);
int  idac_get_voltage_reg(const char *name, int type);
int  idac_set_gpio_analog_mode(int gpio);
void idac_init_core(void);
void idac_init_cpu(void);
void idac_init_dla(void);
void idac_init_dsp(void);
int  idac_get_core_current_max_code(void);
int  idac_get_cpu_current_max_code(void);
int  idac_get_dla_current_max_code(void);
int  idac_get_dsp_current_max_code(void);
int  idac_get_core_max_vol_offset(void);
int  idac_get_cpu_max_vol_offset(void);
int  idac_get_dla_max_vol_offset(void);
int  idac_get_dsp_max_vol_offset(void);
int  voltage_request_chip(const char *name);

#endif //__VOLTAGE_REQUEST_INIT_H
