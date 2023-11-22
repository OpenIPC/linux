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
#include "voltage_request_init.h"

#define REG_EN_IDAC_CORE       (BASE_REG_PMSAR_PA + REG_ID_73)
#define REG_IDAC_CURR_CORE_ADJ (BASE_REG_PMSAR_PA + REG_ID_71)

//####### IDAC VOLTAGE DEFINE FORMAT #########//
// R: 20k ohm
// BIT5:   0.5uA/1uA  (1/0)
#define IDAC_STEP_CUR_SHIFT  5
#define IDAC_STEP_CUR_MASK   BIT5
#define IDAC_STEP_CUR_SEL(n) (((n)&1) << IDAC_STEP_CUR_SHIFT)
#define IDAC_STEP_CUR_VAL(v) (((v)&IDAC_STEP_CUR_MASK) >> IDAC_STEP_CUR_SHIFT)
// BIT4:   Plus/Minus (1/0)
#define IDAC_SRC_SINK_SHIFT  4
#define IDAC_SRC_SINK_MASK   BIT4
#define IDAC_SRC_SINK_SEL(n) (((n)&1) << IDAC_SRC_SINK_SHIFT)
#define IDAC_SRC_SINK_VAL(v) (((v)&IDAC_SRC_SINK_MASK) >> IDAC_SRC_SINK_SHIFT)
// BIT3-0: Step	(0-F)
#define IDAC_STEP_MASK   (BIT3 | BIT2 | BIT1 | BIT0)
#define IDAC_STEP_VAL(v) ((v)&IDAC_STEP_MASK)

#ifndef CONFIG_SS_DUALOS
#define IDAC_TB_VALID_ROW 2
#define IDAC_TB_VALID_COL 31
static const U8 Idac_contrl_tb[IDAC_TB_VALID_ROW][IDAC_TB_VALID_COL] = {
    {0x00, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
     0x18, 0x18, 0x19, 0x19, 0x1A, 0x1A, 0x1B, 0x1B, 0x1C, 0x1C, 0x1D, 0x1D, 0x1E, 0x1E, 0x1F},
    {0x00, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
     0x08, 0x09, 0x09, 0x0A, 0x0A, 0x0B, 0x0B, 0x0C, 0x0C, 0x0D, 0x0D, 0x0E, 0x0E, 0x0F, 0x0F}};

static U16 idac_get_control_setting(int voloff)
{
    int row    = !!(voloff < 0);
    int column = abs(voloff) / 10; // 10mV per step

    if (column > IDAC_TB_VALID_COL)
    {
        printk(KERN_ERR "[%s] the column [%d] is invalid. reset column as 0\n", __FUNCTION__, column);
        column = 0;
    }

    return Idac_contrl_tb[row][column];
}

int idac_set_voltage(const char *name, int base_vol, int setvol)
{
    int voloff = setvol - base_vol;
    u16 mask   = IDAC_STEP_CUR_MASK | IDAC_SRC_SINK_MASK | IDAC_STEP_MASK;

    if (strcmp(name, "core_power"))
        return -1;

    OUTREGMSK16(REG_IDAC_CURR_CORE_ADJ, idac_get_control_setting(voloff), mask);

    return 0;
}

int idac_set_voltage_reg(const char *name, int type, u16 value)
{
    u16 mask = 0;

    if (strcmp(name, "core_power"))
        return -1;

    switch (type)
    {
        case IDAC_CP_VOLTAGE_STEP:
            mask  = IDAC_STEP_MASK;
            value = IDAC_STEP_VAL(value);
            break;
        case IDAC_CP_STEP_CURRENT:
            mask  = IDAC_STEP_CUR_MASK;
            value = IDAC_STEP_CUR_SEL(value);
            break;
        case IDAC_CP_SINK_SOURCE:
            mask  = IDAC_SRC_SINK_MASK;
            value = IDAC_SRC_SINK_SEL(value);
            break;
        default:
            return -1;
    }

    OUTREGMSK16(REG_IDAC_CURR_CORE_ADJ, value, mask);
    return 0;
}

int idac_get_voltage_reg(const char *name, int type)
{
    u16 idac_s;

    if (strcmp(name, "core_power"))
        return -1;

    idac_s = INREG16(REG_IDAC_CURR_CORE_ADJ);
    switch (type)
    {
        case IDAC_CP_VOLTAGE_STEP:
            return IDAC_STEP_VAL(idac_s);
        case IDAC_CP_STEP_CURRENT:
            return IDAC_STEP_CUR_VAL(idac_s);
        case IDAC_CP_SINK_SOURCE:
            return IDAC_SRC_SINK_VAL(idac_s);
    }

    return 0;
}

int idac_set_gpio_analog_mode(int gpio)
{
    if (gpio == PAD_PM_GPIO4)
    {
        SETREG16(BASE_REG_PADTOP_PA + REG_ID_40, BIT0);
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
    // reg_en_idac_cp
    OUTREG16(REG_EN_IDAC_CORE, 0x01);
    // SW_mode[6]
    SETREG16(REG_IDAC_CURR_CORE_ADJ, BIT6);
}

void idac_init_cpu(void) {}
void idac_init_dla(void) {}
void idac_init_dsp(void) {}
#endif

int idac_get_core_current_max_code(void)
{
    return 0;
}

int idac_get_cpu_current_max_code(void)
{
    return 0;
}

int idac_get_dla_current_max_code(void)
{
    return 0;
}

int idac_get_dsp_current_max_code(void)
{
    return 0;
}

int idac_get_core_max_vol_offset(void)
{
    return 0; /* 1uA * R(20k) * max_code */
}

int idac_get_cpu_max_vol_offset(void)
{
    return 0;
}

int idac_get_dla_max_vol_offset(void)
{
    return 0;
}

int idac_get_dsp_max_vol_offset(void)
{
    return 0;
}

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
