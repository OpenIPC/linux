
//#include <string.h>

#include "usb2dev.h"
//#include "PrjCfg.h"
//#include "Utility.h"
//#include "pll.h"
//#include "pll_protected.h"
//#include "dma_protected.h"
//#include "usb2dev_int.h"

#include "kwrap/error_no.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "kwrap/spinlock.h"
#include "kwrap/task.h"
#include "kwrap/platform.h"
#include <linux/slab.h>


#include "emu_usbtest.h"
//#include "emulation.h"
#include "linux/delay.h"

#define EMU_USB 		ENABLE
#define debug_err(x) 	printk x
#define Delay_DelayMs  msleep
#define debug_msg		printk

UINT32 FLG_ID_USBTEST = 0;
THREAD_HANDLE USBOTEST_TSK_ID1 = 0;
THREAD_HANDLE USBOTEST_TSK_ID2 = 0;
THREAD_HANDLE USBOTEST_TSK_ID3 = 0;
THREAD_HANDLE USBOTEST_TSK_ID4 = 0;
THREAD_HANDLE USBOTEST_TSK_ID5 = 0;

THREAD_HANDLE USBOTEST_TSK_IDV1 = 0;
THREAD_HANDLE USBOTEST_TSK_IDV2 = 0;


#if ( EMU_USB == ENABLE )
static EMUUSBTEST_MODE  emu_usb_test_mode;

static UINT8   *puiU2BulkBuf;
static UINT8   *puiU2ControlBuf;
//static _ALIGNED(64) UINT8   puiU2BulkBuf[262144 + 4096];
//static _ALIGNED(64) UINT8   puiU2ControlBuf[16384];
static _ALIGNED(64) UINT8   puiU2IntrBuf[16384   + 4096];
static _ALIGNED(64) UINT8   puiU2IsocInBuf[EMUUSBTEST_ISOC_SIZE + 4096];
static _ALIGNED(64) UINT8   puiU2IsocOutBuf[16384 + 4096];
//static _ALIGNED(64) UINT8   puiU2BulkCbwBuf[64];
static UINT8   *puiU2BulkCbwBuf;
//static _ALIGNED(64) UINT8   puiU2BulkCswBuf[64];
static UINT8   *puiU2BulkCswBuf;

_ALIGNED(64) UINT8   gU2StrDesc0[] = {
	4,                                  /* size of String Descriptor        */
	USB_STRING_DESCRIPTOR_TYPE,         /* String Descriptor type           */
	0x09, 0x04                          /*  Primary/Sub LANGID              */
};

_ALIGNED(64) UINT8   gU2StrDesc1[] = {
	0x1A,                               /* size of String Descriptor        */
	USB_STRING_DESCRIPTOR_TYPE,         /* String Descriptor type           */
	0X46, 0,  //F
	0X61, 0,  //a
	0X72, 0,  //r
	0X61, 0,  //a
	0X64, 0,  //d
	0X61, 0,  //a
	0X79, 0,  //y
	0X20, 0,  //
	0X69, 0,  //i
	0X6E, 0,  //n
	0X63, 0,  //c
	0X2E, 0   //.
};

_ALIGNED(64) UINT8   gU2StrDesc2[] = {
	0x22,                               /* size of String Descriptor        */
	USB_STRING_DESCRIPTOR_TYPE,         /* String Descriptor type           */
	0X46, 0,  //F
	0X4F, 0,  //O
	0X54, 0,  //T
	0X47, 0,  //G
	0X32, 0,  //2
	0X30, 0,  //0
	0X30, 0,  //0
	0X20, 0,  //
	0X45, 0,  //E
	0X56, 0,  //V
	0X2D, 0,  //-
	0X62, 0,  //b
	0X6F, 0,  //o
	0X61, 0,  //a
	0X72, 0,  //r
	0X64, 0   //d
};

_ALIGNED(64) UINT8   gU2StrDesc3[] = {
	0x22,                               /* size of String Descriptor        */
	USB_STRING_DESCRIPTOR_TYPE,         /* String Descriptor type           */
	0X45, 0,  //E
	0X56, 0,  //V
	0X2D, 0,  //-
	0X62, 0,  //b
	0X6F, 0,  //o
	0X61, 0,  //a
	0X72, 0,  //r
	0X64, 0,  //d
	0X20, 0,  //
	0X43, 0,  //C
	0X6F, 0,  //o
	0X6E, 0,  //n
	0X66, 0,  //f
	0X69, 0,  //i
	0X67, 0,  //g
	0X75, 0,  //u
	0X72, 0,  //r
	0X61, 0,  //a
	0X74, 0,  //t
	0X69, 0,  //i
	0X6F, 0,  //o
	0X6E, 0   //n
};

_ALIGNED(64) UINT8   gU2StrDesc4[] = {
	0x22,                               /* size of String Descriptor        */
	USB_STRING_DESCRIPTOR_TYPE,         /* String Descriptor type           */
	0X45, 0,  //E
	0X56, 0,  //V
	0X2D, 0,  //-
	0X62, 0,  //b
	0X6F, 0,  //o
	0X61, 0,  //a
	0X72, 0,  //r
	0X64, 0,  //d
	0X20, 0,  //
	0X49, 0,  //I
	0X6E, 0,  //n
	0X74, 0,  //t
	0X65, 0,  //e
	0X72, 0,  //r
	0X66, 0,  //f
	0X61, 0,  //a
	0X63, 0,  //c
	0X65, 0   //e
};

_ALIGNED(64) USB_DEVICE_DESC gU2HsTestDevDesc = {
	//---- device descriptor ----
	18,                             /* descriptor size                  */
	USB_DEVICE_DESCRIPTOR_TYPE,     /* descriptor type                  */
	0x0200,                         /* spec. release number             */
	0,                              /* class code                       */
	0,                              /* sub class code                   */
	0,                              /* protocol code                    */
	64,                             /* max packet size for endpoint 0   */
	0x2310,                         /* vendor id                        */
	0x5678,                         /* product id                       */
	0x0002,                         /* device release number            */
	1,                              /* manifacturer string id (0x10 in dscroot)          */
	2,                              /* product string id  (0x20 in dscroot)              */
	0,                              /* serial number string id  (0x00 in dscroot)        */
	1                               /* number of possible configuration */
};

_ALIGNED(64) UINT8 gU2HsTestCfgDesc[60] = {
	//---- configuration(cfg-A) -------
	9,                              /* descriptor size                  */
	USB_CONFIGURATION_DESCRIPTOR_TYPE,/* descriptor type                  */
	60,                          /* total length                     */
	0,                              /* total length                     */
	1,                              /* number of interface              */
	1,                              /* configuration value              */
	3,                              /* configuration string id          */
	0xE0,                           /* characteristics                  */
	0x00,                           /* maximum power in 2mA             */

	//----- cfg A interface0 ----
	//USB_INTERFACE_DESCRIPTOR, 9 bytes for Mass Storage
	// 0x09, 0x04, 0x00, 0x00, 0x02, 0x08, 0x06, 0x50, 0x00,
	//---- alt0 --------
	9,                              /* descriptor size      */
	USB_INTERFACE_DESCRIPTOR_TYPE,  /* descriptor type      */
	0,                              /* interface number     */
	0,                              /* alternate setting    */
	6,                              /* number of endpoint   */
	0,                              /* interface class      */
	0,                              /* interface sub-class  */
	0,                              /* interface protocol   */
	4,                              /* interface string id  */

	//USB_ENDPOINT_DESCRIPTOR
	//---- EP1 IN ----
	7,                              /* descriptor size      */
	USB_ENDPOINT_DESCRIPTOR_TYPE,   /* descriptor type      */
	USB_EP_EP1I_ADDRESS,            /* endpoint address     */
	USB_EP_ATR_BULK,                /* character address    */
	0x00,                           /* max packet size      */
	0x02,                           /* max packet size      */
	0,

	//---- EP2 OUT ----
	7,                              /* descriptor size      */
	USB_ENDPOINT_DESCRIPTOR_TYPE,   /* descriptor type      */
	USB_EP_EP2O_ADDRESS,            /* endpoint address     */
	USB_EP_ATR_BULK,                /* character address    */
	0x00,                           /* max packet size      */
	0x02,                           /* max packet size      */
	0,

	//---- EP3 IN ----
	7,                              /* descriptor size      */
	USB_ENDPOINT_DESCRIPTOR_TYPE,   /* descriptor type      */
	USB_EP_EP3I_ADDRESS,            /* endpoint address     */
	USB_EP_ATR_INTERRUPT,           /* character address    */
	0x00,                           /* max packet size      */
	0x04,                           /* max packet size      */
	1,

	//---- EP4 OUT ----
	7,                              /* descriptor size      */
	USB_ENDPOINT_DESCRIPTOR_TYPE,   /* descriptor type      */
	USB_EP_EP4O_ADDRESS,            /* endpoint address     */
	USB_EP_ATR_INTERRUPT,           /* character address    */
	0x00,                           /* max packet size      */
	0x04,                           /* max packet size      */
	1,

	//---- EP5 IN ----
	7,                              /* descriptor size      */
	USB_ENDPOINT_DESCRIPTOR_TYPE,   /* descriptor type      */
	USB_EP_EP5I_ADDRESS,            /* endpoint address     */
	USB_EP_ATR_ISO,                 /* character address    */
	0x00,                           /* max packet size      */
	0x04,                           /* max packet size      */ // HBW1=0x04  HBW2= 0x0C. HBW3=0x14.
	1,

	//---- EP6 OUT ----
	7,                              /* descriptor size      */
	USB_ENDPOINT_DESCRIPTOR_TYPE,   /* descriptor type      */
	USB_EP_EP6O_ADDRESS,            /* endpoint address     */
	USB_EP_ATR_ISO,                 /* character address    */
	0x00,                           /* max packet size      */
	0x04,                           /* max packet size      */ // HBW1=0x04  HBW2= 0x0C. HBW3=0x14.
	1,
};


_ALIGNED(64) UINT8 gU2FsTestCfgDesc[46] = {
	//---- configuration(cfg-A) -------
	9,                              /* descriptor size                  */
	USB_CONFIGURATION_DESCRIPTOR_TYPE,/* descriptor type                  */
	46,                             /* total length                     */
	0,                              /* total length                     */
	1,                              /* number of interface              */
	1,                              /* configuration value              */
	3,                              /* configuration string id          */
	0xE0,                           /* characteristics                  */
	0x00,                           /* maximum power in 2mA             */

	//----- cfg A interface0 ----
	//USB_INTERFACE_DESCRIPTOR, 9 bytes for Mass Storage
	// 0x09, 0x04, 0x00, 0x00, 0x02, 0x08, 0x06, 0x50, 0x00,
	//---- alt0 --------
	9,                              /* descriptor size      */
	USB_INTERFACE_DESCRIPTOR_TYPE,  /* descriptor type      */
	0,                              /* interface number     */
	0,                              /* alternate setting    */
	4,                              /* number of endpoint   */
	0,                              /* interface class      */
	0,                              /* interface sub-class  */
	0,                              /* interface protocol   */
	4,                              /* interface string id  */

	//USB_ENDPOINT_DESCRIPTOR, ep1
	//---- EP1 IN ----
	7,                              /* descriptor size      */
	USB_ENDPOINT_DESCRIPTOR_TYPE,   /* descriptor type      */
	USB_EP_EP1I_ADDRESS,            /* endpoint address     */
	USB_EP_ATR_BULK,                /* character address    */
	0x40,                           /* max packet size      */
	0x00,                           /* max packet size      */
	0,

	//USB_ENDPOINT_DESCRIPTOR, ep2
	//---- EP2 OUT ----
	7,                              /* descriptor size      */
	USB_ENDPOINT_DESCRIPTOR_TYPE,   /* descriptor type      */
	USB_EP_EP2O_ADDRESS,            /* endpoint address     */
	USB_EP_ATR_BULK,                /* character address    */
	0x40,                           /* max packet size      */
	0x00,                           /* max packet size      */
	0,

#if 1
	//---- EP3 IN ----
	7,                              /* descriptor size      */
	USB_ENDPOINT_DESCRIPTOR_TYPE,                 /* descriptor type      */
	USB_EP_EP3I_ADDRESS,            /* endpoint address     */
	USB_EP_ATR_INTERRUPT,           /* character address    */
	0x40,                           /* max packet size      */
	0x00,                           /* max packet size      */
	1,

	//---- EP4 OUT ----
	7,                              /* descriptor size      */
	USB_ENDPOINT_DESCRIPTOR_TYPE,                 /* descriptor type      */
	USB_EP_EP4O_ADDRESS,            /* endpoint address     */
	USB_EP_ATR_INTERRUPT,           /* character address    */
	0x40,                           /* max packet size      */
	0x00,                           /* max packet size      */
	1,
#else
	//---- EP4 OUT ----
	7,                              /* descriptor size      */
	USB_ENDPOINT_DESCRIPTOR_TYPE,   /* descriptor type      */
	USB_EP_EP4O_ADDRESS,            /* endpoint address     */
	USB_EP_ATR_INTERRUPT,           /* character address    */
	0x40,                           /* max packet size      */
	0x00,                           /* max packet size      */
	1,

#if 1
	//---- EP5 IN ----
	7,                              /* descriptor size      */
	USB_ENDPOINT_DESCRIPTOR_TYPE,   /* descriptor type      */
	USB_EP_EP5I_ADDRESS,            /* endpoint address     */
	USB_EP_ATR_ISO,                 /* character address    */
	0xF8,                           /* max packet size      */
	0x03,                           /* max packet size      */
	1,
#else
	//---- EP6 OUT ----
	7,                              /* descriptor size      */
	UDESC_ENDPOINT,                 /* descriptor type      */
	USB_EP_EP6O_ADDRESS,            /* endpoint address     */
	USB_EP_ATR_ISO,                 /* character address    */
	0xF8,                           /* max packet size      */
	0x03,                           /* max packet size      */
	1,
#endif
#endif
};

static _ALIGNED(4) const USB_DEVICE_DESC gU2devQualiDesc = {
	USB_DEV_QUALI_LENGTH,
	USB_DEVICE_QUALIFIER_DESCRIPTOR_TYPE,
	0x0200,                                     /* spec. release number             */
	0,                              /* class code                       */
	0,                           /* sub class code                   */
	0,                           /* protocol code                    */
	0x40,                   /* max packet size for endpoint 0   */
	1,                         /* number of possible configuration */
	0                                           /*reserved                          */
};

static UINT32   gU2TestCxTotal, gU2TestCxCurrent;

typedef struct {
	UINT32   dCBW_Signature;
	UINT32   dCBW_Tag;
	UINT32   dCBW_DataXferLen;
	UINT8    bCBW_Flag;
	UINT8    bCBW_LUN ;
	UINT8    bCBW_CDBLen ;
	UINT8    CBW_CDB[16];
} U2TESTCBW, *PU2TESTCBW;

typedef struct {
	UINT32   dCSW_Signature;
	UINT32   dCSW_Tag;
	UINT32   dCSW_DataResidue;
	UINT8    bCSW_Status;
} U2TESTCSW, *PU2TESTCSW;

static void emu_usb_test_control_vendor_request(UINT32 uiRequest)
{
	if (uiRequest != 0) {
		usb2dev_set_ep_stall(USB_EP0);
		return;
	}
	if (usb2dev_control_data.device_request.bm_request_type & 0x80) {
		/* CX IN */
		usb2dev_control_data.w_length  = usb2dev_control_data.device_request.w_length;
		usb2dev_control_data.p_data    = puiU2ControlBuf;
		usb2dev_reture_setup_data();

	} else {
		/* CX OUT */
		gU2TestCxTotal   = usb2dev_control_data.device_request.w_length;
		gU2TestCxCurrent = 0;
	}
}

static void emu_usb_test_dispatch_event(UINT32 uiEvent)
{

	switch (uiEvent) {
	case USB_EVENT_OUT0: {
			usb2dev_mask_ep_interrupt(USB_EP0);
			set_flg(FLG_ID_USBTEST, EMUUSBTEST_FLAG_EP0_OUT);
		}
		break;
	case USB_EVENT_EP2_RX: {
			usb2dev_mask_ep_interrupt(USB_BULKOUT_EP);
			set_flg(FLG_ID_USBTEST, EMUUSBTEST_FLAG_BULK);
		}
		break;
	case USB_EVENT_EP4_RX: {
			usb2dev_mask_ep_interrupt(USB_INTROUT_EP);
			set_flg(FLG_ID_USBTEST, EMUUSBTEST_FLAG_INTR);
		}
		break;
	case USB_EVENT_EP6_RX: {
			usb2dev_mask_ep_interrupt(USB_ISOCOUT_EP);
			set_flg(FLG_ID_USBTEST, EMUUSBTEST_FLAG_ISOC);
		}
		break;

	case USB_EVENT_EP1_TX: {
			usb2dev_mask_ep_interrupt(USB_BULKIN_EP);
			set_flg(FLG_ID_USBTEST, EMUUSBTEST_FLAG_BULK_IN);
		}
		break;
	case USB_EVENT_EP3_TX: {
			usb2dev_mask_ep_interrupt(USB_INTRIN_EP);
			set_flg(FLG_ID_USBTEST, EMUUSBTEST_FLAG_INTR_IN);
		}
		break;
	case USB_EVENT_EP5_TX: {
			usb2dev_mask_ep_interrupt(USB_ISOCIN_EP);
			set_flg(FLG_ID_USBTEST, EMUUSBTEST_FLAG_ISOC_IN);
		}
		break;


	case USB_EVENT_EP7_TX: {
			usb2dev_mask_ep_interrupt(USB_EP7);
			set_flg(FLG_ID_USBTEST, EMUUSBTEST_FLAG_UVC1);
		}
		break;
	case USB_EVENT_EP8_TX: {
			usb2dev_mask_ep_interrupt(USB_EP8);
			set_flg(FLG_ID_USBTEST, EMUUSBTEST_FLAG_UVC2);
		}
		break;

	case USB_EVENT_CLRFEATURE:
		debug_msg("USB_EVENT_CLRFEATURE\r\n");
		break;
	case USB_EVENT_RESET:
		//debug_msg("USB_EVENT_RESET\r\n");
		set_flg(FLG_ID_USBTEST, EMUUSBTEST_FLAG_RESET);
		break;
	case USB_EVENT_SUSPEND:
		//debug_msg("USB_EVENT_SUSPEND\r\n");
		set_flg(FLG_ID_USBTEST, EMUUSBTEST_FLAG_RESET);
		break;
	case USB_EVENT_RESUME:
		debug_msg("USB_EVENT_RESUME\r\n");
		break;
	default:
		//debug_msg("%s: evt not support: 0x%lx\r\n", __func__, uiEvent);
		break;
	}




}




static void emu_usb_test_open_needed_fifo(void)
{
	if (emu_usb_test_mode == EMUUSBTEST_MODE_UVC_2V_CIO_BIO) {
		usb2dev_unmask_ep_interrupt(USB_EP0);
		usb2dev_unmask_ep_interrupt(USB_EP7);
		usb2dev_unmask_ep_interrupt(USB_EP8);
		usb2dev_unmask_ep_interrupt(USB_BULKOUT_EP);
	} else {

		usb2dev_unmask_ep_interrupt(USB_EP0);
		usb2dev_unmask_ep_interrupt(USB_BULKOUT_EP);
		usb2dev_unmask_ep_interrupt(USB_INTROUT_EP);

		if ((emu_usb_test_mode == EMUUSBTEST_MODE_CIO_BIO_RIO_IIO) || (emu_usb_test_mode == EMUUSBTEST_MODE_CIO_BIO_RIO_II)) {
			usb2dev_unmask_ep_interrupt(USB_ISOCIN_EP);
		}

		if (emu_usb_test_mode == EMUUSBTEST_MODE_CIO_BIO_RIO_IIO) {
			usb2dev_unmask_ep_interrupt(USB_ISOCOUT_EP);
		}
	}
}


#if 1

void emu_usb_test_bulk_thread(void)
{
	FLGPTN          uiFlag;
	static UINT32   BulkTestState = 0;
	PU2TESTCBW      pU2Cbw;
	PU2TESTCSW      pU2Csw;
	UINT32          DMALen, AddrOfs = 0;
	ER				Ret;
	UINT8 			*pBulk_rw_buf;

	//kent_tsk();

	BulkTestState = 0;

	pBulk_rw_buf = (UINT8 *)puiU2BulkBuf;

	#if EMUUSBTEST_DRAM2
	// Test BULK to using DRAM2
	pBulk_rw_buf = (UINT8 *)puiU2BulkBuf+0x40000000;
	#endif

	#if EMUUSBTEST_WP
	{
		UINT32              uiSet = DMA_WPSET_0;//DMA_WPSET_1
		DMA_WRITEPROT_ATTR  ProtectAttr = {0};

		// 1. Setup DMA write protect detect
		ProtectAttr.chEnMask.bUSB   = 1;
		ProtectAttr.chEnMask.bUSB_1 = 1;
		ProtectAttr.uiProtectlel    = DMA_WPLEL_UNWRITE;//DMA_WPLEL_UNWRITE/DMA_RPLEL_UNREAD / DMA_WPLEL_DETECT / DMA_RWPLEL_UNRW
		ProtectAttr.uiStartingAddr  = (UINT32)pBulk_rw_buf;
		ProtectAttr.uiSize          = 262144;

		if (uiSet == DMA_WPSET_0) {
			dma_configWPFunc(uiSet, &ProtectAttr, NULL);
		} else if (uiSet == DMA_WPSET_1) {
			dma_configWPFunc(uiSet, &ProtectAttr, NULL);
		} else {
			dma_configWPFunc(uiSet, &ProtectAttr, NULL);
		}
		dma_enableWPFunc(uiSet);
	}
	#endif

	//coverity[no_escape]
	while (1) {

		set_flg(FLG_ID_USBTEST, EMUUSBTEST_FLAG_IDLE1);
		wai_flg(&uiFlag, FLG_ID_USBTEST, EMUUSBTEST_FLAG_BULK | EMUUSBTEST_FLAG_RESET, TWF_ORW | TWF_CLR);
		clr_flg(FLG_ID_USBTEST, EMUUSBTEST_FLAG_IDLE1);

		if (uiFlag & EMUUSBTEST_FLAG_RESET) {
			BulkTestState       = 0;
			AddrOfs             = 0;
		}

		if (uiFlag & EMUUSBTEST_FLAG_BULK) {
			if (BulkTestState == 0) {

				while (1) {
					DMALen = usb2dev_get_ep_bytecount(USB_BULKOUT_EP);
					if (DMALen == 31) {
						break;
					}
					Delay_DelayMs(1);
				}


				Ret = usb2dev_read_endpoint(USB_BULKOUT_EP, (UINT8 *) puiU2BulkCbwBuf, &DMALen);

				if ((DMALen != 31)||(Ret != E_OK)) {
					debug_err(("E1\r\n"));
				}

				pU2Cbw = (PU2TESTCBW)puiU2BulkCbwBuf;
				pU2Csw = (PU2TESTCSW)puiU2BulkCswBuf;

				/* Check if CBW is legal */
				if (pU2Cbw->dCBW_Signature != 0x43425355) {
					debug_err(("ilegal CBW Sigature\r\n"));
				}

				pU2Csw->dCSW_Signature      = 0x53425355;
				pU2Csw->dCSW_Tag            = pU2Cbw->dCBW_Tag;
				pU2Csw->dCSW_DataResidue    = pU2Cbw->dCBW_DataXferLen;
				pU2Csw->bCSW_Status         = 0;//CSW_GOOD

				usb2dev_unmask_ep_interrupt(USB_BULKOUT_EP);
				wai_flg(&uiFlag, FLG_ID_USBTEST, EMUUSBTEST_FLAG_BULK, TWF_ORW | TWF_CLR);

				#if EMUUSBTEST_WP
				debug_err(("USB-WRITE-DRAM\r\n"));
				#endif

				DMALen = pU2Csw->dCSW_DataResidue;
				Ret = usb2dev_read_endpoint(USB_BULKOUT_EP, (UINT8 *) pBulk_rw_buf + AddrOfs, &DMALen);

				if ((DMALen != pU2Csw->dCSW_DataResidue)||(Ret != E_OK)) {
					debug_err(("E2\r\n"));
				}

				//{
				//	UINT32 kt;
				//	UINT8 *buf1;
				//	buf1 = testbuf + AddrOfs;
				//	for(kt=0;kt<DMALen;kt++) {
				//		if(buf1[kt] != (kt&0xFF)) {
				//			debug_err(("Rx err %d 0x%02X\r\n",kt,buf1[kt]));
				//		}
				//	}
				//}

				usb2dev_unmask_ep_interrupt(USB_BULKIN_EP);
				wai_flg(&uiFlag, FLG_ID_USBTEST, EMUUSBTEST_FLAG_BULK_IN, TWF_ORW | TWF_CLR);

				DMALen = 13;
				Ret = usb2dev_write_endpoint(USB_BULKIN_EP, (UINT8 *) puiU2BulkCswBuf, &DMALen);

				if ((DMALen != 13)||(Ret != E_OK)) {
					debug_err(("E3\r\n"));
				}

				BulkTestState = 1;
			} else if (BulkTestState == 1) {

				while (1) {
					DMALen = usb2dev_get_ep_bytecount(USB_BULKOUT_EP);
					if (DMALen == 31) {
						break;
					}
					Delay_DelayMs(1);
				}

				Ret = usb2dev_read_endpoint(USB_BULKOUT_EP, (UINT8 *) puiU2BulkCbwBuf, &DMALen);

				if ((DMALen != 31)||(Ret != E_OK)) {
					debug_err(("E4\r\n"));
				}

				pU2Cbw = (PU2TESTCBW)puiU2BulkCbwBuf;
				pU2Csw = (PU2TESTCSW)puiU2BulkCswBuf;

				/* Check if CBW is legal */
				if (pU2Cbw->dCBW_Signature != 0x43425355) {
					debug_err(("ilegal CBW Sigature\r\n"));
				}

				pU2Csw->dCSW_Signature      = 0x53425355;
				pU2Csw->dCSW_Tag            = pU2Cbw->dCBW_Tag;
				pU2Csw->dCSW_DataResidue    = pU2Cbw->dCBW_DataXferLen;
				pU2Csw->bCSW_Status         = 0;//CSW_GOOD

				usb2dev_unmask_ep_interrupt(USB_BULKIN_EP);
				wai_flg(&uiFlag, FLG_ID_USBTEST, EMUUSBTEST_FLAG_BULK_IN, TWF_ORW | TWF_CLR);

				#if EMUUSBTEST_WP
				debug_err(("USB-READ-DRAM\r\n"));
				#endif

				DMALen = pU2Csw->dCSW_DataResidue;
				Ret = usb2dev_write_endpoint(USB_BULKIN_EP, (UINT8 *) pBulk_rw_buf + AddrOfs, &DMALen);

				if ((DMALen != pU2Csw->dCSW_DataResidue)||(Ret != E_OK)) {
					debug_err(("E5\r\n"));
				}

				usb2dev_unmask_ep_interrupt(USB_BULKIN_EP);
				wai_flg(&uiFlag, FLG_ID_USBTEST, EMUUSBTEST_FLAG_BULK_IN, TWF_ORW | TWF_CLR);

				DMALen = 13;
				Ret = usb2dev_write_endpoint(USB_BULKIN_EP, (UINT8 *) puiU2BulkCswBuf, &DMALen);

				if ((DMALen != 13)||(Ret != E_OK)) {
					debug_err(("E6\r\n"));
				}

				BulkTestState = 0;

				AddrOfs += 4;
				AddrOfs &= 1023;
			}

			usb2dev_unmask_ep_interrupt(USB_BULKOUT_EP);
		}


	}
}


void emu_usb_test_cx_out_thread(void)
{
	FLGPTN          uiFlag;
	ER				Ret;

	//kent_tsk();

	gU2TestCxTotal      = 0;
	gU2TestCxCurrent    = 0;

	//coverity[no_escape]
	while (1) {

		set_flg(FLG_ID_USBTEST, EMUUSBTEST_FLAG_IDLE2);
		wai_flg(&uiFlag, FLG_ID_USBTEST, EMUUSBTEST_FLAG_EP0_OUT | EMUUSBTEST_FLAG_RESET, TWF_ORW | TWF_CLR);
		clr_flg(FLG_ID_USBTEST, EMUUSBTEST_FLAG_IDLE2);

		if (uiFlag & EMUUSBTEST_FLAG_RESET) {
			gU2TestCxTotal      = 0;
			gU2TestCxCurrent    = 0;
		}

		if (uiFlag & EMUUSBTEST_FLAG_EP0_OUT) {
			UINT32 uiLen;
			//if ((gU2TestCxTotal - gU2TestCxCurrent) > EP0_PACKET_SIZE) {
			//	uiLen = EP0_PACKET_SIZE;
			//} else {
				uiLen = (gU2TestCxTotal - gU2TestCxCurrent);
			//}

			if (uiLen) {
				Ret = usb2dev_read_endpoint(USB_EP0, puiU2ControlBuf + gU2TestCxCurrent, &uiLen);
				if(Ret != E_OK)
					debug_err(("EP0ERR\r\n"));

				gU2TestCxCurrent += uiLen;
			}
			if (!(gU2TestCxTotal - gU2TestCxCurrent)) {
				usb2dev_set_ep0_done();
			}
			usb2dev_unmask_ep_interrupt(USB_EP0);
		}
	}
}


void emu_usb_test_interrupt_thread(void)
{
	FLGPTN          uiFlag;
	UINT32          DMALen, IntrTxCounter = 1, AddrOfs = 0;
	ER				Ret;

	//kent_tsk();

	//coverity[no_escape]
	while (1) {
		set_flg(FLG_ID_USBTEST, EMUUSBTEST_FLAG_IDLE3);
		wai_flg(&uiFlag, FLG_ID_USBTEST, EMUUSBTEST_FLAG_INTR | EMUUSBTEST_FLAG_RESET, TWF_ORW | TWF_CLR);
		clr_flg(FLG_ID_USBTEST, EMUUSBTEST_FLAG_IDLE3);

		if (uiFlag & EMUUSBTEST_FLAG_RESET) {
			IntrTxCounter       = 1;
			AddrOfs             = 0;
		}

		if (uiFlag & EMUUSBTEST_FLAG_INTR) {

			DMALen = IntrTxCounter;
			Ret = usb2dev_read_endpoint(USB_INTROUT_EP, (UINT8 *) puiU2IntrBuf + AddrOfs, &DMALen);

			if ((DMALen != IntrTxCounter)||(Ret != E_OK)) {
				debug_err(("I1\r\n"));
			}

			usb2dev_unmask_ep_interrupt(USB_INTRIN_EP);
			wai_flg(&uiFlag, FLG_ID_USBTEST, EMUUSBTEST_FLAG_INTR_IN, TWF_ORW | TWF_CLR);

			DMALen = IntrTxCounter;
			Ret = usb2dev_write_endpoint(USB_INTRIN_EP, (UINT8 *) puiU2IntrBuf + AddrOfs, &DMALen);

			if ((DMALen != IntrTxCounter)||(Ret != E_OK)) {
				debug_err(("I2\r\n"));
			}

			IntrTxCounter++;

			if (usb2dev_is_highspeed_enabled()) {
				if (IntrTxCounter > 1024 * 16) {
					IntrTxCounter = 1;
				}
			} else {
				if (IntrTxCounter > 64 * 16) {
					IntrTxCounter = 1;
				}
			}

			AddrOfs += 4;
			AddrOfs &= 1023;

			usb2dev_unmask_ep_interrupt(USB_INTROUT_EP);
		}


	}
}



void emu_usb_test_isoc_in_thread(void)
{
	FLGPTN          uiFlag;
	UINT32          DMALen, TxCounter = 1, CurLoc = 0;
	ER				Ret;

	//kent_tsk();

	//coverity[no_escape]
	while (1) {
		set_flg(FLG_ID_USBTEST, EMUUSBTEST_FLAG_IDLE4);
		wai_flg(&uiFlag, FLG_ID_USBTEST, EMUUSBTEST_FLAG_ISOC_IN | EMUUSBTEST_FLAG_RESET, TWF_ORW | TWF_CLR);
		clr_flg(FLG_ID_USBTEST, EMUUSBTEST_FLAG_IDLE4);

		if (uiFlag & EMUUSBTEST_FLAG_RESET) {
			TxCounter       = 1;
			CurLoc          = 0;
		}

		if (uiFlag & EMUUSBTEST_FLAG_ISOC_IN) {
			DMALen = TxCounter;

			if (emu_usb_test_mode == EMUUSBTEST_MODE_CIO_BIO_RIO_IIO) {
				// These delay are used for iso in/out interlace scheduling
				while (usb2dev_check_ep_busy(USB_ISOCOUT_EP)) {
					Delay_DelayMs(3);
				}
			}
			Ret = usb2dev_write_endpoint(USB_ISOCIN_EP, (UINT8 *) puiU2IsocInBuf + CurLoc, &DMALen);

			if (emu_usb_test_mode == EMUUSBTEST_MODE_CIO_BIO_RIO_IIO) {
				// These delay are used for iso in/out interlace scheduling
				Delay_DelayMs(7);
			}

			if ((DMALen != TxCounter)||(Ret != E_OK)) {
				debug_err(("K1\r\n"));
			}

			CurLoc = (CurLoc + TxCounter) % 255;
			while (CurLoc & 0x3) {
				CurLoc += 255;
			}

			TxCounter++;

			if (usb2dev_is_highspeed_enabled()) {
				if (TxCounter > EMUUSBTEST_ISOC_SIZE) {
					TxCounter = 1;
				}
			}

			//debug_msg(",");

			usb2dev_unmask_ep_interrupt(USB_ISOCIN_EP);
		}

	}
}


void emu_usb_test_isoc_out_thread(void)
{
	FLGPTN          uiFlag;
	UINT32          DMALen, CurLoc = 0;
	ER				Ret;

	//kent_tsk();

	//coverity[no_escape]
	while (1) {
		set_flg(FLG_ID_USBTEST, EMUUSBTEST_FLAG_IDLE5);
		wai_flg(&uiFlag, FLG_ID_USBTEST, EMUUSBTEST_FLAG_ISOC | EMUUSBTEST_FLAG_RESET, TWF_ORW | TWF_CLR);
		clr_flg(FLG_ID_USBTEST, EMUUSBTEST_FLAG_IDLE5);

		if (uiFlag & EMUUSBTEST_FLAG_RESET) {
			CurLoc          = 0;
		}

		if (uiFlag & EMUUSBTEST_FLAG_ISOC) {
			DMALen = 16384;

			// These delay are used for iso in/out interlace scheduling
			if (usb2dev_check_ep_busy(USB_ISOCIN_EP)) {
				Delay_DelayMs(10);
			}
			while (usb2dev_check_ep_busy(USB_ISOCIN_EP)) {
				Delay_DelayMs(1);
			}

			usb2dev_clear_ep_fifo(USB_ISOCOUT_EP);
			Ret = usb2dev_read_endpoint(USB_ISOCOUT_EP, (UINT8 *) puiU2IsocOutBuf + CurLoc, &DMALen);

			// These delay are used for iso in/out interlace scheduling
			Delay_DelayMs(7);

			if ((DMALen != 16384)||(Ret != E_OK)) {
				debug_err(("K1\r\n"));
			}


			if (memcmp(puiU2IsocOutBuf + CurLoc, puiU2IsocInBuf + puiU2IsocOutBuf[CurLoc], DMALen)) {
				UINT32 i;
				UINT8 *pbuf1,*pbuf2;

				pbuf1 = puiU2IsocOutBuf + CurLoc;
				pbuf2 = puiU2IsocInBuf + puiU2IsocOutBuf[CurLoc];

				for(i=0;i<16384;i++) {
					if(pbuf1[i] != pbuf2[i])
						break;
				}
				if(i != 1024)
					debug_err(("ISOOUT ERR! %d\r\n",i));
				else {
					debug_msg("o");
					pbuf1 = puiU2IsocOutBuf + CurLoc+1024;
					pbuf2 = puiU2IsocInBuf + puiU2IsocOutBuf[CurLoc+1024];
					for(i=0;i<16384-1024;i++) {
						if(pbuf1[i] != pbuf2[i]){
							debug_err(("ISOOUT ERR! %d\r\n",i+1024));
							break;
						}
					}
				}
			} else {
				puiU2IsocOutBuf[0] = 0;
				puiU2IsocOutBuf[1] = 0xAA;
				puiU2IsocOutBuf[2] = 0x55;
				puiU2IsocOutBuf[3] = 0xFF;
				debug_msg("h");
			}


			CurLoc += 4;
			CurLoc = CurLoc & 1023;

			//debug_msg(".");
			usb2dev_unmask_ep_interrupt(USB_ISOCOUT_EP);
		}

	}
}


void emu_usb_test_isoc_uvc_thread(void)
{
	FLGPTN          uiFlag;
	UINT32          DMALen, TxCounter = 1, CurLoc = 0;
	ER				Ret;

	//kent_tsk();

	//coverity[no_escape]
	while (1) {
		set_flg(FLG_ID_USBTEST, EMUUSBTEST_FLAG_IDLE6);
		wai_flg(&uiFlag, FLG_ID_USBTEST, EMUUSBTEST_FLAG_UVC1 | EMUUSBTEST_FLAG_RESET, TWF_ORW | TWF_CLR);
		clr_flg(FLG_ID_USBTEST, EMUUSBTEST_FLAG_IDLE6);

		if (uiFlag & EMUUSBTEST_FLAG_RESET) {
			TxCounter       = 1;
			CurLoc          = 0;
		}

		if (uiFlag & EMUUSBTEST_FLAG_UVC1) {
			DMALen = TxCounter;

			Delay_DelayMs(10);

			Ret = usb2dev_write_endpoint(USB_EP7, (UINT8 *) puiU2IsocInBuf + CurLoc, &DMALen);

			if ((DMALen != TxCounter)||(Ret != E_OK)) {
				debug_err(("K1\r\n"));
			}

			CurLoc = (CurLoc + TxCounter) % 255;
			while (CurLoc & 0x3) {
				CurLoc += 255;
			}

			TxCounter++;

			if (usb2dev_is_highspeed_enabled()) {
				if (TxCounter > EMUUSBTEST_ISOC_SIZE) {
					TxCounter = 1;
				}
			}

			debug_msg("\r%d  ", TxCounter);

			usb2dev_unmask_ep_interrupt(USB_EP7);
		}

	}
}



void emu_usb_test_isoc_uvc2_thread(void)
{
	FLGPTN          uiFlag;
	UINT32          DMALen, TxCounter = 1, CurLoc = 0;
	ER				Ret;

	//kent_tsk();

	//coverity[no_escape]
	while (1) {
		set_flg(FLG_ID_USBTEST, EMUUSBTEST_FLAG_IDLE7);
		wai_flg(&uiFlag, FLG_ID_USBTEST, EMUUSBTEST_FLAG_UVC2 | EMUUSBTEST_FLAG_RESET, TWF_ORW | TWF_CLR);
		clr_flg(FLG_ID_USBTEST, EMUUSBTEST_FLAG_IDLE7);

		if (uiFlag & EMUUSBTEST_FLAG_RESET) {
			TxCounter       = 1;
			CurLoc          = 0;
		}

		if (uiFlag & EMUUSBTEST_FLAG_UVC2) {
			DMALen = TxCounter;

			Ret = usb2dev_write_endpoint(USB_EP8, (UINT8 *) puiU2IsocInBuf + CurLoc, &DMALen);

			Delay_DelayMs(10);

			if ((DMALen != TxCounter)||(Ret != E_OK)) {
				debug_err(("K1\r\n"));
			}

			CurLoc = (CurLoc + TxCounter) % 255;
			while (CurLoc & 0x3) {
				CurLoc += 255;
			}

			TxCounter++;

			if (usb2dev_is_highspeed_enabled()) {
				if (TxCounter > EMUUSBTEST_ISOC_SIZE) {
					TxCounter = 1;
				}
			}

			debug_msg("  %d  ", TxCounter);

			usb2dev_unmask_ep_interrupt(USB_EP8);
		}

	}
}



#endif

#if 1 // UVC Header test

_ALIGNED(64) UINT8 gU2UvcHsTestCfgDesc[32 + 14] = {
	//---- configuration(cfg-A) -------
	9,                              /* descriptor size                  */
	USB_CONFIGURATION_DESCRIPTOR_TYPE,/* descriptor type                  */
	60,                          /* total length                     */
	0,                              /* total length                     */
	1,                              /* number of interface              */
	1,                              /* configuration value              */
	3,                              /* configuration string id          */
	0xE0,                           /* characteristics                  */
	0x00,                           /* maximum power in 2mA             */

	//----- cfg A interface0 ----
	//USB_INTERFACE_DESCRIPTOR, 9 bytes for Mass Storage
	// 0x09, 0x04, 0x00, 0x00, 0x02, 0x08, 0x06, 0x50, 0x00,
	//---- alt0 --------
	9,                              /* descriptor size      */
	USB_INTERFACE_DESCRIPTOR_TYPE,  /* descriptor type      */
	0,                              /* interface number     */
	0,                              /* alternate setting    */
	4,                              /* number of endpoint   */
	0,                              /* interface class      */
	0,                              /* interface sub-class  */
	0,                              /* interface protocol   */
	4,                              /* interface string id  */

	//USB_ENDPOINT_DESCRIPTOR

	//---- EP1 IN ----
	7,                              /* descriptor size      */
	USB_ENDPOINT_DESCRIPTOR_TYPE,   /* descriptor type      */
	USB_EP_EP1I_ADDRESS,            /* endpoint address     */
	USB_EP_ATR_BULK,                /* character address    */
	0x00,                           /* max packet size      */
	0x02,                           /* max packet size      */
	0,

	//---- EP2 OUT ----
	7,                              /* descriptor size      */
	USB_ENDPOINT_DESCRIPTOR_TYPE,   /* descriptor type      */
	USB_EP_EP2O_ADDRESS,            /* endpoint address     */
	USB_EP_ATR_BULK,                /* character address    */
	0x00,                           /* max packet size      */
	0x02,                           /* max packet size      */
	0,


	//---- EP7 IN ----
	7,                              /* descriptor size      */
	USB_ENDPOINT_DESCRIPTOR_TYPE,   /* descriptor type      */
	USB_EP_EP7I_ADDRESS,            /* endpoint address     */
	USB_EP_ATR_ISO,                 /* character address    */
	0x00,                           /* max packet size      */
	0x04,                           /* max packet size      */ // HBW1=0x04  HBW2= 0x0C. HBW3=0x14.
	1,

	//---- EP8 IN ----
	7,                              /* descriptor size      */
	USB_ENDPOINT_DESCRIPTOR_TYPE,   /* descriptor type      */
	USB_EP_EP8I_ADDRESS,            /* endpoint address     */
	USB_EP_ATR_ISO,                 /* character address    */
	0x00,                           /* max packet size      */
	0x04,                           /* max packet size      */ // HBW1=0x04  HBW2= 0x0C. HBW3=0x14.
	1
};



#endif

void emu_usb_test_open(EMUUSBTEST_MODE test_mode)
{
	UINT32 i;
	USB_MNG	gUSBMng;

	vos_flag_create(&FLG_ID_USBTEST,		NULL,	"FLG_ID_USBTEST");

	puiU2BulkBuf = (UINT8 *)kmalloc(262144 + 4096, GFP_KERNEL);
	puiU2BulkCbwBuf = (UINT8 *)kmalloc(64, GFP_KERNEL);
	puiU2BulkCswBuf = (UINT8 *)kmalloc(64, GFP_KERNEL);
	puiU2ControlBuf = (UINT8 *)kmalloc(16384, GFP_KERNEL);

	usb2dev_init_management(&gUSBMng);

	emu_usb_test_mode = test_mode;
	debug_msg("Open testing mode = %d\r\n", emu_usb_test_mode);

	if ((emu_usb_test_mode == EMUUSBTEST_MODE_CIO_BIO_RIO_IIO) ||
		(emu_usb_test_mode == EMUUSBTEST_MODE_CIO_BIO_RIO_II)) {

		// If SHARE FIFO enable, the usb_int.h must using FIFO4_FIRST mapping.
		usb2dev_set_config(USB_CONFIG_ID_USE_SHARE_FIFO, ENABLE);
	} else {
		usb2dev_set_config(USB_CONFIG_ID_USE_SHARE_FIFO, DISABLE);
	}

	if (emu_usb_test_mode == EMUUSBTEST_MODE_UVC_2V_CIO_BIO) {
		usb2dev_set_callback(USB_CALLBACK_CX_VENDOR_REQUEST, (USB_GENERIC_CB)emu_usb_test_control_vendor_request);

		debug_msg("PLEASE set USB_FIFOMAP_FIFO4FIRST to 0\r\n");
		debug_msg("PLEASE set USB_FIFOMAP_FIFO4FIRST to 0\r\n");


		gUSBMng.p_dev_desc            = (USB_DEVICE_DESC *)&gU2HsTestDevDesc;

		gUSBMng.p_config_desc_hs       = (USB_CONFIG_DESC *)&gU2UvcHsTestCfgDesc;
		gUSBMng.p_config_desc_fs       = (USB_CONFIG_DESC *)&gU2FsTestCfgDesc;
		gUSBMng.p_config_desc_fs_other  = (USB_CONFIG_DESC *)&gU2UvcHsTestCfgDesc;
		gUSBMng.p_config_desc_hs_other  = (USB_CONFIG_DESC *)&gU2FsTestCfgDesc;

		gUSBMng.p_dev_quali_desc       = (USB_DEVICE_DESC *)&gU2devQualiDesc;
		gUSBMng.num_of_configurations  = 1;
		gUSBMng.num_of_strings         = 5;
		gUSBMng.p_string_desc[0]      = (USB_STRING_DESC *)&gU2StrDesc0;
		gUSBMng.p_string_desc[1]      = (USB_STRING_DESC *)&gU2StrDesc1;
		gUSBMng.p_string_desc[2]      = (USB_STRING_DESC *)&gU2StrDesc2;
		gUSBMng.p_string_desc[3]      = (USB_STRING_DESC *)&gU2StrDesc3;
		gUSBMng.p_string_desc[4]      = (USB_STRING_DESC *)&gU2StrDesc4;

		//
		//  Configure USB Endpoint allocation
		//
		for (i = 0; i < 15; i++) {
			gUSBMng.ep_config_hs[i].enable      = FALSE;
			gUSBMng.ep_config_fs[i].enable      = FALSE;
		}

		// Config High Speed endpoint usage.
		gUSBMng.ep_config_hs[USB_EP7 - 1].enable          = TRUE;
		gUSBMng.ep_config_hs[USB_EP7 - 1].blk_size         = 1024;
		gUSBMng.ep_config_hs[USB_EP7 - 1].blk_num          = BLKNUM_SINGLE;
		gUSBMng.ep_config_hs[USB_EP7 - 1].direction       = EP_DIR_IN;
		gUSBMng.ep_config_hs[USB_EP7 - 1].trnsfer_type     = EP_TYPE_ISOCHRONOUS;
		gUSBMng.ep_config_hs[USB_EP7 - 1].max_pkt_size      = 1024;
		gUSBMng.ep_config_hs[USB_EP7 - 1].high_bandwidth   = HBW_NOT;

		gUSBMng.ep_config_hs[USB_EP8 - 1].enable          = TRUE;
		gUSBMng.ep_config_hs[USB_EP8 - 1].blk_size         = 1024;
		gUSBMng.ep_config_hs[USB_EP8 - 1].blk_num          = BLKNUM_SINGLE;
		gUSBMng.ep_config_hs[USB_EP8 - 1].direction       = EP_DIR_IN;
		gUSBMng.ep_config_hs[USB_EP8 - 1].trnsfer_type     = EP_TYPE_ISOCHRONOUS;
		gUSBMng.ep_config_hs[USB_EP8 - 1].max_pkt_size      = 1024;
		gUSBMng.ep_config_hs[USB_EP8 - 1].high_bandwidth   = HBW_NOT;

		gUSBMng.ep_config_hs[USB_BULKIN_EP - 1].enable          = TRUE;
		gUSBMng.ep_config_hs[USB_BULKIN_EP - 1].blk_size         = 512;
		gUSBMng.ep_config_hs[USB_BULKIN_EP - 1].blk_num          = BLKNUM_DOUBLE;
		gUSBMng.ep_config_hs[USB_BULKIN_EP - 1].direction       = EP_DIR_IN;
		gUSBMng.ep_config_hs[USB_BULKIN_EP - 1].trnsfer_type     = EP_TYPE_BULK;
		gUSBMng.ep_config_hs[USB_BULKIN_EP - 1].max_pkt_size      = 512;
		gUSBMng.ep_config_hs[USB_BULKIN_EP - 1].high_bandwidth   = HBW_NOT;

		gUSBMng.ep_config_hs[USB_BULKOUT_EP - 1].enable          = TRUE;
		gUSBMng.ep_config_hs[USB_BULKOUT_EP - 1].blk_size         = 512;
		gUSBMng.ep_config_hs[USB_BULKOUT_EP - 1].blk_num          = BLKNUM_DOUBLE;
		gUSBMng.ep_config_hs[USB_BULKOUT_EP - 1].direction       = EP_DIR_OUT;
		gUSBMng.ep_config_hs[USB_BULKOUT_EP - 1].trnsfer_type     = EP_TYPE_BULK;
		gUSBMng.ep_config_hs[USB_BULKOUT_EP - 1].max_pkt_size      = 512;
		gUSBMng.ep_config_hs[USB_BULKOUT_EP - 1].high_bandwidth   = HBW_NOT;


		// Config Full Speed endpoint usage.
		gUSBMng.ep_config_fs[USB_BULKIN_EP - 1].enable          = TRUE;
		gUSBMng.ep_config_fs[USB_BULKIN_EP - 1].blk_size         = 512;
		gUSBMng.ep_config_fs[USB_BULKIN_EP - 1].blk_num          = BLKNUM_DOUBLE;
		gUSBMng.ep_config_fs[USB_BULKIN_EP - 1].direction       = EP_DIR_IN;
		gUSBMng.ep_config_fs[USB_BULKIN_EP - 1].trnsfer_type     = EP_TYPE_BULK;
		gUSBMng.ep_config_fs[USB_BULKIN_EP - 1].max_pkt_size      = 64;
		gUSBMng.ep_config_fs[USB_BULKIN_EP - 1].high_bandwidth   = HBW_NOT;

		gUSBMng.ep_config_fs[USB_BULKOUT_EP - 1].enable          = TRUE;
		gUSBMng.ep_config_fs[USB_BULKOUT_EP - 1].blk_size         = 512;
		gUSBMng.ep_config_fs[USB_BULKOUT_EP - 1].blk_num          = BLKNUM_DOUBLE;
		gUSBMng.ep_config_fs[USB_BULKOUT_EP - 1].direction       = EP_DIR_OUT;
		gUSBMng.ep_config_fs[USB_BULKOUT_EP - 1].trnsfer_type     = EP_TYPE_BULK;
		gUSBMng.ep_config_fs[USB_BULKOUT_EP - 1].max_pkt_size      = 64;
		gUSBMng.ep_config_fs[USB_BULKOUT_EP - 1].high_bandwidth   = HBW_NOT;

		gUSBMng.ep_config_fs[USB_INTRIN_EP - 1].enable          = TRUE;
		gUSBMng.ep_config_fs[USB_INTRIN_EP - 1].blk_size         = 512;
		gUSBMng.ep_config_fs[USB_INTRIN_EP - 1].blk_num          = BLKNUM_SINGLE;
		gUSBMng.ep_config_fs[USB_INTRIN_EP - 1].direction       = EP_DIR_IN;
		gUSBMng.ep_config_fs[USB_INTRIN_EP - 1].trnsfer_type     = EP_TYPE_INTERRUPT;
		gUSBMng.ep_config_fs[USB_INTRIN_EP - 1].max_pkt_size      = 64;
		gUSBMng.ep_config_fs[USB_INTRIN_EP - 1].high_bandwidth   = HBW_NOT;

		gUSBMng.ep_config_fs[USB_INTROUT_EP - 1].enable          = TRUE;
		gUSBMng.ep_config_fs[USB_INTROUT_EP - 1].blk_size         = 512;
		gUSBMng.ep_config_fs[USB_INTROUT_EP - 1].blk_num          = BLKNUM_SINGLE;
		gUSBMng.ep_config_fs[USB_INTROUT_EP - 1].direction       = EP_DIR_OUT;
		gUSBMng.ep_config_fs[USB_INTROUT_EP - 1].trnsfer_type     = EP_TYPE_INTERRUPT;
		gUSBMng.ep_config_fs[USB_INTROUT_EP - 1].max_pkt_size      = 64;
		gUSBMng.ep_config_fs[USB_INTROUT_EP - 1].high_bandwidth   = HBW_NOT;

		gUSBMng.usb_type                         = USB_TEST;
		gUSBMng.fp_open_needed_fifo              = emu_usb_test_open_needed_fifo;
		gUSBMng.fp_event_callback                      = emu_usb_test_dispatch_event;

		usb2dev_set_ep_config(USB_EP7, USB_EPCFG_ID_AUTOHDR_LEN,          12);
		usb2dev_set_ep_config(USB_EP7, USB_EPCFG_ID_AUTOHDR_START,        ENABLE);
		usb2dev_set_ep_config(USB_EP7, USB_EPCFG_ID_AUTOHDR_STOP,         ENABLE);
		usb2dev_set_ep_config(USB_EP7, USB_EPCFG_ID_AUTOHDR_RSTCOUNTER,   ENABLE);
		usb2dev_set_ep_config(USB_EP7, USB_EPCFG_ID_AUTOHDR_EN,           ENABLE);

		usb2dev_set_ep_config(USB_EP8, USB_EPCFG_ID_AUTOHDR_LEN,          12);
		usb2dev_set_ep_config(USB_EP8, USB_EPCFG_ID_AUTOHDR_START,        ENABLE);
		usb2dev_set_ep_config(USB_EP8, USB_EPCFG_ID_AUTOHDR_STOP,         ENABLE);
		usb2dev_set_ep_config(USB_EP8, USB_EPCFG_ID_AUTOHDR_RSTCOUNTER,   ENABLE);
		usb2dev_set_ep_config(USB_EP8, USB_EPCFG_ID_AUTOHDR_EN,           ENABLE);

		clr_flg(FLG_ID_USBTEST, 0xFFFFFFFF);
		set_flg(FLG_ID_USBTEST, EMUUSBTEST_FLAG_IDLE1 | EMUUSBTEST_FLAG_IDLE2 | EMUUSBTEST_FLAG_IDLE3 | EMUUSBTEST_FLAG_IDLE4 |
				EMUUSBTEST_FLAG_IDLE5 | EMUUSBTEST_FLAG_IDLE6 | EMUUSBTEST_FLAG_IDLE7);

		// start task
		//sta_tsk(USBOTEST_TSK_ID1, 0);
		//sta_tsk(USBOTEST_TSK_ID2, 0);
		//sta_tsk(USBOTEST_TSK_IDV1, 0);
		//sta_tsk(USBOTEST_TSK_IDV2, 0);

		USBOTEST_TSK_ID1 = vos_task_create(emu_usb_test_bulk_thread,  0, "USBOTEST_TSK_ID1",   20,	1024);
		vos_task_resume(USBOTEST_TSK_ID1);
		USBOTEST_TSK_ID2 = vos_task_create(emu_usb_test_cx_out_thread,  0, "USBOTEST_TSK_ID2",   22,	1024);
		vos_task_resume(USBOTEST_TSK_ID2);



		for (i = 0; i < (EMUUSBTEST_ISOC_SIZE + 4096); i++) {
			puiU2IsocInBuf[i] = i % 255;
		}

	} else {
		void *pdesc;

		//debug_msg("PLEASE set USB_FIFOMAP_FIFO4FIRST to 1\r\n");
		//debug_msg("PLEASE set USB_FIFOMAP_FIFO4FIRST to 1\r\n");
		gU2HsTestCfgDesc[2]  = 60;
		gU2HsTestCfgDesc[13] = 6;

		//if(emu_usb_test_mode == EMUUSBTEST_MODE_CIO_BIO_RIO) {
		//	gU2HsTestCfgDesc[13] = 4;
		//	gU2HsTestCfgDesc[2]  = 60-14;
		//}

		usb2dev_set_callback(USB_CALLBACK_CX_VENDOR_REQUEST, (USB_GENERIC_CB)emu_usb_test_control_vendor_request);

		pdesc = kmalloc(sizeof(USB_DEVICE_DESC), GFP_KERNEL);
		memcpy(pdesc, &gU2HsTestDevDesc, sizeof(USB_DEVICE_DESC));
		gUSBMng.p_dev_desc            = (USB_DEVICE_DESC *)pdesc;

		pdesc = kmalloc(60, GFP_KERNEL);
		memcpy(pdesc, &gU2HsTestCfgDesc, 60);
		gUSBMng.p_config_desc_hs       = (USB_CONFIG_DESC *)pdesc;


		pdesc = kmalloc(46, GFP_KERNEL);
		memcpy(pdesc, &gU2FsTestCfgDesc, 46);
		gUSBMng.p_config_desc_fs       = (USB_CONFIG_DESC *)pdesc;

		pdesc = kmalloc(60, GFP_KERNEL);
		memcpy(pdesc, &gU2HsTestCfgDesc, 60);
		gUSBMng.p_config_desc_fs_other  = (USB_CONFIG_DESC *)pdesc;

		pdesc = kmalloc(46, GFP_KERNEL);
		memcpy(pdesc, &gU2FsTestCfgDesc, 46);
		gUSBMng.p_config_desc_hs_other  = (USB_CONFIG_DESC *)pdesc;

		pdesc = kmalloc(sizeof(USB_DEVICE_DESC), GFP_KERNEL);
		memcpy(pdesc, &gU2devQualiDesc, sizeof(USB_DEVICE_DESC));
		gUSBMng.p_dev_quali_desc       = (USB_DEVICE_DESC *)pdesc;

		gUSBMng.num_of_configurations  = 1;
		gUSBMng.num_of_strings         = 5;

		pdesc = kmalloc(4, GFP_KERNEL);
		memcpy(pdesc, &gU2StrDesc0, 4);
		gUSBMng.p_string_desc[0]      = (USB_STRING_DESC *)pdesc;

		pdesc = kmalloc(gU2StrDesc1[0], GFP_KERNEL);
		memcpy(pdesc, &gU2StrDesc1, gU2StrDesc1[0]);
		gUSBMng.p_string_desc[1]      = (USB_STRING_DESC *)&pdesc;

		pdesc = kmalloc(gU2StrDesc2[0], GFP_KERNEL);
		memcpy(pdesc, &gU2StrDesc2, gU2StrDesc2[0]);
		gUSBMng.p_string_desc[2]      = (USB_STRING_DESC *)pdesc;

		pdesc = kmalloc(gU2StrDesc3[0], GFP_KERNEL);
		memcpy(pdesc, &gU2StrDesc3, gU2StrDesc3[0]);
		gUSBMng.p_string_desc[3]      = (USB_STRING_DESC *)pdesc;

		pdesc = kmalloc(gU2StrDesc4[0], GFP_KERNEL);
		memcpy(pdesc, &gU2StrDesc4, gU2StrDesc4[0]);
		gUSBMng.p_string_desc[4]      = (USB_STRING_DESC *)pdesc;

		//
		//  Configure USB Endpoint allocation
		//
		for (i = 0; i < 15; i++) {
			gUSBMng.ep_config_hs[i].enable      = FALSE;
			gUSBMng.ep_config_fs[i].enable      = FALSE;
		}

		// Config High Speed endpoint usage.
		gUSBMng.ep_config_hs[USB_BULKIN_EP - 1].enable          = TRUE;
		gUSBMng.ep_config_hs[USB_BULKIN_EP - 1].blk_size         = 512;
		gUSBMng.ep_config_hs[USB_BULKIN_EP - 1].blk_num          = BLKNUM_DOUBLE;
		gUSBMng.ep_config_hs[USB_BULKIN_EP - 1].direction       = EP_DIR_IN;
		gUSBMng.ep_config_hs[USB_BULKIN_EP - 1].trnsfer_type     = EP_TYPE_BULK;
		gUSBMng.ep_config_hs[USB_BULKIN_EP - 1].max_pkt_size      = 512;
		gUSBMng.ep_config_hs[USB_BULKIN_EP - 1].high_bandwidth   = HBW_NOT;

		gUSBMng.ep_config_hs[USB_BULKOUT_EP - 1].enable          = TRUE;
		gUSBMng.ep_config_hs[USB_BULKOUT_EP - 1].blk_size         = 512;
		gUSBMng.ep_config_hs[USB_BULKOUT_EP - 1].blk_num          = BLKNUM_DOUBLE;
		gUSBMng.ep_config_hs[USB_BULKOUT_EP - 1].direction       = EP_DIR_OUT;
		gUSBMng.ep_config_hs[USB_BULKOUT_EP - 1].trnsfer_type     = EP_TYPE_BULK;
		gUSBMng.ep_config_hs[USB_BULKOUT_EP - 1].max_pkt_size      = 512;
		gUSBMng.ep_config_hs[USB_BULKOUT_EP - 1].high_bandwidth   = HBW_NOT;

		gUSBMng.ep_config_hs[USB_INTRIN_EP - 1].enable          = TRUE;
		gUSBMng.ep_config_hs[USB_INTRIN_EP - 1].blk_size         = 1024;
		gUSBMng.ep_config_hs[USB_INTRIN_EP - 1].blk_num          = BLKNUM_SINGLE;
		gUSBMng.ep_config_hs[USB_INTRIN_EP - 1].direction       = EP_DIR_IN;
		gUSBMng.ep_config_hs[USB_INTRIN_EP - 1].trnsfer_type     = EP_TYPE_INTERRUPT;
		gUSBMng.ep_config_hs[USB_INTRIN_EP - 1].max_pkt_size      = 1024;
		gUSBMng.ep_config_hs[USB_INTRIN_EP - 1].high_bandwidth   = HBW_NOT;

		gUSBMng.ep_config_hs[USB_INTROUT_EP - 1].enable          = TRUE;
		gUSBMng.ep_config_hs[USB_INTROUT_EP - 1].blk_size         = 1024;
		gUSBMng.ep_config_hs[USB_INTROUT_EP - 1].blk_num          = BLKNUM_SINGLE;
		gUSBMng.ep_config_hs[USB_INTROUT_EP - 1].direction       = EP_DIR_OUT;
		gUSBMng.ep_config_hs[USB_INTROUT_EP - 1].trnsfer_type     = EP_TYPE_INTERRUPT;
		gUSBMng.ep_config_hs[USB_INTROUT_EP - 1].max_pkt_size      = 1024;
		gUSBMng.ep_config_hs[USB_INTROUT_EP - 1].high_bandwidth   = HBW_NOT;

		if (usb2dev_get_config(USB_CONFIG_ID_USE_SHARE_FIFO)) {
			gUSBMng.ep_config_hs[USB_ISOCIN_EP - 1].enable          = TRUE;
			gUSBMng.ep_config_hs[USB_ISOCIN_EP - 1].blk_size         = 1024;
			gUSBMng.ep_config_hs[USB_ISOCIN_EP - 1].blk_num          = BLKNUM_SINGLE;
			gUSBMng.ep_config_hs[USB_ISOCIN_EP - 1].direction       = EP_DIR_IN;
			gUSBMng.ep_config_hs[USB_ISOCIN_EP - 1].trnsfer_type     = EP_TYPE_ISOCHRONOUS;
			gUSBMng.ep_config_hs[USB_ISOCIN_EP - 1].max_pkt_size      = 1024;
			gUSBMng.ep_config_hs[USB_ISOCIN_EP - 1].high_bandwidth   = HBW_NOT;

			if (emu_usb_test_mode == EMUUSBTEST_MODE_CIO_BIO_RIO_IIO) {
				gUSBMng.ep_config_hs[USB_ISOCOUT_EP - 1].enable          = TRUE;
				gUSBMng.ep_config_hs[USB_ISOCOUT_EP - 1].blk_size         = 1024;
				gUSBMng.ep_config_hs[USB_ISOCOUT_EP - 1].blk_num          = BLKNUM_SINGLE;
				gUSBMng.ep_config_hs[USB_ISOCOUT_EP - 1].direction       = EP_DIR_OUT;
				gUSBMng.ep_config_hs[USB_ISOCOUT_EP - 1].trnsfer_type     = EP_TYPE_ISOCHRONOUS;
				gUSBMng.ep_config_hs[USB_ISOCOUT_EP - 1].max_pkt_size      = 1024;
				gUSBMng.ep_config_hs[USB_ISOCOUT_EP - 1].high_bandwidth   = HBW_NOT;
			}
		}
		// Config Full Speed endpoint usage.
		gUSBMng.ep_config_fs[USB_BULKIN_EP - 1].enable          = TRUE;
		gUSBMng.ep_config_fs[USB_BULKIN_EP - 1].blk_size         = 512;
		gUSBMng.ep_config_fs[USB_BULKIN_EP - 1].blk_num          = BLKNUM_DOUBLE;
		gUSBMng.ep_config_fs[USB_BULKIN_EP - 1].direction       = EP_DIR_IN;
		gUSBMng.ep_config_fs[USB_BULKIN_EP - 1].trnsfer_type     = EP_TYPE_BULK;
		gUSBMng.ep_config_fs[USB_BULKIN_EP - 1].max_pkt_size      = 64;
		gUSBMng.ep_config_fs[USB_BULKIN_EP - 1].high_bandwidth   = HBW_NOT;

		gUSBMng.ep_config_fs[USB_BULKOUT_EP - 1].enable          = TRUE;
		gUSBMng.ep_config_fs[USB_BULKOUT_EP - 1].blk_size         = 512;
		gUSBMng.ep_config_fs[USB_BULKOUT_EP - 1].blk_num          = BLKNUM_DOUBLE;
		gUSBMng.ep_config_fs[USB_BULKOUT_EP - 1].direction       = EP_DIR_OUT;
		gUSBMng.ep_config_fs[USB_BULKOUT_EP - 1].trnsfer_type     = EP_TYPE_BULK;
		gUSBMng.ep_config_fs[USB_BULKOUT_EP - 1].max_pkt_size      = 64;
		gUSBMng.ep_config_fs[USB_BULKOUT_EP - 1].high_bandwidth   = HBW_NOT;

		gUSBMng.ep_config_fs[USB_INTRIN_EP - 1].enable          = TRUE;
		gUSBMng.ep_config_fs[USB_INTRIN_EP - 1].blk_size         = 512;
		gUSBMng.ep_config_fs[USB_INTRIN_EP - 1].blk_num          = BLKNUM_SINGLE;
		gUSBMng.ep_config_fs[USB_INTRIN_EP - 1].direction       = EP_DIR_IN;
		gUSBMng.ep_config_fs[USB_INTRIN_EP - 1].trnsfer_type     = EP_TYPE_INTERRUPT;
		gUSBMng.ep_config_fs[USB_INTRIN_EP - 1].max_pkt_size      = 64;
		gUSBMng.ep_config_fs[USB_INTRIN_EP - 1].high_bandwidth   = HBW_NOT;

		gUSBMng.ep_config_fs[USB_INTROUT_EP - 1].enable          = TRUE;
		gUSBMng.ep_config_fs[USB_INTROUT_EP - 1].blk_size         = 512;
		gUSBMng.ep_config_fs[USB_INTROUT_EP - 1].blk_num          = BLKNUM_SINGLE;
		gUSBMng.ep_config_fs[USB_INTROUT_EP - 1].direction       = EP_DIR_OUT;
		gUSBMng.ep_config_fs[USB_INTROUT_EP - 1].trnsfer_type     = EP_TYPE_INTERRUPT;
		gUSBMng.ep_config_fs[USB_INTROUT_EP - 1].max_pkt_size      = 64;
		gUSBMng.ep_config_fs[USB_INTROUT_EP - 1].high_bandwidth   = HBW_NOT;

		if (usb2dev_get_config(USB_CONFIG_ID_USE_SHARE_FIFO)) {
			gUSBMng.ep_config_fs[USB_ISOCIN_EP - 1].enable          = TRUE;
			gUSBMng.ep_config_fs[USB_ISOCIN_EP - 1].blk_size         = 1024;
			gUSBMng.ep_config_fs[USB_ISOCIN_EP - 1].blk_num          = BLKNUM_SINGLE;
			gUSBMng.ep_config_fs[USB_ISOCIN_EP - 1].direction       = EP_DIR_IN;
			gUSBMng.ep_config_fs[USB_ISOCIN_EP - 1].trnsfer_type     = EP_TYPE_ISOCHRONOUS;
			gUSBMng.ep_config_fs[USB_ISOCIN_EP - 1].max_pkt_size      = 1020;
			gUSBMng.ep_config_fs[USB_ISOCIN_EP - 1].high_bandwidth   = HBW_NOT;

			if (emu_usb_test_mode == EMUUSBTEST_MODE_CIO_BIO_RIO_IIO) {
				gUSBMng.ep_config_fs[USB_ISOCOUT_EP - 1].enable          = TRUE;
				gUSBMng.ep_config_fs[USB_ISOCOUT_EP - 1].blk_size         = 1024;
				gUSBMng.ep_config_fs[USB_ISOCOUT_EP - 1].blk_num          = BLKNUM_SINGLE;
				gUSBMng.ep_config_fs[USB_ISOCOUT_EP - 1].direction       = EP_DIR_OUT;
				gUSBMng.ep_config_fs[USB_ISOCOUT_EP - 1].trnsfer_type     = EP_TYPE_ISOCHRONOUS;
				gUSBMng.ep_config_fs[USB_ISOCOUT_EP - 1].max_pkt_size      = 1024;
				gUSBMng.ep_config_fs[USB_ISOCOUT_EP - 1].high_bandwidth   = HBW_NOT;
			}
		}

		gUSBMng.usb_type                         = USB_TEST;
		gUSBMng.fp_open_needed_fifo              = emu_usb_test_open_needed_fifo;
		gUSBMng.fp_event_callback                      = emu_usb_test_dispatch_event;

		clr_flg(FLG_ID_USBTEST, 0xFFFFFFFF);
		set_flg(FLG_ID_USBTEST, EMUUSBTEST_FLAG_IDLE1 | EMUUSBTEST_FLAG_IDLE2 | EMUUSBTEST_FLAG_IDLE3 | EMUUSBTEST_FLAG_IDLE4 |
				EMUUSBTEST_FLAG_IDLE5 | EMUUSBTEST_FLAG_IDLE6 | EMUUSBTEST_FLAG_IDLE7);
		// start task
		//sta_tsk(USBOTEST_TSK_ID1, 0);
		//sta_tsk(USBOTEST_TSK_ID2, 0);
		//sta_tsk(USBOTEST_TSK_ID3, 0);
		//sta_tsk(USBOTEST_TSK_ID4, 0);
		//sta_tsk(USBOTEST_TSK_ID5, 0);

		USBOTEST_TSK_ID1 = vos_task_create(emu_usb_test_bulk_thread,  0, "USBOTEST_TSK_ID1",   20,	1024);
		vos_task_resume(USBOTEST_TSK_ID1);
		USBOTEST_TSK_ID2 = vos_task_create(emu_usb_test_cx_out_thread,  0, "USBOTEST_TSK_ID2",   22,	1024);
		vos_task_resume(USBOTEST_TSK_ID2);



		if ((emu_usb_test_mode == EMUUSBTEST_MODE_CIO_BIO_RIO_IIO) ||
			(emu_usb_test_mode == EMUUSBTEST_MODE_CIO_BIO_RIO_II)) {
			for (i = 0; i < (EMUUSBTEST_ISOC_SIZE + 4096); i++) {
				puiU2IsocInBuf[i] = i % 255;
			}
		}

	}

	usb2dev_set_management(&gUSBMng);

	if (E_TMOUT == usb2dev_open()) {
		debug_err(("%s:USB Host No Response\r\n", __func__));
	}


}



void emu_usb_test_close(void)
{
	//FLGPTN          uiFlag;

	//wai_flg(&uiFlag, FLG_ID_USBTEST, EMUUSBTEST_FLAG_IDLE1, TWF_ORW | TWF_CLR);
	vos_task_destroy(USBOTEST_TSK_ID1);
	//wai_flg(&uiFlag, FLG_ID_USBTEST, EMUUSBTEST_FLAG_IDLE2, TWF_ORW | TWF_CLR);
	vos_task_destroy(USBOTEST_TSK_ID2);
	//wai_flg(&uiFlag, FLG_ID_USBTEST, EMUUSBTEST_FLAG_IDLE3, TWF_ORW | TWF_CLR);
	vos_task_destroy(USBOTEST_TSK_ID3);
	//wai_flg(&uiFlag, FLG_ID_USBTEST, EMUUSBTEST_FLAG_IDLE4, TWF_ORW | TWF_CLR);
	vos_task_destroy(USBOTEST_TSK_ID4);
	//wai_flg(&uiFlag, FLG_ID_USBTEST, EMUUSBTEST_FLAG_IDLE5, TWF_ORW | TWF_CLR);
	vos_task_destroy(USBOTEST_TSK_ID5);

	//wai_flg(&uiFlag, FLG_ID_USBTEST, EMUUSBTEST_FLAG_IDLE6, TWF_ORW | TWF_CLR);
	vos_task_destroy(USBOTEST_TSK_IDV1);
	//wai_flg(&uiFlag, FLG_ID_USBTEST, EMUUSBTEST_FLAG_IDLE7, TWF_ORW | TWF_CLR);
	vos_task_destroy(USBOTEST_TSK_IDV2);

	usb2dev_mask_ep_interrupt(USB_BULKOUT_EP);
	usb2dev_mask_ep_interrupt(USB_BULKIN_EP);
	usb2dev_mask_ep_interrupt(USB_INTROUT_EP);
	usb2dev_mask_ep_interrupt(USB_INTRIN_EP);
	usb2dev_mask_ep_interrupt(USB_ISOCIN_EP);
	usb2dev_mask_ep_interrupt(USB_ISOCOUT_EP);

	usb2dev_close();

}
#endif

