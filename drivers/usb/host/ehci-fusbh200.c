/*
 * (C) Copyright David Brownell 2000-2002
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef CONFIG_USB_DEBUG
#define DEBUG
#else
#undef DEBUG
#endif

static struct usb_hcd  *fusbh200_hcd;
static int usbh_pmu_fd = -1;

#define FUSBH200PHY (BIT23|BIT24|BIT25)
static pmuReg_t regUSBHArray[] = {
	/* reg_off,bit_masks,lock_bits,init_val,init_mask */
#if defined(CONFIG_PLATFORM_GM8185_v2)
	{0x38, BIT2, BIT2, 0, BIT2},
#elif defined(CONFIG_PLATFORM_GM8181)
	{0x38, BIT17, BIT17, 0, BIT17},
	{0x6C, FUSBH200PHY, FUSBH200PHY, 0, FUSBH200PHY},
	{0x80, ~0x0, ~0x0, 0, 0},
#endif
};
static pmuRegInfo_t usbh_clk_info = {
	"fusbh200_clk",
	ARRAY_SIZE(regUSBHArray),
	ATTR_TYPE_NONE,             /* no clock source */
	regUSBHArray
};

static int ftc_ehci_init(struct usb_hcd *hcd);

struct hc_driver fusbh200_ehci_driver = {
	.description =		hcd_name,
	.product_desc =		"FUSBH200 HOST",
	.hcd_priv_size =	sizeof(struct ehci_hcd),

	/*
	 * generic hardware linkage
	 */
	.irq =			ehci_irq,
	.flags =		HCD_MEMORY | HCD_USB2,

	/*
	 * basic lifecycle operations
	 */
	.reset =		ftc_ehci_init,
	.start =		ehci_run,
	.stop =			ehci_stop,
	.shutdown =     ehci_shutdown,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue =		ehci_urb_enqueue,
	.urb_dequeue =		ehci_urb_dequeue,
	.endpoint_disable =	ehci_endpoint_disable,
	.endpoint_reset =	ehci_endpoint_reset,

	/*
	 * scheduling support
	 */
	.get_frame_number =	ehci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data =	ehci_hub_status_data,
	.hub_control =		ehci_hub_control,
#if defined(CONFIG_PM)
	.bus_suspend =		ehci_bus_suspend,
	.bus_resume =		ehci_bus_resume,
#endif
	.relinquish_port = ehci_relinquish_port,
	.port_handed_over = ehci_port_handed_over,
	.clear_tt_buffer_complete	= ehci_clear_tt_buffer_complete,
};

#if 0
void ehci_reset_h200_PHY(void)
{
#ifndef REMOVE_COVERHW
	//Reset PHY
	mwH20_Control_Phy_Reset_Set();
	udelay(1000);//1000 About 0.2ms
	mwH20_Control_Phy_Reset_Clr();
#endif
}
#endif

static int ftc_ehci_init(struct usb_hcd *hcd)
{
	int result;
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);

	fotg2xx_dbg("call ftc_ehci_init.......ehci = %x\n", (u32)ehci);
	ehci->big_endian_mmio = 0;

	ehci->caps = hcd->regs;
	ehci->regs = hcd->regs + HC_LENGTH(ehci, ehci_readl(ehci, &ehci->caps->hc_capbase));

	dbg_hcs_params(ehci, "reset");
	dbg_hcc_params(ehci, "reset");

	/* cache this readonly data; minimize chip reads */
	ehci->hcs_params = ehci_readl(ehci, &ehci->caps->hcs_params);

	result = ehci_halt(ehci);
	if (result)
		return result;

	/* data structure init */
	result = ehci_init(hcd);
	if (result)
		return result;

	hcd->has_tt=1; //tt means transaction-translator

	ehci->sbrn = 0x20;

	ehci_reset(ehci);

	ehci_port_power(ehci, 0);

	return result;
}

static void init_FUSBH200_Dev(struct usb_hcd *hcd)
{
#if defined(CONFIG_PLATFORM_GM8185_v2)
	ftpmu010_write_reg(usbh_pmu_fd, 0x38, 0, BIT2);
#elif defined(CONFIG_PLATFORM_GM8181)
	if ((ftpmu010_get_attr(ATTR_TYPE_PMUVER) & 0xF0) == 0x20) { /* 8181T/8186 */
		ftpmu010_write_reg(usbh_pmu_fd, 0x6C, BIT1<<23, BIT1<<23);
	}
	ftpmu010_write_reg(usbh_pmu_fd, 0x38, 0, BIT17);
	mdelay(10); // waiting for PHY clock be stable, while clock source changed from externel to internel, at lease 5ms
	if ((ftpmu010_get_attr(ATTR_TYPE_PMUVER) & 0xF0) == 0x20) { /* 8181T/8186 */
		unsigned int tmp;

		tmp = ftpmu010_read_reg(0x80);
		tmp &= ~0x1F0;
		tmp |= 0x3 << 6; /// IREF_ST[2:0], default:0x4
		tmp |= 0x1 << 4; /// PCR[1:0], default:0x1
		ftpmu010_write_reg(usbh_pmu_fd, 0x80, tmp, tmp);

		mwH20Bit_Clr(hcd->regs,0x34,BIT6); // place PHY in suspend mode
		mdelay(10);
		mwH20Bit_Set(hcd->regs,0x34,BIT6);
		mdelay(10);
	}
#endif
#ifdef CONFIG_711MA_PHY
	//Set cover bit to cover 711MA PHY full speed reset issue
	mwH20_Control_COVER_FS_PHY_Reset_Set(hcd->regs);
#endif
	//Set OTG200 interrupt to high active
	mwH20_Interrupt_OutPut_High_Set(hcd->regs);  //For A320
	//Important: If AHB clock is >=15Mhz and <= 30MHz, please remark this line (Enable half speed)).
	//IF > 30Hz, Disable half speed
	mwH20_Control_VBUS_OFF_Clr(hcd->regs);
#ifdef EnableHalfSpeed
	mwH20_Control_HALFSPEEDEnable_Set(hcd->regs);
#else /* EnableHalfSpeed */
	mwH20_Control_HALFSPEEDEnable_Clr(hcd->regs);
#endif /* EnableHalfSpeed */

	mwH20_Control_ForceHighSpeed_Clr(hcd->regs);
	mwH20_Control_ForceFullSpeed_Clr(hcd->regs);
#if 0 // debug for force usbh phy speed
	mwH20_Control_ForceFullSpeed_Set(hcd->regs); // force to full-speed (do not ack to device's KJ pattern)
	//mwH20_Control_ForceHighSpeed_Set(hcd->regs); // force to high-speed
	if (mwHost20Bit_Rd(0x40, BIT7))
		printk("!!!! Force Phy to Full-Speed !!!!\n");
	else if (mwHost20Bit_Rd(0x40, BIT6))
		printk("!!!! Force Phy to High-Speed !!!!\n");
#endif
	mwH20_Int_BM_OVC_En(hcd->regs);
	mwH20_Int_BM_VBUS_ERR_En(hcd->regs);
}

void fusbh200_hw_init(struct usb_hcd *hcd)
{

	if ((usbh_pmu_fd = ftpmu010_register_reg(&usbh_clk_info)) < 0)
		printk(KERN_ERR "%s: Register USB to PMU failed\n", __func__);
#if defined(CONFIG_PLATFORM_GM8181) && defined(CONFIG_PCI)
#include <linux/pci.h>
	// only BGA684 package has PCI pin out
	// if PCI device mode enabled, no more init FUSBH200 doing, just exit
	if (((ftpmu010_read_reg(0x0)>>5) & 0x07) == 0) {
		if ((* (volatile unsigned int *) (PCIC_FTPCI100_VA_BASE + 0xA0) & BIT4) == 0)
			return -1;
	}
#endif
	init_FUSBH200_Dev(hcd);
#ifdef CONFIG_GM_FUSBH200_LOW_TIMING
	//John add timing adjustment code for fast plug/unplug issue, should set
	//when system boot up
	{
		u32 temp;

		printk("Note : SET lower timing on FPGA, will get low performance\n");
		//Set Timing parameter when system bus too low
		//special on FPGA, Should set as default value (0x01) in real chip
		//With this message, you should check your system bus performance
		temp = readl ((u32)hcd->regs | 0x34);
		writel (temp | 0x0d,  (u32)hcd->regs | 0x34 );
		temp = readl ((u32)hcd->regs | 0x34);
		printk("Timing on misc(0x%08x) = 0x%x\n",(u32)hcd->regs | 0x34, temp);
	}
#endif
}

void fusbh200_hw_deinit(void)
{
#ifdef CONFIG_PLATFORM_GM8185_v2
	ftpmu010_write_reg(usbh_pmu_fd, 0x38, BIT2, BIT2);
#elif defined(CONFIG_PLATFORM_GM8181)
	ftpmu010_write_reg(usbh_pmu_fd, 0x38, BIT17, BIT17);
#endif
	if (ftpmu010_deregister_reg(usbh_pmu_fd) < 0)
		printk(KERN_ERR "%s: Unregister USB from PMU Failed\n", __func__);
}

/**
 * usb_hcd_fusbh200_probe - initialize HCDs
 * @pdev: USB Host Controller being probed
 * Context: !in_interrupt()
 *
 * Allocates basic PCI resources for this USB host controller, and
 * then invokes the start() method for the HCD associated with it
 * through the hotplug entry's driver_data.
 *
 * Store this function in the HCD's struct pci_driver as probe().
 */
int usb_hcd_fusbh200_probe (struct platform_device *pdev)
{
	struct hc_driver	*driver;
	struct usb_hcd		*hcd;
	struct resource		*res;
	int			irq;
	int			retval;

	fotg2xx_dbg("usb_hcd_fotg2xx_probe: %x \n",(u32)pdev);

	if (usb_disabled())
		return -ENODEV;

	driver = &fusbh200_ehci_driver;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev,
			"Found HC with no physical mem. Check %s setup!\n",
			dev_name(&pdev->dev));
		return -ENODEV;
	}
	irq = platform_get_irq(pdev, 0);

	pdev->dev.power.power_state = PMSG_ON;

	fusbh200_hcd = hcd = usb_create_hcd(driver, &pdev->dev, dev_name(&pdev->dev));
	if (!hcd) {
		retval = -ENOMEM;
		goto fail_create_hcd;
	}

	if (driver->flags & HCD_MEMORY) {	// EHCI, OHCI

		hcd->rsrc_start = res->start;
		hcd->rsrc_len = res->end - res->start + 1;

		if (!request_mem_region(hcd->rsrc_start, hcd->rsrc_len,
					driver->description)) {
			dev_dbg(&pdev->dev, "controller already in use\n");
			retval = -EBUSY;
			goto fail_request_resource;
		}
		hcd->regs = ioremap_nocache(hcd->rsrc_start, hcd->rsrc_len);
		if (hcd->regs == NULL) {
			dev_dbg(&pdev->dev, "error mapping memory\n");
			retval = -EFAULT;
			goto fail_ioremap;
		}

	} else { 				// UHCI
		printk("Warning usb_hcd_fotg2xx_probe --- FOTG2XX Not support UHCI Flags \n");
		retval = -ENOMEM;
		goto fail_create_hcd;
	}

	hcd->product_desc = "FUSBH200";

	fusbh200_hw_init(hcd);

	retval = usb_add_hcd (hcd, irq, IRQF_SHARED);
	fotg2xx_dbg("usb_hcd_fotg2xx_probe: %x \n",retval);

	if (retval != 0)
		goto fail_add_hcd;
	return retval;

fail_add_hcd:
	iounmap (hcd->regs);
fail_ioremap:
	release_mem_region (hcd->rsrc_start, hcd->rsrc_len);
fail_request_resource:
	usb_put_hcd (hcd);
fail_create_hcd:
	dev_err (&(pdev->dev), "init %s fail, %d\n", pdev->name, retval);
	return retval;
}

/**
 * usb_hcd_fusbh200_remove - shutdown processing for PCI-based HCDs
 * @pdev: USB Host Controller being removed
 * Context: !in_interrupt()
 *
 * Reverses the effect of usb_hcd_fusbh200_probe(), first invoking
 * the HCD's stop() method.  It is always called from a thread
 * context, normally "rmmod", "apmd", or something similar.
 *
 * Store this function in the HCD's struct pci_driver as remove().
 */
int usb_hcd_fusbh200_remove (struct platform_device *pdev)
{
	struct usb_hcd		*hcd;
	struct resource		*res;

	fusbh200_hw_deinit();

	hcd = fusbh200_hcd;
	if (!hcd)
		return 0;

	usb_remove_hcd (hcd);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev,
			"Found HC with no physical mem. Check %s setup!\n",
			dev_name(&pdev->dev));
		return -ENODEV;
	}

	if (hcd->driver->flags & HCD_MEMORY) {
		iounmap (hcd->regs);
		release_mem_region (hcd->rsrc_start, hcd->rsrc_len);
	}
	else {
		release_region (hcd->rsrc_start, hcd->rsrc_len);
	}
	usb_put_hcd (hcd);
	return 0;
}

static struct platform_driver fusbh200_plat_driver = {
	.probe = usb_hcd_fusbh200_probe,
	.remove = usb_hcd_fusbh200_remove,
	.driver = {
		.name = "GM_fusbh200",
	},
};
