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

// The register
#define REG_EN_IDAC_CORE       (BASE_REG_PMSAR_PA + REG_ID_73)
#define REG_IDAC_CURR_CORE_ADJ (BASE_REG_PMSAR_PA + REG_ID_71)
#define REG_EN_IDAC_CPU        (BASE_REG_PMSLEEP_PA + REG_ID_74)
#define REG_IDAC_CURR_CPU_ADJ  (BASE_REG_PMSAR1_PA + REG_ID_11)
#define REG_EN_IDAC_DLA        (BASE_REG_PMSLEEP_PA + REG_ID_73)
#define REG_IDAC_CURR_DLA_ADJ  (BASE_REG_PMSAR_PA + REG_ID_61)
#define REG_EN_IDAC_DSP        (BASE_REG_PMSLEEP_PA + REG_ID_74)
#define REG_IDAC_CURR_DSP_ADJ  (BASE_REG_PMSAR1_PA + REG_ID_01)

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
    int voloff = setvol - base_vol;
    U16 idac_s = idac_get_control_setting(voloff);

    if (!strcmp(name, "core_power"))
    {
        idac_s = (INREG16(REG_IDAC_CURR_CORE_ADJ) & 0xC0) | idac_s;
        OUTREG16(REG_IDAC_CURR_CORE_ADJ, idac_s);
    }
    else if (!strcmp(name, "cpu_power"))
    {
        idac_s = (INREG16(REG_IDAC_CURR_CPU_ADJ) & 0xC0) | idac_s;
        OUTREG16(REG_IDAC_CURR_CPU_ADJ, idac_s);
    }
    else if (!strcmp(name, "dla_power"))
    {
        idac_s = (INREG16(REG_IDAC_CURR_DLA_ADJ) & 0xC0) | idac_s;
        OUTREG16(REG_IDAC_CURR_DLA_ADJ, idac_s);
    }
    else if (!strcmp(name, "dsp_power"))
    {
        idac_s = (INREG16(REG_IDAC_CURR_DSP_ADJ) & 0xC0) | idac_s;
        OUTREG16(REG_IDAC_CURR_DSP_ADJ, idac_s);
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
    u16 idac_s;
    u16 mask;

    switch (type)
    {
        case IDAC_CP_VOLTAGE_STEP:
            mask = BIT3 | BIT2 | BIT1 | BIT0;
            break;
        case IDAC_CP_STEP_CURRENT:
            mask  = BIT5;
            value = value << 5;
            break;
        case IDAC_CP_SINK_SOURCE:
            mask  = BIT4;
            value = value << 4;
            break;
    }

    if (!strcmp(name, "core_power"))
    {
        idac_s = (INREG16(REG_IDAC_CURR_CORE_ADJ) & ~(mask)) | value;
        OUTREG16(REG_IDAC_CURR_CORE_ADJ, idac_s);
    }
    else if (!strcmp(name, "cpu_power"))
    {
        idac_s = (INREG16(REG_IDAC_CURR_CPU_ADJ) & ~(mask)) | value;
        OUTREG16(REG_IDAC_CURR_CPU_ADJ, idac_s);
    }
    else if (!strcmp(name, "dla_power"))
    {
        idac_s = (INREG16(REG_IDAC_CURR_DLA_ADJ) & ~(mask)) | value;
        OUTREG16(REG_IDAC_CURR_DLA_ADJ, idac_s);
    }
    else if (!strcmp(name, "dsp_power"))
    {
        idac_s = (INREG16(REG_IDAC_CURR_DSP_ADJ) & ~(mask)) | value;
        OUTREG16(REG_IDAC_CURR_DSP_ADJ, idac_s);
    }
    else
    {
        printk(KERN_ERR "[%s] Not Support!!\n", name);
        return -1;
    }

    return 0;
}

int idac_get_voltage_reg(const char *name, int type)
{
    u16 idac_s;

    if (!strcmp(name, "core_power"))
    {
        idac_s = INREG16(REG_IDAC_CURR_CORE_ADJ);
    }
    else if (!strcmp(name, "cpu_power"))
    {
        idac_s = INREG16(REG_IDAC_CURR_CPU_ADJ);
    }
    else if (!strcmp(name, "dla_power"))
    {
        idac_s = INREG16(REG_IDAC_CURR_DLA_ADJ);
    }
    else if (!strcmp(name, "dsp_power"))
    {
        idac_s = INREG16(REG_IDAC_CURR_DSP_ADJ);
    }
    else
    {
        printk(KERN_ERR "[%s] Not Support!!\n", name);
        return -1;
    }

    switch (type)
    {
        case IDAC_CP_VOLTAGE_STEP:
            idac_s = idac_s & (BIT3 | BIT2 | BIT1 | BIT0);
            break;
        case IDAC_CP_STEP_CURRENT:
            idac_s = (idac_s >> 5) & 0x1;
            break;
        case IDAC_CP_SINK_SOURCE:
            idac_s = (idac_s >> 4) & 0x1;
            break;
    }

    return idac_s;
}

int idac_set_gpio_analog_mode(int gpio)
{
    if (gpio == PAD_GPIO4)
    {
        SETREG16(BASE_REG_PADTOP_PA + REG_ID_40, BIT0);
    }
    else if (gpio == PAD_GPIO5)
    {
        SETREG16(BASE_REG_PADTOP_PA + REG_ID_40, BIT1);
    }
    else if (gpio == PAD_GPIO6)
    {
        SETREG16(BASE_REG_PADTOP_PA + REG_ID_40, BIT2);
    }
    else if (gpio == PAD_GPIO7)
    {
        SETREG16(BASE_REG_PADTOP_PA + REG_ID_40, BIT3);
    }
    else
    {
        printk(KERN_ERR "Failed to set gpio%d to analog mode!\n", gpio);
        return -1;
    }

    return 0;
}

void idac_init_core(void)
{
    // Step1.set 0x14_h7F[0]=1 and write back trim value
    SETREG16(BASE_REG_PMSAR_PA + REG_ID_7F, BIT0);
    // Step2. EN_IDAC
    OUTREG16(REG_EN_IDAC_CORE, 0x01);
    // Step3. Adjust CURR_TRIM[5:0] in SW mode
    SETREG16(REG_IDAC_CURR_CORE_ADJ, BIT6); // bit[6] reg_sw_mode, 1: enable to set idac current by SW
}

void idac_init_cpu(void)
{
    // Step1.set 0x14_h7F[0]=1 and write back trim value
    SETREG16(BASE_REG_PMSAR_PA + REG_ID_7F, BIT0);
    // Step2. EN_IDAC_CPU
    SETREG16(REG_EN_IDAC_CPU, BIT8);
    // Step3. Adjust CURR_TRIM[5:0] in SW mode
    SETREG16(REG_IDAC_CURR_CPU_ADJ, BIT6);
}

void idac_init_dla(void)
{
    // Step1.set 0x14_h7F[0]=1 and write back trim value
    SETREG16(BASE_REG_PMSAR_PA + REG_ID_7F, BIT0);
    // Step2. EN_IDAC_DLA
    SETREG16(REG_EN_IDAC_DLA, BIT0);
    // Step3. Adjust CURR_TRIM[5:0] in SW mode
    SETREG16(REG_IDAC_CURR_DLA_ADJ, BIT6);
}

void idac_init_dsp(void)
{
    // Step1.set 0x14_h7F[0]=1 and write back trim value
    SETREG16(BASE_REG_PMSAR_PA + REG_ID_7F, BIT0);
    // Step2. EN_IDAC_DSP
    SETREG16(REG_EN_IDAC_DSP, BIT0);
    // Step3. Adjust CURR_TRIM[5:0] in SW mode
    SETREG16(REG_IDAC_CURR_DSP_ADJ, BIT6);
}

int idac_get_core_current_max_code(void)
{
    return (INREG16(BASE_REG_OTP2_PA + REG_ID_52) & 0xF);
}

int idac_get_cpu_current_max_code(void)
{
    return ((INREG16(BASE_REG_OTP2_PA + REG_ID_52) >> 4) & 0xF);
}

int idac_get_dla_current_max_code(void)
{
    return ((INREG16(BASE_REG_OTP2_PA + REG_ID_52) >> 8) & 0xF);
}

int idac_get_dsp_current_max_code(void)
{
    return ((INREG16(BASE_REG_OTP2_PA + REG_ID_52) >> 12) & 0xF);
}

int idac_get_core_max_vol_offset(void)
{
    return 20 * (INREG16(BASE_REG_OTP2_PA + REG_ID_52) & 0xF); /* 1uA * R(20k) * max_code*/
}

int idac_get_cpu_max_vol_offset(void)
{
    return 20 * ((INREG16(BASE_REG_OTP2_PA + REG_ID_52) >> 4) & 0xF); /* 1uA * R(20k) * max_code*/
}

int idac_get_dla_max_vol_offset(void)
{
    return 20 * ((INREG16(BASE_REG_OTP2_PA + REG_ID_52) >> 8) & 0xF); /* 1uA * R(20k) * max_code*/
}

int idac_get_dsp_max_vol_offset(void)
{
    return 20 * ((INREG16(BASE_REG_OTP2_PA + REG_ID_52) >> 12) & 0xF); /* 1uA * R(20k) * max_code*/
}

int voltage_request_chip(const char *name)
{
    u32                 init_voltage;
    struct device_node *np = NULL;

    np = of_find_node_by_name(NULL, name);

    if (of_property_read_u32(np, "init_voltage", &init_voltage))
    {
        if (!strcmp(name, "core_power"))
        {
            set_core_voltage("core_power", VOLTAGE_DEMANDER_USER, VOLTAGE_CORE_1000);
        }
        else if (!strcmp(name, "cpu_power"))
        {
            set_core_voltage("cpu_power", VOLTAGE_DEMANDER_USER, VOLTAGE_CORE_1000);
        }
        else if (!strcmp(name, "dla_power"))
        {
            set_core_voltage("dla_power", VOLTAGE_DEMANDER_USER, VOLTAGE_CORE_1000);
        }
        else if (!strcmp(name, "dsp_power"))
        {
            set_core_voltage("dsp_power", VOLTAGE_DEMANDER_USER, VOLTAGE_CORE_1000);
        }
    }
    else
    {
        set_core_voltage("core_power", VOLTAGE_DEMANDER_TEMPERATURE, get_tt_voltage("core_power"));
        set_core_voltage("cpu_power", VOLTAGE_DEMANDER_TEMPERATURE, get_tt_voltage("cpu_power"));
        set_core_voltage("dla_power", VOLTAGE_DEMANDER_TEMPERATURE, get_tt_voltage("dla_power"));
        set_core_voltage("dsp_power", VOLTAGE_DEMANDER_TEMPERATURE, get_tt_voltage("dla_power"));
        set_core_voltage(name, VOLTAGE_DEMANDER_USER, init_voltage);
    }
    return 0;
}
