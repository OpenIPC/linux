#include <linux/mii.h>
#include <linux/phy.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include "fh_gmac_phyt.h"
#include <mach/pmu.h>
#include <mach/pinctrl.h>

static int fh_gmac_ephy_reset(Gmac_Object *pGmac, __u32 phy_sel);
static int fh_mdio_set_mii(struct mii_bus *bus);

char *phy_driver_array[] = {
	"Generic PHY",
#if defined(CONFIG_ARCH_FH885xV200) || \
defined(CONFIG_ARCH_FH865x)
	"FH EPHY",
#endif
	0,
};
#ifndef PHY_MAX_ADDR
#define PHY_MAX_ADDR	32
#endif

struct mdio_pin_mux_ref {
	char *old;
	char *restore;
};

unsigned int phy_support_list[] = {
	FH_GMAC_PHY_IP101G,
	FH_GMAC_PHY_RTL8201,
	FH_GMAC_PHY_TI83848,
	FH_GMAC_PHY_INTERNAL,
};

struct mdio_pin_mux_ref _mdio_pin_ref_obj[] = {
	{.old = "MAC_MDC", .restore = 0},
	{.old = "MAC_MDIO", .restore = 0},
	{.old = "MAC_RMII_CLK", .restore = 0},
	{.old = "EX_PHY_RESET_PIN", .restore = 0},
};

void external_phy_pin_sel(Gmac_Object *gmac, int flag)
{
	int i;
	char temp_buf[16] = {0};

	if (flag) {
		for (i = 0; i < ARRAY_SIZE(_mdio_pin_ref_obj); i++) {
			if (strcmp(_mdio_pin_ref_obj[i].old,
			"EX_PHY_RESET_PIN") == 0) {
				sprintf(temp_buf,
				"GPIO%d", gmac->phyreset_gpio);
				_mdio_pin_ref_obj[i].restore =
				fh_pinctrl_smux_backup(temp_buf, temp_buf, 0);
				if (!_mdio_pin_ref_obj[i].restore)
					pr_err("pin mux error!!!!\n");
			} else {
				_mdio_pin_ref_obj[i].restore =
				fh_pinctrl_smux_backup("RMII",
				_mdio_pin_ref_obj[i].old, 0);
				if (!_mdio_pin_ref_obj[i].restore)
					pr_err("pin mux error!!!!\n");
				if (strcmp(_mdio_pin_ref_obj[i].old,
				"MAC_RMII_CLK") == 0)
					fh_pinctrl_smux("RMII",
					"MAC_REF_CLK", 0, 0);
			}
		}
	} else {
		for (i = 0; i < ARRAY_SIZE(_mdio_pin_ref_obj); i++) {
			if (!_mdio_pin_ref_obj[i].restore)
				continue;
			fh_pinctrl_smux_restore("RMII",
			_mdio_pin_ref_obj[i].restore, 0);
			if (strcmp(_mdio_pin_ref_obj[i].old,
			"MAC_RMII_CLK") == 0)
				fh_pinctrl_set_oe("MAC_REF_CLK", 0);
		}
	}
}


static int __fh_mdio_read(Gmac_Object *pGmac, int phyaddr, int phyreg)
{
	int ret;
#ifdef CONFIG_EMULATION
	return 0xe3ff;
#else
	int timeout = 1000;

	if (phyaddr < 0)
		return -ENODEV;

	writel(phyaddr << 11 | gmac_gmii_clock_100_150 << 2 | phyreg << 6 | 0x1,
	       pGmac->remap_addr + REG_GMAC_GMII_ADDRESS);

	while (readl(pGmac->remap_addr + REG_GMAC_GMII_ADDRESS) & 0x1) {
		udelay(100);
		timeout--;
		if (timeout < 0) {
			pr_err("ERROR: %s, timeout, phyaddr: 0x%x, phyreg: 0x%x\n",
					__func__, phyaddr, phyreg);
			break;
		}
	}
	ret = readl(pGmac->remap_addr + REG_GMAC_GMII_DATA);
	/*pr_err("add : reg : data = %x : %x : %x\n",phyaddr, phyreg, ret);*/
	return ret;
#endif
}

static int get_phy_id(Gmac_Object *gmac, int addr, u32 *phy_id)
{
	__u16 phy_reg;

	phy_reg = __fh_mdio_read(gmac, addr, MII_PHYSID1);
	*phy_id = (phy_reg & 0xffff) << 16;
	phy_reg = __fh_mdio_read(gmac, addr, MII_PHYSID2);
	*phy_id |= (phy_reg & 0xffff);

	return 0;
}


int check_white_list(__u32 phy_id, __u32 *list, __u32 size)
{
	int i;
	int ret = -1;

	for (i = 0; i < size; i++) {
		if (phy_id == list[i]) {
			ret = 0;
			break;
		}
	}
	return ret;
}

/* 0 find vaild, others failed*/
int scan_vaild_phy_id(Gmac_Object *gmac, __u32 *white_list, __u32 list_size)
{
	__u32 phy_id = 0;
	int i;
	int ret;

	for (i = 0; i < PHY_MAX_ADDR; i++) {
		ret = get_phy_id(gmac, i, &phy_id);
		if (ret == 0 && (phy_id & 0x1fffffff) != 0x1fffffff) {
			if (check_white_list(phy_id, white_list, list_size))
				continue;
			break;
		}
	}
	if (i == PHY_MAX_ADDR)
		return -1;

	return 0;
}


int fh_phy_reset_api(struct phy_device *phydev)
{
	struct net_device *ndev;
	Gmac_Object *pGmac;

	ndev = phydev->mdio.bus->priv;
	pGmac = netdev_priv(ndev);
	fh_pmu_ephy_sel(pGmac->phy_sel);
	fh_gmac_ephy_reset(pGmac, pGmac->phy_sel);
	fh_mdio_set_mii(pGmac->mii);
	return 0;

}
EXPORT_SYMBOL(fh_phy_reset_api);

int auto_find_phy(Gmac_Object *gmac)
{
	int i;
	int switch_flag = 0;
	char *c_driver;
	int ret;
#ifdef CONFIG_EMULATION
	gmac->phy_sel = EXTERNAL_PHY;
	return 0;
#else

	for (i = 0, c_driver = phy_driver_array[0];
	c_driver != 0; i++, c_driver = phy_driver_array[i]) {
		/* phy init first */
		if (strcmp(c_driver, "FH EPHY") == 0) {
			fh_pmu_ephy_sel(INTERNAL_PHY);
			fh_gmac_ephy_reset(gmac, INTERNAL_PHY);
			gmac->phy_sel = INTERNAL_PHY;
		} else {
			external_phy_pin_sel(gmac, 1);
			switch_flag = 1;
			fh_pmu_ephy_sel(EXTERNAL_PHY);
			fh_gmac_ephy_reset(gmac, EXTERNAL_PHY);
			gmac->phy_sel = EXTERNAL_PHY;
		}
		/* find phy id.. */
		ret = scan_vaild_phy_id(gmac, phy_support_list,
		ARRAY_SIZE(phy_support_list));
		if (!ret)
			break;
		/*restore extenal pin set.*/
		if (switch_flag)
			external_phy_pin_sel(gmac, 0);

	}

	if (!c_driver) {
		pr_err("find no phy..\n");
		gmac->phy_sel = 0;
		if (switch_flag)
			external_phy_pin_sel(gmac, 0);
		return -1;
	}

	if (strcmp(c_driver, "Generic PHY") == 0)
		fh_pinctrl_sdev("RMII", 0);

	return 0;
#endif
}

static int __fh_mdio_write(Gmac_Object *pGmac, int phyaddr, int phyreg,
			 u16 phydata)
{
#ifdef CONFIG_EMULATION
	return 0;
#else
	int timeout = 1000;

	if (phyaddr < 0)
		return -ENODEV;

	writel(phydata, pGmac->remap_addr + REG_GMAC_GMII_DATA);
	writel(0x1 << 1 | phyaddr << 11 | gmac_gmii_clock_100_150 << 2 | phyreg
	       << 6 | 0x1, pGmac->remap_addr + REG_GMAC_GMII_ADDRESS);

	while (readl(pGmac->remap_addr + REG_GMAC_GMII_ADDRESS) & 0x1) {
		udelay(100);
		timeout--;
		if (timeout < 0) {
			pr_err("ERROR: %s, timeout, phyaddr: %d, phyreg: 0x%x, phydata: 0x%x\n",
					__func__, phyaddr, phyreg, phydata);
			break;
		}
	}
	return 0;
#endif
}


static int fh_mdio_read(struct mii_bus *bus, int phyaddr, int phyreg)
{
	struct net_device *ndev = bus->priv;
	Gmac_Object *pGmac = netdev_priv(ndev);

	return __fh_mdio_read(pGmac, phyaddr, phyreg);
}

static int fh_mdio_write(struct mii_bus *bus, int phyaddr, int phyreg,
			 u16 phydata)
{
	struct net_device *ndev = bus->priv;
	Gmac_Object *pGmac = netdev_priv(ndev);

	return __fh_mdio_write(pGmac, phyaddr, phyreg, phydata);
}


static int fh_gmac_ephy_reset(Gmac_Object *pGmac, __u32 phy_sel)
{
	int ret = 0;
#if defined(CONFIG_ARCH_FH885xV200) || \
defined(CONFIG_ARCH_FH865x)
	if (phy_sel == EXTERNAL_PHY) {
		ret = gpio_request(pGmac->phyreset_gpio, "phyreset-gpio");
		if (ret) {
			pr_err("%s: ERROR: request reset pin : %d failed\n",
			__func__, pGmac->phyreset_gpio);
			ret = -ENODEV;
			return ret;
		}

		gpio_direction_output(pGmac->phyreset_gpio,
		GPIOF_OUT_INIT_HIGH);
		gpio_direction_output(pGmac->phyreset_gpio,
		GPIOF_OUT_INIT_LOW);
		/* actual is 20ms */
		usleep_range(10000, 20000);
		gpio_direction_output(pGmac->phyreset_gpio,
		GPIOF_OUT_INIT_HIGH);
		usleep_range(150000, 200000);

		gpio_free(pGmac->phyreset_gpio);

		return ret;
	}
	fh_pmu_internal_ephy_reset();
#else

	ret = gpio_request(pGmac->phyreset_gpio, "phyreset-gpio");
	if (ret) {
		pr_err("%s: ERROR: request reset pin : %d failed\n",
		__func__, pGmac->phyreset_gpio);
		ret = -ENODEV;
		return ret;
	}

	gpio_direction_output(pGmac->phyreset_gpio, GPIOF_OUT_INIT_HIGH);
	gpio_direction_output(pGmac->phyreset_gpio, GPIOF_OUT_INIT_LOW);
	/* actual is 20ms */
	usleep_range(10000, 20000);
	gpio_direction_output(pGmac->phyreset_gpio, GPIOF_OUT_INIT_HIGH);
	usleep_range(150000, 200000);

	gpio_free(pGmac->phyreset_gpio);

	return ret;

#endif
	return 0;
}



int fh_mdio_reset(struct mii_bus *bus)
{
	struct net_device *ndev = bus->priv;
	Gmac_Object *pGmac = netdev_priv(ndev);

	fh_pmu_ephy_sel(pGmac->phy_sel);

	pGmac->phy_reset = fh_gmac_ephy_reset;

	return pGmac->phy_reset(pGmac, pGmac->phy_sel);
}

static int fh_mdio_set_mii(struct mii_bus *bus)
{
	struct net_device *ndev = bus->priv;
	Gmac_Object *pGmac = netdev_priv(ndev);
	__u32 rmii_mode;
	__u32 ret;
	int phyid = pGmac->phyid;

	if (pGmac->phydev == NULL)
		return -ENODEV;

	switch (pGmac->phydev->phy_id) {
	case FH_GMAC_PHY_RTL8201:
		fh_mdio_write(bus, phyid,
				gmac_phyt_rtl8201_page_select, 7);
		fh_mdio_write(bus, phyid,
				gmac_phyt_rtl8201_rmii_mode, 0x1ffa);
		fh_mdio_write(bus, phyid,
				gmac_phyt_rtl8201_page_select, 0);
		break;
	case FH_GMAC_PHY_IP101G:
		fh_mdio_write(bus, phyid,
				gmac_phyt_ip101g_page_select, 16);
		fh_mdio_write(bus, phyid,
				gmac_phyt_rtl8201_rmii_mode, 0x1006);
		fh_mdio_write(bus, phyid,
				gmac_phyt_ip101g_page_select, 0x10);
		break;
	case FH_GMAC_PHY_TI83848:
		rmii_mode = fh_mdio_read(bus, phyid,
				gmac_phyt_ti83848_rmii_mode);
		rmii_mode |= 0x20;
		fh_mdio_write(bus, phyid,
				gmac_phyt_ti83848_rmii_mode, rmii_mode);
		break;
	case FH_GMAC_PHY_INTERNAL:
		pr_info("found fh internel phy...\n");
		/* switch to page 1 */
		fh_mdio_write(bus, phyid, 0x1f, 0x100);
		/* Disable APS */
		fh_mdio_write(bus, phyid, 0x12, 0x4824);
		/*switch to page 2*/
		fh_mdio_write(bus, phyid, 0x1f, 0x200);
		/* PHY AFE TRX optimization */
		fh_mdio_write(bus, phyid, 0x18, 0);
		/* switch to page 6 */
		fh_mdio_write(bus, phyid, 0x1f, 0x600);
		/* PHY AFE ADC optimization */
		fh_mdio_write(bus, phyid, 0x10, 0x555b);
		/* PHYAFE TX optimization */
		fh_mdio_write(bus, phyid, 0x14, 0x708f);
		/* CP current optimization */
		fh_mdio_write(bus, phyid, 0x17, 0x575);
		/* ADC OP BIAS optimization */
		fh_mdio_write(bus, phyid, 0x18, 0);
		/* RX signal detector level optimization */
		fh_mdio_write(bus, phyid, 0x19, 0x408);
		/* PHY AEF PDCW optimization */
		fh_mdio_write(bus, phyid, 0x1c, 0x8880);

		/*drive set below.*/
		/*first debug open*/
		fh_mdio_write(bus, phyid, 0x1f, 0x800);
		fh_mdio_write(bus, phyid, 0x1d, 0x844);
		fh_mdio_write(bus, phyid, 0x1f, 0x600);
		fh_mdio_write(bus, phyid, 0x15, 0x1320);

		fh_mdio_write(bus, phyid, 0x1f, 0x600);
		ret = fh_mdio_read(bus, phyid, 21);
		ret &= ~(0x1f << 8 | 0x1f);
		ret |= 0x10 << 8 | 0x1f | 1 << 13;
		fh_mdio_write(bus, phyid, 21, ret);

		//read back debug data
		fh_mdio_write(bus, phyid, 0x1f, 0x800);
		ret = fh_mdio_read(bus, phyid, 0x1e);
		pr_err("AFE driver [Raw] : [100M] : [10M] = [%x] : [%x] : [%x]\n",
		ret, (ret >> 8) & 0x1f, (ret & 0x1f));
		break;

	default:
#ifdef CONFIG_EMULATION
		return 0;
#else
		return -ENODEV;
#endif
	}

	return 0;
}

int fh_mdio_register(struct net_device *ndev)
{
	int err = 0, found, addr;
	struct mii_bus *new_bus;
	Gmac_Object *pGmac = netdev_priv(ndev);
	struct mdio_device *mdiodev = NULL;

	new_bus = mdiobus_alloc();
	if (new_bus == NULL)
		return -ENOMEM;
	new_bus->name =
	    pGmac->phy_interface ==
	    PHY_INTERFACE_MODE_MII ? "gmac_mii" : "gmac_rmii";
	new_bus->read = &fh_mdio_read;
	new_bus->write = &fh_mdio_write;
	new_bus->reset = &fh_mdio_reset;
	snprintf(new_bus->id, MII_BUS_ID_SIZE, "%x", 0);
	new_bus->priv = ndev;
	new_bus->parent = pGmac->dev;
	err = mdiobus_register(new_bus);
	if (err != 0) {
		pr_err("%s: Cannot register as MDIO bus, error: %d\n",
		       new_bus->name, err);
		goto bus_register_fail;
	}

	pGmac->mii = new_bus;

	found = 0;
	for (addr = 0; addr < 32; addr++) {
		mdiodev = new_bus->mdio_map[addr];
		if (mdiodev) {
			if (pGmac->phyid == -1)
				pGmac->phyid = addr;

			found = 1;
			pGmac->phydev = to_phy_device(&mdiodev->dev);
			break;
		}
	}

	if (pGmac->mii == NULL || mdiodev == NULL) {
		pr_warn("%s: MII BUS or phydev is NULL\n", ndev->name);
		err = -ENXIO;
		goto bus_register_fail;
	}

	err = fh_mdio_set_mii(pGmac->mii);

	if (!found || err) {
		pr_warn("%s: No PHY found\n", ndev->name);
		err = -ENXIO;
		goto bus_register_fail;
	}

	pr_info("%s: PHY ID %08x at %d IRQ %d (%s)%s\n",
		ndev->name, pGmac->phydev->phy_id, addr,
		pGmac->phydev->irq, dev_name(&pGmac->phydev->mdio.dev),
		(addr ==
		 pGmac->phyid) ? " active" : "");

	return 0;

bus_register_fail:
	pGmac->phydev = NULL;
	mdiobus_unregister(new_bus);
	mdiobus_free(new_bus);
	return err;
}

int fh_mdio_unregister(struct net_device *ndev)
{
	Gmac_Object *pGmac = netdev_priv(ndev);

	mdiobus_unregister(pGmac->mii);
	mdiobus_free(pGmac->mii);
	pGmac->mii->priv = NULL;
	pGmac->phydev = NULL;
	return 0;
}
