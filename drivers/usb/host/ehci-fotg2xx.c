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

#include "fotg2xx-config.h"
static struct usb_hcd  *fotg2xx_hcd[FOTG_DEV_NR];

/*
 * hc states include: unknown, halted, ready, running
 * transitional states are messy just now
 * trying to avoid "running" unless urbs are active
 * a "ready" hc can be finishing prefetched work
 */


static int FTC_ehci_init(struct usb_hcd *hcd);

struct hc_driver ehci_driver = {
	.description =		hcd_name,
	.product_desc =		"FOTG2XX HOST",
	.hcd_priv_size =	sizeof(struct ehci_hcd),

	/*
	 * generic hardware linkage
	 */
	.irq =			ehci_irq,
	.flags =		HCD_MEMORY | HCD_USB2,

	/*
	 * basic lifecycle operations
	 */
	.reset =		FTC_ehci_init,
	.start =		ehci_run,
	.stop =			ehci_stop,
	.shutdown =		ehci_shutdown,

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
	.relinquish_port =	ehci_relinquish_port,
	.port_handed_over =	ehci_port_handed_over,
	.clear_tt_buffer_complete	= ehci_clear_tt_buffer_complete,
};

#if 0
/* FOTG2XX Controll Registers Setup When Interrupt  occured  */
void fotg_ehci_irq_macro(void)
{
	if ( mwOTG20_Interrupt_Status_HOST_Rd()){
		mwOTG20_Interrupt_Status_HOST_Clr();
	}
}
#endif

static int FTC_ehci_init(struct usb_hcd *hcd)
{
	int result;
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);

	fotg2xx_dbg("call FTC_ehci_init.......ehci = %x\n", (u32)ehci);
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

	hcd->has_tt = 1; //tt means transaction-translator

	ehci->sbrn = 0x20;

	ehci_reset(ehci);

	ehci_port_power(ehci, 0);

	return result;
}

extern void init_FOTG2XX_Dev(struct usb_hcd *hcd);
extern void exit_FOTG2XX_Dev(struct usb_hcd *hcd);
extern int usb_get_pmu_fd(void);
extern int fotg200_otgd_init(struct platform_device *dev,struct usb_bus *host,struct usb_gadget *gadget);
extern int fotg200_init(struct device *dev,struct usb_bus *host,struct usb_gadget *gadget);

static void fotg2xx_hw_init(struct usb_hcd *hcd)
{
	init_FOTG2XX_Dev(hcd);

#ifdef CONFIG_GM_FOTG2XX_LOW_TIMING
	//John add timing adjustment code for fast plug/unplug issue, should set
	//when system boot up
	{
		u32 temp;

		printk("Note : SET lower timing on FPGA, will get low performance\n");
		//Set Timing parameter when system bus too low
		//special on FPGA, Should set as default value (0x01) in real chip
		//With this message, you should check your system bus performance
		temp = readl ((u32)hcd->regs | 0x40);
		writel (temp | 0x0d,  (u32)hcd->regs | 0x40 );
		temp = readl ((u32)hcd->regs | 0x40);
		printk("Timing on misc(0x%08x) = 0x%x\n", (u32)hcd->regs | 0x40, temp);
	}
#endif
}

static void fotg2xx_hw_deinit(struct usb_hcd *hcd)
{
	exit_FOTG2XX_Dev(hcd);
}

/**
 * usb_hcd_fotg2xx_probe - initialize HCDs
 * @pdev: USB Host Controller being probed
 * Context: !in_interrupt()
 *
 * Allocates basic PCI resources for this USB host controller, and
 * then invokes the start() method for the HCD associated with it
 * through the hotplug entry's driver_data.
 *
 * Store this function in the HCD's struct pci_driver as probe().
 */
int usb_hcd_fotg2xx_probe (struct platform_device *pdev)
{
	struct hc_driver	*driver;
	struct usb_hcd		*hcd;
	struct resource		*res;
	int			irq;
	int			retval;

	fotg2xx_dbg("usb_hcd_fotg2xx_probe: %x \n",(u32)pdev);

	if (usb_disabled())
		return -ENODEV;

	driver = &ehci_driver;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev,
			"Found HC with no physical mem. Check %s setup!\n",
			dev_name(&pdev->dev));
		return -ENODEV;
	}
	irq = platform_get_irq(pdev, 0);

	pdev->dev.power.power_state = PMSG_ON;

    fotg2xx_hcd[pdev->id] = hcd = usb_create_hcd(driver, &pdev->dev, dev_name(&pdev->dev));
	if (!hcd) {
		retval = -ENOMEM;
		goto fail_create_hcd;
	}

	if (driver->flags & HCD_MEMORY) {	// EHCI, OHCI

		hcd->rsrc_start = res->start;
		hcd->rsrc_len = resource_size(res);

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

	hcd->product_desc = "FOTG2XX";

	retval = fotg200_otgd_init(pdev, &hcd->self, NULL);

	fotg2xx_hw_init(hcd);

	retval = usb_add_hcd (hcd, irq, IRQF_SHARED);
	fotg2xx_dbg("usb_hcd_fotg2xx_probe: %x \n",retval);

	if (retval != 0)
		goto fail_add_hcd;

	retval = fotg200_init(&(pdev->dev), &hcd->self, NULL);

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
 * usb_hcd_fotg2xx_remove - shutdown processing for PCI-based HCDs
 * @dev: USB Host Controller being removed
 * Context: !in_interrupt()
 *
 * Reverses the effect of usb_hcd_fotg2xx_probe(), first invoking
 * the HCD's stop() method.  It is always called from a thread
 * context, normally "rmmod", "apmd", or something similar.
 *
 * Store this function in the HCD's struct pci_driver as remove().
 */
int usb_hcd_fotg2xx_remove (struct platform_device *pdev)
{
	struct usb_hcd		*hcd;
	struct resource		*res;

    hcd = fotg2xx_hcd[pdev->id];
	if (!hcd)
		return 0;

	fotg2xx_hw_deinit(hcd);

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

static struct platform_driver fotg210_ehci_driver = {
	.probe = usb_hcd_fotg2xx_probe,
	.remove = usb_hcd_fotg2xx_remove,
	.driver = {
		.name = "fotg210",
		.owner = THIS_MODULE,
	},
};
