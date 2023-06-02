/* =========================================================================
 * The Synopsys DWC ETHER QOS Software Driver and documentation (hereinafter
 * "Software") is an unsupported proprietary work of Synopsys, Inc. unless
 * otherwise expressly agreed to in writing between Synopsys and you.
 *
 * The Software IS NOT an item of Licensed Software or Licensed Product under
 * any End User Software License Agreement or Agreement for Licensed Product
 * with Synopsys or any supplement thereto.  Permission is hereby granted,
 * free of charge, to any person obtaining a copy of this software annotated
 * with this license and the Software, to deal in the Software without
 * restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THIS SOFTWARE IS BEING DISTRIBUTED BY SYNOPSYS SOLELY ON AN "AS IS" BASIS
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE HEREBY DISCLAIMED. IN NO EVENT SHALL SYNOPSYS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * ========================================================================= */

/*!@file: dwc_eth_qos.c
 * @brief: Driver functions.
 */
#include "DWC_ETH_QOS_yheader.h"
#include <plat/nvt-sramctl.h>

#ifdef CONFIG_OF
#include <linux/of_device.h>
#endif

#define DRV_VERSION		"1.04.000"
static UCHAR dev_addr[6] = {0, 0x55, 0x7b, 0xb5, 0x7d, 0xf7};

void __iomem *ETH_QOS_REG_BASE;

void DWC_ETH_QOS_init_all_fptrs(struct DWC_ETH_QOS_prv_data *pdata)
{
	DWC_ETH_QOS_init_function_ptrs_dev(&pdata->hw_if);
	DWC_ETH_QOS_init_function_ptrs_desc(&pdata->desc_if);
}

static void nvt_eth_phy_poweron(void)
{
	unsigned long reg;

	reg = ioread32((void*)(ETH_QOS_REG_BASE + 0x3800 + 0xF8));
	iowrite32(reg | (1<<7), (void *)(ETH_QOS_REG_BASE + 0x3800 + 0xF8));
	udelay(20);
	reg = ioread32((void*)(ETH_QOS_REG_BASE + 0x3800 + 0xC8));
	iowrite32(reg & (~(1<<0)), (void *)(ETH_QOS_REG_BASE + 0x3800 + 0xC8));
	udelay(200);
	reg = ioread32((void*)(ETH_QOS_REG_BASE + 0x3800 + 0xC8));
	iowrite32(reg & (~(1<<1)), (void *)(ETH_QOS_REG_BASE + 0x3800 + 0xC8));
	udelay(250);
	reg = ioread32((void*)(ETH_QOS_REG_BASE + 0x3800 + 0x2E8));
	iowrite32(reg & (~(1<<0)), (void *)(ETH_QOS_REG_BASE + 0x3800 + 0x2E8));
	reg = ioread32((void*)(ETH_QOS_REG_BASE + 0x3800 + 0xCC));
	iowrite32(reg & (~(1<<0)), (void *)(ETH_QOS_REG_BASE + 0x3800 + 0xCC));
	reg = ioread32((void*)(ETH_QOS_REG_BASE + 0x3800 + 0xDC));
	iowrite32(reg | (1<<0), (void *)(ETH_QOS_REG_BASE + 0x3800 + 0xDC));
	reg = ioread32((void*)(ETH_QOS_REG_BASE + 0x3800 + 0x9C));
	iowrite32(reg & (~(1<<0)), (void *)(ETH_QOS_REG_BASE + 0x3800 + 0x9C));
}

static void nvt_eth_env_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct resource *res;
	void __iomem *base;
	PIN_GROUP_CONFIG pinmux_config[1];
#ifdef CONFIG_OF
	struct device_node *np = pdev->dev.of_node;
	int spclk_en = 0;
	int refclk_out = 0;
#endif

	ETH_QOS_REG_BASE = (void __iomem *)(0xFD2B0000);    // pre-assume base address
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(base)) {
		dev_err(&pdev->dev, "Unable to find IORESOURCE_MEM, assume 0xFD2B0000\n");
		ETH_QOS_REG_BASE = (void __iomem *)(0xFD2B0000);    // pre-assume base address
	} else {
		ETH_QOS_REG_BASE = base;
	}
	printk("%s: get IO MEM 0x%px\r\n", __func__, base);

	pinmux_config[0].pin_function = PIN_FUNC_ETH;
	ret = nvt_pinmux_capture(pinmux_config, 1);
	if (ret)
		dev_err(&pdev->dev, "Get eth pinmux fail\n");

	if (pinmux_config[0].config & (PIN_ETH_CFG_RMII|PIN_ETH_CFG_RMII_2)) {
		unsigned long reg;

		printk("%s: pinmux detect RMII 0x%x\r\n", __func__, pinmux_config[0].config);
		if (nvt_get_chip_id() != CHIP_NA51055) {
			struct clk *eth_clk;
			struct clk *src_clk;

			eth_clk = clk_get(&pdev->dev, dev_name(&pdev->dev));
			if (IS_ERR(eth_clk)) {
				pr_err("%s: failed to get eth clock\n", __func__);
				ret = -EPERM;
			} else {
				src_clk = clk_get(NULL, "pll16");
				if (IS_ERR(src_clk)) {
					pr_err("%s: failed to get pll16\n", __func__);
					ret = -EPERM;
				}
				clk_set_parent(eth_clk, src_clk);
				clk_put(src_clk);
			}
			clk_put(eth_clk);
		}
#ifdef CONFIG_OF
		// output 25 MHz as external phy crystal input by DTB setting
		if(!of_property_read_u32(np, "sp-clk", &spclk_en)) {
			printk("%s: find sp-clk %d\r\n", __func__, spclk_en);
			if(spclk_en &&
					(!(pinmux_config[0].config & PIN_ETH_CFG_EXTPHYCLK))) {
				struct clk *clk;

				printk("%s: change extphyclk pinmux\r\n", __func__);
				pinmux_config[0].config |= PIN_ETH_CFG_EXTPHYCLK;

				ret = nvt_pinmux_update(pinmux_config, 1);
				if (ret)
					pr_info("%s(%d) setup EXTPHYCLK pinmux fail\n",
					        __func__,__LINE__);

				clk = clk_get(&pdev->dev, "ext_phy_clk");
				if (IS_ERR(clk)) {
					dev_err(&pdev->dev, "can't find clock %s\n", dev_name(&pdev->dev));
					clk = NULL;
				} else {
					clk_prepare(clk);
					clk_enable(clk);
				}
				clk_put(clk);
			}
		}

		// assert phy reset
		gpio_direction_output(L_GPIO(22), 0);
		gpio_set_value(L_GPIO(22), 0);
		mdelay(20);
		gpio_set_value(L_GPIO(22), 1);
		mdelay(50);
#endif

		// select external phy
		reg = ioread32((void*)(ETH_QOS_REG_BASE + 0x3000 + 0x4));
		iowrite32(reg | (1<<8) | (1<<31), (void *)(ETH_QOS_REG_BASE + 0x3000 + 0x4));

		// setup RMII REF_CLK direction
		reg = ioread32((void*)(ETH_QOS_REG_BASE + 0x3000 + 0x14));
		// pre-assume refclk: phy--> mac
		reg |= 1<<0;
		reg &= ~(1<<4);
#ifdef CONFIG_OF
		// change RMII REF_CLK direction by DTB setting
		if(!of_property_read_u32(np, "ref-clk-out", &refclk_out)) {
			printk("%s: find ref-clk-out %d\r\n", __func__, refclk_out);
			if(refclk_out) {
				printk("%s: change REF_CLK out\r\n", __func__);
				reg &= ~(1<<0);
				reg |= 1<<4;
			}
		}
#endif
		iowrite32(reg | (1<<5), (void *)(ETH_QOS_REG_BASE + 0x3000 + 0x14));
	} else if ((pinmux_config[0].config & PIN_ETH_CFG_RGMII) &&
			(nvt_get_chip_id() != CHIP_NA51055)) {
		unsigned long reg;
		struct clk *eth_clk;
		struct clk *src_clk;

		printk("%s: pinmux detect RGMII 0x%x\r\n", __func__, pinmux_config[0].config);
		eth_clk = clk_get(&pdev->dev, dev_name(&pdev->dev));
                if (IS_ERR(eth_clk)) {
                        pr_err("%s: failed to get eth clock\n", __func__);
                        ret = -EPERM;
                } else {
                        src_clk = clk_get(NULL, "pll16");
                        if (IS_ERR(src_clk)) {
                                pr_err("%s: failed to get pll16\n", __func__);
                                ret = -EPERM;
                        }
                        clk_set_parent(eth_clk, src_clk);
                        clk_put(src_clk);
                }
                clk_put(eth_clk);
#ifdef CONFIG_OF
		// output 25 MHz as external phy crystal input by DTB setting
		if(!of_property_read_u32(np, "sp-clk", &spclk_en)) {
			printk("%s: find sp-clk %d\r\n", __func__, spclk_en);
			if(spclk_en &&
					(!(pinmux_config[0].config & PIN_ETH_CFG_EXTPHYCLK))) {
				struct clk *clk;

				printk("%s: change extphyclk pinmux\r\n", __func__);
				pinmux_config[0].config |= PIN_ETH_CFG_EXTPHYCLK;

				ret = nvt_pinmux_update(pinmux_config, 1);
				if (ret)
					pr_info("%s(%d) setup EXTPHYCLK pinmux fail\n",
					        __func__,__LINE__);

				clk = clk_get(&pdev->dev, "ext_phy_clk");
				if (IS_ERR(clk)) {
					dev_err(&pdev->dev, "can't find clock %s\n", dev_name(&pdev->dev));
					clk = NULL;
				} else {
					clk_prepare(clk);
					clk_enable(clk);
				}
				clk_put(clk);
			}
		}

		// assert phy reset
		gpio_direction_output(L_GPIO(22), 0);
		gpio_set_value(L_GPIO(22), 0);
		mdelay(20);
		gpio_set_value(L_GPIO(22), 1);
		mdelay(50);
#endif

		// select external phy
		reg = ioread32((void*)(base + 0x3000 + 0x4));
		iowrite32(reg | (1<<8), (void *)(base + 0x3000 + 0x4));
		iowrite32(0x1FF, (void *)(base + 0x3000 + 0xC));

		// setup TXD_SRC as RGMII
		reg = ioread32((void*)(base + 0x3000 + 0x14));
		reg |= 1<<4;
		reg &= ~(3<<30);
		reg |= 1<<30;
		iowrite32(reg, (void *)(base + 0x3000 + 0x14));
	} else {
		int emb_phy_driving;

		printk("%s: pinmux detect emb phy 0x%x\r\n", __func__, pinmux_config[0].config);
		nvt_eth_phy_poweron();
		if(!of_property_read_u32(np, "emb-phy-driving", &emb_phy_driving)) {
			unsigned long reg;
			unsigned long step;
			int value;

			reg = ioread32((void*)(ETH_QOS_REG_BASE + 0x3800 + 0x378));
			step = reg & 0x1F;
			reg &= ~0x1F;
			// transform 2's compelment to integer
			if (step & 0x10) {
				step = ~step;
				step++;
				value = step & 0x1F;
				value = 0 - value;
			} else {
				value = step & 0xF;
			}
			value += emb_phy_driving;
			// transform integer to 2's complement
			if (value >= 0) {
				if (value > 15) value = 15;
				step = value;
			} else {
				if (value <= -16) value = -16;
				value = 0 - value;
				step = value;
				step = ~step;
				step++;
			}
			step &= 0x1F;
			reg |= step;
			iowrite32(reg, (void *)(ETH_QOS_REG_BASE + 0x3800 + 0x378));

			printk("%s: DTS emb driving inc %d\r\n", __func__, emb_phy_driving);
		}
	}


}

#ifdef CONFIG_NVT_MDIO_NA51055_CFG

/*!
* \brief API to adjust link parameters.
*
* \details This function will be called by PAL to inform the driver
* about various link parameters like duplex and speed. This function
* will configure the MAC based on link parameters.
*
* \param[in] dev - pointer to net_device structure
*
* \return void
*/

static void DWC_ETH_QOS_adjust_link(struct net_device *dev)
{
	struct DWC_ETH_QOS_prv_data *pdata = netdev_priv(dev);
	struct hw_if_struct *hw_if = &(pdata->hw_if);
	struct phy_device *phydev = pdata->phydev;
	unsigned long flags;
	int new_state = 0;

	if (phydev == NULL)
		return;

	DBGPR_MDIO("-->DWC_ETH_QOS_adjust_link. address %d link %d\n", phydev->addr,
	      phydev->link);

	spin_lock_irqsave(&pdata->lock, flags);

	if (phydev->link) {
		/* Now we make sure that we can be in full duplex mode.
		 * If not, we operate in half-duplex mode */
		if (phydev->duplex != pdata->oldduplex) {
			new_state = 1;
			if (phydev->duplex)
				hw_if->set_full_duplex();
			else {
				hw_if->set_half_duplex();
			}
			pdata->oldduplex = phydev->duplex;;
		}

		/* FLOW ctrl operation */
		if (phydev->pause || phydev->asym_pause) {
			if (pdata->flow_ctrl != pdata->oldflow_ctrl)
				DWC_ETH_QOS_configure_flow_ctrl(pdata);
		}

		if (phydev->speed != pdata->speed) {
			new_state = 1;
			switch (phydev->speed) {
			case SPEED_1000:
				hw_if->set_gmii_speed();
				break;
			case SPEED_100:
				hw_if->set_mii_speed_100();
				break;
			case SPEED_10:
				hw_if->set_mii_speed_10();
				break;
			}
			pdata->speed = phydev->speed;
		}

		if (!pdata->oldlink) {
			hw_if->start_mac_tx_rx();
			printk("%s: start tx/rx\r\n", __func__);
			new_state = 1;
			pdata->oldlink = 1;
		}
	} else if (pdata->oldlink) {
		hw_if->stop_mac_tx_rx();
		printk("%s: stop tx/rx\r\n", __func__);

		new_state = 1;
		pdata->oldlink = 0;
		pdata->speed = 0;
		pdata->oldduplex = -1;
	}

	if (new_state)
		phy_print_status(phydev);

#ifdef DWC_ETH_QOS_CONFIG_EEE
	/* At this stage, it could be need to setup the EEE or adjust some
	 * MAC related HW registers.
	 * */
	pdata->eee_enabled = DWC_ETH_QOS_eee_init(pdata);
#endif

	spin_unlock_irqrestore(&pdata->lock, flags);

	DBGPR_MDIO("<--DWC_ETH_QOS_adjust_link\n");
}

static int DWC_ETH_QOS_init_phy(struct net_device *dev)
{
	struct DWC_ETH_QOS_prv_data *pdata = netdev_priv(dev);
	struct phy_device *phydev = NULL;
	char phy_id_fmt[MII_BUS_ID_SIZE + 3];
	char bus_id[MII_BUS_ID_SIZE];

	DBGPR_MDIO("-->DWC_ETH_QOS_init_phy\n");

	pdata->oldlink = 0;
	pdata->speed = 0;
	pdata->oldduplex = -1;
	pdata->bus_id = 0x1;

	snprintf(bus_id, MII_BUS_ID_SIZE, "dwc_phy-%x", pdata->bus_id);

	snprintf(phy_id_fmt, MII_BUS_ID_SIZE + 3, PHY_ID_FMT, bus_id,
		 pdata->phyaddr);

	if (pdata->phy_node) {
		printk("%s: trying to attach fixed phy\r\n", __func__);
		phydev = of_phy_connect(dev, pdata->phy_node,
			&DWC_ETH_QOS_adjust_link, 0, pdata->interface);
	} else {
	DBGPR_MDIO("trying to attach to %s\n", phy_id_fmt);
	phydev = phy_connect(dev, phy_id_fmt, &DWC_ETH_QOS_adjust_link,
			     pdata->interface);
	}

	if (IS_ERR(phydev)) {
		printk(KERN_ALERT "%s: Could not attach to PHY\n", dev->name);
		return PTR_ERR(phydev);
	}

	if (phydev->phy_id == 0) {
		phy_disconnect(phydev);
		return -ENODEV;
	}

	if (pdata->interface == PHY_INTERFACE_MODE_GMII) {
		phydev->supported = PHY_GBIT_FEATURES;
#ifdef DWC_ETH_QOS_CERTIFICATION_PKTBURSTCNT_HALFDUPLEX
		phydev->supported &= ~SUPPORTED_1000baseT_Full;
#endif
	} else if ((pdata->interface == PHY_INTERFACE_MODE_MII) ||
		(pdata->interface == PHY_INTERFACE_MODE_RMII)) {
		phydev->supported = PHY_BASIC_FEATURES;
	}

#ifndef DWC_ETH_QOS_CONFIG_PGTEST
	phydev->supported |= (SUPPORTED_Pause | SUPPORTED_Asym_Pause);
#endif

    /* Lets Make the code support for both 100M and Giga bit */
//#ifdef DWC_ETH_QOS_CONFIG_PGTEST
//	phydev->supported = PHY_BASIC_FEATURES;
//#endif

	phydev->advertising = phydev->supported;

	DBGPR_MDIO("%s: attached to PHY (UID 0x%x) Link = %d\n", dev->name,
	      phydev->phy_id, phydev->link);

	pdata->phydev = phydev;
//	phy_start(pdata->phydev);

	DBGPR_MDIO("<--DWC_ETH_QOS_init_phy\n");

	return 0;
}

int DWC_ETH_QOS_mdio_connect(struct net_device *dev)
{
	struct DWC_ETH_QOS_prv_data *pdata = netdev_priv(dev);
	int ret = Y_SUCCESS;
	struct device_node *dn;
	int mii_status;
	unsigned short phy_detected = 0;
	int phyaddr = 0;

	pdata->phyaddr=1;
	dn = of_find_compatible_node(NULL, NULL, "nvt,eth_mdio");
	pdata->mii = of_mdio_find_bus(dn);
	if(pdata->mii != NULL){

		for (phyaddr = 0; phyaddr < 32; phyaddr++) {

			mii_status = mdiobus_read(pdata->mii, phyaddr, MII_BMSR);

			if (mii_status != 0x0000 && mii_status != 0xffff) {
				printk(KERN_ALERT "Phy detected at"\
				" ID/ADDR %d\n" , phyaddr);
				phy_detected = 1;
				break;
			}

		}
		if (!phy_detected) {
			printk(KERN_ALERT "No phy could be detected\n");
			return -ENOLINK;
		}
		pdata->phyaddr=phyaddr;
	}else{
		printk(KERN_ALERT "No mdio could be detected\n");
		return -ENOLINK;
	}

	ret = DWC_ETH_QOS_init_phy(dev);
	if (unlikely(ret)) {
		printk(KERN_ALERT "Cannot attach to PHY (error: %d)\n", ret);
	}
	return ret;
}

#endif

/*!
* \brief API to initialize the device.
*
* \details This probing function gets called (during execution of
* pci_register_driver() for already existing devices or later if a
* new device gets inserted) for all PCI devices which match the ID table
* and are not "owned" by the other drivers yet. This function gets passed
* a "struct pci_dev *" for each device whose entry in the ID table matches
* the device. The probe function returns zero when the driver chooses to take
* "ownership" of the device or an error code (negative number) otherwise.
* The probe function always gets called from process context, so it can sleep.
*
* \param[in] pdev - pointer to platform_device structure.
*
* \return integer
*
* \retval 0 on success & -ve number on failure.
*/

static int DWC_ETH_QOS_probe(struct platform_device *pdev)
{

	struct DWC_ETH_QOS_prv_data *pdata = NULL;
	struct net_device *dev = NULL;
	int i, ret = 0;
	struct hw_if_struct *hw_if = NULL;
	struct desc_if_struct *desc_if = NULL;
	UCHAR tx_q_count = 0, rx_q_count = 0;
	struct clk *clk;
	void __iomem *base;

	DBGPR("--> DWC_ETH_QOS_probe\n");

	nvt_eth_env_probe(pdev);
	base = ETH_QOS_REG_BASE;

	/* queue count */
	tx_q_count = get_tx_queue_count();
	rx_q_count = get_rx_queue_count();

	dev = alloc_etherdev_mqs(sizeof(struct DWC_ETH_QOS_prv_data),
				tx_q_count, rx_q_count);
	if (dev == NULL) {
		printk(KERN_ALERT "%s:Unable to alloc new net device\n",
		    DEV_NAME);
		ret = -ENOMEM;
		goto err_out_dev_failed;
	}
	dev->dev_addr[0] = dev_addr[0];
	dev->dev_addr[1] = dev_addr[1];
	dev->dev_addr[2] = dev_addr[2];
	dev->dev_addr[3] = dev_addr[3];
	dev->dev_addr[4] = dev_addr[4];
	dev->dev_addr[5] = dev_addr[5];

	SET_NETDEV_DEV(dev, &pdev->dev);
	pdata = netdev_priv(dev);
	DWC_ETH_QOS_init_all_fptrs(pdata);
	hw_if = &(pdata->hw_if);
	desc_if = &(pdata->desc_if);
	platform_set_drvdata(pdev, dev);
	pdata->pdev = pdev;

	pdata->dev = dev;
	pdata->tx_queue_cnt = tx_q_count;
	pdata->rx_queue_cnt = rx_q_count;

	pdata->mau_dummy_ptr = dma_alloc_coherent(&pdata->pdev->dev,
						DUMMY_BUF_SIZE,
						&pdata->mau_dummy_dma_addr,
						GFP_KERNEL);
	if (pdata->mau_dummy_ptr == NULL) {
		printk("%s: allocate mau dummy buffer fail\r\n", __func__);
		goto err_out_q_alloc_failed;
	}

#ifdef CONFIG_OF
	clk = clk_get(&pdev->dev, dev_name(&pdev->dev));
	if (IS_ERR(clk)) {
		dev_err(&pdev->dev, "can't find clock %s\n", dev_name(&pdev->dev));
		clk = NULL;
	}
	else {
		clk_prepare(clk);
		clk_enable(clk);
	}
	clk_put(clk);
	nvt_disable_sram_shutdown(ETH_SD);
#endif

#ifdef DWC_ETH_QOS_CONFIG_DEBUGFS
	create_debug_files();
	/* to give prv data to debugfs */
	DWC_ETH_QOS_get_pdata(pdata);
#endif

	/* issue software reset to device */
	hw_if->exit(pdata);
	dev->irq = platform_get_irq(pdev, 0);

	DWC_ETH_QOS_get_all_hw_features(pdata);
	//DWC_ETH_QOS_print_all_hw_features(pdata);
	pdata->max_addr_reg_cnt = 32;

	ret = desc_if->alloc_queue_struct(pdata);
	if (ret < 0) {
		printk(KERN_ALERT "ERROR: Unable to alloc Tx/Rx queue\n");
		goto err_out_q_alloc_failed;
	}

	dev->netdev_ops = DWC_ETH_QOS_get_netdev_ops();

	pdata->interface = DWC_ETH_QOS_get_phy_interface(pdata);
#if CONFIG_OF
	if (of_phy_is_fixed_link(pdev->dev.of_node)) {
		printk("%s: fixed link found\r\n", __func__);
		if (of_phy_register_fixed_link(pdev->dev.of_node) == 0) {
			pdata->phy_node = of_node_get(pdev->dev.of_node);
			printk("%s: register fixed phy ok\r\n", __func__);
		}
	}
#endif
	/* Bypass PHYLIB for TBI, RTBI and SGMII interface */
	if (1 == pdata->hw_feat.sma_sel) {
#ifdef CONFIG_NVT_MDIO_NA51055_CFG
		printk("%s: mdio driver connect \r\n", __func__);
		ret = DWC_ETH_QOS_mdio_connect(dev);
#else
		printk("%s: mdio driver register \r\n", __func__);
		ret = DWC_ETH_QOS_mdio_register(dev);
#endif
		if (ret < 0) {
			printk(KERN_ALERT "MDIO bus (id %d) registration failed\n",
			       pdata->bus_id);
			goto err_out_mdio_reg;
		}
	} else {
		printk(KERN_ALERT "%s: MDIO is not present\n\n", DEV_NAME);
	}

#ifndef DWC_ETH_QOS_CONFIG_PGTEST
	/* enabling and registration of irq with magic wakeup */
	if (1 == pdata->hw_feat.mgk_sel) {
		device_set_wakeup_capable(&pdev->dev, 1);
		pdata->wolopts = WAKE_MAGIC;
		enable_irq_wake(dev->irq);
	}

	for (i = 0; i < DWC_ETH_QOS_RX_QUEUE_CNT; i++) {
		struct DWC_ETH_QOS_rx_queue *rx_queue = GET_RX_QUEUE_PTR(i);

		netif_napi_add(dev, &rx_queue->napi, DWC_ETH_QOS_poll_mq,
				(64 * DWC_ETH_QOS_RX_QUEUE_CNT));
	}

	dev->ethtool_ops = DWC_ETH_QOS_get_ethtool_ops();
	if (pdata->hw_feat.tso_en) {
		dev->hw_features = NETIF_F_TSO;
#ifdef DWC_ETH_QOS_CONFIG_UFO
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,14,00)
		dev->hw_features |= NETIF_F_UFO;
#endif
#endif
		dev->hw_features |= NETIF_F_SG;
		dev->hw_features |= NETIF_F_IP_CSUM;
		dev->hw_features |= NETIF_F_IPV6_CSUM;
		printk(KERN_ALERT "Supports TSO, SG and TX COE\n");
	}
	else if (pdata->hw_feat.tx_coe_sel) {
		dev->hw_features = NETIF_F_IP_CSUM ;
		dev->hw_features |= NETIF_F_IPV6_CSUM;
		printk(KERN_ALERT "Supports TX COE\n");
	}

	if (pdata->hw_feat.rx_coe_sel) {
		dev->hw_features |= NETIF_F_RXCSUM;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,6,00)
		dev->hw_features |= NETIF_F_GRO;
		printk(KERN_ALERT "Supports RX COE and GRO\n");
#else
		dev->hw_features |= NETIF_F_LRO;
		printk(KERN_ALERT "Supports RX COE and LRO\n");
#endif
	}
#ifdef DWC_ETH_QOS_ENABLE_VLAN_TAG
	dev->vlan_features |= dev->hw_features;
	dev->hw_features |= NETIF_F_HW_VLAN_CTAG_RX;
	if (pdata->hw_feat.sa_vlan_ins) {
		dev->hw_features |= NETIF_F_HW_VLAN_CTAG_TX;
		printk(KERN_ALERT "VLAN Feature enabled\n");
	}
	if (pdata->hw_feat.vlan_hash_en) {
		dev->hw_features |= NETIF_F_HW_VLAN_CTAG_FILTER;
		printk(KERN_ALERT "VLAN HASH Filtering enabled\n");
	}
#endif /* end of DWC_ETH_QOS_ENABLE_VLAN_TAG */
	dev->features |= dev->hw_features;
	pdata->dev_state |= dev->features;

	DWC_ETH_QOS_init_rx_coalesce(pdata);

#ifdef DWC_ETH_QOS_CONFIG_PTP
	DWC_ETH_QOS_ptp_init(pdata);
#endif	/* end of DWC_ETH_QOS_CONFIG_PTP */

#endif /* end of DWC_ETH_QOS_CONFIG_PGTEST */

	spin_lock_init(&pdata->lock);
	spin_lock_init(&pdata->tx_lock);
	spin_lock_init(&pdata->pmt_lock);
	spin_lock_init(&pdata->rx_int_lock);
	mutex_init(&pdata->ethtool_lock);

#ifdef DWC_ETH_QOS_CONFIG_PGTEST
	ret = DWC_ETH_QOS_alloc_pg(pdata);
	if (ret < 0) {
		printk(KERN_ALERT "ERROR:Unable to allocate PG memory\n");
		goto err_out_pg_failed;
	}
	printk(KERN_ALERT "\n");
	printk(KERN_ALERT "/*******************************************\n");
	printk(KERN_ALERT "*\n");
	printk(KERN_ALERT "* PACKET GENERATOR MODULE ENABLED IN DRIVER\n");
	printk(KERN_ALERT "*\n");
	printk(KERN_ALERT "*******************************************/\n");
	printk(KERN_ALERT "\n");
#endif /* end of DWC_ETH_QOS_CONFIG_PGTEST */

	ret = register_netdev(dev);
	if (ret) {
		printk(KERN_ALERT "%s: Net device registration failed\n",
		    DEV_NAME);
		goto err_out_q_alloc_failed;
	}

	DBGPR("<-- DWC_ETH_QOS_probe\n");

	if (pdata->hw_feat.pcs_sel) {
		netif_carrier_off(dev);
		printk(KERN_ALERT "carrier off till LINK is up\n");
	}

	return 0;

err_out_dev_failed:
#ifdef DWC_ETH_QOS_CONFIG_PTP
	DWC_ETH_QOS_ptp_remove(pdata);
#endif	/* end of DWC_ETH_QOS_CONFIG_PTP */
	platform_set_drvdata(pdev, NULL);
	return ret;

#ifdef DWC_ETH_QOS_CONFIG_PGTEST
	DWC_ETH_QOS_free_pg(pdata);
err_out_pg_failed:
#endif
	if (1 == pdata->hw_feat.sma_sel){
#ifdef CONFIG_NVT_MDIO_NA51055_CFG
		if (pdata->phydev) {
			phy_stop(pdata->phydev);
			phy_disconnect(pdata->phydev);
			pdata->phydev = NULL;
		}
#else
		 DWC_ETH_QOS_mdio_unregister(dev);
#endif
	}

err_out_mdio_reg:
	desc_if->free_queue_struct(pdata);

err_out_q_alloc_failed:
	if (pdata->mau_dummy_ptr != NULL) {
		dma_free_coherent(&pdata->pdev->dev, DUMMY_BUF_SIZE, pdata->mau_dummy_ptr, pdata->mau_dummy_dma_addr);
	}
	free_netdev(dev);
	return ret;
}

/*!
* \brief API to release all the resources from the driver.
*
* \details The remove function gets called whenever a device being handled
* by this driver is removed (either during deregistration of the driver or
* when it is manually pulled out of a hot-pluggable slot). This function
* should reverse operations performed at probe time. The remove function
* always gets called from process context, so it can sleep.
*
* \param[in] pdev - pointer to platform_device structure.
*
* \return void
*/
static int DWC_ETH_QOS_remove(struct platform_device *pdev)
{
	struct net_device *dev = platform_get_drvdata(pdev);
	struct DWC_ETH_QOS_prv_data *pdata = netdev_priv(dev);
	struct desc_if_struct *desc_if = &(pdata->desc_if);
	struct clk *clk;

	DBGPR("--> DWC_ETH_QOS_remove\n");

	if (pdata->irq_number != 0) {
		free_irq(pdata->irq_number, pdata);
		pdata->irq_number = 0;
	}

	if (1 == pdata->hw_feat.sma_sel){
#ifdef CONFIG_NVT_MDIO_NA51055_CFG
		if (pdata->phydev) {
			phy_stop(pdata->phydev);
			phy_disconnect(pdata->phydev);
			pdata->phydev = NULL;
		}
#else
		DWC_ETH_QOS_mdio_unregister(dev);
#endif
	}

#ifdef DWC_ETH_QOS_CONFIG_PTP
	DWC_ETH_QOS_ptp_remove(pdata);
#endif /* end of DWC_ETH_QOS_CONFIG_PTP */

	unregister_netdev(dev);

#ifdef DWC_ETH_QOS_CONFIG_PGTEST
	DWC_ETH_QOS_free_pg(pdata);
#endif /* end of DWC_ETH_QOS_CONFIG_PGTEST */

	desc_if->free_queue_struct(pdata);

	if (pdata->mau_dummy_ptr != NULL) {
		dma_free_coherent(&pdata->pdev->dev, DUMMY_BUF_SIZE, pdata->mau_dummy_ptr, pdata->mau_dummy_dma_addr);
	}

	free_netdev(dev);

#ifdef CONFIG_OF
	clk = clk_get(&pdev->dev, dev_name(&pdev->dev));
	if (IS_ERR(clk)) {
		dev_err(&pdev->dev, "can't find clock %s\n", dev_name(&pdev->dev));
		clk = NULL;
	} else {
		clk_disable(clk);
		clk_unprepare(clk);
	}
	clk_put(clk);
#endif

	DBGPR("<-- DWC_ETH_QOS_remove\n");

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id synopsys_eth_of_dt_ids[] = {
    { .compatible = "nvt,synopsys_eth" },
    {},
};
MODULE_DEVICE_TABLE(of, synopsys_eth_of_dt_ids);
#endif

static struct platform_driver DWC_ETH_QOS_driver = {
	.probe = DWC_ETH_QOS_probe,
	.remove = DWC_ETH_QOS_remove,
#ifdef CONFIG_PM
	.suspend = DWC_ETH_QOS_suspend,
	.resume = DWC_ETH_QOS_resume,
#endif
	.driver = {
			.name = DEV_NAME,
			.owner = THIS_MODULE,
#ifdef CONFIG_OF
			.of_match_table = synopsys_eth_of_dt_ids,
#endif
	},
};

#ifdef CONFIG_PM

/*!
 * \brief Routine to put the device in suspend mode
 *
 * \details This function gets called by PCI core when the device is being
 * suspended. The suspended state is passed as input argument to it.
 * Following operations are performed in this function,
 * - stop the phy.
 * - detach the device from stack.
 * - stop the queue.
 * - Disable napi.
 * - Stop DMA TX and RX process.
 * - Enable power down mode using PMT module or disable MAC TX and RX process.
 * - Save the pci state.
 *
 * \param[in] pdev ??pointer to platform_device structure.
 * \param[in] state ??suspend state of device.
 *
 * \return int
 *
 * \retval 0
 */

static INT DWC_ETH_QOS_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct net_device *dev = platform_get_drvdata(pdev);
	struct clk *clk;
	INT ret = 0;

	DBGPR("-->DWC_ETH_QOS_suspend\n");

	if (!dev || !netif_running(dev)) {
		printk("@@ <--DWC_ETH_QOS_dev_suspend\n");
	} else {
		DWC_ETH_QOS_drv_suspend(dev);
	}

#ifdef CONFIG_OF
        clk = clk_get(&pdev->dev, dev_name(&pdev->dev));
        if (IS_ERR(clk)) {
                dev_err(&pdev->dev, "can't find clock %s\n", dev_name(&pdev->dev));
                clk = NULL;
        } else {
                clk_disable(clk);
                clk_unprepare(clk);
        }
        clk_put(clk);
#endif

	DBGPR("<--DWC_ETH_QOS_suspend\n");

	return ret;
}

/*!
 * \brief Routine to resume device operation
 *
 * \details This function gets called by PCI core when the device is being
 * resumed. It is always called after suspend has been called. These function
 * reverse operations performed at suspend time. Following operations are
 * performed in this function,
 * - restores the saved pci power state.
 * - Wakeup the device using PMT module if supported.
 * - Starts the phy.
 * - Enable MAC and DMA TX and RX process.
 * - Attach the device to stack.
 * - Enable napi.
 * - Starts the queue.
 *
 * \param[in] pdev ?“pointer to platform_device structure.
 *
 * \return int
 *
 * \retval 0
 */

static INT DWC_ETH_QOS_resume(struct platform_device *pdev)
{
	struct net_device *dev = platform_get_drvdata(pdev);
	struct clk *clk;
	INT ret = 0;

	DBGPR("-->DWC_ETH_QOS_resume\n");

#ifdef CONFIG_OF
        clk = clk_get(&pdev->dev, dev_name(&pdev->dev));
        if (IS_ERR(clk)) {
                dev_err(&pdev->dev, "can't find clock %s\n", dev_name(&pdev->dev));
                clk = NULL;
        }
        else {
                clk_prepare(clk);
                clk_enable(clk);
        }
        clk_put(clk);
#endif

	if (!dev || !netif_running(dev)) {
		printk("@@ <--DWC_ETH_QOS_dev_resume\n");
		return 0;
	}

	DWC_ETH_QOS_drv_resume(dev);

	DBGPR("<--DWC_ETH_QOS_resume\n");

	return ret;
}

#endif	/* CONFIG_PM */

/*!
* \brief Macro to register the driver registration function.
*
* \details A module always begin with either the init_module or the function
* you specify with module_init call. This is the entry function for modules;
* it tells the kernel what functionality the module provides and sets up the
* kernel to run the module's functions when they're needed. Once it does this,
* entry function returns and the module does nothing until the kernel wants
* to do something with the code that the module provides.
*/
module_platform_driver(DWC_ETH_QOS_driver);


/*!
* \brief Macro to declare the module author.
*
* \details This macro is used to declare the module's authore.
*/
MODULE_AUTHOR("Synopsys India Pvt Ltd");

/*!
* \brief Macro to describe what the module does.
*
* \details This macro is used to describe what the module does.
*/
MODULE_DESCRIPTION("DWC_ETH_QOS Driver");

/*!
* \brief Macro to describe the module license.
*
* \details This macro is used to describe the module license.
*/
MODULE_VERSION(DRV_VERSION);
MODULE_LICENSE("GPL");
