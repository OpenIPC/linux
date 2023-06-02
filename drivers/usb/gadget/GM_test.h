#ifndef GM_TEST_H_CHK
#define GM_TEST_H_CHK

#define TST_RXT_ID   0x4e
#define TST_BULK_IN  0x1
#define TST_BULK_OUT 0x2
#define TST_INT_IN   0x3
#define TST_INT_OUT  0x4


#ifdef USB_GLOBALS
	#define  USB_EXT
#else
	#define  USB_EXT extern
#endif

//================ 2.Define for Bulk =====================================================================================
	#define MAX_BUFFER_SIZE	0x1000
	#define DBUS_RD				1
	#define DBUS_WR				2
	#define APB_RD				3
	#define APB_WR				4
	#define DBUS2APB			5
	#define APB2DBUS			6

	#define CBW_SIGNATE				0x43425355
	#define CSW_SIGNATE				0x53425355
	#define CSW_STATUS_CMD_PASS			0x00
	#define CSW_STATUS_CMD_FAIL			0x01
	#define CSW_STATUS_PHASE_ERROR		0x02

	typedef enum 
	{
		IDLE,
		STATE_CBW,
		STATE_CB_DATA_IN,
		STATE_CB_DATA_OUT,
		STATE_CSW
	} MassStorageState;

	typedef struct CommandBlockWrapper
	{
		volatile u32 u32Signature;
		volatile u32 u32Tag;
		volatile u32 u32DataTransferLength;
		volatile u8 u8Flags;
		volatile u8 u8LUN;
		volatile u8 u8CBLength;
		volatile u8 u8CB[16];
	} CBW;

	typedef struct CommandStatusWrapper
	{
		volatile u32 u32Signature;
		volatile u32 u32Tag;
		volatile u32 u32DataResidue;
		volatile u8 u8Status;
	} CSW;
#if 0
	extern CBW tCBW;
	extern CSW tCSW;
	extern MassStorageState eUsbMassStorageState;
	
	extern u8   u8VirtMemory[MAX_BUFFER_SIZE];
	extern u32*   u32VirtMemory;
	extern u16   u16VirtMemoryIndex;
#endif	
	extern u32	 u32FIFOUseDMA;
	extern u32	 u32UseDMAChannel;
	extern u8   bDMARunning;
//	extern BOOLEAN bDxDoneAlready;
	
	extern void vUsb_F2_Out(u16 u16FIFOByteCount);
	extern void vUsb_F0_In(void);
	// void vUsb_APInit(struct zero_dev *dev);
	extern void vCheckDMA(u32 Channel, u32 FIFONum);

//================ 3.Define for Interrupt =====================================================================================
	#define IntTransSizeFix 		0
#if 0	
	extern u8 *u8InterruptArray;
	extern u8 u8InterruptCount;
	extern u8 u8Interrupt_TX_COUNT ;
	
	extern u8 u8InterruptOutCount ;
	extern u8 u8Interrupt_RX_COUNT ;
#endif
	extern void vUsb_Interrupt_In(void);
	extern void vUsb_Interrupt_Out(void);

//================   Define for Isochronous =========================
	#define ISO_Wrap 	254
	
	extern u32 *u32ISOArray;
	extern u32 u32ISOInTransferCount;
	extern u32 u32ISOOutTransferCount;
	extern u32 u32ISOINTest[4096];
	//extern fLib_DMA_CH_t DMAChannel_Iso;
	extern u32 u32DMAStatus;
	extern u32 *u32ISOOutArray;
	extern u32 u8ISOOutCount ;

	extern void vUsb_ISO_Initial(void);
	extern void vUsb_ISO_In(void);
	extern void vUsb_ISO_Out(void);
	extern void vAHB_DMAInitial(u32 Channel, u32 SrcWidth,
				u32 DstWidth, u32 SrcSize, u32 SrcCtrl, u32 DstCtrl, 
				u32 Priority, u32 Mode);

	extern void vAHB_DMA_WaitIntStatus(u32 Channel);
	

#endif /* GM_TEST_H_CHK */

