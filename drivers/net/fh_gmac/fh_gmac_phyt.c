#include <linux/mii.h>
#include <linux/phy.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <mach/fh_gmac.h>
#include "fh_gmac_phyt.h"
#include "fh_gmac.h"

static int fh_mdio_read(struct mii_bus *bus, int phyaddr, int phyreg)
{
	struct net_device *ndev = bus->priv;
	Gmac_Object *pGmac = netdev_priv(ndev);
	int timeout = 1000;

	if (phyaddr < 0)
		return -ENODEV;

	writel(phyaddr << 11 | gmac_gmii_clock_100_150 << 2 | phyreg << 6 | 0x1,
	       pGmac->remap_addr + REG_GMAC_GMII_ADDRESS);

	while (readl(pGmac->remap_addr + REG_GMAC_GMII_ADDRESS) & 0x1) {
		udelay(100);
		timeout--;
		if (timeout < 0) {
			printk(KERN_ERR "ERROR: %s, timeout\n", __func__);
			break;
		}
	}

	return readl(pGmac->remap_addr + REG_GMAC_GMII_DATA);
}

static int fh_mdio_write(struct mii_bus *bus, int phyaddr, int phyreg,
			 u16 phydata)
{
	struct net_device *ndev = bus->priv;
	Gmac_Object *pGmac = netdev_priv(ndev);
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
			printk(KERN_ERR "ERROR: %s, timeout\n", __func__);
			break;
		}
	}
	return 0;
}

int fh_mdio_reset(struct mii_bus *bus)
{
	struct net_device *ndev = bus->priv;
	Gmac_Object *pGmac = netdev_priv(ndev);
	struct fh_gmac_platform_data *plat_data;

	plat_data = pGmac->priv_data;

	if (plat_data && plat_data->phy_reset)
		plat_data->phy_reset();

	return 0;
}

int fh_mdio_set_mii(struct mii_bus *bus)
{
	struct net_device *ndev = bus->priv;
	Gmac_Object *pGmac = netdev_priv(ndev);
	__u32 rmii_mode;
	int phyid = pGmac->priv_data->phyid;

	if (pGmac->phydev == NULL)
		return -ENODEV;

	if (pGmac->phy_interface == PHY_INTERFACE_MODE_RMII) {
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
#if defined(CONFIG_ARCH_FH8856) || defined(CONFIG_ARCH_ZY2) \
	|| defined(CONFIG_ARCH_FH8626V100)
			/* adjust ip101g rxd0 & rxd1 drv curr */
			fh_mdio_write(bus, phyid, 26, 0xc5ed);
#endif
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
		default:
			return -ENODEV;
		}
	} else if (pGmac->phy_interface == PHY_INTERFACE_MODE_MII) {
		switch (pGmac->phydev->phy_id) {
		case FH_GMAC_PHY_RTL8201:
			fh_mdio_write(bus, phyid,
					gmac_phyt_rtl8201_page_select, 7);
			fh_mdio_write(bus, phyid,
					gmac_phyt_rtl8201_rmii_mode, 0x6ff3);
			fh_mdio_write(bus, phyid,
					gmac_phyt_rtl8201_page_select, 0);
			break;
		case FH_GMAC_PHY_IP101G:
			fh_mdio_write(bus, phyid,
					gmac_phyt_ip101g_page_select, 16);
			fh_mdio_write(bus, phyid,
					gmac_phyt_rtl8201_rmii_mode, 0x2);
			fh_mdio_write(bus, phyid,
					gmac_phyt_ip101g_page_select, 0x10);
			break;
		case FH_GMAC_PHY_TI83848:
			rmii_mode = fh_mdio_read(bus, phyid,
					gmac_phyt_ti83848_rmii_mode);
			rmii_mode &= ~(0x20);
			fh_mdio_write(bus, phyid,
					gmac_phyt_ti83848_rmii_mode, rmii_mode);
			break;
		default:
			return -ENODEV;
		}
	}

	return 0;
}

int fh_mdio_register(struct net_device *ndev)
{
	int err = 0, found, addr;
	struct mii_bus *new_bus;
	Gmac_Object *pGmac = netdev_priv(ndev);
	struct phy_device *phydev = NULL;

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
		phydev = new_bus->phy_map[addr];
		if (phydev) {
			if (pGmac->priv_data->phyid == -1)
				pGmac->priv_data->phyid = addr;

			found = 1;
			pGmac->phydev = phydev;
			break;
		}
	}

	if (pGmac->mii == NULL || phydev == NULL) {
		pr_warning("%s: MII BUS or phydev is NULL\n", ndev->name);
		err = -ENXIO;
		goto bus_register_fail;
	}

	err = fh_mdio_set_mii(pGmac->mii);

	if (!found || err) {
		pr_warning("%s: No PHY found\n", ndev->name);
		err = -ENXIO;
		goto bus_register_fail;
	}

	pr_info("%s: PHY ID %08x at %d IRQ %d (%s)%s\n",
		ndev->name, pGmac->phydev->phy_id, addr,
		pGmac->phydev->irq, dev_name(&pGmac->phydev->dev),
		(addr ==
		 pGmac->priv_data->phyid) ? " active" : "");

	return 0;

bus_register_fail:
	pGmac->phydev = NULL;
	mdiobus_unregister(new_bus);
	kfree(new_bus);
	return err;
}

int fh_mdio_unregister(struct net_device *ndev)
{
	Gmac_Object *pGmac = netdev_priv(ndev);

	mdiobus_unregister(pGmac->mii);
	pGmac->mii->priv = NULL;
	kfree(pGmac->mii);
	return 0;
}
