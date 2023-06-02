/*
 * drivers/net/phy/na51055.c
 *
 * Driver for Novatek PHYs
 *
 * Copyright (c) 2019 Novatek Microelectronics Corp.
 *
 */
#include <linux/phy.h>
#include <linux/module.h>
//#include <linux/gpio.h>
//#include <nvt-gpio.h>
//#include <nvt-gpio.h>
#include <plat/top.h>
#include <linux/delay.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#endif

#include "na51055_reg.h"
static int bplug = 0;

static enum phy_state na510xx_phy_state = PHY_DOWN;

/**
 * phy_poll_reset - Safely wait until a PHY reset has properly completed
 * @phydev: The PHY device to poll
 *
 * Description: According to IEEE 802.3, Section 2, Subsection 22.2.4.1.1, as
 *   published in 2008, a PHY reset may take up to 0.5 seconds.  The MII BMCR
 *   register must be polled until the BMCR_RESET bit clears.
 *
 *   Furthermore, any attempts to write to PHY registers may have no effect
 *   or even generate MDIO bus errors until this is complete.
 *
 *   Some PHYs (such as the Marvell 88E1111) don't entirely conform to the
 *   standard and do not fully reset after the BMCR_RESET bit is set, and may
 *   even *REQUIRE* a soft-reset to properly restart autonegotiation.  In an
 *   effort to support such broken PHYs, this function is separate from the
 *   standard phy_init_hw() which will zero all the other bits in the BMCR
 *   and reapply all driver-specific and board-specific fixups.
 */
static int phy_poll_reset(struct phy_device *phydev)
{
	/* Poll until the reset bit clears (50ms per retry == 0.6 sec) */
	unsigned int retries = 12;
	int ret;

	do {
		msleep(50);
		ret = phy_read(phydev, MII_BMCR);
		if (ret < 0)
			return ret;
	} while (ret & BMCR_RESET && --retries);
	if (ret & BMCR_RESET)
		return -ETIMEDOUT;

	/* Some chips (smsc911x) may still need up to another 1ms after the
	 * BMCR_RESET bit is cleared before they are usable.
	 */
	msleep(1);
	return 0;
}

/**
 * genphy_soft_reset - software reset the PHY via BMCR_RESET bit
 * @phydev: target phy_device struct
 *
 * Description: Perform a software PHY reset using the standard
 * BMCR_RESET bit and poll for the reset bit to be cleared.
 *
 * Returns: 0 on success, < 0 on failure
 */
static int nvt_soft_reset(struct phy_device *phydev)
{
	int val;
	int ret;

   	val = phy_read(phydev, MII_BMCR);
	if (val < 0)
		return val;

    val |= BMCR_RESET;

	ret = phy_write(phydev, MII_BMCR, val);
	if (ret < 0)
		return ret;

	return phy_poll_reset(phydev);
}

static int nvt_config_aneg(struct phy_device *phydev)
{
	int ret;

	if (phydev->autoneg) {
		iowrite32(0xE0, (void *)(0xFD2B3800 + 0x380));
	} else {
		iowrite32(0xA0, (void *)(0xFD2B3800 + 0x380));
	}
	ret = genphy_config_aneg(phydev);
	if (ret) return ret;

	if (phydev->autoneg) {
		ret = genphy_restart_aneg(phydev);
	}

	return ret;
}

static void eth_phy_poweron(void)
{
	unsigned long reg;

	reg = ioread32((void*)(0xFD2B3800 + 0xF8));
	iowrite32(reg | (1<<7), (void *)(0xFD2B3800 + 0xF8));
	udelay(20);
	reg = ioread32((void*)(0xFD2B3800 + 0xC8));
	iowrite32(reg & (~(1<<0)), (void *)(0xFD2B3800 + 0xC8));
	udelay(200);
	reg = ioread32((void*)(0xFD2B3800 + 0xC8));
	iowrite32(reg & (~(1<<1)), (void *)(0xFD2B3800 + 0xC8));
	udelay(250);
	reg = ioread32((void*)(0xFD2B3800 + 0x2E8));
	iowrite32(reg & (~(1<<0)), (void *)(0xFD2B3800 + 0x2E8));
	reg = ioread32((void*)(0xFD2B3800 + 0xCC));
	iowrite32(reg & (~(1<<0)), (void *)(0xFD2B3800 + 0xCC));
	reg = ioread32((void*)(0xFD2B3800 + 0xDC));
	iowrite32(reg | (1<<0), (void *)(0xFD2B3800 + 0xDC));
	reg = ioread32((void*)(0xFD2B3800 + 0x9C));
	iowrite32(reg & (~(1<<0)), (void *)(0xFD2B3800 + 0x9C));
}

static int nvt_resume(struct phy_device *phydev)
{
	int ret = 0;
	int inv_led = 0;
	PIN_GROUP_CONFIG pinmux_config[1] = {0};
#ifdef CONFIG_OF
	struct device_node* of_node = of_find_node_by_path("/phy@f02b3800");
#endif

	na510xx_phy_state = phydev->state;

#ifdef CONFIG_OF
	if (of_node) {
		printk("%s: find node\r\n", __func__);
		if(!of_property_read_u32(of_node, "led-inv", &inv_led)) {
			printk("%s: led-inv found\r\n", __func__);
		}
	}
#endif
	printk("%s: led-inv = %d\r\n", __func__, inv_led);

	pinmux_config[0].pin_function = PIN_FUNC_ETH;
	ret = nvt_pinmux_capture(pinmux_config, 1);
	if (ret) {
		printk("%s: pinmux not found\r\n", __func__);
	}
	printk("%s: pinmux 0x%x\r\n", __func__, (int)pinmux_config[0].config);

	eth_phy_poweron();

	set_best_setting();
	phy_sw_reset_enable();
	set_break_link_timer();
#if defined(CONFIG_NVT_IVOT_PLAT_NA51055)
	if ((pinmux_config[0].config&PIN_ETH_CFG_LED_1ST_ONLY) ||
			(pinmux_config[0].config&PIN_ETH_CFG_LED_2ND_ONLY)) {
		set_one_led_link_act();
	} else {
		set_two_leds_link_act();
	}
#endif
	if (inv_led) {
		set_led_inv();
	}
	mdelay(10);
	phy_sw_reset_disable();
	genphy_config_aneg(phydev);

	return 0;
}

static int nvt_suspend(struct phy_device *phydev)
{
	bplug = 0;

	return 0;
//	return genphy_suspend(phydev);
}

static void nvt_link_change_notify(struct phy_device *phydev)
{
	if (((na510xx_phy_state!=PHY_HALTED) && (phydev->state == PHY_HALTED)) ||
		((na510xx_phy_state==PHY_CHANGELINK) && (phydev->state == PHY_NOLINK))) {
		bplug = 0;
		eq_reset_enable();
		msleep(50);
		eq_reset_disable();
	} else if ((bplug==0) && (phydev->state == PHY_RUNNING)) {
		phy_write(phydev, 0x1F, 0xC00);
		bplug = 1;
	}
	na510xx_phy_state = phydev->state;
}


static struct phy_driver novatek_drv[] = {
	{
		.phy_id             = 0x00000001,
		.name               = "Novatek Fast Ethernet Phy",
		.phy_id_mask        = 0x001fffff,
		.features           = PHY_BASIC_FEATURES,
		.soft_reset         = nvt_soft_reset,
		.config_init        = genphy_config_init,
		.config_aneg        = nvt_config_aneg,
		.read_status        = genphy_read_status,
		.suspend            = nvt_suspend,
		.resume	            = nvt_resume,
		.link_change_notify	= nvt_link_change_notify,
	},
};

module_phy_driver(novatek_drv);

static struct mdio_device_id __maybe_unused novatek_tbl[] = {
	{ 0x00000001, 0x001fffff },
	{ }
};

MODULE_DEVICE_TABLE(mdio, novatek_tbl);

MODULE_DESCRIPTION("Novatek Ethernet PHY Driver");
MODULE_VERSION("1.00.002");
MODULE_LICENSE("GPL");
