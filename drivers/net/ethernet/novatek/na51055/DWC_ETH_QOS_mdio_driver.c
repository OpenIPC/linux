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
#include <linux/of_mdio.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/delay.h>
/*!@file: DWC_ETH_QOS_mdio.c
 * @brief: Driver functions.
 */
typedef unsigned long ULONG;
typedef int INT;

#define Y_FAILURE 1
#define Y_FALSE 0
#define Y_SUCCESS 0
#define Y_FAILURE 1
#define MAC_GMIIAR_GB_LPOS 0
#define MAC_GMIIAR_GB_HPOS 0

#define MAC_GMIIDR_GD_LPOS 0
#define MAC_GMIIDR_GD_HPOS 15

//#define YDEBUG_MDIO

#define MAC_GMIIAR_RgOffAddr ((volatile ULONG *)(BASE_ADDRESS))

#define MAC_GMIIAR_RgWr(data) do {\
		iowrite32(data, (void *)MAC_GMIIAR_RgOffAddr);\
} while(0)

#define MAC_GMIIAR_RgRd(data) do {\
		(data) = ioread32((void *)MAC_GMIIAR_RgOffAddr);\
} while(0)


#define MAC_GMIIDR_RgOffAddr ((volatile ULONG *)(BASE_ADDRESS + 0x4))

#define MAC_GMIIDR_RgWr(data) do {\
		iowrite32(data, (void *)MAC_GMIIDR_RgOffAddr);\
} while(0)

#define MAC_GMIIDR_RgRd(data) do {\
		(data) = ioread32((void *)MAC_GMIIDR_RgOffAddr);\
} while(0)

#define MAC_GMIIDR_GD_Mask (ULONG)(0xffff)
#define MAC_GMIIDR_GD_Wr_Mask (ULONG)(0xffff0000)
#define MAC_GMIIDR_GD_UdfWr(data) do{\
		ULONG v;\
		MAC_GMIIDR_RgRd(v);\
		v = ((v & MAC_GMIIDR_GD_Wr_Mask) | ((data & MAC_GMIIDR_GD_Mask)<<0));\
		MAC_GMIIDR_RgWr(v);\
} while(0)

#define GET_VALUE(data, lbit, hbit) ((data >>lbit) & (~(~0<<(hbit-lbit+1))))

#ifdef YDEBUG_MDIO
#define DBGPR_MDIO(x...) printk(KERN_ALERT x)
#else
#define DBGPR_MDIO(x...) do {} while (0)
#endif

struct nvt_mdio_priv {
	void __iomem *base;
};
void __iomem *BASE_ADDRESS;
/*!
* \brief read MII PHY register, function called by the driver alone
*
* \details Read MII registers through the API read_phy_reg where the
* related MAC registers can be configured.
*
* \param[in] pdata - pointer to driver private data structure.
* \param[in] phyaddr - the phy address to read
* \param[in] phyreg - the phy regiester id to read
* \param[out] phydata - pointer to the value that is read from the phy registers
*
* \return int
*
* \retval  0 - successfully read data from register
* \retval -1 - error occurred
* \retval  1 - if the feature is not defined.
*/

static u8 mdc_div = 1;

INT DWC_ETH_QOS_mdio_read_direct( int phyaddr, int phyreg, int *phydata)
{

	ULONG retryCount = 1000;
	ULONG vy_count;
	volatile ULONG varMAC_GMIIAR;
	ULONG varMAC_GMIIDR;

	//printk("--> DWC_ETH_QOS_mdio_read: phyaddr = %d, phyreg = %d\n",
	      //phyaddr, phyreg);

	/* wait for any previous MII read/write operation to complete */

	/*Poll Until Poll Condition */
	vy_count = 0;
	while (1) {
		if (vy_count > retryCount) {
			return -Y_FAILURE;
		} else {
			vy_count++;
			mdelay(1);
		}
		MAC_GMIIAR_RgRd(varMAC_GMIIAR);
		if (GET_VALUE(varMAC_GMIIAR, MAC_GMIIAR_GB_LPOS, MAC_GMIIAR_GB_HPOS) == 0) {
			break;
		}
	}
	/* initiate the MII read operation by updating desired */
	/* phy address/id (0 - 31) */
	/* phy register offset */
	/* CSR Clock Range (20 - 35MHz) */
	/* Select read operation */
	/* set busy bit */
	MAC_GMIIAR_RgRd(varMAC_GMIIAR);
	varMAC_GMIIAR = varMAC_GMIIAR & (ULONG) (0x12);

	varMAC_GMIIAR =
	    varMAC_GMIIAR | ((phyaddr) << 21) | ((phyreg) << 16) | (mdc_div << 8)
	    | ((0x3) << 2) | ((0x1) << 0);

    MAC_GMIIAR_RgWr(varMAC_GMIIAR);

	/*DELAY IMPLEMENTATION USING udelay() */
	udelay(10);
	/* wait for MII write operation to complete */

	/*Poll Until Poll Condition */
	vy_count = 0;
	while (1) {
		if (vy_count > retryCount) {
			return -Y_FAILURE;
		} else {
			vy_count++;
			mdelay(1);
		}
		MAC_GMIIAR_RgRd(varMAC_GMIIAR);
		if (GET_VALUE(varMAC_GMIIAR, MAC_GMIIAR_GB_LPOS, MAC_GMIIAR_GB_HPOS) == 0) {
			break;
		}
	}
	/* read the data */
	MAC_GMIIDR_RgRd(varMAC_GMIIDR);
	*phydata =
	    GET_VALUE(varMAC_GMIIDR, MAC_GMIIDR_GD_LPOS, MAC_GMIIDR_GD_HPOS);

	return Y_SUCCESS;
}

/*!
* \brief write MII PHY register, function called by the driver alone
*
* \details Writes MII registers through the API write_phy_reg where the
* related MAC registers can be configured.
*
* \param[in] pdata - pointer to driver private data structure.
* \param[in] phyaddr - the phy address to write
* \param[in] phyreg - the phy regiester id to write
* \param[out] phydata - actual data to be written into the phy registers
*
* \return void
*
* \retval  0 - successfully read data from register
* \retval -1 - error occurred
* \retval  1 - if the feature is not defined.
*/

INT DWC_ETH_QOS_mdio_write_direct( int phyaddr, int phyreg, int phydata)
{
	ULONG retryCount = 1000;
	ULONG vy_count;
	volatile ULONG varMAC_GMIIAR;

	DBGPR_MDIO("--> DWC_ETH_QOS_mdio_write_direct\n");

	/* wait for any previous MII read/write operation to complete */


	/*Poll Until Poll Condition */
	vy_count = 0;
	while (1) {
		if (vy_count > retryCount) {
			printk(KERN_ALERT "not defined");
			DBGPR_MDIO("<-- DWC_ETH_QOS_mdio_write\n");
			return -Y_FAILURE;
		} else {
			vy_count++;
			mdelay(1);
		}
		MAC_GMIIAR_RgRd(varMAC_GMIIAR);
		if (GET_VALUE(varMAC_GMIIAR, MAC_GMIIAR_GB_LPOS, MAC_GMIIAR_GB_HPOS) == 0) {
			break;
		}
	}
	/* write the data */
	MAC_GMIIDR_GD_UdfWr(phydata);
	/* initiate the MII write operation by updating desired */
	/* phy address/id (0 - 31) */
	/* phy register offset */
	/* CSR Clock Range (20 - 35MHz) */
	/* Select write operation */
	/* set busy bit */
	MAC_GMIIAR_RgRd(varMAC_GMIIAR);
	varMAC_GMIIAR = varMAC_GMIIAR & (ULONG) (0x12);

	varMAC_GMIIAR =
	    varMAC_GMIIAR | ((phyaddr) << 21) | ((phyreg) << 16) | (mdc_div << 8)
	    | ((0x1) << 2) | ((0x1) << 0);

	MAC_GMIIAR_RgWr(varMAC_GMIIAR);

	/*DELAY IMPLEMENTATION USING udelay() */
	udelay(10);
	/* wait for MII write operation to complete */

	/*Poll Until Poll Condition */
	vy_count = 0;
	while (1) {
		if (vy_count > retryCount) {
			printk(KERN_ALERT "not defined");
			DBGPR_MDIO("<-- DWC_ETH_QOS_mdio_write\n");
			return -Y_FAILURE;
		} else {
			vy_count++;
			mdelay(1);
		}
		MAC_GMIIAR_RgRd(varMAC_GMIIAR);
		if (GET_VALUE(varMAC_GMIIAR, MAC_GMIIAR_GB_LPOS, MAC_GMIIAR_GB_HPOS) == 0) {
			break;
		}
	}
	DBGPR_MDIO("<-- DWC_ETH_QOS_mdio_write_direct\n");

	return Y_SUCCESS;
}

/*!
* \brief read MII PHY register.
*
* \details Read MII registers through the API read_phy_reg where the
* related MAC registers can be configured.
*
* \param[in] bus - points to the mii_bus structure
* \param[in] phyaddr - the phy address to write
* \param[in] phyreg - the phy register offset to write
*
* \return int
*
* \retval  - value read from given phy register
*/

static INT DWC_ETH_QOS_mdio_read(struct mii_bus *bus, int phyaddr, int phyreg)
{

	ULONG retryCount = 1000;
	ULONG vy_count;
	volatile ULONG varMAC_GMIIAR;
	ULONG varMAC_GMIIDR;

	int phydata;

	DBGPR_MDIO("--> DWC_ETH_QOS_mdio_read: phyaddr = %d, phyreg = %d\n",
	      phyaddr, phyreg);

	/* wait for any previous MII read/write operation to complete */

	/*Poll Until Poll Condition */
	vy_count = 0;
	while (1) {
		if (vy_count > retryCount) {
			return phydata;
		} else {
			vy_count++;
			mdelay(1);
		}
		MAC_GMIIAR_RgRd(varMAC_GMIIAR);
		if (GET_VALUE(varMAC_GMIIAR, MAC_GMIIAR_GB_LPOS, MAC_GMIIAR_GB_HPOS) == 0) {
			break;
		}
	}
	/* initiate the MII read operation by updating desired */
	/* phy address/id (0 - 31) */
	/* phy register offset */
	/* CSR Clock Range (20 - 35MHz) */
	/* Select read operation */
	/* set busy bit */
	MAC_GMIIAR_RgRd(varMAC_GMIIAR);
	varMAC_GMIIAR = varMAC_GMIIAR & (ULONG) (0x12);

	varMAC_GMIIAR =
	    varMAC_GMIIAR | ((phyaddr) << 21) | ((phyreg) << 16) | (mdc_div << 8)
	    | ((0x3) << 2) | ((0x1) << 0);

    MAC_GMIIAR_RgWr(varMAC_GMIIAR);

	/*DELAY IMPLEMENTATION USING udelay() */
	udelay(10);
	/* wait for MII write operation to complete */

	/*Poll Until Poll Condition */
	vy_count = 0;
	while (1) {
		if (vy_count > retryCount) {
			return phydata;
		} else {
			vy_count++;
			mdelay(1);
		}
		MAC_GMIIAR_RgRd(varMAC_GMIIAR);
		if (GET_VALUE(varMAC_GMIIAR, MAC_GMIIAR_GB_LPOS, MAC_GMIIAR_GB_HPOS) == 0) {
			break;
		}
	}
	/* read the data */
	MAC_GMIIDR_RgRd(varMAC_GMIIDR);
	phydata =
	    GET_VALUE(varMAC_GMIIDR, MAC_GMIIDR_GD_LPOS, MAC_GMIIDR_GD_HPOS);

	//return Y_SUCCESS;


	DBGPR_MDIO("<-- DWC_ETH_QOS_mdio_read: phydata = %#x\n", phydata);

	return phydata;
}

/*
* \brief API to write MII PHY register
*
* \details This API is expected to write MII registers with the value being
* passed as the last argument which is done in write_phy_regs API
* called by this function.
*
* \param[in] bus - points to the mii_bus structure
* \param[in] phyaddr - the phy address to write
* \param[in] phyreg - the phy register offset to write
* \param[in] phydata - the register value to write with
*
* \return 0 on success and -ve number on failure.
*/

static INT DWC_ETH_QOS_mdio_write(struct mii_bus *bus, int phyaddr, int phyreg,
				  u16 phydata)
{
	ULONG retryCount = 1000;
	ULONG vy_count;
	volatile ULONG varMAC_GMIIAR;

	INT ret = Y_SUCCESS;

	DBGPR_MDIO("--> DWC_ETH_QOS_mdio_write\n");

	/* wait for any previous MII read/write operation to complete */


	/*Poll Until Poll Condition */
	vy_count = 0;
	while (1) {
		if (vy_count > retryCount) {
			printk(KERN_ALERT "not defined");
			DBGPR_MDIO("<-- DWC_ETH_QOS_mdio_write\n");
			return phydata;
		} else {
			vy_count++;
			mdelay(1);
		}
		MAC_GMIIAR_RgRd(varMAC_GMIIAR);
		if (GET_VALUE(varMAC_GMIIAR, MAC_GMIIAR_GB_LPOS, MAC_GMIIAR_GB_HPOS) == 0) {
			break;
		}
	}
	/* write the data */
	MAC_GMIIDR_GD_UdfWr(phydata);
	/* initiate the MII write operation by updating desired */
	/* phy address/id (0 - 31) */
	/* phy register offset */
	/* CSR Clock Range (20 - 35MHz) */
	/* Select write operation */
	/* set busy bit */
	MAC_GMIIAR_RgRd(varMAC_GMIIAR);
	varMAC_GMIIAR = varMAC_GMIIAR & (ULONG) (0x12);

	varMAC_GMIIAR =
	    varMAC_GMIIAR | ((phyaddr) << 21) | ((phyreg) << 16) | (mdc_div << 8)
	    | ((0x1) << 2) | ((0x1) << 0);

	MAC_GMIIAR_RgWr(varMAC_GMIIAR);

	/*DELAY IMPLEMENTATION USING udelay() */
	udelay(10);
	/* wait for MII write operation to complete */

	/*Poll Until Poll Condition */
	vy_count = 0;
	while (1) {
		if (vy_count > retryCount) {
			printk(KERN_ALERT "not defined");
			DBGPR_MDIO("<-- DWC_ETH_QOS_mdio_write\n");
			return phydata;
		} else {
			vy_count++;
			mdelay(1);
		}
		MAC_GMIIAR_RgRd(varMAC_GMIIAR);
		if (GET_VALUE(varMAC_GMIIAR, MAC_GMIIAR_GB_LPOS, MAC_GMIIAR_GB_HPOS) == 0) {
			break;
		}
	}


	DBGPR_MDIO("<-- DWC_ETH_QOS_mdio_write\n");

	return ret;
}


/*!
* \brief API to reset PHY
*
* \details This API is issue soft reset to PHY core and waits
* until soft reset completes.
*
* \param[in] bus - points to the mii_bus structure
*
* \return 0 on success and -ve number on failure.
*/

static INT DWC_ETH_QOS_mdio_reset(struct mii_bus *bus)
{
	INT phydata;
	int i;
	for (i = 0; i < PHY_MAX_ADDR; i++) {
	DBGPR_MDIO("-->DWC_ETH_QOS_mdio_reset: phyaddr : %d\n", i);
	#if 0 //def DWC_ETH_QOS_CONFIG_PGTEST
		printk(KERN_ALERT "PHY Programming for Autoneg disable\n");
		DWC_ETH_QOS_mdio_read_direct( i, MII_BMCR, &phydata)
		phydata &= ~(1 << 12);
		DWC_ETH_QOS_mdio_write_direct( i, MII_BMCR, phydata);
	#endif

		DWC_ETH_QOS_mdio_read_direct( i, MII_BMCR, &phydata);

		if (phydata < 0)
			return 0;

		/* issue soft reset to PHY */
		phydata |= BMCR_RESET;
		DWC_ETH_QOS_mdio_write_direct( i, MII_BMCR, phydata);

		/* wait until software reset completes */
		/*do {
			DWC_ETH_QOS_mdio_read_direct(i, MII_BMCR, &phydata);
		} while ((phydata >= 0) && (phydata & BMCR_RESET));*/

	#if 0 //def DWC_ETH_QOS_CONFIG_PGTEST
		printk(KERN_ALERT "PHY Programming for Loopback\n");
		DWC_ETH_QOS_mdio_read_direct( i, MII_BMCR, &phydata);
		phydata |= (1 << 14);
		DWC_ETH_QOS_mdio_write_direct( i, MII_BMCR, phydata);
	#endif
	}
	mdelay(500);
	DBGPR_MDIO("<--DWC_ETH_QOS_mdio_reset\n");

	return 0;
}



/*
* \brief API to register mdio.
*
* \details This function will allocate mdio bus and register it
* phy layer.
*
* \param[in] dev - pointer to net_device structure
*
* \return 0 on success and -ve on failure.
*/
int DWC_ETH_QOS_mdio_probe(struct  platform_device *pdev)
{
	struct nvt_mdio_priv *priv;
	struct mii_bus *new_bus = NULL;
	struct resource *r;
	int bus_id;
	int ret;

	DBGPR_MDIO("-->DWC_ETH_QOS_mdio_register\n");

	bus_id = 0x1;
	new_bus = mdiobus_alloc_size(sizeof(struct nvt_mdio_priv));
	if (new_bus == NULL) {
		printk(KERN_ALERT "Unable to allocate mdio bus\n");
		return -ENOMEM;
	}

	new_bus->name = "dwc_phy";
	new_bus->read = DWC_ETH_QOS_mdio_read;
	new_bus->write = DWC_ETH_QOS_mdio_write;
	new_bus->reset = DWC_ETH_QOS_mdio_reset;
	snprintf(new_bus->id, MII_BUS_ID_SIZE, "%s-%x", new_bus->name,
		 bus_id);
	new_bus->parent = &pdev->dev;
	//new_bus->priv = dev;
	priv = new_bus->priv;
	new_bus->phy_mask = 0;

	new_bus->dev.of_node = pdev->dev.of_node;

	BASE_ADDRESS = (void __iomem *)(0xFD2B0200);

	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	BASE_ADDRESS = devm_ioremap_resource(&pdev->dev, r);

	if (IS_ERR(BASE_ADDRESS)) {
		ret = PTR_ERR(BASE_ADDRESS);
		goto err_out_phy_connect;
	}

	priv->base = BASE_ADDRESS;
	//ret = of_mdiobus_register(new_bus, pdev->dev.of_node);
	ret = mdiobus_register(new_bus);

	if (ret < 0) {
		printk(KERN_ALERT "%s: Cannot register as MDIO bus\n",
		    new_bus->name);
		mdiobus_free(new_bus);
		return ret;
	}

	platform_set_drvdata(pdev, new_bus);

	if (unlikely(ret)) {
		printk(KERN_ALERT "Cannot attach to PHY (error: %d)\n", ret);
		goto err_out_phy_connect;
	}

	DBGPR_MDIO("<--DWC_ETH_QOS_mdio_register\n");

	return ret;

 err_out_phy_connect:
	mdiobus_free(new_bus);
	return ret;
}

/*!
* \brief API to unregister mdio.
*
* \details This function will unregister mdio bus and free's the memory
* allocated to it.
*
* \param[in] dev - pointer to net_device structure
*
* \return void
*/

static int DWC_ETH_QOS_mdio_remove(struct platform_device *pdev)
{
	struct mii_bus *bus = platform_get_drvdata(pdev);

	DBGPR_MDIO("-->DWC_ETH_QOS_mdio_remove\n");

	mdiobus_unregister(bus);
	mdiobus_free(bus);

	DBGPR_MDIO("<--DWC_ETH_QOS_mdio_remove\n");

	return 0;
}

static const struct of_device_id DWC_ETH_QOS_mdio_match[] = {
	{ .compatible = "nvt,eth_mdio" },
	{ }
};
MODULE_DEVICE_TABLE(of, DWC_ETH_QOS_mdio_match);

static struct platform_driver DWC_ETH_QOS_mdio_driver = {
	.probe = DWC_ETH_QOS_mdio_probe,
	.remove = DWC_ETH_QOS_mdio_remove,
	.driver = {
		.name = "DWC_ETH_QOS_mdio",
		.owner = THIS_MODULE,
		.of_match_table = DWC_ETH_QOS_mdio_match,
	},
};

module_platform_driver(DWC_ETH_QOS_mdio_driver);

MODULE_DESCRIPTION("DWC_ETH_QOS MDIO interface driver");
MODULE_VERSION("1.00.000");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:DWC_ETH_QOS_mdio");
