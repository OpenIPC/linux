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
#include "gpio.h"

//####### IDAC VOLTAGE DEFINE FORMAT #########//
// BIT5:   0.5uA/1uA  (1/0)
// BIT4:   Plus/Minus (1/0)
// BIT3-0: Step	(0-F)
// R: 20k ohm
#define IDAC_TB_VALID_ROW 2
#define IDAC_TB_VALID_COL 31
const U8 Idac_contrl_tb[IDAC_TB_VALID_ROW][IDAC_TB_VALID_COL] = {
    {0x00, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
     0x18, 0x18, 0x19, 0x19, 0x1A, 0x1A, 0x1B, 0x1B, 0x1C, 0x1C, 0x1D, 0x1D, 0x1E, 0x1E, 0x1F},
    {0x00, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
     0x08, 0x09, 0x09, 0x0A, 0x0A, 0x0B, 0x0B, 0x0C, 0x0C, 0x0D, 0x0D, 0x0E, 0x0E, 0x0F, 0x0F}};

U16 idac_get_control_setting(int voloff)
{
    int row    = (voloff < 0) ? 1 : 0;
    int column = abs(voloff) / 10;

    if (column > IDAC_TB_VALID_COL)
    {
        printk(KERN_ERR "[%s] the column [%d] is invalid. reset column as 0\n", __FUNCTION__, column);
        column = 0;
    }

    return Idac_contrl_tb[row][column];
}

int idac_set_voltage(const char *name, int base_vol, int setvol)
{
    U16 idac_s = 0;
    int voloff = setvol - base_vol;

    if (!strcmp(name, "core_power"))
    {
        idac_s = (INREG16(BASE_REG_PMSAR_PA + REG_ID_71) & 0xC0) | idac_get_control_setting(voloff);
        OUTREG16(BASE_REG_PMSAR_PA + REG_ID_71, idac_s);
    }
    else if (!strcmp(name, "cpu_power")) // cpu + dla
    {
        idac_s = (INREG16(BASE_REG_PMSAR_PA + REG_ID_61) & 0xC0) | idac_get_control_setting(voloff);
        OUTREG16(BASE_REG_PMSAR_PA + REG_ID_61, idac_s);
    }
    else
    {
        printk(KERN_ERR "[%s] Not Support!!\n", name);
        return -1;
    }

    return 0;
}

int idac_set_voltage_reg(const char *name, int type, u16 value)
{
    return 0;
}
int idac_get_voltage_reg(const char *name, int type)
{
    return 0;
}

int idac_set_gpio_analog_mode(int gpio)
{
    if (gpio == PAD_PM_GPIO6)
    {
        SETREG16(BASE_REG_PMPADTOP_PA + REG_ID_64, BIT0);
    }
    /*else if(gpio == PAD_PM_GPIO8)
    {
       SETREG16(BASE_REG_PMPADTOP_PA + REG_ID_64, BIT1);
    }*/
    else if (gpio == PAD_PM_GPIO4)
    {
        SETREG16(BASE_REG_PMPADTOP_PA + REG_ID_64, BIT2);
    }
    else if (gpio == PAD_PM_GPIO5)
    {
        SETREG16(BASE_REG_PMPADTOP_PA + REG_ID_64, BIT3);
    }
    else
    {
        printk(KERN_ERR "Failed to set gpio to analgo mode!!\n");
        return -1;
    }

    return 0;
}

void idac_init_core(void)
{
    // Step1.set 0x14_h7F[0]=1 and write back trim value
    SETREG16(BASE_REG_PMSAR_PA + REG_ID_7F, BIT0);
    // Step2. reg_en_idac_cp
    OUTREG16(BASE_REG_PMSAR_PA + REG_ID_73, 0x01);
    // Step3. Adjust CURR_TRIM[5:0] SW_mode[6]
    SETREG16(BASE_REG_PMSAR_PA + REG_ID_71, BIT6);
}

void idac_init_cpu(void) {}

void idac_init_dla(void) /* cpu + dla */
{
    // Step1.set 0x14_h7F[0]=1 and write back trim value
    SETREG16(BASE_REG_PMSAR_PA + REG_ID_7F, BIT0);
    // Step2. IDAC_DLA
    SETREG16(BASE_REG_PMSLEEP_PA + REG_ID_73, BIT0);
    // Step3. Adjust CURR_TRIM[5:0] SW_mode[6]
    SETREG16(BASE_REG_PMSAR_PA + REG_ID_61, BIT6);
}

void idac_init_dsp(void) {}

int idac_get_core_current_max_code(void)
{
    return (INREG16(BASE_REG_OTP2_PA + REG_ID_5C) & 0xF);
}

int idac_get_cpu_current_max_code(void)
{
    return 0;
}

int idac_get_dla_current_max_code(void)
{
    return ((INREG16(BASE_REG_OTP2_PA + REG_ID_5C) >> 4) & 0xF);
}

int idac_get_dsp_current_max_code(void)
{
    return 0;
}

int idac_get_core_max_vol_offset(void)
{
    return 20 * (INREG16(BASE_REG_OTP2_PA + REG_ID_5C) & 0xF); /* 1uA * R(20k) * max_code*/
}

int idac_get_cpu_max_vol_offset(void)
{
    return 0;
}

int idac_get_dla_max_vol_offset(void)
{
    return 20 * ((INREG16(BASE_REG_OTP2_PA + REG_ID_5C) >> 4) & 0xF); /* 1uA * R(20k) * max_code*/
}

int idac_get_dsp_max_vol_offset(void)
{
    return 0;
}

int voltage_request_chip(const char *name)
{
    u32                 init_voltage;
    struct device_node *np = NULL;

    np = of_find_node_by_name(NULL, name);

    if (of_property_read_u32(np, "init_voltage", &init_voltage))
    {
        if (!strcmp(name, "cpu_power"))
        {
            set_core_voltage("cpu_power", VOLTAGE_DEMANDER_USER, VOLTAGE_CORE_1000);
        }
        else if (!strcmp(name, "core_power"))
        {
            set_core_voltage("core_power", VOLTAGE_DEMANDER_USER, VOLTAGE_CORE_1000);
        }
        else if (!strcmp(name, "dla_power"))
        {
            set_core_voltage("dla_power", VOLTAGE_DEMANDER_USER, VOLTAGE_CORE_1000);
        }
    }
    else
    {
        set_core_voltage("cpu_power", VOLTAGE_DEMANDER_TEMPERATURE, get_tt_voltage("cpu_power"));
        set_core_voltage("core_power", VOLTAGE_DEMANDER_TEMPERATURE, get_tt_voltage("core_power"));
        set_core_voltage("dla_power", VOLTAGE_DEMANDER_TEMPERATURE, get_tt_voltage("dla_power"));

        set_core_voltage(name, VOLTAGE_DEMANDER_USER, init_voltage);
    }
    return 0;
}
