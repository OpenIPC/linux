#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/of.h>

#include <linux/usb.h>
#include <linux/usb/hcd.h>
#include <linux/usb/otg.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>

#include <soc/base.h>
#include <linux/jz_dwc.h>

#include "core.h"
#include "gadget.h"
#include "host.h"
#include "debug.h"

#include "dwc2_jz.h"

#ifdef CONFIG_USB_DWC2_VERBOSE_VERBOSE
int dwc2_core_debug_en = 0;
module_param(dwc2_core_debug_en, int, 0644);

#define DWC2_CORE_DEBUG_MSG(msg...)				\
	do {							\
		if (unlikely(dwc2_core_debug_en)) {		\
			dwc2_printk("core", msg);		\
		}						\
	} while(0)

#else
#define DWC2_CORE_DEBUG_MSG(msg...)  do {  } while(0)
#endif

void calculate_fifo_size(struct dwc2 *dwc) {
	/*
	 * NOTE: we are use "Dedicated FIFO Mode with No Thresholding"
	 *  if need thresholding, the calculation algorithm may need change
	 */

	/**
	 * 3.2.1.1 FIFO SPRAM(Single-Port RAM) mapping:
	 *
	 * 1. One common RxFIFO, used in Host and Device modes
	 * 2. One common Periodic TxFIFO, used in Host mode
	 * 3. Separate IN endpoint transmit FIFO for each Device mode IN endpoints in Dedicated Transmit FIFO
	 *    operation (OTG_EN_DED_TX_FIFO = 1)
	 * 4. The FIFO SPRAM is also used for storing some register values to save gates. In Scatter/Gather DMA
	 *    mode, four SPRAM locations (four 35-bit words) are reserved for this. In DMA and Slave modes
	 *    (non-Scatter/Gather mode), one SPRAM location (one 35-bit word) is used for storing the DMA address.
	 *
	 * Device Mode Mapping
	 * --------------------
	 *
	 * NOTE: when the device is operating in Scatter/Gather mode, then the last
	 *       locations of the SPRAM store the Base Descriptor address, Current
	 *       Descriptor address, Current Buffer address and status quadlet
	 *       information for each endpoint direction (4 locations per Endpoint).
	 *       If an endpoint is bidirectional, then 4 locations will be used for IN,
	 *       and another 4 for OUT
	 * 3.2.4.4 Endpoint Information Controller
	 *       The last locations in the SPRAM are used to hold register values.
	 *    Device Buffer DMA Mode:
	 *       one location per endpoint direction is used in SPRAM to store the
	 *       DIEPDMA and DOEPDMA value. The application writes data and then reads
	 *       it from the same location
	 *       For example, if there are ten bidirectional endpoints, then the last
	 *       20 SPRAM locations are reserved for storing the DMA address for IN
	 *       and OUT endpoints
	 *   Scatter/Gather DMA Mode:
	 *       Four locations per endpoint direction are used in SPRAM to store the
	 *       Base Descriptor address, Current Descriptor address, Current Buffer
	 *       Pointer and the Status Quadlet.
	 *       The application writes data to the base descriptor address.
	 *       When the application reads the location where it wrote the base
	 *       descriptor address, it receives the current descriptor address.
	 *       For example, if there are ten bidirectional endpoints, then the last 80
	 *      locations are reserved for storing these values.
	 *
	 * Figure 3-13
	 *  ________________________
	 *  |                       |
	 *  | DI/OEPDMAn Register   | Depends on the value of OTG_NUM_EPS
	 *  | and Descriptor Status | and OTG_EP_DIRn, see not above
	 *  |      values           |
	 *  -------------------------
	 *  |   TxFIFO #n Packets   |  DIEPTXFn
	 *  -------------------------
	 *  |                       |
	 *  |   ................    |
	 *  |                       |
	 *  -------------------------
	 *  |  TxFIFO #1 Packets    | DIEPTXF1
	 *  -------------------------
	 *  |  TxFIFO #0 Packets    |
	 *  |( up to3 SETUP Packets)| GNPTXFSIZ
	 *  ------------------------
	 *  |                       |
	 *  |     Rx Packets        |  GRXFSIZ
	 *  |                       |
	 *  -------------------------  Address = 0, Rx starting address fixed to 0
	 *
	 *
	 *  Host Mode Mapping
	 *  -----------------
	 *  The host uses one transmit FIFO for all non-periodic OUT transactions and one transmit FIFO for all
	 *  periodic OUT transactions
	 *
	 *  The host uses one receive FIFO for all periodic and non-periodic transactions.
	 *
	 *  ------------------------
	 *  |                      | Depends on the value of OTG_NUM_HOST_CHANNEL
	 *  |  HCDMAn Register     | 1 location per channel for Buffer DMA
	 *  |     Values           | 4 locations per channel for Scatter/Gather DMA
	 *  ------------------------
	 *  | Periodic Tx Packets  |  HPTXFSIZ  (One Common Periodic TxFIFO)
	 *  ------------------------
	 *  | non-Periodic Tx Pkts |  GNPTXFSIZ
	 *  ------------------------
	 *  |      Rx Packets      |  GRXFSIZ  (One common RxFIFO)
	 *  ------------------------  Address = 0, Rx starting address fixed to 0
	 *
	 * Recommended:
	 * RxFIFO size: 2 * (MPS / 4 + 2) + 16
	 * Non-periodic TxFIFO size: 2 * (MPS / 4)
	 * Periodic TxFIFO size: min(MPS * MC / 4, 6000 / 4)
	 */

	/**
	 * Rx FIFO Allocation (rx_fifo_size)
	 * ---------------------------------
	 * RAM for SETUP Packets: 4 * n + 6 locations must be Reserved in the receive FIFO to receive up to
	 * n SETUP packets on control endpoints, where n is the number of control endpoints the device
	 * core supports.
	 *
	 * One location for Global OUT NAK
	 *
	 * Status information is written to the FIFO along with each received packet. Therefore, a minimum
	 * space of (Largest Packet Size / 4) + 1 must be allotted to receive packets. If a high-bandwidth
	 * endpoint is enabled, or multiple isochronous endpoints are enabled, then at least two (Largest
	 * Packet Size / 4) + 1 spaces must be allotted to receive back-to-back packets. Typically, two
	 * (Largest Packet Size / 4) + 1 spaces are recommended so that when the previous packet is being
	 * transferred to AHB, the USB can receive the subsequent packet. If AHB latency is high, you must
	 * allocate enough space to receive multiple packets. This is critical to prevent dropping of any
	 * isochronous packets.
	 *
	 * Typically, one location for each OUT endpoint is recommended.
	 *
	 * one location for eatch endpoint for EPDisable is required
	 *
	 * Tx FIFO Allocation (tx_fifo_size[n])
	 * ------------------------------------
	 * The minimum RAM space required for each IN Endpoint Transmit FIFO is the maximum packet size
	 * for that particular IN endpoint.
	 *
	 * More space allocated in the transmit IN Endpoint FIFO results in a better performance on the USB
	 *and can hide latencies on the AHB.
	 */

	/**
	 * see Databook Chapter 3 - Architecture for more information:
	 * Host: 3.1.4, 3.2.4
	 * Device: 3.1.5, 3.2.4
	 */

	dwc_otg_core_global_regs_t *global_regs = dwc->core_global_regs;
	/*
	 * x = 0: Method 1, no high-bandwidth support
	 * x = 1: Method 2, recommended minimum FIFO depth with support for high-bandwidth
	 * x = 2: Method 3
	 */
	const int x = 1;      /* I have test 2, not function!  1 is ok, 0 is also ok */

	u32 dev_rx_fifo_size;
	u32 host_rx_fifo_size;
	u32 rx_fifo_size;

	u32 host_non_periodic_tx_fifo_size;
	u32 dev_in_ep_tx_fifo_size0;

	fifosize_data_t nptxfifosize;
	fifosize_data_t txfifosize;
	fifosize_data_t ptxfifosize;
	gdfifocfg_data_t gdfifocfg;
	int num_in_eps;
	int i = 0;

	/* NOTE: each fifo max depth is 3648 (NOTE: see register reset value) */
	dev_rx_fifo_size = (4 * 1 + 6) + (x + 1) * (1024 / 4 + 1) +
		(2 * dwc->hwcfgs.hwcfg2.b.num_dev_ep) + 1;
	host_rx_fifo_size = (1024 / 4) + 1 + 1 + 1 * MAX_EPS_CHANNELS;

	rx_fifo_size = max(dev_rx_fifo_size, host_rx_fifo_size);

	/* Rx starting address fixed to 0, its depth is now configured to rx_fifo_size */
	dwc_writel(rx_fifo_size, &global_regs->grxfsiz);

	/* GNPTXFSIZ if used by EP0, its start address is rx_fifo_size */
	dev_in_ep_tx_fifo_size0 = (x + 1) * (64 / 4);
	host_non_periodic_tx_fifo_size = (x + 1) * (512 / 4);

	nptxfifosize.d32 = 0;
	nptxfifosize.b.depth = max(dev_in_ep_tx_fifo_size0, host_non_periodic_tx_fifo_size);
	//nptxfifosize.b.depth = 2 * 512 / 4;
	nptxfifosize.b.startaddr = rx_fifo_size;
	dwc_writel(nptxfifosize.d32, &global_regs->gnptxfsiz);

	/* configure EP1~n FIFO start address and depth */
	txfifosize.b.startaddr = nptxfifosize.b.startaddr + nptxfifosize.b.depth;

	/* number of high bandwidth ep */
	num_in_eps = dwc->hwcfgs.hwcfg4.b.num_in_eps;
	for (i = 0; i < num_in_eps; i++) {
		if (i < (num_in_eps - DWC_NUMBER_OF_HB_EP)) {
			txfifosize.b.depth = ((x + 1) * (512 / 4));
		} else {
			txfifosize.b.depth = ((x + 1) * (1024 / 4));
		}
		dwc_writel(txfifosize.d32, &global_regs->dtxfsiz[i]);

		/* the last FIFO block is also used for Host Mode periodic packets */
		if (i == (num_in_eps - 1)) {
			ptxfifosize.b.startaddr = txfifosize.b.startaddr;
			ptxfifosize.b.depth = txfifosize.b.depth;
			dwc_writel(ptxfifosize.d32, &global_regs->hptxfsiz);
		}

		txfifosize.b.startaddr += txfifosize.b.depth;
	}

	/*
	 * configure FIFO start address and depth for Endpoint Information Controller
	 */
	gdfifocfg.d32 = 0;
	gdfifocfg.b.epinfobase = txfifosize.b.startaddr;
	gdfifocfg.b.gdfifocfg = dwc_readl(&global_regs->ghwcfg3) >> 16;
	dwc_writel(gdfifocfg.d32, &global_regs->gdfifocfg);

	dwc2_flush_tx_fifo(dwc, 0x10);
	dwc2_flush_rx_fifo(dwc);
}

/**
 * Flush a Tx FIFO.
 *
 * See Programming Guide 2.2.3
 */
void dwc2_flush_tx_fifo(struct dwc2 *dwc, const int num)
{
	dwc_otg_core_global_regs_t	*global_regs = dwc->core_global_regs;
	volatile grstctl_t		 greset	     = {.d32 = 0 };
	gintsts_data_t			 gintsts;
	dctl_data_t			 dctl;
	int				 count	     = 0;
	unsigned			 phy_status;

	/* enable PHY */
	phy_status = !jz_otg_phy_is_suspend();
	jz_otg_phy_suspend(0);
	/*
	 * Check that GINTSTS.GINNakEff=0
	 */
	gintsts.d32 = dwc_readl(&global_regs->gintsts);
	if ( (gintsts.b.ginnakeff == 0) && dwc2_is_device_mode(dwc)) {
		/* If this bit is cleared then set DCTL.SGNPInNak=1 */
		dctl.d32 = dwc_readl(&dwc->dev_if.dev_global_regs->dctl);
		dctl.b.sgnpinnak = 1;
		dwc_writel(dctl.d32, &dwc->dev_if.dev_global_regs->dctl);

		/* Wait for GINTSTS.GINNakEff=1 */
		do {
			gintsts.d32 = dwc_readl(&global_regs->gintsts);
			if (++count > 10000) {
				dev_warn(dwc->dev, "%s():%d HANG! GINTSTS=0x%08x\n",
					__func__, __LINE__, gintsts.d32);
				break;
			}

			udelay(1);
		} while(gintsts.b.ginnakeff == 0);
	}

	/* Wait for AHB master IDLE state. */
	/* TODO: if OUT is running, will the AHB idle??? */
	count = 0;
	do {
		greset.d32 = dwc_readl(&global_regs->grstctl);
		if (++count > 100000) {
			dev_warn(dwc->dev, "%s():%d HANG! GRSTCTL=0x%08x GNPTXSTS=0x%08x\n",
				__func__, __LINE__,
				greset.d32, dwc_readl(&global_regs->gnptxsts));
			break;
		}

		udelay(1);
	} while (greset.b.ahbidle == 0);

	/* Check that GRSTCTL.TxFFlsh =0 */
	count = 0;
	do {
		greset.d32 = dwc_readl(&global_regs->grstctl);
		if (++count > 10000) {
			dev_warn(dwc->dev, "%s():%d HANG! GRSTCTL=0x%08x GNPTXSTS=0x%08x\n",
				__func__, __LINE__,
				greset.d32, dwc_readl(&global_regs->gnptxsts));
			break;
		}

		udelay(1);
	} while (greset.b.txfflsh == 1);

	greset.d32 = 0;
	greset.b.txfflsh = 1;
	greset.b.txfnum = num;
	dwc_writel(greset.d32, &global_regs->grstctl);

	count = 0;
	do {
		greset.d32 = dwc_readl(&global_regs->grstctl);
		if (++count > 100000) {
			dev_warn(dwc->dev, "%s():%d HANG! GRSTCTL=0x%08x GNPTXSTS=0x%08x\n",
				__func__, __LINE__,
				greset.d32, dwc_readl(&global_regs->gnptxsts));
			break;
		}

		udelay(1);
	} while (greset.b.txfflsh == 1);

	dctl.d32 = dwc_readl(&dwc->dev_if.dev_global_regs->dctl);
	dctl.b.cgnpinnak = 1;
	dwc_writel(dctl.d32, &dwc->dev_if.dev_global_regs->dctl);

	dwc2_wait_3_phy_clocks();

	jz_otg_phy_suspend(!phy_status);
}

void dwc2_flush_rx_fifo(struct dwc2 *dwc)
{
	dwc_otg_core_global_regs_t	*global_regs = dwc->core_global_regs;
	volatile grstctl_t		 greset	     = {.d32 = 0 };
	gintsts_data_t			 gintsts;
	dctl_data_t			 dctl;
	int				 count	     = 0;
	unsigned			 phy_status;

	/* enable PHY */
	phy_status = !jz_otg_phy_is_suspend();
	jz_otg_phy_suspend(0);


	gintsts.d32 = dwc_readl(&global_regs->gintsts);
	if ((gintsts.b.goutnakeff == 0) && dwc2_is_device_mode(dwc)) {
		dctl.d32 = dwc_readl(&dwc->dev_if.dev_global_regs->dctl);
		dctl.b.sgoutnak = 1;
		dwc_writel(dctl.d32, &dwc->dev_if.dev_global_regs->dctl);

		do {
			gintsts.d32 = dwc_readl(&global_regs->gintsts);
			if (++count > 10000) {
				dev_warn(dwc->dev, "%s:%d HANG! GINTSTS=0x%08x\n",
					__func__, __LINE__, gintsts.d32);
				dwc->do_reset_core = 1;
				break;
			}

			udelay(1);
		} while(gintsts.b.goutnakeff == 0);
	}

	/* Wait for AHB master IDLE state. */
	/* TODO: if OUT is running, will the AHB idle??? */
	count = 0;
	do {
		greset.d32 = dwc_readl(&global_regs->grstctl);
		if (++count > 100000) {
			dev_warn(dwc->dev, "%s:%d HANG! GRSTCTL=0x%08x GNPTXSTS=0x%08x\n",
				__func__, __LINE__,
				greset.d32, dwc_readl(&global_regs->gnptxsts));
			break;
		}

		udelay(1);
	} while (greset.b.ahbidle == 0);

	count = 0;
	do {
		greset.d32 = dwc_readl(&global_regs->grstctl);
		if (++count > 10000) {
			dev_warn(dwc->dev, "%s:%d HANG! GRSTCTL=%0x\n",
				__func__, __LINE__, greset.d32);
			break;
		}

		udelay(1);
	} while (greset.b.rxfflsh == 1);

	greset.d32 = 0;
	greset.b.rxfflsh = 1;
	dwc_writel(greset.d32, &global_regs->grstctl);

	do {
		greset.d32 = dwc_readl(&global_regs->grstctl);
		if (++count > 10000) {
			dev_warn(dwc->dev, "%s:%d HANG! GRSTCTL=%0x\n",
				__func__, __LINE__, greset.d32);
			break;
		}

		udelay(1);
	} while (greset.b.rxfflsh == 1);

	dctl.d32 = dwc_readl(&dwc->dev_if.dev_global_regs->dctl);
	dctl.b.cgoutnak = 1;
	dwc_writel(dctl.d32, &dwc->dev_if.dev_global_regs->dctl);

	dwc2_wait_3_phy_clocks();

	jz_otg_phy_suspend(!phy_status);
}

void dwc2_wait_3_phy_clocks(void) {
	/* PHY Cock: 30MHZ or 60MHZ */
	int ns = (1 * 1000 * 1000 * 1000) / (30 * 1000 * 1000);

	/* yes, we wait 5 phy clocks */
	ns *= 5;

	ndelay(ns);
}

void dwc2_enable_global_interrupts(struct dwc2 *dwc)
{
	gahbcfg_data_t ahbcfg;

	ahbcfg.d32 = dwc_readl(&dwc->core_global_regs->gahbcfg);
	ahbcfg.b.glblintrmsk = 1;
	dwc_writel(ahbcfg.d32, &dwc->core_global_regs->gahbcfg);
}

void dwc2_disable_global_interrupts(struct dwc2 *dwc)
{
	gahbcfg_data_t ahbcfg;

	ahbcfg.d32 = dwc_readl(&dwc->core_global_regs->gahbcfg);
	ahbcfg.b.glblintrmsk = 0;
	dwc_writel(ahbcfg.d32, &dwc->core_global_regs->gahbcfg);
}

/**
 * Do core a soft reset of the core.  Be careful with this because it
 * resets all the internal state machines of the core.
 */
void dwc2_core_reset(struct dwc2 *dwc)
{
	dwc_otg_core_global_regs_t *global_regs = dwc->core_global_regs;
	volatile grstctl_t greset = {.d32 = 0 };
	int count = 0;

	/* Core Soft Reset */
	count = 0;
	greset.b.csftrst = 1;
	dwc_writel(greset.d32, &global_regs->grstctl);
	do {
		greset.d32 = dwc_readl(&global_regs->grstctl);
		if (++count > 10000) {
			dev_warn(dwc->dev, "%s() HANG! Soft Reset GRSTCTL=0x%08x\n",
				__func__, greset.d32);
			break;
		}
		udelay(1);
	} while (greset.b.csftrst == 1);

	/* Wait for AHB master IDLE state. */
	do {
		udelay(10);
		greset.d32 = dwc_readl(&global_regs->grstctl);
		if (++count > 100000) {
			dev_warn(dwc->dev, "%s() HANG! AHB Idle GRSTCTL=0x%08x\n",
				__func__, greset.d32);
			return;
		}
	} while (greset.b.ahbidle == 0);

	dwc2_wait_3_phy_clocks();
}

void dwc2_enable_common_interrupts(struct dwc2 *dwc)
{
	dwc_otg_core_global_regs_t *global_regs = dwc->core_global_regs;
	gintmsk_data_t intr_mask = {.d32 = 0 };

	/* Clear any pending OTG Interrupts */
	dwc_writel(0xFFFFFFFF, &global_regs->gotgint);

	/* Clear any pending interrupts */
	dwc_writel(0xFFFFFFFF, &global_regs->gintsts);

	/*
	 * Enable the interrupts in the GINTMSK.
	 */
	intr_mask.b.modemismatch = 0;
	intr_mask.b.otgintr = 1;

	if (!dwc->dma_enable) {
		intr_mask.b.rxstsqlvl = 1;
	}

	intr_mask.b.conidstschng = 1;
	intr_mask.b.wkupintr = 1;
	intr_mask.b.disconnect = 0;
	intr_mask.b.usbsuspend = 1;
	intr_mask.b.sessreqintr = 1;

#ifdef CONFIG_USB_DWC_OTG_LPM
	if (dwc->core_params->lpm_enable) {
		intr_mask.b.lpmtranrcvd = 1;
	}
#endif
	dwc_writel(intr_mask.d32, &global_regs->gintmsk);
}

static void dwc2_init_csr(struct dwc2 *dwc) {
	/* change to uint8_t*, for convenient to add an offset */
	uint8_t *reg_base = (uint8_t *)dwc->regs;
	int i = 0;

	/* Global CSR */
	dwc->core_global_regs = (dwc_otg_core_global_regs_t *)reg_base;

	/* Device Mode CSR */
	dwc->dev_if.dev_global_regs =
		(dwc_otg_device_global_regs_t *) (reg_base + DWC_DEV_GLOBAL_REG_OFFSET);

	for (i = 0; i < MAX_EPS_CHANNELS; i++) {
		dwc->dev_if.in_ep_regs[i] = (dwc_otg_dev_in_ep_regs_t *)
			(reg_base + DWC_DEV_IN_EP_REG_OFFSET +
				(i * DWC_EP_REG_OFFSET));

		dwc->dev_if.out_ep_regs[i] = (dwc_otg_dev_out_ep_regs_t *)
			(reg_base + DWC_DEV_OUT_EP_REG_OFFSET +
				(i * DWC_EP_REG_OFFSET));
	}

	/* Host Mode CSR */
	dwc->host_if.host_global_regs = (dwc_otg_host_global_regs_t *)
		(reg_base + DWC_OTG_HOST_GLOBAL_REG_OFFSET);

	dwc->host_if.hprt0 =
		(uint32_t *) (reg_base + DWC_OTG_HOST_PORT_REGS_OFFSET);

	for (i = 0; i < MAX_EPS_CHANNELS; i++) {
		dwc->host_if.hc_regs[i] = (dwc_otg_hc_regs_t *)
			(reg_base + DWC_OTG_HOST_CHAN_REGS_OFFSET +
				(i * DWC_OTG_CHAN_REGS_OFFSET));
	}

	/* Power Management */
	dwc->pcgcctl = (uint32_t *) (reg_base + DWC_OTG_PCGCCTL_OFFSET);

	/*
	 * Store the contents of the hardware configuration registers here for
	 * easy access later.
	 */
	dwc->hwcfgs.hwcfg1.d32 = dwc_readl(&dwc->core_global_regs->ghwcfg1);
	dwc->hwcfgs.hwcfg2.d32 = dwc_readl(&dwc->core_global_regs->ghwcfg2);
	dwc->hwcfgs.hwcfg3.d32 = dwc_readl(&dwc->core_global_regs->ghwcfg3);
	dwc->hwcfgs.hwcfg4.d32 = dwc_readl(&dwc->core_global_regs->ghwcfg4);

	/*
	 * Set the SRP sucess bit for FS-I2c
	 */
	//dwc->srp_success = 0;
	//dwc->srp_timer_started = 0;

	dwc->snpsid = dwc_readl(&dwc->core_global_regs->gsnpsid);

	dev_info(dwc->dev, "Core Release: %x.%x%x%x\n",
		(dwc->snpsid >> 12 & 0xF),
		(dwc->snpsid >> 8 & 0xF),
		(dwc->snpsid >> 4 & 0xF), (dwc->snpsid & 0xF));
}

/**
 * This function calculates the number of IN EPS
 * using GHWCFG1 and GHWCFG2 registers values
 *
 * @param dwc Programming view of the DWC_otg controller
 */
static uint32_t calc_num_in_eps(struct dwc2 *dwc)
{
	uint32_t num_in_eps = 0;
	uint32_t num_eps = dwc->hwcfgs.hwcfg2.b.num_dev_ep + 1;
	uint32_t num_tx_fifos = dwc->hwcfgs.hwcfg4.b.num_in_eps + 1;
	int i;

#define DWC_IS_INEP(_i)							\
	({								\
		unsigned int epdir = dwc->hwcfgs.hwcfg1.d32 >> ((_i) * 2);	\
		epdir &= 0x3;						\
									\
		((epdir == DWC_HWCFG1_DIR_BIDIR) || (epdir == DWC_HWCFG1_DIR_IN)); \
	})

	for (i = 0; i < num_eps; ++i) {
		if (DWC_IS_INEP(i))
			num_in_eps++;
	}

#undef DWC_IS_INEP

	if (dwc->hwcfgs.hwcfg4.b.ded_fifo_en)
		num_in_eps = (num_in_eps > num_tx_fifos) ? num_tx_fifos : num_in_eps;

	return num_in_eps;
}

/**
 * This function calculates the number of OUT EPS
 * using GHWCFG1 and GHWCFG2 registers values
 *
 * @param dwc Programming view of the DWC_otg controller
 */
static uint32_t calc_num_out_eps(struct dwc2 *dwc)
{
	uint32_t num_out_eps = 0;
	uint32_t num_eps = dwc->hwcfgs.hwcfg2.b.num_dev_ep + 1;
	int i;

#define DWC_IS_OUTEP(_i)						\
	({								\
		unsigned int epdir = dwc->hwcfgs.hwcfg1.d32 >> ((_i) * 2);	\
		epdir &= 0x3;						\
									\
		((epdir == DWC_HWCFG1_DIR_BIDIR) || (epdir == DWC_HWCFG1_DIR_OUT)); \
	})

	for (i = 0; i < num_eps; ++i) {
		if (DWC_IS_OUTEP(i))
			num_out_eps++;
	}

#undef DWC_IS_OUTEP

	return num_out_eps;
}

/*
 * dwc2_core_init - Low-level initialization of DWC2 Core
 * @dwc: Pointer to our controller context structure
 *
 * Returns 0 on success otherwise negative errno.
 */
int dwc2_core_init(struct dwc2 *dwc)
{
	dwc_otg_core_global_regs_t	*global_regs = dwc->core_global_regs;
	struct dwc2_dev_if		*dev_if	     = &dwc->dev_if;
	gahbcfg_data_t			 ahbcfg	     = {.d32 = 0 };
	gusbcfg_data_t			 usbcfg	     = {.d32 = 0 };
	gotgctl_data_t			 gotgctl     = {.d32 = 0 };
	unsigned			 phy_status;
	int				 i	     = 0;

	/* enable PHY */
	phy_status = !jz_otg_phy_is_suspend();
	jz_otg_phy_suspend(0);


	/* Common Initialization */
	usbcfg.d32 = dwc_readl(&global_regs->gusbcfg);
	usbcfg.b.ulpi_ext_vbus_drv = 0;
	usbcfg.b.term_sel_dl_pulse = 0;
	dwc_writel(usbcfg.d32, &global_regs->gusbcfg);

	/* Reset the Controller */
	dwc2_core_reset(dwc);

	/* TODO: use configurable value here! */
	dwc->adp_enable = 0;
	dwc->power_down = 0;

	dwc->otg_sts = 0;

	/* Initialize parameters from Hardware configuration registers. */
	dwc->dev_if.num_in_eps = calc_num_in_eps(dwc);
	dwc->dev_if.num_out_eps = calc_num_out_eps(dwc);

	WARN( (dwc->dev_if.num_in_eps != dwc->dev_if.num_out_eps),
		"IN and OUT endpoint number not match(%d=%d)\n",
		dwc->dev_if.num_in_eps, dwc->dev_if.num_out_eps);

	for (i = 0; i < dwc->hwcfgs.hwcfg4.b.num_dev_perio_in_ep; i++) {
		dev_if->perio_tx_fifo_size[i] =
			dwc_readl(&global_regs->dtxfsiz[i]) >> 16;
	}

	for (i = 0; i < dwc->hwcfgs.hwcfg4.b.num_in_eps; i++) {
		dev_if->tx_fifo_size[i] =
			dwc_readl(&global_regs->dtxfsiz[i]) >> 16;
	}

	dwc->total_fifo_size = dwc->hwcfgs.hwcfg3.b.dfifo_depth;
	dwc->rx_fifo_size = dwc_readl(&global_regs->grxfsiz);
	dwc->nperio_tx_fifo_size = dwc_readl(&global_regs->gnptxfsiz) >> 16;

	dev_dbg(dwc->dev, "Total FIFO SZ=%d\n", dwc->total_fifo_size);
	dev_dbg(dwc->dev, "Rx FIFO SZ=%d\n", dwc->rx_fifo_size);
	dev_dbg(dwc->dev, "NP Tx FIFO SZ=%d\n", dwc->nperio_tx_fifo_size);

	/* High speed PHY init */
	if (!dwc->phy_inited) {
		dwc->phy_inited = 1;

		/* UTMI+ interface */
		usbcfg.b.ulpi_utmi_sel = 0;
		/* 16-bit */
		usbcfg.b.phyif = 1;
		dwc_writel(usbcfg.d32, &global_regs->gusbcfg);

		/* Reset after setting the PHY parameters */
		dwc2_core_reset(dwc);
	}

	/* we use UTMI+ PHY, so set ULPI fields to 0 */
	usbcfg.d32 = dwc_readl(&global_regs->gusbcfg);
	usbcfg.b.ulpi_fsls = 0;
	usbcfg.b.ulpi_clk_sus_m = 0;
	dwc_writel(usbcfg.d32, &global_regs->gusbcfg);

	/* External DMA Mode burst Settings */
	dev_dbg(dwc->dev, "Architecture: External DMA\n");
	ahbcfg.d32 = 0;
	ahbcfg.b.hburstlen = DWC_GAHBCFG_INT_DMA_BURST_INCR16;
	ahbcfg.b.dmaenable = dwc->dma_enable;
	dwc_writel(ahbcfg.d32, &global_regs->gahbcfg);

	usbcfg.d32 = dwc_readl(&global_regs->gusbcfg);
	usbcfg.b.force_host_mode = 0;
	usbcfg.b.force_dev_mode = 0;

	if (dwc2_usb_mode() == HOST_ONLY) {
		usbcfg.b.force_host_mode = 1;
	} else if (dwc2_usb_mode() == DEVICE_ONLY) {
		usbcfg.b.force_dev_mode = 1;
	}

	usbcfg.b.hnpcap = 0;
	usbcfg.b.srpcap = 0;
	dwc_writel(usbcfg.d32, &global_regs->gusbcfg);

	gotgctl.d32 = dwc_readl(&dwc->core_global_regs->gotgctl);
	gotgctl.b.otgver = dwc->otg_ver;

	if (!IS_ENABLED(CONFIG_DWC2_HOST_OVERCURRENT_DET)) {
		gotgctl.b.vbvalidoven = 1;
		gotgctl.b.vbvalidovval = 1;
	}
	dwc_writel(gotgctl.d32, &dwc->core_global_regs->gotgctl);

	/* Enable common interrupts */
	dwc2_enable_common_interrupts(dwc);

	jz_otg_phy_suspend(!phy_status);

	return 0;
}

static void dwc2_core_exit(struct dwc2 *dwc)
{
}

#if DWC2_DEVICE_MODE_ENABLE
/*
 * Caller must take care of lock
 */
static void dwc2_gadget_disconnect(struct dwc2 *dwc)
{
	if (dwc->gadget_driver && dwc->gadget_driver->disconnect) {
		dwc2_spin_unlock(dwc);
		dwc->gadget_driver->disconnect(&dwc->gadget);
		dwc2_spin_lock(dwc);
	}
}

static void dwc2_gadget_resume(struct dwc2 *dwc) {
	if (dwc->gadget_driver && dwc->gadget_driver->resume) {
		dwc2_spin_unlock(dwc);
		dwc->gadget_driver->resume(&dwc->gadget);
		dwc2_spin_lock(dwc);
	}
}

static void dwc2_gadget_suspend(struct dwc2 *dwc)
{
	if (dwc->gadget_driver && dwc->gadget_driver->suspend) {
		dwc2_spin_unlock(dwc);
		dwc->gadget_driver->suspend(&dwc->gadget);
		dwc2_spin_lock(dwc);
	}
}
#else
#define dwc2_gadget_disconnect(d)	do {  } while(0)
#define dwc2_gadget_resume(d)		do {  } while(0)
#define dwc2_gadget_suspend(d)		do {  } while(0)
#endif	/* DWC2_DEVICE_MODE_ENABLE */

static void dwc2_handle_mode_mismatch_intr(struct dwc2 *dwc)
{
	gintsts_data_t gintsts;

	/* Just Log a warnning message */
	dev_warn(dwc->dev, "Mode Mismatch Interrupt: currently in %s mode\n",
		dwc2_is_host_mode(dwc) ? "Host" : "Device");

	/* Clear interrupt */
	gintsts.d32 = 0;
	gintsts.b.modemismatch = 1;
	dwc_writel(gintsts.d32, &dwc->core_global_regs->gintsts);
}

static void dwc2_handle_otg_intr(struct dwc2 *dwc) {
	dwc_otg_core_global_regs_t *global_regs = dwc->core_global_regs;
	gotgint_data_t gotgint;
	gotgctl_data_t gotgctl;
	gintsts_data_t gintsts;

	gotgint.d32 = dwc_readl(&global_regs->gotgint);
	DWC2_CORE_DEBUG_MSG("%s: otgint = 0x%08x\n",
			__func__, gotgint.d32);

#if DWC2_DEVICE_MODE_ENABLE
	if (gotgint.b.sesenddet) {
		depctl_data_t	doepctl = {.d32 = 0 };
		u32		doepdma;

		dev_info(dwc->dev, "session end detected!\n");
		gotgctl.d32 = dwc_readl(&global_regs->gotgctl);
		DWC2_CORE_DEBUG_MSG("%s:%d gotgctl = 0x%08x op_state = %d\n",
				__func__, __LINE__, gotgctl.d32, dwc->op_state);

		if (dwc->op_state == DWC2_B_HOST) {
			/*
			 * TODO: We currently did not add HNP support
			 * if HNP enable, Initialized the Core for Device mode here
			 */
			dwc->op_state = DWC2_B_PERIPHERAL;
		} else {
			/* If not B_HOST and Device HNP still set. HNP
			 * Did not succeed!*/
			if (gotgctl.b.devhnpen) {
				dev_err(dwc->dev, "Device Not Connected/Responding!\n");
			}

			/*
			 * TODO: If Session End Detected the B-Cable has been disconnected.
			 *       Reset PCD and Gadget driver to a clean state.
			 */
			dwc->lx_state = DWC_OTG_L0;
			dwc2_gadget_handle_session_end(dwc);
			dwc2_gadget_disconnect(dwc);
			dwc2_start_ep0state_watcher(dwc, 0);
			/* TODO: if adp enable, handle ADP Sense here */
		}

		gotgctl.d32 = dwc_readl(&global_regs->gotgctl);
		gotgctl.b.devhnpen = 0;
		dwc_writel(gotgctl.d32, &global_regs->gotgctl);

		if (dwc2_is_device_mode(dwc)) {
			doepctl.d32 = dwc_readl(&dwc->dev_if.out_ep_regs[0]->doepctl);
			doepdma = dwc_readl(&dwc->dev_if.out_ep_regs[0]->doepdma);

			if (doepctl.b.epena && (doepdma != dwc->ctrl_req_addr)) {
				dwc->do_reset_core = 1;
				printk("====>doep0dma = 0x%08x\n", dwc_readl(&dwc->dev_if.out_ep_regs[0]->doepdma));
				printk("====>doep0ctl = 0x%08x\n", dwc_readl(&dwc->dev_if.out_ep_regs[0]->doepctl));
				printk("====>setup_prepared = %d\n", dwc->setup_prepared);
			}
		}

		if (!dwc->keep_phy_on)
			dwc2_turn_off(dwc, true);

		if (IS_ENABLED(CONFIG_USB_DWC2_DEVICE_ONLY) &&
				IS_ENABLED(CONFIG_BOARD_HAS_NO_DETE_FACILITY)) {
			/* ensure bvalid signal is from PHY */
			gotgctl_data_t gotgctl;
			gotgctl.d32 = dwc_readl(&dwc->core_global_regs->gotgctl);
			gotgctl.b.bvalidoven = 0;
			dwc_writel(gotgctl.d32, &dwc->core_global_regs->gotgctl);
		}
	}
#endif

	if (gotgint.b.sesreqsucstschng) {
		dev_info(dwc->dev, " ++OTG Interrupt: Session Reqeust Success Status Change++\n");
		gotgctl.d32 = dwc_readl(&global_regs->gotgctl);
		if (gotgctl.b.sesreqscs) {
			/* TODO: did gadget suspended??? */
			dwc2_gadget_resume(dwc);

			/* Clear Session Request */
			gotgctl.b.sesreq = 0;
			dwc_writel(gotgctl.d32, &global_regs->gotgctl);
		}
	}

	if (gotgint.b.hstnegsucstschng) {
		dev_info(dwc->dev, "OTG Interrupt: hstnegsucstschng\n");
		/*
		 * Print statements during the HNP interrupt handling
		 * can cause it to fail.
		 */
		gotgctl.d32 = dwc_readl(&global_regs->gotgctl);
		if (gotgctl.b.hstnegscs) {
			if (dwc2_is_host_mode(dwc)) {
				/* TODO: handle Host Negotiation Success here! */
				dev_info(dwc->dev, "%s:%d Host Negotiation Success\n", __func__, __LINE__);
			}
		} else {
			gotgctl.b.hnpreq = 0;
			gotgctl.b.devhnpen = 0;
			dwc_writel(gotgctl.d32, &global_regs->gotgctl);
			dev_err(dwc->dev, "Device Not Connected/Responding\n");
		}
	}

	if (gotgint.b.hstnegdet) { /* TODO: we currently did not add HNP support */
		/* The disconnect interrupt is set at the same time as
		 * Host Negotiation Detected.  During the mode
		 * switch all interrupts are cleared so the disconnect
		 * interrupt handler will not get executed.
		 */
		dev_info(dwc->dev, "++OTG Interrupt: "
			"Host Negotiation Detected++ (%s)\n",
			(dwc2_is_host_mode(dwc) ? "Host" : "Device"));
		if (dwc2_is_device_mode(dwc)) {
			dev_info(dwc->dev, "a_suspend->a_peripheral (%d)\n", dwc->op_state);
			dwc->op_state = DWC2_A_PERIPHERAL;
		} else {
			/* TODO: */
		}
	}

	if (gotgint.b.adevtoutchng) {
		dev_info(dwc->dev, " ++OTG Interrupt: "
			"A-Device Timeout Change++\n");
	}

	if (gotgint.b.debdone) {
		dev_dbg(dwc->dev, " ++OTG Interrupt: " "Debounce Done++\n");
	}

	/* Clear GOTGINT */
	dwc_writel(gotgint.d32, &dwc->core_global_regs->gotgint);

	/* Clear interrupt */
	gintsts.d32 = 0;
	gintsts.b.otgintr = 1;
	dwc_writel(gintsts.d32, &dwc->core_global_regs->gintsts);
}

static void dwc2_conn_id_status_change_work(struct work_struct *work)
{
	struct dwc2	*dwc   = container_of(work, struct dwc2, otg_id_work);
	uint32_t	 count = 0;
	gotgctl_data_t	 gotgctl;
	unsigned long	 flags;

	dwc2_disable_global_interrupts(dwc);
	synchronize_irq(dwc->irq);
	gotgctl.d32 = dwc_readl(&dwc->core_global_regs->gotgctl);

	DWC2_CORE_DEBUG_MSG("id status change, gotgctl = 0x%08x\n", gotgctl.d32);

	if (gotgctl.b.conidsts) { /* B-Device connector (Device Mode) */
#if DWC2_DEVICE_MODE_ENABLE
		dctl_data_t	 dctl;
		DWC2_CORE_DEBUG_MSG("init DWC core as B_PERIPHERAL\n");
#else
		DWC2_CORE_DEBUG_MSG("DWC core A_HOST id disconnect\n");
#endif
		jz_set_vbus(dwc, 0);
#if DWC2_DEVICE_MODE_ENABLE
		dwc2_spin_lock_irqsave(dwc, flags);
		dwc2_core_init(dwc);
		dwc2_spin_unlock_irqrestore(dwc, flags);

		/* Wait for switch to device mode. */
		while (!dwc2_is_device_mode(dwc)) {
			msleep(100);
			if (++count > 10000)
				break;
		}

		if (count > 10000)
			dev_warn(dwc->dev, "%s:%d Connection id status change timed out",
				__func__, __LINE__);


		dwc2_spin_lock_irqsave(dwc, flags);
		dwc->op_state = DWC2_B_PERIPHERAL;
		dwc2_device_mode_init(dwc);

		if (likely(!dwc->plugin)) {
			/* soft disconnect */
			dctl.d32 = dwc_readl(&dwc->dev_if.dev_global_regs->dctl);
			dctl.b.sftdiscon = 1;
			dwc_writel(dctl.d32, &dwc->dev_if.dev_global_regs->dctl);

			/* note that we do not close PHY, this is because after set sftdiscon=1,
			 * a session end detect interrupt will be generate
			 */
		} else {
			dctl.d32 = dwc_readl(&dwc->dev_if.dev_global_regs->dctl);
			dctl.b.sftdiscon = dwc->pullup_on ? 0 : 1;
			dwc_writel(dctl.d32, &dwc->dev_if.dev_global_regs->dctl);
		}

		dwc2_spin_unlock_irqrestore(dwc, flags);
#endif		 /* DWC2_DEVICE_MODE_ENABLE */
	} else {	      /* A-Device connector (Host Mode) */
#if DWC2_HOST_MODE_ENABLE
		DWC2_CORE_DEBUG_MSG("init DWC as A_HOST\n");

		dwc2_spin_lock_irqsave(dwc, flags);
		dwc2_core_init(dwc);
		dwc2_spin_unlock_irqrestore(dwc, flags);

		while (!dwc2_is_host_mode(dwc)) {
			msleep(100);
			if (++count > 10000)
				break;
		}

		if (count > 10000)
			dev_warn(dwc->dev, "%s:%d Connection id status change timed out",
				__func__, __LINE__);

		dwc2_spin_lock_irqsave(dwc, flags);
#if !IS_ENABLED(CONFIG_USB_DWC2_HOST_ONLY)
		if (dwc->ep0state != EP0_DISCONNECTED) {
			dwc2_gadget_handle_session_end(dwc);
			dwc2_gadget_disconnect(dwc);
			dwc2_start_ep0state_watcher(dwc, 0);
		}
#endif

		dwc->op_state = DWC2_A_HOST;
		dwc->hcd->self.is_b_host = 0;
		dwc2_host_mode_init(dwc);
		dwc2_spin_unlock_irqrestore(dwc, flags);
		jz_set_vbus(dwc, 1);
		set_bit(HCD_FLAG_POLL_RH, &dwc->hcd->flags);
#endif	/* DWC2_HOST_MODE_ENABLE */
	}
	dwc2_enable_global_interrupts(dwc);
}

static void dwc2_handle_conn_id_status_change_intr(struct dwc2 *dwc) {
	gintsts_data_t gintsts;

	dev_info(dwc->dev, "ID PIN CHANGED!\n");

	schedule_work(&dwc->otg_id_work);

	/* Clear interrupt */
	gintsts.d32 = 0;
	gintsts.b.conidstschng = 1;
	dwc_writel(gintsts.d32, &dwc->core_global_regs->gintsts);
}

/*
 * TODO: currently we did not add SRP,HNP,ADP support.
 * so we only handle two cases: 1) A-Host, 2) B-Peripheral */
static void dwc2_handle_disconnect_intr(struct dwc2 *dwc) {
	gintsts_data_t gintsts;

	if (dwc2_is_device_mode(dwc)) { /* A-Peripheral(TODO) or B-Peripheral */
		/*
		 * when we are in device mode and is disconnect,
		 * there's nothing to do, session end will do all the stuff things
		 */
		dev_info(dwc->dev, "B-Peripheral disconnect event\n");
	} else {	    /* A-Device */
#if DWC2_HOST_MODE_ENABLE
		if (dwc->op_state == DWC2_A_HOST) {
			/* A-Cable still connected but device disconnect */
			dwc2_hcd_handle_device_disconnect_intr(dwc);
		}
#endif
	}

	gintsts.d32 = 0;
	gintsts.b.disconnect = 1;
	dwc_writel(gintsts.d32, &dwc->core_global_regs->gintsts);
}

/**
 * This interrupt indicates that a device is initiating the Session
 * Request Protocol to request the host to turn on bus power so a new
 * session can begin. The handler responds by turning on bus power. If
 * the DWC_otg controller is in low power mode, the handler brings the
 * controller out of low power mode before turning on bus power.
 *
 * @param dwc Programming view of DWC_otg controller.
 */
static void dwc2_handle_session_req_intr(struct dwc2 *dwc)
{
	gintsts_data_t gintsts;

	dev_dbg(dwc->dev, "++Session Request Interrupt++\n");

	if (dwc2_is_device_mode(dwc)) {
		dev_info(dwc->dev, "SRP: Device Mode\n");
	} else {
		hprt0_data_t hprt0;

		dev_info(dwc->dev, "SRP: Host Mode\n");
		/* Turn on the port power bit. */
		hprt0.d32 = dwc2_hc_read_hprt(dwc);
		hprt0.b.prtpwr = 1;
		dwc_writel(hprt0.d32, dwc->host_if.hprt0);
	}

	/* Clear interrupt */
	gintsts.d32 = 0;
	gintsts.b.sessreqintr = 1;
	dwc_writel(gintsts.d32, &dwc->core_global_regs->gintsts);
}

/**
 * This interrupt indicates that the DWC_otg controller has detected a
 * resume or remote wakeup sequence. If the DWC_otg controller is in
 * low power mode, the handler must brings the controller out of low
 * power mode. The controller automatically begins resume
 * signaling. The handler schedules a time to stop resume signaling.
 */
static void dwc2_handle_wakeup_detected_intr(struct dwc2 *dwc) {
	gintsts_data_t gintsts;

	if (dwc2_is_device_mode(dwc)) {
		if (dwc->lx_state == DWC_OTG_L2) {
			dctl_data_t dctl = {.d32 = 0 };

			/* Clear the Remote Wakeup Signaling */
			dctl.d32 = dwc_readl(&dwc->dev_if.dev_global_regs->dctl);
			dctl.b.rmtwkupsig = 0;
			dwc_writel(dctl.d32, &dwc->dev_if.dev_global_regs->dctl);
			dwc2_gadget_resume(dwc);
		} else {
			glpmcfg_data_t lpmcfg;
			lpmcfg.d32 = dwc_readl(&dwc->core_global_regs->glpmcfg);
			lpmcfg.b.hird_thres &= (~(1 << 4));
			dwc_writel(lpmcfg.d32, &dwc->core_global_regs->glpmcfg);
		}
		/** Change to L0 state*/
		dwc->lx_state = DWC_OTG_L0;
	} else {
#if IS_ENABLED(CONFIG_USB)
		dwc->port1_status |= USB_PORT_STATE_RESUME;
		usb_hcd_resume_root_hub(dwc->hcd);
#endif
	}

	/* Clear interrupt */
	gintsts.d32 = 0;
	gintsts.b.wkupintr = 1;
	dwc_writel(gintsts.d32, &dwc->core_global_regs->gintsts);
}

/**
 * This interrupt indicates that SUSPEND state has been detected on
 * the USB.
 *
 * For HNP the USB Suspend interrupt signals the change from
 * "a_peripheral" to "a_host".
 *
 * When power management is enabled the core will be put in low power
 * mode.
 */
static void dwc2_handle_usb_suspend_intr(struct dwc2 *dwc)
{
	gintsts_data_t gintsts;

	if (dwc2_is_device_mode(dwc)) {
		dsts_data_t dsts;
		/* Check the Device status register to determine if the Suspend
		 * state is active. */
		dsts.d32 = dwc_readl(&dwc->dev_if.dev_global_regs->dsts);
		dev_dbg(dwc->dev, "DSTS = 0x%08x\n", dsts.d32);

		dwc2_gadget_suspend(dwc);

#if IS_ENABLED(CONFIG_BOARD_HAS_NO_DETE_FACILITY) && IS_ENABLED(CONFIG_USB_DWC2_DEVICE_ONLY)
		if (dwc->ep0state != EP0_DISCONNECTED) {
			gotgctl_data_t gotgctl;

			gotgctl.d32 = dwc_readl(&dwc->core_global_regs->gotgctl);
			gotgctl.b.bvalidoven = 1;
			gotgctl.b.bvalidovval = 0;
			dwc_writel(gotgctl.d32, &dwc->core_global_regs->gotgctl);
			dev_dbg(dwc->dev, "generate session end by set gotgctl.b.bvalidoven\n");
		}
#endif
	} else {
		dev_info(dwc->dev, "%s:%d: host mode not implemented!\n", __func__, __LINE__);
	}

	/* Change to L2(suspend) state */
	dwc->lx_state = DWC_OTG_L2;

	/* Clear interrupt */
	gintsts.d32 = 0;
	gintsts.b.usbsuspend = 1;
	dwc_writel(gintsts.d32, &dwc->core_global_regs->gintsts);
}

static void dwc2_handle_restore_done_intr(struct dwc2 *dwc) {
	gintsts_data_t gintsts;

	gintsts.d32 = 0;
	gintsts.b.restoredone = 1;
	dwc_writel(gintsts.d32, &dwc->core_global_regs->gintsts);
}

static void dwc2_handle_port_intr(struct dwc2 *dwc) {
	gintsts_data_t gintsts;

	gintsts.d32 = 0;
	gintsts.b.portintr = 1;
	dwc_writel(gintsts.d32, &dwc->core_global_regs->gintsts);
}

static void dwc2_handle_common_interrupts(struct dwc2 *dwc, gintsts_data_t *gintr_status) {
	//gpwrdn_data_t gpwrdn = {.d32 = 0 };

	if (gintr_status->b.modemismatch) {
		dwc2_handle_mode_mismatch_intr(dwc);
		gintr_status->b.modemismatch =  0;
	}
	if (gintr_status->b.otgintr) {
		dwc2_handle_otg_intr(dwc);
		gintr_status->b.otgintr = 0;
	}

	if (gintr_status->b.conidstschng) {
		if (unlikely(gintr_status->b.disconnect || gintr_status->b.portintr)) {
			dev_err(dwc->dev, "disconnect or portintr when conidstschng? BUG!!!!!!\n");
		}
		dwc2_handle_conn_id_status_change_intr(dwc);
		gintr_status->b.conidstschng = 0;
	}

	if (gintr_status->b.disconnect) {
		dwc2_handle_disconnect_intr(dwc);
		gintr_status->b.disconnect = 0;
	}

	if (gintr_status->b.sessreqintr) {
		dwc2_handle_session_req_intr(dwc);
		gintr_status->b.sessreqintr = 0;
	}
	if (gintr_status->b.wkupintr) {
		dwc2_handle_wakeup_detected_intr(dwc);
		gintr_status->b.wkupintr = 0;
	}
	if (gintr_status->b.usbsuspend) {
		dwc2_handle_usb_suspend_intr(dwc);
		gintr_status->b.usbsuspend = 0;
	}

#ifdef CONFIG_USB_DWC2_LPM
	if (gintr_status->b.lpmtranrcvd) {
		dwc2_handle_lpm_intr(dwc);
	}
#endif
	if (gintr_status->b.restoredone) {
		dwc2_handle_restore_done_intr(dwc);
		gintr_status->b.restoredone = 0;
	}

	if (gintr_status->b.portintr && dwc2_is_device_mode(dwc)) {
		/*
		 * The port interrupt occurs while in device mode with HPRT0
		 * Port Enable/Disable.
		 */
		dwc2_handle_port_intr(dwc);
		gintr_status->b.portintr = 0;
	}

	/* TODO: Handle ADP interrupt here */

	/* TODO: handle gpwrdn interrupts here */
	// gpwrdn.d32 = dwc_readl(&dwc->core_global_regs->gpwrdn);
}

static int dwc2_do_reset_device_core(struct dwc2 *dwc) {
#if DWC2_DEVICE_MODE_ENABLE
	dctl_data_t	dctl;

	if (likely(!dwc->do_reset_core || !dwc2_is_device_mode(dwc)))
		return 0;

	dwc->do_reset_core = 0;
	dctl.d32 = dwc_readl(&dwc->dev_if.dev_global_regs->dctl);
	dctl.b.sftdiscon = 1;
	dwc_writel(dctl.d32, &dwc->dev_if.dev_global_regs->dctl);

	mdelay(1);

	dwc2_core_init(dwc);
	mdelay(1);
	dwc->op_state = DWC2_B_PERIPHERAL;
	dwc2_device_mode_init(dwc);
	dwc2_enable_global_interrupts(dwc);

	mdelay(1);

	dctl.d32 = dwc_readl(&dwc->dev_if.dev_global_regs->dctl);
	dctl.b.sftdiscon = dwc->pullup_on && dwc->plugin ? 0 : 1;
	dwc_writel(dctl.d32, &dwc->dev_if.dev_global_regs->dctl);
	return 1;
#else
	return 0;
#endif
}

static inline int dwc_lock(struct dwc2 *dwc)
{
	if (atomic_inc_return(&dwc->in_irq) == 1) {
		dwc->owner_cpu = smp_processor_id();
		return 0;
	} else {
		atomic_dec(&dwc->in_irq);
		return -1;
	}
}

static inline void dwc_unlock(struct dwc2 *dwc)
{
	atomic_dec(&dwc->in_irq);
}

static irqreturn_t dwc2_interrupt(int irq, void *_dwc) {
	struct dwc2			*dwc	     = _dwc;
	dwc_otg_core_global_regs_t	*global_regs = dwc->core_global_regs;
	gintsts_data_t			 gintr_status;
	gintsts_data_t			 gintsts;
	gintsts_data_t			 gintmsk;

	/*
	 * TODO: is it better to call spin_try_lock() here?
	 *       aka, if we can not get the lock, we have two selection:
	 *       1. spin until we get the lock or
	 *       2. abort this interrupt, let the DWC core interrupt us later
	 */

	if (!spin_trylock(&dwc->lock)) {
		return IRQ_HANDLED;
	}

	if (dwc_lock(dwc) < 0) {
		spin_unlock(&dwc->lock);
		return IRQ_HANDLED;
	}

	if (unlikely(dwc2_do_reset_device_core(dwc)))
		goto out;

	gintsts.d32 = dwc_readl(&global_regs->gintsts);
	gintmsk.d32 = dwc_readl(&global_regs->gintmsk);
	gintr_status.d32 = gintsts.d32 & gintmsk.d32;

	DWC2_CORE_DEBUG_MSG("%s:%d gintsts=0x%08x & gintmsk=0x%08x = 0x%08x\n",
		__func__, __LINE__, gintsts.d32, gintmsk.d32, gintr_status.d32);
	//dwc2_trace_gintsts(gintsts.d32, gintmsk.d32);

	if (unlikely(gintr_status.d32 == 0)) {
		int i;
		haint_data_t haint;
		dwc_otg_hc_regs_t *hc_regs;
		hcint_data_t hcint;

		printk("gintsts=0x%08x & gintmsk=0x%08x = 0x%08x\n"
			"haint=0x%08x daint=0x%08x hprt=0x%08x gotgint=0x%08x\n",
		       gintsts.d32, gintmsk.d32, gintr_status.d32,
		       *((volatile unsigned int *)0xb3500414),
		       *((volatile unsigned int *)0xb3500818),
		       *((volatile unsigned int *)0xb3500440),
		       *((volatile unsigned int *)0xb3500004)
			);

		haint.d32 = dwc_readl(&dwc->host_if.host_global_regs->haint);

		for (i = 0; i < MAX_EPS_CHANNELS; i++) {
			if (haint.b2.chint & (1 << i)) {
				hc_regs = dwc->host_if.hc_regs[i];
				hcint.d32 = dwc_readl(&hc_regs->hcint);
				printk("===>hc%d intr 0x%08x\n", i, hcint.d32);
			}
		}

		dwc->do_reset_core = 1;
		if (dwc2_do_reset_device_core(dwc))
			goto out;
	}

	dwc2_handle_common_interrupts(dwc, &gintr_status);

	if (dwc2_is_device_mode(dwc)) {
#if DWC2_DEVICE_MODE_ENABLE
		dwc2_handle_device_mode_interrupt(dwc, &gintr_status);
		if (unlikely(dwc2_do_reset_device_core(dwc)))
			goto out;
#endif
	} else {
#if DWC2_HOST_MODE_ENABLE
		dwc2_handle_host_mode_interrupt(dwc, &gintr_status);
#endif
	}

	if (unlikely(gintr_status.d32)) {
		printk("===>unhandled interrupt! gintr_status = 0x%08x\n",
			gintr_status.d32);
	}

out:
	dwc_unlock(dwc);
	spin_unlock(&dwc->lock);

	return IRQ_HANDLED;
}

static int dwc2_probe(struct platform_device *pdev)
{
	struct resource			*res;
	struct dwc2			*dwc;
	struct device			*dev   = &pdev->dev;
	int				 irq;
	int				 ret   = -ENOMEM;
	struct dwc2_platform_data	*pdata = pdev->dev.platform_data;
	void __iomem			*regs;

	dwc = devm_kzalloc(dev, sizeof(*dwc), GFP_KERNEL);
	if (!dwc) {
		dev_err(dev, "not enough memory\n");
		return -ENOMEM;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq == -ENXIO) {
		dev_err(dev, "missing IRQ\n");
		return -ENODEV;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "missing memory resource\n");
		return -ENODEV;
	}

	res = devm_request_mem_region(dev, res->start, resource_size(res), dev_name(dev));
	if (!res) {
		dev_err(dev, "can't request mem region\n");
		return -ENOMEM;
	}

	regs = devm_ioremap_nocache(dev, res->start, resource_size(res));
	if (!regs) {
		dev_err(dev, "ioremap failed\n");
		return -ENOMEM;
	}

	spin_lock_init(&dwc->lock);
	atomic_set(&dwc->in_irq, 0);
	INIT_WORK(&dwc->otg_id_work, dwc2_conn_id_status_change_work);

	platform_set_drvdata(pdev, dwc);

	dwc->regs = regs;
	dwc->pdev = pdev;
	dwc->dev  = dev;

	dwc->keep_phy_on = !!pdata->keep_phy_on;

	if (dwc->keep_phy_on)
		dev_info(dwc->dev, "Keep PHY ON\n");
	else
		dev_info(dwc->dev, "Dynamic Power Control\n");

	dwc->dma_enable	     = 1;
	dwc->dma_desc_enable = 0;

	if (dwc->dma_enable) {
		if (dwc->dma_desc_enable) {
			dev_info(dwc->dev, "Using Descriptor DMA mode\n");
		} else {
			dev_info(dwc->dev, "Using Buffer DMA mode\n");
		}
	} else {
		dev_info(dwc->dev, "Using Slave mode\n");
		dwc->dma_desc_enable = 0;
	}

	/*
	 * Set OTG version supported
	 * 0: OTG 1.3
	 * 1: OTG 2.0
	 */
	dwc->otg_ver = 1;

	dwc2_init_csr(dwc);
	dwc2_disable_global_interrupts(dwc);

	ret = dwc2_core_init(dwc);
	if (ret) {
		dev_err(dev, "failed to initialize core\n");
		goto fail_init_core;
	}


#if DWC2_HOST_MODE_ENABLE
	ret = dwc2_host_init(dwc);
	if (ret) {
		dev_err(dev, "failed to initialize host\n");
		goto fail_init_host;
	}
#endif

#if DWC2_DEVICE_MODE_ENABLE
	ret = dwc2_gadget_init(dwc);
	if (ret) {
		dev_err(dev, "failed to initialize gadget\n");
		goto fail_init_gadget;
	}
#endif

	ret = devm_request_irq(dwc->dev, irq, dwc2_interrupt, 0, "dwc2", dwc);
	if (ret) {
		dev_err(dwc->dev, "failed to request irq #%d --> %d\n",
			irq, ret);
		goto fail_req_irq;
	}
	dwc->irq = irq;

	ret = dwc2_debugfs_init(dwc);
	if (ret) {
		dev_err(dev, "failed to initialize debugfs\n");
		goto fail_init_debugfs;
	}

	dwc2_enable_global_interrupts(dwc);
	return 0;

fail_init_debugfs:
fail_req_irq:
#if DWC2_DEVICE_MODE_ENABLE
	dwc2_gadget_exit(dwc);
fail_init_gadget:
#endif
#if DWC2_HOST_MODE_ENABLE
	dwc2_host_exit(dwc);
fail_init_host:
#endif
	dwc2_core_exit(dwc);
fail_init_core:

	return ret;
}

static int dwc2_remove(struct platform_device *pdev)
{
	struct dwc2	*dwc = platform_get_drvdata(pdev);

#if DWC2_HOST_MODE_ENABLE
	dwc2_host_exit(dwc);
#endif
#if DWC2_DEVICE_MODE_ENABLE
	dwc2_gadget_exit(dwc);
#endif
	dwc2_core_exit(dwc);

	return 0;
}

#ifdef CONFIG_PM
static int dwc2_suspend(struct platform_device *pdev, pm_message_t state) {
	struct dwc2	*dwc = platform_get_drvdata(pdev);
	unsigned long	 flags;

	dwc2_spin_lock_irqsave(dwc, flags);
	dwc->suspended = 1;
	dwc2_core_debug_en = 1;
	dwc2_turn_off(dwc, false);
	dwc2_spin_unlock_irqrestore(dwc, flags);
	jz_set_vbus(dwc, 0);
	dev_dbg(dwc->dev, "dwc2_suspend-ed\n");
	return 0;
}

extern void dwc2_start_ep0state_watcher(struct dwc2 *dwc, int count);
static int dwc2_resume(struct platform_device *pdev) {
	struct dwc2	*dwc = platform_get_drvdata(pdev);
	unsigned long	 flags;
	bool host_id  = 1;

	dwc2_gpio_irq_mutex_lock(dwc);
	host_id = dwc2_get_id_level(dwc);

	dwc2_spin_lock_irqsave(dwc, flags);
	dwc->suspended = 0;
#if !IS_ENABLED(CONFIG_USB_DWC2_HOST_ONLY)
	if (dwc->ep0state != EP0_DISCONNECTED) {
		dwc2_gadget_handle_session_end(dwc);
		dwc2_gadget_disconnect(dwc);
		dwc2_start_ep0state_watcher(dwc, 0);
	}
#endif

	if (dwc->plugin || dwc->keep_phy_on || !host_id)
		dwc2_turn_on(dwc);

#if !IS_ENABLED(CONFIG_USB_DWC2_HOST_ONLY)
	if (dwc->plugin && dwc->pullup_on && dwc2_is_device_mode(dwc)) {
		dctl_data_t	 dctl;
		dctl.d32 = dwc_readl(&dwc->dev_if.dev_global_regs->dctl);
		dctl.b.sftdiscon = 0;
		dwc_writel(dctl.d32, &dwc->dev_if.dev_global_regs->dctl);
		dwc2_start_ep0state_watcher(dwc, DWC2_EP0STATE_WATCH_COUNT);
	}
#endif

	dwc2_core_debug_en = 0;
	dwc2_spin_unlock_irqrestore(dwc, flags);
	dwc2_gpio_irq_mutex_unlock(dwc);
	return 0;
}

#else  /* CONFIG_PM */
#define dwc2_suspend	NULL
#define dwc2_resume	NULL
#endif



static struct platform_driver dwc2_driver = {
	.probe		= dwc2_probe,
	.remove		= dwc2_remove,
	.suspend	= dwc2_suspend,
	.resume		= dwc2_resume,
	.driver		= {
		.name	= "dwc2",
	},
};

int dwc2_init(void)
{
	return platform_driver_register(&dwc2_driver);
}

void dwc2_exit(void)
{
	platform_driver_unregister(&dwc2_driver);
}
