/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __DWMAC_AXERA_H__
#define __DWMAC_AXERA_H__

#include <linux/module.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>
#include <linux/phy.h>
#include <linux/soc/axera/ax_boardinfo.h>

#define FLASH_SYS_GLB_BASE_ADDR	0x10030000

#define EMAC_FLASH_CLK_MUX_0_ADDR (FLASH_SYS_GLB_BASE_ADDR + 0x0)
#define EMAC_RGMII_TX_SEL       4

#define EMAC_FLASH_SW_RST_ADDR (FLASH_SYS_GLB_BASE_ADDR + 0x14)
#define EMAC_SW_RST             8
#define EMAC_EPHY_SW_RST		9

#define EMAC_FLASH_EMAC0_ADDR (FLASH_SYS_GLB_BASE_ADDR + 0x28)
#define EMAC_RX_CLK_DLY_SEL     5
#define EMAC_PHY_IF_SEL         9
#define EMAC_EXT_PAD_SEL        10
#define EMAC_PHY_LOOPBACK_EN	11

#define EMAC_FLASH_CLK_EB0_ADDR (FLASH_SYS_GLB_BASE_ADDR + 0x4)
#define EMAC_RMII_PHY_EB	4
#define EMAC_RGMII_TX_EB 	3
#define EMAC_PTP_REF_EB 	2
#define EMAC_EPHY_CLK_EB 	13

#define EMAC_FLASH_CLK_EB1_ADDR (FLASH_SYS_GLB_BASE_ADDR + 0x8)
#define EMAC_BW_24M			8
#define EMAC_ACLK			2

#define EMAC_FLASH_SYS_GLB0_ADDR (FLASH_SYS_GLB_BASE_ADDR + 0x144)
#define EMAC_RMII_RX_DIVN		0
#define EMAC_RMII_RX_DIVN_UP	4
#define EMAC_RMII_RX_DIV_EN		5

#define EMAC_FLASH_EPHY_0_ADDR (FLASH_SYS_GLB_BASE_ADDR + 0x20)
#define EMAC_EPHY_SHUTDOWN		0
#define EMAC_EPHY_LED_POL		13
#define EMAC_EFUSE_2_EPHY_OTP_BG	24

#define EMAC_EPHY_LED0_PINMUX_ADDR (0x104F006C)
#define EMAC_EPHY_LED1_PINMUX_ADDR (0x104F0078)
//GPIO1_A28
#define EPHY_LED0_GPIO_NUM	(60)
//GPIO1_A29
#define EPHY_LED1_GPIO_NUM	(61)

/* bus clock freq */
#define EPHY_CLK_25M    25000000

//reset enable
#define EMAC_SYS_REST_EN  1

struct axera_eqos {
	struct device *dev;
	struct plat_stmmacenet_data *plat;
	struct reset_control *emac_rst;
	struct reset_control *ephy_rst;
	struct reset_control *ephy_shutdown;
	int phy_interface;
	int bus_clock;
	int gpiod_reset;
	int bus_id;
	unsigned char rmii_loopback_mode;
	unsigned char out_rmii_mode;
	struct clk *ephy_clk;
	struct clk *rgmii_tx_clk;
	struct clk *rmii_phy_clk;
	unsigned char led0_en;
	unsigned char led1_en;
	unsigned int led0_mode;
	unsigned int led1_mode;
	unsigned int polarity;
};

void select_phy_interface(struct plat_stmmacenet_data *plat_dat);
int emac_get_reset_control(struct axera_eqos *eqos);
void emac_sw_rst(struct axera_eqos *eqos);
void emac_clk_init(struct axera_eqos *eqos);
void axera_eqos_fix_speed(void *priv, unsigned int speed);
int axera_dwmac_config_dt(
			struct platform_device *pdev,
			struct plat_stmmacenet_data *plat,
			struct axera_eqos *eqos
			);
void ax_reset_phy(struct platform_device *pdev, void *private);
void ax_reset_emac(struct platform_device *pdev, void *private);
void ax_shutdown_phy(struct platform_device *pdev, void *private);
int plat_init(struct platform_device *pdev, void *private);
void plat_exit(struct platform_device *pdev, void *private);

#endif /* __COMMON_H__ */