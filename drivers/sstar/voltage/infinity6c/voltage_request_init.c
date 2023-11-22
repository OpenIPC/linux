/*
 * voltage_request_init.c- Sigmastar
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
#include <linux/kernel.h>
#include <linux/of.h>
#include "registers.h"
#include "ms_platform.h"
#include "voltage_ctrl.h"
#include "voltage_ctrl_demander.h"

int voltage_request_chip(const char *name)
{
    u32                 init_voltage = VOLTAGE_CORE_1000;
    struct device_node *np           = NULL;

    if (strcmp(name, "core_power"))
        return -1;

    np = of_find_node_by_name(NULL, name);

    if (np && of_property_read_u32(np, "init_voltage", &init_voltage))
    {
        set_core_voltage("core_power", VOLTAGE_DEMANDER_INIT, VOLTAGE_CORE_1000);
    }
    else
    {
        set_core_voltage(name, VOLTAGE_DEMANDER_INIT, init_voltage);
    }
    /* DEMANDER_INIT is then not used any more */
    set_core_voltage(name, VOLTAGE_DEMANDER_INIT, 0);

    return 0;
}
