/*
 * Mstar battery charger
 * Copyright (C) 2012 MStar Inc.
 * Date: Dec 2012
 */

#include <linux/export.h>
#include <linux/usb.h>
#include <linux/usb/hcd.h>
#include <asm/io.h>
#include <linux/string.h>
#include <mstar/mpatch_macro.h>
#include "bc-mstar.h"
#include "../host/ehci-mstar.h"

// Don't modify the following definition. It's used for register value setting.
#define REG_BC_MODE_IPHONE  0x2
#define REG_BC_MODE_IPAD    0x4 

void usb_bc_enable(struct usb_hcd *hcd, bool enable) 			
{
	if(hcd->bc_base == 0)
	{
		printk("This IC do not support USB BC funciton (%s)\n", __func__);
		return;
	}

	if (enable) {
		printk("BC enable \n");
		writeb(readb((void *)(hcd->utmi_base+(0x1*2-1))) | 0x40, (void *)(hcd->utmi_base+(0x1*2-1)));  //IREF_PDN=1¡¦b1. (utmi+0x01[6] )
		writeb(readb((void *)(hcd->bc_base+(0x3*2-1))) | 0x40, (void *)(hcd->bc_base+(0x3*2-1)));  // [6]= reg_host_bc_en
		writeb(readb((void *)(hcd->bc_base+(0xc*2))) | 0x40, (void *)(hcd->bc_base+(0xc*2)));  // [6]= reg_into_host_bc_sw_tri
		writew(0x0000, (void *)(hcd->bc_base));  // [15:0] = bc_ctl_ov_en
		writeb(readb((void *)(hcd->bc_base+(0xa*2))) | 0x80, (void *)(hcd->bc_base+(0xa*2)));  // [7]=reg_bc_switch_en 
		hcd->bc_enable_flag = true;
	}
	else {
		// disable BC
		printk("BC disable \n");
		writeb(readb((void *)(hcd->bc_base+(0xc*2))) & (u8)(~0x40), (void *)(hcd->bc_base+(0xc*2)));  // [6]= reg_into_host_bc_sw_tri
		writeb(readb((void *)(hcd->bc_base+(0x3*2-1))) & (u8)(~0x40), (void *)(hcd->bc_base+(0x3*2-1)));  // [6]= reg_host_bc_en
		writeb(readb((void *)(hcd->utmi_base+(0x1*2-1))) & (u8)(~0x40), (void *)(hcd->utmi_base+(0x1*2-1)));  //IREF_PDN=1¡¦b1. (utmi+0x01[6] )
		hcd->bc_enable_flag = false;
	}

}

int identify_apple_device(struct usb_device *udev)
{
	char product[128];
	char iPhone_str[] = {"iPhone"};
	u16  iPhone_pid[] = {0x1290,0x1292,0x1294,0x1297,0x129c,0x12a0,0x12a8};	
	char iPad_str[] = {"iPad"};
	u16  iPad_pid[] = {0x129a,0x129f,0x12a2,0x12a3,0x12a4,0x12a5,0x12a6,0x12a9};	
	int  length, i;
    
	// check vid
	if (udev->descriptor.idVendor != 0x05AC)
		return 0; 

	// check product string
	if (udev->descriptor.iProduct) {
		length = usb_string(udev, udev->descriptor.iProduct,
				product, 128);
		if (length > 0) {
			printk("[%s]\n", product);        
			if (strstr(product, iPhone_str)) {
					printk("string match\n");	
					return REG_BC_MODE_IPHONE;
		  	}

			if (strstr(product, iPad_str)){
				printk("string match\n");
				return REG_BC_MODE_IPAD;
			}				
		}
	}
    
	// check pid
	for (i=0; i<(sizeof(iPhone_pid)/sizeof(u16)); i++) {
		if (udev->descriptor.idProduct == iPhone_pid[i]) {
			printk("pid match : 0x%x\n", udev->descriptor.idProduct);	
			return REG_BC_MODE_IPHONE;	
		}
	}

	for (i=0; i<(sizeof(iPad_pid)/sizeof(u16)); i++) {
		if (udev->descriptor.idProduct == iPad_pid[i]) {
			printk("pid match : 0x%x\n", udev->descriptor.idProduct);
			return REG_BC_MODE_IPAD;	
		}
	}    

	return 0;
}


void apple_bc_enable(struct usb_hcd *hcd, bool enable, int mode, int *pBackup) 			
{
	if(hcd->bc_base == 0)
	{
		printk("This IC do not support USB BC funciton (%s)\n", __func__);
		return;
	}

	if (enable) {
		if (hcd->bc_apple_enable_flag == false) {
			printk("Apple BC enable \n");
			
			//disable init. 
			if (hcd->ehc_base)
			{
				*pBackup = readb((void *)(hcd->ehc_base+(0x18*2)));
				writeb(readb((void *)(hcd->ehc_base+(0x18*2))) & (u8)(~0x3F) , (void *)(hcd->ehc_base+(0x18*2)));
			} 
			else if (hcd->xhci_base)
			{
				writeb(readb((void *)(hcd->xhci_base+0x2020)) & (u8)(~0x2) , (void *)(hcd->xhci_base+0x2020));
			}
			
			writeb(readb((void *)(hcd->utmi_base)) | 0x2, (void *)(hcd->utmi_base));  // [1]re_term_override

			#if defined(USB_BATTERY_CHARGE_SETTING_1)
			writew(readw((void *)(hcd->bc_base+(0x2*2))) | 0x5080, (void *)(hcd->bc_base+(0x2*2)));  //[14] reg_host_bc_en=1, [12]reg_bc_debug_mask=1
			if (mode == REG_BC_MODE_IPAD) 
				writeb(readb((void *)(hcd->bc_base+(0x8*2))) | 0x2, (void *)(hcd->bc_base+(0x8*2)));  
			if (mode == REG_BC_MODE_IPHONE) 
				writeb(readb((void *)(hcd->bc_base+(0x8*2))) | 0x1, (void *)(hcd->bc_base+(0x8*2)));  			
			#else
			writeb(readb((void *)(hcd->bc_base+(0x3*2-1))) | 0x50, (void *)(hcd->bc_base+(0x3*2-1)));  //[14] reg_host_bc_en=1, [12]reg_bc_debug_mask=1
			#endif
			
			writeb(readb((void *)(hcd->bc_base+(0xc*2))) | 0x40, (void *)(hcd->bc_base+(0xc*2)));  // [6]= reg_into_host_bc_sw_tri
			writew(0x00FF, (void *)(hcd->bc_base));  // [15:0] = bc_ctl_ov_en
			writeb(readb((void *)(hcd->bc_base+(0xe*2))) | (mode|0x1), (void *)(hcd->bc_base+(0xe*2)));  //[0]reg_apple_bc_en=1, [1:2]=2b'10,ipad mode, [1:2]=2b'01,iphone mode
			hcd->bc_apple_enable_flag = true;
		}
	}
	else {
		if (hcd->bc_apple_enable_flag == true) {			
			printk("Apple BC disable \n");

			writeb(readb((void *)(hcd->utmi_base)) & (u8)(~0x2), (void *)(hcd->utmi_base));  // [1]re_term_override
			
			writew(0x0000, (void *)(hcd->bc_base));  // [15:0] = bc_ctl_ov_en			
			writeb(readb((void *)(hcd->bc_base+(0xc*2))) & (u8)(~0x40), (void *)(hcd->bc_base+(0xc*2)));  // [6]= reg_into_host_bc_sw_tri
			
			#if defined(USB_BATTERY_CHARGE_SETTING_1)
			writew(readw((void *)(hcd->bc_base+(0x2*2))) & (u16)(~0x5080), (void *)(hcd->bc_base+(0x2*2)));  //[14] reg_host_bc_en=1, [12]reg_bc_debug_mask=1
			if (mode == REG_BC_MODE_IPAD) 
				writeb(readb((void *)(hcd->bc_base+(0x8*2))) & (u8)(~0x2), (void *)(hcd->bc_base+(0x8*2)));  
			if (mode == REG_BC_MODE_IPHONE) 
				writeb(readb((void *)(hcd->bc_base+(0x8*2))) & (u8)(~0x1), (void *)(hcd->bc_base+(0x8*2)));  
			#else
			writeb(readb((void *)(hcd->bc_base+(0x3*2-1))) & (u8)(~0x50), (void *)(hcd->bc_base+(0x3*2-1)));  // [6]= reg_host_bc_en
			#endif
			
			writeb(readb((void *)(hcd->bc_base+(0xe*2))) & (u8)(~0x1), (void *)(hcd->bc_base+(0xe*2)));  //[0]reg_apple_bc_en=1
			
			///	hcd->bc_apple_enable_flag = false;  //disable it later. 
		
			//enable init. 
			if (hcd->ehc_base)
			{
				writeb(readb((void *)(hcd->ehc_base+(0x18*2))) | (*pBackup) , (void *)(hcd->ehc_base+(0x18*2)));
			}
			else if (hcd->xhci_base)
			{
				writeb(readb((void *)(hcd->xhci_base+0x2020)) | 0x2 , (void *)(hcd->xhci_base+0x2020));
			}
			hcd->bc_apple_enable_flag = false;		
		}
	}

}

void port_power_switch_on(struct usb_hcd *hcd)
{
	if (!hcd->ppc.port_addr)
		return;

	//set output enable
	if (hcd->ppc.out_en_hi_active)
		writeb(readb((void*)(hcd->ppc.port_addr)) | (hcd->ppc.out_en_bit_addr << 1), (void*)(hcd->ppc.port_addr)); 
	else
		writeb(readb((void*)(hcd->ppc.port_addr)) & ~(hcd->ppc.out_en_bit_addr << 1), (void*)(hcd->ppc.port_addr)); 

	// Turn on vBus
	writeb(readb((void*)(hcd->ppc.port_addr)) | (hcd->ppc.bit_addr << 1), (void*)(hcd->ppc.port_addr)); 
}

void port_power_switch_off(struct usb_hcd *hcd)
{
	if (!hcd->ppc.port_addr)
		return;

	//set output enable
	if (hcd->ppc.out_en_hi_active)
		writeb(readb((void*)(hcd->ppc.port_addr)) | (hcd->ppc.out_en_bit_addr << 1), (void*)(hcd->ppc.port_addr)); 
	else
		writeb(readb((void*)(hcd->ppc.port_addr)) & ~(hcd->ppc.out_en_bit_addr << 1), (void*)(hcd->ppc.port_addr)); 

	// Turn off vBus
	writeb(readb((void*)(hcd->ppc.port_addr)) & ~(hcd->ppc.bit_addr << 1), (void*)(hcd->ppc.port_addr)); 
}

int apple_charger_support(struct usb_hcd *hcd, struct usb_device *udev)
{

	int mode, backup;

	if (hcd->bc_apple_enable_flag == true) {		
		return 0;
	}

	//identify_apple_device
	mode = identify_apple_device(udev);
	if (mode ==0)
	{
		//not apple device or supported device
		return 0;
	}

#if 1
	if (!hcd->ppc.port_addr)
	{
		printk("No port power control !\n");	
		return 0;
	}
#endif
	

	//power down vbus
	#if 1
	port_power_switch_off(hcd);
	#else
	//for Edison 159A
	if (hcd->xhci_base)  {
		writeb(readb((void*)(_MSTAR_PM_BASE+0xF1A*2)) &  (u8)(~0x2), (void*)(_MSTAR_PM_BASE+0xF1A*2)); 
	}
	else if (hcd->ehc_base)	
	{
		switch (hcd->port_index)
		{
			case 1:
				writeb(readb((void*)(_MSTAR_PM_BASE+0x102B67*2-1)) & (u8)(~0x3), (void*)(_MSTAR_PM_BASE+0x102B67*2-1)); 
				break;

			case 2:
				writeb(readb((void*)(_MSTAR_PM_BASE+0xF18*2)) & (u8)(~0x3), (void*)(_MSTAR_PM_BASE+0xF18*2)); 
				break;

			case 3:
				writeb(readb((void*)(_MSTAR_PM_BASE+0xF24*2)) & (u8)(~0x3), (void*)(_MSTAR_PM_BASE+0xF24*2)); 
				break;

			default: break;
		}
	}
	#endif

	msleep(800);

	//enable apple charger
	apple_bc_enable(hcd, true, mode, &backup);

	//power on vbus
	#if 1
	port_power_switch_on(hcd);
	#else
	//for Edison 159A
	if (hcd->xhci_base)
	{
		writeb(readb((void*)(_MSTAR_PM_BASE+0xF1A*2)) | (0x2), (void*)(_MSTAR_PM_BASE+0xF1A*2)); 
	}
	else if (hcd->ehc_base)
	{
		switch (hcd->port_index)
		{
			case 1:
				writeb(readb((void*)(_MSTAR_PM_BASE+0x102B67*2-1)) | 0x1, (void*)(_MSTAR_PM_BASE+0x102B67*2-1)); 
				break;

			case 2:
				writeb(readb((void*)(_MSTAR_PM_BASE+0xF18*2)) | 0x2, (void*)(_MSTAR_PM_BASE+0xF18*2)); 
				break;

			case 3:
				writeb(readb((void*)(_MSTAR_PM_BASE+0xF24*2)) | 0x2, (void*)(_MSTAR_PM_BASE+0xF24*2)); 
				break;

			default: break;
		}
	}
	#endif		

	msleep(500);

	//disable apple charger
	apple_bc_enable(hcd, false, mode, &backup);

	return 1;
}


void usb_power_saving_enable(struct usb_hcd *hcd, bool enable)		
{
	if (enable) {
		printk("usb_power_saving enable \n");

		if (hcd->utmi_base != 0)
		{
			//([0]: power down override, [1]:Termination override, [6]:15Kohm pull low for dp, [7] :15Kohm pull low for dm)
			writeb(readb((void*)(hcd->utmi_base)) | (BIT1|BIT6|BIT7), (void*) (hcd->utmi_base));
			//([2]:TED power down, [3]Preamp power down, [6]TX power down)
			writeb(readb((void*)(hcd->utmi_base+0x1*2-1)) | (BIT2|BIT3|BIT6), (void*) (hcd->utmi_base+0x1*2-1));
		}

		#if !defined(ENABLE_BATTERY_CHARGE) 
		if (hcd->bc_base != 0)
		{
			//BC power down
			writeb(0xFF, (void*) (hcd->bc_base));
		}
		#endif
	}
	else {
		printk("usb_power_saving disable \n");

		if (hcd->utmi_base != 0)
		{
			//([0]: power down override, [1]:Termination override, [6]:15Kohm pull low for dp, [7] :15Kohm pull low for dm)
			writeb(readb((void*)(hcd->utmi_base)) & (u8)(~(BIT1|BIT6|BIT7)), (void*) (hcd->utmi_base));
			//([2]:TED power down, [3]Preamp power down, [6]TX power down)
			writeb(readb((void*)(hcd->utmi_base+0x1*2-1)) & (u8)(~(BIT2|BIT3|BIT6)), (void*) (hcd->utmi_base+0x1*2-1));
		}
		
		#if !defined(ENABLE_BATTERY_CHARGE) 
		if (hcd->bc_base != 0)
		{
			//BC power on
			writeb(0, (void*) (hcd->bc_base));
		}
		#endif
	}
}

EXPORT_SYMBOL_GPL(usb_bc_enable);
EXPORT_SYMBOL_GPL(usb_power_saving_enable);


