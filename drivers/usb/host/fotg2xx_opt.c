/*******************************************************************************
 * Module name: fotg2xx_opt.c
 *
 * Copyright 2009 GM for OTG function
 * All Rights Reserved.
 *
 * The information contained herein is confidential property of Company.
 * The user, copying, transfer or disclosure of such information is
 * prohibited except by express written agreement with Company.
 *
 * Module Description:
 *  This OTG dirver for GM FOTG2XX controller
 *
 ******************************************************************************/
#undef DEBUG

#ifdef CONFIG_USB_DEBUG
#define DEBUG
#else
#undef DEBUG
#endif

#include <linux/module.h>
#include <linux/pci.h>
#include <linux/dmapool.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/reboot.h>
#include <linux/usb.h>
#include <linux/usb/otg.h>
#include <linux/moduleparam.h>
#include <linux/dma-mapping.h>
#include <linux/kallsyms.h>
#include <linux/platform_device.h>
#include <linux/usb/hcd.h>

#include "fotg2xx-config.h"
#include "fotg2xx-ehci-macro.h"
#include "fotg2xx_opt-macro.h"
#include "../gadget/fotg2xx-peri-macro.h"

#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/unaligned.h>
#if defined(CONFIG_PLATFORM_GM8126) || defined(CONFIG_PLATFORM_GM8287) || defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
#include <mach/ftpmu010.h>
#endif
#if defined(CONFIG_PLATFORM_GM8210)
#include <mach/ftpmu010_pcie.h>
#include <mach/fmem.h>
#endif

/* PMU register data */
#if defined(CONFIG_PLATFORM_GM8210)
static void __iomem *h2x0_va_base;
static void __iomem *h2x1_va_base;
static void __iomem *h2x2_va_base;
#endif
static int usb_pmu_fd = -1;
int usb_get_pmu_fd(void)
{
	return usb_pmu_fd;
}
EXPORT_SYMBOL(usb_get_pmu_fd);

/* Register the reg_off and bit_masks that will be used in the future */
#if !defined(CONFIG_PLATFORM_GM8210)

static pmuReg_t regUSBArray[] = {
	/* reg_off,bit_masks,lock_bits,init_val,init_mask */
#if defined(CONFIG_PLATFORM_GM8126)
	{0x38, BIT16, BIT16, 0, BIT16},
	{0xA0, ~0x0, ~0x0, 0x81001, ~0x0}, //OTG control reg.
#elif defined(CONFIG_PLATFORM_GM8287)
    {0xC0, 0x7F | BIT19, 0x7F | BIT19, 0x78 | BIT19, 0x7F | BIT19}, /* OTG0, 0x7f equals BIT0~6 */
    {0xC8, 0x7F | BIT19, 0x7F | BIT19, 0x78 | BIT19, 0x7F | BIT19}, /* OTG1, 0x7f equals BIT0~6 */
#elif defined(CONFIG_PLATFORM_GM8139)
    {0xAC, BIT0, BIT0, 0, BIT0},            /* OTG PHY clock */
    {0xB4, BIT9, BIT9, 0, BIT9},            /* OTG controller clock */
    {0xC0, ~0x0, ~0x0, 0xff80379, ~0x0},    /* OTG PHY: PCR<2:1>=11, TXSW<1:0>=11, PREEMPS_EN=1, PREEMPS<2:0>=111 */
#elif defined(CONFIG_PLATFORM_GM8136)
    {0xAC, BIT0, BIT0, 0, BIT0},            /* OTG PHY clock */
    {0xB4, BIT9 | BIT16, BIT9 | BIT16, BIT16, BIT9 | BIT16},        /* OTG controller clock, BIT9:OTG0, BIT16:OTG1 */
    {0xC0, ~0x0, ~0x0, 0xff8037b, ~0x0},    /* OTG PHY: PCR<2:1>=11, TXSW<1:0>=11, PREEMPS_EN=1, PREEMPS<2:0>=111 */
#endif
};

#else /* for GM8210 */

static pmuPcieReg_t regOTGArray[] = {
	/* reg_off,bit_masks,lock_bits,init_val,init_mask */
#define H2X_APB_BITS (BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15|BIT16)
	/* AHBC(BIT10) and OTG0,1,2(BIT6,5,4) clock, 0:enable */
	{0x30, BIT4|BIT5|BIT6|BIT10, 0, 0, BIT4|BIT5|BIT6|BIT10},
	{0x34, H2X_APB_BITS, 0, 0, H2X_APB_BITS},
	/*
	 * ============== 0x84 is OTG_PHY_CTRL ==============
	 * init_val = 0x260B3D9E means as follows
	 * OTG0: |=BIT1|BIT2|BIT3|BIT4|BIT8;&= ~BIT27&~BIT0;
	 * OTG1: |=BIT10|BIT11|BIT12|BIT13|BIT17;&= ~BIT28&~BIT9;
	 * OTG2: &= ~BIT18;|= BIT19;
	 */
	{0x84, 0xffffffff, 0xffffffff, 0x260B3D9E, 0xffffffff},
};
#endif /* else */

#if !defined(CONFIG_PLATFORM_GM8210)
static pmuRegInfo_t usb_clk_info = {
	"usb_clk",
	ARRAY_SIZE(regUSBArray),
	ATTR_TYPE_NONE,             /* no clock source */
	regUSBArray
};
#else
static pmuPcieRegInfo_t usb_clk_info = {
	"usb_clk",
	ARRAY_SIZE(regOTGArray),
	ATTR_TYPE_PCIE_NONE,             /* no clock source */
	regOTGArray
};
#endif



#define hcd_to_ehci(hcd) ((struct ehci_hcd *)(hcd->hcd_priv))

#define a_host_to_b_device(x) { \
	mwOTG20_stop_host(x); \
	mdwOTGC_Control_A_BUS_REQ_Clr(x); \
	mdwOTGC_Control_A_BUS_DROP_Set(x); \
	mdwPort(x,0x130) &= ~(BIT0|BIT1|BIT2); \
	mUsbGlobIntEnSet(x); \
	mUsbUnPLGClr(x); \
}

#define b_device_to_a_host(x) { \
	mUsbUnPLGSet(x); \
	mUsbGlobIntDis(x); \
	mdwPort(x,0x130) |= (BIT0|BIT1|BIT2); \
	mdwOTGC_Control_A_BUS_DROP_Clr(x) ; \
	mdwOTGC_Control_A_BUS_REQ_Set(x); \
	mwOTG20_start_host(x); \
}

#define QUEUE_LEN 64

u32 fotg2xx_wCurrentInterruptMask;
static int last_fotg2xx_id;
static int last_fotg2xx_role;
static u32 queue[QUEUE_LEN];
static u32 queue_in = 0;
static u32 queue_out = 0;

struct fotg2xx_priv {
	struct otg_transceiver otg_ctrl;
	struct work_struct otg_work;
};
static struct fotg2xx_priv fotg2xx_data[FOTG_DEV_NR];

u32 get_fotg2xx_va(int num)
{
	struct usb_hcd *hcd;

	if (num >= FOTG_DEV_NR)
		panic("No such FOTG2XX device");

	hcd = bus_to_hcd(fotg2xx_data[num].otg_ctrl.host);

	return (u32)hcd->regs;
}
EXPORT_SYMBOL(get_fotg2xx_va);

u32 get_fotg2xx_irq(int num)
{
	struct usb_hcd *hcd;

	if (num >= FOTG_DEV_NR)
		panic("No such FOTG2XX device");

	hcd = bus_to_hcd(fotg2xx_data[num].otg_ctrl.host);

	return hcd->irq;
}
EXPORT_SYMBOL(get_fotg2xx_irq);

void fotg200_handle_irq(int irq)
{
	u32 wINTStatus;
	u32 temp;
	struct usb_hcd *hcd;
	int i;

	for (i = 0;i < FOTG_DEV_NR;i++) {
		hcd = bus_to_hcd(fotg2xx_data[i].otg_ctrl.host);
        if (hcd) {
            if (hcd->irq == irq)
                break;
        } else {
            panic("%s: No such hcd exists!\n", __func__);
            return;
        }
	}

	wINTStatus = mdwOTGC_INT_STS_Rd(hcd->regs);
	mdwOTGC_INT_STS_Clr(hcd->regs,OTGC_INT_IDCHG);
	mdwOTGC_INT_STS_Clr(hcd->regs,wINTStatus);
	if (wINTStatus) {  // something happen
		queue[queue_in] = wINTStatus;
		if (queue_out != queue_in) {
			// more than one work in the queue, we only do the last one by setting work as "0"
			temp = queue_out;
			while (temp != queue_in) {
				queue[temp] = 0;
				temp = (temp + 1) % QUEUE_LEN;
			}
			queue_out = queue_in;
		}
		queue_in = (queue_in + 1) % QUEUE_LEN;
		schedule_work(&(fotg2xx_data[i].otg_work));
	}
}
EXPORT_SYMBOL_GPL(fotg200_handle_irq);

void OTG_handler(struct work_struct *data)
{
	u32 wINTStatus;
	u32 current_id;
	unsigned long  flags;
	u32 temp;
	struct fotg2xx_priv *priv = container_of(data, struct fotg2xx_priv, otg_work);
	struct usb_hcd *hcd = bus_to_hcd(priv->otg_ctrl.host);

	if (queue[queue_out] == 0) {  // happen for multiple work
		printk(" empty OTG work queue %x\n", queue_out);
		//queue_out = (queue_out + 1) % QUEUE_LEN;
		return;
	}

	// in case host mode with connected device
	// we need to wait host HCD has finished job
	msleep(20);

	// for HCD recover unfinished URB, we need more time
	temp = mdwPort(hcd->regs,0x10);
	while (temp & 0x20) {   // host async still enable
		msleep(100);
		temp = mdwPort(hcd->regs,0x10);
	}

	local_irq_save(flags);

	//printk("+++++++++ %x:%x\n",queue_out,queue[queue_out]);
	wINTStatus = queue[queue_out];
	queue_out = (queue_out + 1 ) % QUEUE_LEN;
	current_id = mdwOTGC_Control_ID_Rd(hcd->regs);

	/* Change ID */
	if ( (wINTStatus & OTGC_INT_IDCHG) && (last_fotg2xx_id != current_id) ){
		if ( current_id == OTG_ID_A_TYPE){
			printk("%s: FOTG2XX is now in Mini-A type %x\n", __func__, 0);
			last_fotg2xx_id = OTG_ID_A_TYPE;
			if (mdwOTGC_Control_CROLE_Rd(hcd->regs) == OTG_ROLE_HOST){
				b_device_to_a_host(hcd->regs);
				hcd->driver->reset(hcd);  // add for 2.6.21
				hcd->driver->start(hcd);
			}
		} else {
			printk("%s: FOTG2XX is now in Mini-B type %x\n", __func__, 0);
			last_fotg2xx_id = OTG_ID_B_TYPE;
			if (mdwOTGC_Control_CROLE_Rd(hcd->regs) == OTG_ROLE_DEVICE){
				hcd->driver->stop(hcd);
                printk("%s: do ID change\n", __func__);
				a_host_to_b_device(hcd->regs);
			}
		}
		mdwOTGC_INT_STS_Clr(hcd->regs,OTGC_INT_IDCHG);
	}

	/* Enable Device's HNP */
	if (priv->otg_ctrl.host->b_hnp_enable == 1 &&  !mdwOTGC_Control_A_SET_B_HNP_EN_Rd(hcd->regs)){
		//fotg2xx_dbg("fotg200_irq: enable HNP %x\n",0);
		printk("%s : enable HNP\n", __func__);
		mdwOTGC_Control_A_SET_B_HNP_EN_Set(hcd->regs);
	}

	/* Role Change */
	if (wINTStatus & OTGC_INT_RLCHG) {
		if ( (mdwOTGC_Control_ID_Rd(hcd->regs) == OTG_ID_A_TYPE)) {
			printk(" ID-A OTG Role change... %x\n",wINTStatus);
			if (mdwOTGC_Control_CROLE_Rd(hcd->regs) == OTG_ROLE_HOST){
				mUsbUnPLGSet(hcd->regs);
				mUsbGlobIntDis(hcd->regs);
				mdwPort(hcd->regs,0x130) |= (BIT0|BIT1|BIT2);
				mwOTG20_start_host(hcd->regs);
				priv->otg_ctrl.state = OTG_STATE_A_HOST;
				printk("enter host mode....\n");
			}
			else {
				mwOTG20_stop_host(hcd->regs);
				mdwOTGC_Control_A_SET_B_HNP_EN_Clr(hcd->regs);
				mdwPort(hcd->regs,0x130) &= ~(BIT0|BIT1|BIT2);
				mUsbGlobIntEnSet(hcd->regs);
				mUsbUnPLGClr(hcd->regs);
				priv->otg_ctrl.state = OTG_STATE_A_PERIPHERAL;
				printk("enter device mode....\n");
			}
		}
		else {   // OTG_ID_D_B_TYPE
			printk(" ID-B OTG Role change... %x\n",wINTStatus);
			if (mdwOTGC_Control_CROLE_Rd(hcd->regs) == OTG_ROLE_HOST){
				hcd->state = HC_STATE_RUNNING;

				priv->otg_ctrl.host->is_b_host = 1;
				mUsbGlobIntDis(hcd->regs);
				mdwPort(hcd->regs,0x130) |= (BIT0|BIT1|BIT2);
				mwOTG20_start_host(hcd->regs);
				mdwOTGC_Control_B_HNP_EN_Clr(hcd->regs);
				priv->otg_ctrl.state = OTG_STATE_B_HOST;
				printk("enter host mode....\n");
			}
			else {
				//hcd->driver->stop(hcd);
				//for(cnt=0;cnt<5000;cnt++) udelay(10);
				priv->otg_ctrl.host->is_b_host = 0;
				//mwOTG20_stop_host(hcd->regs);
				mdwPort(hcd->regs,0x130) &= ~(BIT0|BIT1|BIT2);
				mUsbGlobIntEnSet(hcd->regs);
				mUsbUnPLGClr(hcd->regs);
				priv->otg_ctrl.state = OTG_STATE_B_PERIPHERAL;

				//fotg2xx_show_hc_regs();
				//fotg2xx_show_otg_regs();
				printk("enter device mode....\n");
#ifndef CONFIG_USB_GADGET
				// John add for in case gadget driver not enabled while using OTG
				// controller will generate interrupt for suspend, we just to ignore it !!
				printk("WARNING: Your OTG controller don't know how to act as USB device\n");
				printk("         Please enable gadget function\n");
				mdwPort(hcd->regs,0x134) = 0xFFFFFFFF;
				mdwPort(hcd->regs,0x138) = 0xFFFFFFFF;
				mdwPort(hcd->regs,0x13C) = 0xFFFFFFFF;
				mUsbUnPLGSet(hcd->regs);
#endif
			}
		}
	}

	//<1>.Checking the OTG layer interrupt status
	if (wINTStatus>0) {
		if (wINTStatus & OTGC_INT_AVBUSERR)
			printk("Error --- Interrupt OTGC_INT_AVBUSERR=1... \n");
		if (wINTStatus & OTGC_INT_OVC)
			printk("Error --- Attached Device Not Supported: Over Current\n");

		mdwOTGC_INT_STS_Clr(hcd->regs,wINTStatus);
		if ((fotg2xx_wCurrentInterruptMask & wINTStatus)>0) {
			printk("%s: re-drive Vbus...\n", __func__);
			mdwOTGC_Control_A_BUS_REQ_Clr(hcd->regs);
			mdwOTGC_Control_A_BUS_DROP_Set(hcd->regs);
			mdwOTGC_Control_A_BUS_DROP_Clr(hcd->regs);
			mdwOTGC_Control_A_BUS_REQ_Set(hcd->regs);
		}

		mdwOTGC_INT_STS_Clr(hcd->regs,wINTStatus);
		//wFOTGPeri_Port(0xC0)|=BIT1;
	}
	local_irq_restore(flags);
	return;
}

int fotg200_otgd_init(struct platform_device *pdev, struct usb_bus *host, struct usb_gadget *gadget)
{
	fotg2xx_data[pdev->id].otg_ctrl.dev = &pdev->dev;
	fotg2xx_data[pdev->id].otg_ctrl.host = host;
	fotg2xx_data[pdev->id].otg_ctrl.gadget = gadget;

	printk("FOTG2XX Controller Initialization\n");

	return 0;
}
EXPORT_SYMBOL(fotg200_otgd_init);

//=============================================================================
// OTGC_Init()
// Description:1.Init the OTG Structure Variable
//             2.Init the Interrupt register(OTG-Controller layer)
//             3.Call the OTG_RoleChange function to init the Host/Peripheral
// input: none
// output: none
//=============================================================================
static void OTGC_Init(struct usb_bus *host)
{
	u32 dwTemp;
	struct usb_hcd *hcd = bus_to_hcd(host);
#if defined(CONFIG_PLATFORM_GM8210)
    fmem_pci_id_t pci_id;
    fmem_cpu_id_t cpu_id;
#endif

	//Clear the interrupt status
	dwTemp=mdwOTGC_INT_STS_Rd(hcd->regs);
	mdwOTGC_INT_STS_Clr(hcd->regs,dwTemp);

#if defined(CONFIG_PLATFORM_GM8126)
	ftpmu010_write_reg(usb_pmu_fd, 0xA0, 0, BIT3); //Set to Host Mode
#elif defined(CONFIG_PLATFORM_GM8210)
    fmem_get_identifier(&pci_id, &cpu_id);
    if (cpu_id == FMEM_CPU_FA726 && pci_id == FMEM_PCI_HOST) {
        switch (hcd->irq) {
            case USB_FOTG2XX_0_IRQ:
                /* Configure OTG0 in host mode */
                ftpmu010_pcie_write_reg(usb_pmu_fd, 0x84, 0, BIT0);
                break;
            case USB_FOTG2XX_1_IRQ:
                /* Configure OTG1 in host mode */
                ftpmu010_pcie_write_reg(usb_pmu_fd, 0x84, 0, BIT9);
                break;
            case USB_FOTG2XX_2_IRQ:
                /* Configure OTG2 in host mode */
                ftpmu010_pcie_write_reg(usb_pmu_fd, 0x84, 0, BIT18);
                break;
            default:
                panic("Error OTG IRQ: %d\n", hcd->irq);
                break;
        }
    }
#elif defined(CONFIG_PLATFORM_GM8287)
	switch (hcd->irq) {
		case USB_FOTG2XX_0_IRQ:
			/* Configure OTG0 in host mode */
			ftpmu010_write_reg(usb_pmu_fd, 0xC0, 0, BIT2);
			break;
		case USB_FOTG2XX_1_IRQ:
			/* Configure OTG1 in host mode */
			ftpmu010_write_reg(usb_pmu_fd, 0xC8, 0, BIT2);
			break;
		default:
			panic("Error OTG IRQ: %d\n", hcd->irq);
			break;
	}
#elif defined(CONFIG_PLATFORM_GM8139)
	ftpmu010_write_reg(usb_pmu_fd, 0xC0, 0, BIT2); //Set to Host Mode
#elif defined(CONFIG_PLATFORM_GM8136)
	ftpmu010_write_reg(usb_pmu_fd, 0xC0, 0, BIT2); //Set to Host Mode
#endif

	//<1>.Read the ID
	if (mdwOTGC_Control_ID_Rd(hcd->regs) != OTG_ID_A_TYPE){
		//Change to B Type
		//<1.1>.Init Variable
		/*OTG.A_bASRPDET=0;
		  OTG.B_bBSRPDN=0;
		  OTG.CurrentRole=USB_DEVICE_ROLE;*/
		fotg2xx_wCurrentInterruptMask=OTGC_INT_B_TYPE;
		//<1.2>.Init Interrupt
		mdwOTGC_INT_Enable_Clr(hcd->regs,OTGC_INT_A_TYPE);
		mdwOTGC_INT_Enable_Set(hcd->regs,OTGC_INT_B_TYPE);
		//<1.3>.Init the Peripheral
		// OTG_RoleChange(USB_DEVICE_ROLE,xceiver);
		last_fotg2xx_id = OTG_ID_B_TYPE;
		last_fotg2xx_role =  (OTG_ROLE_HOST | OTG_ROLE_DEVICE) +1;
	}
	else {
		//Changfe to A Type
		//<2.1>.Init Variable
		/*OTG.A_bASRPDET=0;
		  OTG.B_bBSRPDN=0;
		  OTG.CurrentRole=USB_HOST_ROLE;*/
		fotg2xx_wCurrentInterruptMask=OTGC_INT_A_TYPE;
		//<2.2>. Init Interrupt
		mdwOTGC_INT_Enable_Clr(hcd->regs,OTGC_INT_B_TYPE);
		mdwOTGC_INT_Enable_Set(hcd->regs,OTGC_INT_A_TYPE);
		//<2.3>.Init the Host
		// OTG_RoleChange(USB_HOST_ROLE,xceiver);
		last_fotg2xx_id = OTG_ID_A_TYPE;
		last_fotg2xx_role =  (OTG_ROLE_HOST | OTG_ROLE_DEVICE) +1;
	}
	//enable role change interrupt
	*((volatile u32 *)(hcd->regs+0x88)) = 0x721;

	printk(KERN_DEBUG"fotg200 int enable = %x\n", mdwFOTGPort(hcd->regs,0x08));
}

int fotg200_init(struct device *dev, struct usb_bus *host, struct usb_gadget *gadget)
{
	struct usb_hcd *hcd = bus_to_hcd(host);

	queue_in =0;
	queue_out=0;

	//Disable Device's Group interrupt
	mdwPort(hcd->regs,0x130) |= (BIT0|BIT1|BIT2);

	//Set unplug to force device to detach to PC
	mdwOTGC_UsbUnPLGSet(hcd->regs);

#if defined(CONFIG_PLATFORM_GM8126)
	//Set OTG200 interrupt to high active, ex. A320
	mwOTG20_Interrupt_OutPut_High_Set(hcd->regs);
#endif

	//Init OTG module's global variables and registers
	OTGC_Init(host);
	//Turn off all OTG interrutp first. It will be turned on when do role change
	mdwOTGC_INT_Enable_Clr(hcd->regs,0xFFFFFFFF);
	//OTG.wCurrentInterruptMask = 0x00000000;
	fotg2xx_wCurrentInterruptMask = OTGC_INT_AVBUSERR|OTGC_INT_OVC;
	mdwOTGC_INT_Enable_Set(hcd->regs,OTGC_INT_IDCHG | OTGC_INT_AVBUSERR | OTGC_INT_OVC);
	return 0;
}
EXPORT_SYMBOL(fotg200_init);

void ehci_reset_otg_PHY(struct usb_bus *host)
{
#ifndef REMOVE_COVERHW
	struct usb_hcd *hcd = bus_to_hcd(host);
	//Reset OTG PHY
	mdwOTGC_Control_PHY_Reset_Set(hcd->regs);
	udelay(1000);//1000 About 0.2ms
	mdwOTGC_Control_PHY_Reset_Clr(hcd->regs);
	printk("Reset Phy %x\n",0);
#endif
}

void ehci_reset_OTG(struct usb_bus *host)
{
#ifndef REMOVE_COVERHW
	struct usb_hcd *hcd = bus_to_hcd(host);
	//Reset OTG controller
	mdwOTGC_Control_OTG_Reset_Set(hcd->regs);
	mdwOTGC_Control_OTG_Reset_Clr(hcd->regs);
#endif
}

/* Setup FOTG2XX Registers  */
void init_FOTG2XX_Dev(struct usb_hcd *hcd)
{
#if defined(CONFIG_PLATFORM_GM8126)
	ftpmu010_write_reg(usb_pmu_fd, 0xA0, 0, (BIT0|BIT1|BIT2)<<17);
	ftpmu010_write_reg(usb_pmu_fd, 0xA0, BIT1<<17, BIT1<<17);       // config reference voltage to 120mV
	ftpmu010_write_reg(usb_pmu_fd, 0xA0, BIT0, BIT0);               // enable VBUS input
	ftpmu010_write_reg(usb_pmu_fd, 0xA0, 0, BIT3);                  // set to host mode
	ftpmu010_write_reg(usb_pmu_fd, 0x38, 0, BIT16);                 // turn on OTG clock
	ftpmu010_write_reg(usb_pmu_fd, 0xA0, BIT8, BIT8);               // select clock source, 0:12 MHz, 1:30 MHz
	ftpmu010_write_reg(usb_pmu_fd, 0xA0, BIT9|BIT11, BIT9|BIT11);   // bit9 & bit11 must all be one
#elif defined(CONFIG_PLATFORM_GM8210)
	/* Turn on AHBC clock */
	//ftpmu010_pcie_write_reg(usb_pmu_fd, 0x30, 0, BIT10);

	/* Turn on H2X_APB clock */
	//ftpmu010_pcie_write_reg(usb_pmu_fd, 0x34, 0, H2X_APB_BITS);

    /* H2X setting:
	 * bit[6:4] = 0 (read command's pre-fetch number)
	 * bit[3:2] = 1 (continuous incremental write burst)
     */
    fmem_pci_id_t pci_id;
    fmem_cpu_id_t cpu_id;
    fmem_get_identifier(&pci_id, &cpu_id);
    if (cpu_id == FMEM_CPU_FA726 && pci_id == FMEM_PCI_HOST) {
        switch (hcd->rsrc_start) {
            case USB_FOTG2XX_0_PA_BASE:
                /* Set OTG0 H2X register */
                writel(readl(h2x0_va_base) & ~BIT3, h2x0_va_base);
                writel(readl(h2x0_va_base) | BIT2, h2x0_va_base);
                //======= OTG0 Part======
                /* BIT2:pllaliv; BIT3:oscouten; BIT8:0=external clock, 1=internal clock */
                ftpmu010_pcie_write_reg(usb_pmu_fd, 0x84, BIT2|BIT3|BIT8, BIT2|BIT3|BIT8);
                /* OTG0 PHY coreclkin. 0: on, 1:off */
                ftpmu010_pcie_write_reg(usb_pmu_fd, 0x84, 0, BIT27);
                /* Disable OTG0 phy POR */
                ftpmu010_pcie_write_reg(usb_pmu_fd, 0x84, BIT4, BIT4);
                /* Configure OTG0 in host mode */
                ftpmu010_pcie_write_reg(usb_pmu_fd, 0x84, 0, BIT0);
                /* Enable OTG0 VBUS input */
                ftpmu010_pcie_write_reg(usb_pmu_fd, 0x84, BIT1, BIT1);
                /* Turn on OTG0 clock */
                ftpmu010_pcie_write_reg(usb_pmu_fd, 0x30, 0, BIT6);
                break;
            case USB_FOTG2XX_1_PA_BASE:
                /* Set OTG1 H2X register */
                writel(readl(h2x1_va_base) & ~BIT3, h2x1_va_base);
                writel(readl(h2x1_va_base) | BIT2, h2x1_va_base);
                //======= OTG1 Part======
                /* BIT11:pllaliv; BIT12:oscouten; BIT17:0=external clock, 1=internal clock */
                ftpmu010_pcie_write_reg(usb_pmu_fd, 0x84, BIT11|BIT12|BIT17, BIT11|BIT12|BIT17);
                /* OTG1 PHY coreclkin. 0: on, 1:off */
                ftpmu010_pcie_write_reg(usb_pmu_fd, 0x84, 0, BIT28);
                /* Disable OTG1 phy POR */
                ftpmu010_pcie_write_reg(usb_pmu_fd, 0x84, BIT13, BIT13);
                /* Configure OTG1 in host mode */
                ftpmu010_pcie_write_reg(usb_pmu_fd, 0x84, 0, BIT9);
                /* Enable OTG1 VBUS input */
                ftpmu010_pcie_write_reg(usb_pmu_fd, 0x84, BIT10, BIT10);
                /* Turn on OTG1 clock */
                ftpmu010_pcie_write_reg(usb_pmu_fd, 0x30, 0, BIT5);
                break;
            case USB_FOTG2XX_2_PA_BASE:
                /* Set OTG2 H2X register */
                writel(readl(h2x2_va_base) & ~BIT3, h2x2_va_base);
                writel(readl(h2x2_va_base) | BIT2, h2x2_va_base);
                //======= OTG2(PHY 1.1) Part======
                /* Configure OTG2 in host mode */
                ftpmu010_pcie_write_reg(usb_pmu_fd, 0x84, 0, BIT18);
                /* Enable OTG2 VBUS input */
                ftpmu010_pcie_write_reg(usb_pmu_fd, 0x84, BIT19, BIT19);
                /* Turn on OTG2 clock */
                ftpmu010_pcie_write_reg(usb_pmu_fd, 0x30, 0, BIT4);
                /* force to full-speed */
                mwOTG20_Control_ForceFullSpeed_Set(hcd->regs);
                break;
            default:
                panic("Error OTG Base: 0x%x\n", (u32)hcd->rsrc_start);
                break;
        }
    }
#elif defined(CONFIG_PLATFORM_GM8287)
    switch (hcd->rsrc_start) {
        case USB_FOTG2XX_0_PA_BASE:
            ftpmu010_write_reg(usb_pmu_fd, 0xC0, BIT4 | BIT5 | BIT19, BIT4 | BIT5 | BIT19);
            ftpmu010_write_reg(usb_pmu_fd, 0xC0, 0, BIT0); /* OTG0 PHY clock */
            ftpmu010_write_reg(usb_pmu_fd, 0xC0, BIT6, BIT6);
            ftpmu010_write_reg(usb_pmu_fd, 0xC0, BIT3, BIT3);
            ftpmu010_write_reg(usb_pmu_fd, 0xC0, 0, BIT2);
            ftpmu010_write_reg(usb_pmu_fd, 0xC0, 0, BIT1); /* OTG0 hclk on */
            break;
        case USB_FOTG2XX_1_PA_BASE:
            ftpmu010_write_reg(usb_pmu_fd, 0xC8, BIT4 | BIT5 | BIT19, BIT4 | BIT5 | BIT19);
            ftpmu010_write_reg(usb_pmu_fd, 0xC8, 0, BIT0); /* OTG1 PHY clock */
            ftpmu010_write_reg(usb_pmu_fd, 0xC8, BIT6, BIT6);
            ftpmu010_write_reg(usb_pmu_fd, 0xC8, BIT3, BIT3);
            ftpmu010_write_reg(usb_pmu_fd, 0xC8, 0, BIT2);
            ftpmu010_write_reg(usb_pmu_fd, 0xC8, 0, BIT1); /* OTG1 hclk on */
            break;
        default:
            panic("Error OTG Base: 0x%x\n", (u32)hcd->rsrc_start);
            break;
    }
#elif defined(CONFIG_PLATFORM_GM8136)
    switch (hcd->rsrc_start) {
        case USB_FOTG2XX_0_PA_BASE:
            {
                /*
                 * set the parameter of USB PHY, depends on package
                 */
                uint pkg_id, val;

                pkg_id = (ftpmu010_read_reg(0x00) >> 2) & 0x1F;
                switch (pkg_id) {
                    case 0x04:
                    case 0x10:
                    case 0x14:
                        val = BIT24 | BIT25; // LQFP
                        break;
                    default:
                        val = BIT24 | BIT25 | BIT26 | BIT27; // BGA
                        break;
                }
                ftpmu010_write_reg(usb_pmu_fd, 0xC0, val, BIT24 | BIT25 | BIT26 | BIT27);
            }
            break;
    }
#endif

	mdelay(10); // waiting for PHY clock be stable, while clock source changed from externel to internel, at lease 5ms
	mwOTG20Bit_Set(hcd->regs,0x40,BIT6); // put PHY into suspend mode
	mdelay(10);
	mwOTG20Bit_Clr(hcd->regs,0x40,BIT6);
	mdelay(10);

#ifdef CONFIG_OTG
	//Host controller with OTG fucntion ==> Turn on Vbus
	//Disable OTG interrupt
    mdwFOTGPort(hcd->regs,0x08) &= 0xffffff00;

	//Drive Vbus for FOTG100 (Default device A role)]
	if ( mdwOTGC_Control_ID_Rd(hcd->regs) == OTG_ID_A_TYPE ) {
		u32 temp;
		udelay(1000);
		printk("Enter Device A\n");
		temp = mdwOTGC_INT_STS_Rd(hcd->regs);
		mdwOTGC_INT_STS_Clr(hcd->regs,temp);
		mdwOTGC_Control_A_BUS_DROP_Set(hcd->regs);
		mdwOTGC_Control_A_BUS_REQ_Clr(hcd->regs);
		udelay(1000);
		printk("Drive Vbus because of ID pin shows Device A\n");
		mdwOTGC_Control_A_BUS_DROP_Clr(hcd->regs);
		mdwOTGC_Control_A_BUS_REQ_Set(hcd->regs);
		udelay(1000);
	}
#endif

#ifdef CONFIG_711MA_PHY
	//Set cover bit to cover 711MA PHY full speed reset issue
	mwOTG20_Control_COVER_FS_PHY_Reset_Set(hcd->regs);
#endif

#if defined(CONFIG_PLATFORM_GM8126)
	//Set OTG200 interrupt to high active, ex. A320
	mwOTG20_Interrupt_OutPut_High_Set(hcd->regs);
#endif

	//Basic initialization for FPFA version IP
	mwPeri20_Control_ChipEnable_Set(hcd->regs);

	//Important: If AHB clock is >=15Mhz and <= 30MHz, please remark this line (Enable half speed)).
	//IF > 30Hz, Disable half speed
#ifdef EnableHalfSpeed
	mwPeri20_Control_HALFSPEEDEnable_Set(hcd->regs);
#else /* EnableHalfSpeed */
	mwPeri20_Control_HALFSPEEDEnable_Clr(hcd->regs);
#endif /* EnableHalfSpeed */

#if 0
	mwOTG20_Control_ForceHighSpeed_Clr(hcd->regs);
	mwOTG20_Control_ForceFullSpeed_Clr(hcd->regs);
#endif

#if 0 // debug for force otg phy speed
	mwOTG20_Control_ForceFullSpeed_Set(hcd->regs); // force to full-speed (do not ack to device's KJ pattern)
	//mwOTG20_Control_ForceHighSpeed_Set(hcd->regs); // force to high-speed
	if (mwOTG20Bit_Rd(hcd->regs,0x80,BIT12))
		printk("!!!! Force Phy to Full-Speed !!!!\n");
	else if (mwOTG20Bit_Rd(hcd->regs,0x80,BIT14))
		printk("!!!! Force Phy to High-Speed !!!!\n");
#endif
}
EXPORT_SYMBOL(init_FOTG2XX_Dev);

void exit_FOTG2XX_Dev(struct usb_hcd *hcd)
{
#if defined(CONFIG_PLATFORM_GM8126)
	ftpmu010_write_reg(usb_pmu_fd, 0x38, BIT16, BIT16);
#elif defined(CONFIG_PLATFORM_GM8210)
    fmem_pci_id_t pci_id;
    fmem_cpu_id_t cpu_id;
    fmem_get_identifier(&pci_id, &cpu_id);
    if (cpu_id == FMEM_CPU_FA726 && pci_id == FMEM_PCI_HOST) {
        ftpmu010_pcie_write_reg(usb_pmu_fd, 0x30, 1, BIT6); /* Turn off OTG0 clock */
        ftpmu010_pcie_write_reg(usb_pmu_fd, 0x30, 1, BIT5); /* Turn off OTG1 clock */
        ftpmu010_pcie_write_reg(usb_pmu_fd, 0x30, 1, BIT4); /* Turn off OTG2 clock */
    }
#elif defined(CONFIG_PLATFORM_GM8287)
	ftpmu010_write_reg(usb_pmu_fd, 0xC0, BIT0, BIT0); /* OTG0 PHY clock */
	ftpmu010_write_reg(usb_pmu_fd, 0xC0, BIT1, BIT1); /* OTG0 controller clock */
	ftpmu010_write_reg(usb_pmu_fd, 0xC8, BIT0, BIT0); /* OTG1 PHY clock */
	ftpmu010_write_reg(usb_pmu_fd, 0xC8, BIT1, BIT1); /* OTG1 controller clock */
#elif defined(CONFIG_PLATFORM_GM8139)
	ftpmu010_write_reg(usb_pmu_fd, 0xAC, BIT0, BIT0); /* OTG PHY clock */
	ftpmu010_write_reg(usb_pmu_fd, 0xB4, BIT9, BIT9); /* OTG controller clock */
#elif defined(CONFIG_PLATFORM_GM8136)
    ftpmu010_write_reg(usb_pmu_fd, 0xAC, BIT0, BIT0); /* OTG PHY clock */
    ftpmu010_write_reg(usb_pmu_fd, 0xB4, BIT9, BIT9); /* OTG controller clock */
#endif
}
EXPORT_SYMBOL(exit_FOTG2XX_Dev);

static int __init ftusb_init(void)
{
	int i, ret = 0;

#if defined(CONFIG_PLATFORM_GM8210)
    fmem_pci_id_t pci_id;
    fmem_cpu_id_t cpu_id;
    fmem_get_identifier(&pci_id, &cpu_id);
    if (cpu_id == FMEM_CPU_FA726 && pci_id == FMEM_PCI_HOST) {
        /* Map H2X */
        h2x0_va_base = ioremap(H2X0_FOTG2XX_PA_BASE, H2X0_FOTG2XX_PA_SIZE);
        h2x1_va_base = ioremap(H2X1_FOTG2XX_PA_BASE, H2X1_FOTG2XX_PA_SIZE);
        h2x2_va_base = ioremap(H2X2_FOTG2XX_PA_BASE, H2X2_FOTG2XX_PA_SIZE);
    }
#endif

	for (i = 0;i < FOTG_DEV_NR;i++) {
		memset(&fotg2xx_data[i], 0, sizeof(fotg2xx_data[i]));
		INIT_WORK(&fotg2xx_data[i].otg_work, OTG_handler);
	}
#if defined(CONFIG_PLATFORM_GM8126) || defined(CONFIG_PLATFORM_GM8287) || defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
	if ((usb_pmu_fd = ftpmu010_register_reg(&usb_clk_info)) < 0) {
		ret = -1;
		printk(KERN_ERR "%s: Register USB to PMU failed\n", __func__);
	}
#endif
#if defined(CONFIG_PLATFORM_GM8210)
    if (cpu_id == FMEM_CPU_FA726 && pci_id == FMEM_PCI_HOST) {
        if ((usb_pmu_fd = ftpmu010_pcie_register_reg(&usb_clk_info)) < 0) {
            ret = -1;
            printk(KERN_ERR "%s: Register USB to PMU failed\n", __func__);
        }
    }
#endif
	return ret;
}
static void __exit ftusb_exit(void)
{
#if defined(CONFIG_PLATFORM_GM8126) || defined(CONFIG_PLATFORM_GM8287) || defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
	if (ftpmu010_deregister_reg(usb_pmu_fd) < 0)
		printk(KERN_ERR "%s: Unregister USB from PMU Failed\n", __func__);
#endif
#if defined(CONFIG_PLATFORM_GM8210)
    fmem_pci_id_t pci_id;
    fmem_cpu_id_t cpu_id;
    fmem_get_identifier(&pci_id, &cpu_id);
    if (cpu_id == FMEM_CPU_FA726 && pci_id == FMEM_PCI_HOST) {
        iounmap(h2x0_va_base);
        iounmap(h2x1_va_base);
        iounmap(h2x2_va_base);
        if (ftpmu010_pcie_deregister_reg(usb_pmu_fd) < 0)
            printk(KERN_ERR "%s: Unregister USB from PMU Failed\n", __func__);
    }
#endif
}
module_init(ftusb_init);
module_exit(ftusb_exit);

MODULE_AUTHOR("<GM-tech.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("OTG change mode driver");
