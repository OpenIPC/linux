/* ==========================================================================
 * $File: //dwh/usb_iip/dev/software/otg/linux/drivers/fh_otg_driver.c $
 * $Revision: #105 $
 * $Date: 2015/10/13 $
 * $Change: 2974245 $
 *
 * Synopsys HS OTG Linux Software Driver and documentation (hereinafter,
 * "Software") is an Unsupported proprietary work of Synopsys, Inc. unless
 * otherwise expressly agreed to in writing between Synopsys and you.
 *
 * The Software IS NOT an item of Licensed Software or Licensed Product under
 * any End User Software License Agreement or Agreement for Licensed Product
 * with Synopsys or any supplement thereto. You are permitted to use and
 * redistribute this Software in source and binary forms, with or without
 * modification, provided that redistributions of source code must retain this
 * notice. You may not view, use, disclose, copy or distribute this file or
 * any information contained herein except pursuant to this license grant from
 * Synopsys. If you do not agree with this notice, including the disclaimer
 * below, then you are not authorized to use the Software.
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
 * ========================================================================== */

/** @file
 * The fh_otg_driver module provides the initialization and cleanup entry
 * points for the FH_otg driver. This module will be dynamically installed
 * after Linux is booted using the insmod command. When the module is
 * installed, the fh_otg_driver_init function is called. When the module is
 * removed (using rmmod), the fh_otg_driver_cleanup function is called.
 *
 * This module also defines a data structure for the fh_otg_driver, which is
 * used in conjunction with the standard ARM lm_device structure. These
 * structures allow the OTG driver to comply with the standard Linux driver
 * model in which devices and drivers are registered with a bus driver. This
 * has the benefit that Linux can expose attributes of the driver and device
 * in its special sysfs file system. Users can then read or write files in
 * this file system to perform diagnostics on the driver components or the
 * device.
 */

#include <linux/platform_device.h>

#include "fh_otg_os_dep.h"
#include "../fh_common_port/fh_os.h"
#include "fh_otg_dbg.h"
#include "fh_otg_driver.h"
#include "fh_otg_attr.h"
#include "fh_otg_core_if.h"
#include "fh_otg_pcd_if.h"
#include "fh_otg_hcd_if.h"

#define FH_DRIVER_VERSION	"3.30a 13-OCT-2015"
#define FH_DRIVER_DESC		"HS OTG USB Controller driver"

static const char fh_driver_name[] = "fh_otg";
int g_irq;

extern int pcd_init(struct platform_device *dev, int irq);

extern int hcd_init(struct platform_device *dev, int irq);

extern int pcd_remove(struct platform_device *dev, int irq);

extern void hcd_remove(struct platform_device *dev);

extern void fh_otg_adp_start(fh_otg_core_if_t * core_if, uint8_t is_host);

/*-------------------------------------------------------------------------*/
/* Encapsulate the module parameter settings */

struct fh_otg_driver_module_params {
	int32_t opt;
	int32_t otg_cap;
	int32_t dma_enable;
	int32_t dma_desc_enable;
	int32_t dma_burst_size;
	int32_t speed;
	int32_t host_support_fs_ls_low_power;
	int32_t host_ls_low_power_phy_clk;
	int32_t enable_dynamic_fifo;
	int32_t data_fifo_size;
	int32_t dev_rx_fifo_size;
	int32_t dev_nperio_tx_fifo_size;
	uint32_t dev_perio_tx_fifo_size[MAX_PERIO_FIFOS];
	int32_t host_rx_fifo_size;
	int32_t host_nperio_tx_fifo_size;
	int32_t host_perio_tx_fifo_size;
	int32_t max_transfer_size;
	int32_t max_packet_count;
	int32_t host_channels;
	int32_t dev_endpoints;
	int32_t phy_type;
	int32_t phy_utmi_width;
	int32_t phy_ulpi_ddr;
	int32_t phy_ulpi_ext_vbus;
	int32_t i2c_enable;
	int32_t ulpi_fs_ls;
	int32_t ts_dline;
	int32_t en_multiple_tx_fifo;
	uint32_t dev_tx_fifo_size[MAX_TX_FIFOS];
	uint32_t thr_ctl;
	uint32_t tx_thr_length;
	uint32_t rx_thr_length;
	int32_t pti_enable;
	int32_t mpi_enable;
	int32_t lpm_enable;
	int32_t besl_enable;
	int32_t baseline_besl;
	int32_t deep_besl;
	int32_t ic_usb_cap;
	int32_t ahb_thr_ratio;
	int32_t power_down;
	int32_t reload_ctl;
	int32_t dev_out_nak;
	int32_t cont_on_bna;
	int32_t ahb_single;
	int32_t otg_ver;
	int32_t adp_enable;
};

static struct fh_otg_driver_module_params fh_otg_module_params = {
	.opt = -1,
	.otg_cap = 0,
	.dma_enable = 1,
	.dma_desc_enable = 1,
	.dma_burst_size = -1,
	.speed = 0,
	.host_support_fs_ls_low_power = 0,
	.host_ls_low_power_phy_clk = 0,
	.enable_dynamic_fifo = 1,
	.data_fifo_size = -1,
	.dev_rx_fifo_size = 549,
	.dev_nperio_tx_fifo_size = 256,
	.dev_perio_tx_fifo_size = {
				   /* dev_perio_tx_fifo_size_1 */
				   32,
				   256,
				   256,
				   256,
				   256,
				   256,
				   16,
				   16,
				   16,
				   16,
				   16,
				   16,
				   16,
				   16,
				   16 
				   /* 15 */
				   },
	.host_rx_fifo_size = 542,
	.host_nperio_tx_fifo_size = 256,
	.host_perio_tx_fifo_size = 512,
	.max_transfer_size = 65535,
	.max_packet_count = 511,
	.host_channels = 16,
	.dev_endpoints = 5,
	.phy_type = 1,
	.phy_utmi_width = 8,
	.phy_ulpi_ddr = 0,
	.phy_ulpi_ext_vbus = 0,
	.i2c_enable = 0,
	.ulpi_fs_ls = 0,
	.ts_dline = 0,
	.en_multiple_tx_fifo = 1,
	.dev_tx_fifo_size = {
			     /* dev_tx_fifo_size */
			     32,
			     256,
			     256,
			     256,
			     256,
			     256,
			     16,
			     16,
			     16,
			     16,
			     16,
			     16,
			     16,
			     16,
			     16
			     /* 15 */
			     },
	.thr_ctl = 0,
	.tx_thr_length = -1,
	.rx_thr_length = -1,
	.pti_enable = 0,
	.mpi_enable = 0,
	.lpm_enable = 0,
	.besl_enable = 0,
	.baseline_besl = 0,
	.deep_besl = -1,
	.ic_usb_cap = 0,
	.ahb_thr_ratio = 0,
	.power_down = 0,
	.reload_ctl = 0,
	.dev_out_nak = 0,
	.cont_on_bna = 0,
	.ahb_single = 0,
	.otg_ver = 0,
	.adp_enable = -1,
};


/**
 * Global Debug Level Mask.
 */
uint32_t g_dbg_lvl = 0;		/* OFF */


/**
 * This function is called during module intialization
 * to pass module parameters to the FH_OTG CORE.
 */
static int set_parameters(fh_otg_core_if_t * core_if)
{
	int retval = 0;
	int i;

	if (fh_otg_module_params.otg_cap != -1) {
		retval +=
		    fh_otg_set_param_otg_cap(core_if,
					      fh_otg_module_params.otg_cap);
	}
	printk(KERN_ERR "dma_enable :%d\n", fh_otg_module_params.dma_enable);
	if (fh_otg_module_params.dma_enable != -1) {
		retval +=
		    fh_otg_set_param_dma_enable(core_if,
						 fh_otg_module_params.
						 dma_enable);
	}
	printk(KERN_ERR "dma_desc_enable :%d\n", fh_otg_module_params.dma_desc_enable);
	if (fh_otg_module_params.dma_desc_enable != -1) {
		retval +=
		    fh_otg_set_param_dma_desc_enable(core_if,
						      fh_otg_module_params.
						      dma_desc_enable);
	}
	if (fh_otg_module_params.opt != -1) {
		retval +=
		    fh_otg_set_param_opt(core_if, fh_otg_module_params.opt);
	}
	if (fh_otg_module_params.dma_burst_size != -1) {
		retval +=
		    fh_otg_set_param_dma_burst_size(core_if,
						     fh_otg_module_params.
						     dma_burst_size);
	}
	if (fh_otg_module_params.host_support_fs_ls_low_power != -1) {
		retval +=
		    fh_otg_set_param_host_support_fs_ls_low_power(core_if,
								   fh_otg_module_params.
								   host_support_fs_ls_low_power);
	}
	if (fh_otg_module_params.enable_dynamic_fifo != -1) {
		retval +=
		    fh_otg_set_param_enable_dynamic_fifo(core_if,
							  fh_otg_module_params.
							  enable_dynamic_fifo);
	}
	if (fh_otg_module_params.data_fifo_size != -1) {
		retval +=
		    fh_otg_set_param_data_fifo_size(core_if,
						     fh_otg_module_params.
						     data_fifo_size);
	}
	if (fh_otg_module_params.dev_rx_fifo_size != -1) {
		retval +=
		    fh_otg_set_param_dev_rx_fifo_size(core_if,
						       fh_otg_module_params.
						       dev_rx_fifo_size);
	}
	if (fh_otg_module_params.dev_nperio_tx_fifo_size != -1) {
		retval +=
		    fh_otg_set_param_dev_nperio_tx_fifo_size(core_if,
							      fh_otg_module_params.
							      dev_nperio_tx_fifo_size);
	}
	if (fh_otg_module_params.host_rx_fifo_size != -1) {
		retval +=
		    fh_otg_set_param_host_rx_fifo_size(core_if,
							fh_otg_module_params.host_rx_fifo_size);
	}
	if (fh_otg_module_params.host_nperio_tx_fifo_size != -1) {
		retval +=
		    fh_otg_set_param_host_nperio_tx_fifo_size(core_if,
							       fh_otg_module_params.
							       host_nperio_tx_fifo_size);
	}
	if (fh_otg_module_params.host_perio_tx_fifo_size != -1) {
		retval +=
		    fh_otg_set_param_host_perio_tx_fifo_size(core_if,
							      fh_otg_module_params.
							      host_perio_tx_fifo_size);
	}
	if (fh_otg_module_params.max_transfer_size != -1) {
		retval +=
		    fh_otg_set_param_max_transfer_size(core_if,
							fh_otg_module_params.
							max_transfer_size);
	}
	if (fh_otg_module_params.max_packet_count != -1) {
		retval +=
		    fh_otg_set_param_max_packet_count(core_if,
						       fh_otg_module_params.
						       max_packet_count);
	}
	if (fh_otg_module_params.host_channels != -1) {
		retval +=
		    fh_otg_set_param_host_channels(core_if,
						    fh_otg_module_params.
						    host_channels);
	}
	if (fh_otg_module_params.dev_endpoints != -1) {
		retval +=
		    fh_otg_set_param_dev_endpoints(core_if,
						    fh_otg_module_params.
						    dev_endpoints);
	}
	if (fh_otg_module_params.phy_type != -1) {
		retval +=
		    fh_otg_set_param_phy_type(core_if,
					       fh_otg_module_params.phy_type);
	}
	if (fh_otg_module_params.speed != -1) {
		retval +=
		    fh_otg_set_param_speed(core_if,
					    fh_otg_module_params.speed);
	}
	if (fh_otg_module_params.host_ls_low_power_phy_clk != -1) {
		retval +=
		    fh_otg_set_param_host_ls_low_power_phy_clk(core_if,
								fh_otg_module_params.
								host_ls_low_power_phy_clk);
	}
	if (fh_otg_module_params.phy_ulpi_ddr != -1) {
		retval +=
		    fh_otg_set_param_phy_ulpi_ddr(core_if,
						   fh_otg_module_params.
						   phy_ulpi_ddr);
	}
	if (fh_otg_module_params.phy_ulpi_ext_vbus != -1) {
		retval +=
		    fh_otg_set_param_phy_ulpi_ext_vbus(core_if,
							fh_otg_module_params.
							phy_ulpi_ext_vbus);
	}
	if (fh_otg_module_params.phy_utmi_width != -1) {
		retval +=
		    fh_otg_set_param_phy_utmi_width(core_if,
						     fh_otg_module_params.
						     phy_utmi_width);
	}
	if (fh_otg_module_params.ulpi_fs_ls != -1) {
		retval +=
		    fh_otg_set_param_ulpi_fs_ls(core_if,
						 fh_otg_module_params.ulpi_fs_ls);
	}
	if (fh_otg_module_params.ts_dline != -1) {
		retval +=
		    fh_otg_set_param_ts_dline(core_if,
					       fh_otg_module_params.ts_dline);
	}
	if (fh_otg_module_params.i2c_enable != -1) {
		retval +=
		    fh_otg_set_param_i2c_enable(core_if,
						 fh_otg_module_params.
						 i2c_enable);
	}
	if (fh_otg_module_params.en_multiple_tx_fifo != -1) {
		retval +=
		    fh_otg_set_param_en_multiple_tx_fifo(core_if,
							  fh_otg_module_params.
							  en_multiple_tx_fifo);
	}
	for (i = 0; i < 15; i++) {
		if (fh_otg_module_params.dev_perio_tx_fifo_size[i] != -1) {
			retval +=
			    fh_otg_set_param_dev_perio_tx_fifo_size(core_if,
								     fh_otg_module_params.
								     dev_perio_tx_fifo_size
								     [i], i);
		}
	}

	for (i = 0; i < 15; i++) {
		if (fh_otg_module_params.dev_tx_fifo_size[i] != -1) {
			retval += fh_otg_set_param_dev_tx_fifo_size(core_if,
								     fh_otg_module_params.
								     dev_tx_fifo_size
								     [i], i);
		}
	}
	if (fh_otg_module_params.thr_ctl != -1) {
		retval +=
		    fh_otg_set_param_thr_ctl(core_if,
					      fh_otg_module_params.thr_ctl);
	}
	if (fh_otg_module_params.mpi_enable != -1) {
		retval +=
		    fh_otg_set_param_mpi_enable(core_if,
						 fh_otg_module_params.
						 mpi_enable);
	}
	if (fh_otg_module_params.pti_enable != -1) {
		retval +=
		    fh_otg_set_param_pti_enable(core_if,
						 fh_otg_module_params.
						 pti_enable);
	}
	if (fh_otg_module_params.lpm_enable != -1) {
		retval +=
		    fh_otg_set_param_lpm_enable(core_if,
						 fh_otg_module_params.
						 lpm_enable);
	}	
	if (fh_otg_module_params.besl_enable != -1) {
		retval +=
		    fh_otg_set_param_besl_enable(core_if,
						 fh_otg_module_params.
						 besl_enable);
	}
	if (fh_otg_module_params.baseline_besl != -1) {
		retval +=
		    fh_otg_set_param_baseline_besl(core_if,
						 fh_otg_module_params.
						 baseline_besl);
	}
	if (fh_otg_module_params.deep_besl != -1) {
		retval +=
		    fh_otg_set_param_deep_besl(core_if,
						 fh_otg_module_params.
						 deep_besl);
	}		
	if (fh_otg_module_params.ic_usb_cap != -1) {
		retval +=
		    fh_otg_set_param_ic_usb_cap(core_if,
						 fh_otg_module_params.
						 ic_usb_cap);
	}
	if (fh_otg_module_params.tx_thr_length != -1) {
		retval +=
		    fh_otg_set_param_tx_thr_length(core_if,
						    fh_otg_module_params.tx_thr_length);
	}
	if (fh_otg_module_params.rx_thr_length != -1) {
		retval +=
		    fh_otg_set_param_rx_thr_length(core_if,
						    fh_otg_module_params.
						    rx_thr_length);
	}
	if (fh_otg_module_params.ahb_thr_ratio != -1) {
		retval +=
		    fh_otg_set_param_ahb_thr_ratio(core_if,
						    fh_otg_module_params.ahb_thr_ratio);
	}
	if (fh_otg_module_params.power_down != -1) {
		retval +=
		    fh_otg_set_param_power_down(core_if,
						 fh_otg_module_params.power_down);
	}
	if (fh_otg_module_params.reload_ctl != -1) {
		retval +=
		    fh_otg_set_param_reload_ctl(core_if,
						 fh_otg_module_params.reload_ctl);
	}

	if (fh_otg_module_params.dev_out_nak != -1) {
		retval +=
			fh_otg_set_param_dev_out_nak(core_if,
			fh_otg_module_params.dev_out_nak);
	}

	if (fh_otg_module_params.cont_on_bna != -1) {
		retval +=
			fh_otg_set_param_cont_on_bna(core_if,
			fh_otg_module_params.cont_on_bna);
	}

	if (fh_otg_module_params.ahb_single != -1) {
		retval +=
			fh_otg_set_param_ahb_single(core_if,
			fh_otg_module_params.ahb_single);
	}

	if (fh_otg_module_params.otg_ver != -1) {
		retval +=
		    fh_otg_set_param_otg_ver(core_if,
					      fh_otg_module_params.otg_ver);
	}
	if (fh_otg_module_params.adp_enable != -1) {
		retval +=
		    fh_otg_set_param_adp_enable(core_if,
						 fh_otg_module_params.
						 adp_enable);
	}
	return retval;
}


/**
 * This function is the top level interrupt handler for the Common
 * (Device and host modes) interrupts.
 */
static irqreturn_t fh_otg_common_irq(int irq, void *dev)
{
	int32_t retval = IRQ_NONE;

	retval = fh_otg_handle_common_intr(dev);
	if (retval != 0) {
		S3C2410X_CLEAR_EINTPEND();
	}
	return IRQ_RETVAL(retval);
}

/**
 * This function is called when a lm_device is unregistered with the
 * fh_otg_driver. This happens, for example, when the rmmod command is
 * executed. The device may or may not be electrically present. If it is
 * present, the driver stops device processing. Any resources used on behalf
 * of this device are freed.
 *
 * @param _dev
 */
static int fh_otg_driver_remove(struct platform_device *dev)
{
	int retval = 0;
	fh_otg_device_t *otg_dev = platform_get_drvdata(dev);


	printk(KERN_ERR "%s (%p)\n", __func__, dev);

	if (!otg_dev) {
		/* Memory allocation for the fh_otg_device failed. */
		FH_DEBUGPL(DBG_ANY, "%s: otg_dev NULL!\n", __func__);
		return -EINVAL;
	}
#ifndef FH_DEVICE_ONLY
	if (otg_dev->hcd) {
		hcd_remove(dev);
	} else {
		FH_DEBUGPL(DBG_ANY, "%s: otg_dev->hcd NULL!\n", __func__);

	}
#endif

#ifndef FH_HOST_ONLY
	if (otg_dev->pcd) {
		pcd_remove(dev, g_irq);
	} else {
		FH_DEBUGPL(DBG_ANY, "%s: otg_dev->pcd NULL!\n", __func__);

	}
#endif
	/*
	 * Free the IRQ
	 */	 
	if (otg_dev->common_irq_installed) {
		free_irq(g_irq, otg_dev);
	} else {
		FH_DEBUGPL(DBG_ANY, "%s: There is no installed irq!\n", __func__);

	}

	if (otg_dev->core_if) {
		fh_otg_cil_remove(otg_dev->core_if);
	} else {
		FH_DEBUGPL(DBG_ANY, "%s: otg_dev->core_if NULL!\n", __func__);

	}

	/*
	 * Remove the device attributes
	 */
	//fh_otg_attr_remove(dev);

	/*
	 * Return the memory.
	 */
	if (otg_dev->os_dep.base)
		iounmap(otg_dev->os_dep.base);
	FH_FREE(otg_dev);

	/*
	 * Clear the drvdata pointer.
	 */

    release_mem_region(otg_dev->os_dep.rsrc_start, otg_dev->os_dep.rsrc_len);
	platform_set_drvdata(dev, 0);
	g_irq = 0;
	return retval;
}

/**
 * This function is called when an lm_device is bound to a
 * fh_otg_driver. It creates the driver components required to
 * control the device (CIL, HCD, and PCD) and it initializes the
 * device. The driver components are stored in a fh_otg_device
 * structure. A reference to the fh_otg_device is saved in the
 * lm_device. This allows the driver to access the fh_otg_device
 * structure on subsequent calls to driver methods for this device.
 *
 * @param _dev Bus device
 */
static int fh_otg_driver_probe(struct platform_device *dev)
{
	int retval = 0;
	fh_otg_device_t *fh_otg_device;
	struct resource *res;
	int irq;

	//printk(KERN_ERR "fh_otg_driver_probe(%p)\n", dev);

	fh_otg_device = FH_ALLOC(sizeof(fh_otg_device_t));

	if (!fh_otg_device) {
		dev_err(&dev->dev, "kmalloc of fh_otg_device failed\n");
		return -ENOMEM;
	}

	memset(fh_otg_device, 0, sizeof(*fh_otg_device));
	fh_otg_device->os_dep.reg_offset = 0xFFFFFFFF;

	/*
	 * Map the FH_otg Core memory into virtual address space.
	 */
	res = platform_get_resource(dev, IORESOURCE_IRQ, 0);
	if (!res) {
		dev_err(&dev->dev,
			"Found HC with no IRQ. Check %s setup!\n",
			dev_name(&dev->dev));
		return -ENODEV;
	}
	irq = res->start;
	g_irq = irq;
	
	res = platform_get_resource(dev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&dev->dev,
			"Found HC with no register addr. Check %s setup!\n",
			dev_name(&dev->dev));
		return -ENODEV;
	}
	fh_otg_device->os_dep.rsrc_start = res->start;
	fh_otg_device->os_dep.rsrc_len = res->end - res->start + 1;


	printk(KERN_ERR "resource: start=%08x, len=%08x\n",
		    (unsigned)fh_otg_device->os_dep.rsrc_start,
		    (unsigned)fh_otg_device->os_dep.rsrc_len);
	
	if (!request_mem_region
	    (fh_otg_device->os_dep.rsrc_start, fh_otg_device->os_dep.rsrc_len,
	     "fh_otg")) {
		dev_dbg(&dev->dev, "error requesting memory\n");
		FH_FREE(fh_otg_device);
		return -EFAULT;
	}

	fh_otg_device->os_dep.base =
	    ioremap_nocache(fh_otg_device->os_dep.rsrc_start,
			    fh_otg_device->os_dep.rsrc_len);
	if (fh_otg_device->os_dep.base == NULL) {
		dev_dbg(&dev->dev, "error mapping memory\n");
		release_mem_region(fh_otg_device->os_dep.rsrc_start,
				   fh_otg_device->os_dep.rsrc_len);
		FH_FREE(fh_otg_device);
		return -EFAULT;
	}
	//printk(KERN_INFO "base=0x%p (before adjust) \n",
	//	fh_otg_device->os_dep.base);
	fh_otg_device->os_dep.base = (char *)fh_otg_device->os_dep.base;
	printk(KERN_ERR "base=0x%p (after adjust) \n",
		fh_otg_device->os_dep.base);
	printk(KERN_INFO "%s: mapped PA 0x%x to VA 0x%p\n", __func__,
		(unsigned)fh_otg_device->os_dep.rsrc_start,
		fh_otg_device->os_dep.base);

	/*
	 * Initialize driver data to point to the global FH_otg
	 * Device structure.
	 */

	dev_dbg(&dev->dev, "fh_otg_device=0x%p\n", fh_otg_device);

	fh_otg_device->core_if = fh_otg_cil_init(fh_otg_device->os_dep.base);
	if (!fh_otg_device->core_if) {
		dev_err(&dev->dev, "CIL initialization failed!\n");
		retval = -ENOMEM;
		goto fail;
	}

	/*
	 * Attempt to ensure this device is really a FH_otg Controller.
	 * Read and verify the SNPSID register contents. The value should be
	 * 0x45F42XXX or 0x45F42XXX, which corresponds to either "OT2" or "OTG3",
	 * as in "OTG version 2.XX" or "OTG version 3.XX".
	 */

	if (((fh_otg_get_gsnpsid(fh_otg_device->core_if) & 0xFFFFF000) !=	0x4F542000) &&
		((fh_otg_get_gsnpsid(fh_otg_device->core_if) & 0xFFFFF000) != 0x4F543000) && 
		((fh_otg_get_gsnpsid(fh_otg_device->core_if) & 0xFFFFF000) != 0x4F544000)) {
		dev_err(&dev->dev, "Bad value for SNPSID: 0x%08x\n",
			fh_otg_get_gsnpsid(fh_otg_device->core_if));
		retval = -EINVAL;
		goto fail;
	}

	/*
	 * Validate parameter values.
	 */
	if (set_parameters(fh_otg_device->core_if)) {
		retval = -EINVAL;
		goto fail;
	}
	
	/*
	 * Create Device Attributes in sysfs
	 */
	/*fh_otg_attr_create(dev);*/

	
	/*
	 * Disable the global interrupt until all the interrupt
	 * handlers are installed.
	 */
	fh_otg_disable_global_interrupts(fh_otg_device->core_if);

	/*
	 * Install the interrupt handler for the common interrupts before
	 * enabling common interrupts in core_init below.
	 */
	retval = request_irq(irq, fh_otg_common_irq,
			     IRQF_SHARED | IRQF_DISABLED | IRQ_LEVEL, "fh_otg",
			     fh_otg_device);
	if (retval) {
		FH_ERROR("request of irq%d failed\n", irq);
		retval = -EBUSY;
		goto fail;
	} else {
		fh_otg_device->common_irq_installed = 1;
	}

	/*
	 * Initialize the FH_otg core.
	 */
	fh_otg_core_init(fh_otg_device->core_if);
	fh_otg_device->os_dep.pdev = dev;
	platform_set_drvdata(dev, fh_otg_device);

#ifndef FH_HOST_ONLY
	/*
	 * Initialize the PCD
	 */
	retval = pcd_init(dev, irq);
	if (retval != 0) {
		FH_ERROR("pcd_init failed\n");
		fh_otg_device->pcd = NULL;
		goto fail;
	}
#endif

#ifndef FH_DEVICE_ONLY
	/*
	 * Initialize the HCD
	 */
	retval = hcd_init(dev, irq);
	if (retval != 0) {
		FH_ERROR("hcd_init failed\n");
		fh_otg_device->hcd = NULL;
		goto fail;
	}
#endif

	/*
	 * Enable the global interrupt after all the interrupt
	 * handlers are installed if there is no ADP support else 
	 * perform initial actions required for Internal ADP logic.
	 */
	if (!fh_otg_get_param_adp_enable(fh_otg_device->core_if))	
		fh_otg_enable_global_interrupts(fh_otg_device->core_if);
	else
		fh_otg_adp_start(fh_otg_device->core_if, 
							fh_otg_is_host_mode(fh_otg_device->core_if));


	return 0;

fail:
	fh_otg_driver_remove(dev);
	return retval;
}

static const struct of_device_id fh_of_match_table[] = {
	{ .compatible = "brcm,bcm2835-usb", .data = NULL },
	{ .compatible = "rockchip,rk3066-usb", .data = NULL },
	{ .compatible = "fullhan,fh2", .data = NULL },
	{ .compatible = "samsung,s3c6400-hsotg", .data = NULL},
	{},
};
MODULE_DEVICE_TABLE(of, fh_of_match_table);

static struct platform_driver fh_otg_driver = {
	.driver = {
		.owner = THIS_MODULE,
        .name = fh_driver_name,
        .of_match_table = fh_of_match_table,
	},
		   
	.probe = fh_otg_driver_probe,
	.remove = fh_otg_driver_remove,
};


static int __init fh_otg_driver_init(void)
{
	return platform_driver_register(&fh_otg_driver);
}
module_init(fh_otg_driver_init);

static void __exit fh_otg_driver_cleanup(void)
{
	platform_driver_unregister(&fh_otg_driver);
}

module_exit(fh_otg_driver_cleanup);

MODULE_DESCRIPTION(FH_DRIVER_DESC);
MODULE_AUTHOR("Synopsys Inc.");
MODULE_LICENSE("GPL");

module_param_named(otg_cap, fh_otg_module_params.otg_cap, int, 0444);
MODULE_PARM_DESC(otg_cap, "OTG Capabilities 0=HNP&SRP 1=SRP Only 2=None");
module_param_named(opt, fh_otg_module_params.opt, int, 0444);
MODULE_PARM_DESC(opt, "OPT Mode");
module_param_named(dma_enable, fh_otg_module_params.dma_enable, int, 0444);
MODULE_PARM_DESC(dma_enable, "DMA Mode 0=Slave 1=DMA enabled");

module_param_named(dma_desc_enable, fh_otg_module_params.dma_desc_enable, int,
		   0444);
MODULE_PARM_DESC(dma_desc_enable,
		 "DMA Desc Mode 0=Address DMA 1=DMA Descriptor enabled");

module_param_named(dma_burst_size, fh_otg_module_params.dma_burst_size, int,
		   0444);
MODULE_PARM_DESC(dma_burst_size,
		 "DMA Burst Size 1, 4, 8, 16, 32, 64, 128, 256");
module_param_named(speed, fh_otg_module_params.speed, int, 0444);
MODULE_PARM_DESC(speed, "Speed 0=High Speed 1=Full Speed");
module_param_named(host_support_fs_ls_low_power,
		   fh_otg_module_params.host_support_fs_ls_low_power, int,
		   0444);
MODULE_PARM_DESC(host_support_fs_ls_low_power,
		 "Support Low Power w/FS or LS 0=Support 1=Don't Support");
module_param_named(host_ls_low_power_phy_clk,
		   fh_otg_module_params.host_ls_low_power_phy_clk, int, 0444);
MODULE_PARM_DESC(host_ls_low_power_phy_clk,
		 "Low Speed Low Power Clock 0=48Mhz 1=6Mhz");
module_param_named(enable_dynamic_fifo,
		   fh_otg_module_params.enable_dynamic_fifo, int, 0444);
MODULE_PARM_DESC(enable_dynamic_fifo, "0=cC Setting 1=Allow Dynamic Sizing");
module_param_named(data_fifo_size, fh_otg_module_params.data_fifo_size, int,
		   0444);
MODULE_PARM_DESC(data_fifo_size,
		 "Total number of words in the data FIFO memory 32-32768");
module_param_named(dev_rx_fifo_size, fh_otg_module_params.dev_rx_fifo_size,
		   int, 0444);
MODULE_PARM_DESC(dev_rx_fifo_size, "Number of words in the Rx FIFO 16-32768");
module_param_named(dev_nperio_tx_fifo_size,
		   fh_otg_module_params.dev_nperio_tx_fifo_size, int, 0444);
MODULE_PARM_DESC(dev_nperio_tx_fifo_size,
		 "Number of words in the non-periodic Tx FIFO 16-32768");
module_param_named(dev_perio_tx_fifo_size_1,
		   fh_otg_module_params.dev_perio_tx_fifo_size[0], int, 0444);
MODULE_PARM_DESC(dev_perio_tx_fifo_size_1,
		 "Number of words in the periodic Tx FIFO 4-768");
module_param_named(dev_perio_tx_fifo_size_2,
		   fh_otg_module_params.dev_perio_tx_fifo_size[1], int, 0444);
MODULE_PARM_DESC(dev_perio_tx_fifo_size_2,
		 "Number of words in the periodic Tx FIFO 4-768");
module_param_named(dev_perio_tx_fifo_size_3,
		   fh_otg_module_params.dev_perio_tx_fifo_size[2], int, 0444);
MODULE_PARM_DESC(dev_perio_tx_fifo_size_3,
		 "Number of words in the periodic Tx FIFO 4-768");
module_param_named(dev_perio_tx_fifo_size_4,
		   fh_otg_module_params.dev_perio_tx_fifo_size[3], int, 0444);
MODULE_PARM_DESC(dev_perio_tx_fifo_size_4,
		 "Number of words in the periodic Tx FIFO 4-768");
module_param_named(dev_perio_tx_fifo_size_5,
		   fh_otg_module_params.dev_perio_tx_fifo_size[4], int, 0444);
MODULE_PARM_DESC(dev_perio_tx_fifo_size_5,
		 "Number of words in the periodic Tx FIFO 4-768");
module_param_named(dev_perio_tx_fifo_size_6,
		   fh_otg_module_params.dev_perio_tx_fifo_size[5], int, 0444);
MODULE_PARM_DESC(dev_perio_tx_fifo_size_6,
		 "Number of words in the periodic Tx FIFO 4-768");
module_param_named(dev_perio_tx_fifo_size_7,
		   fh_otg_module_params.dev_perio_tx_fifo_size[6], int, 0444);
MODULE_PARM_DESC(dev_perio_tx_fifo_size_7,
		 "Number of words in the periodic Tx FIFO 4-768");
module_param_named(dev_perio_tx_fifo_size_8,
		   fh_otg_module_params.dev_perio_tx_fifo_size[7], int, 0444);
MODULE_PARM_DESC(dev_perio_tx_fifo_size_8,
		 "Number of words in the periodic Tx FIFO 4-768");
module_param_named(dev_perio_tx_fifo_size_9,
		   fh_otg_module_params.dev_perio_tx_fifo_size[8], int, 0444);
MODULE_PARM_DESC(dev_perio_tx_fifo_size_9,
		 "Number of words in the periodic Tx FIFO 4-768");
module_param_named(dev_perio_tx_fifo_size_10,
		   fh_otg_module_params.dev_perio_tx_fifo_size[9], int, 0444);
MODULE_PARM_DESC(dev_perio_tx_fifo_size_10,
		 "Number of words in the periodic Tx FIFO 4-768");
module_param_named(dev_perio_tx_fifo_size_11,
		   fh_otg_module_params.dev_perio_tx_fifo_size[10], int, 0444);
MODULE_PARM_DESC(dev_perio_tx_fifo_size_11,
		 "Number of words in the periodic Tx FIFO 4-768");
module_param_named(dev_perio_tx_fifo_size_12,
		   fh_otg_module_params.dev_perio_tx_fifo_size[11], int, 0444);
MODULE_PARM_DESC(dev_perio_tx_fifo_size_12,
		 "Number of words in the periodic Tx FIFO 4-768");
module_param_named(dev_perio_tx_fifo_size_13,
		   fh_otg_module_params.dev_perio_tx_fifo_size[12], int, 0444);
MODULE_PARM_DESC(dev_perio_tx_fifo_size_13,
		 "Number of words in the periodic Tx FIFO 4-768");
module_param_named(dev_perio_tx_fifo_size_14,
		   fh_otg_module_params.dev_perio_tx_fifo_size[13], int, 0444);
MODULE_PARM_DESC(dev_perio_tx_fifo_size_14,
		 "Number of words in the periodic Tx FIFO 4-768");
module_param_named(dev_perio_tx_fifo_size_15,
		   fh_otg_module_params.dev_perio_tx_fifo_size[14], int, 0444);
MODULE_PARM_DESC(dev_perio_tx_fifo_size_15,
		 "Number of words in the periodic Tx FIFO 4-768");
module_param_named(host_rx_fifo_size, fh_otg_module_params.host_rx_fifo_size,
		   int, 0444);
MODULE_PARM_DESC(host_rx_fifo_size, "Number of words in the Rx FIFO 16-32768");
module_param_named(host_nperio_tx_fifo_size,
		   fh_otg_module_params.host_nperio_tx_fifo_size, int, 0444);
MODULE_PARM_DESC(host_nperio_tx_fifo_size,
		 "Number of words in the non-periodic Tx FIFO 16-32768");
module_param_named(host_perio_tx_fifo_size,
		   fh_otg_module_params.host_perio_tx_fifo_size, int, 0444);
MODULE_PARM_DESC(host_perio_tx_fifo_size,
		 "Number of words in the host periodic Tx FIFO 16-32768");
module_param_named(max_transfer_size, fh_otg_module_params.max_transfer_size,
		   int, 0444);
/** @todo Set the max to 512K, modify checks */
MODULE_PARM_DESC(max_transfer_size,
		 "The maximum transfer size supported in bytes 2047-65535");
module_param_named(max_packet_count, fh_otg_module_params.max_packet_count,
		   int, 0444);
MODULE_PARM_DESC(max_packet_count,
		 "The maximum number of packets in a transfer 15-511");
module_param_named(host_channels, fh_otg_module_params.host_channels, int,
		   0444);
MODULE_PARM_DESC(host_channels,
		 "The number of host channel registers to use 1-16");
module_param_named(dev_endpoints, fh_otg_module_params.dev_endpoints, int,
		   0444);
MODULE_PARM_DESC(dev_endpoints,
		 "The number of endpoints in addition to EP0 available for device mode 1-15");
module_param_named(phy_type, fh_otg_module_params.phy_type, int, 0444);
MODULE_PARM_DESC(phy_type, "0=Reserved 1=UTMI+ 2=ULPI");
module_param_named(phy_utmi_width, fh_otg_module_params.phy_utmi_width, int,
		   0444);
MODULE_PARM_DESC(phy_utmi_width, "Specifies the UTMI+ Data Width 8 or 16 bits");
module_param_named(phy_ulpi_ddr, fh_otg_module_params.phy_ulpi_ddr, int, 0444);
MODULE_PARM_DESC(phy_ulpi_ddr,
		 "ULPI at double or single data rate 0=Single 1=Double");
module_param_named(phy_ulpi_ext_vbus, fh_otg_module_params.phy_ulpi_ext_vbus,
		   int, 0444);
MODULE_PARM_DESC(phy_ulpi_ext_vbus,
		 "ULPI PHY using internal or external vbus 0=Internal");
module_param_named(i2c_enable, fh_otg_module_params.i2c_enable, int, 0444);
MODULE_PARM_DESC(i2c_enable, "FS PHY Interface");
module_param_named(ulpi_fs_ls, fh_otg_module_params.ulpi_fs_ls, int, 0444);
MODULE_PARM_DESC(ulpi_fs_ls, "ULPI PHY FS/LS mode only");
module_param_named(ts_dline, fh_otg_module_params.ts_dline, int, 0444);
MODULE_PARM_DESC(ts_dline, "Term select Dline pulsing for all PHYs");
module_param_named(debug, g_dbg_lvl, int, 0444);
MODULE_PARM_DESC(debug, "");

module_param_named(en_multiple_tx_fifo,
		   fh_otg_module_params.en_multiple_tx_fifo, int, 0444);
MODULE_PARM_DESC(en_multiple_tx_fifo,
		 "Dedicated Non Periodic Tx FIFOs 0=disabled 1=enabled");
module_param_named(dev_tx_fifo_size_1,
		   fh_otg_module_params.dev_tx_fifo_size[0], int, 0444);
MODULE_PARM_DESC(dev_tx_fifo_size_1, "Number of words in the Tx FIFO 4-768");
module_param_named(dev_tx_fifo_size_2,
		   fh_otg_module_params.dev_tx_fifo_size[1], int, 0444);
MODULE_PARM_DESC(dev_tx_fifo_size_2, "Number of words in the Tx FIFO 4-768");
module_param_named(dev_tx_fifo_size_3,
		   fh_otg_module_params.dev_tx_fifo_size[2], int, 0444);
MODULE_PARM_DESC(dev_tx_fifo_size_3, "Number of words in the Tx FIFO 4-768");
module_param_named(dev_tx_fifo_size_4,
		   fh_otg_module_params.dev_tx_fifo_size[3], int, 0444);
MODULE_PARM_DESC(dev_tx_fifo_size_4, "Number of words in the Tx FIFO 4-768");
module_param_named(dev_tx_fifo_size_5,
		   fh_otg_module_params.dev_tx_fifo_size[4], int, 0444);
MODULE_PARM_DESC(dev_tx_fifo_size_5, "Number of words in the Tx FIFO 4-768");
module_param_named(dev_tx_fifo_size_6,
		   fh_otg_module_params.dev_tx_fifo_size[5], int, 0444);
MODULE_PARM_DESC(dev_tx_fifo_size_6, "Number of words in the Tx FIFO 4-768");
module_param_named(dev_tx_fifo_size_7,
		   fh_otg_module_params.dev_tx_fifo_size[6], int, 0444);
MODULE_PARM_DESC(dev_tx_fifo_size_7, "Number of words in the Tx FIFO 4-768");
module_param_named(dev_tx_fifo_size_8,
		   fh_otg_module_params.dev_tx_fifo_size[7], int, 0444);
MODULE_PARM_DESC(dev_tx_fifo_size_8, "Number of words in the Tx FIFO 4-768");
module_param_named(dev_tx_fifo_size_9,
		   fh_otg_module_params.dev_tx_fifo_size[8], int, 0444);
MODULE_PARM_DESC(dev_tx_fifo_size_9, "Number of words in the Tx FIFO 4-768");
module_param_named(dev_tx_fifo_size_10,
		   fh_otg_module_params.dev_tx_fifo_size[9], int, 0444);
MODULE_PARM_DESC(dev_tx_fifo_size_10, "Number of words in the Tx FIFO 4-768");
module_param_named(dev_tx_fifo_size_11,
		   fh_otg_module_params.dev_tx_fifo_size[10], int, 0444);
MODULE_PARM_DESC(dev_tx_fifo_size_11, "Number of words in the Tx FIFO 4-768");
module_param_named(dev_tx_fifo_size_12,
		   fh_otg_module_params.dev_tx_fifo_size[11], int, 0444);
MODULE_PARM_DESC(dev_tx_fifo_size_12, "Number of words in the Tx FIFO 4-768");
module_param_named(dev_tx_fifo_size_13,
		   fh_otg_module_params.dev_tx_fifo_size[12], int, 0444);
MODULE_PARM_DESC(dev_tx_fifo_size_13, "Number of words in the Tx FIFO 4-768");
module_param_named(dev_tx_fifo_size_14,
		   fh_otg_module_params.dev_tx_fifo_size[13], int, 0444);
MODULE_PARM_DESC(dev_tx_fifo_size_14, "Number of words in the Tx FIFO 4-768");
module_param_named(dev_tx_fifo_size_15,
		   fh_otg_module_params.dev_tx_fifo_size[14], int, 0444);
MODULE_PARM_DESC(dev_tx_fifo_size_15, "Number of words in the Tx FIFO 4-768");

module_param_named(thr_ctl, fh_otg_module_params.thr_ctl, int, 0444);
MODULE_PARM_DESC(thr_ctl,
		 "Thresholding enable flag bit 0 - non ISO Tx thr., 1 - ISO Tx thr., 2 - Rx thr.- bit 0=disabled 1=enabled");
module_param_named(tx_thr_length, fh_otg_module_params.tx_thr_length, int,
		   0444);
MODULE_PARM_DESC(tx_thr_length, "Tx Threshold length in 32 bit DWORDs");
module_param_named(rx_thr_length, fh_otg_module_params.rx_thr_length, int,
		   0444);
MODULE_PARM_DESC(rx_thr_length, "Rx Threshold length in 32 bit DWORDs");

module_param_named(pti_enable, fh_otg_module_params.pti_enable, int, 0444);
module_param_named(mpi_enable, fh_otg_module_params.mpi_enable, int, 0444);
module_param_named(lpm_enable, fh_otg_module_params.lpm_enable, int, 0444);
MODULE_PARM_DESC(lpm_enable, "LPM Enable 0=LPM Disabled 1=LPM Enabled");

module_param_named(besl_enable, fh_otg_module_params.besl_enable, int, 0444);
MODULE_PARM_DESC(besl_enable, "BESL Enable 0=BESL Disabled 1=BESL Enabled");
module_param_named(baseline_besl, fh_otg_module_params.baseline_besl, int, 0444);
MODULE_PARM_DESC(baseline_besl, "Set the baseline besl value");
module_param_named(deep_besl, fh_otg_module_params.deep_besl, int, 0444);
MODULE_PARM_DESC(deep_besl, "Set the deep besl value");

module_param_named(ic_usb_cap, fh_otg_module_params.ic_usb_cap, int, 0444);
MODULE_PARM_DESC(ic_usb_cap,
		 "IC_USB Capability 0=IC_USB Disabled 1=IC_USB Enabled");
module_param_named(ahb_thr_ratio, fh_otg_module_params.ahb_thr_ratio, int,
		   0444);
MODULE_PARM_DESC(ahb_thr_ratio, "AHB Threshold Ratio");
module_param_named(power_down, fh_otg_module_params.power_down, int, 0444);
MODULE_PARM_DESC(power_down, "Power Down Mode");
module_param_named(reload_ctl, fh_otg_module_params.reload_ctl, int, 0444);
MODULE_PARM_DESC(reload_ctl, "HFIR Reload Control");
module_param_named(dev_out_nak, fh_otg_module_params.dev_out_nak, int, 0444);
MODULE_PARM_DESC(dev_out_nak, "Enable Device OUT NAK");
module_param_named(cont_on_bna, fh_otg_module_params.cont_on_bna, int, 0444);
MODULE_PARM_DESC(cont_on_bna, "Enable Enable Continue on BNA");
module_param_named(ahb_single, fh_otg_module_params.ahb_single, int, 0444);
MODULE_PARM_DESC(ahb_single, "Enable AHB Single Support");
module_param_named(adp_enable, fh_otg_module_params.adp_enable, int, 0444);
MODULE_PARM_DESC(adp_enable, "ADP Enable 0=ADP Disabled 1=ADP Enabled");
module_param_named(otg_ver, fh_otg_module_params.otg_ver, int, 0444);
MODULE_PARM_DESC(otg_ver, "OTG revision supported 0=OTG 1.3 1=OTG 2.0");

/** @page "Module Parameters"
 *
 * The following parameters may be specified when starting the module.
 * These parameters define how the FH_otg controller should be
 * configured. Parameter values are passed to the CIL initialization
 * function fh_otg_cil_init
 *
 * Example: <code>modprobe fh_otg speed=1 otg_cap=1</code>
 *

 <table>
 <tr><td>Parameter Name</td><td>Meaning</td></tr>

 <tr>
 <td>otg_cap</td>
 <td>Specifies the OTG capabilities. The driver will automatically detect the
 value for this parameter if none is specified.
 - 0: HNP and SRP capable (default, if available)
 - 1: SRP Only capable
 - 2: No HNP/SRP capable
 </td></tr>

 <tr>
 <td>dma_enable</td>
 <td>Specifies whether to use slave or DMA mode for accessing the data FIFOs.
 The driver will automatically detect the value for this parameter if none is
 specified.
 - 0: Slave
 - 1: DMA (default, if available)
 </td></tr>

 <tr>
 <td>dma_burst_size</td>
 <td>The DMA Burst size (applicable only for External DMA Mode).
 - Values: 1, 4, 8 16, 32, 64, 128, 256 (default 32)
 </td></tr>

 <tr>
 <td>speed</td>
 <td>Specifies the maximum speed of operation in host and device mode. The
 actual speed depends on the speed of the attached device and the value of
 phy_type.
 - 0: High Speed (default)
 - 1: Full Speed
 </td></tr>

 <tr>
 <td>host_support_fs_ls_low_power</td>
 <td>Specifies whether low power mode is supported when attached to a Full
 Speed or Low Speed device in host mode.
 - 0: Don't support low power mode (default)
 - 1: Support low power mode
 </td></tr>

 <tr>
 <td>host_ls_low_power_phy_clk</td>
 <td>Specifies the PHY clock rate in low power mode when connected to a Low
 Speed device in host mode. This parameter is applicable only if
 HOST_SUPPORT_FS_LS_LOW_POWER is enabled.
 - 0: 48 MHz (default)
 - 1: 6 MHz
 </td></tr>

 <tr>
 <td>enable_dynamic_fifo</td>
 <td> Specifies whether FIFOs may be resized by the driver software.
 - 0: Use cC FIFO size parameters
 - 1: Allow dynamic FIFO sizing (default)
 </td></tr>

 <tr>
 <td>data_fifo_size</td>
 <td>Total number of 4-byte words in the data FIFO memory. This memory
 includes the Rx FIFO, non-periodic Tx FIFO, and periodic Tx FIFOs.
 - Values: 32 to 32768 (default 8192)

 Note: The total FIFO memory depth in the FPGA configuration is 8192.
 </td></tr>

 <tr>
 <td>dev_rx_fifo_size</td>
 <td>Number of 4-byte words in the Rx FIFO in device mode when dynamic
 FIFO sizing is enabled.
 - Values: 16 to 32768 (default 1064)
 </td></tr>

 <tr>
 <td>dev_nperio_tx_fifo_size</td>
 <td>Number of 4-byte words in the non-periodic Tx FIFO in device mode when
 dynamic FIFO sizing is enabled.
 - Values: 16 to 32768 (default 1024)
 </td></tr>

 <tr>
 <td>dev_perio_tx_fifo_size_n (n = 1 to 15)</td>
 <td>Number of 4-byte words in each of the periodic Tx FIFOs in device mode
 when dynamic FIFO sizing is enabled.
 - Values: 4 to 768 (default 256)
 </td></tr>

 <tr>
 <td>host_rx_fifo_size</td>
 <td>Number of 4-byte words in the Rx FIFO in host mode when dynamic FIFO
 sizing is enabled.
 - Values: 16 to 32768 (default 1024)
 </td></tr>

 <tr>
 <td>host_nperio_tx_fifo_size</td>
 <td>Number of 4-byte words in the non-periodic Tx FIFO in host mode when
 dynamic FIFO sizing is enabled in the core.
 - Values: 16 to 32768 (default 1024)
 </td></tr>

 <tr>
 <td>host_perio_tx_fifo_size</td>
 <td>Number of 4-byte words in the host periodic Tx FIFO when dynamic FIFO
 sizing is enabled.
 - Values: 16 to 32768 (default 1024)
 </td></tr>

 <tr>
 <td>max_transfer_size</td>
 <td>The maximum transfer size supported in bytes.
 - Values: 2047 to 65,535 (default 65,535)
 </td></tr>

 <tr>
 <td>max_packet_count</td>
 <td>The maximum number of packets in a transfer.
 - Values: 15 to 511 (default 511)
 </td></tr>

 <tr>
 <td>host_channels</td>
 <td>The number of host channel registers to use.
 - Values: 1 to 16 (default 12)

 Note: The FPGA configuration supports a maximum of 12 host channels.
 </td></tr>

 <tr>
 <td>dev_endpoints</td>
 <td>The number of endpoints in addition to EP0 available for device mode
 operations.
 - Values: 1 to 15 (default 6 IN and OUT)

 Note: The FPGA configuration supports a maximum of 6 IN and OUT endpoints in
 addition to EP0.
 </td></tr>

 <tr>
 <td>phy_type</td>
 <td>Specifies the type of PHY interface to use. By default, the driver will
 automatically detect the phy_type.
 - 0: Full Speed
 - 1: UTMI+ (default, if available)
 - 2: ULPI
 </td></tr>

 <tr>
 <td>phy_utmi_width</td>
 <td>Specifies the UTMI+ Data Width. This parameter is applicable for a
 phy_type of UTMI+. Also, this parameter is applicable only if the
 OTG_HSPHY_WIDTH cC parameter was set to "8 and 16 bits", meaning that the
 core has been configured to work at either data path width.
 - Values: 8 or 16 bits (default 16)
 </td></tr>

 <tr>
 <td>phy_ulpi_ddr</td>
 <td>Specifies whether the ULPI operates at double or single data rate. This
 parameter is only applicable if phy_type is ULPI.
 - 0: single data rate ULPI interface with 8 bit wide data bus (default)
 - 1: double data rate ULPI interface with 4 bit wide data bus
 </td></tr>

 <tr>
 <td>i2c_enable</td>
 <td>Specifies whether to use the I2C interface for full speed PHY. This
 parameter is only applicable if PHY_TYPE is FS.
 - 0: Disabled (default)
 - 1: Enabled
 </td></tr>

 <tr>
 <td>ulpi_fs_ls</td>
 <td>Specifies whether to use ULPI FS/LS mode only.
 - 0: Disabled (default)
 - 1: Enabled
 </td></tr>

 <tr>
 <td>ts_dline</td>
 <td>Specifies whether term select D-Line pulsing for all PHYs is enabled.
 - 0: Disabled (default)
 - 1: Enabled
 </td></tr>
 
 <tr>
 <td>en_multiple_tx_fifo</td>
 <td>Specifies whether dedicatedto tx fifos are enabled for non periodic IN EPs.
 The driver will automatically detect the value for this parameter if none is
 specified.
 - 0: Disabled
 - 1: Enabled (default, if available)
 </td></tr>

 <tr>
 <td>dev_tx_fifo_size_n (n = 1 to 15)</td>
 <td>Number of 4-byte words in each of the Tx FIFOs in device mode
 when dynamic FIFO sizing is enabled.
 - Values: 4 to 768 (default 256)
 </td></tr>

 <tr>
 <td>tx_thr_length</td>
 <td>Transmit Threshold length in 32 bit double words
 - Values: 8 to 128 (default 64)
 </td></tr>

 <tr>
 <td>rx_thr_length</td>
 <td>Receive Threshold length in 32 bit double words
 - Values: 8 to 128 (default 64)
 </td></tr>

<tr>
 <td>thr_ctl</td>
 <td>Specifies whether to enable Thresholding for Device mode. Bits 0, 1, 2 of 
 this parmater specifies if thresholding is enabled for non-Iso Tx, Iso Tx and
 Rx transfers accordingly.
 The driver will automatically detect the value for this parameter if none is
 specified.
 - Values: 0 to 7 (default 0)
 Bit values indicate:
 - 0: Thresholding disabled
 - 1: Thresholding enabled
 </td></tr>

<tr>
 <td>dma_desc_enable</td>
 <td>Specifies whether to enable Descriptor DMA mode.
 The driver will automatically detect the value for this parameter if none is
 specified.
 - 0: Descriptor DMA disabled
 - 1: Descriptor DMA (default, if available)
 </td></tr>

<tr>
 <td>mpi_enable</td>
 <td>Specifies whether to enable MPI enhancement mode.
 The driver will automatically detect the value for this parameter if none is
 specified.
 - 0: MPI disabled (default)
 - 1: MPI enable
 </td></tr>

<tr>
 <td>pti_enable</td>
 <td>Specifies whether to enable PTI enhancement support.
 The driver will automatically detect the value for this parameter if none is
 specified.
 - 0: PTI disabled (default)
 - 1: PTI enable
 </td></tr>

<tr>
 <td>lpm_enable</td>
 <td>Specifies whether to enable LPM support.
 The driver will automatically detect the value for this parameter if none is
 specified.
 - 0: LPM disabled
 - 1: LPM enable (default, if available)
 </td></tr>
  
 <tr>
 <td>besl_enable</td>
 <td>Specifies whether to enable LPM Errata support.
 The driver will automatically detect the value for this parameter if none is
 specified.
 - 0: LPM Errata disabled (default)
 - 1: LPM Errata enable 
 </td></tr>
 
  <tr>
 <td>baseline_besl</td>
 <td>Specifies the baseline besl value.
 - Values: 0 to 15 (default 0)
 </td></tr>
 
  <tr>
 <td>deep_besl</td>
 <td>Specifies the deep besl value.
 - Values: 0 to 15 (default 15)
 </td></tr>

<tr>
 <td>ic_usb_cap</td>
 <td>Specifies whether to enable IC_USB capability.
 The driver will automatically detect the value for this parameter if none is
 specified.
 - 0: IC_USB disabled (default, if available)
 - 1: IC_USB enable 
 </td></tr>

<tr>
 <td>ahb_thr_ratio</td>
 <td>Specifies AHB Threshold ratio.
 - Values: 0 to 3 (default 0)
 </td></tr>

<tr>
 <td>power_down</td>
 <td>Specifies Power Down(Hibernation) Mode.
 The driver will automatically detect the value for this parameter if none is
 specified.
 - 0: Power Down disabled (default)
 - 2: Power Down enabled
 </td></tr>
 
 <tr>
 <td>reload_ctl</td>
 <td>Specifies whether dynamic reloading of the HFIR register is allowed during
 run time. The driver will automatically detect the value for this parameter if
 none is specified. In case the HFIR value is reloaded when HFIR.RldCtrl == 1'b0
 the core might misbehave.
 - 0: Reload Control disabled (default)
 - 1: Reload Control enabled
 </td></tr>

 <tr>
 <td>dev_out_nak</td>
 <td>Specifies whether  Device OUT NAK enhancement enabled or no.
 The driver will automatically detect the value for this parameter if
 none is specified. This parameter is valid only when OTG_EN_DESC_DMA == 1'b1.
 - 0: The core does not set NAK after Bulk OUT transfer complete (default)
 - 1: The core sets NAK after Bulk OUT transfer complete
 </td></tr>

 <tr>
 <td>cont_on_bna</td>
 <td>Specifies whether Enable Continue on BNA enabled or no. 
 After receiving BNA interrupt the core disables the endpoint,when the
 endpoint is re-enabled by the application the  
 - 0: Core starts processing from the DOEPDMA descriptor (default)
 - 1: Core starts processing from the descriptor which received the BNA.
 This parameter is valid only when OTG_EN_DESC_DMA == 1'b1.
 </td></tr>

 <tr>
 <td>ahb_single</td>
 <td>This bit when programmed supports SINGLE transfers for remainder data
 in a transfer for DMA mode of operation. 
 - 0: The remainder data will be sent using INCR burst size (default)
 - 1: The remainder data will be sent using SINGLE burst size.
 </td></tr>

<tr>
 <td>adp_enable</td>
 <td>Specifies whether ADP feature is enabled.
 The driver will automatically detect the value for this parameter if none is
 specified.
 - 0: ADP feature disabled (default)
 - 1: ADP feature enabled
 </td></tr>

  <tr>
 <td>otg_ver</td>
 <td>Specifies whether OTG is performing as USB OTG Revision 2.0 or Revision 1.3
 USB OTG device.
 - 0: OTG 2.0 support disabled (default)
 - 1: OTG 2.0 support enabled 
 </td></tr>

*/
