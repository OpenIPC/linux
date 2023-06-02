/*
 * Klins Chen <klins_chen@novatek.com.tw>
 *
 */

#include "na51000.h"
#include <linux/clk.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/of_address.h>
//#include <plat/efuse_protected.h>

#define USB_SIMULATE_POWEROFF 0

#ifdef CONFIG_DMA_COHERENT
#define USBH_ENABLE_INIT  (USBH_ENABLE_CE \
			| USB_MCFG_PFEN | USB_MCFG_RDCOMB \
			| USB_MCFG_SSDEN | USB_MCFG_UCAM \
			| USB_MCFG_EBMEN | USB_MCFG_EMEMEN)
#else
#define USBH_ENABLE_INIT  (USBH_ENABLE_CE \
			| USB_MCFG_PFEN | USB_MCFG_RDCOMB \
			| USB_MCFG_SSDEN \
			| USB_MCFG_EBMEN | USB_MCFG_EMEMEN)
#endif

#if 0
static void u2phy_setreg(struct ehci_hcd *ehci, u32 ofs, u32 val)
{
	writel(val, (void __iomem *)(ehci->phy_reg + ofs));
}

static u32 u2phy_getreg(struct ehci_hcd *ehci, u32 ofs)
{
	return readl((void __iomem *)(ehci->phy_reg + ofs));
}
#endif
/*
#ifdef CONFIG_PM
static void nvtim_plat_resume(struct usb_hcd *hcd)
{
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	int retval, retry = 0;

	ehci_writel(ehci, ehci->periodic_dma, &ehci->regs->frame_list);
	ehci_writel(ehci, (u32)ehci->async->qh_dma, &ehci->regs->async_next);
	retry = 0;

	do {
		ehci_writel(ehci, INTR_MASK,
			&ehci->regs->intr_enable);

		retval = ehci_handshake(ehci, &ehci->regs->intr_enable,
				INTR_MASK, INTR_MASK, 250);
		retry++;
	} while (retval != 0);

	if (unlikely(retval != 0))
		ehci_err(ehci, "write fail!\n");
}
#endif
*/

static void nvtim_init_usbhc(struct platform_device *pdev)
{
	struct usb_hcd	*hcd = platform_get_drvdata(pdev);
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	u32		usbbase;
	u32		tmpval = 0;

	usbbase = (u32)ehci->caps;

	/* Set INT_POLARITY */
	tmpval = readl((volatile unsigned long *)(usbbase+0xC4));
	tmpval = 0xB;
	writel(tmpval, (volatile unsigned long *)(usbbase+0xC4));

	/* Clear DEVPHY_SUSPEND[5] */
	tmpval = readl((volatile unsigned long *)(usbbase+0x1C8));
	tmpval &= ~(0x1<<31);
	writel(tmpval, (volatile unsigned long *)(usbbase+0x1C8));

	/* Set USB ID & VBUSI */
	tmpval = readl((volatile unsigned long *)(usbbase+0x1D8));
	tmpval &= ~0xFF;
	tmpval |= 0x22;
	tmpval |= (0x1<<20);
	tmpval &= ~(0x1<<21);
	writel(tmpval, (volatile unsigned long *)(usbbase+0x1D8));

	/* Clear FORCE_FS[9] and handle HALF_SPEED[1] */
	tmpval = readl((volatile unsigned long *)(usbbase+0x100));
	tmpval &= ~(0x1<<9);
	#ifdef CONFIG_FPGA_EMULATION
	tmpval |=  (0x1<<1);
	#endif
	writel(tmpval, (volatile unsigned long *)(usbbase+0x100));

	/* Clear HOSTPHY_SUSPEND[6] */
	tmpval = readl((volatile unsigned long *)(usbbase+0x40));
	tmpval &= ~(0x3F << 16);
	tmpval |=  (0x22 << 16);
	tmpval &= ~(0x1<<6);
	writel(tmpval, (volatile unsigned long *)(usbbase+0x40));

	/* Clear EOF1=3[3:2] EOF2[5:4]=0 */
	tmpval = readl((volatile unsigned long *)(usbbase+0x40));
	tmpval &= ~(0x3<<4);
	tmpval |=  (0x3<<2);
	tmpval &= ~(0x3F<<8);
	tmpval |=  (0x22<<8);
	writel(tmpval, (volatile unsigned long *)(usbbase+0x40));

#if USB_SIMULATE_POWEROFF
	printk("R0x40=0x%08X\r\n",readl((volatile unsigned long *)(usbbase+0x40)));
#endif

	/* A_BUS_DROP[5] = 0 */
	tmpval = readl((volatile unsigned long *)(usbbase+0x80));
	tmpval &= ~(0x1<<5);
	writel(tmpval, (volatile unsigned long *)(usbbase+0x80));

	udelay(2);

	/* A_BUS_REQ[4] = 1 */
	tmpval = readl((volatile unsigned long *)(usbbase+0x80));
	tmpval |= (0x1<<4);
	writel(tmpval, (volatile unsigned long *)(usbbase+0x80));

#if 0
	/* Configure PHY related settings below */
	{
		UINT16 data=0;
		INT32 result=0;
		UINT32 temp;
		UINT8 u2_trim_swctrl=6, u2_trim_sqsel=4, u2_trim_resint=8;

		result= efuse_readParamOps(EFUSE_USBC_TRIM_DATA, &data);
		if(result == 0) {
			u2_trim_swctrl = data&0x7;
			u2_trim_sqsel  = (data>>3)&0x7;
			u2_trim_resint = (data>>6)&0x1F;
		}

		u2phy_setreg(0x51, 0x20);
		u2phy_setreg(0x50, 0x30);

		temp = u2phy_getreg(0x06);
		temp &= ~(0x7<<1);
		temp |= (u2_trim_swctrl<<1);
		u2phy_setreg(0x06, temp);

		temp = u2phy_getreg(0x05);
		temp &= ~(0x7<<2);
		temp |= (u2_trim_sqsel<<2);
		u2phy_setreg(0x05, temp);

		u2phy_setreg(0x52, 0x60+u2_trim_resint);
		u2phy_setreg(0x51, 0x00);

		writel(0x100+u2_trim_resint, (volatile unsigned long *)(usbbase+0x30C));
	}
#endif
}

static void nvtim_stop_usbhc(struct platform_device *pdev)
{
	struct usb_hcd	*hcd = platform_get_drvdata(pdev);
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	u32		usbbase;
	u32		tmpval = 0;
	struct device   *dev = &pdev->dev;

	usbbase = (u32)ehci->caps;

	/* Host Reset */
	writel((readl((volatile unsigned long *)(usbbase+0x10)) | 0x2), (volatile unsigned long *)(usbbase+0x10));

	/* A_BUS_REQ[4] = 0 */
	tmpval = readl((volatile unsigned long *)(usbbase+0x80));
	tmpval &= ~(0x1<<4);
	writel(tmpval, (volatile unsigned long *)(usbbase+0x80));

	/* A_BUS_DROP[5] = 1 */
	tmpval = readl((volatile unsigned long *)(usbbase+0x80));
	tmpval |= (0x1<<5);
	writel(tmpval, (volatile unsigned long *)(usbbase+0x80));

	{
		struct clk *usbclk;

		usbclk = clk_get(dev, dev_name(dev));
		if (!IS_ERR(usbclk)) {
			clk_prepare(usbclk);
			clk_unprepare(usbclk);
			clk_put(usbclk);
		} else {
			pr_err("%s: %s not found\n", __func__, dev_name(&pdev->dev));
		}
	}

}

static void nvtim_fuse_log(struct ehci_hcd *ehci)
{
#if 0
	u32 fusedata=0;
	u32	usbbase;

	usbbase = (u32)ehci->caps;

	fusedata += ((readl((volatile unsigned long *)(usbbase+(0x1000+(0x6<<2)))) & 0xFF) << 8);
	fusedata += ((readl((volatile unsigned long *)(usbbase+(0x1000+(0x5<<2)))) & 0xFF) << 16);
	ehci->fuseData = fusedata;
	//printk("ehci->fuseData = 0x%08X\n",ehci->fuseData);
#endif
}

/* configure so an HC device and id are always provided */
/* always called with process context; sleeping is OK */

/**
 * nvtim_usb_ehci_probe - initialize nt96660-based HCDs
 * Context: !in_interrupt()
 *
 * Allocates basic resources for this USB host controller, and
 * then invokes the start() method for the HCD associated with it
 * through the hotplug entry's driver_data.
 *
 */
static int nvtim_usb_ehci_probe(const struct hc_driver *driver,
			struct usb_hcd **hcd_out, struct platform_device *pdev)
{
	struct device_node *dn = pdev->dev.of_node;
	struct usb_hcd *hcd;
	struct ehci_hcd *ehci;
	struct resource res_mem;
	int retval;
	int irq;
	struct device *dev = &pdev->dev;
	struct clk *phyclk, *usbclk;
	u32 channel, phy_reg;

#ifdef CONFIG_USE_OF
	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		pr_debug("resource[1] is not IORESOURCE_IRQ");
		return -ENXIO;
	}

	of_property_read_u32(pdev->dev.of_node, "phy_reg",
				&phy_reg);

	of_property_read_u32(pdev->dev.of_node, "channel",
				&channel);
#else
	if (pdev->resource[1].flags != IORESOURCE_IRQ) {
		pr_debug("resource[1] is not IORESOURCE_IRQ");
		retval = -ENOMEM;
		return retval;
	}
	irq = pdev->resource[1].start;
#endif

	retval = of_address_to_resource(dn, 0, &res_mem);
	if (retval) {
		dev_err(&pdev->dev, "no address resource provided for index 0");
		return retval;
	}

	hcd = usb_create_hcd(driver, &pdev->dev, driver->description);
	if (!hcd)
		return -ENOMEM;


	hcd->product_desc = driver->product_desc;
	hcd->rsrc_start = res_mem.start;
	hcd->rsrc_len = resource_size(&res_mem);

	hcd->regs = devm_ioremap_resource(&pdev->dev, &res_mem);
	if (!hcd->regs) {
		pr_debug("ioremap failed");
		retval = -ENOMEM;
		goto err_register;
	}

	/* This part is used in debug usage for using real usb addr. */
	/* hcd->regs =(void __iomem*) hcd->rsrc_start;
	 * if (!hcd->regs) {
	 *	pr_debug("ioremap failed");
	 *	retval = -ENOMEM;
	 *	goto err_register;
	 * }
	 */

	if (channel)
		phyclk = clk_get(NULL, "u2host_phy_clk1");
	else
		phyclk = clk_get(NULL, "u2host_phy_clk0");

	if (!IS_ERR(phyclk)) {
		clk_prepare(phyclk);
		clk_enable(phyclk);
	} else {
		pr_err("%s: otgphy_clk not found\n", __func__);
	}

	usbclk = clk_get(dev, dev_name(dev));
	if (!IS_ERR(usbclk)) {
		clk_prepare(usbclk);
		clk_enable(usbclk);
	} else {
		pr_err("%s: %s not found\n", __func__, dev_name(&pdev->dev));
	}

	ehci = hcd_to_ehci(hcd);
	ehci->caps = hcd->regs;
	ehci->regs = (void __iomem *)ehci->caps +
	HC_LENGTH(ehci, readl((volatile unsigned long *)(&ehci->caps->hc_capbase)));

	ehci->sbrn = HCD_USB2;

	/* cache this readonly data; minimize chip reads */
	ehci->hcs_params = ehci_readl(ehci, &ehci->caps->hcs_params);

	ehci->phy_reg = phy_reg;

	nvtim_fuse_log(ehci);
	nvtim_init_usbhc(pdev);

	retval = usb_add_hcd(hcd, irq, IRQF_SHARED);
	if (retval == 0) {
		platform_set_drvdata(pdev, hcd);
		return retval;
	}

	nvtim_stop_usbhc(pdev);

err_register:
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
	usb_put_hcd(hcd);

	return retval;
}

/* may be called without controller electrically present */
/* may be called with controller, bus, and devices active */
static void nvtim_usb_ehci_remove(struct usb_hcd *hcd,
				struct platform_device *dev)
{
	usb_remove_hcd(hcd);
	//release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
	pr_debug("calling usb_put_hcd\n");
	usb_put_hcd(hcd);
	nvtim_stop_usbhc(dev);
}

static int nvtim_ehci_init(struct usb_hcd *hcd)
{
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	u32 temp;
	int retval;
	u32 hcc_params;
	struct ehci_qh_hw   *hw;

	spin_lock_init(&ehci->lock);

	ehci->need_io_watchdog = 1;

	hrtimer_init(&ehci->hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_ABS);
	ehci->hrtimer.function = ehci_hrtimer_func;
	ehci->next_hrtimer_event = EHCI_HRTIMER_NO_EVENT;

	hcc_params = ehci_readl(ehci, &ehci->caps->hcc_params);

	ehci->uframe_periodic_max = 100;

	/*
	 * hw default: 1K periodic list heads, one per frame.
	 * periodic_size can shrink by USBCMD update if hcc_params allows.
	 */
	ehci->periodic_size = DEFAULT_I_TDPS;

	INIT_LIST_HEAD(&ehci->async_unlink);
	INIT_LIST_HEAD(&ehci->async_idle);
	INIT_LIST_HEAD(&ehci->intr_unlink);
	INIT_LIST_HEAD(&ehci->intr_unlink_wait);
	INIT_LIST_HEAD(&ehci->intr_qh_list);
	INIT_LIST_HEAD(&ehci->cached_itd_list);
	INIT_LIST_HEAD(&ehci->cached_sitd_list);
	INIT_LIST_HEAD(&ehci->tt_list);

	retval = ehci_mem_init(ehci, GFP_KERNEL);
	if (retval < 0)
		return retval;

	/* controllers may cache some of the periodic schedule ... */
	hcc_params = ehci_readl(ehci, &ehci->caps->hcc_params);
	if (HCC_ISOC_CACHE(hcc_params))	/* full frame cache */
		ehci->i_thresh = 8;
	else				/* N microframes cached */
		ehci->i_thresh = 2 + HCC_ISOC_THRES(hcc_params);

	/*
	 * dedicate a qh for the async ring head, since we couldn't unlink
	 * a 'real' qh without stopping the async schedule [4.8].  use it
	 * as the 'reclamation list head' too.
	 * its dummy is used in hw_alt_next of many tds, to prevent the qh
	 * from automatically advancing to the next td after short reads.
	 */
	ehci->async->qh_next.qh = NULL;
	hw = ehci->async->hw;
	hw->hw_next = QH_NEXT(ehci, ehci->async->qh_dma);
	hw->hw_info1 = cpu_to_hc32(ehci, QH_HEAD);
	hw->hw_token = cpu_to_hc32(ehci, QTD_STS_HALT);
	hw->hw_qtd_next = EHCI_LIST_END(ehci);
	ehci->async->qh_state = QH_STATE_LINKED;
	hw->hw_alt_next = QTD_NEXT(ehci, ehci->async->dummy->qtd_dma);

	/* clear interrupt enables, set irq latency */
	if (log2_irq_thresh < 0 || log2_irq_thresh > 6)
		log2_irq_thresh = 0;
	temp = 1 << (16 + log2_irq_thresh);
	ehci->has_ppcd = 0;

	if (HCC_CANPARK(hcc_params)) {
		/* HW default park == 3, on hardware that supports it (like
		 * NVidia and ALI silicon), maximizes throughput on the async
		 * schedule by avoiding QH fetches between transfers.
		 *
		 * With fast usb storage devices and NForce2, "park" seems to
		 * make problems:  throughput reduction (!), data errors...
		 */
		if (park) {
			park = min_t(unsigned, park, 3);
			temp |= CMD_PARK;
			temp |= park << 8;
		}
	}

	if (HCC_PGM_FRAMELISTLEN(hcc_params)) {
		/* periodic schedule size can be smaller than default */
		temp &= ~(3 << 2);
		temp |= (EHCI_TUNE_FLS << 2);
		switch (EHCI_TUNE_FLS) {
		case 0:
			ehci->periodic_size = 1024; break;
		case 1:
			ehci->periodic_size = 512; break;
		case 2:
			ehci->periodic_size = 256; break;
		default:
			BUG();
		}
	}

	ehci->command = temp;
	hcd->has_tt = 1;
	hcd->self.sg_tablesize = 0;
	return 0;
}


static void nvtim_patch_usbhc(struct usb_hcd *hcd)
{
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	unsigned int	command = ehci_readl(ehci, &ehci->regs->command);
	int retval, retry = 0;

	command |= CMD_RESET;
	ehci_writel(ehci, command, &ehci->regs->command);

	do {
		retval = ehci_handshake(ehci, &ehci->regs->command,
					CMD_RESET, 0, 250 * 1000);
		retry++;
	} while (retval && retry < 3);

	if (unlikely(retval != 0 && retry >= 3))
		ehci_err(ehci, "reset fail!\n");

	command = ehci->command;

	ehci_writel(ehci, (command & ~((unsigned int)(CMD_RUN|CMD_PSE|CMD_ASE)))
			, &ehci->regs->command);
	ehci_writel(ehci, ehci->periodic_dma, &ehci->regs->frame_list);
	ehci_writel(ehci, (u32)ehci->async->qh_dma, &ehci->regs->async_next);
	retry = 0;

	do {
		ehci_writel(ehci, INTR_MASK,
			&ehci->regs->intr_enable);
		retval = ehci_handshake(ehci, &ehci->regs->intr_enable,
					INTR_MASK, INTR_MASK, 250);
		retry++;
	} while (retval != 0);

	if (unlikely(retval != 0))
		ehci_err(ehci, "write fail!\n");

	ehci->command &= ~((unsigned int)(CMD_PSE|CMD_ASE));
	set_bit(1, &hcd->porcd);
}

static const struct hc_driver nvtivot_ehci_hc_driver = {
	.description =	  hcd_name,
	.product_desc =	 "usbhc-nvtivot",
	.hcd_priv_size =	sizeof(struct ehci_hcd),

	/*
	 * generic hardware linkage
	 */
	.irq =		ehci_irq,
	.flags =	HCD_MEMORY | HCD_USB2 | HCD_BH,

	/*
	 * basic lifecycle operations
	 */
	.reset =	nvtim_ehci_init,
	.start =	ehci_run,
	.stop =		ehci_stop,
	.shutdown =	ehci_shutdown,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue =	  ehci_urb_enqueue,
	.urb_dequeue =	  ehci_urb_dequeue,
	.endpoint_disable = ehci_endpoint_disable,
	.endpoint_reset =   ehci_endpoint_reset,

	/*
	 * scheduling support
	 */
	.get_frame_number = ehci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data =  ehci_hub_status_data,
	.hub_control =	  ehci_hub_control,
	.bus_suspend =	  ehci_bus_suspend,
	.bus_resume =	   ehci_bus_resume,
	.relinquish_port =  ehci_relinquish_port,
	.port_handed_over = ehci_port_handed_over,

	.clear_tt_buffer_complete = ehci_clear_tt_buffer_complete,
	.port_nc =		  nvtim_patch_usbhc,
};

static int nvtim_ehci_hcd_drv_probe(struct platform_device *pdev)
{
	struct usb_hcd *hcd = NULL;
	int ret;

	pr_debug("In nvtim_ehci_hcd_drv_probe\n");

	if (usb_disabled())
		return -ENODEV;

	ret = nvtim_usb_ehci_probe(&nvtivot_ehci_hc_driver, &hcd, pdev);
	return ret;
}

static int nvtim_ehci_hcd_drv_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
	pr_debug("In nvtim_ehci_hcd_drv_remove\n");
	nvtim_usb_ehci_remove(hcd, pdev);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

#ifdef CONFIG_PM
static void nvtim_fuse_restore(struct ehci_hcd *ehci)
{
	u32	usbbase;

	usbbase = (u32)ehci->caps;

	writel((ehci->fuseData >> 8)&0xFF,  (volatile unsigned long *)(usbbase+(0x1000+(0x6<<2))));
	writel((ehci->fuseData >> 16)&0xFF, (volatile unsigned long *)(usbbase+(0x1000+(0x5<<2))));
	//printk("ehci->fuseData = 0x%08X\n",ehci->fuseData);
}

static void nvtim_suspend_usbhc(struct platform_device *dev)
{
	struct usb_hcd	*hcd = platform_get_drvdata(dev);
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	u32		usbbase;

	usbbase = (u32)ehci->caps;

	/* Host PHY Suspend */
	writel((readl((volatile unsigned long *)(usbbase+0x40)) | 0x40), (volatile unsigned long *)(usbbase+0x40));
	udelay(1000);

}

static int nvtim_ehci_suspend(struct device *dev)
{
	struct usb_hcd *hcd = dev_get_drvdata(dev);
	bool do_wakeup = device_may_wakeup(dev);
	struct platform_device *pdev = to_platform_device(dev);
	int rc;
	int irq;

#ifdef CONFIG_USE_OF
	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		pr_debug("resource[1] is not IORESOURCE_IRQ");
		return -ENXIO;
	}
#else
	if (dev->resource[1].flags != IORESOURCE_IRQ) {
		pr_debug("resource[1] is not IORESOURCE_IRQ");
		retval = -ENOMEM;
		return retval;
	}
	irq = dev->resource[1].start;
#endif

	rc = ehci_suspend(hcd, do_wakeup);
	disable_irq(irq);

	//nvtim_stop_usbhc(pdev);
	nvtim_suspend_usbhc(pdev);

	return rc;
}


static int nvtim_ehci_resume(struct device *dev)
{
	struct usb_hcd *hcd = dev_get_drvdata(dev);
	struct platform_device *pdev = to_platform_device(dev);
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	int irq;

#ifdef CONFIG_USE_OF
	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		pr_debug("resource[1] is not IORESOURCE_IRQ");
		return -ENXIO;
	}
#else
	if (dev->resource[1].flags != IORESOURCE_IRQ) {
		pr_debug("resource[1] is not IORESOURCE_IRQ");
		retval = -ENOMEM;
		return retval;
	}
	irq = dev->resource[1].start;
#endif

	nvtim_init_usbhc(pdev);
	nvtim_fuse_restore(ehci);
	//nvtim_plat_resume(hcd);

	ehci_resume(hcd, false);
	enable_irq(irq);
	return 0;
}
#else
static int nvtim_ehci_suspend(struct device *dev)
{
	ehci_err(hcd_to_ehci(dev_get_drvdata(dev)),
		"+-%s[%d]:CONFIG_PM NOT DEFINED\r\n", __func__, __LINE__);
	return 0;
}
static int nvtim_ehci_resume(struct device *dev)
{
	ehci_err(hcd_to_ehci(dev_get_drvdata(dev)),
		"+-%s[%d]:CONFIG_PM NOT DEFINED\r\n", __func__, __LINE__);
	return 0;
}
#endif

static const struct dev_pm_ops nvtivot_ehci_pm_ops = {
	.suspend	= nvtim_ehci_suspend,
	.resume	 = nvtim_ehci_resume,
};

#ifdef CONFIG_OF
static const struct of_device_id nvtivot_ehci_match[] = {
	{ .compatible = "nvt,ehci-nvtivot" },
	{},
};

MODULE_DEVICE_TABLE(of, nvtivot_ehci_match);
#endif

MODULE_ALIAS("ehci-nvtivot");
static struct platform_driver ehci_hcd_na51068_driver = {
	.probe = nvtim_ehci_hcd_drv_probe,
	.remove = nvtim_ehci_hcd_drv_remove,
	.driver = {
		.name = "ehci-nvtivot",
		.pm = &nvtivot_ehci_pm_ops,
#ifdef CONFIG_OF
		.of_match_table = nvtivot_ehci_match,
#endif
	}
};

MODULE_VERSION("1.00.000");
