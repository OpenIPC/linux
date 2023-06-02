/*
	Pinmux module driver.

	This file is the driver of Piumux module.

	@file		na51055_pinmux_host.c
	@ingroup
	@note		Nothing.

	Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/
#include "na51055_pinmux.h"

static spinlock_t top_lock;
#define loc_cpu(lock, flags) spin_lock_irqsave(lock, flags)
#define unl_cpu(lock, flags) spin_unlock_irqrestore(lock, flags)

static uint32_t mipi_lvds_sensor[2] = {0, 0};
static int mclk_sensor[3] = {0, 0, 0};
static int mclk2_sensor[2][2] = {{0, 0}, {0, 0}};
static int sn_mclk_en[2] = {0,0};
static uint32_t lcd_mask = 0x0;
static uint32_t lgpio_mask = 0x1ffffff;  /*LCD0 ~ LCD24*/

typedef int (*PINMUX_CONFIG_HDL)(uint32_t);

static int pinmux_config_sdio(uint32_t config);
static int pinmux_config_sdio2(uint32_t config);
static int pinmux_config_sdio3(uint32_t config);
static int pinmux_config_nand(uint32_t config);
static int pinmux_config_sensor(uint32_t config);
static int pinmux_config_sensor2(uint32_t config);
static int pinmux_config_mipi_lvds(uint32_t config);
static int pinmux_config_i2c(uint32_t config);
static int pinmux_config_sif(uint32_t config);
static int pinmux_config_uart(uint32_t config);
static int pinmux_config_spi(uint32_t config);
static int pinmux_config_sdp(uint32_t config);
static int pinmux_config_remote(uint32_t config);
static int pinmux_config_pwm(uint32_t config);
static int pinmux_config_pwm2(uint32_t config);
static int pinmux_config_ccnt(uint32_t config);
static int pinmux_config_audio(uint32_t config);
static int pinmux_config_lcd(uint32_t config);
static int pinmux_config_tv(uint32_t config);
static int pinmux_config_eth(uint32_t config);
static int pinmux_config_misc(uint32_t config);

union TOP_REG0 top_reg0;
union TOP_REG1 top_reg1;
union TOP_REG2 top_reg2;
union TOP_REG3 top_reg3;
union TOP_REG4 top_reg4;
union TOP_REG5 top_reg5;
union TOP_REG6 top_reg6;
union TOP_REG7 top_reg7;
union TOP_REG8 top_reg8;
union TOP_REG9 top_reg9;
union TOP_REGCGPIO0 top_reg_cgpio0;
union TOP_REGPGPIO0 top_reg_pgpio0;
union TOP_REGSGPIO0 top_reg_sgpio0;
union TOP_REGLGPIO0 top_reg_lgpio0;
union TOP_REGDGPIO0 top_reg_dgpio0;
union TOP_REGHGPIO0 top_reg_hgpio0;

static uint32_t disp_pinmux_config[] =
{
	PINMUX_DISPMUX_SEL_NONE,            // LCD
	PINMUX_DISPMUX_SEL_NONE,            // LCD2
	PINMUX_TV_HDMI_CFG_NORMAL,          // TV
	PINMUX_TV_HDMI_CFG_NORMAL           // HDMI
};

static PINMUX_CONFIG_HDL pinmux_config_hdl[] =
{
	pinmux_config_sdio,
	pinmux_config_sdio2,
	pinmux_config_sdio3,
	pinmux_config_nand,
	pinmux_config_sensor,
	pinmux_config_sensor2,
	pinmux_config_mipi_lvds,
	pinmux_config_i2c,
	pinmux_config_sif,
	pinmux_config_uart,
	pinmux_config_spi,
	pinmux_config_sdp,
	pinmux_config_remote,
	pinmux_config_pwm,
	pinmux_config_pwm2,
	pinmux_config_ccnt,
	pinmux_config_audio,
	pinmux_config_lcd,
	pinmux_config_tv,
	pinmux_config_eth,
	pinmux_config_misc
};

struct nvt_pinctrl_info info_get_id[1] = {0};
#include <linux/of.h>
uint32_t nvt_get_chip_id(void)
{
	union TOP_VERSION_REG top_version;
	struct device_node *top;
	static u32 top_reg_addr = 0;
	u32 value[6] = {};

	if (!top_reg_addr) {
		top = of_find_compatible_node(NULL, NULL, "nvt,nvt_top");
		if (top) {
			if (!of_property_read_u32_array(top, "reg", value, 6))
				top_reg_addr = (u32) ioremap_nocache(value[0], 0x10);
			else {
				pr_err("*** %s not get top reg ***\n", __func__);
				return -ENOMEM;
			}
		} else {
			pr_err("*** %s not get dts node ***\n", __func__);
			return -ENOMEM;
		}
	}

	if (top_reg_addr) {
		info_get_id->top_base = (void *) top_reg_addr;
		top_version.reg = TOP_GETREG(info_get_id, TOP_VERSION_REG_OFS);
	} else {
		pr_err("invalid pinmux address\n");
		return -ENOMEM;
	}

	return top_version.bit.CHIP_ID;
}
EXPORT_SYMBOL(nvt_get_chip_id);

static int logo_determination(u32 *lcd_type)
{
	u32 m_logo = 0x0;
	struct device_node* of_node = of_find_node_by_path("/logo");

	if (of_node) {
        of_property_read_u32(of_node, "enable", &m_logo);

        of_property_read_u32(of_node, "lcd_type", lcd_type);
	}

	return m_logo;
}


static void gpio_info_show(struct nvt_pinctrl_info *info, u32 gpio_number, u32 start_offset)
{
	int i = 0, j = 0;
	u32 reg_value;
	char* gpio_name[] = {"C_GPIO", "P_GPIO", "S_GPIO", "L_GPIO", "D_GPIO", "H_GPIO", "A_GPIO"};
	char name[10];

	if (start_offset == TOP_REGCGPIO0_OFS)
		strcpy(name, gpio_name[0]);
	else if (start_offset == TOP_REGPGPIO0_OFS)
		strcpy(name, gpio_name[1]);
	else if (start_offset == TOP_REGSGPIO0_OFS)
		strcpy(name, gpio_name[2]);
	else if (start_offset == TOP_REGLGPIO0_OFS)
		strcpy(name, gpio_name[3]);
	else if (start_offset == TOP_REGDGPIO0_OFS)
		strcpy(name, gpio_name[4]);
	else if (start_offset == TOP_REGHGPIO0_OFS)
		strcpy(name, gpio_name[5]);
	else if (start_offset == TOP_REGAGPIO0_OFS)
		strcpy(name, gpio_name[6]);


	if (gpio_number > 0x20) {
		reg_value = TOP_GETREG(info, start_offset);

		for (i = 0; i < 0x20; i++) {
			if (reg_value & (1 << i))
				pr_info("%s%-2d       GPIO\n", name, i);
			else
				pr_info("%s%-2d     FUNCTION\n", name, i);
		}

		reg_value = TOP_GETREG(info, start_offset + 0x4);

		for (j = 0; j < (gpio_number - 0x20); j++) {
			if (reg_value & (1 << j))
				pr_info("%s%-2d       GPIO\n", name, i);
			else
				pr_info("%s%-2d     FUNCTION\n", name, i);
			i++;
		}
	} else {
		reg_value = TOP_GETREG(info, start_offset);
		for (i = 0; i < gpio_number; i++) {
			if (reg_value & (1 << i))
				pr_info("%s%-2d       GPIO\n", name, i);
			else
				pr_info("%s%-2d     FUNCTION\n", name, i);
		}
	}

}

void pinmux_gpio_parsing(struct nvt_pinctrl_info *info)
{
	pr_info("\n  PIN         STATUS\n");

	gpio_info_show(info, C_GPIO_NUM, TOP_REGCGPIO0_OFS);
	gpio_info_show(info, P_GPIO_NUM, TOP_REGPGPIO0_OFS);
	gpio_info_show(info, S_GPIO_NUM, TOP_REGSGPIO0_OFS);
	gpio_info_show(info, L_GPIO_NUM, TOP_REGLGPIO0_OFS);
	gpio_info_show(info, D_GPIO_NUM, TOP_REGDGPIO0_OFS);
	gpio_info_show(info, H_GPIO_NUM, TOP_REGHGPIO0_OFS);
	gpio_info_show(info, A_GPIO_NUM, TOP_REGAGPIO0_OFS);
}

void pinmux_preset(struct nvt_pinctrl_info *info)
{
	spin_lock_init(&top_lock);
}


/**
	Get Display PINMUX setting

	Get Display PINMUX setting.
	Display driver (LCD/TV/HDMI) can get mode setting from pinmux_init()

	@param[in] id   LCD ID
			- @b PINMUX_DISP_ID_LCD: 1st LCD
			- @b PINMUX_DISP_ID_LCD2: 2nd LCD
			- @b PINMUX_DISP_ID_TV: TV
			- @b PINMUX_DISP_ID_HDMI: HDMI

	@return LCD pinmux setting
*/
PINMUX_LCDINIT pinmux_get_dispmode(PINMUX_FUNC_ID id)
{
	if (id <= PINMUX_FUNC_ID_LCD2) {
		return disp_pinmux_config[id] & ~(PINMUX_DISPMUX_SEL_MASK | PINMUX_PMI_CFG_MASK | PINMUX_LCD_SEL_FEATURE_MSK);
	} else if (id <= PINMUX_FUNC_ID_HDMI) {
		return disp_pinmux_config[id] & ~PINMUX_HDMI_CFG_MASK;
	}

	return 0;
}
EXPORT_SYMBOL(pinmux_get_dispmode);

/**
	Read pinmux data from controller base

	Read pinmux data from controller base

	@param[in] info	nvt_pinctrl_info
*/

void pinmux_parsing(struct nvt_pinctrl_info *info)
{
	u32 value;
	unsigned long flags = 0;
	union TOP_REG0 local_top_reg0;
	union TOP_REG1 local_top_reg1;
	union TOP_REG2 local_top_reg2;
	union TOP_REG3 local_top_reg3;
	union TOP_REG4 local_top_reg4;
	union TOP_REG5 local_top_reg5;
	union TOP_REG6 local_top_reg6;
	union TOP_REG7 local_top_reg7;
	union TOP_REG8 local_top_reg8;
	union TOP_REG9 local_top_reg9;
	union TOP_REGSGPIO0 local_top_reg_sgpio0;
	union TOP_REGHGPIO0 local_top_reg_hgpio0;
	union TOP_REGDGPIO0 local_top_reg_dgpio0;
	union TOP_REGPGPIO0 local_top_reg_pgpio0;

	loc_cpu(&top_lock, flags);

	local_top_reg0.reg = TOP_GETREG(info, TOP_REG0_OFS);
	local_top_reg1.reg = TOP_GETREG(info, TOP_REG1_OFS);
	local_top_reg2.reg = TOP_GETREG(info, TOP_REG2_OFS);
	local_top_reg3.reg = TOP_GETREG(info, TOP_REG3_OFS);
	local_top_reg4.reg = TOP_GETREG(info, TOP_REG4_OFS);
	local_top_reg5.reg = TOP_GETREG(info, TOP_REG5_OFS);
	local_top_reg6.reg = TOP_GETREG(info, TOP_REG6_OFS);
	local_top_reg7.reg = TOP_GETREG(info, TOP_REG7_OFS);
	local_top_reg8.reg = TOP_GETREG(info, TOP_REG8_OFS);
	local_top_reg9.reg = TOP_GETREG(info, TOP_REG9_OFS);
	local_top_reg_sgpio0.reg = TOP_GETREG(info, TOP_REGSGPIO0_OFS);
	local_top_reg_hgpio0.reg = TOP_GETREG(info, TOP_REGHGPIO0_OFS);
	local_top_reg_dgpio0.reg = TOP_GETREG(info, TOP_REGDGPIO0_OFS);
	local_top_reg_pgpio0.reg = TOP_GETREG(info, TOP_REGPGPIO0_OFS);


	/*Parsing SDIO1*/
	value = (local_top_reg1.bit.SDIO_EXIST ? PIN_SDIO_CFG_4BITS : 0);
	info->top_pinmux[PIN_FUNC_SDIO].config = value;
	info->top_pinmux[PIN_FUNC_SDIO].pin_function = PIN_FUNC_SDIO;

	/*Parsing SDIO2*/
	value = (local_top_reg1.bit.SDIO2_EXIST ? PIN_SDIO_CFG_4BITS : 0);
	info->top_pinmux[PIN_FUNC_SDIO2].config = value;
	info->top_pinmux[PIN_FUNC_SDIO2].pin_function = PIN_FUNC_SDIO2;

	/*Parsing SDIO3*/
	if (local_top_reg1.bit.SDIO3_EN) {
		value = (local_top_reg1.bit.SDIO3_BUS_WIDTH ? PIN_SDIO_CFG_8BITS : PIN_SDIO_CFG_4BITS);
	} else
		value = 0;

	info->top_pinmux[PIN_FUNC_SDIO3].config = value;
	info->top_pinmux[PIN_FUNC_SDIO3].pin_function = PIN_FUNC_SDIO3;

	/*Parsing NAND*/
	if (local_top_reg1.bit.SPI_EXIST) {
#ifdef CONFIG_MTD_SPINAND
		value = PIN_NAND_CFG_SPI_NAND;
#elif defined(CONFIG_MTD_SPINOR)
		value = PIN_NAND_CFG_SPI_NOR;
#endif
	} else
		value = 0;

	info->top_pinmux[PIN_FUNC_NAND].config = value;
	info->top_pinmux[PIN_FUNC_NAND].pin_function = PIN_FUNC_NAND;

	/*Parsing SENSOR*/
	value = mipi_lvds_sensor[0];

	switch (local_top_reg3.bit.SENSOR) {
	case SENSOR_ENUM_12BITS_1ST:
		value |= PIN_SENSOR_CFG_12BITS;
		break;
	case SENSOR_ENUM_12BITS_2ND:
		value |= PIN_SENSOR_CFG_12BITS_2ND;
		break;
	case SENSOR_ENUM_CCIR8BITS:
		value |= PIN_SENSOR_CFG_CCIR8BITS;
		break;
	case SENSOR_ENUM_CCIR16BITS:
		value |= PIN_SENSOR_CFG_CCIR16BITS;
		break;
	default:
		break;
	}

	if (mipi_lvds_sensor[0] && (local_top_reg3.bit.SEN_VSHS == 1))
		value |= PIN_SENSOR_CFG_LVDS_VDHD;

	if ((local_top_reg3.bit.SEN_MCLK == SENMCLK_SEL_ENUM_MCLK) && mclk_sensor[0])
		value |= PIN_SENSOR_CFG_MCLK;

	if ((local_top_reg3.bit.SEN_MCLK == SENMCLK_SEL_ENUM_MCLK_2ND) && mclk_sensor[1])
		value |= PIN_SENSOR_CFG_MCLK_2ND;

	if ((local_top_reg3.bit.SEN_MCLK == SENMCLK_SEL_ENUM_MCLK_3RD) && mclk_sensor[2])
		value |= PIN_SENSOR_CFG_MCLK_3RD;

	if ((local_top_reg3.bit.SEN_MCLK2 == SENMCLK_SEL_ENUM_MCLK) && mclk2_sensor[0][0])
		value |= PIN_SENSOR_CFG_MCLK2;

	if (local_top_reg3.bit.SP_CLK == SP_CLK_SEL_ENUM_SPCLK)
		value |= PIN_SENSOR_CFG_SPCLK;

	if (local_top_reg3.bit.SP_CLK == SP_CLK_SEL_ENUM_SP_2_CLK)
		value |= PIN_SENSOR_CFG_SPCLK_2ND;

	if (local_top_reg6.bit.SP2_CLK == SP2_CLK_SEL_ENUM_SP2CLK)
		value |= PIN_SENSOR_CFG_SP2CLK;

	if (local_top_reg6.bit.SP2_CLK == SP2_CLK_SEL_ENUM_SP2_2_CLK)
		value |= PIN_SENSOR_CFG_SP2CLK_2ND;

	info->top_pinmux[PIN_FUNC_SENSOR].config = value;
	info->top_pinmux[PIN_FUNC_SENSOR].pin_function = PIN_FUNC_SENSOR;

	/*Parsing SENSOR2*/
	value = mipi_lvds_sensor[1];
	switch (local_top_reg3.bit.SENSOR2) {
	case SENSOR2_ENUM_CCIR8BITS:
		value |= PIN_SENSOR2_CFG_CCIR8BITS;
		break;
	case SENSOR2_ENUM_12BITS:
		value |= PIN_SENSOR2_CFG_12BITS;
		break;
	default:
		break;
	}

	if ((mipi_lvds_sensor[1] != 0) && (local_top_reg_sgpio0.bit.SGPIO_4 == GPIO_ID_EMUM_FUNC) \
			&& (local_top_reg_sgpio0.bit.SGPIO_5 == GPIO_ID_EMUM_FUNC))
		value |= PIN_SENSOR2_CFG_LVDS_VDHD;

	if ((top_reg3.bit.SENSOR2 == SENSOR2_ENUM_CCIR8BITS) && (local_top_reg_pgpio0.bit.PGPIO_11 == GPIO_ID_EMUM_FUNC))
		value |= PIN_SENSOR2_CFG_CCIR8BITS_FIELD;

	if ((top_reg3.bit.SENSOR2 == SENSOR2_ENUM_CCIR8BITS) && (local_top_reg_pgpio0.bit.PGPIO_9 == GPIO_ID_EMUM_FUNC) \
			&& (local_top_reg_pgpio0.bit.PGPIO_10 == GPIO_ID_EMUM_FUNC))
		value |= PIN_SENSOR2_CFG_CCIR8BITS_VDHD;


	if ((local_top_reg_sgpio0.bit.SGPIO_0 == GPIO_ID_EMUM_FUNC) && sn_mclk_en[1])
		value |= PIN_SENSOR2_CFG_SN_MCLK;

	if ((local_top_reg3.bit.SEN_MCLK2 == SENMCLK_SEL_ENUM_MCLK) && mclk2_sensor[1][0])
		value |= PIN_SENSOR2_CFG_MCLK;

	if ((local_top_reg3.bit.SEN_MCLK2 == SENMCLK_SEL_ENUM_MCLK_2ND) && mclk2_sensor[1][1])
		value |= PIN_SENSOR2_CFG_MCLK_2ND;

	if (local_top_reg3.bit.SN3_MCLK == SENMCLK_SEL_ENUM_MCLK)
		value |= PIN_SENSOR2_CFG_SN3_MCLK;

	info->top_pinmux[PIN_FUNC_SENSOR2].config = value;
	info->top_pinmux[PIN_FUNC_SENSOR2].pin_function = PIN_FUNC_SENSOR2;

	/*Parsing MIPI_LVDS*/
	value = 0x0;

	if ((local_top_reg_hgpio0.bit.HSIGPIO_4 == GPIO_ID_EMUM_FUNC) && \
		(local_top_reg_hgpio0.bit.HSIGPIO_5 == GPIO_ID_EMUM_FUNC))
		value |= PIN_MIPI_LVDS_CFG_CLK0;

	if ((local_top_reg_hgpio0.bit.HSIGPIO_10 == GPIO_ID_EMUM_FUNC) && \
		(local_top_reg_hgpio0.bit.HSIGPIO_11 == GPIO_ID_EMUM_FUNC))
		value |= PIN_MIPI_LVDS_CFG_CLK1;

	if ((local_top_reg_hgpio0.bit.HSIGPIO_0 == GPIO_ID_EMUM_FUNC) && \
		(local_top_reg_hgpio0.bit.HSIGPIO_1 == GPIO_ID_EMUM_FUNC))
		value |= PIN_MIPI_LVDS_CFG_DAT0;

	if ((local_top_reg_hgpio0.bit.HSIGPIO_2 == GPIO_ID_EMUM_FUNC) && \
		(local_top_reg_hgpio0.bit.HSIGPIO_3 == GPIO_ID_EMUM_FUNC))
		value |= PIN_MIPI_LVDS_CFG_DAT1;

	if ((local_top_reg_hgpio0.bit.HSIGPIO_6 == GPIO_ID_EMUM_FUNC) && \
		(local_top_reg_hgpio0.bit.HSIGPIO_7 == GPIO_ID_EMUM_FUNC))
		value |= PIN_MIPI_LVDS_CFG_DAT2;

	if ((local_top_reg_hgpio0.bit.HSIGPIO_8 == GPIO_ID_EMUM_FUNC) && \
		(local_top_reg_hgpio0.bit.HSIGPIO_9 == GPIO_ID_EMUM_FUNC))
		value |= PIN_MIPI_LVDS_CFG_DAT3;

	info->top_pinmux[PIN_FUNC_MIPI_LVDS].config = value;
	info->top_pinmux[PIN_FUNC_MIPI_LVDS].pin_function = PIN_FUNC_MIPI_LVDS;

	/*Parsing I2C*/
	value = 0x0;

	if (local_top_reg5.bit.I2C == I2C_ENUM_I2C)
		value |= PIN_I2C_CFG_CH1;

	if (local_top_reg5.bit.I2C == I2C_ENUM_I2C_2ND)
		value |= PIN_I2C_CFG_CH1_2ND_PINMUX;

	if (local_top_reg5.bit.I2C2 == I2C_ENUM_I2C)
		value |= PIN_I2C_CFG_CH2;

	if (local_top_reg5.bit.I2C2 == I2C_ENUM_I2C_2ND)
		value |= PIN_I2C_CFG_CH2_2ND_PINMUX;

	if (local_top_reg5.bit.I2C3 == I2C_ENUM_I2C)
		value |= PIN_I2C_CFG_CH3;

	if (local_top_reg5.bit.I2C3 == I2C_ENUM_I2C_2ND)
		value |= PIN_I2C_CFG_CH3_2ND_PINMUX;

	if (local_top_reg5.bit.I2C4 == I2C_ENUM_I2C)
		value |= PIN_I2C_CFG_CH4;

	if (local_top_reg5.bit.I2C4 == I2C_ENUM_I2C_2ND)
		value |= PIN_I2C_CFG_CH4_2ND_PINMUX;

	if (local_top_reg4.bit.I2C5 == I2C_ENUM_I2C)
		value |= PIN_I2C_CFG_CH5;

	if (local_top_reg4.bit.I2C5 == I2C_ENUM_I2C_2ND)
		value |= PIN_I2C_CFG_CH5_2ND_PINMUX;

	info->top_pinmux[PIN_FUNC_I2C].config = value;
	info->top_pinmux[PIN_FUNC_I2C].pin_function = PIN_FUNC_I2C;

	/*Parsing SIF*/
	value = 0x0;

	if (local_top_reg5.bit.SIFCH0 == SIFCH_ENUM_SIF)
		value |= PIN_SIF_CFG_CH0;

	if (local_top_reg5.bit.SIFCH1 == SIFCH_ENUM_SIF)
		value |= PIN_SIF_CFG_CH1;

	switch (local_top_reg5.bit.SIFCH2) {
	case SIFCH_ENUM_SIF:
		value |= PIN_SIF_CFG_CH2;
		break;
	case SIFCH_ENUM_SIF_2ND:
		value |= PIN_SIF_CFG_CH2_2ND_PINMUX;
		break;
	case SIFCH_ENUM_SIF_3RD:
		value |= PIN_SIF_CFG_CH2_3RD_PINMUX;
		break;
	default:
		break;
	}

	if (local_top_reg5.bit.SIFCH3 == SIFCH_ENUM_SIF)
		value |= PIN_SIF_CFG_CH3;

	info->top_pinmux[PIN_FUNC_SIF].config = value;
	info->top_pinmux[PIN_FUNC_SIF].pin_function = PIN_FUNC_SIF;

	/*Parsing UART*/
	value = 0x0;

	if (local_top_reg9.bit.UART == UART_ENUM_UART)
		value |= PIN_UART_CFG_CH1;

	switch (local_top_reg9.bit.UART2) {
	case UART2_ENUM_1ST_PINMUX:
		value |= PIN_UART_CFG_CH2;
		break;
	case UART2_ENUM_2ND_PINMUX:
		value |= PIN_UART_CFG_CH2 | PIN_UART_CFG_CH2_2ND;
		break;
	case UART2_ENUM_3RD_PINMUX:  //Reserved
		value |= PIN_UART_CFG_CH2 | PIN_UART_CFG_CH2_3RD;
		break;
	default:
		break;
	}

	switch (local_top_reg9.bit.UART2_CTSRTS) {
	case UART_CTSRTS_PINMUX:
		value |= PIN_UART_CFG_CH2_CTSRTS;
		break;
	case UART_CTSRTS_DIROE:
		value |= PIN_UART_CFG_CH2_DIROE;
		break;
	default:
		break;
	}

	switch (local_top_reg9.bit.UART3) {
	case UART3_ENUM_1ST_PINMUX:
		value |= PIN_UART_CFG_CH3;
		break;
	case UART3_ENUM_2ND_PINMUX:
		value |= PIN_UART_CFG_CH3 | PIN_UART_CFG_CH3_2ND;
		break;
	case UART3_ENUM_3RD_PINMUX:  //Reserved
		value |= PIN_UART_CFG_CH3 | PIN_UART_CFG_CH3_3RD;
		break;
	case UART3_ENUM_4TH_PINMUX:
		value |= PIN_UART_CFG_CH3 | PIN_UART_CFG_CH3_4TH;
		break;
	case UART3_ENUM_5TH_PINMUX:
		value |= PIN_UART_CFG_CH3 | PIN_UART_CFG_CH3_5TH;
		break;
	default:
		break;
	}

	switch (local_top_reg9.bit.UART3_CTSRTS) {
	case UART_CTSRTS_PINMUX:
		value |= PIN_UART_CFG_CH3_CTSRTS;
		break;
	case UART_CTSRTS_DIROE:
		value |= PIN_UART_CFG_CH3_DIROE;
		break;
	default:
		break;
	}

	switch (local_top_reg9.bit.UART4) {
	case UART4_ENUM_1ST_PINMUX:
		value |= PIN_UART_CFG_CH4;
		break;
	case UART4_ENUM_2ND_PINMUX:
		value |= PIN_UART_CFG_CH4 | PIN_UART_CFG_CH4_2ND;
		break;
	default:
		break;
	}

	switch (local_top_reg9.bit.UART4_CTSRTS) {
	case UART_CTSRTS_PINMUX:
		value |= PIN_UART_CFG_CH4_CTSRTS;
		break;
	case UART_CTSRTS_DIROE:
		value |= PIN_UART_CFG_CH4_DIROE;
		break;
	default:
		break;
	}

	switch (local_top_reg9.bit.UART5) {
	case UART5_ENUM_1ST_PINMUX:
		value |= PIN_UART_CFG_CH5;
		break;
	case UART5_ENUM_2ND_PINMUX:
		value |= PIN_UART_CFG_CH5 | PIN_UART_CFG_CH5_2ND;
		break;
	default:
		break;
	}

	switch (local_top_reg9.bit.UART5_CTSRTS) {
	case UART_CTSRTS_PINMUX:
		value |= PIN_UART_CFG_CH5_CTSRTS;
		break;
	case UART_CTSRTS_DIROE:
		value |= PIN_UART_CFG_CH5_DIROE;
		break;
	default:
		break;
	}

	switch (local_top_reg9.bit.UART6) {
	case UART6_ENUM_1ST_PINMUX:
		value |= PIN_UART_CFG_CH6;
		break;
	case UART6_ENUM_2ND_PINMUX:
		value |= PIN_UART_CFG_CH6 | PIN_UART_CFG_CH6_2ND;
		break;
	default:
		break;
	}

	switch (local_top_reg9.bit.UART6_CTSRTS) {
	case UART_CTSRTS_PINMUX:
		value |= PIN_UART_CFG_CH6_CTSRTS;
		break;
	case UART_CTSRTS_DIROE:
		value |= PIN_UART_CFG_CH6_DIROE;
		break;
	default:
		break;
	}

	info->top_pinmux[PIN_FUNC_UART].config = value;
	info->top_pinmux[PIN_FUNC_UART].pin_function = PIN_FUNC_UART;

	/*Parsing SPI*/
	value = 0x0;

	switch (local_top_reg5.bit.SPI) {
	case SPI_1ST_PINMUX:
		value |= PIN_SPI_CFG_CH1;
		break;
	case SPI_2ND_PINMUX:
		value |= PIN_SPI_CFG_CH1 | PIN_SPI_CFG_CH1_2ND_PINMUX;
		break;
	default:
		break;
	}

	if (local_top_reg5.bit.SPI_DAT == SPI_DAT_ENUM_2BIT)
		value |= PIN_SPI_CFG_CH1_2BITS;

	switch (local_top_reg5.bit.SPI2) {
	case SPI2_1ST_PINMUX:
		value |= PIN_SPI_CFG_CH2;
		break;
	case SPI2_2ND_PINMUX:
		value |= PIN_SPI_CFG_CH2 | PIN_SPI_CFG_CH2_2ND_PINMUX;
		break;
	default:
		break;
	}

	if (local_top_reg5.bit.SPI2_DAT == SPI_DAT_ENUM_2BIT)
		value |= PIN_SPI_CFG_CH2_2BITS;

	switch (local_top_reg5.bit.SPI3) {
	case SPI3_1ST_PINMUX:
		value |= PIN_SPI_CFG_CH3;
		break;
	case SPI3_2ND_PINMUX:
		value |= PIN_SPI_CFG_CH3 | PIN_SPI_CFG_CH3_2ND_PINMUX;
		break;
	case SPI3_3RD_PINMUX:
		value |= PIN_SPI_CFG_CH3 | PIN_SPI_CFG_CH3_3RD_PINMUX;
		break;
	default:
		break;
	}

	if (local_top_reg5.bit.SPI3_DAT == SPI_DAT_ENUM_2BIT)
		value |= PIN_SPI_CFG_CH3_2BITS;

	switch (local_top_reg5.bit.SPI3_RDY) {
	case SPI3_RDY_1ST_PINMUX:
	case SPI3_RDY_2ND_PINMUX:
	case SPI3_RDY_3RD_PINMUX:
		value |= PIN_SPI_CFG_CH3_RDY;
		break;
	default:
		break;
	}

	switch (local_top_reg4.bit.SPI4) {
	case SPI4_1ST_PINMUX:
		value |= PIN_SPI_CFG_CH4;
		break;
	case SPI4_2ND_PINMUX:
		value |= PIN_SPI_CFG_CH4 | PIN_SPI_CFG_CH4_2ND_PINMUX;
		break;
	default:
		break;
	}

	if (local_top_reg4.bit.SPI4_DAT == SPI_DAT_ENUM_2BIT)
		value |= PIN_SPI_CFG_CH4_2BITS;

	switch (local_top_reg4.bit.SPI5) {
	case SPI5_1ST_PINMUX:
		value |= PIN_SPI_CFG_CH5;
		break;
	case SPI5_2ND_PINMUX:
		value |= PIN_SPI_CFG_CH5 | PIN_SPI_CFG_CH5_2ND_PINMUX;
		break;
	default:
		break;
	}

	if (local_top_reg4.bit.SPI5_DAT == SPI_DAT_ENUM_2BIT)
		value |= PIN_SPI_CFG_CH5_2BITS;

	info->top_pinmux[PIN_FUNC_SPI].config = value;
	info->top_pinmux[PIN_FUNC_SPI].pin_function = PIN_FUNC_SPI;

	/*Parsing SDP*/
	value = 0x0;

	if (local_top_reg5.bit.SDP == SDP_1ST_PINMUX)
		value |= PIN_SDP_CFG_CH1;

	switch (local_top_reg5.bit.SDP) {
	case SDP_1ST_PINMUX:
		value |= PIN_SDP_CFG_CH1;
		break;
	case SDP_2ND_PINMUX:
		value |= PIN_SDP_CFG_CH2;
		break;
	default:
		break;
	}

	info->top_pinmux[PIN_FUNC_SDP].config = value;
	info->top_pinmux[PIN_FUNC_SDP].pin_function = PIN_FUNC_SDP;

	/*Parsing REMOTE*/
	value = 0x0;

	switch (local_top_reg6.bit.REMOTE) {
	case REMOTE_ENUM_REMOTE_1ST:
		value |= PIN_REMOTE_CFG_CH1;
		break;
	case REMOTE_ENUM_REMOTE_2ND:
		value |= PIN_REMOTE_CFG_CH2;
		break;
	default:
		break;
	}

	info->top_pinmux[PIN_FUNC_REMOTE].config = value;
	info->top_pinmux[PIN_FUNC_REMOTE].pin_function = PIN_FUNC_REMOTE;

	/*Parsing PWM*/
	value = 0x0;

	switch (local_top_reg7.bit.PWM0) {
	case PWM_ENUM_PWM:
		value |= PIN_PWM_CFG_PWM0_1;
		break;
	case PWM_ENUM_PWM_2ND:
		value |= PIN_PWM_CFG_PWM0_2;
		break;
	case PWM_ENUM_PWM_3RD:
		value |= PIN_PWM_CFG_PWM0_3;
		break;
	case PWM_ENUM_PWM_4TH:
		value |= PIN_PWM_CFG_PWM0_4;
		break;
	case PWM_ENUM_PWM_5TH:
		value |= PIN_PWM_CFG_PWM0_5;
		break;
	default:
		break;
	}

	switch (local_top_reg7.bit.PWM1) {
	case PWM_ENUM_PWM:
		value |= PIN_PWM_CFG_PWM1_1;
		break;
	case PWM_ENUM_PWM_2ND:
		value |= PIN_PWM_CFG_PWM1_2;
		break;
	case PWM_ENUM_PWM_3RD:
		value |= PIN_PWM_CFG_PWM1_3;
		break;
	case PWM_ENUM_PWM_4TH:
		value |= PIN_PWM_CFG_PWM1_4;
		break;
	case PWM_ENUM_PWM_5TH:
		value |= PIN_PWM_CFG_PWM1_5;
		break;
	default:
		break;
	}

	switch (local_top_reg7.bit.PWM2) {
	case PWM_ENUM_PWM:
		value |= PIN_PWM_CFG_PWM2_1;
		break;
	case PWM_ENUM_PWM_2ND:
		value |= PIN_PWM_CFG_PWM2_2;
		break;
	case PWM_ENUM_PWM_3RD:
		value |= PIN_PWM_CFG_PWM2_3;
		break;
	case PWM_ENUM_PWM_4TH:
		value |= PIN_PWM_CFG_PWM2_4;
		break;
	case PWM_ENUM_PWM_5TH:
		value |= PIN_PWM_CFG_PWM2_5;
		break;
	default:
		break;
	}

	switch (local_top_reg7.bit.PWM3) {
	case PWM_ENUM_PWM:
		value |= PIN_PWM_CFG_PWM3_1;
		break;
	case PWM_ENUM_PWM_2ND:
		value |= PIN_PWM_CFG_PWM3_2;
		break;
	case PWM_ENUM_PWM_3RD:
		value |= PIN_PWM_CFG_PWM3_3;
		break;
	case PWM_ENUM_PWM_4TH:
		value |= PIN_PWM_CFG_PWM3_4;
		break;
	case PWM_ENUM_PWM_5TH:
		value |= PIN_PWM_CFG_PWM3_5;
		break;
	default:
		break;
	}

	switch (local_top_reg6.bit.PWM8) {
	case PWM_ENUM_PWM:
		value |= PIN_PWM_CFG_PWM8_1;
		break;
	case PWM_ENUM_PWM_2ND:
		value |= PIN_PWM_CFG_PWM8_2;
		break;
	case PWM_ENUM_PWM_3RD:
		value |= PIN_PWM_CFG_PWM8_3;
		break;
	case PWM_ENUM_PWM_4TH:
		value |= PIN_PWM_CFG_PWM8_4;
		break;
	case PWM_ENUM_PWM_5TH:
		value |= PIN_PWM_CFG_PWM8_5;
		break;
	default:
		break;
	}

	switch (local_top_reg6.bit.PWM9) {
	case PWM_ENUM_PWM:
		value |= PIN_PWM_CFG_PWM9_1;
		break;
	case PWM_ENUM_PWM_2ND:
		value |= PIN_PWM_CFG_PWM9_2;
		break;
	case PWM_ENUM_PWM_3RD:
		value |= PIN_PWM_CFG_PWM9_3;
		break;
	case PWM_ENUM_PWM_4TH:
		value |= PIN_PWM_CFG_PWM9_4;
		break;
	case PWM_ENUM_PWM_5TH:
		value |= PIN_PWM_CFG_PWM9_5;
		break;
	default:
		break;
	}

	info->top_pinmux[PIN_FUNC_PWM].config = value;
	info->top_pinmux[PIN_FUNC_PWM].pin_function = PIN_FUNC_PWM;

	/*Parsing PWM2*/
	value = 0x0;

	switch (local_top_reg7.bit.PWM4) {
	case PWM_ENUM_PWM:
		value |= PIN_PWM_CFG2_PWM4_1;
		break;
	case PWM_ENUM_PWM_2ND:
		value |= PIN_PWM_CFG2_PWM4_2;
		break;
	case PWM_ENUM_PWM_3RD:
		value |= PIN_PWM_CFG2_PWM4_3;
		break;
	case PWM_ENUM_PWM_4TH:
		value |= PIN_PWM_CFG2_PWM4_4;
		break;
	case PWM_ENUM_PWM_5TH:
		value |= PIN_PWM_CFG2_PWM4_5;
		break;
	default:
		break;
	}

	switch (local_top_reg7.bit.PWM5) {
	case PWM_ENUM_PWM:
		value |= PIN_PWM_CFG2_PWM5_1;
		break;
	case PWM_ENUM_PWM_2ND:
		value |= PIN_PWM_CFG2_PWM5_2;
		break;
	case PWM_ENUM_PWM_3RD:
		value |= PIN_PWM_CFG2_PWM5_3;
		break;
	case PWM_ENUM_PWM_4TH:
		value |= PIN_PWM_CFG2_PWM5_4;
		break;
	case PWM_ENUM_PWM_5TH:
		value |= PIN_PWM_CFG2_PWM5_5;
		break;
	default:
		break;
	}

	switch (local_top_reg7.bit.PWM6) {
	case PWM_ENUM_PWM:
		value |= PIN_PWM_CFG2_PWM6_1;
		break;
	case PWM_ENUM_PWM_2ND:
		value |= PIN_PWM_CFG2_PWM6_2;
		break;
	case PWM_ENUM_PWM_3RD:
		value |= PIN_PWM_CFG2_PWM6_3;
		break;
	case PWM_ENUM_PWM_4TH:
		value |= PIN_PWM_CFG2_PWM6_4;
		break;
	case PWM_ENUM_PWM_5TH:
		value |= PIN_PWM_CFG2_PWM6_5;
		break;
	default:
		break;
	}

	switch (local_top_reg7.bit.PWM7) {
	case PWM_ENUM_PWM:
		value |= PIN_PWM_CFG2_PWM7_1;
		break;
	case PWM_ENUM_PWM_2ND:
		value |= PIN_PWM_CFG2_PWM7_2;
		break;
	case PWM_ENUM_PWM_3RD:
		value |= PIN_PWM_CFG2_PWM7_3;
		break;
	case PWM_ENUM_PWM_4TH:
		value |= PIN_PWM_CFG2_PWM7_4;
		break;
	case PWM_ENUM_PWM_5TH:
		value |= PIN_PWM_CFG2_PWM7_5;
		break;
	default:
		break;
	}

	switch (local_top_reg6.bit.PWM10) {
	case PWM_ENUM_PWM:
		value |= PIN_PWM_CFG2_PWM10_1;
		break;
	case PWM_ENUM_PWM_2ND:
		value |= PIN_PWM_CFG2_PWM10_2;
		break;
	case PWM_ENUM_PWM_3RD:
		value |= PIN_PWM_CFG2_PWM10_3;
		break;
	case PWM_ENUM_PWM_4TH:
		value |= PIN_PWM_CFG2_PWM10_4;
		break;
	case PWM_ENUM_PWM_5TH:
		value |= PIN_PWM_CFG2_PWM10_5;
		break;
	default:
		break;
	}

	switch (local_top_reg6.bit.PWM11) {
	case PWM_ENUM_PWM:
		value |= PIN_PWM_CFG2_PWM11_1;
		break;
	case PWM_ENUM_PWM_2ND:
		value |= PIN_PWM_CFG2_PWM11_2;
		break;
	case PWM_ENUM_PWM_3RD:
		value |= PIN_PWM_CFG2_PWM11_3;
		break;
	case PWM_ENUM_PWM_4TH:
		value |= PIN_PWM_CFG2_PWM11_4;
		break;
	case PWM_ENUM_PWM_5TH:
		value |= PIN_PWM_CFG2_PWM11_5;
		break;
	default:
		break;
	}

	info->top_pinmux[PIN_FUNC_PWM2].config = value;
	info->top_pinmux[PIN_FUNC_PWM2].pin_function = PIN_FUNC_PWM2;

	/*Parsing CCNT*/
	value = 0x0;

	switch (local_top_reg7.bit.PI_CNT) {
	case PICNT_ENUM_PICNT:
		value |= PIN_PWM_CFG_CCNT;
		break;
	case PICNT_ENUM_PICNT2:
		value |= PIN_PWM_CFG_CCNT | PIN_PWM_CFG_CCNT_2ND;
		break;
	default:
		break;
	}

	switch (local_top_reg7.bit.PI_CNT2) {
	case PICNT_ENUM_PICNT:
		value |= PIN_PWM_CFG_CCNT2;
		break;
	case PICNT_ENUM_PICNT2:
		value |= PIN_PWM_CFG_CCNT2 | PIN_PWM_CFG_CCNT2_2ND;
		break;
	default:
		break;
	}

	switch (local_top_reg7.bit.PI_CNT3) {
	case PICNT_ENUM_PICNT:
		value |= PIN_PWM_CFG_CCNT3;
		break;
	case PICNT_ENUM_PICNT2:
		value |= PIN_PWM_CFG_CCNT3 | PIN_PWM_CFG_CCNT3_2ND;
		break;
	default:
		break;
	}

	info->top_pinmux[PIN_FUNC_CCNT].config = value;
	info->top_pinmux[PIN_FUNC_CCNT].pin_function = PIN_FUNC_CCNT;

	/*Parsing AUDIO*/
	value = 0x0;

	if (local_top_reg6.bit.AUDIO == AUDIO_ENUM_I2S)
		value |= PIN_AUDIO_CFG_I2S;
	else if (local_top_reg6.bit.AUDIO == AUDIO_ENUM_I2S_2ND_PINMUX)
		value |= PIN_AUDIO_CFG_I2S_2ND_PINMUX;

	if (local_top_reg6.bit.AUDIO_MCLK == AUDIO_MCLK_MCLK)
		value |= PIN_AUDIO_CFG_MCLK;
	else if (local_top_reg6.bit.AUDIO_MCLK == AUDIO_MCLK_MCLK_2ND_PINMUX)
		value |= PIN_AUDIO_CFG_MCLK_2ND_PINMUX;

	switch (local_top_reg6.bit.DIGITAL_MIC) {
	case DMIC_ENUM_DMIC:
		value |= PIN_AUDIO_CFG_DMIC;
		break;
	case DMIC_ENUM_DMIC_2ND_PINMUX:
		value |= PIN_AUDIO_CFG_DMIC_2ND;
		break;
	case DMIC_ENUM_DMIC_3RD_PINMUX:
		value |= PIN_AUDIO_CFG_DMIC_3RD;
		break;
	default:
		break;
	}

	info->top_pinmux[PIN_FUNC_AUDIO].config = value;
	info->top_pinmux[PIN_FUNC_AUDIO].pin_function = PIN_FUNC_AUDIO;

	/*Parsing LCD*/
	info->top_pinmux[PIN_FUNC_LCD].config = disp_pinmux_config[PINMUX_FUNC_ID_LCD];
	info->top_pinmux[PIN_FUNC_LCD].pin_function = PIN_FUNC_LCD;

	/*Parsing TV*/
	info->top_pinmux[PIN_FUNC_TV].config = disp_pinmux_config[PINMUX_FUNC_ID_TV];
	info->top_pinmux[PIN_FUNC_TV].pin_function = PIN_FUNC_TV;

	/*Parsing ETH*/
	value = 0x0;

	switch (local_top_reg6.bit.ETH_LED) {
	case ETH_LED_ENUM_LED1:
		value |= PIN_ETH_CFG_INTERANL | PIN_ETH_CFG_LED1;
		if (local_top_reg_dgpio0.bit.DGPIO_0 == 1) {
			// 1st is GPIO => 2nd only
			value |= PIN_ETH_CFG_LED_2ND_ONLY;
		} else if (local_top_reg_dgpio0.bit.DGPIO_1 == 1) {
			// 2nd is GPIO => 1st only
			value |= PIN_ETH_CFG_LED_1ST_ONLY;
		}
		break;
	case ETH_LED_ENUM_LED2:
		value |= PIN_ETH_CFG_INTERANL | PIN_ETH_CFG_LED2;
		if (local_top_reg_dgpio0.bit.DGPIO_5 == 1) {
			// 1st is GPIO => 2nd only
			value |= PIN_ETH_CFG_LED_2ND_ONLY;
		} else if (local_top_reg_dgpio0.bit.DGPIO_6 == 1) {
			// 2nd is GPIO => 1st only
			value |= PIN_ETH_CFG_LED_1ST_ONLY;
		}
		break;
	default:
		break;
	}

	if (nvt_get_chip_id() == CHIP_NA51055)
	{
		if (local_top_reg6.bit.ETH == ETH_ID_ENUM_RMII)
			value |= PIN_ETH_CFG_RMII;
	}
	else
	{
		if (local_top_reg4.bit.ETH == ETH_ID_ENUM_RMII)
			value |= PIN_ETH_CFG_RMII;
		else if (local_top_reg4.bit.ETH == ETH_ID_ENUM_RMII_2)
			value |= PIN_ETH_CFG_RMII_2;
		else if (local_top_reg4.bit.ETH == ETH_ID_ENUM_RGMII)
			value |= PIN_ETH_CFG_RGMII;
	}

	if (local_top_reg6.bit.EXT_PHYCLK == ETH_EXTPHY_CLK_FUNC)
		value |= PIN_ETH_CFG_EXTPHYCLK;

	info->top_pinmux[PIN_FUNC_ETH].config = value;
	info->top_pinmux[PIN_FUNC_ETH].pin_function = PIN_FUNC_ETH;

	/*Parsing MISC*/
	value = 0x0;

	if (local_top_reg6.bit.RTC_CLK == RTC_ENUM_RTCCLK)
		value |= PIN_MISC_CFG_RTCLK;

	switch (local_top_reg3.bit.SP_CLK) {
	case SP_CLK_SEL_ENUM_GPIO:
		value |= PIN_MISC_CFG_SPCLK_NONE;
		break;
	case SP_CLK_SEL_ENUM_SPCLK:
		value |= PIN_MISC_CFG_SPCLK;
		break;
	case SP_CLK_SEL_ENUM_SP_2_CLK:
		value |= PIN_MISC_CFG_SPCLK_2ND;
		break;
	default:
		break;
	}

	switch (local_top_reg6.bit.SP2_CLK) {
	case SP_CLK_SEL_ENUM_GPIO:
		value |= PIN_MISC_CFG_SP2CLK_NONE;
		break;
	case SP2_CLK_SEL_ENUM_SP2CLK:
		value |= PIN_MISC_CFG_SP2CLK;
		break;
	case SP2_CLK_SEL_ENUM_SP2_2_CLK:
		value |= PIN_MISC_CFG_SP2CLK_2ND;
		break;
	default:
		break;
	}

	info->top_pinmux[PIN_FUNC_MISC].config = value;
	info->top_pinmux[PIN_FUNC_MISC].pin_function = PIN_FUNC_MISC;

	unl_cpu(&top_lock, flags);
}

/**
	Configure pinmux controller

	Configure pinmux controller by upper layer

	@param[in] info	nvt_pinctrl_info
	@return void
*/

ER pinmux_init(struct nvt_pinctrl_info *info)
{
	uint32_t i, j;
	int err;
	unsigned long flags = 0;
    u32 lcd_type = 0x0;
	int logoboot = logo_determination(&lcd_type);
	static int boot_hint = 1;

	/*Assume all PINMUX is GPIO*/
	top_reg1.reg = 0;
	top_reg2.reg = 0;
	top_reg3.reg = 0;
	top_reg4.reg = 0;
	top_reg5.reg = 0;
	top_reg6.reg = 0;
	top_reg7.reg = 0;
	top_reg8.reg = 0x01;
	top_reg9.reg = 0;
	top_reg_cgpio0.reg = 0xFFFFFFFF;
	top_reg_pgpio0.reg = 0xFFFFFFFF;
	top_reg_sgpio0.reg = 0xFFFFFFFF;
	top_reg_lgpio0.reg = 0xFFFFFFFF;
	top_reg_hgpio0.reg = 0xFFFFFFFF;
	top_reg0.reg = TOP_GETREG(info, TOP_REG0_OFS);
	if (top_reg0.bit.EJTAG_SEL) {
		top_reg1.reg = 0x100000;
		top_reg_dgpio0.reg = 0x00000783;
	} else
		top_reg_dgpio0.reg = 0x000007FF;

	if (boot_hint && logoboot) {
		top_reg_lgpio0.reg = TOP_GETREG(info, TOP_REGLGPIO0_OFS);
	}

	/*Enter critical section*/
	loc_cpu(&top_lock, flags);

	/*Reset config value*/
	for (i = 0; i < sizeof(mipi_lvds_sensor)/sizeof(mipi_lvds_sensor[0]); i++) {
		mipi_lvds_sensor[i] = 0;
		for (j = 0; j < sizeof(mclk2_sensor[0])/sizeof(mclk2_sensor[0][0]); j++)
			mclk2_sensor[i][j] = 0;
	}

	for (i = 0; i < sizeof(mclk_sensor)/sizeof(mclk_sensor[0]); i++)
		mclk_sensor[i] = 0;

	for (i = 0; i < PIN_FUNC_MAX; i++) {
		if (info->top_pinmux[i].pin_function != i) {
			pr_err("top_config[%d].pinFunction context error\n", i);
			/*Leave critical section*/
			unl_cpu(&top_lock, flags);
			return E_CTX;
		}

		err = pinmux_config_hdl[i](info->top_pinmux[i].config);
		if (err != E_OK) {
			pr_err("top_config[%d].config config error\n", i);
			/*Leave critical section*/
			unl_cpu(&top_lock, flags);
			return err;
		}
	}

	TOP_SETREG(info, TOP_REG1_OFS, top_reg1.reg);
	//TOP_SETREG(info, TOP_REG2_OFS, top_reg2.reg);
	TOP_SETREG(info, TOP_REG3_OFS, top_reg3.reg);
	TOP_SETREG(info, TOP_REG4_OFS, top_reg4.reg);
	TOP_SETREG(info, TOP_REG5_OFS, top_reg5.reg);
	TOP_SETREG(info, TOP_REG6_OFS, top_reg6.reg);
	TOP_SETREG(info, TOP_REG7_OFS, top_reg7.reg);
	TOP_SETREG(info, TOP_REG8_OFS, top_reg8.reg);
	TOP_SETREG(info, TOP_REG9_OFS, top_reg9.reg);

	TOP_SETREG(info, TOP_REGCGPIO0_OFS, top_reg_cgpio0.reg);
	TOP_SETREG(info, TOP_REGPGPIO0_OFS, top_reg_pgpio0.reg);
	TOP_SETREG(info, TOP_REGSGPIO0_OFS, top_reg_sgpio0.reg);
	if (lcd_mask & (~top_reg_lgpio0.reg & lgpio_mask)) {
		pr_err("Conflict with locked LCD, lcd_mask = 0x%x, ~(lgpio0) = 0x%x\r\n", lcd_mask, (~top_reg_lgpio0.reg & lgpio_mask));
	}
	TOP_SETREG(info, TOP_REGLGPIO0_OFS, top_reg_lgpio0.reg & (~lcd_mask & lgpio_mask));  /*Lock lcd pin*/
	TOP_SETREG(info, TOP_REGDGPIO0_OFS, top_reg_dgpio0.reg);
	TOP_SETREG(info, TOP_REGHGPIO0_OFS, top_reg_hgpio0.reg);

	/*Leave critical section*/
	unl_cpu(&top_lock, flags);

    if (boot_hint && logoboot) {
        pr_info("logoboot enable, lcd_type = %08x\r\n",lcd_type);
		pinmux_set_config(PINMUX_FUNC_ID_LCD, lcd_type);
		boot_hint = 0;
	}


	return E_OK;
}


/*-----------------------------------------------------------------------------*/
/* PINMUX Interface Functions							*/
/*-----------------------------------------------------------------------------*/
static int pinmux_config_sdio(uint32_t config)
{
	if (config == PIN_SDIO_CFG_NONE) {
	} else {
		if (config & PIN_SDIO_CFG_2ND_PINMUX) {
			pr_err("SDIO does NOT have 2nd pinmux pad\r\n");
			return E_PAR;
		}

		if (config & PIN_SDIO_CFG_8BITS) {
			pr_err("SDIO does NOT support 8 bits\r\n");
			return E_PAR;
		}

		//pinmux conflict checking
		if (top_reg9.bit.UART2 == UART2_ENUM_2ND_PINMUX) {
			pr_err("SDIO conflict with UART2_2\r\n");
			return E_PAR;
		}

		if (top_reg5.bit.I2C3 == I2C_ENUM_I2C_2ND) {
			pr_err("SDIO conflict with I2C3_2\r\n");
			return E_PAR;
		}

		if (top_reg5.bit.SPI3 == SPI3_2ND_PINMUX) {
			pr_err("SDIO conflict with SPI3_2 \r\n");
			return E_PAR;
		}

		if (top_reg5.bit.SDP == SDP_1ST_PINMUX) {
			pr_err("SDIO conflict with SDP_1 \r\n");
			return E_PAR;
		}

		if (top_reg6.bit.REMOTE == REMOTE_ENUM_REMOTE_2ND) {
			pr_err("SDIO conflict with remote_in_2 \r\n");
			return E_PAR;
		}

		if (top_reg6.bit.DIGITAL_MIC == DMIC_ENUM_DMIC_3RD_PINMUX) {
			pr_err("SDIO conflict with DMCLK_3 \r\n");
			return E_PAR;
		}

		if ((top_reg7.bit.PWM0 == PWM_ENUM_PWM_2ND) ||
			(top_reg7.bit.PWM1 == PWM_ENUM_PWM_2ND) ||
			(top_reg7.bit.PWM2 == PWM_ENUM_PWM_2ND) ||
			(top_reg7.bit.PWM3 == PWM_ENUM_PWM_2ND) ||
			(top_reg7.bit.PWM4 == PWM_ENUM_PWM_2ND) ||
			(top_reg7.bit.PWM5 == PWM_ENUM_PWM_2ND)) {

			pr_err("SDIO conflict with PWM0_2 ~PWM5_2 \r\n");
			return E_PAR;
		}

		top_reg1.bit.SDIO_EXIST = SDIO_EXIST_EN;    // SDIO exist

		top_reg_cgpio0.bit.CGPIO_11 = GPIO_ID_EMUM_FUNC;
		top_reg_cgpio0.bit.CGPIO_12 = GPIO_ID_EMUM_FUNC;
		top_reg_cgpio0.bit.CGPIO_13 = GPIO_ID_EMUM_FUNC;
		top_reg_cgpio0.bit.CGPIO_14 = GPIO_ID_EMUM_FUNC;
		top_reg_cgpio0.bit.CGPIO_15 = GPIO_ID_EMUM_FUNC;
		top_reg_cgpio0.bit.CGPIO_16 = GPIO_ID_EMUM_FUNC;
	}

	return E_OK;
}

static int pinmux_config_sdio2(uint32_t config)
{
	if (config == PIN_SDIO_CFG_NONE) {
	} else {
		if (config & PIN_SDIO_CFG_8BITS) {
			pr_err("SDIO2 does NOT support 8 bits\r\n");
			return E_PAR;
		}

		if (config & PIN_SDIO_CFG_2ND_PINMUX) {
			pr_err("SDIO2 does NOT have 2nd pinmux pad\r\n");
			return E_PAR;
		}

		//pinmux conflict checking
		if (top_reg5.bit.I2C2 == I2C_ENUM_I2C_2ND) {
			pr_err("SDIO2 conflict with I2C2_2\r\n");
			return E_PAR;
		}

		if (top_reg9.bit.UART3 == UART3_ENUM_2ND_PINMUX) {
			pr_err("SDIO2 conflict with UART3_2\r\n");
			return E_PAR;
		}

		if (top_reg5.bit.SPI == SPI_1ST_PINMUX) {
			pr_err("SDIO2 conflict with SPI_1 \r\n");
			return E_PAR;
		}

		if ((top_reg7.bit.PWM6 == PWM_ENUM_PWM_2ND) ||
			(top_reg7.bit.PWM7 == PWM_ENUM_PWM_2ND) ||
			(top_reg6.bit.PWM8 == PWM_ENUM_PWM_2ND) ||
			(top_reg6.bit.PWM9 == PWM_ENUM_PWM_2ND) ||
			(top_reg6.bit.PWM10 == PWM_ENUM_PWM_2ND) ||
			(top_reg6.bit.PWM11 == PWM_ENUM_PWM_2ND)){

			pr_err("SDIO2 conflict with PWM6_2 ~PWM11_2 \r\n");
			return E_PAR;
		}

		top_reg1.bit.SDIO2_EXIST = SDIO2_EXIST_EN;  // SD CLK exist

		top_reg_cgpio0.bit.CGPIO_17 = GPIO_ID_EMUM_FUNC;
		top_reg_cgpio0.bit.CGPIO_18 = GPIO_ID_EMUM_FUNC;
		top_reg_cgpio0.bit.CGPIO_19 = GPIO_ID_EMUM_FUNC;
		top_reg_cgpio0.bit.CGPIO_20 = GPIO_ID_EMUM_FUNC;
		top_reg_cgpio0.bit.CGPIO_21 = GPIO_ID_EMUM_FUNC;
		top_reg_cgpio0.bit.CGPIO_22 = GPIO_ID_EMUM_FUNC;
	}

	return E_OK;
}

static int pinmux_config_sdio3(uint32_t config)
{
	if (config == PIN_SDIO_CFG_NONE) {
	} else {
		if (config & PIN_SDIO_CFG_2ND_PINMUX) {
			pr_err("SDIO3 does NOT support 2ND_PINMUX\r\n");
			return E_PAR;
		} else {
			//pinmux conflict checking
			if (top_reg9.bit.UART3 == UART3_ENUM_5TH_PINMUX) {
				pr_err("SDIO3 conflict with UART3_5\r\n");
				return E_PAR;
			}

			if(top_reg6.bit.AUDIO == AUDIO_ENUM_I2S_2ND_PINMUX) {
				pr_err("SDIO3 conflict with I2S_2\r\n");
				return E_PAR;
			}

			if(top_reg6.bit.SP2_CLK == SP2_CLK_SEL_ENUM_SP2_2_CLK) {
				pr_err("SDIO3 conflict with SP_CLK2_2\r\n");
				return E_PAR;
			}

			if (config & PIN_SDIO_CFG_8BITS) {
				if (nvt_get_chip_id() == CHIP_NA51084) {
					if (top_reg9.bit.UART4 == UART4_ENUM_2ND_PINMUX) {
						pr_err("SDIO3 8-bit conflict with UART4_2\r\n");
						return E_PAR;
					}

					if(top_reg5.bit.I2C4 == I2C_ENUM_I2C_2ND) {
						pr_err("SDIO3 8-bit conflict with I2C4_2\r\n");
						return E_PAR;
					}

					if(top_reg4.bit.SPI4 == SPI4_2ND_PINMUX) {
						pr_err("SDIO3 8-bit conflict with SPI4_2\r\n");
						return E_PAR;
					}
				}

				if ((top_reg7.bit.PWM0 == PWM_ENUM_PWM_4TH) ||
					(top_reg7.bit.PWM1 == PWM_ENUM_PWM_4TH) ||
					(top_reg7.bit.PWM2 == PWM_ENUM_PWM_4TH) ||
					(top_reg7.bit.PWM3 == PWM_ENUM_PWM_4TH)) {

					pr_err("SDIO3 8-bit conflict with PWM0_4 ~PWM3_4 \r\n");
					return E_PAR;
				}

				top_reg1.bit.SDIO3_BUS_WIDTH = SDIO3_BUS_WIDTH_8BITS;

				top_reg_cgpio0.bit.CGPIO_4 = GPIO_ID_EMUM_FUNC;
				top_reg_cgpio0.bit.CGPIO_5 = GPIO_ID_EMUM_FUNC;
				top_reg_cgpio0.bit.CGPIO_6 = GPIO_ID_EMUM_FUNC;
				top_reg_cgpio0.bit.CGPIO_7 = GPIO_ID_EMUM_FUNC;
			}

			top_reg_cgpio0.bit.CGPIO_0 = GPIO_ID_EMUM_FUNC;
			top_reg_cgpio0.bit.CGPIO_1 = GPIO_ID_EMUM_FUNC;
			top_reg_cgpio0.bit.CGPIO_2 = GPIO_ID_EMUM_FUNC;
			top_reg_cgpio0.bit.CGPIO_3 = GPIO_ID_EMUM_FUNC;
			top_reg_cgpio0.bit.CGPIO_8 = GPIO_ID_EMUM_FUNC;
			top_reg_cgpio0.bit.CGPIO_9 = GPIO_ID_EMUM_FUNC;
		}

		top_reg1.bit.SDIO3_EN = SDIO3_EN;  // SD3 pinmux enable

	}

	return E_OK;
}

static int pinmux_config_nand(uint32_t config)
{
	if (config == PIN_NAND_CFG_NONE) {
	} else if (config & (PIN_NAND_CFG_1CS|PIN_NAND_CFG_2CS|PIN_NAND_CFG_SPI_NAND|PIN_NAND_CFG_SPI_NOR)) {

		if (top_reg1.bit.SDIO3_EN == SDIO3_EN) {
			pr_err("NAND conflict with SDIO3\r\n");
			return E_OBJ;
		}

		if (config & PIN_NAND_CFG_2CS) {
			pr_err("Not support PIN_NAND_CFG_2CS\r\n");
			return E_OBJ;
		}

		if (config & PIN_NAND_CFG_SPI_NAND) {
			if (config & PIN_NAND_CFG_SPI_NOR) {
				pr_err("conflict with SPI NOR\r\n");
				return E_OBJ;
			} else {
				top_reg1.bit.SPI_EXIST = SPI_EXIST_EN;
			}
		} else {
			if (config & PIN_NAND_CFG_SPI_NAND) {
				pr_err("conflict with SPI NAND\r\n");
				return E_OBJ;
			} else {
				top_reg1.bit.SPI_EXIST = SPI_EXIST_EN;
			}
		}

		top_reg_cgpio0.bit.CGPIO_0 = GPIO_ID_EMUM_FUNC;
		top_reg_cgpio0.bit.CGPIO_1 = GPIO_ID_EMUM_FUNC;
		top_reg_cgpio0.bit.CGPIO_2 = GPIO_ID_EMUM_FUNC;
		top_reg_cgpio0.bit.CGPIO_3 = GPIO_ID_EMUM_FUNC;
		top_reg_cgpio0.bit.CGPIO_8 = GPIO_ID_EMUM_FUNC;
		top_reg_cgpio0.bit.CGPIO_10 = GPIO_ID_EMUM_FUNC;
	} else {
		pr_err("invalid config: 0x%x\r\n", config);
		return E_PAR;
	}
	return E_OK;
}

static int pinmux_config_sensor(uint32_t config)
{
	uint32_t tmp;

	if (config == PIN_SENSOR_CFG_NONE) {
	} else {
		mipi_lvds_sensor[0] = 0;
		tmp = config & (PIN_SENSOR_CFG_12BITS | PIN_SENSOR_CFG_MIPI | \
				PIN_SENSOR_CFG_LVDS | PIN_SENSOR_CFG_CCIR8BITS | \
				PIN_SENSOR_CFG_CCIR16BITS | PIN_SENSOR_CFG_12BITS_2ND);

		switch (tmp) {
		case PIN_SENSOR_CFG_12BITS:
			top_reg3.bit.SENSOR = SENSOR_ENUM_12BITS_1ST;
			top_reg_hgpio0.bit.HSIGPIO_0 = GPIO_ID_EMUM_GPIO;
			top_reg_hgpio0.bit.HSIGPIO_1 = GPIO_ID_EMUM_GPIO;
			top_reg_hgpio0.bit.HSIGPIO_2 = GPIO_ID_EMUM_GPIO;
			top_reg_hgpio0.bit.HSIGPIO_3 = GPIO_ID_EMUM_GPIO;
			top_reg_hgpio0.bit.HSIGPIO_4 = GPIO_ID_EMUM_GPIO;
			top_reg_hgpio0.bit.HSIGPIO_5 = GPIO_ID_EMUM_GPIO;
			top_reg_hgpio0.bit.HSIGPIO_6 = GPIO_ID_EMUM_GPIO;
			top_reg_hgpio0.bit.HSIGPIO_7 = GPIO_ID_EMUM_GPIO;
			top_reg_hgpio0.bit.HSIGPIO_8 = GPIO_ID_EMUM_GPIO;
			top_reg_hgpio0.bit.HSIGPIO_9 = GPIO_ID_EMUM_GPIO;
			top_reg_hgpio0.bit.HSIGPIO_10 = GPIO_ID_EMUM_GPIO;
			top_reg_hgpio0.bit.HSIGPIO_11 = GPIO_ID_EMUM_GPIO;
			top_reg_sgpio0.bit.SGPIO_1 = GPIO_ID_EMUM_FUNC; //PXCLK
			top_reg_sgpio0.bit.SGPIO_2 = GPIO_ID_EMUM_FUNC; //VD
			top_reg_sgpio0.bit.SGPIO_3 = GPIO_ID_EMUM_FUNC; //HD
			break;

		case PIN_SENSOR_CFG_12BITS_2ND:
			top_reg3.bit.SENSOR = SENSOR_ENUM_12BITS_2ND;  //parallel data
			//D0 .. D11
			top_reg_pgpio0.bit.PGPIO_0 = GPIO_ID_EMUM_GPIO;
			top_reg_pgpio0.bit.PGPIO_1 = GPIO_ID_EMUM_GPIO;
			top_reg_pgpio0.bit.PGPIO_2 = GPIO_ID_EMUM_GPIO;
			top_reg_pgpio0.bit.PGPIO_3 = GPIO_ID_EMUM_GPIO;
			top_reg_pgpio0.bit.PGPIO_4 = GPIO_ID_EMUM_GPIO;
			top_reg_pgpio0.bit.PGPIO_5 = GPIO_ID_EMUM_GPIO;
			top_reg_pgpio0.bit.PGPIO_6 = GPIO_ID_EMUM_GPIO;
			top_reg_pgpio0.bit.PGPIO_7 = GPIO_ID_EMUM_GPIO;
			top_reg_pgpio0.bit.PGPIO_8 = GPIO_ID_EMUM_GPIO;
			top_reg_pgpio0.bit.PGPIO_9 = GPIO_ID_EMUM_GPIO;
			top_reg_pgpio0.bit.PGPIO_10 = GPIO_ID_EMUM_GPIO;
			top_reg_pgpio0.bit.PGPIO_11 = GPIO_ID_EMUM_GPIO;
			top_reg_pgpio0.bit.PGPIO_12 = GPIO_ID_EMUM_FUNC; //PXCLK
			top_reg_sgpio0.bit.SGPIO_2 = GPIO_ID_EMUM_FUNC; //VD
			top_reg_sgpio0.bit.SGPIO_3 = GPIO_ID_EMUM_FUNC; //HD
			break;

		case PIN_SENSOR_CFG_CCIR8BITS:
			top_reg3.bit.SENSOR = SENSOR_ENUM_CCIR8BITS;
			//SN_Y/C0:C7
			top_reg_hgpio0.bit.HSIGPIO_2 = GPIO_ID_EMUM_GPIO;
			top_reg_hgpio0.bit.HSIGPIO_3 = GPIO_ID_EMUM_GPIO;
			top_reg_hgpio0.bit.HSIGPIO_4 = GPIO_ID_EMUM_GPIO;
			top_reg_hgpio0.bit.HSIGPIO_5 = GPIO_ID_EMUM_GPIO;
			top_reg_hgpio0.bit.HSIGPIO_6 = GPIO_ID_EMUM_GPIO;
			top_reg_hgpio0.bit.HSIGPIO_7 = GPIO_ID_EMUM_GPIO;
			top_reg_hgpio0.bit.HSIGPIO_8 = GPIO_ID_EMUM_GPIO;
			top_reg_hgpio0.bit.HSIGPIO_9 = GPIO_ID_EMUM_GPIO;

			top_reg_sgpio0.bit.SGPIO_1 = GPIO_ID_EMUM_FUNC; //PXCLK
			top_reg_sgpio0.bit.SGPIO_2 = GPIO_ID_EMUM_FUNC; //VD
			top_reg_sgpio0.bit.SGPIO_3 = GPIO_ID_EMUM_FUNC; //HD
			top_reg_sgpio0.bit.SGPIO_4 = GPIO_ID_EMUM_FUNC; //Field
			break;

		case PIN_SENSOR_CFG_CCIR16BITS:
			top_reg3.bit.SENSOR = SENSOR_ENUM_CCIR16BITS;
			//SN_Y0:Y7    CCIR-IN1
			top_reg_hgpio0.bit.HSIGPIO_2 = GPIO_ID_EMUM_GPIO;
			top_reg_hgpio0.bit.HSIGPIO_3 = GPIO_ID_EMUM_GPIO;
			top_reg_hgpio0.bit.HSIGPIO_4 = GPIO_ID_EMUM_GPIO;
			top_reg_hgpio0.bit.HSIGPIO_5 = GPIO_ID_EMUM_GPIO;
			top_reg_hgpio0.bit.HSIGPIO_6 = GPIO_ID_EMUM_GPIO;
			top_reg_hgpio0.bit.HSIGPIO_7 = GPIO_ID_EMUM_GPIO;
			top_reg_hgpio0.bit.HSIGPIO_8 = GPIO_ID_EMUM_GPIO;
			top_reg_hgpio0.bit.HSIGPIO_9 = GPIO_ID_EMUM_GPIO;
			//SN_C0:C7    CCIR-IN2
			top_reg_pgpio0.bit.PGPIO_0 = GPIO_ID_EMUM_FUNC;
			top_reg_pgpio0.bit.PGPIO_1 = GPIO_ID_EMUM_FUNC;
			top_reg_pgpio0.bit.PGPIO_2 = GPIO_ID_EMUM_FUNC;
			top_reg_pgpio0.bit.PGPIO_3 = GPIO_ID_EMUM_FUNC;
			top_reg_pgpio0.bit.PGPIO_4 = GPIO_ID_EMUM_FUNC;
			top_reg_pgpio0.bit.PGPIO_5 = GPIO_ID_EMUM_FUNC;
			top_reg_pgpio0.bit.PGPIO_6 = GPIO_ID_EMUM_FUNC;
			top_reg_pgpio0.bit.PGPIO_7 = GPIO_ID_EMUM_FUNC;

			top_reg_sgpio0.bit.SGPIO_1 = GPIO_ID_EMUM_FUNC; //PXCLK
			top_reg_sgpio0.bit.SGPIO_2 = GPIO_ID_EMUM_FUNC; //VD
			top_reg_sgpio0.bit.SGPIO_3 = GPIO_ID_EMUM_FUNC; //HD
			top_reg_sgpio0.bit.SGPIO_4 = GPIO_ID_EMUM_FUNC; //Field
			break;


		case PIN_SENSOR_CFG_MIPI:
			top_reg3.bit.SENSOR = SENSOR_ENUM_CSIMODE;
			mipi_lvds_sensor[0] = PIN_SENSOR_CFG_MIPI;
			break;

		case PIN_SENSOR_CFG_LVDS:
			top_reg3.bit.SENSOR = SENSOR_ENUM_CSIMODE;
			mipi_lvds_sensor[0] = PIN_SENSOR_CFG_LVDS;
			break;

		default:
			pr_err("%s no bus width assigned: 0x%x\r\n", __FUNCTION__, config);
			return E_PAR;
		}

		// Don't need check I2C2_2, because VD/HD is checked first
		// When sensor is LVDS/MIPI and LVDS_VDHD is selected
		if ((mipi_lvds_sensor[0] != 0) && (config & PIN_SENSOR_CFG_LVDS_VDHD)) {
			// Assing LVDS VD/HD when project layer select this config
			top_reg3.bit.SEN_VSHS = VD_HD_SEL_ENUM_SIE_VDHD;
			top_reg_sgpio0.bit.SGPIO_2 = GPIO_ID_EMUM_FUNC; //XVS
			top_reg_sgpio0.bit.SGPIO_3 = GPIO_ID_EMUM_FUNC; //XHS
		}

		if (config & PIN_SENSOR_CFG_MCLK) {
			top_reg_sgpio0.bit.SGPIO_0 = GPIO_ID_EMUM_FUNC;
			top_reg3.bit.SEN_MCLK = SENMCLK_SEL_ENUM_MCLK;
			mclk_sensor[0] = 1;
		}

		if (config & PIN_SENSOR_CFG_MCLK_2ND) {
			//top_reg_sgpio0.bit.HSIGPIO_11 = GPIO_ID_EMUM_FUNC;
			//top_reg3.bit.SEN_MCLK = SENMCLK_SEL_ENUM_MCLK_2ND;
			//mclk_sensor[1] = 1;
			//52x compatible - donot support this function
			pr_err("Not support PIN_SENSOR_CFG_MCLK_2ND\r\n");
			return E_PAR;
		}

		if (config & PIN_SENSOR_CFG_MCLK_3RD) {
			//top_reg_hgpio0.bit.HSIGPIO_9 = GPIO_ID_EMUM_FUNC;
			//top_reg3.bit.SEN_MCLK = SENMCLK_SEL_ENUM_MCLK_3RD;
			//mclk_sensor[2] = 1;
			//52x compatible - donot support this function
			pr_err("Not support PIN_SENSOR_CFG_MCLK_3RD\r\n");
			return E_PAR;
		}

		if (config & PIN_SENSOR_CFG_MCLK2) {
			top_reg_sgpio0.bit.SGPIO_1 = GPIO_ID_EMUM_FUNC;
			top_reg3.bit.SEN_MCLK2 = SENMCLK_SEL_ENUM_MCLK;
			mclk2_sensor[0][0] = 1;
		}

		if (config & (PIN_SENSOR_CFG_SPCLK | PIN_SENSOR_CFG_SPCLK_2ND |PIN_SENSOR_CFG_SPCLK_3RD)) {
			if (config & PIN_SENSOR_CFG_SPCLK_2ND) {
				//check PICNT3_1
				if (top_reg7.bit.PI_CNT3 == PICNT_ENUM_PICNT) {
					pr_err("SP_CLK_2 conflict with PICNT3_1\r\n");
					return E_OBJ;
				}

				//check I2S_MCLK
				if (top_reg6.bit.AUDIO_MCLK == AUDIO_MCLK_MCLK) {
					pr_err("SP_CLK_2 conflict with AUDIO MCLK\r\n");
					return E_OBJ;
				}

				top_reg_lgpio0.bit.LGPIO_23 = GPIO_ID_EMUM_FUNC;
				top_reg3.bit.SP_CLK = SP_CLK_SEL_ENUM_SP_2_CLK;
			}
			else if (config & PIN_SENSOR_CFG_SPCLK_3RD) {

				if(top_reg7.bit.PWM4 == PWM_ENUM_PWM_4TH) {
					pr_err("SP_CLK_3 conflict with PWM4_4\r\n");
					return E_OBJ;
				}

				top_reg_dgpio0.bit.DGPIO_3 = GPIO_ID_EMUM_FUNC;
				top_reg3.bit.SP_CLK = SP_CLK_SEL_ENUM_SP_3_CLK;

			} else {
				//check PICNT3_1
				if (top_reg7.bit.PI_CNT3 == PICNT_ENUM_PICNT) {
					pr_err("SP_CLK_1 conflict with PICNT3_1\r\n");
					return E_OBJ;
				}

				if (top_reg5.bit.SPI3 == SPI3_1ST_PINMUX) {
					pr_err("SP_CLK_1 conflict with SPI3_1 \r\n");
					return E_OBJ;
				}

				if(top_reg9.bit.UART2 == UART2_ENUM_1ST_PINMUX) {
					pr_err("SP_CLK_1 conflict with UART2_1 \r\n");
					return E_OBJ;
				}

				top_reg_pgpio0.bit.PGPIO_17 = GPIO_ID_EMUM_FUNC;
				top_reg3.bit.SP_CLK = SP_CLK_SEL_ENUM_SPCLK;
			}
		}

		if (config & (PIN_SENSOR_CFG_SP2CLK | PIN_SENSOR_CFG_SP2CLK_2ND|PIN_SENSOR_CFG_SP2CLK_3RD)) {
			if (config & PIN_SENSOR_CFG_SP2CLK_2ND) {

				if (top_reg1.bit.SDIO3_EN == SDIO3_EN) {
					pr_err("SP_CLK2 conflict with SDIO3\r\n");
					return E_OBJ;
				}

				top_reg_cgpio0.bit.CGPIO_9 = GPIO_ID_EMUM_FUNC;
				top_reg6.bit.SP2_CLK = SP2_CLK_SEL_ENUM_SP2_2_CLK;
			}
			else if (config & PIN_SENSOR_CFG_SP2CLK_3RD) {

				if(top_reg7.bit.PWM5 == PWM_ENUM_PWM_4TH) {
					pr_err("SP_CLK2_3 conflict with PWM5_4\r\n");
					return E_OBJ;
                }

				top_reg_dgpio0.bit.DGPIO_4 = GPIO_ID_EMUM_FUNC;
				top_reg6.bit.SP2_CLK = SP2_CLK_SEL_ENUM_SP2_3_CLK;
			}else {
				//check PICNT1_2
				if (top_reg7.bit.PI_CNT == PICNT_ENUM_PICNT2) {
					pr_err("SP_CLK2 conflict with PICNT1_2\r\n");
					return E_OBJ;
				}

				if (top_reg5.bit.SPI3 == SPI3_1ST_PINMUX) {
					pr_err("SP_CLK2 conflict with SPI3_1 \r\n");
					return E_OBJ;
				}

				if(top_reg9.bit.UART2 == UART2_ENUM_1ST_PINMUX) {
					pr_err("SP_CLK2 conflict with UART2_1 \r\n");
					return E_OBJ;
				}

				top_reg_pgpio0.bit.PGPIO_18 = GPIO_ID_EMUM_FUNC;
				top_reg6.bit.SP2_CLK = SP2_CLK_SEL_ENUM_SP2CLK;
			}
		}

	}

	return E_OK;
}

static int pinmux_config_sensor2(uint32_t config)
{
	uint32_t tmp;

	if (config == PIN_SENSOR2_CFG_NONE) {
	} else {
		mipi_lvds_sensor[1] = 0;
		tmp = config & (PIN_SENSOR2_CFG_CCIR8BITS | PIN_SENSOR2_CFG_12BITS | \
				PIN_SENSOR2_CFG_CCIR16BITS | PIN_SENSOR2_CFG_MIPI | PIN_SENSOR2_CFG_LVDS);
		switch (tmp) {
		case PIN_SENSOR2_CFG_CCIR8BITS:
		case (PIN_SENSOR2_CFG_CCIR8BITS|PIN_SENSOR2_CFG_MIPI):
			top_reg_pgpio0.bit.PGPIO_0 = GPIO_ID_EMUM_GPIO;
			top_reg_pgpio0.bit.PGPIO_1 = GPIO_ID_EMUM_GPIO;
			top_reg_pgpio0.bit.PGPIO_2 = GPIO_ID_EMUM_GPIO;
			top_reg_pgpio0.bit.PGPIO_3 = GPIO_ID_EMUM_GPIO;
			top_reg_pgpio0.bit.PGPIO_4 = GPIO_ID_EMUM_GPIO;
			top_reg_pgpio0.bit.PGPIO_5 = GPIO_ID_EMUM_GPIO;
			top_reg_pgpio0.bit.PGPIO_6 = GPIO_ID_EMUM_GPIO;
			top_reg_pgpio0.bit.PGPIO_7 = GPIO_ID_EMUM_GPIO;
			top_reg_pgpio0.bit.PGPIO_8 = GPIO_ID_EMUM_FUNC; // SN2_PXCLK
			top_reg_pgpio0.bit.PGPIO_9 = GPIO_ID_EMUM_GPIO; // SN2_XVS
			top_reg_pgpio0.bit.PGPIO_10 = GPIO_ID_EMUM_GPIO; // SN2_XHS

			if (config & PIN_SENSOR2_CFG_CCIR8BITS_FIELD) {
				top_reg_pgpio0.bit.PGPIO_11 = GPIO_ID_EMUM_FUNC; // SN2_Field
			} else {
				top_reg_pgpio0.bit.PGPIO_11 = GPIO_ID_EMUM_GPIO; // SN2_Field
			}

                        if (config & PIN_SENSOR2_CFG_CCIR8BITS_VDHD) {
				top_reg_pgpio0.bit.PGPIO_9 = GPIO_ID_EMUM_FUNC; // SN2_XVS
				top_reg_pgpio0.bit.PGPIO_10 = GPIO_ID_EMUM_FUNC; // SN2_XHS
			}

			top_reg3.bit.SENSOR2 = SENSOR2_ENUM_CCIR8BITS;

			if(tmp&PIN_SENSOR2_CFG_MIPI){
				top_reg3.bit.SENSOR = SENSOR_ENUM_CSIMODE;
				mipi_lvds_sensor[1] = PIN_SENSOR2_CFG_MIPI;
			}
			break;
		case PIN_SENSOR2_CFG_12BITS:
			top_reg_pgpio0.bit.PGPIO_0 = GPIO_ID_EMUM_GPIO;
			top_reg_pgpio0.bit.PGPIO_1 = GPIO_ID_EMUM_GPIO;
			top_reg_pgpio0.bit.PGPIO_2 = GPIO_ID_EMUM_GPIO;
			top_reg_pgpio0.bit.PGPIO_3 = GPIO_ID_EMUM_GPIO;
			top_reg_pgpio0.bit.PGPIO_4 = GPIO_ID_EMUM_GPIO;
			top_reg_pgpio0.bit.PGPIO_5 = GPIO_ID_EMUM_GPIO;
			top_reg_pgpio0.bit.PGPIO_6 = GPIO_ID_EMUM_GPIO;
			top_reg_pgpio0.bit.PGPIO_7 = GPIO_ID_EMUM_GPIO;
			top_reg_pgpio0.bit.PGPIO_8 = GPIO_ID_EMUM_GPIO;
			top_reg_pgpio0.bit.PGPIO_9 = GPIO_ID_EMUM_GPIO;
			top_reg_pgpio0.bit.PGPIO_10 = GPIO_ID_EMUM_GPIO;
			top_reg_pgpio0.bit.PGPIO_11 = GPIO_ID_EMUM_GPIO;
			top_reg_pgpio0.bit.PGPIO_12 = GPIO_ID_EMUM_FUNC; //PXCLK
			top_reg_sgpio0.bit.SGPIO_4 = GPIO_ID_EMUM_FUNC; //VD
			top_reg_sgpio0.bit.SGPIO_5 = GPIO_ID_EMUM_FUNC; //HD

			top_reg3.bit.SENSOR2 = SENSOR2_ENUM_12BITS;

			break;
		case PIN_SENSOR2_CFG_CCIR16BITS:
			if (top_reg3.bit.SENSOR != SENSOR_ENUM_GPIO) {
				pr_err("SENSOR2 CCIR 16 conflict with SENSOR\r\n");
				return E_OBJ;
			}
			break;
		case PIN_SENSOR2_CFG_MIPI:
			top_reg3.bit.SENSOR = SENSOR_ENUM_CSIMODE;
			mipi_lvds_sensor[1] = PIN_SENSOR2_CFG_MIPI;
			break;

		case PIN_SENSOR2_CFG_LVDS:
			top_reg3.bit.SENSOR = SENSOR_ENUM_CSIMODE;
			mipi_lvds_sensor[1] = PIN_SENSOR2_CFG_LVDS;
			break;

		default:
			pr_err("%s no bus width assigned: 0x%x\r\n", __FUNCTION__, config);
			break;
		}

		if ((mipi_lvds_sensor[1] != 0) && (config & PIN_SENSOR2_CFG_LVDS_VDHD)) {
			// Assing LVDS VD/HD when project layer select this config
			top_reg3.bit.SEN2_VSHS = VD_HD_SEL_ENUM_SIE_VDHD;
			top_reg_sgpio0.bit.SGPIO_4 = GPIO_ID_EMUM_FUNC; //XVS
			top_reg_sgpio0.bit.SGPIO_5 = GPIO_ID_EMUM_FUNC; //XHS
		}

		if (config & PIN_SENSOR2_CFG_SN_MCLK) {
			top_reg_sgpio0.bit.SGPIO_0 = GPIO_ID_EMUM_FUNC;
			top_reg3.bit.SEN_MCLK = SENMCLK_SEL_ENUM_MCLK;
			sn_mclk_en[1] = 1;
		}

		if (config & PIN_SENSOR2_CFG_MCLK) {
			top_reg_sgpio0.bit.SGPIO_1 = GPIO_ID_EMUM_FUNC;
			top_reg3.bit.SEN_MCLK2 = SENMCLK_SEL_ENUM_MCLK;
			mclk2_sensor[1][0] = 1;
		}

		if (config & PIN_SENSOR2_CFG_MCLK_2ND) {
			top_reg_sgpio0.bit.SGPIO_12 = GPIO_ID_EMUM_FUNC;
			top_reg3.bit.SEN_MCLK2 = SENMCLK_SEL_ENUM_MCLK_2ND;
			mclk2_sensor[1][1] = 1;
		}

		if (config & PIN_SENSOR2_CFG_SN3_MCLK) {
			top_reg_pgpio0.bit.PGPIO_12 = GPIO_ID_EMUM_FUNC;
			top_reg3.bit.SN3_MCLK = SENMCLK_SEL_ENUM_MCLK;
		}
	}

	return E_OK;
}

static int pinmux_config_mipi_lvds(uint32_t config)
{
	if (config == PIN_MIPI_LVDS_CFG_NONE) {
	} else {
		// check clk lane 0
		if (config & PIN_MIPI_LVDS_CFG_CLK0) {
			if (top_reg3.bit.SENSOR == SENSOR_ENUM_12BITS_1ST) {
				pr_err("MIPI CLK0 conflict with SENSOR 12 or SENSOR2 10 bits\r\n");
				return E_OBJ;
			}
			top_reg_hgpio0.bit.HSIGPIO_4 = GPIO_ID_EMUM_FUNC;
			top_reg_hgpio0.bit.HSIGPIO_5 = GPIO_ID_EMUM_FUNC;
			pad_set_pull_updown(PAD_PIN_HSIGPIO4, PAD_NONE);
			pad_set_pull_updown(PAD_PIN_HSIGPIO5, PAD_NONE);
		}

		// check clk lane 1
		if (config & PIN_MIPI_LVDS_CFG_CLK1) {
			top_reg_hgpio0.bit.HSIGPIO_10 = GPIO_ID_EMUM_FUNC;
			top_reg_hgpio0.bit.HSIGPIO_11 = GPIO_ID_EMUM_FUNC;
			pad_set_pull_updown(PAD_PIN_HSIGPIO10, PAD_NONE);
			pad_set_pull_updown(PAD_PIN_HSIGPIO11, PAD_NONE);
		}

		// check data lane 0
		if (config & PIN_MIPI_LVDS_CFG_DAT0) {
			if (top_reg3.bit.SENSOR == SENSOR_ENUM_12BITS_1ST) {
				pr_err("MIPI D0 conflict with SENSOR 12 or SENSOR2 10 bits\r\n");
				return E_OBJ;
			}
			top_reg_hgpio0.bit.HSIGPIO_0 = GPIO_ID_EMUM_FUNC;
			top_reg_hgpio0.bit.HSIGPIO_1 = GPIO_ID_EMUM_FUNC;
			pad_set_pull_updown(PAD_PIN_HSIGPIO0, PAD_NONE);
			pad_set_pull_updown(PAD_PIN_HSIGPIO1, PAD_NONE);
		}

		if (config & (PIN_MIPI_LVDS_CFG_DAT1 | PIN_MIPI_LVDS_CFG_DAT2 | PIN_MIPI_LVDS_CFG_DAT3 | PIN_MIPI_LVDS_CFG_DAT4)) {
			if (top_reg3.bit.SENSOR != SENSOR_ENUM_CSIMODE) {
				pr_err("MIPI D1~4, CK0 conflict with SENSOR\r\n");
				return E_OBJ;
			}

			if (config & PIN_MIPI_LVDS_CFG_DAT1) {
				top_reg_hgpio0.bit.HSIGPIO_2 = GPIO_ID_EMUM_FUNC;
				top_reg_hgpio0.bit.HSIGPIO_3 = GPIO_ID_EMUM_FUNC;
				//pull down disabled
				pad_set_pull_updown(PAD_PIN_HSIGPIO2, PAD_NONE);
				pad_set_pull_updown(PAD_PIN_HSIGPIO3, PAD_NONE);
			}
			if (config & PIN_MIPI_LVDS_CFG_DAT2) {
				top_reg_hgpio0.bit.HSIGPIO_6 = GPIO_ID_EMUM_FUNC;
				top_reg_hgpio0.bit.HSIGPIO_7 = GPIO_ID_EMUM_FUNC;
				//pull down disabled
				pad_set_pull_updown(PAD_PIN_HSIGPIO6, PAD_NONE);
				pad_set_pull_updown(PAD_PIN_HSIGPIO7, PAD_NONE);
			}
			if (config & PIN_MIPI_LVDS_CFG_DAT3) {
				top_reg_hgpio0.bit.HSIGPIO_8 = GPIO_ID_EMUM_FUNC;
				top_reg_hgpio0.bit.HSIGPIO_9 = GPIO_ID_EMUM_FUNC;
				//pull down disabled
				pad_set_pull_updown(PAD_PIN_HSIGPIO8, PAD_NONE);
				pad_set_pull_updown(PAD_PIN_HSIGPIO9, PAD_NONE);
			}
			if (config & PIN_MIPI_LVDS_CFG_DAT4) {
				pr_err("Only support 2C4D\r\n");
				return E_OBJ;
			}
		}
		// check data lane 5~9
		if (config & (PIN_MIPI_LVDS_CFG_DAT5 | PIN_MIPI_LVDS_CFG_DAT6 | PIN_MIPI_LVDS_CFG_DAT7 | PIN_MIPI_LVDS_CFG_DAT8 | PIN_MIPI_LVDS_CFG_DAT9)) {
			pr_err("Only support 2C4D\r\n");
			return E_OBJ;
		}

	}

	return E_OK;
}

static int pinmux_config_i2c(uint32_t config)
{
	if (config == PIN_I2C_CFG_NONE) {
	} else {
		if (config & PIN_I2C_CFG_CH1) {
			if (top_reg5.bit.SIFCH0 == SIFCH_ENUM_SIF) {
				pr_err("I2C_1 conflict with SIF_CH0\r\n");
				return E_OBJ;
			}

			if ((top_reg7.bit.PWM6 == PWM_ENUM_PWM_3RD) ||
				(top_reg7.bit.PWM7 == PWM_ENUM_PWM_3RD)){

				pr_err("I2C_1 conflict with PWM6_3/PWM7_3 \r\n");
				return E_OBJ;
			}

			top_reg_sgpio0.bit.SGPIO_7 = GPIO_ID_EMUM_FUNC;
			top_reg_sgpio0.bit.SGPIO_8 = GPIO_ID_EMUM_FUNC;
			top_reg5.bit.I2C = I2C_ENUM_I2C;
		}

		if (config & PIN_I2C_CFG_CH1_2ND_PINMUX) {
			pr_err("not support I2C_2\r\n");
			return E_OBJ;
			//if(top_reg3.bit.SEN_MCLK == SENMCLK_SEL_ENUM_MCLK_2ND) {
			//	pr_err("I2C_2 conflict with SN_MCLK_2 \r\n");
			//	return E_OBJ;
			//}

			//top_reg_hgpio0.bit.HSIGPIO_10 = GPIO_ID_EMUM_FUNC;
			//top_reg_hgpio0.bit.HSIGPIO_11 = GPIO_ID_EMUM_FUNC;

			//top_reg5.bit.I2C = I2C_ENUM_I2C_2ND;
		}

		if (config & PIN_I2C_CFG_CH2) {
			if (top_reg5.bit.SPI == SPI_2ND_PINMUX) {
				pr_err("I2C2_1 conflict with SPI_2\r\n");
				return E_OBJ;
			}

			if (top_reg5.bit.SIFCH1 == SIFCH_ENUM_SIF) {
				pr_err("I2C2_1 conflict with SIF_CH1\r\n");
				return E_OBJ;
			}

			top_reg_sgpio0.bit.SGPIO_10 = GPIO_ID_EMUM_FUNC;
			top_reg_sgpio0.bit.SGPIO_11 = GPIO_ID_EMUM_FUNC;

			top_reg5.bit.I2C2 = I2C_ENUM_I2C;
		}

		if (config & PIN_I2C_CFG_CH2_2ND_PINMUX) {
			if (top_reg5.bit.SPI == SPI_1ST_PINMUX) {
				pr_err("I2C2_2 conflict with SPI_1\r\n");
				return E_OBJ;
			}

			if (top_reg1.bit.SDIO2_EXIST == SDIO2_EXIST_EN) {
				pr_err("I2C2_2 conflict with SDIO2\r\n");
				return E_OBJ;
			}

			if ((top_reg7.bit.PWM6 == PWM_ENUM_PWM_2ND) ||
				(top_reg7.bit.PWM7 == PWM_ENUM_PWM_2ND)){

				pr_err("I2C2_2 conflict with PWM6_2/PWM7_2 \r\n");
				return E_OBJ;
			}

			top_reg_cgpio0.bit.CGPIO_17 = GPIO_ID_EMUM_FUNC;
			top_reg_cgpio0.bit.CGPIO_18 = GPIO_ID_EMUM_FUNC;

			top_reg5.bit.I2C2 = I2C_ENUM_I2C_2ND;
		}

		if (config & PIN_I2C_CFG_CH3) {
			top_reg_pgpio0.bit.PGPIO_22 = GPIO_ID_EMUM_FUNC;
			top_reg_pgpio0.bit.PGPIO_21 = GPIO_ID_EMUM_FUNC;
			top_reg5.bit.I2C3 = I2C_ENUM_I2C;
		}

		if (config & PIN_I2C_CFG_CH3_2ND_PINMUX) {
			if (top_reg1.bit.SDIO_EXIST == SDIO_EXIST_EN) {
				pr_err("I2C3_2 conflict with to SDIO\r\n");
				return E_OBJ;
			}

			if (top_reg6.bit.DIGITAL_MIC == DMIC_ENUM_DMIC_3RD_PINMUX) {
				pr_err("I2C3_2 conflict with DMCLK_3 \r\n");
				return E_OBJ;
			}

			if (top_reg5.bit.SDP == SDP_1ST_PINMUX) {
				pr_err("I2C3_2 conflict with SDP_1 \r\n");
				return E_OBJ;
			}

			if ((top_reg7.bit.PWM4 == PWM_ENUM_PWM_2ND) ||
				(top_reg7.bit.PWM5 == PWM_ENUM_PWM_2ND)){

				pr_err("I2C3_2 conflict with PWM4_2/PWM5_2 \r\n");
				return E_OBJ;
			}

			if (top_reg5.bit.SPI3 == SPI3_2ND_PINMUX) {
				pr_err("I2C3_2 conflict with SPI3_2\r\n");
				return E_OBJ;
			}

			top_reg_cgpio0.bit.CGPIO_11 = GPIO_ID_EMUM_FUNC;
			top_reg_cgpio0.bit.CGPIO_12 = GPIO_ID_EMUM_FUNC;

			top_reg5.bit.I2C3 = I2C_ENUM_I2C_2ND;
		}

		if (nvt_get_chip_id() == CHIP_NA51084) {

			if (config & PIN_I2C_CFG_CH4) {

				if(top_reg7.bit.PI_CNT == PICNT_ENUM_PICNT) {
					pr_err("I2C4_1 conflict with PICNT_1\r\n");
					return E_PAR;
				}

				top_reg_pgpio0.bit.PGPIO_11 = GPIO_ID_EMUM_FUNC;
				top_reg_pgpio0.bit.PGPIO_12 = GPIO_ID_EMUM_FUNC;
				top_reg5.bit.I2C4 = I2C_ENUM_I2C;
			}

			if (config & PIN_I2C_CFG_CH4_2ND_PINMUX) {

				if (top_reg9.bit.UART4_CTSRTS == UART_CTSRTS_PINMUX) {
					pr_err("I2C4_2 conflict with UART4_2 CTSRTS\r\n");
					return E_PAR;
				}

				if(top_reg1.bit.SDIO3_BUS_WIDTH == SDIO3_BUS_WIDTH_8BITS) {
					pr_err("I2C4_2 conflict with SDIO3 8bit\r\n");
					return E_PAR;
				}

				if(top_reg4.bit.SPI4 == SPI4_2ND_PINMUX) {
					pr_err("I2C4_2 conflict with SPI4_2\r\n");
					return E_PAR;
				}

				if (	(top_reg7.bit.PWM2 == PWM_ENUM_PWM_4TH) ||
					(top_reg7.bit.PWM3 == PWM_ENUM_PWM_4TH)) {

					pr_err("I2C4_2 conflict with PWM2_4 ~PWM3_4 \r\n");
					return E_PAR;
				}

				top_reg_cgpio0.bit.CGPIO_6 = GPIO_ID_EMUM_FUNC;
				top_reg_cgpio0.bit.CGPIO_7 = GPIO_ID_EMUM_FUNC;
				top_reg5.bit.I2C4 = I2C_ENUM_I2C_2ND;
			}

			if (config & PIN_I2C_CFG_CH5) {
				top_reg_pgpio0.bit.PGPIO_2 = GPIO_ID_EMUM_FUNC;
				top_reg_pgpio0.bit.PGPIO_3 = GPIO_ID_EMUM_FUNC;
				top_reg4.bit.I2C5 = I2C_ENUM_I2C;
			}

			if (config & PIN_I2C_CFG_CH5_2ND_PINMUX) {

				if(top_reg9.bit.UART5_CTSRTS != UART_CTSRTS_GPIO) {
					pr_err("I2C5_2 conflict with UART5_2 CTSRTS \r\n");
					return E_PAR;
				}

				top_reg_dgpio0.bit.DGPIO_17 = GPIO_ID_EMUM_FUNC;
				top_reg_dgpio0.bit.DGPIO_18 = GPIO_ID_EMUM_FUNC;

				top_reg4.bit.I2C5 = I2C_ENUM_I2C_2ND;
			}
		}

	}

	return E_OK;
}

static int pinmux_config_sif(uint32_t config)
{
	if (config == PIN_SIF_CFG_NONE) {
	} else {
		if (config & PIN_SIF_CFG_CH0) {
			if (top_reg5.bit.I2C == I2C_ENUM_I2C) {
				pr_err("SIF_CH0 conflict with I2C_1\r\n");
				return E_OBJ;
			}

			if (top_reg1.bit.EXTROM_EXIST == EXTROM_EXIST_EN) {
				pr_err("SIF_CH0 conflict with BMC\r\n");
				return E_OBJ;
			}

			if (top_reg9.bit.UART3 == UART3_ENUM_4TH_PINMUX) {
				pr_err("SIF_CH0 conflict with UART3_4\r\n");
				return E_OBJ;
			}

			if ((top_reg7.bit.PWM5 == PWM_ENUM_PWM_3RD) ||
				(top_reg7.bit.PWM6 == PWM_ENUM_PWM_3RD) ||
				(top_reg7.bit.PWM7 == PWM_ENUM_PWM_3RD)){

				pr_err("SIF_CH0 conflict with PWM5_3/PWM6_3/PWM7_3 \r\n");
				return E_OBJ;
			}

			top_reg_sgpio0.bit.SGPIO_6 = GPIO_ID_EMUM_FUNC;
			top_reg_sgpio0.bit.SGPIO_7 = GPIO_ID_EMUM_FUNC;
			top_reg_sgpio0.bit.SGPIO_8 = GPIO_ID_EMUM_FUNC;

			top_reg5.bit.SIFCH0 = SIFCH_ENUM_SIF;
		}

		if (config & (PIN_SIF_CFG_CH1|PIN_SIF_CFG_CH1_2ND_PINMUX)) {
			if (top_reg5.bit.SPI == SPI_2ND_PINMUX) {
				pr_err("SIF_CH1 conflict with SPI_2\r\n");
				return E_OBJ;
			}

			if (top_reg5.bit.I2C2 == I2C_ENUM_I2C) {
				pr_err("SIF_CH1 conflict with I2C2_1\r\n");
				return E_OBJ;
			}

			if (config & PIN_SIF_CFG_CH1_2ND_PINMUX) {
				pr_err("SIF_CH1 no 2ND PINMUX\r\n");
				return E_OBJ;
			} else {
				top_reg_sgpio0.bit.SGPIO_9 = GPIO_ID_EMUM_FUNC;
				top_reg_sgpio0.bit.SGPIO_10 = GPIO_ID_EMUM_FUNC;
				top_reg_sgpio0.bit.SGPIO_11 = GPIO_ID_EMUM_FUNC;
			}

			top_reg5.bit.SIFCH1 = SIFCH_ENUM_SIF;
		}

		if (config & (PIN_SIF_CFG_CH2|PIN_SIF_CFG_CH2_2ND_PINMUX|PIN_SIF_CFG_CH2_3RD_PINMUX)) {
			if (config & PIN_SIF_CFG_CH2) {
				if (top_reg3.bit.SP_CLK == SP_CLK_SEL_ENUM_SP_2_CLK) {
					pr_err("SIF_CH2_1 conflict with SP_CLK_2\r\n");
					return E_OBJ;
				}

				top_reg_lgpio0.bit.LGPIO_22 = GPIO_ID_EMUM_FUNC;
				top_reg_lgpio0.bit.LGPIO_23 = GPIO_ID_EMUM_FUNC;
				top_reg_lgpio0.bit.LGPIO_24 = GPIO_ID_EMUM_FUNC;

				top_reg5.bit.SIFCH2 = SIFCH_ENUM_SIF;
			} else if (config & PIN_SIF_CFG_CH2_2ND_PINMUX) {
				if (top_reg5.bit.SPI3 == SPI3_1ST_PINMUX) {
					pr_err("SIF_CH2_2 conflict with SPI3_1\r\n");
					return E_OBJ;
				}
				top_reg_pgpio0.bit.PGPIO_16 = GPIO_ID_EMUM_FUNC;
				top_reg_pgpio0.bit.PGPIO_19 = GPIO_ID_EMUM_FUNC;
				top_reg_pgpio0.bit.PGPIO_20 = GPIO_ID_EMUM_FUNC;

				top_reg5.bit.SIFCH2 = SIFCH_ENUM_SIF_2ND;
			} else if (config & PIN_SIF_CFG_CH2_3RD_PINMUX) {
				if ((top_reg6.bit.PWM8 == PWM_ENUM_PWM_4TH) ||
					(top_reg6.bit.PWM9 == PWM_ENUM_PWM_4TH) ||
					(top_reg6.bit.PWM10 == PWM_ENUM_PWM_4TH)){

					pr_err("SIF_CH2_3 conflict with PWM8_4 ~PWM10_4 \r\n");
					return E_OBJ;
				}

				if (top_reg6.bit.DIGITAL_MIC == DMIC_ENUM_DMIC_2ND_PINMUX) {
					pr_err("SIF_CH2_3 conflict with DMCLK_2 \r\n");
					return E_OBJ;
				}

				top_reg_dgpio0.bit.DGPIO_8 = GPIO_ID_EMUM_FUNC;
				top_reg_dgpio0.bit.DGPIO_9 = GPIO_ID_EMUM_FUNC;
				top_reg_dgpio0.bit.DGPIO_10 = GPIO_ID_EMUM_FUNC;

				top_reg5.bit.SIFCH2 = SIFCH_ENUM_SIF_3RD;
			}
		}

		if (config & PIN_SIF_CFG_CH3) {
			if (top_reg5.bit.SPI2 == SPI2_1ST_PINMUX) {
				pr_err("SIF_CH3 conflict with SPI2_1\r\n");
				return E_OBJ;
			}

			if (top_reg9.bit.UART3 == UART3_ENUM_1ST_PINMUX) {
				pr_err("SIF_CH3 conflict with UART3_1\r\n");
				return E_OBJ;
			}

			if(top_reg7.bit.PI_CNT2 == PICNT_ENUM_PICNT) {
				pr_err("SIF_CH3 conflict with PICNT2_1\r\n");
				return E_OBJ;
			}

			top_reg_pgpio0.bit.PGPIO_13 = GPIO_ID_EMUM_FUNC;
			top_reg_pgpio0.bit.PGPIO_14 = GPIO_ID_EMUM_FUNC;
			top_reg_pgpio0.bit.PGPIO_15 = GPIO_ID_EMUM_FUNC;

			top_reg5.bit.SIFCH3 = SIFCH_ENUM_SIF;
		}
	}

	return E_OK;
}

static int pinmux_config_uart(uint32_t config)
{
	if (config == PIN_UART_CFG_NONE) {
	} else if (config & (PIN_UART_CFG_CH1|PIN_UART_CFG_CH2|PIN_UART_CFG_CH3|PIN_UART_CFG_CH4|PIN_UART_CFG_CH5|PIN_UART_CFG_CH6)) {
		if (config & (PIN_UART_CFG_CH1|PIN_UART_CFG_CH1_TX)) {
			top_reg_pgpio0.bit.PGPIO_23 = GPIO_ID_EMUM_FUNC;

			if (config & (PIN_UART_CFG_CH1)) {
				top_reg_pgpio0.bit.PGPIO_24 = GPIO_ID_EMUM_FUNC;
			}
			top_reg9.bit.UART = UART_ENUM_UART;
		}

		if (config & PIN_UART_CFG_CH2) {
			if (config & PIN_UART_CFG_CH2_2ND) {
				if (top_reg1.bit.SDIO_EXIST == SDIO_EXIST_EN) {
					pr_err("UART2 conflict with to SDIO: 0x%x\r\n", config);
					return E_OBJ;
				}

				if (top_reg5.bit.SPI3 == SPI3_2ND_PINMUX) {
					pr_err("UART2 conflict with to SPI3_2: 0x%x\r\n", config);
					return E_OBJ;
				}

				if (config & PIN_UART_CFG_CH2_DIROE) {
					top_reg_cgpio0.bit.CGPIO_14 = GPIO_ID_EMUM_FUNC;

					top_reg9.bit.UART2_CTSRTS = UART_CTSRTS_DIROE;
				}

				if (config & PIN_UART_CFG_CH2_CTSRTS) {
					top_reg_cgpio0.bit.CGPIO_13 = GPIO_ID_EMUM_FUNC;
					top_reg_cgpio0.bit.CGPIO_14 = GPIO_ID_EMUM_FUNC;
					top_reg9.bit.UART2_CTSRTS = UART_CTSRTS_PINMUX;
				}

				top_reg_cgpio0.bit.CGPIO_15 = GPIO_ID_EMUM_FUNC;
				top_reg_cgpio0.bit.CGPIO_16 = GPIO_ID_EMUM_FUNC;
				top_reg9.bit.UART2 = UART2_ENUM_2ND_PINMUX;
			}else if (config & PIN_UART_CFG_CH2_3RD){

				if (nvt_get_chip_id() == CHIP_NA51084) {
					pr_err("CHIP_NA51084 not support UART2_3\r\n");
					return E_OBJ;
				}


				if (config & PIN_UART_CFG_CH2_DIROE) {
					top_reg_pgpio0.bit.PGPIO_2 = GPIO_ID_EMUM_FUNC;

					top_reg9.bit.UART2_CTSRTS = UART_CTSRTS_DIROE;
				}

				if (config & PIN_UART_CFG_CH2_CTSRTS) {
					top_reg_pgpio0.bit.PGPIO_2 = GPIO_ID_EMUM_FUNC;
					top_reg_pgpio0.bit.PGPIO_3 = GPIO_ID_EMUM_FUNC;

					top_reg9.bit.UART2_CTSRTS = UART_CTSRTS_PINMUX;
				}

				top_reg_pgpio0.bit.PGPIO_0 = GPIO_ID_EMUM_FUNC;
				top_reg_pgpio0.bit.PGPIO_1 = GPIO_ID_EMUM_FUNC;

				top_reg9.bit.UART2 = UART2_ENUM_3RD_PINMUX;
			} else {
				if (top_reg6.bit.AUDIO == AUDIO_ENUM_I2S) {
					pr_err("UART2 conflict with to AUDIO: 0x%x\r\n", config);
					return E_OBJ;
				}

				if (top_reg5.bit.SPI3 == SPI3_1ST_PINMUX) {
					pr_err("UART2 conflict with SPI3_1\r\n");
					return E_OBJ;
				}

				if (top_reg5.bit.SIFCH2 == SIFCH_ENUM_SIF_2ND) {
					pr_err("UART2 conflict with SIF_CH2_2\r\n");
					return E_OBJ;
				}

				if (config & PIN_UART_CFG_CH2_CTSRTS) {
					if (top_reg7.bit.PI_CNT2 == PICNT_ENUM_PICNT) {
						pr_err("UART2 conflict with PICNT2_1: 0x%x\r\n", config);
						return E_OBJ;
					}

					top_reg_pgpio0.bit.PGPIO_19 = GPIO_ID_EMUM_FUNC;
					top_reg_pgpio0.bit.PGPIO_20 = GPIO_ID_EMUM_FUNC;
					top_reg9.bit.UART2_CTSRTS = UART_CTSRTS_PINMUX;
				}

				if (config & PIN_UART_CFG_CH2_DIROE) {
					top_reg_pgpio0.bit.PGPIO_19 = GPIO_ID_EMUM_FUNC;

					top_reg9.bit.UART2_CTSRTS = UART_CTSRTS_DIROE;
				}

				top_reg_pgpio0.bit.PGPIO_17 = GPIO_ID_EMUM_FUNC;
				top_reg_pgpio0.bit.PGPIO_18 = GPIO_ID_EMUM_FUNC;
				top_reg9.bit.UART2 = UART2_ENUM_1ST_PINMUX;
			}
		}

		if (config & PIN_UART_CFG_CH3) {
			if (config & PIN_UART_CFG_CH3_2ND) {
				if (top_reg6.bit.PWM8 == PWM_ENUM_PWM) {
					pr_err("UART3_2 conflict with to PWM8: 0x%x\r\n", config);
					return E_OBJ;
				}

				if (top_reg6.bit.PWM9 == PWM_ENUM_PWM) {
					pr_err("UART3_2 conflict with to PWM9: 0x%x\r\n", config);
					return E_OBJ;
				}

				if (top_reg5.bit.SPI2 == SPI2_2ND_PINMUX) {
					pr_err("UART3_2 conflict with to SPI2_2: 0x%x\r\n", config);
					return E_OBJ;
				}

				if (config & PIN_UART_CFG_CH3_DIROE) {
					top_reg_cgpio0.bit.CGPIO_20 = GPIO_ID_EMUM_FUNC;

					top_reg9.bit.UART3_CTSRTS = UART_CTSRTS_DIROE;
				}

				if (config & PIN_UART_CFG_CH3_CTSRTS) {
					if (top_reg6.bit.PWM10 == PWM_ENUM_PWM) {
						pr_err("UART3_2 conflict with to PWM10: 0x%x\r\n", config);
						return E_OBJ;
					}
					if (top_reg6.bit.PWM11 == PWM_ENUM_PWM) {
						pr_err("UART3_2 conflict with to PWM11: 0x%x\r\n", config);
						return E_OBJ;
					}

					top_reg_cgpio0.bit.CGPIO_19 = GPIO_ID_EMUM_FUNC;
					top_reg_cgpio0.bit.CGPIO_20 = GPIO_ID_EMUM_FUNC;

					top_reg9.bit.UART3_CTSRTS = UART_CTSRTS_PINMUX;
				}


				top_reg_cgpio0.bit.CGPIO_21 = GPIO_ID_EMUM_FUNC;
				top_reg_cgpio0.bit.CGPIO_22 = GPIO_ID_EMUM_FUNC;

				top_reg9.bit.UART3 = UART3_ENUM_2ND_PINMUX;
			} else if (config & PIN_UART_CFG_CH3_3RD) {
				if (nvt_get_chip_id() == CHIP_NA51084) {
					pr_err("CHIP_NA51084 not support UART3_3\r\n");
					return E_OBJ;
				}

				if (config & PIN_UART_CFG_CH3_DIROE) {
					top_reg_pgpio0.bit.PGPIO_6 = GPIO_ID_EMUM_FUNC;

					top_reg9.bit.UART3_CTSRTS = UART_CTSRTS_DIROE;
				}

				if (config & PIN_UART_CFG_CH3_CTSRTS) {
					top_reg_pgpio0.bit.PGPIO_6 = GPIO_ID_EMUM_FUNC;
					top_reg_pgpio0.bit.PGPIO_7 = GPIO_ID_EMUM_FUNC;

					top_reg9.bit.UART3_CTSRTS = UART_CTSRTS_PINMUX;
				}

				top_reg_pgpio0.bit.PGPIO_4 = GPIO_ID_EMUM_FUNC;
				top_reg_pgpio0.bit.PGPIO_5 = GPIO_ID_EMUM_FUNC;

				top_reg9.bit.UART3 = UART3_ENUM_3RD_PINMUX;
			} else if (config & PIN_UART_CFG_CH3_4TH) {
				top_reg_sgpio0.bit.SGPIO_4 = GPIO_ID_EMUM_FUNC;
				top_reg_sgpio0.bit.SGPIO_6 = GPIO_ID_EMUM_FUNC;

				top_reg9.bit.UART3 = UART3_ENUM_4TH_PINMUX;
			} else if (config & PIN_UART_CFG_CH3_5TH) {
				top_reg_cgpio0.bit.CGPIO_2 = GPIO_ID_EMUM_FUNC;
				top_reg_cgpio0.bit.CGPIO_3 = GPIO_ID_EMUM_FUNC;

				top_reg9.bit.UART3 = UART3_ENUM_5TH_PINMUX;
			} else {
				if (top_reg5.bit.SPI == SPI_2ND_PINMUX) {
					pr_err("UART3 conflict with to SPI_2: 0x%x\r\n", config);
					return E_OBJ;
				}

				if (top_reg5.bit.SIFCH3 == SIFCH_ENUM_SIF) {
					pr_err("UART3 conflict with to SIF3: 0x%x\r\n", config);
					return E_OBJ;
				}

				if (top_reg6.bit.SP2_CLK == SP2_CLK_SEL_ENUM_SP2_2_CLK) {
					pr_err("UART3 conflict with to SP_CLK2_2: 0x%x\r\n", config);
					return E_OBJ;
				}
				if (config & PIN_UART_CFG_CH3_CTSRTS) {
					top_reg_pgpio0.bit.PGPIO_15 = GPIO_ID_EMUM_FUNC;
					top_reg_pgpio0.bit.PGPIO_16 = GPIO_ID_EMUM_FUNC;

					top_reg9.bit.UART3_CTSRTS = UART_CTSRTS_PINMUX;
				}

				if (config & PIN_UART_CFG_CH3_DIROE) {
					top_reg_pgpio0.bit.PGPIO_15 = GPIO_ID_EMUM_FUNC;

					top_reg9.bit.UART3_CTSRTS = UART_CTSRTS_DIROE;
				}

				top_reg_pgpio0.bit.PGPIO_13 = GPIO_ID_EMUM_FUNC;
				top_reg_pgpio0.bit.PGPIO_14 = GPIO_ID_EMUM_FUNC;

				top_reg9.bit.UART3 = UART3_ENUM_1ST_PINMUX;
			}
		}

		if (config & PIN_UART_CFG_CH4) {
			if (config & PIN_UART_CFG_CH4_2ND) {
				if (config & PIN_UART_CFG_CH4_CTSRTS) {
					if(top_reg5.bit.I2C4 == I2C_ENUM_I2C_2ND) {
						pr_err("UART4_2 RTSCTS conflict with I2C4_2\r\n");
						return E_PAR;
					}

					if (	(top_reg7.bit.PWM2 == PWM_ENUM_PWM_4TH) ||
						(top_reg7.bit.PWM3 == PWM_ENUM_PWM_4TH)) {

						pr_err("UART4_2 RTSCTS conflict with PWM2_4 ~PWM3_4 \r\n");
						return E_PAR;
					}

					top_reg_cgpio0.bit.CGPIO_6 = GPIO_ID_EMUM_FUNC;
					top_reg_cgpio0.bit.CGPIO_7 = GPIO_ID_EMUM_FUNC;
					top_reg9.bit.UART4_CTSRTS = UART_CTSRTS_PINMUX;
				}

				if (config & PIN_UART_CFG_CH4_DIROE) {

					if (top_reg7.bit.PWM2 == PWM_ENUM_PWM_4TH) {

						pr_err("UART4_2 DIROE conflict with PWM2_4 \r\n");
						return E_PAR;
					}

					if(top_reg5.bit.I2C4 == I2C_ENUM_I2C_2ND) {
						pr_err("UART4_2 DIROE conflict with I2C4_2\r\n");
						return E_PAR;
					}

					top_reg_cgpio0.bit.CGPIO_6 = GPIO_ID_EMUM_FUNC;
					top_reg9.bit.UART4_CTSRTS = UART_CTSRTS_DIROE;
				}


				if(top_reg1.bit.SDIO3_BUS_WIDTH == SDIO3_BUS_WIDTH_8BITS) {
					pr_err("UART4_2 conflict with SDIO3 8bit\r\n");
					return E_PAR;
				}

				if(top_reg4.bit.SPI4 == SPI4_2ND_PINMUX) {
					pr_err("UART4_2 conflict with SPI4_2\r\n");
					return E_PAR;
				}

				if (	(top_reg7.bit.PWM0 == PWM_ENUM_PWM_4TH) ||
					(top_reg7.bit.PWM1 == PWM_ENUM_PWM_4TH)) {

					pr_err("UART4_2 conflict with PWM0_4 ~PWM1_4 \r\n");
					return E_PAR;
				}

				top_reg_cgpio0.bit.CGPIO_4 = GPIO_ID_EMUM_FUNC;
				top_reg_cgpio0.bit.CGPIO_5 = GPIO_ID_EMUM_FUNC;
				top_reg9.bit.UART4 = UART4_ENUM_2ND_PINMUX;
			} else {
				if (config & PIN_UART_CFG_CH4_CTSRTS) {
					top_reg_pgpio0.bit.PGPIO_2 = GPIO_ID_EMUM_FUNC;
					top_reg_pgpio0.bit.PGPIO_3 = GPIO_ID_EMUM_FUNC;
					top_reg9.bit.UART4_CTSRTS = UART_CTSRTS_PINMUX;
				}

				if (config & PIN_UART_CFG_CH4_DIROE) {
					top_reg_pgpio0.bit.PGPIO_2 = GPIO_ID_EMUM_FUNC;
					top_reg9.bit.UART4_CTSRTS = UART_CTSRTS_DIROE;
				}


				top_reg_pgpio0.bit.PGPIO_0 = GPIO_ID_EMUM_FUNC;
				top_reg_pgpio0.bit.PGPIO_1 = GPIO_ID_EMUM_FUNC;
				top_reg9.bit.UART4 = UART4_ENUM_1ST_PINMUX;
			}
		}

		if (config & PIN_UART_CFG_CH5) {
			if (config & PIN_UART_CFG_CH5_2ND) {
				if (config & PIN_UART_CFG_CH5_CTSRTS) {
					if(top_reg4.bit.I2C5 == I2C_ENUM_I2C_2ND) {
						pr_err("UART5_2 CTSRTS conflict with I2C5_2\r\n");
						return E_PAR;
					}
					top_reg_dgpio0.bit.DGPIO_17 = GPIO_ID_EMUM_FUNC;
					top_reg_dgpio0.bit.DGPIO_18 = GPIO_ID_EMUM_FUNC;
					top_reg9.bit.UART5_CTSRTS = UART_CTSRTS_PINMUX;
				}

				if (config & PIN_UART_CFG_CH5_DIROE) {
					if(top_reg4.bit.I2C5 == I2C_ENUM_I2C_2ND) {
						pr_err("UART5_2 DIROE conflict with I2C5_2\r\n");
						return E_PAR;
					}
					top_reg_dgpio0.bit.DGPIO_17 = GPIO_ID_EMUM_FUNC;
					top_reg9.bit.UART5_CTSRTS = UART_CTSRTS_DIROE;
				}

				top_reg_dgpio0.bit.DGPIO_15 = GPIO_ID_EMUM_FUNC;
				top_reg_dgpio0.bit.DGPIO_16 = GPIO_ID_EMUM_FUNC;
				top_reg9.bit.UART5 = UART5_ENUM_2ND_PINMUX;
			} else {
				if (config & PIN_UART_CFG_CH5_CTSRTS) {
					top_reg_pgpio0.bit.PGPIO_6 = GPIO_ID_EMUM_FUNC;
					top_reg_pgpio0.bit.PGPIO_7 = GPIO_ID_EMUM_FUNC;
					top_reg9.bit.UART5_CTSRTS = UART_CTSRTS_PINMUX;
				}

				if (config & PIN_UART_CFG_CH5_DIROE) {
					top_reg_pgpio0.bit.PGPIO_6 = GPIO_ID_EMUM_FUNC;
					top_reg9.bit.UART5_CTSRTS = UART_CTSRTS_DIROE;
				}


				top_reg_pgpio0.bit.PGPIO_4 = GPIO_ID_EMUM_FUNC;
				top_reg_pgpio0.bit.PGPIO_5 = GPIO_ID_EMUM_FUNC;
				top_reg9.bit.UART5 = UART5_ENUM_1ST_PINMUX;
			}
		}

		if (config & PIN_UART_CFG_CH6) {
			if (config & PIN_UART_CFG_CH6_2ND) {
				if (config & PIN_UART_CFG_CH6_CTSRTS) {
					pr_err("CHIP_NA51084 not support UART6_2 flow control\r\n");
				}

				if (config & PIN_UART_CFG_CH6_DIROE) {
					top_reg_dgpio0.bit.DGPIO_21= GPIO_ID_EMUM_FUNC;

					top_reg9.bit.UART6_CTSRTS = UART_CTSRTS_DIROE;
				}

				top_reg_dgpio0.bit.DGPIO_19 = GPIO_ID_EMUM_FUNC;
				top_reg_dgpio0.bit.DGPIO_20 = GPIO_ID_EMUM_FUNC;
				top_reg9.bit.UART6 = UART6_ENUM_2ND_PINMUX;
			} else {
				if (config & PIN_UART_CFG_CH6_CTSRTS) {
					top_reg_pgpio0.bit.PGPIO_10 = GPIO_ID_EMUM_FUNC;
					top_reg_pgpio0.bit.PGPIO_11 = GPIO_ID_EMUM_FUNC;
					top_reg9.bit.UART6_CTSRTS = UART_CTSRTS_PINMUX;
				}

				if (config & PIN_UART_CFG_CH6_DIROE) {
					top_reg_pgpio0.bit.PGPIO_10 = GPIO_ID_EMUM_FUNC;
					top_reg9.bit.UART6_CTSRTS = UART_CTSRTS_DIROE;
				}

				top_reg_pgpio0.bit.PGPIO_8 = GPIO_ID_EMUM_FUNC;
				top_reg_pgpio0.bit.PGPIO_9 = GPIO_ID_EMUM_FUNC;
				top_reg9.bit.UART6 = UART6_ENUM_1ST_PINMUX;
			}
		}

	} else {
		pr_err("invalid config: 0x%x\r\n", config);
		return E_PAR;
	}

	return E_OK;
}

static int pinmux_config_spi(uint32_t config)
{
	if (config == PIN_SPI_CFG_NONE) {
	} else {
		if (config & (PIN_SPI_CFG_CH1 | PIN_SPI_CFG_CH1_2ND_PINMUX | PIN_SPI_CFG_CH1_3RD_PINMUX|PIN_SPI_CFG_CH1_2BITS | PIN_SPI_CFG_CH1_4BITS | PIN_SPI_CFG_CH1_DI_ONLY)) {
			if (config & PIN_SPI_CFG_CH1_2ND_PINMUX) {
				if (config & PIN_SPI_CFG_CH1_4BITS) {
					pr_err("SPI not support 4Bit mode\r\n");
					return E_OBJ;
				}

				if (top_reg5.bit.I2C2 == I2C_ENUM_I2C) {
					pr_err("SPI_2 conflict with I2C2_1\r\n");
					return E_OBJ;
				}

				if (top_reg5.bit.SIFCH1 == SIFCH_ENUM_SIF) {
					pr_err("SPI_2 conflict with SIF_CH1\r\n");
					return E_OBJ;
				}

				if (config & PIN_SPI_CFG_CH1_2BITS) {
					top_reg_sgpio0.bit.SGPIO_12 = GPIO_ID_EMUM_FUNC;
					top_reg5.bit.SPI_DAT = SPI_DAT_ENUM_2BIT;
				}
				top_reg_sgpio0.bit.SGPIO_9 = GPIO_ID_EMUM_FUNC;
				top_reg_sgpio0.bit.SGPIO_10 = GPIO_ID_EMUM_FUNC;
				top_reg_sgpio0.bit.SGPIO_11 = GPIO_ID_EMUM_FUNC;

				top_reg5.bit.SPI = SPI_2ND_PINMUX;
			}
			else if (config & PIN_SPI_CFG_CH1_3RD_PINMUX) {

				if (config & PIN_SPI_CFG_CH1_2BITS) {
					top_reg_pgpio0.bit.PGPIO_11 = GPIO_ID_EMUM_FUNC;
					top_reg5.bit.SPI_DAT = SPI_DAT_ENUM_2BIT;
				}
				top_reg_pgpio0.bit.PGPIO_8 = GPIO_ID_EMUM_FUNC;
				top_reg_pgpio0.bit.PGPIO_9 = GPIO_ID_EMUM_FUNC;
				top_reg_pgpio0.bit.PGPIO_10 = GPIO_ID_EMUM_FUNC;

				top_reg5.bit.SPI = SPI_3RD_PINMUX;

			} else {
				if (config & PIN_SPI_CFG_CH1_4BITS) {
					pr_err("SPI not support 4Bit mode\r\n");
					return E_OBJ;
				}

				if (top_reg5.bit.I2C2 == I2C_ENUM_I2C_2ND) {
					pr_err("SPI_1 conflict with I2C2_2\r\n");
					return E_OBJ;
				}

				if ((top_reg9.bit.UART3 == UART3_ENUM_2ND_PINMUX) &&
					(top_reg9.bit.UART3_CTSRTS == UART_CTSRTS_PINMUX)){
					pr_err("SPI_1 conflict with UART3_2 flow control\r\n");

					return E_OBJ;
				}

				if (top_reg1.bit.SDIO2_EXIST == SDIO2_EXIST_EN) {
					pr_err("SPI_1 conflict with SDIO2\r\n");
					return E_OBJ;
				}

				if ((top_reg7.bit.PWM6 == PWM_ENUM_PWM_2ND) ||
					(top_reg7.bit.PWM7 == PWM_ENUM_PWM_2ND) ||
					(top_reg6.bit.PWM8 == PWM_ENUM_PWM_2ND) ||
					(top_reg6.bit.PWM9 == PWM_ENUM_PWM_2ND)){

					pr_err("SPI_1 conflict with PWM6_2/PWM7_2/PWM8_2/PWM9_2 \r\n");
					return E_OBJ;
				}

                if (config & PIN_SPI_CFG_CH1_DI_ONLY) {
					top_reg5.bit.SPI_DAT = SPI_DAT_ENUM_2BIT;
					top_reg5.bit.SPI = SPI_1ST_PINMUX;
					top_reg_cgpio0.bit.CGPIO_20 = GPIO_ID_EMUM_FUNC;
					top_reg_cgpio0.bit.CGPIO_17 = GPIO_ID_EMUM_GPIO;
					top_reg_cgpio0.bit.CGPIO_18 = GPIO_ID_EMUM_GPIO;
					top_reg_cgpio0.bit.CGPIO_19 = GPIO_ID_EMUM_GPIO;

                } else {
					if (config & PIN_SPI_CFG_CH1_2BITS) {
						top_reg_cgpio0.bit.CGPIO_20 = GPIO_ID_EMUM_FUNC;
						top_reg5.bit.SPI_DAT = SPI_DAT_ENUM_2BIT;
					}

					top_reg5.bit.SPI = SPI_1ST_PINMUX;
					top_reg_cgpio0.bit.CGPIO_17 = GPIO_ID_EMUM_FUNC;
					top_reg_cgpio0.bit.CGPIO_18 = GPIO_ID_EMUM_FUNC;
					top_reg_cgpio0.bit.CGPIO_19 = GPIO_ID_EMUM_FUNC;
                }
			}
		}

		if (config & (PIN_SPI_CFG_CH2 | PIN_SPI_CFG_CH2_2ND_PINMUX | PIN_SPI_CFG_CH2_2BITS)) {
			if (config & PIN_SPI_CFG_CH2_2ND_PINMUX) {

				if (nvt_get_chip_id() == CHIP_NA51084) {
					pr_err("CHIP_NA51084 not support SPI2_2\r\n");
					return E_OBJ;
				}

				if ((top_reg7.bit.PWM0 == PWM_ENUM_PWM) ||
					(top_reg7.bit.PWM1 == PWM_ENUM_PWM) ||
					(top_reg7.bit.PWM2 == PWM_ENUM_PWM)){

					pr_err("SPI2_2 conflict with PWM0_1 ~PWM2_1 \r\n");
					return E_OBJ;
				}

				if (top_reg9.bit.UART2 == UART2_ENUM_3RD_PINMUX) {
					pr_err("SPI2_2 conflict with UART2_3\r\n");
					return E_OBJ;
				}

				if (config & PIN_SPI_CFG_CH2_2BITS) {
					if (top_reg7.bit.PWM3 == PWM_ENUM_PWM){
						pr_err("SPI2_2 conflict with PWM3_1 \r\n");
						return E_OBJ;
					}
					top_reg_pgpio0.bit.PGPIO_3 = GPIO_ID_EMUM_FUNC;

					top_reg5.bit.SPI2_DAT = SPI_DAT_ENUM_2BIT;
				}

				top_reg_pgpio0.bit.PGPIO_0 = GPIO_ID_EMUM_FUNC;
				top_reg_pgpio0.bit.PGPIO_1 = GPIO_ID_EMUM_FUNC;
				top_reg_pgpio0.bit.PGPIO_2 = GPIO_ID_EMUM_FUNC;

				top_reg5.bit.SPI2 = SPI2_2ND_PINMUX;
			} else {
				if (top_reg7.bit.PI_CNT2 == PICNT_ENUM_PICNT) {
					pr_err("SPI2_1 conflict with PI_CNT2_1\r\n");
					return E_OBJ;
				}

				if (top_reg5.bit.SIFCH3 == SIFCH_ENUM_SIF) {
					pr_err("SPI2_1 conflict with SIF_CH3\r\n");
					return E_OBJ;
				}

				if (top_reg9.bit.UART3 == UART3_ENUM_1ST_PINMUX) {
					pr_err("SPI2_1 conflict with UART3_1\r\n");
					return E_OBJ;
				}

				if (config & PIN_SPI_CFG_CH2_2BITS) {
					top_reg_pgpio0.bit.PGPIO_16 = GPIO_ID_EMUM_FUNC;

					top_reg5.bit.SPI2_DAT = SPI_DAT_ENUM_2BIT;
				}

				top_reg_pgpio0.bit.PGPIO_13 = GPIO_ID_EMUM_FUNC;
				top_reg_pgpio0.bit.PGPIO_14 = GPIO_ID_EMUM_FUNC;
				top_reg_pgpio0.bit.PGPIO_15 = GPIO_ID_EMUM_FUNC;

				top_reg5.bit.SPI2 = SPI2_1ST_PINMUX;
			}
		}

		if (config & (PIN_SPI_CFG_CH3 | PIN_SPI_CFG_CH3_2ND_PINMUX | PIN_SPI_CFG_CH3_3RD_PINMUX |PIN_SPI_CFG_CH3_2BITS | PIN_SPI_CFG_CH3_RDY)) {
			if (config & PIN_SPI_CFG_CH3_2ND_PINMUX) {
				if (top_reg1.bit.SDIO_EXIST == SDIO_EXIST_EN) {
					pr_err("SPI3_2 conflict with to SDIO\r\n");
					return E_OBJ;
				}

				if (top_reg5.bit.I2C3 == I2C_ENUM_I2C_2ND) {
					pr_err("SPI3_2 conflict with I2C3_2\r\n");
					return E_OBJ;
				}

				if (top_reg5.bit.SDP == SDP_1ST_PINMUX) {
					pr_err("SPI3_2 conflict with SDP_1 \r\n");
					return E_OBJ;
				}

				if (top_reg6.bit.DIGITAL_MIC == DMIC_ENUM_DMIC_3RD_PINMUX) {
					pr_err("SPI3_2 conflict with DMCLK_3 \r\n");
					return E_OBJ;
				}

				if (config & PIN_SPI_CFG_CH3_RDY) {
					top_reg_cgpio0.bit.CGPIO_15 = GPIO_ID_EMUM_FUNC;

					top_reg5.bit.SPI3_RDY = SPI3_RDY_2ND_PINMUX;
				}

				if (config & PIN_SPI_CFG_CH3_2BITS) {
					if (top_reg6.bit.REMOTE == REMOTE_ENUM_REMOTE_2ND) {
						pr_err("SPI3_2 conflict with remote_in_2 \r\n");
						return E_OBJ;
					}
					top_reg_cgpio0.bit.CGPIO_14 = GPIO_ID_EMUM_FUNC;

					top_reg5.bit.SPI3_DAT = SPI_DAT_ENUM_2BIT;
				}

				top_reg_cgpio0.bit.CGPIO_12 = GPIO_ID_EMUM_FUNC;
				top_reg_cgpio0.bit.CGPIO_11 = GPIO_ID_EMUM_FUNC;
				top_reg_cgpio0.bit.CGPIO_13 = GPIO_ID_EMUM_FUNC;

				top_reg5.bit.SPI3 = SPI3_2ND_PINMUX;
			} else if (config & PIN_SPI_CFG_CH3_3RD_PINMUX) {

				if (nvt_get_chip_id() == CHIP_NA51084) {
					pr_err("CHIP_NA51084 not support SPI3_3\r\n");
					return E_OBJ;
				}

				if (top_reg9.bit.UART3 == UART3_ENUM_3RD_PINMUX) {
					pr_err("SPI3_3 conflict with UART3_3\r\n");
					return E_OBJ;
				}

				if(top_reg5.bit.SDP == SDP_2ND_PINMUX){
					pr_err("SPI3_3 conflict with SDP_2 \r\n");
					return E_OBJ;
				}

				if (config & PIN_SPI_CFG_CH3_RDY) {
					//52x compatible - donot support this function
					pr_err("SPI3_3 not support RDY\r\n");
					return E_OBJ;
					//top_reg_pgpio0.bit.PGPIO_8 = GPIO_ID_EMUM_FUNC;
					//top_reg5.bit.SPI3_RDY = SPI3_RDY_3RD_PINMUX;
				}

				if (config & PIN_SPI_CFG_CH3_2BITS) {
					top_reg_pgpio0.bit.PGPIO_7 = GPIO_ID_EMUM_FUNC;
					top_reg5.bit.SPI3_DAT = SPI_DAT_ENUM_2BIT;
				}

				top_reg_pgpio0.bit.PGPIO_4 = GPIO_ID_EMUM_FUNC;
				top_reg_pgpio0.bit.PGPIO_5 = GPIO_ID_EMUM_FUNC;
				top_reg_pgpio0.bit.PGPIO_6 = GPIO_ID_EMUM_FUNC;

				top_reg5.bit.SPI3 = SPI3_3RD_PINMUX;
			} else {
				if (top_reg9.bit.UART2 == UART2_ENUM_1ST_PINMUX) {
					pr_err("SPI3_1 conflict with UART2 1st\r\n");
					return E_OBJ;
				} else if (top_reg6.bit.AUDIO == AUDIO_ENUM_I2S) {
					pr_err("SPI3_1 conflict with I2S_1\r\n");
					return E_OBJ;
				}

				if (config & PIN_SPI_CFG_CH3_RDY) {
					top_reg_pgpio0.bit.PGPIO_12 = GPIO_ID_EMUM_FUNC;
					top_reg5.bit.SPI3_RDY = SPI3_RDY_1ST_PINMUX;
				}

				if (config & PIN_SPI_CFG_CH3_2BITS) {
					if (top_reg7.bit.PI_CNT2 == PICNT_ENUM_PICNT) {
						pr_err("SPI3_1 2bit mode conflict with PICNT2_1\r\n");
						return E_OBJ;
					}

					top_reg_pgpio0.bit.PGPIO_20 = GPIO_ID_EMUM_FUNC;
					top_reg5.bit.SPI3_DAT = SPI_DAT_ENUM_2BIT;
				}

				top_reg_pgpio0.bit.PGPIO_17 = GPIO_ID_EMUM_FUNC;
				top_reg_pgpio0.bit.PGPIO_18 = GPIO_ID_EMUM_FUNC;
				top_reg_pgpio0.bit.PGPIO_19 = GPIO_ID_EMUM_FUNC;

				top_reg5.bit.SPI3 = SPI3_1ST_PINMUX;
			}
		}

        if (nvt_get_chip_id() == CHIP_NA51084) {
	        if (config & (PIN_SPI_CFG_CH4 | PIN_SPI_CFG_CH4_2ND_PINMUX | PIN_SPI_CFG_CH4_2BITS)) {
				if (config & PIN_SPI_CFG_CH4_2ND_PINMUX) {
					if (config & PIN_SPI_CFG_CH4_2BITS) {
					    top_reg_cgpio0.bit.CGPIO_7 = GPIO_ID_EMUM_FUNC;
						top_reg4.bit.SPI4_DAT = SPI_DAT_ENUM_2BIT;
					}
					top_reg_cgpio0.bit.CGPIO_4 = GPIO_ID_EMUM_FUNC;
					top_reg_cgpio0.bit.CGPIO_5 = GPIO_ID_EMUM_FUNC;
					top_reg_cgpio0.bit.CGPIO_6 = GPIO_ID_EMUM_FUNC;

					top_reg4.bit.SPI4 = SPI4_2ND_PINMUX;
				} else {
					if (config & PIN_SPI_CFG_CH4_2BITS) {
						top_reg_pgpio0.bit.PGPIO_3 = GPIO_ID_EMUM_FUNC;
						top_reg4.bit.SPI4_DAT = SPI_DAT_ENUM_2BIT;
					}

					top_reg_pgpio0.bit.PGPIO_0 = GPIO_ID_EMUM_FUNC;
					top_reg_pgpio0.bit.PGPIO_1 = GPIO_ID_EMUM_FUNC;
					top_reg_pgpio0.bit.PGPIO_2 = GPIO_ID_EMUM_FUNC;

					top_reg4.bit.SPI4 = SPI4_1ST_PINMUX;
				}
			}

			if (config & (PIN_SPI_CFG_CH5 | PIN_SPI_CFG_CH5_2ND_PINMUX | PIN_SPI_CFG_CH5_2BITS)) {
				if (config & PIN_SPI_CFG_CH5_2ND_PINMUX) {
					if (config & PIN_SPI_CFG_CH5_2BITS) {
						top_reg_dgpio0.bit.DGPIO_14 = GPIO_ID_EMUM_FUNC;
						top_reg4.bit.SPI5_DAT = SPI_DAT_ENUM_2BIT;
					}
					top_reg_dgpio0.bit.DGPIO_11 = GPIO_ID_EMUM_FUNC;
					top_reg_dgpio0.bit.DGPIO_12 = GPIO_ID_EMUM_FUNC;
					top_reg_dgpio0.bit.DGPIO_13 = GPIO_ID_EMUM_FUNC;

					top_reg4.bit.SPI5 = SPI5_2ND_PINMUX;
				} else {
					if (config & PIN_SPI_CFG_CH5_2BITS) {
						top_reg_pgpio0.bit.PGPIO_7 = GPIO_ID_EMUM_FUNC;
						top_reg4.bit.SPI5_DAT = SPI_DAT_ENUM_2BIT;
					}
					top_reg_pgpio0.bit.PGPIO_4 = GPIO_ID_EMUM_FUNC;
					top_reg_pgpio0.bit.PGPIO_5 = GPIO_ID_EMUM_FUNC;
					top_reg_pgpio0.bit.PGPIO_6 = GPIO_ID_EMUM_FUNC;

					top_reg4.bit.SPI5 = SPI5_1ST_PINMUX;
				}
			}
       	}else {
		if (config & (PIN_SPI_CFG_CH4 | PIN_SPI_CFG_CH4_2ND_PINMUX | PIN_SPI_CFG_CH4_2BITS | PIN_SPI_CFG_CH4_RDY)) {
			pr_err("Not support SPI4\r\n");
		}

		if (config & (PIN_SPI_CFG_CH5 | PIN_SPI_CFG_CH5_2ND_PINMUX | PIN_SPI_CFG_CH5_2BITS)) {
			pr_err("Not support SPI5\r\n");
			}
       	}
	}

	return E_OK;
}

static int pinmux_config_sdp(uint32_t config)
{
	if (config == PIN_SDP_CFG_NONE) {
	} else if (config & PIN_SDP_CFG_CH1) {
 		if (top_reg1.bit.SDIO_EXIST == SDIO_EXIST_EN) {
			pr_err("SDP_1 conflict with to SDIO\r\n");
			return E_OBJ;
		}

		if (top_reg9.bit.UART2 == UART2_ENUM_2ND_PINMUX) {
			pr_err("SDP_1 conflict with UART2_2\r\n");
			return E_OBJ;
		}

		if (top_reg5.bit.I2C3 == I2C_ENUM_I2C_2ND) {
			pr_err("SDP_1 conflict with I2C3_2\r\n");
			return E_OBJ;
		}

		if (top_reg5.bit.SPI3 == SPI3_2ND_PINMUX) {
			pr_err("SDP_1 conflict with SPI3_2 \r\n");
			return E_OBJ;
		}

		if (top_reg6.bit.REMOTE == REMOTE_ENUM_REMOTE_2ND) {
			pr_err("SDP_1 conflict with remote_in_2 \r\n");
			return E_OBJ;
		}

		if (top_reg6.bit.DIGITAL_MIC == DMIC_ENUM_DMIC_3RD_PINMUX) {
			pr_err("SDP_1 conflict with DMCLK_3 \r\n");
			return E_OBJ;
		}

		if ((top_reg7.bit.PWM0 == PWM_ENUM_PWM_2ND) ||
			(top_reg7.bit.PWM1 == PWM_ENUM_PWM_2ND) ||
			(top_reg7.bit.PWM2 == PWM_ENUM_PWM_2ND) ||
			(top_reg7.bit.PWM4 == PWM_ENUM_PWM_2ND) ||
			(top_reg7.bit.PWM5 == PWM_ENUM_PWM_2ND)){

			pr_err("SDP_1 conflict with PWM0_2/PWM1_2/PWM2_2/PWM4_2/PWM5_2 \r\n");
			return E_OBJ;
		}

		top_reg_cgpio0.bit.CGPIO_11 = GPIO_ID_EMUM_FUNC;
		top_reg_cgpio0.bit.CGPIO_12 = GPIO_ID_EMUM_FUNC;
		top_reg_cgpio0.bit.CGPIO_13 = GPIO_ID_EMUM_FUNC;
		top_reg_cgpio0.bit.CGPIO_14 = GPIO_ID_EMUM_FUNC;
		top_reg_cgpio0.bit.CGPIO_15 = GPIO_ID_EMUM_FUNC;

		top_reg5.bit.SDP = SDP_1ST_PINMUX;
	} else if (config & PIN_SDP_CFG_CH2) {
		if (top_reg5.bit.SPI3 == SPI3_3RD_PINMUX) {
			pr_err("SDP_2 conflict with SPI3_3 \r\n");
			return E_OBJ;
		}

		if ((top_reg7.bit.PWM4 == PWM_ENUM_PWM) ||
			(top_reg7.bit.PWM5 == PWM_ENUM_PWM) ||
			(top_reg7.bit.PWM6 == PWM_ENUM_PWM) ||
			(top_reg7.bit.PWM7 == PWM_ENUM_PWM) ||
			(top_reg6.bit.PWM8 == PWM_ENUM_PWM)){

			pr_err("SDP_2 conflict with PWM4_1 ~PWM8_1 \r\n");
			return E_OBJ;
		}

		if (top_reg9.bit.UART3 == UART3_ENUM_3RD_PINMUX) {
			pr_err("SDP_2 conflict with UART3_3\r\n");
			return E_OBJ;
		}

		if (top_reg6.bit.DIGITAL_MIC == DMIC_ENUM_DMIC) {
			pr_err("SDP_2 conflict with DMCLK_1 \r\n");
			return E_OBJ;
		}

		top_reg_pgpio0.bit.PGPIO_4 = GPIO_ID_EMUM_FUNC;
		top_reg_pgpio0.bit.PGPIO_5 = GPIO_ID_EMUM_FUNC;
		top_reg_pgpio0.bit.PGPIO_6 = GPIO_ID_EMUM_FUNC;
		top_reg_pgpio0.bit.PGPIO_7 = GPIO_ID_EMUM_FUNC;
		top_reg_pgpio0.bit.PGPIO_8 = GPIO_ID_EMUM_FUNC;

		top_reg5.bit.SDP = SDP_2ND_PINMUX;
	} else {
		pr_err("invalid config: 0x%x\r\n", config);
		return E_PAR;
	}

	return E_OK;
}

static int pinmux_config_remote(uint32_t config)
{
	if (config == PIN_REMOTE_CFG_NONE) {
	} else if (config & PIN_REMOTE_CFG_CH1) {
		if (top_reg5.bit.SPI3 == SPI3_1ST_PINMUX) {
			pr_err("Remote_1 conflict with SPI3_1 \r\n");
			return E_OBJ;
		}

		if(top_reg9.bit.UART2 == UART2_ENUM_1ST_PINMUX) {
			pr_err("Remote_1 conflict with UART2_1 \r\n");
			return E_OBJ;
		}

		if (top_reg7.bit.PI_CNT3 == PICNT_ENUM_PICNT) {
			pr_err("Remote_1 conflict with PICNT3_1\r\n");
			return E_OBJ;
		}

		if (top_reg3.bit.SP_CLK ==SP_CLK_SEL_ENUM_SPCLK) {
			pr_err("Remote_1 conflict with SP_CLK_1\n");
			return E_OBJ;
		}

		if (top_reg6.bit.AUDIO ==AUDIO_ENUM_I2S) {
			pr_err("Remote_1 conflict with I2S_1\n");
			return E_OBJ;
		}
		top_reg_pgpio0.bit.PGPIO_17 = GPIO_ID_EMUM_FUNC;
		top_reg6.bit.REMOTE = REMOTE_ENUM_REMOTE_1ST;
	} else if (config & PIN_REMOTE_CFG_CH2) {
 		if (top_reg1.bit.SDIO_EXIST == SDIO_EXIST_EN) {
			pr_err("Remote_2 conflict with to SDIO\r\n");
			return E_OBJ;
		}

		if (top_reg9.bit.UART2 == UART2_ENUM_2ND_PINMUX) {
			pr_err("Remote_2 conflict with UART2_2\r\n");
			return E_OBJ;
		}

		if (top_reg5.bit.SPI3 == SPI3_2ND_PINMUX) {
			pr_err("Remote_2 conflict with SPI3_2 \r\n");
			return E_OBJ;
		}

		if (top_reg7.bit.PWM1 == PWM_ENUM_PWM_2ND){
			pr_err("Remote_2 conflict with PWM1_2 \r\n");
			return E_OBJ;
		}

		if (top_reg5.bit.SDP == SDP_1ST_PINMUX) {
			pr_err("Remote_2 conflict with SDP_1 \r\n");
			return E_OBJ;
		}

		top_reg_cgpio0.bit.CGPIO_14 = GPIO_ID_EMUM_FUNC;
		top_reg6.bit.REMOTE = REMOTE_ENUM_REMOTE_2ND;
	}	else if (config & PIN_REMOTE_CFG_CH3) {
		top_reg_dgpio0.bit.DGPIO_2 = GPIO_ID_EMUM_FUNC;
		top_reg6.bit.REMOTE = REMOTE_ENUM_REMOTE_3RD;
	}

	else {
		pr_err("invalid config: 0x%x\r\n", config);
		return E_PAR;
	}

	return E_OK;
}

static int pinmux_config_pwm(uint32_t config)
{
	/*[PWM0]*/
	if (config & PIN_PWM_CFG_PWM0_1) {
		top_reg_pgpio0.bit.PGPIO_0 = GPIO_ID_EMUM_FUNC;
		top_reg7.bit.PWM0 = PWM_ENUM_PWM;
	} else if (config & PIN_PWM_CFG_PWM0_2) {
		top_reg_cgpio0.bit.CGPIO_13 = GPIO_ID_EMUM_FUNC;
		top_reg7.bit.PWM0 = PWM_ENUM_PWM_2ND;
	} else if (config & PIN_PWM_CFG_PWM0_3) {
		top_reg_sgpio0.bit.SGPIO_1 = GPIO_ID_EMUM_FUNC;
		top_reg7.bit.PWM0 = PWM_ENUM_PWM_3RD;
	} else if (config & PIN_PWM_CFG_PWM0_4) {
		top_reg_cgpio0.bit.CGPIO_4 = GPIO_ID_EMUM_FUNC;
		top_reg7.bit.PWM0 = PWM_ENUM_PWM_4TH;
	} else if (config & PIN_PWM_CFG_PWM0_5) {
		top_reg_lgpio0.bit.LGPIO_0 = GPIO_ID_EMUM_FUNC;
		top_reg7.bit.PWM0 = PWM_ENUM_PWM_5TH;
	}

	/*[PWM1]*/
	if (config & PIN_PWM_CFG_PWM1_1) {
		top_reg_pgpio0.bit.PGPIO_1 = GPIO_ID_EMUM_FUNC;
		top_reg7.bit.PWM1 = PWM_ENUM_PWM;
	} else if (config & PIN_PWM_CFG_PWM1_2) {
		top_reg_cgpio0.bit.CGPIO_14 = GPIO_ID_EMUM_FUNC;
		top_reg7.bit.PWM1 = PWM_ENUM_PWM_2ND;
	} else if (config & PIN_PWM_CFG_PWM1_3) {
		top_reg_sgpio0.bit.SGPIO_2 = GPIO_ID_EMUM_FUNC;
		top_reg7.bit.PWM1 = PWM_ENUM_PWM_3RD;
	} else if (config & PIN_PWM_CFG_PWM1_4) {
		top_reg_cgpio0.bit.CGPIO_5 = GPIO_ID_EMUM_FUNC;
		top_reg7.bit.PWM1 = PWM_ENUM_PWM_4TH;
	} else if (config & PIN_PWM_CFG_PWM1_5) {
		top_reg_lgpio0.bit.LGPIO_1 = GPIO_ID_EMUM_FUNC;
		top_reg7.bit.PWM1 = PWM_ENUM_PWM_5TH;
	}

	/*[PWM2]*/
	if (config & PIN_PWM_CFG_PWM2_1) {
		top_reg_pgpio0.bit.PGPIO_2 = GPIO_ID_EMUM_FUNC;
		top_reg7.bit.PWM2 = PWM_ENUM_PWM;
	} else if (config & PIN_PWM_CFG_PWM2_2) {
		top_reg_cgpio0.bit.CGPIO_15 = GPIO_ID_EMUM_FUNC;
		top_reg7.bit.PWM2 = PWM_ENUM_PWM_2ND;
	} else if (config & PIN_PWM_CFG_PWM2_3) {
		top_reg_sgpio0.bit.SGPIO_3 = GPIO_ID_EMUM_FUNC;
		top_reg7.bit.PWM2 = PWM_ENUM_PWM_3RD;
	} else if (config & PIN_PWM_CFG_PWM2_4) {
		top_reg_cgpio0.bit.CGPIO_6 = GPIO_ID_EMUM_FUNC;
		top_reg7.bit.PWM2 = PWM_ENUM_PWM_4TH;
	} else if (config & PIN_PWM_CFG_PWM2_5) {
		top_reg_lgpio0.bit.LGPIO_2 = GPIO_ID_EMUM_FUNC;
		top_reg7.bit.PWM2 = PWM_ENUM_PWM_5TH;
	}

	/*[PWM3]*/
	if (config & PIN_PWM_CFG_PWM3_1) {
		top_reg_pgpio0.bit.PGPIO_3 = GPIO_ID_EMUM_FUNC;
		top_reg7.bit.PWM3 = PWM_ENUM_PWM;
	} else if (config & PIN_PWM_CFG_PWM3_2) {
		top_reg_cgpio0.bit.CGPIO_16 = GPIO_ID_EMUM_FUNC;
		top_reg7.bit.PWM3 = PWM_ENUM_PWM_2ND;
	} else if (config & PIN_PWM_CFG_PWM3_3) {
		top_reg_sgpio0.bit.SGPIO_4= GPIO_ID_EMUM_FUNC;
		top_reg7.bit.PWM3 = PWM_ENUM_PWM_3RD;
	} else if (config & PIN_PWM_CFG_PWM3_4) {
		top_reg_cgpio0.bit.CGPIO_7 = GPIO_ID_EMUM_FUNC;
		top_reg7.bit.PWM3 = PWM_ENUM_PWM_4TH;
	} else if (config & PIN_PWM_CFG_PWM3_5) {
		top_reg_lgpio0.bit.LGPIO_3 = GPIO_ID_EMUM_FUNC;
		top_reg7.bit.PWM3 = PWM_ENUM_PWM_5TH;
	}

	/*[PWM8]*/
	if (config & PIN_PWM_CFG_PWM8_1) {
		top_reg_pgpio0.bit.PGPIO_8 = GPIO_ID_EMUM_FUNC;
		top_reg6.bit.PWM8 = PWM_ENUM_PWM;
	} else if (config & PIN_PWM_CFG_PWM8_2) {
		top_reg_cgpio0.bit.CGPIO_19 = GPIO_ID_EMUM_FUNC;
		top_reg6.bit.PWM8 = PWM_ENUM_PWM_2ND;
	} else if (config & PIN_PWM_CFG_PWM8_3) {
		//top_reg_hgpio0.bit.HSIGPIO_6 = GPIO_ID_EMUM_FUNC;
		//top_reg6.bit.PWM8 = PWM_ENUM_PWM_3RD;
		//52x compatible - donot support this function
		pr_err("Not support PWM8_3\r\n");
		return E_PAR;
	} else if (config & PIN_PWM_CFG_PWM8_4) {
		top_reg_dgpio0.bit.DGPIO_8 = GPIO_ID_EMUM_FUNC;
		top_reg6.bit.PWM8 = PWM_ENUM_PWM_4TH;
	} else if (config & PIN_PWM_CFG_PWM8_5) {
		top_reg_lgpio0.bit.LGPIO_8 = GPIO_ID_EMUM_FUNC;
		top_reg6.bit.PWM8 = PWM_ENUM_PWM_5TH;
	}

	/*[PWM9]*/
	if (config & PIN_PWM_CFG_PWM9_1) {
		top_reg_pgpio0.bit.PGPIO_9 = GPIO_ID_EMUM_FUNC;
		top_reg6.bit.PWM9 = PWM_ENUM_PWM;
	} else if (config & PIN_PWM_CFG_PWM9_2) {
		top_reg_cgpio0.bit.CGPIO_20 = GPIO_ID_EMUM_FUNC;
		top_reg6.bit.PWM9 = PWM_ENUM_PWM_2ND;
	} else if (config & PIN_PWM_CFG_PWM9_3) {
		//top_reg_hgpio0.bit.HSIGPIO_7 = GPIO_ID_EMUM_FUNC;
		//top_reg6.bit.PWM9 = PWM_ENUM_PWM_3RD;
		//52x compatible - donot support this function
		pr_err("Not support PWM9_3\r\n");
		return E_PAR;
	} else if (config & PIN_PWM_CFG_PWM9_4) {
		top_reg_dgpio0.bit.DGPIO_9 = GPIO_ID_EMUM_FUNC;
		top_reg6.bit.PWM9 = PWM_ENUM_PWM_4TH;
	} else if (config & PIN_PWM_CFG_PWM9_5) {
		top_reg_lgpio0.bit.LGPIO_9 = GPIO_ID_EMUM_FUNC;
		top_reg6.bit.PWM9 = PWM_ENUM_PWM_5TH;
	}

	return E_OK;
}

static int pinmux_config_pwm2(uint32_t config)
{
	/*[PWM4]*/
	if (config & PIN_PWM_CFG2_PWM4_1) {
		top_reg_pgpio0.bit.PGPIO_4 = GPIO_ID_EMUM_FUNC;
		top_reg7.bit.PWM4 = PWM_ENUM_PWM;
	} else if (config & PIN_PWM_CFG2_PWM4_2) {
		top_reg_cgpio0.bit.CGPIO_11 = GPIO_ID_EMUM_FUNC;
		top_reg7.bit.PWM4 = PWM_ENUM_PWM_2ND;
	} else if (config & PIN_PWM_CFG2_PWM4_3) {
		top_reg_sgpio0.bit.SGPIO_5= GPIO_ID_EMUM_FUNC;
		top_reg7.bit.PWM4 = PWM_ENUM_PWM_3RD;
	} else if (config & PIN_PWM_CFG2_PWM4_4) {
		top_reg_dgpio0.bit.DGPIO_3 = GPIO_ID_EMUM_FUNC;
		top_reg7.bit.PWM4 = PWM_ENUM_PWM_4TH;
	} else if (config & PIN_PWM_CFG2_PWM4_5) {
		top_reg_lgpio0.bit.LGPIO_4 = GPIO_ID_EMUM_FUNC;
		top_reg7.bit.PWM4 = PWM_ENUM_PWM_5TH;
	}

	/*[PWM5]*/
	if (config & PIN_PWM_CFG2_PWM5_1) {
		top_reg_pgpio0.bit.PGPIO_5 = GPIO_ID_EMUM_FUNC;
		top_reg7.bit.PWM5 = PWM_ENUM_PWM;
	} else if (config & PIN_PWM_CFG2_PWM5_2) {
		top_reg_cgpio0.bit.CGPIO_12 = GPIO_ID_EMUM_FUNC;
		top_reg7.bit.PWM5 = PWM_ENUM_PWM_2ND;
	} else if (config & PIN_PWM_CFG2_PWM5_3) {
		top_reg_sgpio0.bit.SGPIO_6 = GPIO_ID_EMUM_FUNC;
		top_reg7.bit.PWM5 = PWM_ENUM_PWM_3RD;
	} else if (config & PIN_PWM_CFG2_PWM5_4) {
		top_reg_dgpio0.bit.DGPIO_4 = GPIO_ID_EMUM_FUNC;
		top_reg7.bit.PWM5 = PWM_ENUM_PWM_4TH;
	} else if (config & PIN_PWM_CFG2_PWM5_5) {
		top_reg_lgpio0.bit.LGPIO_5 = GPIO_ID_EMUM_FUNC;
		top_reg7.bit.PWM5 = PWM_ENUM_PWM_5TH;
	}

	/*[PWM6]*/
	if (config & PIN_PWM_CFG2_PWM6_1) {
		top_reg_pgpio0.bit.PGPIO_6 = GPIO_ID_EMUM_FUNC;
		top_reg7.bit.PWM6 = PWM_ENUM_PWM;
	} else if (config & PIN_PWM_CFG2_PWM6_2) {
		top_reg_cgpio0.bit.CGPIO_17 = GPIO_ID_EMUM_FUNC;
		top_reg7.bit.PWM6 = PWM_ENUM_PWM_2ND;
	} else if (config & PIN_PWM_CFG2_PWM6_3) {
		top_reg_sgpio0.bit.SGPIO_7 = GPIO_ID_EMUM_FUNC;
		top_reg7.bit.PWM6 = PWM_ENUM_PWM_3RD;
	} else if (config & PIN_PWM_CFG2_PWM6_4) {
		top_reg_dgpio0.bit.DGPIO_5 = GPIO_ID_EMUM_FUNC;
		top_reg7.bit.PWM6 = PWM_ENUM_PWM_4TH;
	} else if (config & PIN_PWM_CFG2_PWM6_5) {
		top_reg_lgpio0.bit.LGPIO_6 = GPIO_ID_EMUM_FUNC;
		top_reg7.bit.PWM6 = PWM_ENUM_PWM_5TH;
	}

	/*[PWM7]*/
	if (config & PIN_PWM_CFG2_PWM7_1) {
		top_reg_pgpio0.bit.PGPIO_7 = GPIO_ID_EMUM_FUNC;
		top_reg7.bit.PWM7 = PWM_ENUM_PWM;
	} else if (config & PIN_PWM_CFG2_PWM7_2) {
		top_reg_cgpio0.bit.CGPIO_18 = GPIO_ID_EMUM_FUNC;
		top_reg7.bit.PWM7 = PWM_ENUM_PWM_2ND;
	} else if (config & PIN_PWM_CFG2_PWM7_3) {
		top_reg_sgpio0.bit.SGPIO_8 = GPIO_ID_EMUM_FUNC;
		top_reg7.bit.PWM7 = PWM_ENUM_PWM_3RD;
	} else if (config & PIN_PWM_CFG2_PWM7_4) {
		top_reg_dgpio0.bit.DGPIO_6 = GPIO_ID_EMUM_FUNC;
		top_reg7.bit.PWM7 = PWM_ENUM_PWM_4TH;
	} else if (config & PIN_PWM_CFG2_PWM7_5) {
		top_reg_lgpio0.bit.LGPIO_7 = GPIO_ID_EMUM_FUNC;
		top_reg7.bit.PWM7 = PWM_ENUM_PWM_5TH;
	}

	/*[PWM10]*/
	if (config & PIN_PWM_CFG2_PWM10_1) {
		top_reg_pgpio0.bit.PGPIO_10 = GPIO_ID_EMUM_FUNC;
		top_reg6.bit.PWM10 = PWM_ENUM_PWM;
	} else if (config & PIN_PWM_CFG2_PWM10_2) {
		top_reg_cgpio0.bit.CGPIO_21 = GPIO_ID_EMUM_FUNC;
		top_reg6.bit.PWM10 = PWM_ENUM_PWM_2ND;
	} else if (config & PIN_PWM_CFG2_PWM10_3) {
		//top_reg_hgpio0.bit.HSIGPIO_8 = GPIO_ID_EMUM_FUNC;
		//top_reg6.bit.PWM10 = PWM_ENUM_PWM_3RD;
		//52x compatible - donot support this function
		pr_err("Not support PWM10_3\r\n");
		return E_PAR;
	} else if (config & PIN_PWM_CFG2_PWM10_4) {
		top_reg_dgpio0.bit.DGPIO_10 = GPIO_ID_EMUM_FUNC;
		top_reg6.bit.PWM10 = PWM_ENUM_PWM_4TH;
	} else if (config & PIN_PWM_CFG2_PWM10_5) {
		top_reg_lgpio0.bit.LGPIO_10 = GPIO_ID_EMUM_FUNC;
		top_reg6.bit.PWM10 = PWM_ENUM_PWM_5TH;
	}

	/*[PWM11]*/
	if (config & PIN_PWM_CFG2_PWM11_1) {
		top_reg_pgpio0.bit.PGPIO_11 = GPIO_ID_EMUM_FUNC;
		top_reg6.bit.PWM11 = PWM_ENUM_PWM;
	} else if (config & PIN_PWM_CFG2_PWM11_2) {
		top_reg_cgpio0.bit.CGPIO_22 = GPIO_ID_EMUM_FUNC;
		top_reg6.bit.PWM11 = PWM_ENUM_PWM_2ND;
	} else if (config & PIN_PWM_CFG2_PWM11_3) {
		//top_reg_hgpio0.bit.HSIGPIO_9 = GPIO_ID_EMUM_FUNC;
		//top_reg6.bit.PWM11 = PWM_ENUM_PWM_3RD;
		//52x compatible - donot support this function
		pr_err("Not support PWM11_3\r\n");
		return E_PAR;
	} else if (config & PIN_PWM_CFG2_PWM11_4) {
		top_reg_dgpio0.bit.DGPIO_7 = GPIO_ID_EMUM_FUNC;
		top_reg6.bit.PWM11 = PWM_ENUM_PWM_4TH;
	} else if (config & PIN_PWM_CFG2_PWM11_5) {
		top_reg_lgpio0.bit.LGPIO_11 = GPIO_ID_EMUM_FUNC;
		top_reg6.bit.PWM11 = PWM_ENUM_PWM_5TH;
	}

	return E_OK;
}

static int pinmux_config_ccnt(uint32_t config)
{

	if (config & (PIN_PWM_CFG_CCNT | PIN_PWM_CFG_CCNT_2ND)) {
		if (config & PIN_PWM_CFG_CCNT_2ND) {
			if (top_reg6.bit.SP2_CLK == SP2_CLK_SEL_ENUM_SP2CLK) {
				pr_err("PICNT conflict with to SP_CLK2_1: 0x%x\r\n", config);
				return E_OBJ;
			}

			top_reg_pgpio0.bit.PGPIO_18 = GPIO_ID_EMUM_FUNC;
			top_reg7.bit.PI_CNT = PICNT_ENUM_PICNT2;
		} else {

			top_reg_pgpio0.bit.PGPIO_12 = GPIO_ID_EMUM_FUNC;

			top_reg7.bit.PI_CNT = PICNT_ENUM_PICNT;
		}
	}
	if (config & (PIN_PWM_CFG_CCNT2 | PIN_PWM_CFG_CCNT2_2ND)) {
		if (config & PIN_PWM_CFG_CCNT2_2ND) {
			if (top_reg6.bit.ETH != ETH_ID_ENUM_GPIO) {
				pr_err("PICNT2_2 conflict with ETH: 0x%x\r\n",top_reg6.bit.ETH);
				return E_OBJ;
			}

			top_reg_pgpio0.bit.PGPIO_19 = GPIO_ID_EMUM_FUNC;
			top_reg7.bit.PI_CNT2 = PICNT_ENUM_PICNT2;
		} else {
			if ((top_reg9.bit.UART2 == UART2_ENUM_1ST_PINMUX) &&
				(top_reg9.bit.UART2_CTSRTS == UART_CTSRTS_PINMUX)) {
				pr_err("PICNT2 conflict with UART2_1\r\n");
				return E_OBJ;
			}
			if (top_reg6.bit.AUDIO == AUDIO_ENUM_I2S) {
				pr_err("PICNT2 conflict with AUDIO\r\n");
				return E_OBJ;
			}
			top_reg_pgpio0.bit.PGPIO_14 = GPIO_ID_EMUM_FUNC;

			top_reg7.bit.PI_CNT2 = PICNT_ENUM_PICNT;
		}
	}
	if (config & (PIN_PWM_CFG_CCNT3 | PIN_PWM_CFG_CCNT3_2ND)) {
		if (config & PIN_PWM_CFG_CCNT3_2ND) {
			if (top_reg6.bit.ETH != ETH_ID_ENUM_GPIO) {
				pr_err("PICNT3_2 conflict with ETH: 0x%x \r\n",top_reg6.bit.ETH);
				return E_OBJ;
			}

			top_reg_pgpio0.bit.PGPIO_20 = GPIO_ID_EMUM_FUNC;
			top_reg7.bit.PI_CNT3 = PICNT_ENUM_PICNT2;
		} else {
			if (top_reg6.bit.AUDIO_MCLK == AUDIO_MCLK_MCLK) {
				pr_err("PICNT3 conflict with I2S MCLK\r\n");
				return E_OBJ;
			}
			if (top_reg3.bit.SP_CLK == SP_CLK_SEL_ENUM_SP_2_CLK) {
				pr_err("PICNT3 conflict with SP_CLK_2\r\n");
				return E_OBJ;
			}

			top_reg_pgpio0.bit.PGPIO_17 = GPIO_ID_EMUM_FUNC;

			top_reg7.bit.PI_CNT3 = PICNT_ENUM_PICNT;
		}
	}

	return E_OK;
}

static int pinmux_config_audio(uint32_t config)
{
	if (config & (PIN_AUDIO_CFG_I2S)) {
		if (top_reg9.bit.UART2 == UART2_ENUM_1ST_PINMUX) {
			pr_err("AUDIO I2S_1 conflict with UART2_1\r\n");
			return E_OBJ;
		}
		if (top_reg7.bit.PI_CNT2 == PICNT_ENUM_PICNT) {
			pr_err("AUDIO I2S_1 conflict with PICNT2_1\r\n");
			return E_OBJ;
		}
		if (top_reg5.bit.SPI3 == SPI3_1ST_PINMUX) {
			pr_err("AUDIO I2S_1 conflict with SPI3_1\r\n");
			return E_OBJ;
		}

		top_reg_pgpio0.bit.PGPIO_17 = GPIO_ID_EMUM_FUNC;
		top_reg_pgpio0.bit.PGPIO_18 = GPIO_ID_EMUM_FUNC;
		top_reg_pgpio0.bit.PGPIO_19 = GPIO_ID_EMUM_FUNC;
		top_reg_pgpio0.bit.PGPIO_20 = GPIO_ID_EMUM_FUNC;

		top_reg6.bit.AUDIO = AUDIO_ENUM_I2S;

	}else if (config & (PIN_AUDIO_CFG_I2S_2ND_PINMUX)) {
		if(top_reg1.bit.SDIO3_BUS_WIDTH == SDIO3_BUS_WIDTH_8BITS) {
			pr_err("AUDIO I2S_2 conflict with SDIO3 8bits\r\n");
			return E_OBJ;
		}

		if ((top_reg7.bit.PWM0 == PWM_ENUM_PWM_4TH) ||
			(top_reg7.bit.PWM1 == PWM_ENUM_PWM_4TH) ||
			(top_reg7.bit.PWM2 == PWM_ENUM_PWM_4TH) ||
			(top_reg7.bit.PWM3 == PWM_ENUM_PWM_4TH)){

				pr_err("AUDIO I2S_2 conflict with PWM0_4 ~PWM3_4 \r\n");
				return E_OBJ;
		}

		top_reg_cgpio0.bit.CGPIO_4 = GPIO_ID_EMUM_FUNC;
		top_reg_cgpio0.bit.CGPIO_5 = GPIO_ID_EMUM_FUNC;
		top_reg_cgpio0.bit.CGPIO_6 = GPIO_ID_EMUM_FUNC;
		top_reg_cgpio0.bit.CGPIO_7 = GPIO_ID_EMUM_FUNC;

		top_reg6.bit.AUDIO = AUDIO_ENUM_I2S_2ND_PINMUX;
	}

	if (config & (PIN_AUDIO_CFG_MCLK)) {
		if(top_reg5.bit.SIFCH2 == SIFCH_ENUM_SIF_2ND){
			pr_err("I2S_MCLK_1 conflict with SB3_2\r\n");
			return E_OBJ;
		}

		top_reg_pgpio0.bit.PGPIO_16 = GPIO_ID_EMUM_FUNC;

		top_reg6.bit.AUDIO_MCLK = AUDIO_MCLK_MCLK;

	} else if (config & (PIN_AUDIO_CFG_MCLK_2ND_PINMUX)) {
		if (top_reg6.bit.PWM11 == PWM_ENUM_PWM_4TH){
			pr_err("AUDIO I2S_2 conflict with PWM11_4\r\n");
			return E_OBJ;
		}
		top_reg_dgpio0.bit.DGPIO_7 = GPIO_ID_EMUM_FUNC;
		top_reg6.bit.AUDIO_MCLK = AUDIO_MCLK_MCLK_2ND_PINMUX;
	}


	if (config & (PIN_AUDIO_CFG_DMIC)) {
		if (top_reg6.bit.SP2_CLK == SP2_CLK_SEL_ENUM_SP2CLK) {
			pr_err("AUDIO DMIC conflict with SP_CLK2_1\r\n");
			return E_OBJ;
		}

		if (top_reg7.bit.PI_CNT == PICNT_ENUM_PICNT) {
			pr_err("AUDIO DMIC conflict with PICNT_1\r\n");
			return E_OBJ;
		}
		top_reg_pgpio0.bit.PGPIO_7 = GPIO_ID_EMUM_FUNC;
		top_reg_pgpio0.bit.PGPIO_8 = GPIO_ID_EMUM_FUNC;
		top_reg_pgpio0.bit.PGPIO_9 = GPIO_ID_EMUM_FUNC;

		top_reg6.bit.DIGITAL_MIC = DMIC_ENUM_DMIC;
		top_reg6.bit.DM_DATA0 = DMIC_DATA_ENUM_DMIC;
		top_reg6.bit.DM_DATA1 = DMIC_DATA_ENUM_DMIC;

	} else if (config & (PIN_AUDIO_CFG_DMIC_2ND)) {
		top_reg_dgpio0.bit.DGPIO_8 = GPIO_ID_EMUM_FUNC;
		top_reg_dgpio0.bit.DGPIO_9 = GPIO_ID_EMUM_FUNC;
		top_reg_dgpio0.bit.DGPIO_10 = GPIO_ID_EMUM_FUNC;

		top_reg6.bit.DIGITAL_MIC = DMIC_ENUM_DMIC_2ND_PINMUX;
		top_reg6.bit.DM_DATA0 = DMIC_DATA_ENUM_DMIC;
		top_reg6.bit.DM_DATA1 = DMIC_DATA_ENUM_DMIC;

	} else if (config & (PIN_AUDIO_CFG_DMIC_3RD)) {
		top_reg_cgpio0.bit.CGPIO_11 = GPIO_ID_EMUM_FUNC;
		top_reg_cgpio0.bit.CGPIO_12 = GPIO_ID_EMUM_FUNC;
		top_reg_cgpio0.bit.CGPIO_13 = GPIO_ID_EMUM_FUNC;

		top_reg6.bit.DIGITAL_MIC = DMIC_ENUM_DMIC_3RD_PINMUX;
		top_reg6.bit.DM_DATA0 = DMIC_DATA_ENUM_DMIC;
		top_reg6.bit.DM_DATA1 = DMIC_DATA_ENUM_DMIC;
	}

	return E_OK;
}

static int pinmux_config_lcd(uint32_t config)
{
	uint32_t tmp;

	tmp = config & PINMUX_DISPMUX_SEL_MASK;
	if (tmp == PINMUX_DISPMUX_SEL_MASK) {
		pr_err("invalid locate: 0x%x\r\n", config);
		return E_PAR;
	}

	disp_pinmux_config[PINMUX_FUNC_ID_LCD] = config;

	return E_OK;
}

static int pinmux_config_tv(uint32_t config)
{
	uint32_t tmp;

	tmp = config & PINMUX_TV_HDMI_CFG_MASK;
	if ((tmp != PINMUX_TV_HDMI_CFG_NORMAL) && (tmp != PINMUX_TV_HDMI_CFG_PINMUX_ON)) {
		pr_err("invalid config: 0x%x\r\n", config);
		return E_PAR;
	}

	disp_pinmux_config[PINMUX_FUNC_ID_TV] = config;

	return E_OK;
}

static int pinmux_config_eth(uint32_t config)
{
	if (config == PIN_ETH_CFG_NONE) {
	} else if (config & PIN_ETH_CFG_INTERANL) {
		if (config & PIN_ETH_CFG_LED1) {
			if (config & PIN_ETH_CFG_LED_1ST_ONLY) {
				top_reg_dgpio0.bit.DGPIO_0 = GPIO_ID_EMUM_FUNC;
			} else if (config & PIN_ETH_CFG_LED_2ND_ONLY) {
				top_reg_dgpio0.bit.DGPIO_1 = GPIO_ID_EMUM_FUNC;
			} else {
				top_reg_dgpio0.bit.DGPIO_0 = GPIO_ID_EMUM_FUNC;
				top_reg_dgpio0.bit.DGPIO_1 = GPIO_ID_EMUM_FUNC;
			}
			top_reg6.bit.ETH_LED = ETH_LED_ENUM_LED1;
		} else if (config & PIN_ETH_CFG_LED2) {
			if (config & PIN_ETH_CFG_LED_1ST_ONLY) {
				top_reg_dgpio0.bit.DGPIO_5 = GPIO_ID_EMUM_FUNC;
			} else if (config & PIN_ETH_CFG_LED_2ND_ONLY) {
				top_reg_dgpio0.bit.DGPIO_6 = GPIO_ID_EMUM_FUNC;
			} else {
				top_reg_dgpio0.bit.DGPIO_5 = GPIO_ID_EMUM_FUNC;
				top_reg_dgpio0.bit.DGPIO_6 = GPIO_ID_EMUM_FUNC;
			}
			top_reg6.bit.ETH_LED = ETH_LED_ENUM_LED2;
		}
	} else if (config & (PIN_ETH_CFG_RMII)) {

		if (config & PIN_ETH_CFG_RMII) {
			u32 lcd_mode;

			if (disp_pinmux_config[PINMUX_FUNC_ID_LCD] & PINMUX_DISPMUX_SEL_LCD) {
				lcd_mode = disp_pinmux_config[PINMUX_FUNC_ID_LCD] & ~(PINMUX_DISPMUX_SEL_LCD | PINMUX_LCDMODE_AUTO_PINMUX);
				if ((lcd_mode == PINMUX_LCDMODE_RGB_PARALL) || (lcd_mode == PINMUX_LCDMODE_CCIR601) ||
					((lcd_mode >= PINMUX_LCDMODE_RGB_PARALL666) && (lcd_mode <= PINMUX_LCDMODE_RGB_PARALL_DELTA)) ||
					((lcd_mode >= PINMUX_LCDMODE_MI_FMT0) && (lcd_mode <= PINMUX_LCDMODE_MI_SERIAL_SEP))) {
					pr_err("ETH conflict with LCD\r\n");
					return E_OBJ;
				}
			}

			top_reg_lgpio0.bit.LGPIO_0 = GPIO_ID_EMUM_FUNC;
			top_reg_lgpio0.bit.LGPIO_1 = GPIO_ID_EMUM_FUNC;
			top_reg_lgpio0.bit.LGPIO_2 = GPIO_ID_EMUM_FUNC;
			top_reg_lgpio0.bit.LGPIO_3 = GPIO_ID_EMUM_FUNC;
			top_reg_lgpio0.bit.LGPIO_22 = GPIO_ID_EMUM_GPIO;  // Ethernet PHY reset pin
			top_reg_lgpio0.bit.LGPIO_4 = GPIO_ID_EMUM_FUNC;
			top_reg_lgpio0.bit.LGPIO_5 = GPIO_ID_EMUM_FUNC;
			top_reg_lgpio0.bit.LGPIO_6 = GPIO_ID_EMUM_FUNC;
			top_reg_lgpio0.bit.LGPIO_7 = GPIO_ID_EMUM_FUNC;   // Clock output for PHY (25MHz)
			top_reg_lgpio0.bit.LGPIO_8 = GPIO_ID_EMUM_FUNC;

			if (nvt_get_chip_id() == CHIP_NA51055)
			{
				top_reg6.bit.ETH = ETH_ID_ENUM_RMII;
			}
			else
			{
				top_reg4.bit.ETH = ETH_ID_ENUM_RMII;
			}

		}

		if (config & PIN_ETH_CFG_EXTPHYCLK) {
			top_reg_lgpio0.bit.LGPIO_7 = GPIO_ID_EMUM_FUNC;   // Clock output for PHY (25MHz)
			top_reg6.bit.EXT_PHYCLK = ETH_EXTPHY_CLK_FUNC;
		}

		// MDC/MDIO
		top_reg_lgpio0.bit.LGPIO_9 = GPIO_ID_EMUM_FUNC;
		top_reg_lgpio0.bit.LGPIO_10 = GPIO_ID_EMUM_FUNC;
#ifdef _FPGA_EMULATION_
		if (config & PIN_ETH_CFG_LED1) {
			top_reg_dgpio0.bit.DGPIO_0 = GPIO_ID_EMUM_FUNC;
			top_reg_dgpio0.bit.DGPIO_1 = GPIO_ID_EMUM_FUNC;
			top_reg6.bit.ETH_LED = ETH_LED_ENUM_LED1;
		} else if (config & PIN_ETH_CFG_LED2) {
			top_reg_dgpio0.bit.DGPIO_5 = GPIO_ID_EMUM_FUNC;
			top_reg_dgpio0.bit.DGPIO_6 = GPIO_ID_EMUM_FUNC;
			top_reg6.bit.ETH_LED = ETH_LED_ENUM_LED2;
		}
#endif
	}
    else if(config & (PIN_ETH_CFG_RMII_2)){
		if (nvt_get_chip_id() == CHIP_NA51055) {
			pr_err("CHIP_NA51055 not support RMII_2\r\n");
			return E_OBJ;
		}

		if(top_reg5.bit.SIFCH2 == SIFCH_ENUM_SIF){
			pr_err("RMII_2 conflict with SB3_1\r\n");
			return E_OBJ;
		}

		top_reg_lgpio0.bit.LGPIO_11 = GPIO_ID_EMUM_FUNC;
		top_reg_lgpio0.bit.LGPIO_12 = GPIO_ID_EMUM_FUNC;
		top_reg_lgpio0.bit.LGPIO_13 = GPIO_ID_EMUM_FUNC;
		top_reg_lgpio0.bit.LGPIO_16 = GPIO_ID_EMUM_FUNC;
		top_reg_lgpio0.bit.LGPIO_17 = GPIO_ID_EMUM_FUNC;
		top_reg_lgpio0.bit.LGPIO_18 = GPIO_ID_EMUM_FUNC;
		top_reg_lgpio0.bit.LGPIO_19 = GPIO_ID_EMUM_FUNC;
		top_reg_lgpio0.bit.LGPIO_23 = GPIO_ID_EMUM_FUNC;
		top_reg_lgpio0.bit.LGPIO_24 = GPIO_ID_EMUM_FUNC;

		top_reg4.bit.ETH = ETH_ID_ENUM_RMII_2;

		if (config & PIN_ETH_CFG_EXTPHYCLK) {
			top_reg_dgpio0.bit.DGPIO_0 = GPIO_ID_EMUM_FUNC;   // Clock output for PHY (25MHz)
			top_reg6.bit.EXT_PHYCLK = ETH_EXTPHY_CLK_FUNC;
		}
    } else if(config & (PIN_ETH_CFG_RGMII)){
		if (nvt_get_chip_id() == CHIP_NA51055) {
			pr_err("CHIP_NA51055 not support RGMII\r\n");
			return E_OBJ;
		}

		if(top_reg5.bit.SIFCH2 == SIFCH_ENUM_SIF){
			pr_err("RGMII conflict with SB3_1\r\n");
			return E_OBJ;
		}

		top_reg_lgpio0.bit.LGPIO_11 = GPIO_ID_EMUM_FUNC;
		top_reg_lgpio0.bit.LGPIO_12 = GPIO_ID_EMUM_FUNC;
		top_reg_lgpio0.bit.LGPIO_13 = GPIO_ID_EMUM_FUNC;
		top_reg_lgpio0.bit.LGPIO_14 = GPIO_ID_EMUM_FUNC;
		top_reg_lgpio0.bit.LGPIO_15 = GPIO_ID_EMUM_FUNC;
		top_reg_lgpio0.bit.LGPIO_16 = GPIO_ID_EMUM_FUNC;
		top_reg_lgpio0.bit.LGPIO_17 = GPIO_ID_EMUM_FUNC;
		top_reg_lgpio0.bit.LGPIO_18 = GPIO_ID_EMUM_FUNC;
		top_reg_lgpio0.bit.LGPIO_19 = GPIO_ID_EMUM_FUNC;
		top_reg_lgpio0.bit.LGPIO_20 = GPIO_ID_EMUM_FUNC;
		top_reg_lgpio0.bit.LGPIO_21 = GPIO_ID_EMUM_FUNC;
		top_reg_lgpio0.bit.LGPIO_22 = GPIO_ID_EMUM_FUNC;
		top_reg_lgpio0.bit.LGPIO_23 = GPIO_ID_EMUM_FUNC;
		top_reg_lgpio0.bit.LGPIO_24 = GPIO_ID_EMUM_FUNC;

		top_reg4.bit.ETH = ETH_ID_ENUM_RGMII;

		if (config & PIN_ETH_CFG_EXTPHYCLK) {
			top_reg_dgpio0.bit.DGPIO_0 = GPIO_ID_EMUM_FUNC;   // Clock output for PHY (25MHz)
			top_reg6.bit.EXT_PHYCLK = ETH_EXTPHY_CLK_FUNC;
		}
	}

	return E_OK;
}

static int pinmux_config_misc(uint32_t config)
{
	if (config == PIN_MISC_CFG_NONE) {
		return E_OK;
	}

	if (config & PIN_MISC_CFG_RTCLK) {
		if (top_reg7.bit.PWM1 == PWM_ENUM_PWM) {
			pr_err("RTCLK conflict with PWM1_1 \r\n");
			return E_OBJ;
		}

		if (top_reg9.bit.UART2 == UART2_ENUM_3RD_PINMUX) {
			pr_err("RTCLK conflict with UART2_3\r\n");
			return E_OBJ;
		}

		if(top_reg5.bit.SPI2 == SPI2_2ND_PINMUX) {
			pr_err("RTCLK conflict with SPI2_2\r\n");
			return E_OBJ;
		}

		top_reg6.bit.RTC_CLK = RTC_ENUM_RTCCLK;
		top_reg_pgpio0.bit.PGPIO_1 = GPIO_ID_EMUM_FUNC;

	}

	if (config & PIN_MISC_CFG_SPCLK_NONE) {
		//top_reg_lgpio0.bit.LGPIO_20 = GPIO_ID_EMUM_GPIO;
		//top_reg_pgpio0.bit.PGPIO_19 = GPIO_ID_EMUM_GPIO;
		top_reg3.bit.SP_CLK = SP_CLK_SEL_ENUM_GPIO;
	} else if (config & PIN_MISC_CFG_SPCLK) {
		if (top_reg6.bit.ETH == ETH_ID_ENUM_RMII) {
			pr_err("SP_CLK_1 conflict with ETH RMII\r\n");
			return E_OBJ;
		}

		top_reg_lgpio0.bit.LGPIO_20 = GPIO_ID_EMUM_FUNC;
		top_reg3.bit.SP_CLK = SP_CLK_SEL_ENUM_SPCLK;
	} else if (config & PIN_MISC_CFG_SPCLK_2ND) {
		if (top_reg7.bit.PI_CNT3 == PICNT_ENUM_PICNT) {
			pr_err("SP_CLK_2 conflict with PICNT3_1\r\n");
			return E_OBJ;
		}

		//check I2S_MCLK
		if (top_reg6.bit.AUDIO_MCLK == AUDIO_MCLK_MCLK) {
			pr_err("SP_CLK_2 conflict with AUDIO MCLK\r\n");
			return E_OBJ;
		}

		top_reg_pgpio0.bit.PGPIO_19 = GPIO_ID_EMUM_FUNC;
		top_reg3.bit.SP_CLK = SP_CLK_SEL_ENUM_SP_2_CLK;
	}

	if (config & PIN_MISC_CFG_SP2CLK_NONE) {
		//top_reg_pgpio0.bit.PGPIO_24 = GPIO_ID_EMUM_GPIO;
		//top_reg_pgpio0.bit.PGPIO_15 = GPIO_ID_EMUM_GPIO;
		top_reg6.bit.SP2_CLK = SP_CLK_SEL_ENUM_GPIO;
	} else if (config & PIN_MISC_CFG_SP2CLK) {
		if (top_reg7.bit.PI_CNT == PICNT_ENUM_PICNT) {
			pr_err("SP_CLK2 conflict with PICNT1_1\r\n");
			return E_OBJ;
		}

		if (top_reg6.bit.DIGITAL_MIC == DMIC_ENUM_DMIC) {
			pr_err("SP_CLK2 conflict with to DM_CLK: 0x%x\r\n", config);
			return E_OBJ;
		}

		top_reg_pgpio0.bit.PGPIO_24 = GPIO_ID_EMUM_FUNC;
		top_reg6.bit.SP2_CLK = SP2_CLK_SEL_ENUM_SP2CLK;
	} else if (config & PIN_MISC_CFG_SP2CLK_2ND) {
		if (top_reg5.bit.SPI == SPI_2ND_PINMUX) {
			pr_err("SP_CLK2_2 conflict with SPI_2\r\n");
			return E_OBJ;
		}

		if (top_reg9.bit.UART3 == UART3_ENUM_1ST_PINMUX) {
			pr_err("SP_CLK2_2 conflict with UART3_1\r\n");
			return E_OBJ;
		}

		if (top_reg5.bit.SIFCH3 == SIFCH_ENUM_SIF) {
			pr_err("SP_CLK2_2 conflict with SIF3\r\n");
			return E_OBJ;
		}

		top_reg_pgpio0.bit.PGPIO_15 = GPIO_ID_EMUM_FUNC;
		top_reg6.bit.SP2_CLK = SP2_CLK_SEL_ENUM_SP2_2_CLK;
	}

	return E_OK;
}

/*
	Set Primary LCD pinmux

	Select LCD pinmux with IDE1 controller

	@param[in] config   pinmux config (from configuration table)
	@param[in] pinmux   pinmux type

	@return
		- @b E_OK: success
		- @b E_PAR: invalid pinmux
		- @b E_OBJ: pinmux conflict
*/
static ER pinmux_select_primary_lcd(struct nvt_pinctrl_info *info, u32 config, u32 pinmux)
{
	u32 pinmux_type;
	union TOP_REG2 local_top_reg2;
	union TOP_REGLGPIO0 local_top_reg_lgpio0;
	unsigned long flags = 0;
	lcd_mask = 0x0;

	local_top_reg2.reg = TOP_GETREG(info, TOP_REG2_OFS);
	local_top_reg_lgpio0.reg = TOP_GETREG(info, TOP_REGLGPIO0_OFS);

	pinmux_type = pinmux & ~(PINMUX_LCD_SEL_FEATURE_MSK | PINMUX_PMI_CFG_MASK);

	/*1. check pinmux conflict */
	switch (pinmux_type) {
	case PINMUX_LCD_SEL_GPIO:
		break;
	case PINMUX_LCD_SEL_CCIR656_16BITS:
	case PINMUX_LCD_SEL_CCIR656:
	case PINMUX_LCD_SEL_SERIAL_RGB_8BITS:
	case PINMUX_LCD_SEL_SERIAL_RGB_6BITS:
	case PINMUX_LCD_SEL_SERIAL_YCbCr_8BITS:
	case PINMUX_LCD_SEL_CCIR601_16BITS:
	case PINMUX_LCD_SEL_CCIR601:
	case PINMUX_LCD_SEL_PARALLE_RGB666:
	case PINMUX_LCD_SEL_PARALLE_RGB888:
		//pr_err("Primary LCD configure fail -- Not support this format !\r\n");
	case PINMUX_LCD_SEL_PARALLE_RGB565:
	case PINMUX_LCD_SEL_RGB_16BITS:
		break;
	case PINMUX_LCD_SEL_PARALLE_MI_8BITS:
	case PINMUX_LCD_SEL_PARALLE_MI_9BITS:
	case PINMUX_LCD_SEL_PARALLE_MI_16BITS:
	case PINMUX_LCD_SEL_PARALLE_MI_18BITS:
	case PINMUX_LCD_SEL_SERIAL_MI_SDI_SDO:
	case PINMUX_LCD_SEL_SERIAL_MI_SDIO:
		pr_err("Primary LCD configure MI fail -- Not support MI !\r\n");
		return E_OBJ;
	case PINMUX_LCD_SEL_MIPI:
		break;
	default:
		pr_err("invalid primary LCD config: 0x%x\r\n", pinmux);
		return E_PAR;
	}

	/*2. setup pinmux registers*/
	local_top_reg2.bit.LCD_TYPE = LCDTYPE_ENUM_GPIO;
	local_top_reg2.bit.CCIR_HVLD_VVLD = CCIR_HVLD_VVLD_GPIO;
	local_top_reg2.bit.PLCD_DE = PLCD_DE_ENUM_GPIO;
	switch (pinmux_type) {
	case PINMUX_LCD_SEL_GPIO:
		break;
	case PINMUX_LCD_SEL_CCIR656:
	case PINMUX_LCD_SEL_CCIR656_16BITS:
		if (pinmux_type == PINMUX_LCD_SEL_CCIR656)
			local_top_reg2.bit.CCIR_DATA_WIDTH = CCIR_DATA_WIDTH_8BITS;
		else
			local_top_reg2.bit.CCIR_DATA_WIDTH = CCIR_DATA_WIDTH_16BITS;

		local_top_reg2.bit.LCD_TYPE = LCDTYPE_ENUM_CCIR656;
		break;
	case PINMUX_LCD_SEL_CCIR601:
		if (pinmux & PINMUX_LCD_SEL_HVLD_VVLD)
			local_top_reg2.bit.CCIR_HVLD_VVLD = CCIR_HVLD_VVLD_EN;

		if (pinmux & PINMUX_LCD_SEL_FIELD)
			local_top_reg2.bit.CCIR_FIELD = CCIR_FIELD_EN;

		local_top_reg2.bit.CCIR_DATA_WIDTH = CCIR_DATA_WIDTH_8BITS;
		local_top_reg2.bit.LCD_TYPE = LCDTYPE_ENUM_CCIR601;
		break;
	case PINMUX_LCD_SEL_CCIR601_16BITS:
		local_top_reg2.bit.CCIR_DATA_WIDTH = CCIR_DATA_WIDTH_16BITS;
		local_top_reg2.bit.LCD_TYPE = LCDTYPE_ENUM_CCIR601;
		break;
	case PINMUX_LCD_SEL_PARALLE_RGB565:
		local_top_reg2.bit.LCD_TYPE = LCDTYPE_ENUM_PARALLEL_LCD;
		break;
	case PINMUX_LCD_SEL_SERIAL_RGB_8BITS:
		local_top_reg2.bit.LCD_TYPE = LCDTYPE_ENUM_SERIALRGB_8BITS;
		break;
	case PINMUX_LCD_SEL_SERIAL_RGB_6BITS:
		local_top_reg2.bit.LCD_TYPE = LCDTYPE_ENUM_SERIALRGB_6BITS;
		break;
	case PINMUX_LCD_SEL_SERIAL_YCbCr_8BITS:
		local_top_reg2.bit.LCD_TYPE = LCDTYPE_ENUM_SERIALYCBCR_8BITS;
		break;
	case PINMUX_LCD_SEL_RGB_16BITS:
		local_top_reg2.bit.LCD_TYPE = LCDTYPE_ENUM_RGB_16BITS;
		break;
	case PINMUX_LCD_SEL_MIPI:
		local_top_reg2.bit.LCD_TYPE = LCDTYPE_ENUM_MIPI_DSI;
		if (pinmux & PINMUX_LCD_SEL_TE_ENABLE) {
			lcd_mask |= BIT(22);
			local_top_reg_lgpio0.bit.LGPIO_22 = GPIO_ID_EMUM_FUNC;
			local_top_reg2.bit.TE_SEL = TE_SEL_EN;
		}
		break;
	case PINMUX_LCD_SEL_PARALLE_RGB666:
		local_top_reg2.bit.LCD_TYPE = LCDTYPE_ENUM_PARARGB666_LCD;
		break;
	case PINMUX_LCD_SEL_PARALLE_RGB888:
		local_top_reg2.bit.LCD_TYPE = LCDTYPE_ENUM_PARARGB888_LCD;
		break;
	default:
		break;
	}

	if (pinmux & PINMUX_LCD_SEL_DE_ENABLE) {
		lcd_mask |= BIT(11);
		local_top_reg_lgpio0.bit.LGPIO_11 = GPIO_ID_EMUM_FUNC;
		local_top_reg2.bit.PLCD_DE = PLCD_DE_ENUM_DE;
	}

	if ((pinmux_type >= PINMUX_LCD_SEL_CCIR656) && (pinmux_type <= PINMUX_LCD_SEL_RGB_16BITS)) {
		if ((pinmux_type != PINMUX_LCD_SEL_CCIR656) && (pinmux_type != PINMUX_LCD_SEL_CCIR656_16BITS)) {
			if (!(pinmux & PINMUX_LCD_SEL_NO_HVSYNC)) {
				lcd_mask |= (BIT(9) | BIT(10));
				local_top_reg_lgpio0.bit.LGPIO_9 = GPIO_ID_EMUM_FUNC;
				local_top_reg_lgpio0.bit.LGPIO_10 = GPIO_ID_EMUM_FUNC;
			} else {
				if (nvt_get_chip_id() == CHIP_NA51084) {
					pr_err("[WARNNING] 528 not support PINMUX_LCD_SEL_NO_HVSYNC\r\n");
					return E_OBJ;
				}
			}
		}
		if (pinmux_type != PINMUX_LCD_SEL_SERIAL_RGB_6BITS) {
			lcd_mask |= (BIT(0) | BIT(1));
			local_top_reg_lgpio0.bit.LGPIO_0 = GPIO_ID_EMUM_FUNC;
			local_top_reg_lgpio0.bit.LGPIO_1 = GPIO_ID_EMUM_FUNC;
		}

		lcd_mask |= (BIT(2) | BIT(3) | BIT(4) | BIT(5) | BIT(6) | BIT(7) | BIT(8));
		local_top_reg_lgpio0.bit.LGPIO_2 = GPIO_ID_EMUM_FUNC;
		local_top_reg_lgpio0.bit.LGPIO_3 = GPIO_ID_EMUM_FUNC;
		local_top_reg_lgpio0.bit.LGPIO_4 = GPIO_ID_EMUM_FUNC;
		local_top_reg_lgpio0.bit.LGPIO_5 = GPIO_ID_EMUM_FUNC;
		local_top_reg_lgpio0.bit.LGPIO_6 = GPIO_ID_EMUM_FUNC;
		local_top_reg_lgpio0.bit.LGPIO_7 = GPIO_ID_EMUM_FUNC;
		local_top_reg_lgpio0.bit.LGPIO_8 = GPIO_ID_EMUM_FUNC;

		if ((pinmux_type == PINMUX_LCD_SEL_CCIR656_16BITS) || (pinmux_type == PINMUX_LCD_SEL_CCIR601_16BITS) ||
			(pinmux_type == PINMUX_LCD_SEL_PARALLE_RGB565) || (pinmux_type == PINMUX_LCD_SEL_PARALLE_RGB666) ||
			(pinmux_type == PINMUX_LCD_SEL_RGB_16BITS)) {
			lcd_mask |= (BIT(12) | BIT(13) | BIT(14) | BIT(15) | BIT(16) | BIT(17) | BIT(18) | BIT(19));
			local_top_reg_lgpio0.bit.LGPIO_12 = GPIO_ID_EMUM_FUNC;
			local_top_reg_lgpio0.bit.LGPIO_13 = GPIO_ID_EMUM_FUNC;
			local_top_reg_lgpio0.bit.LGPIO_14 = GPIO_ID_EMUM_FUNC;
			local_top_reg_lgpio0.bit.LGPIO_15 = GPIO_ID_EMUM_FUNC;
			local_top_reg_lgpio0.bit.LGPIO_16 = GPIO_ID_EMUM_FUNC;
			local_top_reg_lgpio0.bit.LGPIO_17 = GPIO_ID_EMUM_FUNC;
			local_top_reg_lgpio0.bit.LGPIO_18 = GPIO_ID_EMUM_FUNC;
			local_top_reg_lgpio0.bit.LGPIO_19 = GPIO_ID_EMUM_FUNC;
		}

		if ((pinmux_type == PINMUX_LCD_SEL_PARALLE_RGB666)) {
			lcd_mask |= (BIT(20) | BIT(21));
			local_top_reg_lgpio0.bit.LGPIO_20 = GPIO_ID_EMUM_FUNC;
			local_top_reg_lgpio0.bit.LGPIO_21 = GPIO_ID_EMUM_FUNC;
		}

		if ((pinmux_type == PINMUX_LCD_SEL_CCIR601)) {
			if (pinmux & PINMUX_LCD_SEL_HVLD_VVLD) {
				lcd_mask |= (BIT(12) | BIT(13));
				local_top_reg_lgpio0.bit.LGPIO_12 = GPIO_ID_EMUM_FUNC;
				local_top_reg_lgpio0.bit.LGPIO_13 = GPIO_ID_EMUM_FUNC;
			}
			if (pinmux & PINMUX_LCD_SEL_FIELD) {
				lcd_mask |= BIT(14);
				local_top_reg_lgpio0.bit.LGPIO_14 = GPIO_ID_EMUM_FUNC;
			}
		}
	}

	/*Enter critical section*/
	loc_cpu(&top_lock, flags);

	TOP_SETREG(info, TOP_REG2_OFS, local_top_reg2.reg);
	TOP_SETREG(info, TOP_REGLGPIO0_OFS, local_top_reg_lgpio0.reg);
	pr_debug("%s: pinmux_type = 0x%x, lcd_mask = 0x%x\r\n", __func__, pinmux_type, lcd_mask);

	/*Leave critical section*/
	unl_cpu(&top_lock, flags);

	return E_OK;
}

/**
	Set Function PINMUX

	Set Function PINMUX.

	@param[in] id   Function ID
			- @b PINMUX_FUNC_ID_LCD: 1st LCD
			- @b PINMUX_FUNC_ID_LCD2: 2nd LCD
			- @b PINMUX_FUNC_ID_TV: TV
			- @b PINMUX_FUNC_ID_HDMI: HDMI
	@param[in] pinmux pinmux setting

	@return
		- @b E_OK: success
		- @b E_ID: id out of range
*/
int pinmux_set_host(struct nvt_pinctrl_info *info, PINMUX_FUNC_ID id, u32 pinmux)
{
	ER ret = E_OK;
	switch ((u32)id) {
	case PINMUX_FUNC_ID_LCD:
	case PINMUX_FUNC_ID_LCD2: {
		u32 lcd_location = disp_pinmux_config[id] & PINMUX_DISPMUX_SEL_MASK;
		if (lcd_location == PINMUX_DISPMUX_SEL_LCD)
			ret = pinmux_select_primary_lcd(info, disp_pinmux_config[id], pinmux);
		}
		break;
	default:
		ret = E_ID;
		break;
	}

	return ret;
}
