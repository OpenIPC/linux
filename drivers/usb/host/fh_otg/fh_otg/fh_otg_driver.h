/* ==========================================================================
 * $File: //dwh/usb_iip/dev/software/otg/linux/drivers/fh_otg_driver.h $
 * $Revision: #21 $
 * $Date: 2015/10/12 $
 * $Change: 2972621 $
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

#ifndef __FH_OTG_DRIVER_H__
#define __FH_OTG_DRIVER_H__

/** @file
 * This file contains the interface to the Linux driver.
 */
#include "fh_otg_os_dep.h"
#include "fh_otg_core_if.h"

/* Type declarations */
struct fh_otg_pcd;
struct fh_otg_hcd;

/**
 * This structure is a wrapper that encapsulates the driver components used to
 * manage a single FH_otg controller.
 */
typedef struct fh_otg_device {
	/** Structure containing OS-dependent stuff. KEEP THIS STRUCT AT THE
	 * VERY BEGINNING OF THE DEVICE STRUCT. OSes such as FreeBSD and NetBSD
	 * require this. */
	struct os_dependent os_dep;

	/** Pointer to the core interface structure. */
	fh_otg_core_if_t *core_if;

	/** Pointer to the PCD structure. */
	struct fh_otg_pcd *pcd;

	/** Pointer to the HCD structure. */
	struct fh_otg_hcd *hcd;

	/** Flag to indicate whether the common IRQ handler is installed. */
	uint8_t common_irq_installed;

} fh_otg_device_t;

/*We must clear S3C24XX_EINTPEND external interrupt register 
 * because after clearing in this register trigerred IRQ from 
 * H/W core in kernel interrupt can be occured again before OTG
 * handlers clear all IRQ sources of Core registers because of
 * timing latencies and Low Level IRQ Type.
 */
#ifdef CONFIG_MACH_IPMATE
#define  S3C2410X_CLEAR_EINTPEND()   \
do { \
	__raw_writel(1UL << 11,S3C24XX_EINTPEND); \
} while (0)
#else
#define  S3C2410X_CLEAR_EINTPEND()   do { } while (0)
#endif

#endif
