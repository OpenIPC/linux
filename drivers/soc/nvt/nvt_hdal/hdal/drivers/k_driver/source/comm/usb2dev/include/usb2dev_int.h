/*
    USB device driver internal header file

    This file define some internal data structures for USB device

    @file       usb_int.h
    @ingroup    mIDrvUSB_Device
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#ifndef _USBINT_H
#define _USBINT_H

#ifndef __KERNEL__
#include "rcw_macro.h"
#include "kwrap/type.h"
#include "io_address.h"
#include "interrupt.h"
#include "dma.h"
#include "dma_protected.h"
#include "gpio.h"

#include "kwrap/error_no.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "kwrap/spinlock.h"
#include "kwrap/task.h"
#include "kwrap/platform.h"

#define __MODULE__    rtos_usb2dev
#include <kwrap/debug.h>
extern unsigned int rtos_usb2dev_debug_level;

#else
#include "mach/rcw_macro.h"
#include "kwrap/type.h"
#include "usb2dev_drv.h"
#include "usb2dev_dbg.h"
#include  <mach/fmem.h>

#include "kwrap/error_no.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "kwrap/spinlock.h"
#include "kwrap/task.h"
#include "kwrap/util.h"
#include "kwrap/platform.h"


#define _USB_KERN_SELFTEST 0

#if _USB_KERN_SELFTEST
#define dma_flushWriteCache(addr, size) fmem_dcache_sync((void *)addr, size, DMA_TO_DEVICE)
#define dma_flushReadCache(addr, size) fmem_dcache_sync((void *)addr, size, DMA_FROM_DEVICE)
#define dma_getPhyAddr(parm) fmem_lookup_pa(parm)
#else
#define dma_flushWriteCache(addr, size) //fmem_dcache_sync((void *)addr, size, DMA_TO_DEVICE)
#define dma_flushReadCache(addr, size) //fmem_dcache_sync((void *)addr, size, DMA_FROM_DEVICE)
#define dma_getPhyAddr(parm) parm//fmem_lookup_pa(parm)
#endif
#define dma_flushWriteCache2(addr, size) fmem_dcache_sync((void *)addr, size, DMA_TO_DEVICE)
#define dma_flushReadCache2(addr, size) fmem_dcache_sync((void *)addr, size, DMA_FROM_DEVICE)
#define dma_getPhyAddr2(parm) fmem_lookup_pa(parm)

#endif




#include "usb2dev.h"




#define _USB2DEV_TODO_ 0

#define _FPGA_EMULATION_ 0
#define _EMULATION_ 0
#define _USE_VDMA_FIFO_ 1
#define _USE_CXF_PIO_	0


#if _EMULATION_
#define USB_FIFOMAP_FIFO4FIRST          0
#else
#define USB_FIFOMAP_FIFO4FIRST          0
#endif

#ifdef __KERNEL__
extern UINT32 _USB2DEV_REG_BASE_ADDR[MODULE_REG_NUM];
#define USB_SETREG(ofs, value)          iowrite32(value, (void *)(_USB2DEV_REG_BASE_ADDR[0] + ofs))
#define USB_GETREG(ofs)                 ioread32((void *)(_USB2DEV_REG_BASE_ADDR[0] + ofs))
#define USBPHY_SETREG(ofs, value)       OUTW((void *)0xFD600000, (value))
#define USBPHY_GETREG(ofs)              INW((void *)0xFD600000+(ofs))
#else
#define USB_SETREG(ofs, value)          OUTW(IOADDR_USB_REG_BASE+(ofs), (value))
#define USB_GETREG(ofs)                 INW(IOADDR_USB_REG_BASE+(ofs))
#endif

#define _USB_CHECK_PARAM_               0      ///< for debug message
#define USB_MNG_MAGICNO                 0xAD5A609F

#define USB2_POWERDOWN_TEST				DISABLE

//#define FLGPTN_USB              FLGPTN_BIT(0)
//#define FLGPTN_USB_DMA_RDY      FLGPTN_BIT(1)

#define USB_DELAY_US_POLL(x)	vos_util_delay_us_polling(x)
#define USB_DELAY_US(x)			vos_task_delay_ms(((x+999)/1000))
#define USB_DELAY_MS(x)			vos_task_delay_ms((x))
#define wai_sem					vos_sem_wait
#define sig_sem					vos_sem_sig


#define TSK_PRI_USBCTRL        9
#define TSK_PRI_USBSETUP       9
#define TSK_STKSIZE_USBCTRL    4096
#define TSK_STKSIZE_USBSETUP   4096

//
//  USB Trimming Define
//
#define USB_TRIM_SWCTRL_DEFAULT         0x04
#define USB_TRIM_SWCTRL_BITMASK         0x07
#define USB_TRIM_SWCTRL_BITOFS          0

#define USB_TRIM_SQSEL_DEFAULT          0x04
#define USB_TRIM_SQSEL_BITMASK          0x38
#define USB_TRIM_SQSEL_BITOFS           3

#define USB_TRIM_IREFSEL_DEFAULT        0x01
#define USB_TRIM_IREFSEL_BITMASK        0xC0
#define USB_TRIM_IREFSEL_BITOFS         6

#define USB_TRIM_RES_DEFAULT            0xD
#define USB_TRIM_RES_BITMASK            0x7C0
#define USB_TRIM_RES_BITOFS             6


#if _USB_CHECK_PARAM_
#define USB_PutString(x)          printk(x)
#define USB_PutString1         		printk
#else
#define USB_PutString(x)
#define USB_PutString1(x,y)
#endif

// add compilation option for STANDARD/OLY charger detection
#define USB_STANDARD_CHARGER_DETECT     (1)     // 0: for OLY charger detect, 1: for USB charger spec

// Detect whether connecting to a real USB host
#define USB_HOST_TIMEOUT_PERIOD         2000    //uint:ms

//#define USB_MAX_DMA_LENGTH      0x1ffff       ///< 17 bits, 128KB-1


// Flag patterns : FLG_ID_USBTSK
#define FLGUSB_SIG              FLGPTN_BIT(0)      //  USB signal (interrupt)
#define FLGUSB_SETUP            FLGPTN_BIT(1)      //  USB EP0 SETUP INT
#define FLGUSB_CTLTSK_IDLE      FLGPTN_BIT(2)      //  USB control task idle
#define FLGUSB_SETUPTSK_IDLE    FLGPTN_BIT(3)      //  USB setup task idle
#define FLGUSB_EP0_DMADONE      FLGPTN_BIT(6)      //  USB setup task idle
#define FLGUSB_DMADONE          FLGPTN_BIT(7)      //  DMA  done (or abort)
#define FLGUSB_DMA0DONE         FLGPTN_BIT(8)      //  DMA2 done (or abort)
#define FLGUSB_DMA1DONE         FLGPTN_BIT(9)      //  DMA2 done (or abort)
#define FLGUSB_DMA2DONE         FLGPTN_BIT(10)     //  DMA2 done (or abort)
#define FLGUSB_DMA3DONE         FLGPTN_BIT(11)     //  DMA3 done (or abort)
#define FLGUSB_DMA4DONE         FLGPTN_BIT(12)     //  DMA4 done (or abort)
#define FLGUSB_DMA5DONE         FLGPTN_BIT(13)     //  DMA5 done (or abort)
#define FLGUSB_DMA6DONE         FLGPTN_BIT(14)     //  DMA5 done (or abort)
#define FLGUSB_DMA7DONE         FLGPTN_BIT(15)     //  DMA5 done (or abort)
#define FLGUSB_DMA8DONE         FLGPTN_BIT(16)     //  DMA5 done (or abort)
#define FLGUSB_DMA9DONE         FLGPTN_BIT(17)     //  DMA5 done (or abort)
#define FLGUSB_DMA10DONE        FLGPTN_BIT(18)     //  DMA5 done (or abort)
#define FLGUSB_DMA11DONE        FLGPTN_BIT(19)     //  DMA5 done (or abort)
#define FLGUSB_DMA12DONE        FLGPTN_BIT(20)     //  DMA5 done (or abort)
#define FLGUSB_DMA13DONE        FLGPTN_BIT(21)     //  DMA5 done (or abort)
#define FLGUSB_DMA14DONE        FLGPTN_BIT(22)     //  DMA5 done (or abort)
#define FLGUSB_DMA15DONE        FLGPTN_BIT(23)     //  DMA5 done (or abort)
#define FLGUSB_SETUPTSK_STOP    FLGPTN_BIT(24)     //  USB setup task stop
#define FLGUSB_CTLTSK_STOP      FLGPTN_BIT(25)     //  USB control task stop
#define FLGUSB_USER_EVENT_SET   FLGPTN_BIT(26)     //  User Space Event Set
#define FLGUSB_USER_EVENT_DONE  FLGPTN_BIT(27)     //  User Space Event Done

#define FLGUSB_DMADONEALL       (FLGUSB_DMADONE|FLGUSB_DMA0DONE|FLGUSB_DMA1DONE|FLGUSB_DMA2DONE|FLGUSB_DMA3DONE|FLGUSB_DMA4DONE|FLGUSB_DMA5DONE|FLGUSB_DMA6DONE|FLGUSB_DMA7DONE|FLGUSB_DMA8DONE|FLGUSB_DMA9DONE|FLGUSB_DMA10DONE|FLGUSB_DMA11DONE)

#define FLGPTN_USB_DMA_RDY      FLGPTN_BIT(28)

#define BIT0            0x00000001
#define BIT1            0x00000002
#define BIT2            0x00000004
#define BIT3            0x00000008
#define BIT4            0x00000010
#define BIT5            0x00000020
#define BIT6            0x00000040
#define BIT7            0x00000080
#define BIT8            0x00000100
#define BIT9            0x00000200
#define BIT10           0x00000400
#define BIT11           0x00000800
#define BIT12           0x00001000
#define BIT13           0x00002000
#define BIT14           0x00004000
#define BIT15           0x00008000
#define BIT16           0x00010000
#define BIT17           0x00020000
#define BIT18           0x00040000
#define BIT19           0x00080000
#define BIT20           0x00100000
#define BIT21           0x00200000
#define BIT22           0x00400000
#define BIT23           0x00800000
#define BIT24           0x01000000
#define BIT25           0x02000000
#define BIT26           0x04000000
#define BIT27           0x08000000
#define BIT28           0x10000000
#define BIT29           0x20000000
#define BIT30           0x40000000
#define BIT31           0x80000000

#define TEST_J          0x02
#define TEST_K          0x04
#define TEST_SE0_NAK    0x08
#define TEST_PKY        0x10

#define USB_I2C_CLK_GPIO        (P_GPIO_2)
#define USB_I2C_DAT_GPIO        (P_GPIO_0)


/*
   USB speed
*/
typedef enum {
	USB_DEV_HS,                         // USB device in High Speed
	USB_DEV_FS,                         // USB device in Full Speed
	ENUM_DUMMY4WORD(USB_DEV_SPEED)
} USB_DEV_SPEED;

typedef enum {
	USB_ACT_IDLE,                       // CX action idle
	USB_ACT_DONE,                       // CX action done
	USB_ACT_STALL,
	ENUM_DUMMY4WORD(USB_ACTION)
} USB_ACTION;

/*
   USB FIFO number 0~3
*/
typedef enum {
	USB_FIFO0,                          // USB FIFO number 0
	USB_FIFO1,                          // USB FIFO number 1
	USB_FIFO2,                          // USB FIFO number 2
	USB_FIFO3,                          // USB FIFO number 3
	USB_FIFO4,                          // USB FIFO number 4
	USB_FIFO5,                          // USB FIFO number 5
	USB_FIFO6,                          // USB FIFO number 6
	USB_FIFO7,                          // USB FIFO number 7
	USB_FIFO8,                          // USB FIFO number 8
	USB_FIFO9,                          // USB FIFO number 9
	USB_FIFO10,                         // USB FIFO number 10
	USB_FIFO11,                         // USB FIFO number 11
	USB_FIFO12,                         // USB FIFO number 12
	USB_FIFO13,                         // USB FIFO number 13
	USB_FIFO14,                         // USB FIFO number 14
	USB_FIFO15,                         // USB FIFO number 15

	USB_FIFO_MAX,
	USB_FIFO_NOT_USE,
	ENUM_DUMMY4WORD(USB_FIFO_NUM)
} USB_FIFO_NUM;

/*
   USB FIFO direction
*/
typedef enum {
	USB_FIFO_OUT,                       // FIFO direction OUT
	USB_FIFO_IN,                        // FIFO direction IN
	USB_FIFO_BI,                        // FIFO bi-direction
	USB_FIFO_NOT,                       // FIFO not used
	ENUM_DUMMY4WORD(USB_FIFO_DIR)
} USB_FIFO_DIR;

/*
    USB line state definition
*/
typedef enum {
	USB_LINESTS_SE0,                    // line status: SE0
	USB_LINESTS_J,                      // line status: J state
	USB_LINESTS_K,                      // line status: K state
	USB_LINESTS_SE1,                    // line status: SE1
	ENUM_DUMMY4WORD(USB_LINESTS)
} USB_LINESTS;

/*
    USB DMA Channel
*/
typedef enum {
	USB_DMACH_CH1   = 0x01,
	USB_DMACH_CH2   = 0x02,
	USB_DMACH_CH3   = 0x04,
	USB_DMACH_CH4   = 0x08,
	USB_DMACH_CH5   = 0x10,
	USB_DMACH_CH6   = 0x20,
	USB_DMACH_CH7   = 0x40,
	USB_DMACH_CH8   = 0x80,
	USB_DMACH_CH9   = 0x100,
	USB_DMACH_CH10  = 0x200,
	USB_DMACH_CH11  = 0x400,
	USB_DMACH_CH12  = 0x800,
	USB_DMACH_CH13  = 0x1000,
	USB_DMACH_CH14  = 0x2000,
	USB_DMACH_CH15  = 0x4000,
	USB_DMACH_CH16  = 0x8000,

	USB_DMACH_CONTROL= 0x80000000,
	USB_DMACH_ALL    = 0x8000FFFF,

	ENUM_DUMMY4WORD(USB_DMACH)
} USB_DMACH;


/*
    USB UVC Auto Header Config Struct
*/
typedef struct {
	USB_EP  EPn;

	BOOL    bEn;
	BOOL    bStart;
	BOOL    bStop;
	UINT8   uiLength;

} USB_AUTOHDR_CFG;



extern ID FLG_ID_USBTSK;
//extern ID FLG_ID_USB;

extern THREAD_HANDLE USBCtlStartTsk_ID;
extern THREAD_HANDLE USBSetupHalTsk_ID;

extern THREAD_RETTYPE USBCtlStartTsk(void *pvParameters);
extern THREAD_RETTYPE USBSetupHalTsk(void *pvParameters);

extern USB_MNG          gUSBManage;

extern void usb_devINTGrp3(void);

extern unsigned short   bUSBflag_configuration;
extern UINT32           gUSBCause;
extern UINT32           gUSBCauseGroup[4];
extern UINT32           gEPMap[15];
extern UINT32  			gFIFOInMap[USB_FIFO_MAX];
extern UINT32  			gFIFOOutMap[USB_FIFO_MAX];
extern USB_ACTION       gUSBCxFinishAction;
extern BOOL             bAbortDMA;
extern BOOL             gUSBOpen;
extern USB_GENERIC_CB   gUsbSetInterfaceHdl;
extern USB_GENERIC_CB   gUsbReadQueueDone;
extern USB_GENERIC_CB   gUsbChargingCB;
extern USB_CHARGER_STS  gChargerSts;
extern BOOL             gUsbUseShareFIFO;
#if _EMULATION_
extern USB_FIFO_NUM     gTestFifoOffset;
#endif
extern USB_GENERIC_CB   gfpStdUnknownReq;

extern BOOL				usb_suspend_to_dramoff_en;

extern USB_CTRL_MODE    gUsbCtrlMode;

extern USB_GENERIC_CB   gfpVendorReqHdl;
extern FP				gfpClassRequest;
extern UINT8            gUSBIsrCause;


void                    ReqReserved(void);
BOOL                    usb_EPStallStatus(USB_EP EPn);

void                    usb_FIFO_EPxCfg(void);
USB_EP_DIR              usb_getEpDirection(USB_EP EPn);
USB_EP                  usb_getInEpFromFifo(USB_FIFO_NUM FIFOn);
USB_EP                  usb_getOutEpFromFifo(USB_FIFO_NUM FIFOn);
USB_FIFO_NUM            usb_getFifoFromEp(USB_EP EPn);
BOOL                    usb_abortDMA(USB_EP EPn, USB_DMACH DmaCH);

extern void (*StandardDeviceRequest[])(void);
extern void (*USBINTVector[])(void);
extern volatile UINT32  USBPhyDisable;

extern void             usb_initFOTG200(void);
extern void             usb_setControllerState(USB_CONTROLLER_STATE eUSBCtrlState);
extern void             usb2dev_set_vbusi(BOOL bPlug);

extern void             usb_setID(BOOL bDevice);
extern ER               usb_writePhyReg(UINT32 uiOffset, UINT32 uiValue);
extern ER               usb_readPhyReg(UINT32 uiOffset, UINT32 *puiValue);
extern ER               usb_writePhy2Reg(UINT32 uiOffset, UINT32 uiValue);
extern ER               usb_readPhy2Reg(UINT32 uiOffset, UINT32 *puiValue);

extern irqreturn_t      usb_isr(int irq, void *devid);

extern void             usb_intisr(void);

extern unsigned short   bUSBflag_Remote_wakeup;

extern ER usb2dev_wait_ep_done_timeout(USB_EP EPn, UINT32 *pDMALen, UINT32 timeout_ms);
#endif    // _USBINT_H

