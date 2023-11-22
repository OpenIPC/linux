/*
 * bc-sstar.h- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */

#ifndef _BC_SSTAR_H_
#define _BC_SSTAR_H_

extern void usb_bc_enable(struct usb_hcd *hcd, bool enable);
extern int  apple_charger_support(struct usb_hcd *hcd, struct usb_device *udev);

extern void usb_power_saving_enable(struct usb_hcd *hcd, bool enable);
extern void usb20mac_sram_power_saving(struct usb_hcd *hcd, bool enable);
extern void usb30mac_sram_power_saving(struct usb_hcd *hcd, bool enable);
extern void dma_ehci_enable_over4GB(struct usb_hcd *hcd, dma_addr_t dma);
#endif /* _BC_SSTAR_H_ */