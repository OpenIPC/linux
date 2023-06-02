/*
    USB device driver

    Primary interface for USB device driver

    @file       usb.h
    @ingroup    mIDrvUSB_Device
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2014.  All rights reserved.
*/

#ifndef __KERNEL__
#include <stdio.h>
#include <string.h>
#include "interrupt.h"
#include "pll.h"
#include "pll_protected.h"
#include "efuse_protected.h"
#else
#include <plat/efuse_protected.h>
#include <linux/gpio.h>
#include <plat/nvt-gpio.h>
#include "linux/delay.h"
#endif

#include "usb2dev.h"
#include "usb2dev_reg.h"
#include "usb2dev_int.h"
//#include "dma_protected.h"
//#include "gpio.h"
//#include "top.h"
//#include "pad.h"
//#include "Utility.h"
//#include "uart.h"
//#include "SxCmd.h"

unsigned int rtos_usb2dev_debug_level = NVT_DBG_WRN;

static  VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags) vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags) vk_spin_unlock_irqrestore(&my_lock, flags)

#if (_FPGA_EMULATION_ == 1)
#include "i2c.h"
// PinmuxCfg.c
static I2C_SESSION  gUsbI2cSes;
static PI2C_OBJ     pUsbI2CObj;
#endif

// Global funcs & variables
USB_MNG                     gUSBManage;
UINT32                      gUSBCauseGroup[4];
UINT32                      gUSBCause;
volatile UINT32             USBPhyDisable;

BOOL                        gUSBOpen                                = FALSE;
USB_GENERIC_CB              gUsbSetInterfaceHdl                     = NULL;
USB_CTRL_MODE               gUsbCtrlMode                            = USB_CTRL_MODE_DEVICE;
USB_GENERIC_CB              gUsbChargingCB                          = NULL;
USB_CHARGER_STS             gChargerSts                             = USB_CHARGER_STS_NONE;
BOOL                        gUsbUseShareFIFO                        = FALSE;
#if _EMULATION_
USB_FIFO_NUM                gTestFifoOffset                         = USB_FIFO0;
#endif
	   BOOL					usb_suspend_to_dramoff_en				= FALSE;


static USB_CONTROLLER_STATE USBControllerState                      = USB_CONTROLLER_STATE_NONE;
static BOOL                 bUsbHighSpeed                           = TRUE;
static BOOL                 bStandardCharger                        = TRUE;

ID FLG_ID_USBTSK;
//ID FLG_ID_USB;
ID SEMID_USB;


THREAD_HANDLE USBCtlStartTsk_ID;
THREAD_HANDLE USBSetupHalTsk_ID;


//
// Internal function declaration
//
static ER                   usb_openInternal(BOOL bChkCharger, UINT32 uiDebounce);
static BOOL                 usb_chkDataContact(void);
static BOOL                 usb_chkPCbyLineSts(void);
#if _USB2DEV_TODO_
static ER 					usb_powerdown_callback(CLK_PDN_MODE Mode, BOOL bEnter);
#endif
#if (_EMULATION_ == ENABLE)
extern ER                   usb_openDebounceTest(UINT32 uiDebounce);
extern UINT32               usb_readIntGrp2(void);
extern UINT32               usb_getLineStatus(void);
extern void                 usb_testDataContact(void);
extern void                 usb_testChargingPort(void);
extern void                 usb_testChargingPortSecondary(void);
#endif

#ifdef __KERNEL__
#define GPIO_INT_USBPLUGIN 43
static UINT32 gpio_getIntStatus(UINT32 ipin)
{
	gpio_direction_input(D_GPIO(7));
	return (UINT32) gpio_get_value(D_GPIO(7));
}
#endif

/**
    @addtogroup mIDrvUSB_Device
*/
//@{

/*
    USB device ISR

    USB controller device interrupt handler.

    @return void
*/
irqreturn_t usb_isr(int irq, void *devid)
{
	REGVALUE                    devIntGrp, devIntGrpMsk;

	USB_PutString("usb_isr\n");

	devIntGrp    = USB_GETREG(USB_DEVINTGROUP_REG_OFS);
	devIntGrpMsk = ~USB_GETREG(USB_DEVINTGROUPMASK_REG_OFS);
	devIntGrp = devIntGrp & devIntGrpMsk;

	if (devIntGrp & BIT3) {
		USB_PutString(" Interrupt Source Group 3 !!  \r\n");
		usb_devINTGrp3();
	}

	if (devIntGrp & (BIT0|BIT1|BIT2)) {
		usb2dev_set_config(USB_CONFIG_ID_INTEN, DISABLE);
		gUSBCause = devIntGrp & (BIT0|BIT1|BIT2);
		iset_flg(FLG_ID_USBTSK, FLGUSB_SIG);
	}

	return IRQ_HANDLED;
}

/*
    Lock USB

    This function will lock USB module

    @return
        - @b E_OK: Done with no errors.
        - @b E_ID: Outside semaphore ID number range
        - @b E_NOEXS: Semaphore does not yet exist
*/
static ER usb_lock(void)
{
	ER erReturn;

	erReturn = wai_sem(SEMID_USB);
	if (erReturn != E_OK) {
		return erReturn;
	}

	gUSBOpen = TRUE;
	return E_OK;
}

/*
    Unlock USB

    This function will unlock USB module

    @return
        - @b E_OK: Done with no errors.
        - @b E_ID: Outside semaphore ID number range
        - @b E_NOEXS: Semaphore does not yet exist
        - @b E_QOVR: Semaphore's counter error, maximum counter < counter
*/
static ER usb_unlock(void)
{
	gUSBOpen = FALSE;

	sig_sem(SEMID_USB);
	return E_OK;
}

/*
    Attach USB driver.

    This function will enalbe USB clock.

    @return void
*/
static void usb_attach(void)
{}

/*
    Detach driver.

    This function will disable USB clock

    @return void
*/
static void usb_detach(void)
{}

/*
    FOTG200 USB device controller initialization.

    This function is internal used for usb2dev_open() to initialize
    some setting of USB device controller

    @return void
*/
void usb_initFOTG200(void)
{
	T_USB_DEVADDR_REG           devAddr;
	T_USB_PHYTSTSELECT_REG      devPhyTest;
	T_USB_DEVIDLECNT_REG        devIdleCnt;
	T_USB_DEVMAINCTRL_REG       devMainCtl;
	T_USB_DEVTEST_REG           devTest;
	T_USB_GLOBALINTMASK_REG     glbIntMsk;
	T_USB_DEVINTMASKGROUP0_REG  devIntMskGrp0;
	T_USB_DEVINTMASKGROUP2_REG  devIntMskGrp2;
#if _USE_VDMA_FIFO_
	T_USB_DEVVDMA_CTRL_REG      devVdmaCtrl;
#endif

	//set USB Device in unconfigure state
	devAddr.reg          = USB_GETREG(USB_DEVADDR_REG_OFS);
	devAddr.bit.AFT_CONF = 0;
	// Fix printer connect fail if test mode is enabled
	devAddr.bit.RST_DEBOUNCE_INVL = 5; // set debounce time to 5ms
	USB_SETREG(USB_DEVADDR_REG_OFS, devAddr.reg);

	// chip enable
	devMainCtl.reg = USB_GETREG(USB_DEVMAINCTRL_REG_OFS);
	devMainCtl.bit.CHIP_EN    = 1;

#ifdef CONFIG_NVT_FPGA_EMULATION
	devMainCtl.bit.HALF_SPEED = 1;
#else
	devMainCtl.bit.HALF_SPEED = 0;
#endif
	USB_SETREG(USB_DEVMAINCTRL_REG_OFS, devMainCtl.reg);

	// suspend counter
	devIdleCnt.reg = USB_GETREG(USB_DEVIDLECNT_REG_OFS);
	devIdleCnt.bit.IDLE_CNT = 0;
	USB_SETREG(USB_DEVIDLECNT_REG_OFS, devIdleCnt.reg);

	// clear interrupt
	USB_SETREG(USB_DEVINTGROUP2_REG_OFS, 0xFFFFFFFF);

	// disable all fifo interrupt
	USB_SETREG(USB_DEVINTMASKGROUP1_REG_OFS, 0xFFFFFF);

	// soft reset
	devMainCtl.reg = USB_GETREG(USB_DEVMAINCTRL_REG_OFS);
	devMainCtl.bit.SFRST = 1;
	USB_SETREG(USB_DEVMAINCTRL_REG_OFS, devMainCtl.reg);
	// soft reset clear
	devMainCtl.bit.SFRST = 0;
	USB_SETREG(USB_DEVMAINCTRL_REG_OFS, devMainCtl.reg);

	// clear all fifo
	devTest.reg = USB_GETREG(USB_DEVTEST_REG_OFS);
	devTest.bit.TST_CLRFF = 1;
	USB_SETREG(USB_DEVTEST_REG_OFS, devTest.reg);

	// disable Host & OTG interrupt
	glbIntMsk.reg = USB_GETREG(USB_GLOBALINTMASK_REG_OFS);
	glbIntMsk.bit.MOTG_INT = 1;
	glbIntMsk.bit.MHC_INT  = 1;
	glbIntMsk.bit.INT_POLARITY  = 1;
	USB_SETREG(USB_GLOBALINTMASK_REG_OFS, glbIntMsk.reg);

	// maks CX_IN/CX_OUT/CX_COMEND interrupt
	devIntMskGrp0.reg = USB_GETREG(USB_DEVINTMASKGROUP0_REG_OFS);
	devIntMskGrp0.bit.MCX_IN_INT  = 1;
	devIntMskGrp0.bit.MCX_OUT_INT = 1;
	devIntMskGrp0.bit.MCX_COMEND  = 1;
	USB_SETREG(USB_DEVINTMASKGROUP0_REG_OFS, devIntMskGrp0.reg);

	// mask DEV_IDLE interrupt
	devIntMskGrp2.reg = USB_GETREG(USB_DEVINTMASKGROUP2_REG_OFS);
	devIntMskGrp2.bit.MDEV_IDLE             = 1;
	devIntMskGrp2.bit.MDEV_WAKEUP_BYVBUS    = 1;
	devIntMskGrp2.bit.MTX0BYTE_INT    = 1;
	USB_SETREG(USB_DEVINTMASKGROUP2_REG_OFS, devIntMskGrp2.reg);

#if _USE_VDMA_FIFO_
	devVdmaCtrl.reg = USB_GETREG(USB_DEVVDMA_CTRL_REG_OFS);
#ifndef _NVT_EMULATION_
	devVdmaCtrl.bit.VDMA_EN = 0;
	USB_SETREG(USB_DEVVDMA_CTRL_REG_OFS, devVdmaCtrl.reg);
	//DBG_DUMP("TOGGLE VDMA_EN\r\n");
	USB_DELAY_MS(1);
#endif
	devVdmaCtrl.bit.VDMA_EN = 1;
	USB_SETREG(USB_DEVVDMA_CTRL_REG_OFS, devVdmaCtrl.reg);
#endif

	// enable global interrupt
	devMainCtl.reg = USB_GETREG(USB_DEVMAINCTRL_REG_OFS);
	devMainCtl.bit.GLINT_EN = 1;
	USB_SETREG(USB_DEVMAINCTRL_REG_OFS, devMainCtl.reg);

	devPhyTest.reg = USB_GETREG(USB_PHYTSTSELECT_REG_OFS);
	devPhyTest.bit.UNPLUG = 0;
	USB_SETREG(USB_PHYTSTSELECT_REG_OFS, devPhyTest.reg);

	if (gUSBManage.fp_event_callback) {
		gUSBManage.fp_event_callback(USB_EVENT_CONNECT);
	}

	if ((gUsbChargingCB != NULL) && (gChargerSts == USB_CHARGER_STS_NONE)) {
		gUsbChargingCB(USB_CHARGE_EVENT_CONNECT);
	}
}

/*
    Check PC by line status

    @return
        - @b TRUE: PC
        - @b FALSE: no PC (maybe charger)
*/
static BOOL usb_chkPCbyLineSts(void)
{
	T_USB_DEVMAINCTRL_REG  devMainCtl;
	T_USB_PHYTSTSELECT_REG  devPhyTest;
	T_USB_HCPORTSTACTRL_REG portStatusReg;
	unsigned long      flags;

	// Assume UCLK is already enabled

	// chip enable
	devMainCtl.reg = USB_GETREG(USB_DEVMAINCTRL_REG_OFS);
	devMainCtl.bit.CHIP_EN = 1; // bit[5]

	// soft reset
	devMainCtl.reg = USB_GETREG(USB_DEVMAINCTRL_REG_OFS);
	devMainCtl.bit.SFRST = 1; // bit[4]
	USB_SETREG(USB_DEVMAINCTRL_REG_OFS, devMainCtl.reg);
	// soft reset clear
	devMainCtl.bit.SFRST = 0; // bit[4]
	USB_SETREG(USB_DEVMAINCTRL_REG_OFS, devMainCtl.reg);

	// Lock CPU to prevent we raise D+ pull-up too long
	loc_cpu(flags);

	// Enable D+ pull-up
	devPhyTest.reg = USB_GETREG(USB_PHYTSTSELECT_REG_OFS);
	devPhyTest.bit.UNPLUG = 0; // bit[0]
	USB_SETREG(USB_PHYTSTSELECT_REG_OFS, devPhyTest.reg);

	USB_DELAY_US_POLL(20);

	portStatusReg.reg = USB_GETREG(USB_HCPORTSTACTRL_REG_OFS);

	// Restore CPU interrupt
	unl_cpu(flags);

	// Disable D+ pull-up
	devPhyTest.bit.UNPLUG = 1; // bit[0]
	USB_SETREG(USB_PHYTSTSELECT_REG_OFS, devPhyTest.reg);

	if (portStatusReg.bit.LINE_STS == 0x02) { // If line staus == J state
//        DBG_ERR("line sts check: PC\r\n");
		return TRUE;        // PC
	} else {
//        DBG_ERR("line sts check: charger\r\n");
		return FALSE;       // not PC (charger)
	}
}

/*
    Open USB driver internally

    Initialize USB module and enable interrupt, start running
    control & setup task for ISR etc...

    @param[in] bChkCharger      detect usb charger
                                - @b TRUE: only use this API to detect charger
                                - @b FALSE: use this API to connect with usb host
    @param[in] uiDebounce       debounce time to detect usb charger, unit: 10ms

    @return
        - @b E_OK: open success
        - @b E_OACV: USB driver is already opened
        - @b E_ID: Outside semaphore ID number range
        - @b E_NOEXS: Semaphore does not yet exist
        - @b E_SYS: USB has been unplug, so ignore this request...
        - @b E_TMOUT: USB Host does not respond,ex: charger
        - @b E_NOSPT: USB Host is disconnected
*/
static ER usb_openInternal(BOOL bChkCharger, UINT32 uiDebounce)
{
	ER  erReturn;
	T_USB_DEVDMACTRL1_REG       devDMACtl1;
	T_USB_DEVDMACTRL1_REG       devDMACtl1_bak;
	T_USB_OTGCTRLSTATUS_REG     otgCtrlSts;
	T_USB_PHYTOP_REG            RegTop;

	if (gUSBOpen) {
		DBG_ERR("driver already opened\r\n");
		return E_OACV;
	}
	erReturn = usb_lock();
	if (erReturn != E_OK) {
		return erReturn;
	}

	//if (bChkCharger == FALSE) {
	//	erReturn = clr_flg(FLG_ID_USB, FLGPTN_USB);
	//	if (erReturn != E_OK) {
	//		return erReturn;
	//	}
	//}

	//Reason: To avoid re-enter USB device after "Safety Removed" state
	//If we don't anything, we can NOT access USB registers in suspend mode.
	if (usb2dev_get_controller_state() == USB_CONTROLLER_STATE_SUSPEND) {
		//Hardware reset USB register.... try to leave suspend mode.
#ifndef __KERNEL__
#if 0//USB2_TEST
		pll_enableSystemReset(USB2_RSTN);
		pll_disableSystemReset(USB2_RSTN);
#else
		pll_enableSystemReset(USB_RSTN);
		pll_disableSystemReset(USB_RSTN);
#endif

#else
{
	unsigned long      flags;

	loc_cpu(flags);
	iowrite32( (ioread32((void *)(0xFD020084)) & ~(0x1<<19)), (void *)(0xFD020084));
	iowrite32( (ioread32((void *)(0xFD020084)) |  (0x1<<19)), (void *)(0xFD020084));
	unl_cpu(flags);
}
#endif

		usb2dev_power_on_init(bUsbHighSpeed);

		usb_setControllerState(USB_CONTROLLER_STATE_NONE);
	}

	usb_attach();

	// Set Controller as Device Mode
	RegTop.reg = USB_GETREG(USB_PHYTOP_REG_OFS);
	RegTop.bit.USB_ID = 1;
	USB_SETREG(USB_PHYTOP_REG_OFS, RegTop.reg);
	usb2dev_set_vbusi(TRUE);
	if (bChkCharger == FALSE) {
		// start running control & setup task for isr
		USBCtlStartTsk_ID = vos_task_create(USBCtlStartTsk,  0, "USBCtlStartTsk",   TSK_PRI_USBCTRL,	TSK_STKSIZE_USBCTRL);
		vos_task_resume(USBCtlStartTsk_ID);
		USBSetupHalTsk_ID = vos_task_create(USBSetupHalTsk,  0, "USBSetupHalTsk",   TSK_PRI_USBSETUP,	TSK_STKSIZE_USBSETUP);
		vos_task_resume(USBSetupHalTsk_ID);
	}

	// decrease VBUS debounce time from 540us to 200us
	otgCtrlSts.reg = USB_GETREG(USB_OTGCTRLSTATUS_REG_OFS);
	otgCtrlSts.bit.Vbus_FLT_SEL = 1;
	USB_SETREG(USB_OTGCTRLSTATUS_REG_OFS, otgCtrlSts.reg);

	//turn on the usb physical layer.
	devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
	devDMACtl1_bak.reg = devDMACtl1.reg;
	devDMACtl1.bit.DEVPHY_SUSPEND = 0;  // bit[31]
	USB_SETREG(USB_DEVDMACTRL1_REG_OFS, devDMACtl1.reg);
	if (devDMACtl1_bak.bit.DEVPHY_SUSPEND == 1) {
		// wait VBUS debounce time passed
		USB_DELAY_US_POLL(200);
	}

	usb_initFOTG200();

	// Clear FIFOs
	usb2dev_clear_fifo();

#if (USB_STANDARD_CHARGER_DETECT == 1)
	if (bChkCharger == TRUE) {
		return E_OK;
	}
#else
	{
		// Detect USB host by RESET signal
		T_USB_DEVINTGROUP2_REG  devIntGrp2;
		UINT32 i = USB_HOST_TIMEOUT_PERIOD;

		while (i) {
			// The USB Driver interrupt has not enabled. Polling sts is safe.
			devIntGrp2.reg = USB_GETREG(USB_DEVINTGROUP2_REG_OFS);
			if (devIntGrp2.bit.USBRST_INT == 1) {
				//DBG_WRN("#Host:0x%x,i=%d\n\r",devIntGrp2.reg, i);
				break;
			} else {
				USB_DELAY_MS(1);
				i--;
			}
		}
		if (i == 0) {
			DBG_ERR("No USBHost\r\n");
			return E_TMOUT;
		}
	}
#endif

	// Pre-set USB DMA flag to mark DMA channel is available
	set_flg(FLG_ID_USBTSK, FLGPTN_USB_DMA_RDY);

	#ifndef __KERNEL__
	request_irq(INT_ID_USB, usb_isr, IRQF_TRIGGER_HIGH, 0, 0);
	#endif

	return erReturn;
}

/*
    set USB controller state.

    @param[in] eUSBCtrlState    USB controller state
                                - @b USB_CONTROLLER_STATE_NONE: USB is not used
                                - @b USB_CONTROLLER_STATE_RESET: USB is in reset state
                                - @b USB_CONTROLLER_STATE_SUSPEND: USB is in suspend state
    @return void
*/
void usb_setControllerState(USB_CONTROLLER_STATE eUSBCtrlState)
{
	USBControllerState = eUSBCtrlState;
}

/*
    Check Data Contact

    Check Data Contact (DP/DM is connected with host)

    @return
        - @b TRUE: contact with host
        - @b FALSE: NOT contact with host
*/
static BOOL usb_chkDataContact(void)
{
	BOOL bContact = TRUE;
	T_USB_PHYCTRL0B_REG       PhyReg0B;
	T_USB_PHYCTRL0C_REG       PhyReg0C;
	T_USB_CHARGER_CONTROL_REG chargerCtrlReg;

	// Set DP/DM Pull-Down resist as SW control
	PhyReg0B.reg = USBPHY_GETREG(USB_PHYCTRL0B_REG_OFS);
	PhyReg0B.bit.DATARPDM = 1;
	PhyReg0B.bit.DATARPDP = 1;
	USBPHY_SETREG(USB_PHYCTRL0B_REG_OFS, PhyReg0B.reg);

	// Enable DM Pull-Down resist
	PhyReg0C.reg = USBPHY_GETREG(USB_PHYCTRL0C_REG_OFS);
	PhyReg0C.bit.DATARPDM = 1;
	USBPHY_SETREG(USB_PHYCTRL0C_REG_OFS, PhyReg0C.reg);
#if 0
	// Check DCD for 4x3= 12ms
	for (i = 0; i < 3; i++) {
		chargerCtrlReg.reg = USB_GETREG(USB_CHARGER_CONTROL_REG_OFS);
		chargerCtrlReg.bit.DCD_DETECT_EN = 1;
		USB_SETREG(USB_CHARGER_CONTROL_REG_OFS, chargerCtrlReg.reg);

		// The Host is delay about 3.8ms to get result.
		// So we must delay 4ms.
		USB_DELAY_MS(4);

		chargerCtrlReg.reg = USB_GETREG(USB_CHARGER_CONTROL_REG_OFS);
		if (chargerCtrlReg.bit.DCD_DETECT_EN == 1) {
			bContact = FALSE;
			break;
		}
	}
#else
	chargerCtrlReg.reg = USB_GETREG(USB_CHARGER_CONTROL_REG_OFS);
	chargerCtrlReg.bit.DCD_DETECT_EN = 1;
	USB_SETREG(USB_CHARGER_CONTROL_REG_OFS, chargerCtrlReg.reg);

	if (in_interrupt() || in_atomic() || irqs_disabled()) {
		USB_DELAY_US_POLL(11000);
	} else {
		USB_DELAY_MS(11);
	}

	chargerCtrlReg.reg = USB_GETREG(USB_CHARGER_CONTROL_REG_OFS);
	if (chargerCtrlReg.bit.DCD_DETECT_EN == 1) {
		bContact = FALSE;
	}
#endif

	chargerCtrlReg.bit.DCD_DETECT_EN = 0;
	USB_SETREG(USB_CHARGER_CONTROL_REG_OFS, chargerCtrlReg.reg);

	// Disable DM Pull-Down resist
	PhyReg0C.bit.DATARPDM = 0;
	USBPHY_SETREG(USB_PHYCTRL0C_REG_OFS, PhyReg0C.reg);

	// Restore DP/DM Pull-Down resist as HW control
	PhyReg0B.bit.DATARPDM = 0;
	PhyReg0B.bit.DATARPDP = 0;
	USBPHY_SETREG(USB_PHYCTRL0B_REG_OFS, PhyReg0B.reg);

	return bContact;
}

static void usb_enableHighSpeed(BOOL bHighSpeed)
{
	T_USB_DEVMAINCTRL_REG devMainCtl;

	devMainCtl.reg = USB_GETREG(USB_DEVMAINCTRL_REG_OFS);

	if (bHighSpeed) {
		devMainCtl.bit.FORCE_FS = 0; // bit[9]
	} else {
		devMainCtl.bit.FORCE_FS = 1; // bit[9]
	}

	USB_SETREG(USB_DEVMAINCTRL_REG_OFS, devMainCtl.reg);
}

#if _USB2DEV_TODO_
/*
	Power Down Mode Clock Callback to make PHY saving power
*/
static ER usb_powerdown_callback(CLK_PDN_MODE Mode, BOOL bEnter)
{
	#if _FPGA_EMULATION_
	return E_OK;
	#endif

	if (!usb_suspend_to_dramoff_en) {

	if (gUSBOpen) {
		DBG_ERR("USB Driver shall be close first.\r\n");
	}

	if((Mode == CLK_PDN_MODE_SLEEP1) || (Mode <= CLK_PDN_MODE_SLEEP2) || (Mode <= CLK_PDN_MODE_SLEEP3)) {

		if(bEnter) {
			// IP provided to make PHY deeply sleeping
			OUTW(0xF06010D0, 0xFF);
			OUTW(0xF06010D4, 0x34);
			OUTW(0xF0601008, 0x14);
			OUTW(0xF0601000, 0x88);
			OUTW(0xF0601030, 0x35);
			OUTW(0xF060102c, 0xFF);
		} else {
			pll_enableSystemReset(USB_RSTN);
			pll_disableSystemReset(USB_RSTN);

			usb2dev_power_on_init(bUsbHighSpeed);

			usb_setControllerState(USB_CONTROLLER_STATE_NONE);
		}
	}
	}
	return E_OK;
}

static void usb_showDescriptor(UINT8 *pDesc, CHAR *pName)
{
	UINT32 i,len,mark=0;

	if(pDesc == NULL) {
		DBG_DUMP("^RNone of %s Desc\r\n\n",pName);
	} else {
		DBG_DUMP("^Y%s Desc:",pName);

		len = pDesc[0];
		if((pDesc[1] == USB_CONFIGURATION_DESCRIPTOR_TYPE) || (pDesc[1] == USB_BOS_DESCRIPTOR_TYPE))
			len = pDesc[2];

		for(i=0;i<len;i++) {
			if(!(i&0xF))
				DBG_DUMP("\r\n");


			if(i == mark) {
				mark += pDesc[i];
				DBG_DUMP("^C0x%02X ",pDesc[i]);
			} else {
				DBG_DUMP("0x%02X ",pDesc[i]);
			}
		}
		DBG_DUMP("\r\n");
	}
}

BOOL usb_debugDump(CHAR *strCmd);
BOOL usb_debugDump(CHAR *strCmd)
{
	T_USB_TOP_REG           RegTop;
	T_USB_PHYTSTSELECT_REG  devPhyTest;
	T_USB_DEVMAXPS_EP_REG  	EpMaxps;
	UINT32					i;

	RegTop.reg 		= USB_GETREG(USB_TOP_REG_OFS);
	devPhyTest.reg 	= USB_GETREG(USB_PHYTSTSELECT_REG_OFS);
	DBG_DUMP("Device Roll as ");
	if(RegTop.bit.ID) {
		DBG_DUMP("DEVICE.(%s) in %s\r\n",devPhyTest.bit.UNPLUG?"DISABLE":"ENABLE",usb2dev_is_highspeed_enabled()?"HS":"FS");

		for (i = 1; i < 16; i++) {
			EpMaxps.reg = USB_GETREG(USB_DEVMAXPS_EP1_REG_OFS+((i-1)<<2));
			DBG_DUMP("EP%02d-%s(%s) FIFO-Blk-No=%d Ofs=%d\r\n",i,(gUSBManage.ep_config_hs[i-1].Direction == EP_DIR_OUT)?"OUT":"IN "
				,gUSBManage.ep_config_hs[i-1].enable?"ENABLE ":"DISABLE"
				,((gUSBManage.ep_config_hs[i-1].blk_size>512)+1)*gUSBManage.ep_config_hs[i-1].blk_num
				, (gUSBManage.ep_config_hs[i-1].Direction == EP_DIR_OUT)?EpMaxps.bit.FNO_OEP:EpMaxps.bit.FNO_IEP);
		}

		usb_showDescriptor((UINT8 *)gUSBManage.p_dev_desc, 		"DEVICE");
		usb_showDescriptor((UINT8 *)gUSBManage.p_config_desc_hs, 	"HS_CONFIG");
		usb_showDescriptor((UINT8 *)gUSBManage.p_config_desc_fs, 	"FS_CONFIG");
	} else {
		DBG_DUMP("HOST.(%s)\r\n",USB_GETREG(0x10)&0x1?"ENABLE":"DISABLE");
	}

	return TRUE;
}
#endif

static void usb_installCmd(void)
{
#if _USB2DEV_TODO_
	static BOOL bInstall = FALSE;
	static SXCMD_BEGIN(usb, "usb debug message dump")
	SXCMD_ITEM("usb2",  usb_debugDump,  "dump usb 2.0 info")
	SXCMD_END()

	if (bInstall == FALSE) {
		SxCmd_AddTable(usb);
		bInstall = TRUE;
	}
#endif
}

#if 1
/**
    @name USB Driver API
*/
//@{

/**
    Open USB driver

    Initialize USB module and enable interrupt, start running
    control & setup task for ISR etc...

    @return
        - @b E_OK: open success
        - @b E_OACV: USB driver is already opened
        - @b E_ID: Outside semaphore ID number range
        - @b E_NOEXS: Semaphore does not yet exist
        - @b E_SYS: USB has been unplug, so ignore this request...
*/
ER usb2dev_open(void)
{
	usb_installCmd();

	return usb_openInternal(FALSE, 0);
}

/**
    Check usb charger

    Check if connect with a usb charger

    @param[in] uiDebounce       debounce time to detect usb charger, unit: 10ms

    @return
        - @b USB_CHARGER_STS_NONE: PC or not charger detected
        - @b USB_CHARGER_STS_CHARGING_DOWNSTREAM_PORT: charging downstream port
        - @b USB_CHARGER_STS_CHARGER: charger detected
*/
USB_CHARGER_STS usb2dev_check_charger(UINT32 uiDebounce)
{
#if 1
	T_USB_DEVDMACTRL1_REG  devDMACtl1;
	T_USB_DEVDMACTRL1_REG  devDMACtl1_bak;

	if (gUSBOpen) {
		DBG_ERR("driver already opened\r\n");
		return USB_CHARGER_STS_NONE;
	}

	usb_attach();

	devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
	devDMACtl1_bak.reg            = devDMACtl1.reg;
	devDMACtl1.bit.DEVPHY_SUSPEND = 0;
	USB_SETREG(USB_DEVDMACTRL1_REG_OFS, devDMACtl1.reg);

	if ((!bStandardCharger) && (usb_chkDataContact() == FALSE) && gpio_getIntStatus(GPIO_INT_USBPLUGIN)) {
		// If support non-standard charger, VBUS detect, but no D+/D- not correctly contacted
		// Then it might be D+/D- open type charger
		gChargerSts = USB_CHARGER_STS_CHARGER;
	} else {
		// If D+/D- is contacted, follow standard charger detection flow
		T_USB_CHARGER_CONTROL_REG   chargerCtrlReg;
		T_USB_BUS_STATUS_REG        busStsReg;

		chargerCtrlReg.reg = USB_GETREG(USB_CHARGER_CONTROL_REG_OFS);
		chargerCtrlReg.bit.CHG_DETECT_EN = 1;
		USB_SETREG(USB_CHARGER_CONTROL_REG_OFS, chargerCtrlReg.reg);

		if (in_interrupt() || in_atomic() || irqs_disabled()) {
			USB_DELAY_US_POLL(40000);
		} else {
			USB_DELAY_MS(40);
		}

		busStsReg.reg = USB_GETREG(USB_BUS_STATUS_REG_OFS);
		chargerCtrlReg.bit.CHG_DETECT_EN = 0;
		USB_SETREG(USB_CHARGER_CONTROL_REG_OFS, chargerCtrlReg.reg);

		if (busStsReg.bit.CHG_DET == 0) {
			// standard downstream port. i.e. PC

			if (bStandardCharger) {
				gChargerSts = USB_CHARGER_STS_NONE;
			} else {
				if (usb_chkPCbyLineSts() == TRUE) {

					if ((gpio_getIntStatus(GPIO_INT_USBPLUGIN) == 0) ||
						(usb_chkDataContact() == FALSE)) {
						// Double confirm USB plug status
						DBG_ERR("line sts check: disconnect\r\n");
						gChargerSts = USB_CHARGER_STS_DISCONNECT;
					} else {
						DBG_ERR("line sts check: PC\r\n");
						gChargerSts = USB_CHARGER_STS_NONE;
					}
				} else {
					DBG_ERR("line sts check: charger\r\n");
					gChargerSts = USB_CHARGER_STS_CHARGER;
				}
			}
		} else {
			// charger or charging downstream port
			chargerCtrlReg.reg = USB_GETREG(USB_CHARGER_CONTROL_REG_OFS);
			chargerCtrlReg.bit.SECOND_CHG_DETECT_EN = 1;
			USB_SETREG(USB_CHARGER_CONTROL_REG_OFS, chargerCtrlReg.reg);


			if (in_interrupt() || in_atomic() || irqs_disabled()) {
				USB_DELAY_US_POLL(40000);
			} else {
				USB_DELAY_MS(40);
			}

			busStsReg.reg = USB_GETREG(USB_BUS_STATUS_REG_OFS);
			chargerCtrlReg.bit.SECOND_CHG_DETECT_EN = 0;
			USB_SETREG(USB_CHARGER_CONTROL_REG_OFS, chargerCtrlReg.reg);
			if (busStsReg.bit.CHG_DET == 0) {
				// charging downstream port
				if (bStandardCharger) {
					gChargerSts = USB_CHARGER_STS_CHARGING_DOWNSTREAM_PORT;
				} else {

					if (usb_chkPCbyLineSts() == TRUE) {

						if ((gpio_getIntStatus(GPIO_INT_USBPLUGIN) == 0) ||
							(usb_chkDataContact() == FALSE)) {
							// Double confirm USB plug status
							DBG_ERR("line sts check2: disconnect\r\n");
							gChargerSts = USB_CHARGER_STS_DISCONNECT;
						} else {
							DBG_ERR("line sts check2: charge PC\r\n");
							gChargerSts = USB_CHARGER_STS_CHARGING_DOWNSTREAM_PORT;
						}
//                        gChargerSts = USB_CHARGER_STS_CHARGING_DOWNSTREAM_PORT;
					} else {
						DBG_ERR("line sts check2: charger\r\n");
						gChargerSts = USB_CHARGER_STS_CHARGER;
					}
				}
			} else {
				// dedicated charging port
				gChargerSts = USB_CHARGER_STS_CHARGER;
			}
		}
	}

	USB_SETREG(USB_DEVDMACTRL1_REG_OFS, devDMACtl1_bak.reg);

	usb_detach();

	if (gUsbChargingCB != NULL) {
		switch (gChargerSts) {
		case USB_CHARGER_STS_CHARGER:
		case USB_CHARGER_STS_CHARGING_DOWNSTREAM_PORT: {
				gUsbChargingCB(USB_CHARGE_EVENT_CHARGER);
			}
			break;
		default:
			break;
		}
	}

	return gChargerSts;
#else
	return USB_CHARGER_STS_CHARGER;
#endif
}

void usb2dev_init(void)
{
	//vos_flag_create(&FLG_ID_USB,		NULL,	"FLG_ID_USB");
	vos_flag_create(&FLG_ID_USBTSK,		NULL,	"FLG_ID_USBTSK");
	vos_sem_create(&SEMID_USB,	1,		"SEMID_USB");
}

void usb2dev_uninit(void)
{
	vos_sem_destroy(SEMID_USB);
	vos_flag_destroy(FLG_ID_USBTSK);
	//vos_flag_destroy(FLG_ID_USB);
}

static BOOL usb2dev_init_done;

/**
    USB Power on initialization.

    @note This API SHOULD be invoked in system init phase. Else USB may be malfunctioned.

    @param[in] bHighSpeed   set USB in high/full speed mode
                            - @b TRUE: high speed mode
                            - @b FALSE: full speed mode
    @return void
*/
void usb2dev_power_on_init(BOOL bHighSpeed)
{
	T_USB_DEVDMACTRL1_REG       devDMACtl1;
	T_USB_HCMISC_REG            hcMiscReg;

	//DBG_DUMP("enter\r\n");

	if(!usb2dev_init_done) {
		usb2dev_init();
		usb2dev_init_done = 1;
	}

#if 0 //def _FPGA_EMULATION_
#if (_FPGA_EMULATION_ == 1)

	pll_enableSystemReset(USB_RSTN);
	pll_disableSystemReset(USB_RSTN);

	// For PHY board I2C connection
	pUsbI2CObj = i2c_getDrvObject(0);
	if (pUsbI2CObj->open(&gUsbI2cSes) != E_OK) {
		DBG_ERR("Open I2C driver fail\r\n");
		return;
	}
	pUsbI2CObj->setConfig(gUsbI2cSes, I2C_CONFIG_ID_MODE,        I2C_MODE_MASTER);
	pUsbI2CObj->setConfig(gUsbI2cSes, I2C_CONFIG_ID_BUSCLOCK,    80000);
	pUsbI2CObj->setConfig(gUsbI2cSes, I2C_CONFIG_ID_HANDLE_NACK, TRUE);
	// switch to page-0
	usb_writePhyReg(0xFF, 0x00);
	// Switch TC gpio to usb mode.
	usb_writePhyReg(0xBF, 0x0A);
	pUsbI2CObj->close(gUsbI2cSes);

#endif
#endif

	gUsbCtrlMode    = USB_CTRL_MODE_DEVICE;
	//gfpVendorReqHdl = NULL;

	usb_attach();

	// default set to device mode
	usb_setID(TRUE);

	//It is possible that loader has turn off the USB physical layer.
	//So we should turn on the usb physical layer.
	devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
	devDMACtl1.bit.DEVPHY_SUSPEND = 0;
	USB_SETREG(USB_DEVDMACTRL1_REG_OFS, devDMACtl1.reg);

	vos_util_delay_us_polling(50);

	usb_enableHighSpeed(bHighSpeed);
	bUsbHighSpeed = bHighSpeed;

	// Bug fix for host resume pulse
	hcMiscReg.reg = USB_GETREG(USB_HCMISC_REG_OFS);
	hcMiscReg.bit.RESM_SE0_CNT = 0x22;
	USB_SETREG(USB_HCMISC_REG_OFS, hcMiscReg.reg);

	// Clear FIFOs
	usb2dev_clear_fifo();

	gUSBCxFinishAction = USB_ACT_IDLE;

	#if !_FPGA_EMULATION_

	usb_writePhyReg(0x51, 0x20);
	usb_writePhyReg(0x50, 0x30);

	/*
		Apply eFuse settings
	*/
	{
		UINT16 	SWCTRL=4,SQSEL=4,RESINT=0x8;
		UINT32  temp;

		#if 1//_USB2DEV_TODO_
		{
			UINT16 	data = 0;
			INT32 	result;

			result= efuse_readParamOps(EFUSE_USBC_TRIM_DATA, &data);
			if(result >= 0) {
				SWCTRL = data&0x7;
				SQSEL  = (data>>3)&0x7;
				RESINT = (data>>6)&0x1F;
			}
		}
		#endif

		usb_readPhyReg(0x06, &temp);
		temp &= ~(0x7<<1);
		temp |= (SWCTRL<<1);
		usb_writePhyReg(0x06, temp);

		usb_readPhyReg(0x05, &temp);
		temp &= ~(0x7<<2);
		temp |= (SQSEL<<2);
		usb_writePhyReg(0x05, temp);

		USB_SETREG(0x30C,		0x100+RESINT);
		usb_writePhyReg(0x52,	0x60+RESINT);
	}
	usb_writePhyReg(0x51, 0x00);
	#endif

	//USB Device is unattached.
	//Disable USB physcial layer. IT will auto re-function when USB plug-in
	devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
	devDMACtl1.bit.DEVPHY_SUSPEND = 1;
	USB_SETREG(USB_DEVDMACTRL1_REG_OFS, devDMACtl1.reg);

	#ifndef __KERNEL__
	{
		UINT32                  uiApbFreq;
		T_USB_DCD_COUNTER_REG   RegDcdCnt;

		uiApbFreq       = pll_getClockRate(PLL_CLKSEL_APB);

		RegDcdCnt.reg   = USB_GETREG(USB_DCD_COUNTER_REG_OFS);
		if (uiApbFreq == PLL_CLKSEL_APB_80) {
			RegDcdCnt.bit.DCD_COUNT = 808080;
		} else if (uiApbFreq == PLL_CLKSEL_APB_120) {
			RegDcdCnt.bit.DCD_COUNT = 1212120;
		} else if (uiApbFreq == PLL_CLKSEL_APB_60) {
			RegDcdCnt.bit.DCD_COUNT = 606060;
		}else {
			RegDcdCnt.bit.DCD_COUNT = 0;
		}
		USB_SETREG(USB_DCD_COUNTER_REG_OFS, RegDcdCnt.reg);
	}
	#else
	{
		T_USB_DCD_COUNTER_REG   RegDcdCnt;

		RegDcdCnt.reg   = USB_GETREG(USB_DCD_COUNTER_REG_OFS);
		RegDcdCnt.bit.DCD_COUNT = 606060;
		USB_SETREG(USB_DCD_COUNTER_REG_OFS, RegDcdCnt.reg);
	}
	#endif

	//turn off USB clock in real chip to save power!
	usb_detach();

	#ifndef __KERNEL__
	//Set VBUS GPIO status as level trigger.
	//we don't open GPIO here, since we don't require it...
	gpio_setIntTypePol(GPIO_INT_USBPLUGIN, GPIO_INTTYPE_LEVEL, GPIO_INTPOL_POSHIGH);
	#endif

	#if _USB2DEV_TODO_
	clk_set_callback(CLK_CALLBACK_USB_PWRDN, usb_powerdown_callback);
	#endif

	//DBG_DUMP("exit\r\n");

}

/**
    USB Change state

    Calling this function to get the current plug status.
    Application should remember the plug/unplug state to avoid calling
    open/close function multiple!

    @return
        - @b FALSE: Unplug State.
        - @b TRUE: Plug State.
*/
BOOL usb2dev_state_change(void)
{
	if (!gUSBOpen) {
		if (gpio_getIntStatus(GPIO_INT_USBPLUGIN))          // VBUSI GPIO is HIGH
		{
			BOOL bContatct = TRUE;

			if (bStandardCharger) {                         // If standard charger, also check DCD
				T_USB_DEVDMACTRL1_REG  devDMACtl1;
				T_USB_DEVDMACTRL1_REG  devDMACtl1_bak;

				usb_attach();

				devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
				devDMACtl1_bak.reg = devDMACtl1.reg;
				devDMACtl1.bit.DEVPHY_SUSPEND = 0;  // bit[31]
				USB_SETREG(USB_DEVDMACTRL1_REG_OFS, devDMACtl1.reg);

				bContatct = usb_chkDataContact();

				USB_SETREG(USB_DEVDMACTRL1_REG_OFS, devDMACtl1_bak.reg);

				usb_detach();
			}

			if (bContatct) {
				USBPhyDisable = 0;
				usb2dev_set_vbusi(TRUE);
				return TRUE;
			}
		}

		//USB CLK is disable, and GPIO status is off, so USB is in unplug state.
		gChargerSts = USB_CHARGER_STS_NONE;
		USBPhyDisable = 0xFFFFFFFF;
		usb2dev_set_vbusi(FALSE);
		return FALSE;

	} else {

		if (gpio_getIntStatus(GPIO_INT_USBPLUGIN))          // VBUSI GPIO is HIGH
		{
			USBPhyDisable = 0;
			usb2dev_set_vbusi(TRUE);
			return TRUE;
		} else {
			gChargerSts = USB_CHARGER_STS_NONE;
			USBPhyDisable = 0xFFFFFFFF;
			usb2dev_set_vbusi(FALSE);
			return FALSE;
		}
	}
}

/**
    Close USB device driver access.

    This function will stop USB DMA, terminate control and setup task,
    disable interrupt and USB clock etc...

    @return
        - @b E_OK: open success
        - @b E_OACV: USB driver is not opened
        - @b E_ID: Outside semaphore ID number range
        - @b E_NOEXS: Semaphore does not yet exist
        - @b E_QOVR: Semaphore's counter error, maximum counter < counter
*/
ER usb2dev_close(void)
{
	ER                      erReturn;
	FLGPTN                  uiFlag;
	T_USB_PHYTSTSELECT_REG  devPhyTest;
	T_USB_DEVMAINCTRL_REG   devMainCtl;
	T_USB_DEVDMACTRL1_REG   devDMACtl1;

	if (!gUSBOpen) {
		DBG_ERR("driver not opened\r\n");
		return E_OACV;
	}

	#ifndef __KERNEL__
	free_irq(INT_ID_USB, 0);
	#endif

	// Abort All ongoing DMA Channels
	usb_abortDMA(USB_EP_NOT_USE, USB_DMACH_ALL);

	vos_flag_wait_interruptible(&uiFlag, FLG_ID_USBTSK, FLGUSB_SETUPTSK_IDLE, TWF_ORW);
	set_flg(FLG_ID_USBTSK, FLGUSB_SETUPTSK_STOP);

	vos_flag_wait_interruptible(&uiFlag, FLG_ID_USBTSK, FLGUSB_CTLTSK_IDLE, TWF_ORW);
	set_flg(FLG_ID_USBTSK, FLGUSB_CTLTSK_STOP);


	if ((gUsbChargingCB != NULL) && (gChargerSts == USB_CHARGER_STS_NONE)) {
		gUsbChargingCB(USB_CHARGE_EVENT_DISCONNECT);
	}

	// 1. WE CAN NOT READ USB_DetectVBUS() since USB CLK could be turn off in real case!!
	// 2. If you want to use USB_DetectVBUS(), you should check USB CLK is on!
	if (gpio_getIntStatus(GPIO_INT_USBPLUGIN) == 0) {
		// If USB cable is unplug from host
		//Reason: To avoid access USB Register when device enter suspend mode.
		if (usb2dev_get_controller_state() == USB_CONTROLLER_STATE_SUSPEND) {
			//This case is USB device enter suspend mode...
			//Notice: Device will not Reenumeration during this state....
			erReturn = usb_unlock();
			if (erReturn != E_OK) {
				return erReturn;
			}
			return E_OK;
		}

		if (gUSBManage.fp_event_callback) {
			gUSBManage.fp_event_callback(USB_EVENT_DISCONNECT);
		}

		//Clear this flag will be done in AP open if AP want to reconnect USB!
		devPhyTest.reg = USB_GETREG(USB_PHYTSTSELECT_REG_OFS);
		devPhyTest.bit.UNPLUG = 1; // bit[0]
		USB_SETREG(USB_PHYTSTSELECT_REG_OFS, devPhyTest.reg);

		// soft reset
		devMainCtl.reg = USB_GETREG(USB_DEVMAINCTRL_REG_OFS);
		devMainCtl.bit.SFRST = 1; // bit[4]
		USB_SETREG(USB_DEVMAINCTRL_REG_OFS, devMainCtl.reg);
		// soft reset clear
		devMainCtl.bit.SFRST = 0; // bit[4]
		USB_SETREG(USB_DEVMAINCTRL_REG_OFS, devMainCtl.reg);
	}
	else
	{
		#ifdef __KERNEL__
		if (usb2dev_get_controller_state() == USB_CONTROLLER_STATE_SUSPEND) {
			unsigned long      flags;

			loc_cpu(flags);
			iowrite32( (ioread32((void *)(0xFD020084)) & ~(0x1<<19)), (void *)(0xFD020084));
			iowrite32( (ioread32((void *)(0xFD020084)) |  (0x1<<19)), (void *)(0xFD020084));
			unl_cpu(flags);

			usb2dev_power_on_init(bUsbHighSpeed);
		}
		#endif

		// USB cable connect with host
		// Even usb cable is removed, we should remove DP pull-up.
		// Else PC will consider device is still connected
		devPhyTest.reg = USB_GETREG(USB_PHYTSTSELECT_REG_OFS);
		devPhyTest.bit.UNPLUG = 1;
		USB_SETREG(USB_PHYTSTSELECT_REG_OFS, devPhyTest.reg);

		//Disable Physical if device is unplug.
		//It will be failed if device in connect mode...
		devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
		devDMACtl1.bit.DEVPHY_SUSPEND = 1;  // bit[31]
		USB_SETREG(USB_DEVDMACTRL1_REG_OFS, devDMACtl1.reg);

		usb_detach();

		//Reason: To avoid access USB Register when device enter suspend mode.
		usb_setControllerState(USB_CONTROLLER_STATE_NONE);

	}

	gfpVendorReqHdl     = NULL;
	gUsbSetInterfaceHdl = NULL;

	erReturn = usb_unlock();
	if (erReturn != E_OK) {
		return erReturn;
	}

	return E_OK;
}

/**
    Set VBUSI state

    Force VBUSI state for USB controller
    This API should be invoked when upper layer want to force USB connect without usb2dev_state_change()

    @param[in] bPlug    state of current VBUSI
                        - @b TRUE: VBUSI is HIGH
                        - @b FALSE: VBUSI is LOW

    @return void
*/
void usb2dev_set_vbusi(BOOL bPlug)
{
	T_USB_PHYTOP_REG   RegTop;
	unsigned long      flags;

	loc_cpu(flags);

	RegTop.reg = USB_GETREG(USB_PHYTOP_REG_OFS);
	RegTop.bit.USB_VBUSI = bPlug;
	USB_SETREG(USB_PHYTOP_REG_OFS, RegTop.reg);
	unl_cpu(flags);
}

/**
    Set the callback function when wakeup from standby mode

    User can use this API to register call back when system is waked up from power down/standby mode.
    The installed call back will be invoked if system is waked up by USB (resume/reset).

    @param[in] fpIsr    User defined ISR for wakeup interrupt

    @return void
*/
void usb2dev_set_standby_wakeup(DRV_CB fpIsr)
{
#ifndef __KERNEL__
	//clear GPIO_INT_USBWAKEUP status first
	gpio_clearIntStatus(GPIO_INT_USBWAKEUP);
	gpio_enableInt(GPIO_INT_USBWAKEUP);
	gpio_setIntIsr(GPIO_INT_USBWAKEUP, fpIsr);
#endif
}

/**
    get USB controller state.

    @return    USB controller state
        - @b USB_CONTROLLER_STATE_NONE: USB is not used
        - @b USB_CONTROLLER_STATE_RESET: USB is in reset state
        - @b USB_CONTROLLER_STATE_SUSPEND: USB is in suspend state
*/
USB_CONTROLLER_STATE usb2dev_get_controller_state(void)
{
	return USBControllerState;
}

/**
    Check set configuration result

    This API is provided to upper layer to check if USB is under set configuration state.

    @return
        - @b TRUE: under set configuration
        - @b FALSE: not under set configuration
*/
BOOL usb2dev_check_configured(void)
{
	T_USB_DEVADDR_REG devAddr;

	devAddr.reg = USB_GETREG(USB_DEVADDR_REG_OFS);
	return devAddr.bit.AFT_CONF;
}

/**
    Set USB controller configuration

    @param[in] configID         configuration identifier
    @param[in] configContext    configuration context for configID

    @return
        - @b E_OK: set configuration success
        - @b E_NOSPT: configID is not supported
*/
ER usb2dev_set_config(USB_CONFIG_ID configID, UINT32 configContext)
{
	ER erReturn = E_OK;

	switch (configID) {
	case USB_CONFIG_ID_STANDARD_CHARGER: {
			bStandardCharger = configContext;
		}
		break;
	case USB_CONFIG_ID_USE_SHARE_FIFO: {
			gUsbUseShareFIFO = configContext > 0;
		}
		break;
	case USB_CONFIG_ID_SUSPEND_DRAMOFF: {
			usb_suspend_to_dramoff_en = configContext > 0;
		}
		break;


		//
		//  emulation use only.
		//

#if _EMULATION_
	case USB_CONFIG_ID_FIFO_START_OFS: {
			gTestFifoOffset = configContext;
		}
		break;
#else
	case USB_CONFIG_ID_FIFO_START_OFS: {
			DBG_WRN("Reserved function. %d\r\n", configID);
		}
		break;
#endif

	case USB_CONFIG_ID_INTEN: {
			unsigned long      flags;

			loc_cpu(flags);

			if(configContext > 0)
				USB_SETREG(USB_DEVINTGROUPMASK_REG_OFS, 0x0);
			else
				USB_SETREG(USB_DEVINTGROUPMASK_REG_OFS, 0x7);

			unl_cpu(flags);
		}
		break;

	default: {
			erReturn = E_NOSPT;
			DBG_WRN("ID No Suprted (%d)\r\n", configID);
		}
		break;
	}

	return erReturn;
}

/**
    Get USB controller configuration

    @param[in] configID         configuration identifier

    @return configuration context for configID
*/
UINT32 usb2dev_get_config(USB_CONFIG_ID configID)
{
	switch (configID) {
	case USB_CONFIG_ID_STANDARD_CHARGER: {
			return bStandardCharger;
		}
	case USB_CONFIG_ID_SQUELCH_LEVEL: {
			T_USB_PHYCTRL05_REG phySquelch;
			phySquelch.reg = USBPHY_GETREG(USB_PHYCTRL05_REG_OFS);
			return phySquelch.bit.SQ_SEL;
		}
		break;
	case USB_CONFIG_ID_HS_SWING: {
			T_USB_PHYCTRL06_REG phySwingReg;

			phySwingReg.reg = USBPHY_GETREG(USB_PHYCTRL06_REG_OFS);
			return phySwingReg.bit.SWCTRL;
		}
		break;
	case USB_CONFIG_ID_REF_CURRENT: {
			T_USB_PHYCTRL3A_REG phyRefAdjReg;

			phyRefAdjReg.reg = USBPHY_GETREG(USB_PHYCTRL3A_REG_OFS);
			return phyRefAdjReg.bit.IREFSEL;
		}
		break;
	case USB_CONFIG_ID_USE_SHARE_FIFO: {
			return gUsbUseShareFIFO;
		}
		break;
	case USB_CONFIG_ID_CHECK_CXOUT: {
			T_USB_DEVINTGROUP0_REG devintgrp0;

			devintgrp0.reg = USB_GETREG(USB_DEVINTGROUP0_REG_OFS);
			return devintgrp0.bit.CX_OUT_INT;
		}
		break;


	default: {
			return 0;
		}
	}
}

/**
    Set usb call back routine

    This function provides a facility for upper layer to install callback routine.

    @param[in] callBackID   callback routine identifier
    @param[in] pCallBack    callback routine to be installed

    @return
        - @b E_OK: install callback success
        - @b E_NOSPT: callBackID is not valid
*/
ER usb2dev_set_callback(USB_CALLBACK callBackID, USB_GENERIC_CB pCallBack)
{
	switch (callBackID) {
	case USB_CALLBACK_CHARGING_EVENT: {
			gUsbChargingCB = pCallBack;
		}
		break;
	case USB_CALLBACK_CX_VENDOR_REQUEST: {
			gfpVendorReqHdl = pCallBack;
		}
		break;
	case USB_CALLBACK_CX_CLASS_REQUEST: {
			gfpClassRequest = (FP)pCallBack;
		}
		break;
	case USB_CALLBACK_SET_INTERFACE: {
			gUsbSetInterfaceHdl = pCallBack;
		}
		break;
	case USB_CALLBACK_STD_UNKNOWN_REQ:
			gfpStdUnknownReq = pCallBack;
		break;

	default: {
			return E_NOSPT;
		}
	}

	return E_OK;
}

/**
    Get the current setting of USB speed mode.

    @return
        - @b TRUE: device is in High speed
        - @b FALSE: device is in full speed
*/
BOOL usb2dev_is_highspeed_enabled(void)
{
	T_USB_DEVMAINCTRL_REG devMainCtl;

	devMainCtl.reg = USB_GETREG(USB_DEVMAINCTRL_REG_OFS);
	return (BOOL)(devMainCtl.bit.HS_EN);
}

/**
    Initialize USB Management Infomation Table Data Structure

    Initialize USB Management Infomation Table Data Structure.
    User can allocate local parameter of USB_MNG and apply usb_initManagement to it.
    And then start configure the management table.

    @param[in] pMng USB Management Infomation Table pointer

    @return void
*/
void usb2dev_init_management(USB_MNG *pMng)
{
	if (pMng) {
		memset(pMng, 0x00, sizeof(USB_MNG));
		pMng->magic_no = USB_MNG_MAGICNO;
	}
}

/**
    Apply USB Management Infomation Table

    Apply USB Management Infomation Table to USB Driver.

    @param[in] pMng USB Management Infomation Table pointer

    @return
     - @b E_OK:  Apply management info success.
     - @b E_PAR: Input parameter is NULL pointer.
     - @b E_CTX: No Init. Please apply usb_initManagement() to pMng first.
*/
ER usb2dev_set_management(USB_MNG *pMng)
{
	if (!pMng) {
		DBG_ERR("NULL PTR!\r\n");
		return E_PAR;
	}

	if (pMng->magic_no != USB_MNG_MAGICNO) {
		DBG_ERR("UN-INIT!\r\n");
		return E_CTX;
	}

	memcpy(&gUSBManage, pMng, sizeof(USB_MNG));
	return E_OK;
}

/**
    Get Current USB Management Infomation Table

    Get Current  USB Management Infomation Table to USB Driver.

    @param[out] pMng USB Management Infomation Table pointer

    @return
     - @b E_OK:  Apply management info success.
     - @b E_PAR: Input parameter is NULL pointer.
     - @b E_CTX: No Init. Please apply usb_initManagement() to pMng first.
*/
ER usb2dev_get_management(USB_MNG *pMng)
{
	if (!pMng) {
		DBG_ERR("NULL PTR!\r\n");
		return E_PAR;
	}

	memcpy(pMng, &gUSBManage, sizeof(USB_MNG));
	return E_OK;
}

#endif

#if (_EMULATION_ == ENABLE)
/*
    Open USB reset debounce test
    (Engineer usage API)

    @param[in] uiDebounce   reset debounce interval (unit: ms)

    @return
        - @b E_OK: open success
        - @b Else: fail
*/
ER usb_openDebounceTest(UINT32 uiDebounce)
{
	T_USB_DEVDMACTRL1_REG   devDMACtl1;
	T_USB_DEVDMACTRL1_REG   devDMACtl1_bak;
	T_USB_OTGCTRLSTATUS_REG otgCtrlSts;
	T_USB_DEVMAINCTRL_REG   devMainCtl;
	T_USB_PHYTSTSELECT_REG  devPhyTest;
	T_USB_DEVADDR_REG       devaddr;
	//if (gUSBOpen)
	//{
	//    DBG_ERR("driver already opened\r\n");
	//    return E_OACV;
	//}

	//erReturn = usb_lock();
	//if (erReturn != E_OK)
	//{
	//    return erReturn;
	//}

	//Reason: To avoid re-enter USB device after "Safety Removed" state
	//If we don't anything, we can NOT access USB registers in suspend mode.
	if (usb2dev_get_controller_state() == USB_CONTROLLER_STATE_SUSPEND) {
		//Hardware reset USB register.... try to leave suspend mode.
#if _USE_USB_CH2_
		pll_enableSystemReset(USB2_RSTN);
		pll_disableSystemReset(USB2_RSTN);
#else
		pll_enableSystemReset(USB_RSTN);
		pll_disableSystemReset(USB_RSTN);
#endif

		usb_setControllerState(USB_CONTROLLER_STATE_NONE);
	}

	usb_attach();

	otgCtrlSts.reg = USB_GETREG(USB_OTGCTRLSTATUS_REG_OFS);
	otgCtrlSts.bit.Vbus_FLT_SEL = 1;    // decrease VBUS debounce time from 540ms to 200ms
	USB_SETREG(USB_OTGCTRLSTATUS_REG_OFS, otgCtrlSts.reg);

	//turn on the usb physical layer.
	devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
	devDMACtl1_bak.reg = devDMACtl1.reg;
	devDMACtl1.bit.DEVPHY_SUSPEND = 0;  // bit[31]
	USB_SETREG(USB_DEVDMACTRL1_REG_OFS, devDMACtl1.reg);
	if (devDMACtl1_bak.bit.DEVPHY_SUSPEND == 1) {
		// wait VBUS debounce time passed
		USB_DELAY_US_POLL(200);
	}

	// chip enable
	devMainCtl.reg = USB_GETREG(USB_DEVMAINCTRL_REG_OFS);
	devMainCtl.bit.CHIP_EN = 1; // bit[5]
#ifdef _FPGA_EMULATION_
#if (_FPGA_EMULATION_ == 1)
	devMainCtl.bit.HALF_SPEED = 1;
#endif
#endif
	// Write tested debounce interval
	devaddr.reg = USB_GETREG(USB_DEVADDR_REG_OFS);
	devaddr.bit.RST_DEBOUNCE_INVL = uiDebounce;
	USB_SETREG(USB_DEVADDR_REG_OFS, devaddr.reg);

	// soft reset
	devMainCtl.reg = USB_GETREG(USB_DEVMAINCTRL_REG_OFS);
	devMainCtl.bit.SFRST = 1; // bit[4]
	USB_SETREG(USB_DEVMAINCTRL_REG_OFS, devMainCtl.reg);
	// soft reset clear
	devMainCtl.bit.SFRST = 0; // bit[4]
	USB_SETREG(USB_DEVMAINCTRL_REG_OFS, devMainCtl.reg);

#if 0//USB2_TEST
	drv_disableInt(DRV_INT_USB2);
#else
	//drv_disableInt(DRV_INT_USB);
#endif

	devPhyTest.reg = USB_GETREG(USB_PHYTSTSELECT_REG_OFS);
	devPhyTest.bit.UNPLUG = 0; // bit[0]
	USB_SETREG(USB_PHYTSTSELECT_REG_OFS, devPhyTest.reg);

	return E_OK;
}

/*
    Read USB interupt status group2
    (Engineer usage API)

    For test f/w to get interrupt status of reset flag

    @return interrupt status
*/
UINT32 usb_readIntGrp2(void)
{
	return USB_GETREG(USB_DEVINTGROUP2_REG_OFS);
}

/*
    Read line status
    (Engineer usage API)

    For test f/w to get line status

    @return line status
*/
UINT32 usb_getLineStatus(void)
{
	T_USB_HCPORTSTACTRL_REG portStatusReg;

	portStatusReg.reg = USB_GETREG(USB_HCPORTSTACTRL_REG_OFS);
	return portStatusReg.bit.LINE_STS;
}

/*
    Test Data Contact

    Test Data Contact (for h/w verification)

    @return void
*/
void usb_testDataContact(void)
{
	T_USB_DEVDMACTRL1_REG  devDMACtl1;
	//T_USB_DEVDMACTRL1_REG  devDMACtl1_bak;

	usb_attach();

	devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
	//devDMACtl1_bak.reg = devDMACtl1.reg;
	devDMACtl1.bit.DEVPHY_SUSPEND = 0;  // bit[31]
	USB_SETREG(USB_DEVDMACTRL1_REG_OFS, devDMACtl1.reg);

	usb2dev_set_vbusi(TRUE);

	if (usb_chkDataContact()) {
		DBG_DUMP("\r\nUSB data contact detected\r\n");
	} else {
		DBG_DUMP("\r\nUSB data contact NOT detected\r\n");
	}

	usb2dev_set_vbusi(FALSE);

	usb_detach();

}

/*
    Test charging port

    Test charging port (for h/w verification)

    @return void
*/
void usb_testChargingPort(void)
{
#if 0
	T_USB_CHARGER_CONTROL_REG chargerCtrlReg;
	T_USB_BUS_STATUS_REG busStsReg;
	T_USB_DEVDMACTRL1_REG  devDMACtl1;
	//T_USB_DEVDMACTRL1_REG  devDMACtl1_bak;

	usb_attach();

	devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
	//devDMACtl1_bak.reg = devDMACtl1.reg;
	devDMACtl1.bit.DEVPHY_SUSPEND = 0;  // bit[31]
	USB_SETREG(USB_DEVDMACTRL1_REG_OFS, devDMACtl1.reg);

	DBG_ERR("Vdat_ref: Detect Range (0.25 - 0.4)V  to  (0.8 ~ 2.0)V   \r");
	DBG_ERR("Press 'q' to exit...\r\n");

	chargerCtrlReg.reg = USB_GETREG(USB_CHARGER_CONTROL_REG_OFS);
	chargerCtrlReg.bit.CHG_DETECT_EN = 1;
	USB_SETREG(USB_CHARGER_CONTROL_REG_OFS, chargerCtrlReg.reg);

	while (1) {
		CHAR input;

		busStsReg.reg = USB_GETREG(USB_BUS_STATUS_REG_OFS);
		if (busStsReg.bit.CHG_DET == 0) {
			DBG_ERR("Vdat_ref: outside range\r");
		} else {
			DBG_ERR("Vdat_ref: inside range\r");
		}

		uart_chkChar(&input);
		if (input == 'q') {
			break;
		}

		USB_DELAY_MS(50);
	}

	chargerCtrlReg.bit.CHG_DETECT_EN = 0;
	USB_SETREG(USB_CHARGER_CONTROL_REG_OFS, chargerCtrlReg.reg);

	usb_detach();
#endif
}

/*
    Test charging port secondary

    Test charging port secondary (for h/w verification)

    @return void
*/
void usb_testChargingPortSecondary(void)
{
#if 0
	T_USB_CHARGER_CONTROL_REG chargerCtrlReg;
	T_USB_BUS_STATUS_REG busStsReg;
	T_USB_DEVDMACTRL1_REG  devDMACtl1;
	//T_USB_DEVDMACTRL1_REG  devDMACtl1_bak;

	usb_attach();

	devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
	//devDMACtl1_bak.reg = devDMACtl1.reg;
	devDMACtl1.bit.DEVPHY_SUSPEND = 0;  // bit[31]
	USB_SETREG(USB_DEVDMACTRL1_REG_OFS, devDMACtl1.reg);

	DBG_ERR("Vdat_ref: Detect Range (0.25 - 0.4)V  to  (0.8 ~ 2.0)V   \r");
	DBG_ERR("Press 'q' to exit...\r\n");

	chargerCtrlReg.reg = USB_GETREG(USB_CHARGER_CONTROL_REG_OFS);
	chargerCtrlReg.bit.SECOND_CHG_DETECT_EN = 1;
	USB_SETREG(USB_CHARGER_CONTROL_REG_OFS, chargerCtrlReg.reg);

	while (1) {
		CHAR input;

		busStsReg.reg = USB_GETREG(USB_BUS_STATUS_REG_OFS);
		if (busStsReg.bit.CHG_DET == 0) {
			DBG_ERR("Vdat_ref: outside range\r");
		} else {
			DBG_ERR("Vdat_ref: inside range\r");
		}

		uart_chkChar(&input);
		if (input == 'q') {
			break;
		}

		USB_DELAY_MS(50);
	}

	chargerCtrlReg.bit.SECOND_CHG_DETECT_EN = 0;
	USB_SETREG(USB_CHARGER_CONTROL_REG_OFS, chargerCtrlReg.reg);


	usb_detach();
#endif
}
#endif

/*

*/
void usb_setID(BOOL bDevice)
{
	T_USB_PHYTOP_REG RegTop;
	unsigned long    flags;

	loc_cpu(flags);

	RegTop.reg = USB_GETREG(USB_PHYTOP_REG_OFS);
	RegTop.bit.USB_ID = bDevice;
	USB_SETREG(USB_PHYTOP_REG_OFS, RegTop.reg);
	unl_cpu(flags);
}


/*
    Write USB PHY register

    @param[in] uiOffset     register address
    @param[in] uiValue      register value

    @return
        - @b E_OK: success
*/
ER usb_writePhyReg(UINT32 uiOffset, UINT32 uiValue)
{
#ifdef _FPGA_EMULATION_
#if (_FPGA_EMULATION_ == 1)
	I2C_DATA    I2cData;
	I2C_BYTE    I2cByte[4];
	I2C_STS     I2cSts;
	UINT32      ADDR = 0xC0;

	i2c_lock(gUsbI2cSes);

	I2cData.pByte           = I2cByte;
	I2cData.ByteCount       = I2C_BYTE_CNT_3;

	I2cByte[0].uiValue      = ADDR;
	I2cByte[0].Param        = I2C_BYTE_PARAM_START;

	I2cByte[1].uiValue      = uiOffset;
	I2cByte[1].Param        = I2C_BYTE_PARAM_NONE;

	I2cByte[2].uiValue      = uiValue;
	I2cByte[2].Param        = I2C_BYTE_PARAM_STOP;

	I2cSts = i2c_transmit(&I2cData);
	if ((I2cSts != I2C_STS_OK) && (I2cSts != I2C_STS_STOP)) {
		DBG_ERR("Tx Err. %d\r\n", I2cSts);
	}

	i2c_unlock(gUsbI2cSes);
#else
	// Real board
	USBPHY_SETREG(USB_PHY_BASE_OFS + uiOffset * 4, uiValue);

	// [510 bug patch]: Add dummy read after write
	//USB_GETREG(USB_PHY_BASE_OFS + uiOffset * 4);
#endif
#endif

	return E_OK;
}

/*
    Read USB PHY register

    @param[in] uiOffset     register address
    @param[out] puiValue    register value

    @return
        - @b E_OK: success
*/
ER usb_readPhyReg(UINT32 uiOffset, UINT32 *puiValue)
{
#ifdef _FPGA_EMULATION_
#if (_FPGA_EMULATION_ == 1)
	I2C_DATA    I2cData;
	I2C_BYTE    I2cByte[4];
	I2C_STS     I2cSts;
	UINT32      ADDR = 0xC0;

	i2c_lock(gUsbI2cSes);

	I2cData.pByte           = I2cByte;
	I2cData.ByteCount       = I2C_BYTE_CNT_3;

	I2cByte[0].uiValue      = ADDR;
	I2cByte[0].Param        = I2C_BYTE_PARAM_START;

	I2cByte[1].uiValue      = uiOffset;
	I2cByte[1].Param        = I2C_BYTE_PARAM_NONE;

	I2cByte[2].uiValue      = ADDR | 0x1;
	I2cByte[2].Param        = I2C_BYTE_PARAM_START;

	I2cSts = i2c_transmit(&I2cData);
	if (I2cSts != I2C_STS_OK) {
		DBG_ERR("Tx Err1. %d\r\n", I2cSts);
	}


	I2cData.ByteCount   = I2C_BYTE_CNT_1;
	I2cByte[0].Param    = I2C_BYTE_PARAM_NACK | I2C_BYTE_PARAM_STOP;
	I2cSts = i2c_receive(&I2cData);
	if ((I2cSts != I2C_STS_OK) && (I2cSts != I2C_STS_STOP)) {
		DBG_ERR("RX Err. %d\r\n", I2cSts);
	}

	i2c_unlock(gUsbI2cSes);

	*puiValue = I2cByte[0].uiValue;
#else
	// Real board
	*puiValue = USBPHY_GETREG(USB_PHY_BASE_OFS + uiOffset * 4);
#endif
#endif

	return E_OK;
}
//@}

//@}
