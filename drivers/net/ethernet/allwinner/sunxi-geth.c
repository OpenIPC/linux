/*
* Allwinner ethernet mac driver.
*
* Copyright(c) 2022-2027 Allwinnertech Co., Ltd.
* Author: xuminghui <xuminghui@allwinnertech.com>
*
* This file is licensed under the terms of the GNU General Public
* License version 2.  This program is licensed "as is" without any
* warranty of any kind, whether express or implied.
*/
//#define DEBUG
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/mii.h>
#include <linux/gpio.h>
#include <linux/crc32.h>
#include <linux/skbuff.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/crypto.h>
#include <crypto/algapi.h>
#include <crypto/hash.h>
#include <linux/err.h>
#include <linux/scatterlist.h>
#include <linux/regulator/consumer.h>
#include <linux/of_net.h>
#include <linux/of_gpio.h>
#include <linux/io.h>
#include <linux/sunxi-sid.h>
#include <linux/sunxi-gpio.h>
#include <linux/reset.h>
#include <linux/pwm.h>
#include "sunxi-geth.h"

#define SUNXI_GMAC_VERSION "2.0.2"

#define AC200_EPHY	"ac200-ephy"
#define AC300_EPHY	"ac300-ephy"
#define AC300_DEV	"ac300"

#define DMA_DESC_RX	256
#define DMA_DESC_TX	256
#define BUDGET		(dma_desc_rx / 4)
#define TX_THRESH	(dma_desc_tx / 4)

#define HASH_TABLE_SIZE	64
#define MAX_BUF_SZ	(SZ_2K - 1)

#define PWM_DUTY_NS	205
#define PWM_PERIOD_NS	410
#define POWER_CHAN_NUM	3

#undef PKT_DEBUG
#undef DESC_PRINT

#define circ_cnt(head, tail, size) (((head) > (tail)) ? \
					((head) - (tail)) : \
					((head) - (tail)) & ((size) - 1))

#define circ_space(head, tail, size) circ_cnt((tail), ((head) + 1), (size))

#define circ_inc(n, s) (((n) + 1) % (s))

#define GETH_MAC_ADDRESS "00:00:00:00:00:00"
static char *mac_str = GETH_MAC_ADDRESS;
module_param(mac_str, charp, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mac_str, "MAC Address String.(xx:xx:xx:xx:xx:xx)");

static int rxmode = 1;
module_param(rxmode, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(rxmode, "DMA threshold control value");

static int txmode = 1;
module_param(txmode, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(txmode, "DMA threshold control value");

static int pause = 0x400;
module_param(pause, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(pause, "Flow Control Pause Time");

#define TX_TIMEO	5000
static int watchdog = TX_TIMEO;
module_param(watchdog, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(watchdog, "Transmit timeout in milliseconds");

static int dma_desc_rx = DMA_DESC_RX;
module_param(dma_desc_rx, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(dma_desc_rx, "The number of receive's descriptors");

static int dma_desc_tx = DMA_DESC_TX;
module_param(dma_desc_tx, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(dma_desc_tx, "The number of transmit's descriptors");

/* - 0: Flow Off
 * - 1: Rx Flow
 * - 2: Tx Flow
 * - 3: Rx & Tx Flow
 */
static int flow_ctrl;
module_param(flow_ctrl, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(flow_ctrl, "Flow control [0: off, 1: rx, 2: tx, 3: both]");

struct geth_priv {
	struct dma_desc *dma_tx;
	struct sk_buff **tx_sk;
	unsigned int tx_clean;
	unsigned int tx_dirty;
	dma_addr_t dma_tx_phy;

	unsigned long buf_sz;

	struct dma_desc *dma_rx;
	struct sk_buff **rx_sk;
	unsigned int rx_clean;
	unsigned int rx_dirty;
	dma_addr_t dma_rx_phy;

	struct net_device *ndev;
	struct device *dev;
	struct napi_struct napi;

	struct geth_extra_stats xstats;

	struct mii_bus *mii;
	int link;
	int speed;
	int duplex;

#define INT_PHY				0	/* Internal phy , such as ephy */
#define EXT_PHY				1	/* External phy , ac300(phy_id = 0xc0000000 is EXT_PHY)*/
	u32 phy_ext;

#define SUPPLY_GENERAL_POWER 		0	/* Common power supply */
#define SUPPLY_INDEPENDENT_POWER 	1	/* Independent device power supply , such as axpXXX */
	u32 supply_type;

#define RST_SOFTWARE_TYPE		0	/* Reset requires only software, not hardware pins */
#define RST_HARDWARE_TYPE		1	/* Hardware pins are required for reset */
	u32 phy_rst_type;

#define INT_CLK_TYPE			0	/* Internal clk 25M , provided by SOC */
#define EXT_CLK_TYPE			1	/* External clk 25M , provided by crystal oscillator */
	u32 phy_clk_type;

	int phy_interface;
	void __iomem *base;
	void __iomem *base_phy;
	struct clk *geth_clk;
	struct clk *ephy_clk;
	struct reset_control *reset;
	struct pinctrl *pinctrl;

	struct regulator *gmac_power[POWER_CHAN_NUM];
	bool is_suspend;
	int phyrst;
	u8  rst_active_low;
	/* definition spinlock */
	spinlock_t lock;
	spinlock_t tx_lock;

	/* whether using ephy_clk */
	int use_ephy_clk;
	int phy_addr;

	/* wake-on-lane */
	int wol_pin;
	int wol_irq;
	int wolopts;

	char *phy_name;

	/* adjust transmit clock delay, value: 0~7 */
	/* adjust receive clock delay, value: 0~31 */
	u32 tx_delay;
	u32 rx_delay;

	/* ephy clk */
	struct pwm_device *pwm;
	u32 pwm_channel;

	/* resume work */
	struct work_struct eth_work;

	/* for debug phy reg */
	struct mii_reg_dump mii_reg;

	/* special for ac300 */
	struct phy_device *spec_phydev;
	int spec_phy_addr;
};

static u64 geth_dma_mask = DMA_BIT_MASK(32);

void sunxi_udelay(int n)
{
	udelay(n);
}

static int geth_stop(struct net_device *ndev);
static int geth_open(struct net_device *ndev);
static void geth_tx_complete(struct geth_priv *priv);
static void geth_rx_refill(struct net_device *ndev);

#ifdef CONFIG_GETH_ATTRS
static ssize_t adjust_bgs_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int value = 0;
	u32 efuse_value;
	struct net_device *ndev = to_net_dev(dev);
	struct geth_priv *priv = netdev_priv(ndev);

	if (priv->phy_ext == INT_PHY) {
		value = readl(priv->base_phy) >> 28;
		if (sunxi_efuse_read(EFUSE_OEM_NAME, &efuse_value) != 0)
			pr_err("get PHY efuse fail!\n");
		else
#if IS_ENABLED(CONFIG_ARCH_SUN50IW2)
			value = value - ((efuse_value >> 24) & 0x0F);
#else
			pr_warn("miss config come from efuse!\n");
#endif
	}

	return sprintf(buf, "bgs: %d\n", value);
}

static ssize_t adjust_bgs_write(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	unsigned int out = 0;
	struct net_device *ndev = to_net_dev(dev);
	struct geth_priv *priv = netdev_priv(ndev);
	u32 clk_value = readl(priv->base_phy);
	u32 efuse_value;

	out = simple_strtoul(buf, NULL, 10);

	if (priv->phy_ext == INT_PHY) {
		clk_value &= ~(0xF << 28);
		if (sunxi_efuse_read(EFUSE_OEM_NAME, &efuse_value) != 0)
			pr_err("get PHY efuse fail!\n");
		else
#if IS_ENABLED(CONFIG_ARCH_SUN50IW2)
			clk_value |= (((efuse_value >> 24) & 0x0F) + out) << 28;
#else
			pr_warn("miss config come from efuse!\n");
#endif
	}

	writel(clk_value, priv->base_phy);

	return count;
}

static struct device_attribute adjust_reg[] = {
	__ATTR(adjust_bgs, 0664, adjust_bgs_show, adjust_bgs_write),
};

static int geth_create_attrs(struct net_device *ndev)
{
	int j, ret;

	for (j = 0; j < ARRAY_SIZE(adjust_reg); j++) {
		ret = device_create_file(&ndev->dev, &adjust_reg[j]);
		if (ret)
			goto sysfs_failed;
	}
	goto succeed;

sysfs_failed:
	while (j--)
		device_remove_file(&ndev->dev, &adjust_reg[j]);
succeed:
	return ret;
}
#endif

#ifdef DEBUG
static void desc_print(struct dma_desc *desc, int size)
{
#ifdef DESC_PRINT
	int i;

	for (i = 0; i < size; i++) {
		u32 *x = (u32 *)(desc + i);

		pr_info("\t%d [0x%08lx]: %08x %08x %08x %08x\n",
			i, (unsigned long)(&desc[i]),
			x[0], x[1], x[2], x[3]);
	}
	pr_info("\n");
#endif
}
#endif

static ssize_t extra_tx_stats_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	struct geth_priv *priv = netdev_priv(ndev);

	if (!dev) {
		pr_err("Argment is invalid\n");
		return 0;
	}

	if (!ndev) {
		pr_err("Net device is null\n");
		return 0;
	}

	return sprintf(buf, "tx_underflow: %lu\ntx_carrier: %lu\n"
			"tx_losscarrier: %lu\nvlan_tag: %lu\n"
			"tx_deferred: %lu\ntx_vlan: %lu\n"
			"tx_jabber: %lu\ntx_frame_flushed: %lu\n"
			"tx_payload_error: %lu\ntx_ip_header_error: %lu\n\n",
			priv->xstats.tx_underflow, priv->xstats.tx_carrier,
			priv->xstats.tx_losscarrier, priv->xstats.vlan_tag,
			priv->xstats.tx_deferred, priv->xstats.tx_vlan,
			priv->xstats.tx_jabber, priv->xstats.tx_frame_flushed,
			priv->xstats.tx_payload_error, priv->xstats.tx_ip_header_error);
}
static DEVICE_ATTR(extra_tx_stats, 0444, extra_tx_stats_show, NULL);

static ssize_t extra_rx_stats_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	struct geth_priv *priv = netdev_priv(ndev);

	if (!dev) {
		pr_err("Argment is invalid\n");
		return 0;
	}

	if (!ndev) {
		pr_err("Net device is null\n");
		return 0;
	}

	return sprintf(buf, "rx_desc: %lu\nsa_filter_fail: %lu\n"
			"overflow_error: %lu\nipc_csum_error: %lu\n"
			"rx_collision: %lu\nrx_crc: %lu\n"
			"dribbling_bit: %lu\nrx_length: %lu\n"
			"rx_mii: %lu\nrx_multicast: %lu\n"
			"rx_gmac_overflow: %lu\nrx_watchdog: %lu\n"
			"da_rx_filter_fail: %lu\nsa_rx_filter_fail: %lu\n"
			"rx_missed_cntr: %lu\nrx_overflow_cntr: %lu\n"
			"rx_vlan: %lu\n\n",
			priv->xstats.rx_desc, priv->xstats.sa_filter_fail,
			priv->xstats.overflow_error, priv->xstats.ipc_csum_error,
			priv->xstats.rx_collision, priv->xstats.rx_crc,
			priv->xstats.dribbling_bit, priv->xstats.rx_length,
			priv->xstats.rx_mii, priv->xstats.rx_multicast,
			priv->xstats.rx_gmac_overflow, priv->xstats.rx_length,
			priv->xstats.da_rx_filter_fail, priv->xstats.sa_rx_filter_fail,
			priv->xstats.rx_missed_cntr, priv->xstats.rx_overflow_cntr,
			priv->xstats.rx_vlan);
}
static DEVICE_ATTR(extra_rx_stats, 0444, extra_rx_stats_show, NULL);

static ssize_t gphy_test_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct net_device *ndev = dev_get_drvdata(dev);

	if (!dev) {
		pr_err("Argment is invalid\n");
		return 0;
	}

	if (!ndev) {
		pr_err("Net device is null\n");
		return 0;
	}

	return sprintf(buf, "Usage:\necho [0/1/2/3/4] > gphy_test\n"
			"0 - Normal Mode\n"
			"1 - Transmit Jitter Test\n"
			"2 - Transmit Jitter Test(MASTER mode)\n"
			"3 - Transmit Jitter Test(SLAVE mode)\n"
			"4 - Transmit Distortion Test\n\n");
}

static ssize_t gphy_test_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	struct geth_priv *priv = netdev_priv(ndev);
	u16 value = 0;
	int ret = 0;
	u16 data = 0;

	if (!dev) {
		pr_err("Argument is invalid\n");
		return count;
	}

	if (!ndev) {
		pr_err("Net device is null\n");
		return count;
	}

	data = sunxi_mdio_read(priv->base, priv->phy_addr, MII_CTRL1000);

	ret = kstrtou16(buf, 0, &value);
	if (ret)
		return ret;

	if (value >= 0 && value <= 4) {
		data &= ~(0x7 << 13);
		data |= value << 13;
		sunxi_mdio_write(priv->base, priv->phy_addr, MII_CTRL1000, data);
		pr_info("Set MII_CTRL1000(0x09) Reg: 0x%x\n", data);
	} else {
		pr_info("unknown value (%d)\n", value);
	}

	return count;
}

static DEVICE_ATTR(gphy_test, 0664, gphy_test_show, gphy_test_store);

static ssize_t mii_read_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct net_device *ndev = NULL;
	struct geth_priv *priv = NULL;

	if (dev == NULL) {
		pr_err("Argment is invalid\n");
		return 0;
	}

	ndev = dev_get_drvdata(dev);
	if (ndev == NULL) {
		pr_err("Net device is null\n");
		return 0;
	}

	priv = netdev_priv(ndev);
	if (priv == NULL) {
		pr_err("geth_priv is null\n");
		return 0;
	}

	if (!netif_running(ndev)) {
		pr_warn("eth is down!\n");
		return 0;
	}

	priv->mii_reg.value = sunxi_mdio_read(priv->base, priv->mii_reg.addr, priv->mii_reg.reg);
	return sprintf(buf, "ADDR[0x%02x]:REG[0x%02x] = 0x%04x\n",
		       priv->mii_reg.addr, priv->mii_reg.reg, priv->mii_reg.value);
}

static ssize_t mii_read_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct net_device *ndev = NULL;
	struct geth_priv *priv = NULL;
	int ret = 0;
	u16 reg, addr;
	char *ptr;

	ptr = (char *)buf;

	if (dev == NULL) {
		pr_err("Argment is invalid\n");
		return count;
	}

	ndev = dev_get_drvdata(dev);
	if (ndev == NULL) {
		pr_err("Net device is null\n");
		return count;
	}

	priv = netdev_priv(ndev);
	if (priv == NULL) {
		pr_err("geth_priv is null\n");
		return count;
	}

	if (!netif_running(ndev)) {
		pr_warn("eth is down!\n");
		return count;
	}

	ret = sunxi_parse_read_str(ptr, &addr, &reg);
	if (ret)
		return ret;

	priv->mii_reg.addr = addr;
	priv->mii_reg.reg = reg;

	return count;
}

static DEVICE_ATTR(mii_read, 0664, mii_read_show, mii_read_store);

static ssize_t mii_write_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct net_device *ndev = NULL;
	struct geth_priv *priv = NULL;
	u16 bef_val, aft_val;

	if (dev == NULL) {
		pr_err("Argment is invalid\n");
		return 0;
	}

	ndev = dev_get_drvdata(dev);
	if (ndev == NULL) {
		pr_err("Net device is null\n");
		return 0;
	}

	priv = netdev_priv(ndev);
	if (priv == NULL) {
		pr_err("geth_priv is null\n");
		return 0;
	}

	if (!netif_running(ndev)) {
		pr_warn("eth is down!\n");
		return 0;
	}

	bef_val = sunxi_mdio_read(priv->base, priv->mii_reg.addr, priv->mii_reg.reg);
	sunxi_mdio_write(priv->base, priv->mii_reg.addr, priv->mii_reg.reg, priv->mii_reg.value);
	aft_val = sunxi_mdio_read(priv->base, priv->mii_reg.addr, priv->mii_reg.reg);
	return sprintf(buf, "before ADDR[0x%02x]:REG[0x%02x] = 0x%04x\n"
			    "after  ADDR[0x%02x]:REG[0x%02x] = 0x%04x\n",
			    priv->mii_reg.addr, priv->mii_reg.reg, bef_val,
			    priv->mii_reg.addr, priv->mii_reg.reg, aft_val);
}

static ssize_t mii_write_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct net_device *ndev = NULL;
	struct geth_priv *priv = NULL;
	int ret = 0;
	u16 reg, addr, val;
	char *ptr;

	ptr = (char *)buf;

	if (dev == NULL) {
		pr_err("Argment is invalid\n");
		return count;
	}

	ndev = dev_get_drvdata(dev);
	if (ndev == NULL) {
		pr_err("Net device is null\n");
		return count;
	}

	priv = netdev_priv(ndev);
	if (priv == NULL) {
		pr_err("geth_priv is null\n");
		return count;
	}

	if (!netif_running(ndev)) {
		pr_warn("eth is down!\n");
		return count;
	}

	ret = sunxi_parse_write_str(ptr, &addr, &reg, &val);
	if (ret)
		return ret;

	priv->mii_reg.reg = reg;
	priv->mii_reg.addr = addr;
	priv->mii_reg.value = val;

	return count;
}

static DEVICE_ATTR(mii_write, 0664, mii_write_show, mii_write_store);

static ssize_t loopback_test_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "Usage:\necho [0/1/2] > loopback_test\n"
			"0 - Normal Mode\n"
			"1 - Mac loopback test mode\n"
			"2 - Phy loopback test mode\n");
}

static ssize_t loopback_test_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct net_device *ndev = NULL;
	struct geth_priv *priv = NULL;
	u16 value = 0;
	int ret = 0;
	u16 data = 0;

	if (dev == NULL) {
		pr_err("Argment is invalid\n");
		return count;
	}

	ndev = dev_get_drvdata(dev);
	if (ndev == NULL) {
		pr_err("Net device is null\n");
		return count;
	}

	priv = netdev_priv(ndev);
	if (priv == NULL) {
		pr_err("geth_priv is null\n");
		return count;
	}

	if (!netif_running(ndev)) {
		pr_warn("eth is down!\n");
		return count;
	}

	ret = kstrtou16(buf, 0, &value);
	if (ret)
		return ret;

	if (value == 0) { /* normal mode */
		/* clear mac loopback */
		sunxi_mac_loopback(priv->base, 0);

		/* clear phy loopback */
		data = sunxi_mdio_read(priv->base, priv->phy_addr, MII_BMCR);
		sunxi_mdio_write(priv->base, priv->phy_addr, MII_BMCR, data & ~BMCR_LOOPBACK);
	} else if (value == 1) { /* mac loopback test mode */
		data = sunxi_mdio_read(priv->base, priv->phy_addr, MII_BMCR);
		sunxi_mdio_write(priv->base, priv->phy_addr, MII_BMCR, data & ~BMCR_LOOPBACK);

		sunxi_mac_loopback(priv->base, 1);
	} else if (value == 2) { /* phy loopback test mode */
		sunxi_mac_loopback(priv->base, 0);

		data = sunxi_mdio_read(priv->base, priv->phy_addr, MII_BMCR);
		sunxi_mdio_write(priv->base, priv->phy_addr, MII_BMCR, data | BMCR_LOOPBACK);
	} else {
		pr_err("Undefined value (%d)\n", value);
	}

	return count;
}
static DEVICE_ATTR(loopback_test, 0664, loopback_test_show, loopback_test_store);


static int geth_power_on(struct geth_priv *priv)
{
	int value;
	int i;

	value = readl(priv->base_phy);
	value &= ~(1 << 15);

	/*
	 * GMAC can choose independent power supply,
	 * but the latest Soc not use this way.
	 */
	if (priv->supply_type == SUPPLY_INDEPENDENT_POWER) {
		for (i = 0; i < POWER_CHAN_NUM; i++) {
			if (IS_ERR_OR_NULL(priv->gmac_power[i]))
				continue;
			if (regulator_enable(priv->gmac_power[i]) != 0) {
				pr_err("gmac-power%d enable error\n", i);
				return -EINVAL;
			}
		}
	}

	writel(value, priv->base_phy);

	return 0;
}

static void geth_power_off(struct geth_priv *priv)
{
	int value;
	int i;

	if (priv->phy_ext == EXT_PHY) {
		value = readl(priv->base_phy);
		value |= (1 << 16);
		writel(value, priv->base_phy);
	} else {
		for (i = 0; i < POWER_CHAN_NUM; i++) {
			if (IS_ERR_OR_NULL(priv->gmac_power[i]))
				continue;
			regulator_disable(priv->gmac_power[i]);
		}
	}
}

/* PHY interface operations */
static int geth_mdio_read(struct mii_bus *bus, int phyaddr, int phyreg)
{
	struct net_device *ndev = bus->priv;
	struct geth_priv *priv = netdev_priv(ndev);

	return (int)sunxi_mdio_read(priv->base,  phyaddr, phyreg);
}

static int geth_mdio_write(struct mii_bus *bus, int phyaddr,
			   int phyreg, u16 data)
{
	struct net_device *ndev = bus->priv;
	struct geth_priv *priv = netdev_priv(ndev);

	sunxi_mdio_write(priv->base, phyaddr, phyreg, data);

	return 0;
}

static int geth_mdio_reset(struct mii_bus *bus)
{
	struct net_device *ndev = bus->priv;
	struct geth_priv *priv = netdev_priv(ndev);

	return sunxi_mdio_reset(priv->base);
}

static void geth_adjust_link(struct net_device *ndev)
{
	struct geth_priv *priv = netdev_priv(ndev);
	struct phy_device *phydev = ndev->phydev;
	unsigned long flags;
	int new_state = 0;

	if (!phydev)
		return;

	spin_lock_irqsave(&priv->lock, flags);
	if (phydev->link) {
		/* Now we make sure that we can be in full duplex mode.
		 * If not, we operate in half-duplex mode.
		 */
		if (phydev->duplex != priv->duplex) {
			new_state = 1;
			priv->duplex = phydev->duplex;
		}
		/* Flow Control operation */
		if (phydev->pause)
			sunxi_flow_ctrl(priv->base, phydev->duplex,
					flow_ctrl, pause);

		if (phydev->speed != priv->speed) {
			new_state = 1;
			priv->speed = phydev->speed;
		}

		if (priv->link == 0) {
			new_state = 1;
			priv->link = phydev->link;
		}

		if (new_state)
			sunxi_set_link_mode(priv->base, priv->duplex, priv->speed);

#ifdef LOOPBACK_DEBUG
		phydev->state = PHY_FORCING;
#endif

	} else if (priv->link != phydev->link) {
		new_state = 1;
		priv->link = 0;
		priv->speed = 0;
		priv->duplex = -1;
	}
	spin_unlock_irqrestore(&priv->lock, flags);

	if (new_state)
		phy_print_status(phydev);
}

static void geth_phy_hw_reset(struct net_device *ndev)
{
	struct geth_priv *priv = netdev_priv(ndev);
	if (priv->phy_rst_type != RST_HARDWARE_TYPE)
		return;

	gpio_direction_output(priv->phyrst, priv->rst_active_low);
	msleep(50);
	gpio_direction_output(priv->phyrst, !priv->rst_active_low);
	msleep(50);
}

static int geth_mdiobus_get_phy(struct net_device *ndev,
				struct phy_device **dev,
				const char *dev_name,
				int *dev_addr)
{
	struct geth_priv *priv = netdev_priv(ndev);
	struct phy_device *tmp = NULL;
	int addr;

	for (addr = 0; addr < PHY_MAX_ADDR; addr++) {
		tmp = mdiobus_get_phy(priv->mii, addr);

		if ((IS_ERR_OR_NULL(tmp))
			|| (IS_ERR_OR_NULL(tmp->drv))
			|| (IS_ERR_OR_NULL(tmp->drv->name))) {
				continue;
		}

		if (tmp->phy_id == 0xffff) {
			if (!IS_ERR_OR_NULL(tmp))
				phy_device_remove(tmp);
			tmp = mdiobus_scan(priv->mii, addr);
		}

		if (strcmp(tmp->drv->name, dev_name) == 0) {
			netdev_dbg(priv->ndev, "PHY Name = %s, PHY Id = 0x%x\n",
					dev_name, tmp->phy_id);
			*dev = tmp;
			*dev_addr = addr;
			break;
		}
	}
	return 0;
}

static int geth_find_phy(struct net_device *ndev)
{
	struct geth_priv *priv = netdev_priv(ndev);

	/* FIX: if use ac300, we need to get ac300 handle */
	if (strcmp(priv->phy_name, AC300_EPHY) == 0) {
		priv->spec_phydev = NULL;
		geth_mdiobus_get_phy(ndev, &priv->spec_phydev, AC300_DEV, &priv->spec_phy_addr);
		if (!priv->spec_phydev) {
			netdev_err(ndev, "%s not found!\n", AC300_DEV);
			return -EINVAL;
		}

		/* init ac300 */
		phy_init_hw(priv->spec_phydev);

	}

	/* find phy addr */
	geth_mdiobus_get_phy(ndev, &ndev->phydev, priv->phy_name, &priv->phy_addr);
	if (!ndev->phydev) {
		netdev_err(ndev, "%s PHY not found!\n", priv->phy_name);
		return -EINVAL;
	}

	return 0;
}

static int geth_phy_init(struct net_device *ndev)
{
	int value, ret;
	struct mii_bus *new_bus;
	struct geth_priv *priv = netdev_priv(ndev);
	struct phy_device *phydev = ndev->phydev;

	new_bus = mdiobus_alloc();
	if (!new_bus) {
		netdev_err(ndev, "Failed to alloc new mdio bus\n");
		return -ENOMEM;
	}

	new_bus->name = dev_name(priv->dev);
	new_bus->read = &geth_mdio_read;
	new_bus->write = &geth_mdio_write;
	new_bus->reset = &geth_mdio_reset;
	snprintf(new_bus->id, MII_BUS_ID_SIZE, "%s-%x", new_bus->name, 0);
	new_bus->parent = priv->dev;
	new_bus->priv = ndev;

	if (mdiobus_register(new_bus)) {
		pr_err("%s: Cannot register as MDIO bus\n", new_bus->name);
		goto reg_fail;
	}

	priv->mii = new_bus;

	/* find phydev in mdio bus */
	ret = geth_find_phy(ndev);
	if (ret < 0) {
		ret =  -EINVAL;
		goto err;
	}

	/* save the oldest phy scan method */
	/*
	{
		int addr;

		for (addr = 0; addr < PHY_MAX_ADDR; addr++) {
			struct phy_device *phydev_tmp = mdiobus_get_phy(new_bus, addr);

			if (phydev_tmp && (phydev_tmp->phy_id != 0x00)) {
				phydev = phydev_tmp;
				priv->phy_addr = addr;
				break;
			}
		}
	}
	*/

	phydev = ndev->phydev;
	if (!phydev) {
		netdev_err(ndev, "No PHY found!\n");
		goto err;
	}

	phydev->irq = PHY_POLL;

	value = phy_connect_direct(ndev, phydev, &geth_adjust_link, priv->phy_interface);
	if (value) {
		netdev_err(ndev, "Could not attach to PHY\n");
		goto err;
	} else {
		netdev_info(ndev, "%s: Type(%d) PHY ID %08x at %d IRQ %s (%s)\n",
			    ndev->name, phydev->interface, phydev->phy_id,
			    phydev->mdio.addr, "poll", dev_name(&phydev->mdio.dev));
	}

	/*
	 * The self negotiation declaration needs
	 * to be placed after the PHY connection
	 */
	phydev->supported &= PHY_GBIT_FEATURES | PHY_BASIC_FEATURES;
	phydev->advertising = phydev->supported;

	return 0;

err:
	mdiobus_unregister(new_bus);
reg_fail:
	mdiobus_free(new_bus);

	return -EINVAL;
}

static int geth_phy_release(struct net_device *ndev)
{
	struct geth_priv *priv = netdev_priv(ndev);
	struct phy_device *phydev = ndev->phydev;
	int value = 0;

	/* Stop and disconnect the PHY */
	if (phydev)
		phy_stop(phydev);

	priv->link = PHY_DOWN;
	priv->speed = 0;
	priv->duplex = -1;

	if (phydev) {
		value = phy_read(phydev, MII_BMCR);
		phy_write(phydev, MII_BMCR, (value | BMCR_PDOWN));
	}

	if (phydev) {
		phy_disconnect(phydev);
		ndev->phydev = NULL;
	}

	if (priv->spec_phydev)
		phy_suspend(priv->spec_phydev);

	if (priv->mii) {
		mdiobus_unregister(priv->mii);
		priv->mii->priv = NULL;
		mdiobus_free(priv->mii);
		priv->mii = NULL;
	}

	return 0;
}

static void geth_rx_refill(struct net_device *ndev)
{
	struct geth_priv *priv = netdev_priv(ndev);
	struct dma_desc *desc;
	struct sk_buff *sk = NULL;
	dma_addr_t paddr;

	while (circ_space(priv->rx_clean, priv->rx_dirty, dma_desc_rx) > 0) {
		int entry = priv->rx_clean;

		/* Find the dirty's desc and clean it */
		desc = priv->dma_rx + entry;

		if (priv->rx_sk[entry] == NULL) {
			sk = netdev_alloc_skb_ip_align(ndev, priv->buf_sz);

			if (unlikely(sk == NULL))
				break;

			priv->rx_sk[entry] = sk;
			paddr = dma_map_single(priv->dev, sk->data,
					       priv->buf_sz, DMA_FROM_DEVICE);
			desc_buf_set(desc, paddr, priv->buf_sz);
		}

		/* sync memery */
		wmb();
		desc_set_own(desc);
		priv->rx_clean = circ_inc(priv->rx_clean, dma_desc_rx);
	}
}

/* geth_dma_desc_init - initialize the RX/TX descriptor list
 * @ndev: net device structure
 * Description: initialize the list for dma.
 */
static int geth_dma_desc_init(struct net_device *ndev)
{
	struct geth_priv *priv = netdev_priv(ndev);
	unsigned int buf_sz;

	priv->rx_sk = kzalloc(sizeof(struct sk_buff *) * dma_desc_rx,
				GFP_KERNEL);
	if (!priv->rx_sk) {
		netdev_err(ndev, "rx_sk alloc is failed\n");
		goto rx_sk_err;
	}
	priv->tx_sk = kzalloc(sizeof(struct sk_buff *) * dma_desc_tx,
				GFP_KERNEL);
	if (!priv->tx_sk) {
		netdev_err(ndev, "tx_sk alloc is failed\n");
		goto tx_sk_err;
	}

	/* Set the size of buffer depend on the MTU & max buf size */
	buf_sz = MAX_BUF_SZ;

	priv->dma_tx = dma_alloc_coherent(priv->dev,
					dma_desc_tx *
					sizeof(struct dma_desc),
					&priv->dma_tx_phy,
					GFP_KERNEL);
	if (!priv->dma_tx) {
		netdev_err(ndev, "dma_tx alloc is failed\n");
		goto dma_tx_err;
	}

	priv->dma_rx = dma_alloc_coherent(priv->dev,
					dma_desc_rx *
					sizeof(struct dma_desc),
					&priv->dma_rx_phy,
					GFP_KERNEL);
	if (!priv->dma_rx) {
		netdev_err(ndev, "dma_rx alloc is failed\n");
		goto dma_rx_err;
	}

	priv->buf_sz = buf_sz;
	return 0;

dma_rx_err:
	dma_free_coherent(priv->dev, dma_desc_rx * sizeof(struct dma_desc),
			  priv->dma_tx, priv->dma_tx_phy);
dma_tx_err:
	kfree(priv->tx_sk);
tx_sk_err:
	kfree(priv->rx_sk);
rx_sk_err:
	return -ENOMEM;
}

static void geth_free_rx_sk(struct geth_priv *priv)
{
	int i;

	for (i = 0; i < dma_desc_rx; i++) {
		if (priv->rx_sk[i] != NULL) {
			struct dma_desc *desc = priv->dma_rx + i;

			dma_unmap_single(priv->dev, (u32)desc_buf_get_addr(desc),
					 desc_buf_get_len(desc),
					 DMA_FROM_DEVICE);
			dev_kfree_skb_any(priv->rx_sk[i]);
			priv->rx_sk[i] = NULL;
		}
	}
}

static void geth_free_tx_sk(struct geth_priv *priv)
{
	int i;

	for (i = 0; i < dma_desc_tx; i++) {
		if (priv->tx_sk[i] != NULL) {
			struct dma_desc *desc = priv->dma_tx + i;

			if (desc_buf_get_addr(desc))
				dma_unmap_single(priv->dev, (u32)desc_buf_get_addr(desc),
						 desc_buf_get_len(desc),
						 DMA_TO_DEVICE);
			dev_kfree_skb_any(priv->tx_sk[i]);
			priv->tx_sk[i] = NULL;
		}
	}
}

static void geth_free_dma_desc(struct geth_priv *priv)
{
	/* Free the region of consistent memory previously allocated for the DMA */
	dma_free_coherent(priv->dev, dma_desc_tx * sizeof(struct dma_desc),
			  priv->dma_tx, priv->dma_tx_phy);
	dma_free_coherent(priv->dev, dma_desc_rx * sizeof(struct dma_desc),
			  priv->dma_rx, priv->dma_rx_phy);

	kfree(priv->rx_sk);
	kfree(priv->tx_sk);
}

#if IS_ENABLED(CONFIG_PM)
static int geth_select_gpio_state(struct pinctrl *pctrl, char *name)
{
	int ret = 0;
	struct pinctrl_state *pctrl_state = NULL;

	pctrl_state = pinctrl_lookup_state(pctrl, name);
	if (IS_ERR(pctrl_state)) {
		pr_err("gmac pinctrl_lookup_state(%s) failed! return %p\n",
						name, pctrl_state);
		return -EINVAL;
	}

	ret = pinctrl_select_state(pctrl, pctrl_state);
	if (ret < 0)
		pr_err("gmac pinctrl_select_state(%s) failed! return %d\n",
						name, ret);

	return ret;
}

static int geth_suspend(struct device *dev)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	struct geth_priv *priv = netdev_priv(ndev);

	cancel_work_sync(&priv->eth_work);

	if (!ndev || !netif_running(ndev))
		return 0;

	priv->is_suspend = true;

	spin_lock(&priv->lock);
	netif_device_detach(ndev);
	spin_unlock(&priv->lock);

	geth_stop(ndev);

	if (priv->phy_rst_type == RST_HARDWARE_TYPE)
		geth_select_gpio_state(priv->pinctrl, PINCTRL_STATE_SLEEP);

	return 0;
}

static void geth_resume_work(struct work_struct *work)
{
	struct geth_priv *priv = container_of(work, struct geth_priv, eth_work);
	struct net_device *ndev = priv->ndev;

	if (!netif_running(ndev))
		return;

	if (priv->phy_rst_type == RST_HARDWARE_TYPE)
		geth_select_gpio_state(priv->pinctrl, PINCTRL_STATE_DEFAULT);

	spin_lock(&priv->lock);
	netif_device_attach(ndev);
	spin_unlock(&priv->lock);

	geth_open(ndev);

	priv->is_suspend = false;

}

static void geth_resume(struct device *dev)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	struct geth_priv *priv = netdev_priv(ndev);
	schedule_work(&priv->eth_work);
}

static int geth_freeze(struct device *dev)
{
	return 0;
}

static int geth_restore(struct device *dev)
{
	return 0;
}

static const struct dev_pm_ops geth_pm_ops = {
	.complete = geth_resume,
	.prepare = geth_suspend,
	.suspend = NULL,
	.resume = NULL,
	.freeze = geth_freeze,
	.restore = geth_restore,
};
#else
static const struct dev_pm_ops geth_pm_ops;
#endif /* CONFIG_PM */

#define sunxi_get_soc_chipid(x) {}
static void geth_chip_hwaddr(u8 *addr)
{
#define MD5_SIZE	16
#define CHIP_SIZE	16

	struct crypto_ahash *tfm;
	struct ahash_request *req;
	struct scatterlist sg;
	u8 result[MD5_SIZE];
	u8 chipid[CHIP_SIZE];
	int i = 0;
	int ret = -1;

	memset(chipid, 0, sizeof(chipid));
	memset(result, 0, sizeof(result));

	sunxi_get_soc_chipid((u8 *)chipid);

	tfm = crypto_alloc_ahash("md5", 0, CRYPTO_ALG_ASYNC);
	if (IS_ERR(tfm)) {
		pr_err("Failed to alloc md5\n");
		return;
	}

	req = ahash_request_alloc(tfm, GFP_KERNEL);
	if (!req)
		goto out;

	ahash_request_set_callback(req, 0, NULL, NULL);

	ret = crypto_ahash_init(req);
	if (ret) {
		pr_err("crypto_ahash_init() failed\n");
		goto out;
	}

	sg_init_one(&sg, chipid, sizeof(chipid));
	ahash_request_set_crypt(req, &sg, result, sizeof(chipid));
	ret = crypto_ahash_update(req);
	if (ret) {
		pr_err("crypto_ahash_update() failed for id\n");
		goto out;
	}

	ret = crypto_ahash_final(req);
	if (ret) {
		pr_err("crypto_ahash_final() failed for result\n");
		goto out;
	}

	ahash_request_free(req);

	/* Choose md5 result's [0][2][4][6][8][10] byte as mac address */
	for (i = 0; i < ETH_ALEN; i++)
		addr[i] = result[2 * i];
	addr[0] &= 0xfe; /* clear multicast bit */
	addr[0] |= 0x02; /* set local assignment bit (IEEE802) */

out:
	crypto_free_ahash(tfm);
}

static void geth_check_addr(struct net_device *ndev, unsigned char *mac)
{
	int i;
	char *p = mac;

	if (!is_valid_ether_addr(ndev->dev_addr)) {
		for (i = 0; i < ETH_ALEN; i++, p++)
			ndev->dev_addr[i] = simple_strtoul(p, &p, 16);

		if (!is_valid_ether_addr(ndev->dev_addr))
			geth_chip_hwaddr(ndev->dev_addr);

		if (!is_valid_ether_addr(ndev->dev_addr)) {
			random_ether_addr(ndev->dev_addr);
			pr_warn("%s: Use random mac address\n", ndev->name);
		}
	}
}

static int geth_clk_enable(struct geth_priv *priv)
{
	int phy_interface, ret;
	u32 clk_value;

	if (clk_prepare_enable(priv->geth_clk))
		pr_err("try to enable geth_clk failed!\n");

	if (priv->phy_clk_type == INT_CLK_TYPE) {
		ret = clk_prepare_enable(priv->ephy_clk);
		if (ret) {
			pr_err("try to enable ephy_clk failed!\n");
			goto ephy_clk_err;
		}
	}

	phy_interface = priv->phy_interface;

	clk_value = readl(priv->base_phy);
	if (phy_interface == PHY_INTERFACE_MODE_RGMII)
		clk_value |= 0x00000004;
	else
		clk_value &= (~0x00000004);

	clk_value &= (~0x00002003);
	if (phy_interface == PHY_INTERFACE_MODE_RGMII
			|| phy_interface == PHY_INTERFACE_MODE_GMII)
		clk_value |= 0x00000002;
	else if (phy_interface == PHY_INTERFACE_MODE_RMII)
		clk_value |= 0x00002000;

	/*
	 * Adjust Tx/Rx clock delay
	 * Tx clock delay: 0~7
	 * Rx clock delay: 0~31
	 */
	clk_value &= ~(0x07 << 10);
	clk_value |= ((priv->tx_delay & 0x07) << 10);
	clk_value &= ~(0x1F << 5);
	clk_value |= ((priv->rx_delay & 0x1F) << 5);

	writel(clk_value, priv->base_phy);

	return 0;

ephy_clk_err:
	clk_disable(priv->geth_clk);

	return ret;
}

static void geth_clk_disable(struct geth_priv *priv)
{
	if (((priv->phy_ext == INT_PHY) || priv->use_ephy_clk)
			&& !IS_ERR_OR_NULL(priv->ephy_clk))
		clk_disable_unprepare(priv->ephy_clk);

	clk_disable_unprepare(priv->geth_clk);
}

static void geth_tx_err(struct geth_priv *priv)
{
	netif_stop_queue(priv->ndev);

	sunxi_stop_tx(priv->base);

	geth_free_tx_sk(priv);
	memset(priv->dma_tx, 0, dma_desc_tx * sizeof(struct dma_desc));
	desc_init_chain(priv->dma_tx, (unsigned long)priv->dma_tx_phy, dma_desc_tx);
	priv->tx_dirty = 0;
	priv->tx_clean = 0;
	sunxi_start_tx(priv->base, priv->dma_tx_phy);

	priv->ndev->stats.tx_errors++;
	netif_wake_queue(priv->ndev);
}

static inline void geth_schedule(struct geth_priv *priv)
{
	if (likely(napi_schedule_prep(&priv->napi))) {
		sunxi_int_disable(priv->base);
		__napi_schedule(&priv->napi);
	}
}

static irqreturn_t geth_interrupt(int irq, void *dev_id)
{
	struct net_device *ndev = (struct net_device *)dev_id;
	struct geth_priv *priv = netdev_priv(ndev);
	int status;

	if (unlikely(!ndev)) {
		pr_err("%s: invalid ndev pointer\n", __func__);
		return IRQ_NONE;
	}

	status = sunxi_int_status(priv->base, (void *)(&priv->xstats));

	if (likely(status == handle_tx_rx))
		geth_schedule(priv);
	else if (unlikely(status == tx_hard_error_bump_tc))
		netdev_info(ndev, "Do nothing for bump tc\n");
	else if (unlikely(status == tx_hard_error))
		geth_tx_err(priv);
	else
		netdev_info(ndev, "Do nothing.....\n");

	return IRQ_HANDLED;
}

static int geth_open(struct net_device *ndev)
{
	struct geth_priv *priv = netdev_priv(ndev);
	int ret = 0;

	ret = geth_power_on(priv);
	if (ret) {
		netdev_err(ndev, "Power on is failed\n");
		ret = -EINVAL;
	}

	ret = geth_clk_enable(priv);
	if (ret) {
		netdev_err(ndev, "Clk enable is failed\n");
		ret = -EINVAL;
	}

	/*
	 * When changing the configuration of GMAC and PHY,
	 * it is necessary to turn off the carrier on the link.
	 */
	netif_carrier_off(ndev);

	/* phy use hw pin to reset */
	geth_phy_hw_reset(ndev);

	ret = geth_phy_init(ndev);
	if (ret) {
		netdev_err(ndev, "phy init again\n");
		ret = geth_phy_init(ndev);
		if (ret)
			goto err;
	}

	ret = sunxi_mac_reset((void *)priv->base, &sunxi_udelay, 10000);
	if (ret) {
		netdev_err(ndev, "Initialize hardware error\n");
		goto desc_err;
	}
	sunxi_mac_init(priv->base, txmode, rxmode);
	sunxi_set_umac(priv->base, ndev->dev_addr, 0);

	if (!priv->is_suspend) {
		ret = geth_dma_desc_init(ndev);
		if (ret) {
			ret = -EINVAL;
			goto desc_err;
		}
	}

	memset(priv->dma_tx, 0, dma_desc_tx * sizeof(struct dma_desc));
	memset(priv->dma_rx, 0, dma_desc_rx * sizeof(struct dma_desc));

	desc_init_chain(priv->dma_rx, (unsigned long)priv->dma_rx_phy, dma_desc_rx);
	desc_init_chain(priv->dma_tx, (unsigned long)priv->dma_tx_phy, dma_desc_tx);

	priv->rx_clean = 0;
	priv->rx_dirty = 0;
	priv->tx_clean = 0;
	priv->tx_dirty = 0;
	geth_rx_refill(ndev);

	/* Extra statistics */
	memset(&priv->xstats, 0, sizeof(struct geth_extra_stats));

	if (ndev->phydev)
		phy_start(ndev->phydev);

	sunxi_start_rx(priv->base, (unsigned long)((struct dma_desc *)
		       priv->dma_rx_phy + priv->rx_dirty));
	sunxi_start_tx(priv->base, (unsigned long)((struct dma_desc *)
		       priv->dma_tx_phy + priv->tx_clean));

	napi_enable(&priv->napi);
	netif_start_queue(ndev);

	/* Enable the Rx/Tx */
	sunxi_mac_enable(priv->base);

	return 0;

desc_err:
	geth_phy_release(ndev);
err:
	geth_clk_disable(priv);
	if (priv->is_suspend)
		napi_enable(&priv->napi);

	geth_power_off(priv);

	return ret;
}

static int geth_stop(struct net_device *ndev)
{
	struct geth_priv *priv = netdev_priv(ndev);

	netif_stop_queue(ndev);
	napi_disable(&priv->napi);

	netif_carrier_off(ndev);

	/* Release PHY resources */
	//if (priv->is_suspend)
	//	geth_suspend_phy_release(ndev);
	//else
	geth_phy_release(ndev);

	/* Disable Rx/Tx */
	sunxi_mac_disable(priv->base);

	geth_clk_disable(priv);
	geth_power_off(priv);

	netif_tx_lock_bh(ndev);
	/* Release the DMA TX/RX socket buffers */
	geth_free_rx_sk(priv);
	geth_free_tx_sk(priv);
	netif_tx_unlock_bh(ndev);

	return 0;
}

static void geth_tx_complete(struct geth_priv *priv)
{
	unsigned int entry = 0;
	struct sk_buff *skb = NULL;
	struct dma_desc *desc = NULL;
	int tx_stat;

	spin_lock(&priv->tx_lock);
	while (circ_cnt(priv->tx_dirty, priv->tx_clean, dma_desc_tx) > 0) {
		entry = priv->tx_clean;
		desc = priv->dma_tx + entry;

		/* Check if the descriptor is owned by the DMA. */
		if (desc_get_own(desc))
			break;

		/* Verify tx error by looking at the last segment */
		if (desc_get_tx_ls(desc)) {
			tx_stat = desc_get_tx_status(desc, (void *)(&priv->xstats));

			if (likely(!tx_stat))
				priv->ndev->stats.tx_packets++;
			else
				priv->ndev->stats.tx_errors++;
		}

		dma_unmap_single(priv->dev, (u32)desc_buf_get_addr(desc),
				 desc_buf_get_len(desc), DMA_TO_DEVICE);

		skb = priv->tx_sk[entry];
		priv->tx_sk[entry] = NULL;
		desc_init(desc);

		/* Find next dirty desc */
		priv->tx_clean = circ_inc(entry, dma_desc_tx);

		if (unlikely(skb == NULL))
			continue;

		dev_kfree_skb(skb);
	}

	if (unlikely(netif_queue_stopped(priv->ndev)) &&
	    circ_space(priv->tx_dirty, priv->tx_clean, dma_desc_tx) >
	    TX_THRESH) {
		netif_wake_queue(priv->ndev);
	}
	spin_unlock(&priv->tx_lock);
}

static netdev_tx_t geth_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	struct geth_priv  *priv = netdev_priv(ndev);
	unsigned int entry;
	struct dma_desc *desc, *first;
	unsigned int len, tmp_len = 0;
	int i, csum_insert;
	int nfrags = skb_shinfo(skb)->nr_frags;
	dma_addr_t paddr;

	spin_lock(&priv->tx_lock);
	if (unlikely(circ_space(priv->tx_dirty, priv->tx_clean,
	    dma_desc_tx) < (nfrags + 1))) {
		if (!netif_queue_stopped(ndev)) {
			netdev_err(ndev, "%s: BUG! Tx Ring full when queue awake\n", __func__);
			netif_stop_queue(ndev);
		}
		spin_unlock(&priv->tx_lock);

		return NETDEV_TX_BUSY;
	}

	csum_insert = (skb->ip_summed == CHECKSUM_PARTIAL);
	entry = priv->tx_dirty;
	first = priv->dma_tx + entry;
	desc = priv->dma_tx + entry;

	len = skb_headlen(skb);
	priv->tx_sk[entry] = skb;

#ifdef PKT_DEBUG
	printk("======TX PKT DATA: ============\n");
	/* dump the packet */
	print_hex_dump(KERN_DEBUG, "skb->data: ", DUMP_PREFIX_NONE,
		       16, 1, skb->data, 64, true);
#endif

	/* Every desc max size is 2K */
	while (len != 0) {
		desc = priv->dma_tx + entry;
		tmp_len = ((len > MAX_BUF_SZ) ?  MAX_BUF_SZ : len);

		paddr = dma_map_single(priv->dev, skb->data, tmp_len, DMA_TO_DEVICE);
		if (dma_mapping_error(priv->dev, paddr)) {
			dev_kfree_skb(skb);
			return -EIO;
		}
		desc_buf_set(desc, paddr, tmp_len);
		/* Don't set the first's own bit, here */
		if (first != desc) {
			priv->tx_sk[entry] = NULL;
			desc_set_own(desc);
		}

		entry = circ_inc(entry, dma_desc_tx);
		len -= tmp_len;
	}

	for (i = 0; i < nfrags; i++) {
		const skb_frag_t *frag = &skb_shinfo(skb)->frags[i];

		len = skb_frag_size(frag);
		desc = priv->dma_tx + entry;
		paddr = skb_frag_dma_map(priv->dev, frag, 0, len, DMA_TO_DEVICE);
		if (dma_mapping_error(priv->dev, paddr)) {
			dev_kfree_skb(skb);
			return -EIO;
		}

		desc_buf_set(desc, paddr, len);
		desc_set_own(desc);
		priv->tx_sk[entry] = NULL;
		entry = circ_inc(entry, dma_desc_tx);
	}

	ndev->stats.tx_bytes += skb->len;
	priv->tx_dirty = entry;
	desc_tx_close(first, desc, csum_insert);

	desc_set_own(first);
	spin_unlock(&priv->tx_lock);

	if (circ_space(priv->tx_dirty, priv->tx_clean, dma_desc_tx) <=
			(MAX_SKB_FRAGS + 1)) {
		netif_stop_queue(ndev);
		if (circ_space(priv->tx_dirty, priv->tx_clean, dma_desc_tx) >
				TX_THRESH)
			netif_wake_queue(ndev);
	}

#ifdef DEBUG
	printk("=======TX Descriptor DMA: 0x%08llx\n", priv->dma_tx_phy);
	printk("Tx pointor: dirty: %d, clean: %d\n", priv->tx_dirty, priv->tx_clean);
	desc_print(priv->dma_tx, dma_desc_tx);
#endif
	sunxi_tx_poll(priv->base);
	geth_tx_complete(priv);

	return NETDEV_TX_OK;
}

static int geth_rx(struct geth_priv *priv, int limit)
{
	unsigned int rxcount = 0;
	unsigned int entry;
	struct dma_desc *desc;
	struct sk_buff *skb;
	int status;
	int frame_len;

	while (rxcount < limit) {
		entry = priv->rx_dirty;
		desc = priv->dma_rx + entry;

		if (desc_get_own(desc))
			break;

		rxcount++;
		priv->rx_dirty = circ_inc(priv->rx_dirty, dma_desc_rx);

		/* Get length & status from hardware */
		frame_len = desc_rx_frame_len(desc);
		status = desc_get_rx_status(desc, (void *)(&priv->xstats));

		netdev_dbg(priv->ndev, "Rx frame size %d, status: %d\n",
			   frame_len, status);

		skb = priv->rx_sk[entry];
		if (unlikely(!skb)) {
			netdev_err(priv->ndev, "Skb is null\n");
			priv->ndev->stats.rx_dropped++;
			break;
		}

#ifdef PKT_DEBUG
		printk("======RX PKT DATA: ============\n");
		/* dump the packet */
		print_hex_dump(KERN_DEBUG, "skb->data: ", DUMP_PREFIX_NONE,
				16, 1, skb->data, 64, true);
#endif

		if (status == discard_frame) {
			netdev_dbg(priv->ndev, "Get error pkt\n");
			priv->ndev->stats.rx_errors++;
			continue;
		}

		if (unlikely(status != llc_snap))
			frame_len -= ETH_FCS_LEN;

		priv->rx_sk[entry] = NULL;

		skb_put(skb, frame_len);
		dma_unmap_single(priv->dev, (u32)desc_buf_get_addr(desc),
				 desc_buf_get_len(desc), DMA_FROM_DEVICE);

		skb->protocol = eth_type_trans(skb, priv->ndev);

		skb->ip_summed = CHECKSUM_UNNECESSARY;
		napi_gro_receive(&priv->napi, skb);

		priv->ndev->stats.rx_packets++;
		priv->ndev->stats.rx_bytes += frame_len;
	}

#ifdef DEBUG
	if (rxcount > 0) {
		printk("======RX Descriptor DMA: 0x%08llx=\n", priv->dma_rx_phy);
		printk("RX pointor: dirty: %d, clean: %d\n", priv->rx_dirty, priv->rx_clean);
		desc_print(priv->dma_rx, dma_desc_rx);
	}
#endif
	geth_rx_refill(priv->ndev);

	return rxcount;
}

static int geth_poll(struct napi_struct *napi, int budget)
{
	struct geth_priv *priv = container_of(napi, struct geth_priv, napi);
	int work_done = 0;

	geth_tx_complete(priv);
	work_done = geth_rx(priv, budget);

	if (work_done < budget) {
		napi_complete(napi);
		sunxi_int_enable(priv->base);
	}

	return work_done;
}

static int geth_change_mtu(struct net_device *ndev, int new_mtu)
{
	int max_mtu;

	if (netif_running(ndev)) {
		pr_err("%s: must be stopped to change its MTU\n", ndev->name);
		return -EBUSY;
	}

	max_mtu = SKB_MAX_HEAD(NET_SKB_PAD + NET_IP_ALIGN);

	if ((new_mtu < 46) || (new_mtu > max_mtu)) {
		pr_err("%s: invalid MTU, max MTU is: %d\n", ndev->name, max_mtu);
		return -EINVAL;
	}

	ndev->mtu = new_mtu;
	netdev_update_features(ndev);

	return 0;
}

static netdev_features_t geth_fix_features(struct net_device *ndev,
					   netdev_features_t features)
{
	return features;
}

static void geth_set_rx_mode(struct net_device *ndev)
{
	struct geth_priv *priv = netdev_priv(ndev);
	unsigned int value = 0;

	pr_debug("%s: # mcasts %d, # unicast %d\n",
		 __func__, netdev_mc_count(ndev), netdev_uc_count(ndev));

	spin_lock(&priv->lock);
	if (ndev->flags & IFF_PROMISC) {
		value = GETH_FRAME_FILTER_PR;
	} else if ((netdev_mc_count(ndev) > HASH_TABLE_SIZE) ||
		   (ndev->flags & IFF_ALLMULTI)) {
		value = GETH_FRAME_FILTER_PM;	/* pass all multi */
		sunxi_hash_filter(priv->base, ~0UL, ~0UL);
	} else if (!netdev_mc_empty(ndev)) {
		u32 mc_filter[2];
		struct netdev_hw_addr *ha;

		/* Hash filter for multicast */
		value = GETH_FRAME_FILTER_HMC;

		memset(mc_filter, 0, sizeof(mc_filter));
		netdev_for_each_mc_addr(ha, ndev) {
			/* The upper 6 bits of the calculated CRC are used to
			 *  index the contens of the hash table
			 */
			int bit_nr = bitrev32(~crc32_le(~0, ha->addr, 6)) >> 26;
			/* The most significant bit determines the register to
			 * use (H/L) while the other 5 bits determine the bit
			 * within the register.
			 */
			mc_filter[bit_nr >> 5] |= 1 << (bit_nr & 31);
		}
		sunxi_hash_filter(priv->base, mc_filter[0], mc_filter[1]);
	}

	/* Handle multiple unicast addresses (perfect filtering)*/
	if (netdev_uc_count(ndev) > 16) {
		/* Switch to promiscuous mode is more than 8 addrs are required */
		value |= GETH_FRAME_FILTER_PR;
	} else {
		int reg = 1;
		struct netdev_hw_addr *ha;

		netdev_for_each_uc_addr(ha, ndev) {
			sunxi_set_umac(priv->base, ha->addr, reg);
			reg++;
		}
	}

#ifdef FRAME_FILTER_DEBUG
	/* Enable Receive all mode (to debug filtering_fail errors) */
	value |= GETH_FRAME_FILTER_RA;
#endif
	sunxi_set_filter(priv->base, value);
	spin_unlock(&priv->lock);
}

static void geth_tx_timeout(struct net_device *ndev)
{
	struct geth_priv *priv = netdev_priv(ndev);

	geth_tx_err(priv);
}

static int geth_ioctl(struct net_device *ndev, struct ifreq *rq, int cmd)
{
	if (!netif_running(ndev))
		return -EINVAL;

	if (!ndev->phydev)
		return -EINVAL;

	return phy_mii_ioctl(ndev->phydev, rq, cmd);
}

/* Configuration changes (passed on by ifconfig) */
static int geth_config(struct net_device *ndev, struct ifmap *map)
{
	if (ndev->flags & IFF_UP)	/* can't act on a running interface */
		return -EBUSY;

	/* Don't allow changing the I/O address */
	if (map->base_addr != ndev->base_addr) {
		pr_warn("%s: can't change I/O address\n", ndev->name);
		return -EOPNOTSUPP;
	}

	/* Don't allow changing the IRQ */
	if (map->irq != ndev->irq) {
		pr_warn("%s: can't change IRQ number %d\n", ndev->name, ndev->irq);
		return -EOPNOTSUPP;
	}

	return 0;
}

static int geth_set_mac_address(struct net_device *ndev, void *p)
{
	struct geth_priv *priv = netdev_priv(ndev);
	struct sockaddr *addr = p;

	if (!is_valid_ether_addr(addr->sa_data))
		return -EADDRNOTAVAIL;

	memcpy(ndev->dev_addr, addr->sa_data, ndev->addr_len);
	sunxi_set_umac(priv->base, ndev->dev_addr, 0);

	return 0;
}

int geth_set_features(struct net_device *ndev, netdev_features_t features)
{
	struct geth_priv *priv = netdev_priv(ndev);

	if (features & NETIF_F_LOOPBACK && netif_running(ndev))
		sunxi_mac_loopback(priv->base, 1);
	else
		sunxi_mac_loopback(priv->base, 0);

	return 0;
}

#if IS_ENABLED(CONFIG_NET_POLL_CONTROLLER)
/* Polling receive - used by NETCONSOLE and other diagnostic tools
 * to allow network I/O with interrupts disabled.
 */
static void geth_poll_controller(struct net_device *dev)
{
	disable_irq(dev->irq);
	geth_interrupt(dev->irq, dev);
	enable_irq(dev->irq);
}
#endif

static const struct net_device_ops geth_netdev_ops = {
	.ndo_init = NULL,
	.ndo_open = geth_open,
	.ndo_start_xmit = geth_xmit,
	.ndo_stop = geth_stop,
	.ndo_change_mtu = geth_change_mtu,
	.ndo_fix_features = geth_fix_features,
	.ndo_set_rx_mode = geth_set_rx_mode,
	.ndo_tx_timeout = geth_tx_timeout,
	.ndo_do_ioctl = geth_ioctl,
	.ndo_set_config = geth_config,
#if IS_ENABLED(CONFIG_NET_POLL_CONTROLLER)
	.ndo_poll_controller = geth_poll_controller,
#endif
	.ndo_set_mac_address = geth_set_mac_address,
	.ndo_set_features = geth_set_features,
};

static int geth_check_if_running(struct net_device *ndev)
{
	if (!netif_running(ndev))
		return -EBUSY;
	return 0;
}

static int geth_get_sset_count(struct net_device *netdev, int sset)
{
	int len;

	switch (sset) {
	case ETH_SS_STATS:
		len = 0;
		return len;
	default:
		return -EOPNOTSUPP;
	}
}

static int geth_ethtool_getsettings(struct net_device *ndev,
				    struct ethtool_cmd *cmd)
{
	struct geth_priv *priv = netdev_priv(ndev);
	struct phy_device *phy = ndev->phydev;
	int rc;

	if (phy == NULL) {
		netdev_err(ndev, "%s: %s: PHY is not registered\n",
		       __func__, ndev->name);
		return -ENODEV;
	}

	if (!netif_running(ndev)) {
		pr_err("%s: interface is disabled: we cannot track "
		       "link speed / duplex setting\n", ndev->name);
		return -EBUSY;
	}

	cmd->transceiver = XCVR_INTERNAL;
	spin_lock_irq(&priv->lock);
	rc = phy_ethtool_gset(phy, cmd);
	spin_unlock_irq(&priv->lock);

	return rc;
}

static int geth_ethtool_setsettings(struct net_device *ndev,
				    struct ethtool_cmd *cmd)
{
	struct geth_priv *priv = netdev_priv(ndev);
	struct phy_device *phy = ndev->phydev;
	int rc;

	spin_lock(&priv->lock);
	rc = phy_ethtool_sset(phy, cmd);
	spin_unlock(&priv->lock);

	return rc;
}

static void geth_ethtool_getdrvinfo(struct net_device *ndev,
				    struct ethtool_drvinfo *info)
{
	strlcpy(info->driver, "sunxi_geth", sizeof(info->driver));

#define DRV_MODULE_VERSION "SUNXI Gbgit driver V1.1"

	strcpy(info->version, DRV_MODULE_VERSION);
	info->fw_version[0] = '\0';
}

/**
 * geth_ethool_get_pauseparam - Get the pause parameter setting for Tx/Rx.
 *
 * @ndev:	Pointer to net_device structure
 * @epause:	Pointer to ethtool_pauseparam structure
 *
 * This implements ethtool command for getting geth ethernet pause frame
 * setting. Issue "ethtool -a ethx" to execute this function.
 */
static void geth_ethtool_get_pauseparam(struct net_device *ndev,
					struct ethtool_pauseparam *epause)
{
	struct geth_priv *priv = netdev_priv(ndev);

	epause->autoneg = 0;
	epause->tx_pause = sunxi_read_tx_flowctl(priv->base);
	epause->rx_pause = sunxi_read_rx_flowctl(priv->base);
}

/**
 * geth_ethtool_set_pauseparam - Set device pause paramter(flow contrl)
 *				settings.
 * @ndev:	Pointer to net_device structure
 * @epause:	Pointer to ethtool_pauseparam structure
 *
 * This implements ethtool command for enabling flow control on Rx and Tx.
 * Issue "ethtool -A ethx tx on|off" under linux prompt to execute this
 * function.
 *
 */
static int geth_ethtool_set_pauseparam(struct net_device *ndev,
					struct ethtool_pauseparam *epause)
{
	struct geth_priv *priv = netdev_priv(ndev);

	if (epause->tx_pause) {
		sunxi_write_tx_flowctl(priv->base, 1);
		netdev_info(ndev, "Tx flow control on\n");
	} else {
		sunxi_write_tx_flowctl(priv->base, 0);
		netdev_info(ndev, "Tx flow control off\n");
	}

	if (epause->rx_pause) {
		sunxi_write_rx_flowctl(priv->base, 1);
		netdev_info(ndev, "Rx flow control on\n");
	} else {
		sunxi_write_rx_flowctl(priv->base, 0);
		netdev_info(ndev, "Rx flow control off\n");
	}

	return 0;
}

/**
 * geth_ethtool_get_wol - Get device wake-on-lan settings.
 *
 * @ndev:	Pointer to net_device structure
 * @wol:	Pointer to ethtool_wolinfo structure
 *
 * This implements ethtool command for get wake-on-lan settings.
 * Issue "ethtool -s ethx wol p|u|m|b|a|g|s|d" under linux prompt to execute
 * this function.
 */
static void geth_ethtool_get_wol(struct net_device *ndev,
				struct ethtool_wolinfo *wol)
{
	struct geth_priv *priv = netdev_priv(ndev);

	spin_lock_irq(&priv->lock);
	wol->supported = WAKE_MAGIC;
	wol->wolopts = priv->wolopts;
	spin_unlock_irq(&priv->lock);

	netdev_err(ndev, "wake-on-lane func is not supported yet\n");
}

/**
 * geth_ethtool_set_wol - set device wake-on-lan settings.
 *
 * @ndev:	Pointer to net_device structure
 * @wol:	Pointer to ethtool_wolinfo structure
 *
 * This implements ethtool command for set wake-on-lan settings.
 * Issue "ethtool -s ethx wol p|u|n|b|a|g|s|d" under linux prompt to execute
 * this function.
 */
static int geth_ethtool_set_wol(struct net_device *ndev,
				struct ethtool_wolinfo *wol)
{
	/*
	 * TODO: Wake-on-lane function need to be supported.
	 */

	return 0;
}

static const struct ethtool_ops geth_ethtool_ops = {
	.begin = geth_check_if_running,
	.get_settings = geth_ethtool_getsettings,
	.set_settings = geth_ethtool_setsettings,
	.get_link = ethtool_op_get_link,
	.get_pauseparam = geth_ethtool_get_pauseparam,
	.set_pauseparam = geth_ethtool_set_pauseparam,
	.get_wol = geth_ethtool_get_wol,
	.set_wol = geth_ethtool_set_wol,
	.get_sset_count = geth_get_sset_count,
	.get_drvinfo = geth_ethtool_getdrvinfo,
};

/* config hardware resource */
static int geth_hw_init(struct platform_device *pdev)
{
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct geth_priv *priv = netdev_priv(ndev);
	struct device_node *np = pdev->dev.of_node;
	struct property *prop = NULL;
	int ret = 0;
	struct resource *res;
	struct gpio_config cfg;
	const char *gmac_power;
	char power[20];
	int i;
	struct device_node *phy_node;

	/* config memery resource */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		netdev_err(ndev, "get gmac memory failed\n");
		return -ENODEV;
	}

	priv->base = devm_ioremap_resource(&pdev->dev, res);
	if (!priv->base) {
		netdev_err(ndev, "gmac memory mapping failed\n");
		return -ENOMEM;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!res) {
		netdev_err(ndev, "get phy memory failed\n");
		ret = -ENODEV;
		goto mem_err;
	}

	priv->base_phy = devm_ioremap_resource(&pdev->dev, res);
	if (!priv->base_phy) {
		netdev_err(ndev, "phy memory mapping failed\n");
		ret = -ENOMEM;
		goto mem_err;
	}

	/* config IRQ */
	ndev->irq = platform_get_irq_byname(pdev, "gmacirq");
	if (ndev->irq < 0) {
		netdev_err(ndev, "GMAC IRQ not found\n");
		ret = -ENXIO;
		goto irq_err;
	}

	ret = request_irq(ndev->irq, geth_interrupt, IRQF_SHARED, dev_name(&pdev->dev), ndev);
	if (ret < 0) {
		netdev_err(ndev, "Could not request irq %d, error: %d\n", ndev->irq, ret);
		ret = -EINVAL;
		goto irq_err;
	}

	/* gmac config clock */
	priv->geth_clk = of_clk_get_by_name(np, "gmac");
	if (IS_ERR_OR_NULL(priv->geth_clk)) {
		netdev_err(ndev, "Get gmac clock failed!\n");
		ret = -EINVAL;
		goto get_para_err;
	}

	/* phy interface */
	priv->phy_interface = of_get_phy_mode(np);
	if (priv->phy_interface != PHY_INTERFACE_MODE_MII &&
	    priv->phy_interface != PHY_INTERFACE_MODE_RGMII &&
	    priv->phy_interface != PHY_INTERFACE_MODE_RMII) {

		netdev_err(ndev, "Get gmac phy interface err!\n");
		priv->phy_interface = PHY_INTERFACE_MODE_MII;
		ret = -EINVAL;
		goto get_para_err;
	}

	/* get tx delay */
	ret = of_property_read_u32(np, "tx-delay", &priv->tx_delay);
	if (ret != 0) {
		netdev_err(ndev, "Get gmac tx-delay err\n");
		priv->tx_delay = 0;
		ret = -EINVAL;
		goto get_para_err;
	}

	/* get rx delay */
	ret = of_property_read_u32(np, "rx-delay", &priv->rx_delay);
	if (ret != 0) {
		netdev_err(ndev, "Get gmac rx-delay err\n");
		priv->rx_delay = 0;
		ret = -EINVAL;
		goto get_para_err;
	}

	phy_node = of_get_next_child(np, NULL);
	if (phy_node) {
		/* get phy type */
		ret = of_property_read_u32(phy_node, "phy-type", &priv->phy_ext);
		if (ret != 0) {
			netdev_err(ndev, "Get phy-type failed\n");
			priv->phy_ext = EXT_PHY;
			ret = -EINVAL;
			goto get_para_err;
		}

		/* get phy clk type */
		ret = of_property_read_u32(phy_node, "phy-clk-type", &priv->phy_clk_type);
		if (ret != 0) {
			netdev_err(ndev, "Get phy-clk-type failed\n");
			priv->phy_clk_type = EXT_CLK_TYPE;
			ret = -EINVAL;
			goto get_para_err;
		}

		/* get phy rst type */
		ret = of_property_read_u32(phy_node, "phy-rst-type", &priv->phy_rst_type);
		if (ret != 0) {
			netdev_err(ndev, "Get phy-rst-type failed\n");
			priv->phy_rst_type = RST_HARDWARE_TYPE;
			ret = -EINVAL;
			goto get_para_err;
		}

		/* get power type */
		ret = of_property_read_u32(phy_node, "power-type", &priv->supply_type);
		if (ret != 0) {
			netdev_err(ndev, "Get power-type failed\n");
			priv->supply_type = SUPPLY_GENERAL_POWER;
			ret = -EINVAL;
			goto get_para_err;
		}

		/* get phy name */
		prop = of_find_property(phy_node, "phy-name", NULL);
		if (IS_ERR_OR_NULL(prop)) {
			netdev_err(ndev, "Get sunxi-phy-name failed\n");
			ret = -EINVAL;
			goto get_para_err;
		}
		priv->phy_name = prop->value;

		/*FIX: if we use ac200/ac300, need to get pwm channel as phy clk */
		if ((strcmp(priv->phy_name, AC300_EPHY) == 0) ||
			(strcmp(priv->phy_name, AC200_EPHY) == 0)) {
			ret = of_property_read_u32(phy_node, "pwm-channel", &priv->pwm_channel);
			if (ret) {
				netdev_err(ndev, "Get ac200/ac300 pwm failed\n");
				ret = -EINVAL;
				goto get_para_err;
			}

			priv->pwm = pwm_request(priv->pwm_channel, NULL);
			if (IS_ERR_OR_NULL(priv->pwm)) {
				netdev_err(ndev, "Get pwm failed\n");
				ret = -EINVAL;
				goto get_para_err;
			}

			ret = pwm_config(priv->pwm, PWM_DUTY_NS, PWM_PERIOD_NS);
			if (ret) {
				netdev_err(ndev, "Config pwm failed\n");
				ret = -EINVAL;
				goto get_para_err;
			}

			ret = pwm_enable(priv->pwm);
			if (ret) {
				netdev_err(ndev, "Enable pwm failed\n");
				ret = -EINVAL;
				goto get_para_err;
			}
		}
	}

	if (priv->phy_clk_type == INT_CLK_TYPE) {
		priv->ephy_clk = of_clk_get_by_name(np, "ephy");
		if (IS_ERR_OR_NULL(priv->ephy_clk)) {
			netdev_err(ndev, "Get ephy clk failed\n");
			ret = -EINVAL;
			goto get_para_err;
		}
	}

	/* get reset hw pin */
	if (priv->phy_rst_type == RST_HARDWARE_TYPE) {
		priv->phyrst = of_get_named_gpio_flags(np,
							"phy-rst", 0, (enum of_gpio_flags *)&cfg);
		if (gpio_is_valid(priv->phyrst)) {
			if (gpio_request(priv->phyrst, "phy-rst") < 0) {
				netdev_err(ndev, "gmac reset gpio request fail\n");
				ret = -EINVAL;
				goto phyrst_err;
			}
		}

		priv->rst_active_low = (cfg.data == OF_GPIO_ACTIVE_LOW) ? 1 : 0;
		priv->pinctrl = devm_pinctrl_get_select_default(&pdev->dev);
		if (IS_ERR_OR_NULL(priv->pinctrl)) {
			netdev_err(ndev, "gmac pinctrl error\n");
			ret = -EINVAL;
			goto phyrst_ctrl_err;
		}
	}

	/* config power regulator */
	if (priv->supply_type == SUPPLY_INDEPENDENT_POWER) {
		for (i = 0; i < POWER_CHAN_NUM; i++) {
			snprintf(power, 15, "gmac-power%d", i);
			ret = of_property_read_string(np, power, &gmac_power);
			if (ret) {
				priv->gmac_power[i] = NULL;
				netdev_info(ndev, "gmac-power%d: NULL\n", i);
				continue;
			}
			priv->gmac_power[i] = regulator_get(NULL, gmac_power);
			if (IS_ERR_OR_NULL(priv->gmac_power[i])) {
				netdev_err(ndev, "gmac-power%d get error!\n", i);
				ret = -EINVAL;
				goto supply_type_err;
			}
		}
	}

	return 0;

supply_type_err:
	if (priv->phy_rst_type == RST_HARDWARE_TYPE) {
		if (!IS_ERR_OR_NULL(priv->pinctrl))
			devm_pinctrl_put(priv->pinctrl);
	}
	if (priv->supply_type == SUPPLY_INDEPENDENT_POWER) {
		for (i = 0; i < POWER_CHAN_NUM; i++) {
			if (IS_ERR_OR_NULL(priv->gmac_power[i]))
				continue;
			regulator_put(priv->gmac_power[i]);
		}
	}
phyrst_ctrl_err:
	if (priv->phy_rst_type == RST_HARDWARE_TYPE) {
		if (gpio_is_valid(priv->phyrst))
			gpio_free(priv->phyrst);
	}
phyrst_err:
	if (priv->phy_clk_type == INT_CLK_TYPE) {
		if (!IS_ERR_OR_NULL(priv->pwm))
			pwm_put(priv->pwm);
	}
get_para_err:
	free_irq(ndev->irq, ndev);
irq_err:
	devm_iounmap(&pdev->dev, priv->base_phy);
mem_err:
	devm_iounmap(&pdev->dev, priv->base);

	return ret;
}

static void geth_hw_release(struct platform_device *pdev)
{
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct geth_priv *priv = netdev_priv(ndev);
	int i;

	devm_iounmap(&pdev->dev, (priv->base_phy));
	devm_iounmap(&pdev->dev, priv->base);
	free_irq(ndev->irq, ndev);
	if (priv->geth_clk)
		clk_put(priv->geth_clk);

	if (priv->supply_type == SUPPLY_INDEPENDENT_POWER) {
		for (i = 0; i < POWER_CHAN_NUM; i++) {
			if (IS_ERR_OR_NULL(priv->gmac_power[i]))
				continue;
			regulator_put(priv->gmac_power[i]);
		}
	}

	if (priv->phy_rst_type == RST_HARDWARE_TYPE) {
		if (!IS_ERR_OR_NULL(priv->pinctrl))
			devm_pinctrl_put(priv->pinctrl);

		if (gpio_is_valid(priv->phyrst))
			gpio_free(priv->phyrst);
	}

	if (priv->phy_clk_type == INT_CLK_TYPE) {
		if (!IS_ERR_OR_NULL(priv->ephy_clk))
			clk_put(priv->ephy_clk);
	}

	if ((strcmp(priv->phy_name, AC300_EPHY) == 0) ||
		(strcmp(priv->phy_name, AC200_EPHY) == 0)) {
		if (!IS_ERR_OR_NULL(priv->pwm))
			pwm_put(priv->pwm);
	}
}

/**
 * geth_probe
 * @pdev: platform device pointer
 * Description: the driver is initialized through platform_device.
 */
static int geth_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct net_device *ndev = NULL;
	struct geth_priv *priv;

	pr_info("sunxi gmac driver's version: %s\n", SUNXI_GMAC_VERSION);

#if IS_ENABLED(CONFIG_OF)
	pdev->dev.dma_mask = &geth_dma_mask;
	pdev->dev.coherent_dma_mask = DMA_BIT_MASK(32);
#endif

	ndev = alloc_etherdev(sizeof(struct geth_priv));
	if (!ndev) {
		dev_err(&pdev->dev, "could not allocate device.\n");
		return -ENOMEM;
	}
	SET_NETDEV_DEV(ndev, &pdev->dev);

	priv = netdev_priv(ndev);
	platform_set_drvdata(pdev, ndev);

	/* Must set private data to pdev, before call it */
	ret = geth_hw_init(pdev);
	if (0 != ret) {
		pr_err("geth_hw_init fail!\n");
		goto hw_err;
	}

	/* setup the netdevice, fill the field of netdevice */
	ether_setup(ndev);
	ndev->netdev_ops = &geth_netdev_ops;
	netdev_set_default_ethtool_ops(ndev, &geth_ethtool_ops);
	ndev->base_addr = (unsigned long)priv->base;

	priv->ndev = ndev;
	priv->dev = &pdev->dev;

	/* TODO: support the VLAN frames */
	ndev->hw_features = NETIF_F_SG | NETIF_F_HIGHDMA | NETIF_F_IP_CSUM |
				NETIF_F_IPV6_CSUM | NETIF_F_RXCSUM;

	ndev->features |= ndev->hw_features;
	ndev->hw_features |= NETIF_F_LOOPBACK;
	ndev->priv_flags |= IFF_UNICAST_FLT;

	ndev->watchdog_timeo = msecs_to_jiffies(watchdog);

	netif_napi_add(ndev, &priv->napi, geth_poll,  BUDGET);

	spin_lock_init(&priv->lock);
	spin_lock_init(&priv->tx_lock);

	/* The last val is mdc clock ratio */
	sunxi_geth_register((void *)ndev->base_addr, HW_VERSION, 0x03);

	ret = register_netdev(ndev);
	if (ret) {
		netif_napi_del(&priv->napi);
		pr_err("Error: Register %s failed\n", ndev->name);
		goto reg_err;
	}

	/* Before open the device, the mac address should be set */
	geth_check_addr(ndev, mac_str);

	/* malloc dma desc */
	geth_dma_desc_init(ndev);

#ifdef CONFIG_GETH_ATTRS
	geth_create_attrs(ndev);
#endif
	device_create_file(&pdev->dev, &dev_attr_gphy_test);
	device_create_file(&pdev->dev, &dev_attr_mii_read);
	device_create_file(&pdev->dev, &dev_attr_mii_write);
	device_create_file(&pdev->dev, &dev_attr_loopback_test);
	device_create_file(&pdev->dev, &dev_attr_extra_tx_stats);
	device_create_file(&pdev->dev, &dev_attr_extra_rx_stats);

	device_enable_async_suspend(&pdev->dev);

#if IS_ENABLED(CONFIG_PM)
	INIT_WORK(&priv->eth_work, geth_resume_work);
#endif

	return 0;

reg_err:
	geth_hw_release(pdev);
hw_err:
	platform_set_drvdata(pdev, NULL);
	free_netdev(ndev);

	return ret;
}

static int geth_remove(struct platform_device *pdev)
{
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct geth_priv *priv = netdev_priv(ndev);

	device_remove_file(&pdev->dev, &dev_attr_gphy_test);
	device_remove_file(&pdev->dev, &dev_attr_mii_read);
	device_remove_file(&pdev->dev, &dev_attr_mii_write);
	device_remove_file(&pdev->dev, &dev_attr_loopback_test);
	device_remove_file(&pdev->dev, &dev_attr_extra_tx_stats);
	device_remove_file(&pdev->dev, &dev_attr_extra_rx_stats);
	netif_napi_del(&priv->napi);
	unregister_netdev(ndev);
	geth_free_dma_desc(priv);
	geth_hw_release(pdev);
	platform_set_drvdata(pdev, NULL);
	free_netdev(ndev);
	return 0;
}

static const struct of_device_id geth_of_match[] = {
	{.compatible = "allwinner,sunxi-gmac",},
	{},
};
MODULE_DEVICE_TABLE(of, geth_of_match);

static struct platform_driver geth_driver = {
	.probe	= geth_probe,
	.remove = geth_remove,
	.driver = {
		   .name = "sunxi-gmac",
		   .owner = THIS_MODULE,
		   .pm = &geth_pm_ops,
		   .of_match_table = geth_of_match,
	},
};
module_platform_driver(geth_driver);

static int __init set_mac_addr(char *str)
{
	char *p = str;

	if (str && strlen(str))
		memcpy(mac_str, p, 18);

	return 0;
}
__setup("mac_addr=", set_mac_addr);

MODULE_DESCRIPTION("Allwinner Gigabit Ethernet driver");
MODULE_AUTHOR("fuzhaoke <fuzhaoke@allwinnertech.com>");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_VERSION("2.0.2");
