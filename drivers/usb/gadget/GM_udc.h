/*
 * GM USB220 ("FTUSB220") USB Device Controller driver
 *
 * Copyright (C) 2004-2005 John
 *      by John Chiang
 * Copyright (C) 2004 GM corp.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
//*************************************************************************
//****************************** 1.Name Define ****************************
//*************************************************************************

//////////////////////////////////////////////


//#define MESS_ERROR	(0x01 << 0)
//#define MESS_WARNING	(0x01 << 1)
//#define MESS_INFO		(0x01 << 2)
//#define MESS_DEBUG	(0x01 << 7)
////////////////////////////////////////////

#define TRUE			1
#define FALSE			0

#define MASK_F0			0xF0

// Block Size define
#define BLK512BYTE		1
#define BLK1024BYTE		2

#define BLK64BYTE		1
#define BLK128BYTE		2

// Block toggle number define
#define SINGLE_BLK		1
#define DOUBLE_BLK		2
#define TRIBLE_BLK		3

// Endpoint transfer type
#define TF_TYPE_CONTROL		0
#define TF_TYPE_ISOCHRONOUS	1
#define TF_TYPE_BULK		2
#define TF_TYPE_INTERRUPT	3

// ***********Caution***************
// Endpoint or FIFO direction define
// Please don't change this define, this will cause very serious problems.
// Because someone change this define form my original code(FUSB220 firmware on cpe),
// and didn't check where this register use. And let i waste 2 days to debug. Damn!!
// IN = 0, OUT = 1....please don't change this define anymore. (YPING left his message.)
#define DIRECTION_IN	1
#define DIRECTION_OUT	0

// FIFO number define
#define FIFO0	0x0
#define FIFO1	0x1
#define FIFO2	0x2
#define FIFO3	0x3
#define FIFO4	0x4
#define FIFO5	0x5
#define FIFO6	0x6
#define FIFO7	0x7
#define FIFO8	0x8
#define FIFO9	0x9

// Endpoint number define
#define EP0         0x00
#define EP1         0x01
#define EP2         0x02
#define EP3         0x03
#define EP4         0x04
#define EP5         0x05
#define EP6         0x06
#define EP7         0x07
#define EP8         0x08
#define EP9         0x09
#define EP10        0x10
#define EP11        0x11
#define EP12        0x12
#define EP13        0x13
#define EP14        0x14
#define EP15        0x15

#define BIT0		0x00000001
#define BIT1		0x00000002
#define BIT2		0x00000004
#define BIT3		0x00000008
#define BIT4		0x00000010
#define BIT5		0x00000020
#define BIT6		0x00000040
#define BIT7		0x00000080

#define BIT8		0x00000100
#define BIT9		0x00000200
#define BIT10		0x00000400
#define BIT11		0x00000800
#define BIT12		0x00001000
#define BIT13		0x00002000
#define BIT14		0x00004000
#define BIT15		0x00008000

#define BIT16		0x00010000
#define BIT17		0x00020000
#define BIT18		0x00040000
#define BIT19		0x00080000
#define BIT20		0x00100000
#define BIT21		0x00200000
#define BIT22		0x00400000
#define BIT23		0x00800000

#define BIT24		0x01000000
#define BIT25		0x02000000
#define BIT26		0x04000000
#define BIT27		0x08000000
#define BIT28		0x10000000
#define BIT29		0x20000000
#define BIT30		0x40000000
#define BIT31		0x80000000


#define	DMA_ADDR_INVALID	(~(dma_addr_t)0)
#define  AHB_DMA_MAX_LEN	8192

// Buffer allocation for each EP
#define USB_EPX_BUFSIZ	4096

#define	MAX_FIFO_SIZE	64    // reset value for EP
#define	MAX_EP0_SIZE	0x40  // ep0 fifo size

#define USB_EP0_BUFSIZ	256

// Used for test pattern traffic
#if 0
#define TEST_J		0x02
#define TEST_K		0x04
#define TEST_SE0_NAK	0x08
#define TEST_PKY	0x10
#endif

#define FUSB220_CURRENT_SUPPORT_EP	5	//ep0~ep4

#define FUSB220_MAX_EP				10	// 1..10
#define FUSB220_MAX_FIFO			10	// 0.. 9
//#define USB_AHBDAC 3

/*-------------------------------------------------------------------------*/
//*************************************************************************
//****************************** 2.Structure Define************************
//*************************************************************************

/* DRIVER DATA STRUCTURES and UTILITIES */

struct FTC_ep
{
	struct usb_ep	 ep;
	struct FTC_udc	*dev;
	unsigned long 	 irqs;
	u32              wDMA_Set_Length;

	unsigned num:8,
		 dma:1,
		 is_in:1,
		 stopped:1,
		 dma_running:1;

	/* analogous to a host-side qh */
	struct list_head			queue;
	const struct usb_endpoint_descriptor	*desc;
};

struct FTC_request
{
	struct usb_request	req;
	struct list_head	queue;
	u32 			u32DMACurrTxLen;
	dma_addr_t		CurDmaStartAddr;
	unsigned		mapped:1;
};

enum ep0state
{
	EP0_DISCONNECT,	 /* no host */
	EP0_IDLE,		 /* between STATUS ack and SETUP report */
	EP0_IN, EP0_OUT, /* data stage */
	EP0_STATUS,		 /* status stage */
	EP0_STALL,		 /* data or status stages */
	EP0_SUSPEND,	 /* usb suspend */
};

typedef enum
{
	CMD_VOID,				// No command
	CMD_GET_DESCRIPTOR,		// Get_Descriptor command
	CMD_SET_DESCRIPTOR,		// Set_Descriptor command
	CMD_TEST_MODE           // Test_Mode command
} CommandType;

typedef enum
{
	PIO_Mode,
	AHB_DMA,
	APB_DMA
} DMA_mode;

typedef enum
{
	CLEAR_STALL = 0,
	PROTO_STALL,
	FUNC_STALL,
} Stall_state;


struct FTC_udc
{
	/* each pci device provides one gadget, several endpoints */
	struct usb_gadget		gadget;
	spinlock_t			lock;
	struct FTC_ep			ep[FUSB220_MAX_EP];
	struct usb_gadget_driver	*driver;

	struct usb_request		*EP0req;

	enum ep0state			ep0state;
	unsigned			got_irq:1,
					got_region:1,
					req_config:1,
					configured:1,
					enabled:1;

	struct usb_ctrlrequest		ControlCmd;

	u8 u8UsbConfigValue;
	u8 u8UsbInterfaceValue;
	u8 u8UsbInterfaceAlternateSetting;

	CommandType eUsbCxCommand;
	DMA_mode Dma_Status;

	u8 bUsbBufferEmpty;

	u8 u16TxRxCounter;

	u8 usb_interrupt_level1;
	u8 usb_interrupt_level1_Save;
	u8 usb_interrupt_level1_Mask;

	u8 *pu8DescriptorEX;

	//11/2/05' AHB_DMA
	//ahb_dma_data_t  *ahb_dma;
	u8 EPUseDMA;   //EP for DMA
	struct  FTC_request *ReqForDMA;

	/* statistics... */
	unsigned long irqs;
	u8 u8LineCount;
	int chForDMA;
	u32 va_base;
};

/*-------------------------------------------------------------------------*/
//*************************************************************************
//****************************** 3.Debug Info and hardware feature Define************
//*************************************************************************
#define NoFixPort			0
#define FixPort			1
#define USB_DataPort		NoFixPort

#define DBG_OFF 	0x00
#define DBG_CTRL 	0x01
#define DBG_BULK 	0x02
#define DBG_ISO		0x04
#define DBG_INT		0x08
#define DBG_FUNC	0x10
#define DBG_TMP		0x20

#ifdef CONFIG_GM_FUSB220
#define DBG_BRAD_BULK   0x40
#endif

#define USB_DBG 	(DBG_OFF)
//#define USB_DBG 	(DBG_OFF|DBG_TMP|DBG_FUNC|DBG_CTRL)
//#define USB_DBG 	(DBG_FUNC |DBG_BULK|DBG_CTRL|DBG_BRAD_BULK|DBG_TMP) //( DBG_FUNC |DBG_BULK)//|DBG_TMP|DBG_FUNC)//|DBG_CTRL)

#define xprintk(dev,level,fmt,args...) printk(level "%s : " fmt , __func__ , ## args)
#define wprintk(level,fmt,args...) printk(level "%s : " fmt , __func__ , ## args)

#ifdef DEBUG
#define udc_dbg(dev,fmt,args...) xprintk(dev , KERN_DEBUG , fmt , ## args)
#else
#define udc_dbg(dev,fmt,args...) do { } while (0)
#endif /* DEBUG */

#define VERBOSE

#ifdef VERBOSE
#define Vdbg udc_dbg
#else
#define Vdbg(dev,fmt,args...) do { } while (0)
#endif	/* VERBOSE */

#define error(dev,fmt,args...) xprintk(dev , KERN_ERR , fmt , ## args)
#define uwarn(dev,fmt,args...) xprintk(dev , KERN_WARNING , fmt , ## args)
#define Info(dev,fmt,args...) xprintk(dev , KERN_INFO , fmt , ## args)

#if (USB_DBG & DBG_TMP)
#define DBG_TEMP(fmt,args...) wprintk(KERN_INFO , fmt , ## args)
#else
#define DBG_TEMP(fmt,args...)
#endif

#if (USB_DBG & DBG_FUNC)
#define DBG_FUNCC(fmt,args...) wprintk(KERN_INFO , fmt , ## args)
#else
#define DBG_FUNCC(fmt,args...)
#endif

#if (USB_DBG & DBG_CTRL)
#define DBG_CTRLL(fmt,args...) wprintk(KERN_INFO , fmt , ## args)
#else
#define DBG_CTRLL(fmt,args...)
#endif

#if (USB_DBG & DBG_BULK)
#define DBG_BULKK(fmt,args...) wprintk(KERN_INFO , fmt , ## args)
#else
#define DBG_BULKK(fmt,args...)
#endif


#ifdef CONFIG_GM_FUSB220
#if (USB_DBG & DBG_BRAD_BULK)
#define DBG_BRAD(dev,fmt,args...) xprintk(dev , KERN_DEBUG , fmt , ## args)
#define DBG_BRAD_FUNCC(fmt,args...) wprintk(KERN_INFO , fmt , ## args)
#define DBG_BRAD_CTRLL(fmt,args...) wprintk(KERN_INFO , fmt , ## args)
#define ERROR_BRAD(dev,fmt,args...) xprintk(dev , KERN_ERR , fmt , ## args)
#else
#define DBG_BRAD_BULKK(fmt, args...)
#define DBG_BRAD(dev,fmt,args...)
#define DBG_BRAD_FUNCC(fmt,args...)
#define DBG_BRAD_CTRLL(fmt,args...)
#define ERROR_BRAD(dev,fmt,args...)
#endif
#endif

/*-------------------------------------------------------------------------*/
//*************************************************************************
//****************************** 4.Others Define************************
//*************************************************************************

/* 2.5 stuff that's sometimes missing in 2.4 */

#ifndef container_of
#define	container_of	list_entry
#endif

#ifndef likely
#define likely(x)	(x)
#define unlikely(x)	(x)
#endif

#ifndef BUG_ON
#define BUG_ON(condition) do { if (unlikely((condition)!=0)) BUG(); } while(0)
#endif

#ifndef WARN_ON
#define	WARN_ON(x)	do { } while (0)
#endif

#if 0//ndef	IRQ_NONE
typedef void irqreturn_t;
#define IRQ_NONE
#define IRQ_HANDLED
#define IRQ_RETVAL(x)
#endif

#define Cmd_Service_Fail	 			0
#define Cmd_Already_Service 			1
#define Cmd_Let_Gadget_To_Service 	2

//*************************************************************************
//****************************** 5.Function Export Define************************
//*************************************************************************
//static void vUsbFIFO_EPxCfg_FS(void);
//static void vUsbFIFO_EPxCfg_HS(void);

#ifdef CONFIG_GM_FUSB220
static int FTC_fifo_status(struct usb_ep *_ep);
#endif

//static u8 bClear_feature(struct FTC_udc *dev);
//static u8 bGet_status(struct FTC_udc *dev);
//static u8 bSet_address(struct FTC_udc *dev);
//static u8 bSet_descriptor(struct FTC_udc *dev);
//static u8 bSet_feature(struct FTC_udc *dev);
//static u8 bGet_descriptor(struct FTC_udc *dev);
//static void vGet_configuration(struct FTC_udc *dev);
//static u8 bSet_configuration(struct FTC_udc *dev);
//static u8 bGet_interface(struct FTC_udc *dev);
//static u8 bSet_interface(struct FTC_udc *dev);
//static u8 bSynch_frame(struct FTC_udc *dev);
//static u8 bStandardCommand(struct FTC_udc *dev);

//*************************************************************************
//****************************** 6.ED/FIFO Config Define************************
//*************************************************************************

//**************************************
//*** Full Speed HW ED/FIFO Configuration Area ***
#define FULL_ED1_bBLKSIZE	 BLK64BYTE
#define FULL_ED1_bBLKNO      SINGLE_BLK
#define FULL_ED1_bDIRECTION  DIRECTION_IN
#define FULL_ED1_bTYPE       TF_TYPE_BULK
#define FULL_ED1_MAXPACKET   64

#define FULL_ED2_bBLKSIZE	BLK64BYTE
#define FULL_ED2_bBLKNO      SINGLE_BLK
#define FULL_ED2_bDIRECTION  DIRECTION_OUT
#define FULL_ED2_bTYPE       TF_TYPE_BULK
#define FULL_ED2_MAXPACKET   64

#define FULL_ED3_bBLKSIZE	BLK64BYTE
#define FULL_ED3_bBLKNO      SINGLE_BLK
#define FULL_ED3_bDIRECTION  DIRECTION_IN
#define FULL_ED3_bTYPE       TF_TYPE_INTERRUPT
#define FULL_ED3_MAXPACKET   64

#define FULL_ED4_bBLKSIZE	BLK64BYTE
#define FULL_ED4_bBLKNO      SINGLE_BLK
#define FULL_ED4_bDIRECTION  DIRECTION_OUT
#define FULL_ED4_bTYPE       TF_TYPE_INTERRUPT
#define FULL_ED4_MAXPACKET   64
//**************************************************

//**************************************
//*** High Speed HW ED/FIFO Configuration Area ***
#define HIGH_ED1_bBLKSIZE	BLK512BYTE
#define HIGH_ED1_bDIRECTION  DIRECTION_IN
#define HIGH_ED1_bTYPE       TF_TYPE_BULK
#define HIGH_ED1_MAXPACKET   512

#define HIGH_ED2_bBLKSIZE	BLK512BYTE
#define HIGH_ED2_bDIRECTION  DIRECTION_OUT
#define HIGH_ED2_bTYPE       TF_TYPE_BULK
#define HIGH_ED2_MAXPACKET   512

#define HIGH_ED3_bBLKSIZE	BLK64BYTE
#define HIGH_ED3_bBLKNO      SINGLE_BLK
#define HIGH_ED3_bDIRECTION  DIRECTION_IN
#define HIGH_ED3_bTYPE       TF_TYPE_INTERRUPT
#define HIGH_ED3_MAXPACKET   64

#define HIGH_ED4_bBLKSIZE	BLK64BYTE
#define HIGH_ED4_bBLKNO      SINGLE_BLK
#define HIGH_ED4_bDIRECTION  DIRECTION_OUT
#define HIGH_ED4_bTYPE       TF_TYPE_INTERRUPT
#define HIGH_ED4_MAXPACKET   64
//**************************************************

#ifdef CONFIG_GM_FUSB220
#define FIFOEnBit 0x80
#define HIGH_ED1_bBLKNO      DOUBLE_BLK
#define HIGH_ED2_bBLKNO      DOUBLE_BLK
#else
#define FIFOEnBit 0x20
#define HIGH_ED1_bBLKNO      SINGLE_BLK
#define HIGH_ED2_bBLKNO      SINGLE_BLK
#endif

#define FULL_ED1_FIFO_START  	FIFO0
#define FULL_ED2_FIFO_START  	(FULL_ED1_FIFO_START+(FULL_ED1_bBLKNO *FULL_ED1_bBLKSIZE))
#define FULL_ED3_FIFO_START  	(FULL_ED2_FIFO_START+(FULL_ED2_bBLKNO *FULL_ED2_bBLKSIZE))
#define FULL_ED4_FIFO_START  	(FULL_ED3_FIFO_START+(FULL_ED3_bBLKNO *FULL_ED3_bBLKSIZE))

#define FULL_EP1_Map			(FULL_ED1_FIFO_START |(FULL_ED1_FIFO_START << 4)|(0xF0 >> (4*(1-FULL_ED1_bDIRECTION))))
#define FULL_EP1_FIFO_Map    	((FULL_ED1_bDIRECTION << 4) | EP1)
#define FULL_EP1_FIFO_Config 	(FIFOEnBit | ((FULL_ED1_bBLKSIZE - 1) << 4) | ((FULL_ED1_bBLKNO - 1) << 2) | FULL_ED1_bTYPE)
#define FULL_EP2_Map        		(FULL_ED2_FIFO_START |(FULL_ED2_FIFO_START << 4)|(0xF0 >> (4*(1-FULL_ED2_bDIRECTION))))
#define FULL_EP2_FIFO_Map    	((FULL_ED2_bDIRECTION << 4) | EP2)
#define FULL_EP2_FIFO_Config 	(FIFOEnBit | ((FULL_ED2_bBLKSIZE - 1) << 4) | ((FULL_ED2_bBLKNO - 1) << 2) | FULL_ED2_bTYPE)
#define FULL_EP3_Map        		(FULL_ED3_FIFO_START |(FULL_ED3_FIFO_START << 4)|(0xF0 >> (4*(1-FULL_ED3_bDIRECTION))))
#define FULL_EP3_FIFO_Map    	((FULL_ED3_bDIRECTION << 4) | EP3)
#define FULL_EP3_FIFO_Config 	(FIFOEnBit | ((FULL_ED3_bBLKSIZE - 1) << 4) | ((FULL_ED3_bBLKNO - 1) << 2) | FULL_ED3_bTYPE)
#define FULL_EP4_Map        		(FULL_ED4_FIFO_START |(FULL_ED4_FIFO_START << 4)|(0xF0 >> (4*(1-FULL_ED4_bDIRECTION))))
#define FULL_EP4_FIFO_Map    	((FULL_ED4_bDIRECTION << 4) | EP4)
#define FULL_EP4_FIFO_Config 	(FIFOEnBit | ((FULL_ED4_bBLKSIZE - 1) << 4) | ((FULL_ED4_bBLKNO - 1) << 2) | FULL_ED4_bTYPE)


#define HIGH_ED1_FIFO_START  	FIFO0
#define HIGH_ED2_FIFO_START  	(HIGH_ED1_FIFO_START+(HIGH_ED1_bBLKNO *HIGH_ED1_bBLKSIZE))
#define HIGH_ED3_FIFO_START  	(HIGH_ED2_FIFO_START+(HIGH_ED2_bBLKNO *HIGH_ED2_bBLKSIZE))
#define HIGH_ED4_FIFO_START  	(HIGH_ED3_FIFO_START+(HIGH_ED3_bBLKNO *HIGH_ED3_bBLKSIZE))

#define HIGH_EP1_Map        		(HIGH_ED1_FIFO_START |(HIGH_ED1_FIFO_START << 4)|(0xF0 >> (4*(1-HIGH_ED1_bDIRECTION))))
#define HIGH_EP1_FIFO_Map    	((HIGH_ED1_bDIRECTION << 4) | EP1)
#define HIGH_EP1_FIFO_Config 	(FIFOEnBit | ((HIGH_ED1_bBLKSIZE - 1) << 4) | ((HIGH_ED1_bBLKNO - 1) << 2) | HIGH_ED1_bTYPE)
#define HIGH_EP2_Map        		(HIGH_ED2_FIFO_START |(HIGH_ED2_FIFO_START << 4)|(0xF0 >> (4*(1-HIGH_ED2_bDIRECTION))))
#define HIGH_EP2_FIFO_Map    	((HIGH_ED2_bDIRECTION << 4) | EP2)
#define HIGH_EP2_FIFO_Config 	(FIFOEnBit | ((HIGH_ED2_bBLKSIZE - 1) << 4) | ((HIGH_ED2_bBLKNO - 1) << 2) | HIGH_ED2_bTYPE)
#define HIGH_EP3_Map        		(HIGH_ED3_FIFO_START |(HIGH_ED3_FIFO_START << 4)|(0xF0 >> (4*(1-HIGH_ED3_bDIRECTION))))
#define HIGH_EP3_FIFO_Map    	((HIGH_ED3_bDIRECTION << 4) | EP3)
#define HIGH_EP3_FIFO_Config 	(FIFOEnBit | ((HIGH_ED3_bBLKSIZE - 1) << 4) | ((HIGH_ED3_bBLKNO - 1) << 2) | HIGH_ED3_bTYPE)
#define HIGH_EP4_Map        		(HIGH_ED4_FIFO_START |(HIGH_ED4_FIFO_START << 4)|(0xF0 >> (4*(1-HIGH_ED4_bDIRECTION))))
#define HIGH_EP4_FIFO_Map    	((HIGH_ED4_bDIRECTION << 4) | EP4)
#define HIGH_EP4_FIFO_Config 	(FIFOEnBit | ((HIGH_ED4_bBLKSIZE - 1) << 4) | ((HIGH_ED4_bBLKNO - 1) << 2) | HIGH_ED4_bTYPE)


//*************************************************************************
//****************************** 7.HW Macro Define************************
//*************************************************************************

#ifdef CONFIG_GM_FUSB220
#include "fusb220-macro.h"
#else
#include "fotg2xx-peri-macro.h"
#endif
