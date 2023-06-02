/*
* linux/drivers/usb/host/ohci-ambarella.c
* driver for Full speed (USB1.1) USB host controller on Ambarella processors
*
* History:
*	2010/08/11 - [Cao Rongrong] created file
*
* Copyright (C) 2008 by Ambarella, Inc.
* http://www.ambarella.com
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the
* Free Software Foundation, Inc., 59 Temple Place - Suite 330,
* Boston, MA  02111-1307, USA.
*/

#include <linux/platform_device.h>
#include <linux/of.h>
#include <mach/hardware.h>

extern int usb_disabled(void);

struct ohci_ambarella {
	struct ohci_hcd ohci;
	struct usb_phy *phy;
	u32 nports;
};


static struct ohci_ambarella *hcd_to_ohci_ambarella(struct usb_hcd *hcd)
{
	struct ohci_hcd *ohci = hcd_to_ohci(hcd);

	return container_of(ohci, struct ohci_ambarella, ohci);
}

static int ohci_ambarella_start(struct usb_hcd *hcd)
{
	struct ohci_hcd	*ohci = hcd_to_ohci(hcd);
	struct ohci_ambarella *amb_ohci = hcd_to_ohci_ambarella(hcd);
	int ret;

	/* OHCI will still detect 2 ports even though usb port1 is configured
	 * as device port, so we fake the port number manually and report
	 * it to OHCI.*/
	ohci->num_ports = amb_ohci->nports;

	if ((ret = ohci_init(ohci)) < 0)
		return ret;

	if ((ret = ohci_run(ohci)) < 0) {
		dev_err(hcd->self.controller, "can't start\n");
		ohci_stop(hcd);
		return ret;
	}

	return 0;
}

static const struct hc_driver ohci_ambarella_hc_driver = {
	.description =		hcd_name,
	.product_desc =		"Ambarella OHCI",
	.hcd_priv_size =	sizeof(struct ohci_ambarella),

	/*
	 * generic hardware linkage
	 */
	.irq =			ohci_irq,
	.flags =		HCD_USB11 | HCD_MEMORY,

	/*
	 * basic lifecycle operations
	 */
	.start =		ohci_ambarella_start,
	.stop =			ohci_stop,
	.shutdown =		ohci_shutdown,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue =		ohci_urb_enqueue,
	.urb_dequeue =		ohci_urb_dequeue,
	.endpoint_disable =	ohci_endpoint_disable,

	/*
	 * scheduling support
	 */
	.get_frame_number =	ohci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data =	ohci_hub_status_data,
	.hub_control =		ohci_hub_control,
#ifdef	CONFIG_PM
	.bus_suspend =		ohci_bus_suspend,
	.bus_resume =		ohci_bus_resume,
#endif
	.start_port_reset =	ohci_start_port_reset,
};

static int ohci_hcd_ambarella_drv_probe(struct platform_device *pdev)
{
	struct ohci_ambarella *amb_ohci;
	struct usb_hcd *hcd;
	struct resource *res;
	struct usb_phy *phy;
	int irq, ret;

	if (usb_disabled())
		return -ENODEV;

	/* Right now device-tree probed devices don't get dma_mask set.
	 * Since shared usb code relies on it, set it here for now.
	 * Once we have dma capability bindings this can go away.
	 */
	if (!pdev->dev.dma_mask)
		pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;
	if (!pdev->dev.coherent_dma_mask)
		pdev->dev.coherent_dma_mask = DMA_BIT_MASK(32);

	hcd = usb_create_hcd(&ohci_ambarella_hc_driver, &pdev->dev, "AmbUSB");
	if (!hcd)
		return -ENOMEM;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(&pdev->dev, "Unable to get IRQ resource\n");
		ret = irq;
		goto amb_ohci_err;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "Unable to get memory resource\n");
		ret = -ENODEV;
		goto amb_ohci_err;
	}

	hcd->rsrc_start = res->start;
	hcd->rsrc_len = resource_size(res);
	hcd->regs = devm_ioremap(&pdev->dev, hcd->rsrc_start, hcd->rsrc_len);
	if (!hcd->regs) {
		dev_err(&pdev->dev, "ioremap failed\n");
		ret = -ENOMEM;
		goto amb_ohci_err;
	}

	amb_ohci = hcd_to_ohci_ambarella(hcd);

	/* get the PHY device */
	phy = devm_usb_get_phy_by_phandle(&pdev->dev, "amb,usbphy", 0);
	if (IS_ERR(phy)) {
		ret = PTR_ERR(phy);
		dev_err(&pdev->dev, "Can't get USB PHY %d\n", ret);
		goto amb_ohci_err;
	}

	ret = of_property_read_u32(phy->dev->of_node,
				"amb,host-phy-num", &amb_ohci->nports);
	if (ret < 0){
		dev_err(&pdev->dev, "Can't get host phy num %d\n", ret);
		goto amb_ohci_err;
	}

	usb_phy_init(phy);
	amb_ohci->phy = phy;

	ohci_hcd_init(hcd_to_ohci(hcd));

	ret = usb_add_hcd(hcd, irq, IRQF_TRIGGER_HIGH);
	if (ret < 0)
		goto amb_ohci_err;

	platform_set_drvdata(pdev, hcd);

	return 0;

amb_ohci_err:
	usb_put_hcd(hcd);
	return ret;
}

static int ohci_hcd_ambarella_drv_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);

	usb_remove_hcd(hcd);
	usb_put_hcd(hcd);

	return 0;
}

#ifdef CONFIG_PM

/* Maybe just need to suspend/resume controller via HAL function. */
static int ohci_hcd_ambarella_drv_suspend(struct device *dev)
{
	struct usb_hcd *hcd = dev_get_drvdata(dev);
	struct ohci_hcd *ohci = hcd_to_ohci(hcd);

	if (time_before(jiffies, ohci->next_statechange))
		msleep(5);
	ohci->next_statechange = jiffies;

	return 0;
}

static int ohci_hcd_ambarella_drv_resume(struct device *dev)
{
	struct usb_hcd *hcd = dev_get_drvdata(dev);
	struct ohci_hcd *ohci = hcd_to_ohci(hcd);

	if (time_before(jiffies, ohci->next_statechange))
		msleep(5);
	ohci->next_statechange = jiffies;

	ohci_resume(hcd, false);

	return 0;
}

static struct dev_pm_ops ambarella_ohci_pmops = {
	.suspend	= ohci_hcd_ambarella_drv_suspend,
	.resume		= ohci_hcd_ambarella_drv_resume,
	.freeze		= ohci_hcd_ambarella_drv_suspend,
	.thaw		= ohci_hcd_ambarella_drv_resume,
};

#define AMBARELLA_OHCI_PMOPS &ambarella_ohci_pmops

#else
#define AMBARELLA_OHCI_PMOPS NULL
#endif

static const struct of_device_id ambarella_ohci_dt_ids[] = {
	{ .compatible = "ambarella,ohci", },
	{},
};
MODULE_DEVICE_TABLE(of, ambarella_ohci_dt_ids);

static struct platform_driver ohci_hcd_ambarella_driver = {
	.probe		= ohci_hcd_ambarella_drv_probe,
	.remove		= ohci_hcd_ambarella_drv_remove,
	.shutdown	= usb_hcd_platform_shutdown,
	.driver		= {
		.name	= "ambarella-ohci",
		.owner	= THIS_MODULE,
		.pm	= AMBARELLA_OHCI_PMOPS,
		.of_match_table = of_match_ptr(ambarella_ohci_dt_ids),
	},
};

MODULE_ALIAS("platform:ambarella-ohci");
