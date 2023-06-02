/*
    USB Standard request processing module

    This file handle Standard request for USB device and is
    for internal used only.

    @file       usb_DevReqStd.c
    @ingroup    mIDrvUSB_Device
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#include "usb2dev.h"
#include "usb2dev_reg.h"
#include "usb2dev_int.h"

// Internal function declaration for StandardDeviceRequest[]
static void ReqGetStatus(void);
static void ReqClrFeature(void);
static void ReqSetFeature(void);
static void ReqGetDescriptor(void);
static void ReqSetAddress(void);
static void ReqGetConfig(void);
static void ReqSetConfig(void);
static void ReqGetInterface(void);
static void ReqSetInterface(void);
static void ReqSynchFrame(void);

//
// Internal static functions
//
static void usb_clrEPFIFOMap(void);
static void usb_clrAllEP(void);
static BOOL usb_isFIFOEnabled(UINT32 FIFOn);
static void usb_clrEPStall(USB_EP EPn);
static void usb_rstEPDataToggle(USB_EP EPn);


// processing of USB setup transaction TYPE = Standard
_ALIGNED(4) void (*StandardDeviceRequest[])(void) = {
	ReqGetStatus,           //0
	ReqClrFeature,          //1
	ReqReserved,            //2
	ReqSetFeature,          //3
	ReqReserved,            //4
	ReqSetAddress,          //5
	ReqGetDescriptor,       //6
	ReqReserved,            //7
	ReqGetConfig,           //8
	ReqSetConfig,           //9
	ReqGetInterface,        //10
	ReqSetInterface,        //11
	ReqSynchFrame,          //12
	ReqReserved,            //13
	ReqReserved,            //14
	ReqReserved             //15
};

unsigned short bUSBflag_configuration = 0;
unsigned short bUSBflag_Remote_wakeup = 0;

// for more than one interface.
static UINT8 g_Alternating = 0;
static UINT8 g_Interface = 0;

//BOOL  bOTGDMARunning;
USB_ACTION  gUSBCxFinishAction;


/**
    @addtogroup mIDrvUSB_Device
*/
//@{

#if 1
/*
    Processing of GetStatus request, S-I-O command.

    @return void
*/
static void ReqGetStatus(void)
{
	UINT8               bRecipient;
	UINT8               bIndex;
	USB_DEVICE_REQUEST  *pDevReq;
	UINT8               uRecipientStatus[2];
	UINT32              DMALen;

	pDevReq     = (USB_DEVICE_REQUEST *)&usb2dev_control_data.device_request;
	bRecipient  = pDevReq->bm_request_type & USB_RECIPIENT;
	bIndex      = pDevReq->w_index;

	uRecipientStatus[0] = 0;
	uRecipientStatus[1] = 0;

	if (bRecipient == USB_RECIPIENT_DEVICE) {
		// Return 2-byte's Device status (Bit1:Remote_Wakeup, Bit0:Self_Powered) to Host
		if ((bUSBflag_Remote_wakeup & 0x01) == 1) {
			uRecipientStatus[0] |= 0x02;
		}

		if (gUSBManage.p_config_desc->bm_attributes & USB_DEVDESC_ATB_SELF_POWER) {
			uRecipientStatus[0] |= 0x01;
		}

		gUSBCxFinishAction = USB_ACT_DONE;
	} else if (bRecipient == USB_RECIPIENT_INTERFACE) {
		// Return 2-byte ZEROs Interface status to Host
		gUSBCxFinishAction = USB_ACT_DONE;
	} else if (bRecipient == USB_RECIPIENT_ENDPOINT) {
		USB_FIFO_NUM FIFOn;

		// Return 2-byte's ep status (Bit0:halted) to Host
		// assume endpoint is stalled
		uRecipientStatus[0] = 0x01;

		switch (bIndex & USB_EP_ADDRESS_MASK) {
		case USB_EP1:
		case USB_EP2:
		case USB_EP3:
		case USB_EP4:
		case USB_EP5:
		case USB_EP6:
		case USB_EP7:
		case USB_EP8:
		case USB_EP9:
		case USB_EP10:
		case USB_EP11:
		case USB_EP12:
		case USB_EP13:
		case USB_EP14:
		case USB_EP15:
			{
				FIFOn = usb_getFifoFromEp(bIndex & USB_EP_ADDRESS_MASK);

				if ((FIFOn != USB_FIFO_NOT_USE) && usb_isFIFOEnabled(FIFOn)) {

					if (((USB_EP_IN_ADDRESS & bIndex)  && (usb_getEpDirection(bIndex & USB_EP_ADDRESS_MASK) == EP_DIR_IN)) ||
						((!(USB_EP_IN_ADDRESS & bIndex)) && (usb_getEpDirection(bIndex & USB_EP_ADDRESS_MASK) == EP_DIR_OUT))) {
						if (usb_EPStallStatus(bIndex & USB_EP_ADDRESS_MASK) == FALSE) {
							uRecipientStatus[0] = 0x00;
						}

						gUSBCxFinishAction = USB_ACT_DONE;
						break;
					}
				}
			}
		// If no FIFO maps to this EP, fall through to default case.
		default: {
				gUSBCxFinishAction = USB_ACT_STALL;
			}
			return;
		}


	}    // End of else if (bRecipient == USB_RECIPIENT_ENDPOINT)
	else {
		gUSBCxFinishAction = USB_ACT_STALL;
		return;
	}

	DMALen = 2;
	usb2dev_write_endpoint(USB_EP0, uRecipientStatus, &DMALen);

	USB_PutString1(" ReqGetStatus = %x  \r\n", uRecipientStatus[0]);
}



/*
    Processing of ClearFeature request, S-I command.

    @return void
*/
static void ReqClrFeature(void)
{
	UINT8                   bRecipient;
	UINT8                   FeatureSelect;
	UINT8                   bIndex;
	USB_DEVICE_REQUEST      *pDevReq;
	T_USB_DEVMAINCTRL_REG   devMainCtl;

	pDevReq         = (USB_DEVICE_REQUEST *)&usb2dev_control_data.device_request;
	bRecipient      = pDevReq->bm_request_type & USB_RECIPIENT;
	FeatureSelect   = pDevReq->w_value;
	bIndex          = pDevReq->w_index;

	if ((bRecipient == USB_RECIPIENT_DEVICE) && (FeatureSelect == USB_FEATURE_REMOTE_WAKEUP)) {
		bUSBflag_Remote_wakeup &= ~0x01;

		// clear Remote Wakeup
		devMainCtl.reg = USB_GETREG(USB_DEVMAINCTRL_REG_OFS);
		devMainCtl.bit.CAP_RMWAKUP = 0;
		USB_SETREG(USB_DEVMAINCTRL_REG_OFS, devMainCtl.reg);

		gUSBCxFinishAction = USB_ACT_DONE;
	} else if ((bRecipient == USB_RECIPIENT_ENDPOINT) && (FeatureSelect == USB_FEATURE_ENDPOINT_STALL)) {
		USB_FIFO_NUM FIFOn;

		switch (bIndex & USB_EP_ADDRESS_MASK) {
		case USB_EP0: {
				// EP0 stall is cleared automatically when SETUP is received
				// clear EP0 Stall
				gUSBCxFinishAction = USB_ACT_DONE;
			}
			break;

		case USB_EP1:
		case USB_EP2:
		case USB_EP3:
		case USB_EP4:
		case USB_EP5:
		case USB_EP6:
		case USB_EP7:
		case USB_EP8:
		case USB_EP9:
		case USB_EP10:
		case USB_EP11:
		case USB_EP12:
		case USB_EP13:
		case USB_EP14:
		case USB_EP15:
			{
				FIFOn = usb_getFifoFromEp(bIndex & USB_EP_ADDRESS_MASK);

				if ((FIFOn != USB_FIFO_NOT_USE) && usb_isFIFOEnabled(FIFOn)) {
					if (((USB_EP_IN_ADDRESS & bIndex)  && (usb_getEpDirection(bIndex & USB_EP_ADDRESS_MASK) == EP_DIR_IN)) ||
						((!(USB_EP_IN_ADDRESS & bIndex)) && (usb_getEpDirection(bIndex & USB_EP_ADDRESS_MASK) == EP_DIR_OUT))) {

						// clear EP Stall
						usb_clrEPStall(USB_EP_ADDRESS_MASK & bIndex);

						// reset toggle sequence for IN EP. spec page 256.
						usb_rstEPDataToggle(USB_EP_ADDRESS_MASK & bIndex);

						if (gUSBManage.fp_event_callback) {
							gUSBManage.fp_event_callback(USB_EVENT_CLRFEATURE);
						}

						gUSBCxFinishAction = USB_ACT_DONE;
						break;

					}
				}
			}
		// If no FIFO maps to this EP, fall through to default case.
		default: {
				gUSBCxFinishAction = USB_ACT_STALL;
			}
			return;
		}
	} else {
		usb2dev_set_ep_stall(USB_EP0);
		usb2dev_set_ep0_done();
		return;
	}

}



/*
    Processing of SetFeature request, S-I command.

    @return void
*/
static void ReqSetFeature(void)
{
	UINT8                           bRecipient;
	UINT8                           FeatureSelect;
	UINT16                          bIndex;
	UINT8                           i, TstPkt[60];
	UINT32                          TstPktLen;
	USB_DEVICE_REQUEST              *pDevReq;
	T_USB_DEVCXCFGFIFOSTATUS_REG    devCXCFG;
	T_USB_DEVMAINCTRL_REG           devMainCtl;

	pDevReq         = (USB_DEVICE_REQUEST *)&usb2dev_control_data.device_request;
	bRecipient      = pDevReq->bm_request_type & USB_RECIPIENT;
	FeatureSelect   = pDevReq->w_value;
	bIndex          = pDevReq->w_index;

	if ((bRecipient == USB_RECIPIENT_DEVICE) && (FeatureSelect == USB_FEATURE_REMOTE_WAKEUP)) {
		bUSBflag_Remote_wakeup |= 0x01;

		// set Remote Wakeup capability
		devMainCtl.reg = USB_GETREG(USB_DEVMAINCTRL_REG_OFS);
		devMainCtl.bit.CAP_RMWAKUP = 1;
		USB_SETREG(USB_DEVMAINCTRL_REG_OFS, devMainCtl.reg);

		gUSBCxFinishAction = USB_ACT_DONE;
	}

	else if ((bRecipient == USB_RECIPIENT_ENDPOINT) && (FeatureSelect == USB_FEATURE_ENDPOINT_STALL)) {
		USB_FIFO_NUM FIFOn;

		switch (USB_EP_ADDRESS_MASK & bIndex) {
		case USB_EP1:
		case USB_EP2:
		case USB_EP3:
		case USB_EP4:
		case USB_EP5:
		case USB_EP6:
		case USB_EP7:
		case USB_EP8:
		case USB_EP9:
		case USB_EP10:
		case USB_EP11:
		case USB_EP12:
		case USB_EP13:
		case USB_EP14:
		case USB_EP15:
			{
				FIFOn = usb_getFifoFromEp(USB_EP_ADDRESS_MASK & bIndex);
				if ((FIFOn != USB_FIFO_NOT_USE) && usb_isFIFOEnabled(FIFOn)) {
					if (((USB_EP_IN_ADDRESS & bIndex)  && (usb_getEpDirection(bIndex & USB_EP_ADDRESS_MASK) == EP_DIR_IN)) ||
						((!(USB_EP_IN_ADDRESS & bIndex)) && (usb_getEpDirection(bIndex & USB_EP_ADDRESS_MASK) == EP_DIR_OUT))) {
						usb2dev_set_ep_stall(USB_EP_ADDRESS_MASK & bIndex);

						gUSBCxFinishAction = USB_ACT_DONE;
						break;
					}
				}
			}
		// If no FIFO maps to this EP, fall through to default case.
		default: {
				gUSBCxFinishAction = USB_ACT_STALL;
			}
			return;
		}
	} else if ((bRecipient == USB_RECIPIENT_DEVICE) && (FeatureSelect == USB_FEATURE_POWER_D0)) {
		switch (bIndex >> 8) {
		case 0x1: {
				usb2dev_set_ep0_done();
				USB_DELAY_MS(100);
				USB_SETREG(USB_PHYTSTSELECT_REG_OFS, TEST_J);
				DBG_ERR("TEST_J\r\n");

				USBPHY_SETREG(0x10D4, 0xE2);
				USBPHY_SETREG(0x10D0, 0xFF);
				USBPHY_SETREG(0x1088, 0x00);
				USBPHY_SETREG(0x108C, 0xFF);
				USBPHY_SETREG(0x1080, 0x31);
			}
			break;

		case 0x2: {
				usb2dev_set_ep0_done();
				USB_DELAY_MS(100);
				USB_SETREG(USB_PHYTSTSELECT_REG_OFS, TEST_K);
				DBG_ERR("TEST_K\r\n");

				USBPHY_SETREG(0x10D4, 0xE2);
				USBPHY_SETREG(0x10D0, 0xFF);
				USBPHY_SETREG(0x1088, 0x00);
				USBPHY_SETREG(0x108C, 0x00);
				USBPHY_SETREG(0x1080, 0x31);
			}
			break;

		case 0x3: {
				T_USB_DEVADDR_REG   devAddr;

				// The hardware test packet's address is fixed as 0x1.
				devAddr.reg = USB_GETREG(USB_DEVADDR_REG_OFS);
				devAddr.bit.DEVADR = 0x1;
				USB_SETREG(USB_DEVADDR_REG_OFS, devAddr.reg);

				usb2dev_set_ep0_done();
				USB_DELAY_MS(100);

				USB_SETREG(USB_PHYTSTSELECT_REG_OFS, TEST_SE0_NAK);
				DBG_ERR("TEST_SE0_NAK\r\n");
			}
			break;

		case 0x4: {
				usb2dev_set_ep0_done();     // special case: follow the test sequence
				USB_DELAY_MS(100);

				USB_SETREG(USB_PHYTSTSELECT_REG_OFS, TEST_PKY);
				for (i = 0; i < 9; i++) { // JKJKJKJK x 9
					TstPkt[i] = 0x00;
				}
				for (i = 9; i < 17; i++) { // 8*AA
					TstPkt[i] = 0xAA;
				}
				for (i = 17; i < 25; i++) { // 8*EE
					TstPkt[i] = 0xEE;
				}

				TstPkt[25] = 0xFE;

				for (i = 26; i < 37; i++) { // 11*FF
					TstPkt[i] = 0xFF;
				}

				TstPkt[37] = 0x7F;
				TstPkt[38] = 0xBF;
				TstPkt[39] = 0xDF;
				TstPkt[40] = 0xEF;
				TstPkt[41] = 0xF7;
				TstPkt[42] = 0xFB;
				TstPkt[43] = 0xFD;
				TstPkt[44] = 0xFC;
				TstPkt[45] = 0x7E;
				TstPkt[46] = 0xBF;
				TstPkt[47] = 0xDF;
				TstPkt[48] = 0xEF;
				TstPkt[49] = 0xF7;
				TstPkt[50] = 0xFB;
				TstPkt[51] = 0xFD;
				TstPkt[52] = 0x7E;

				TstPktLen = 53;
				usb2dev_write_endpoint(USB_EP0, TstPkt, &TstPktLen);

				// data transfer is done for test packet
				devCXCFG.reg = USB_GETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS);
				devCXCFG.bit.TST_PKDONE = 1; // bit[1]
				USB_SETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS, devCXCFG.reg);

				#if 0
				// SET PHY Test Packet Enable
				usb_writePhyReg(0x14, 0x2A);
				USB_DELAY_MS(500);
				usb_writePhyReg(0x14, 0x2B);
				USB_DELAY_MS(500);
				#endif
				DBG_ERR("TEST_PKY\r\n");
			}
			break;

		default: {
				usb2dev_set_ep_stall(USB_EP0);
			}
			return;
		}
	} else { //test packet or ...
		usb2dev_set_ep_stall(USB_EP0);
		usb2dev_set_ep0_done();
		return;
	}
}


/*
    Processing of GetDescriptor request, S-I-I-I-O command.

    @return void
*/
static void ReqGetDescriptor(void)
{
	UINT32                  *pDataAddr;
	//UINT8                   *pDscptr;
	UINT16                  datasize;
	UINT16                  DescType;
	UINT16                  DescIdx;
	USB_DEVICE_REQUEST      *pDevReq;
	USB_MNG                 *pUSBMng;
	T_USB_DEVMAINCTRL_REG   devMainCtl;

	pDevReq = (USB_DEVICE_REQUEST *)&usb2dev_control_data.device_request;

	USB_PutString(("ReqGetDescriptor \r\n"));

	//full speed only device report as it is USB 1.1
	devMainCtl.reg = USB_GETREG(USB_DEVMAINCTRL_REG_OFS);
	if (devMainCtl.bit.FORCE_FS == 0) {
		gUSBManage.p_dev_desc->bcd_usb    = 0x0200;
	} else {
		gUSBManage.p_dev_desc->bcd_usb    = 0x0110;
	}

	if (devMainCtl.bit.HS_EN == 1) {
		gUSBManage.p_config_desc         = gUSBManage.p_config_desc_hs;
		gUSBManage.p_config_others_desc   = gUSBManage.p_config_desc_hs_other;
	} else {
		gUSBManage.p_config_desc         = gUSBManage.p_config_desc_fs;
		gUSBManage.p_config_others_desc   = gUSBManage.p_config_desc_fs_other;
	}

	pUSBMng = (USB_MNG *)&gUSBManage;

	//----- decode descriptor type -----------------
	DescType = (pDevReq->w_value & 0xff00) >> 8;
	DescIdx  =   pDevReq->w_value & 0x00ff;

	//----- device descriptor -----------------------
	if (DescType == USB_DEVICE_DESCRIPTOR_TYPE) {
		datasize = sizeof(USB_DEVICE_DESC);
		pDataAddr = (UINT32 *)pUSBMng->p_dev_desc;
	}
	//----- configuration descriptor ----------------
	else if (DescType == USB_CONFIGURATION_DESCRIPTOR_TYPE) {
		if (DescIdx >= pUSBMng->num_of_configurations) {
			gUSBCxFinishAction = USB_ACT_STALL;
			return;
		} else {
			datasize    = (UINT16)pUSBMng->p_config_desc->w_total_length;
			pDataAddr   = (UINT32 *)pUSBMng->p_config_desc;
			//pDscptr     = (UINT8 *)pUSBMng->p_config_desc;

			pUSBMng->p_config_desc->b_descriptor_type = USB_CONFIGURATION_DESCRIPTOR_TYPE;
		}
	}
	//----- string descriptor ------------------------
	else if (DescType == USB_STRING_DESCRIPTOR_TYPE) {
		//---- unsupported string descriptor or invalid index ----
		if ((!pUSBMng->num_of_strings) || (pUSBMng->num_of_strings <= DescIdx)) {
			gUSBCxFinishAction = USB_ACT_STALL;
			return;
		}

		datasize  = (UINT16)(pUSBMng->p_string_desc[DescIdx]->b_length);
		pDataAddr = (UINT32 *)pUSBMng->p_string_desc[DescIdx];
	}
	//full speed only device report as it is USB 1.1
	//USB 1.1 should respond device qualifier descriptor and
	//other speed configuration descriptor as error
	//----- Device_Qualifier descriptor ------------------------
	else if ((devMainCtl.bit.FORCE_FS == 0) && (DescType == USB_DEVICE_QUALIFIER_DESCRIPTOR_TYPE)) {
		datasize  = (UINT16)(pUSBMng->p_dev_quali_desc->b_length);
		pDataAddr = (UINT32 *)pUSBMng->p_dev_quali_desc;
	}
	//-----Other_Speed_Configuration ------------------------
	else if ((devMainCtl.bit.FORCE_FS == 0) && (DescType == USB_OTHER_SPEED_CONFIGURATION)) {
		datasize = (UINT16)(gUSBManage.p_config_others_desc->w_total_length);          //March 22, 2005. Support only one configure!!

		pDataAddr = (UINT32 *)(gUSBManage.p_config_others_desc);
		//pDscptr = (UINT8 *)(pUSBMng->p_config_desc);
		pUSBMng->p_config_desc->b_descriptor_type = USB_OTHER_SPEED_CONFIGURATION;
	}
	//----- Unknown descriptor ------------
	else {

		if(gfpStdUnknownReq != NULL) {
			gfpStdUnknownReq(0);
			return;
		} else {
			gUSBCxFinishAction = USB_ACT_STALL;
			return;
		}
	}

	if (datasize > usb2dev_control_data.w_length) {
		datasize = usb2dev_control_data.w_length;
	}

	usb2dev_control_data.w_length = datasize;
	usb2dev_control_data.p_data   = (UINT8 *)pDataAddr;

	usb2dev_reture_setup_data();
}

/*
    Processing of SetAddress request, S-I command.

    @return void
*/
static void ReqSetAddress(void)
{
	UINT8               ADDR;
	USB_DEVICE_REQUEST  *pDevReq;
	T_USB_DEVADDR_REG   devAddr;

	USB_PutString(("ReqSetAddress \r\n"));

	pDevReq = (USB_DEVICE_REQUEST *)&usb2dev_control_data.device_request;
	ADDR    = pDevReq->w_value & 0x7F;

	devAddr.reg = USB_GETREG(USB_DEVADDR_REG_OFS);
	devAddr.bit.DEVADR = ADDR;
	USB_SETREG(USB_DEVADDR_REG_OFS, devAddr.reg);

	gUSBCxFinishAction = USB_ACT_DONE;
}

/*
    Processing of GetConfig request, S-I-O command.

    @return void
*/
static void ReqGetConfig(void)
{
	UINT8   pTmp[2];
	UINT32  DMALen;

	// If the device isn't configured, returned value is zero
	if (bUSBflag_configuration == 1) {
		pTmp[0] = 0x01;
	} else {
		pTmp[0] = 0x00;
	}

	DMALen = 1;
	usb2dev_write_endpoint(USB_EP0, pTmp, &DMALen);

	gUSBCxFinishAction = USB_ACT_DONE;
}

/*
    Processing of SetConfig request, S-I command.

    @return void
*/
static void ReqSetConfig(void)
{
	UINT8               ConfigValue;
	USB_DEVICE_REQUEST  *pDevReq;
	T_USB_DEVADDR_REG   devAddr;

	USB_PutString(("ReqSetConfig \r\n"));

	pDevReq     = (USB_DEVICE_REQUEST *)&usb2dev_control_data.device_request;
	ConfigValue = pDevReq->w_value;

	if (ConfigValue == 0) {
		bUSBflag_configuration = 0;

		// clear Config
		devAddr.reg = USB_GETREG(USB_DEVADDR_REG_OFS);
		devAddr.bit.AFT_CONF = 0;
		USB_SETREG(USB_DEVADDR_REG_OFS, devAddr.reg);

		gUSBCxFinishAction = USB_ACT_DONE;
	} else if (ConfigValue == 1) { //interface number : the "1"
		bUSBflag_configuration = 1;

		// Clear all EPs fifo mapping.
		usb_clrEPFIFOMap();

		// Set New EPs fifo mapping
		usb_FIFO_EPxCfg();

		if (usb2dev_is_highspeed_enabled()) {
			/*high speed*/
			USB_SETREG(USB_DEVSOFTIMERMASK_REG_OFS, 0x44c);
		} else {
			/*full speed*/
			USB_SETREG(USB_DEVSOFTIMERMASK_REG_OFS, 0x2710);
		}

		// set after config
		devAddr.reg = USB_GETREG(USB_DEVADDR_REG_OFS);
		devAddr.bit.AFT_CONF = 1;
		USB_SETREG(USB_DEVADDR_REG_OFS, devAddr.reg);

		usb_clrAllEP();

		//open needed fifo interrupt for the specific class
		if (gUSBManage.fp_open_needed_fifo) {
			(gUSBManage.fp_open_needed_fifo)();
		}

		if (gUSBManage.fp_event_callback) {
			gUSBManage.fp_event_callback(USB_EVENT_SET_CONFIG);
		}

		if ((gUsbChargingCB != NULL) && (gChargerSts == USB_CHARGER_STS_NONE)) {
			USB_CHARGE_EVENT chargeEvent = USB_CHARGE_EVENT_CONNECT;

			if (!(gUSBManage.p_config_desc->bm_attributes & USB_DEVDESC_ATB_SELF_POWER)) {
				// Only bus-power can draw up to 500mA after configured
				chargeEvent = USB_CHARGE_EVENT_CONFIGURED;
			}

			gUsbChargingCB(chargeEvent);
		}

		gUSBCxFinishAction = USB_ACT_DONE;
	}
}

/*
    Processing of GetInterface request, S-I-O command.

    @return void
*/
static void ReqGetInterface(void)
{
	T_USB_DEVADDR_REG devAddr;
	UINT8 pTmp[2];
	UINT32 DMALen;

	devAddr.reg = USB_GETREG(USB_DEVADDR_REG_OFS);

	if (devAddr.bit.AFT_CONF == 0) {
		//we don't check w_index... It should be zero!!
		gUSBCxFinishAction = USB_ACT_STALL;
		return;
	}

	//since we have no alternating setting, we reponse "value =0"
	// Add for more than one interface.
	pTmp[0]     = g_Alternating;
	DMALen      = 1;
	usb2dev_write_endpoint(USB_EP0, pTmp, &DMALen);

	gUSBCxFinishAction = USB_ACT_DONE;
}

/*
    Processing of SetInterface request, S-I command.

    @return void
*/
static void ReqSetInterface(void)
{
	USB_DEVICE_REQUEST  *pDevReq;

	pDevReq         = (USB_DEVICE_REQUEST *)&usb2dev_control_data.device_request;
	g_Alternating   = pDevReq->w_value & 0xFF;
	g_Interface     = pDevReq->w_index;

	// Add for more than one interface.
	if (g_Interface <= 7) {
		//usb_clrEPFIFOMap();
		//usb_FIFO_EPxCfg();
		//usb_clrAllEP();

		if (gUsbSetInterfaceHdl != NULL) {
			USB_SETINTERFACE_EVENT event;

			switch (g_Interface) {
			case 0: {
					event = USB_SETINTERFACE_EVENT_INT00_ALT00;
				}
				break;
			case 1: {
					event = USB_SETINTERFACE_EVENT_INT01_ALT00;
				}
				break;
			case 2: {
					event = USB_SETINTERFACE_EVENT_INT02_ALT00;
				}
				break;
			case 3: {
					event = USB_SETINTERFACE_EVENT_INT03_ALT00;
				}
				break;
			case 4: {
					event = USB_SETINTERFACE_EVENT_INT04_ALT00;
				}
				break;
			case 5: {
					event = USB_SETINTERFACE_EVENT_INT05_ALT00;
				}
				break;
			case 6: {
					event = USB_SETINTERFACE_EVENT_INT06_ALT00;
				}
				break;
			case 7: {
					event = USB_SETINTERFACE_EVENT_INT07_ALT00;
				}
				break;
			default: {
					gUSBCxFinishAction = USB_ACT_STALL;
				}
				return;
			}

			if (g_Alternating <= 3) {
				event |= g_Alternating;
			} else {
				gUSBCxFinishAction = USB_ACT_STALL;
				return;
			}

			gUsbSetInterfaceHdl(event);
		}

		gUSBCxFinishAction = USB_ACT_DONE;
	} else {
		gUSBCxFinishAction = USB_ACT_STALL;
	}

}

/*
    Processing of SyncFrame request

    If this EP is an Iso EP, return 2 bytes frame number.
    Else stall this command.

    @return void
*/
static void ReqSynchFrame(void)
{
	USB_FIFO_NUM            FIFOn;
	USB_EP_TYPE             epType = EP_TYPE_NOT_USE;
	USB_DEVICE_REQUEST      *pDevReq;
	T_USB_DEVFIFOCFG_REG    FIFOCfgReg;

	pDevReq = (USB_DEVICE_REQUEST *)&usb2dev_control_data.device_request;

	switch (USB_EP_ADDRESS_MASK & pDevReq->w_index) {
	case USB_EP1:
	case USB_EP2:
	case USB_EP3:
	case USB_EP4:
	case USB_EP5:
	case USB_EP6:
	case USB_EP7:
	case USB_EP8:
	case USB_EP9:
	case USB_EP10:
	case USB_EP11:
	case USB_EP12:
	case USB_EP13:
	case USB_EP14:
	case USB_EP15:
		{
			FIFOn           = usb_getFifoFromEp(USB_EP_ADDRESS_MASK & pDevReq->w_index);
			FIFOCfgReg.reg  = USB_GETREG(USB_DEVFIFOCFG_REG_OFS + ((FIFOn >> 2) << 2));
			epType          = (FIFOCfgReg.reg >> ((FIFOn & 0x3) << 3)) & 0x3;

			if (epType == EP_TYPE_ISOCHRONOUS) {
				UINT8               pTmp[2];
				UINT32              DMALen;
				T_USB_DEVSOFNUM_REG sofNum;

				sofNum.reg = USB_GETREG(USB_DEVSOFNUM_REG_OFS);

				pTmp[0] =  sofNum.reg & 0xFF;
				pTmp[1] = (sofNum.reg >> 8) & 0x7;

				DMALen = 2;
				usb2dev_write_endpoint(USB_EP0, pTmp, &DMALen);
				gUSBCxFinishAction = USB_ACT_DONE;
			} else {
				gUSBCxFinishAction = USB_ACT_STALL;
			}
		}
		break;
	default:
		gUSBCxFinishAction = USB_ACT_STALL;
		break;
	}
}


/*
    Process Reserved Request.

    @return void
*/
void ReqReserved(void)
{
	gUSBCxFinishAction = USB_ACT_STALL;
}
#endif
#if 1
/*
    Clear all endpoint and FIFO mapping.

    @return void
*/
static void usb_clrEPFIFOMap(void)
{
	// Clear All EPx & FIFOx map register
	USB_SETREG(USB_DEVMAXPS_INEP1_REG_OFS,    0x200);
	USB_SETREG(USB_DEVMAXPS_INEP2_REG_OFS,    0x200);
	USB_SETREG(USB_DEVMAXPS_INEP3_REG_OFS,    0x200);
	USB_SETREG(USB_DEVMAXPS_INEP4_REG_OFS,    0x200);
	USB_SETREG(USB_DEVMAXPS_INEP5_REG_OFS,    0x200);
	USB_SETREG(USB_DEVMAXPS_INEP6_REG_OFS,    0x200);
	USB_SETREG(USB_DEVMAXPS_INEP7_REG_OFS,    0x200);
	USB_SETREG(USB_DEVMAXPS_INEP8_REG_OFS,    0x200);
	USB_SETREG(USB_DEVMAXPS_INEP9_REG_OFS,    0x200);
	USB_SETREG(USB_DEVMAXPS_INEP10_REG_OFS,   0x200);
	USB_SETREG(USB_DEVMAXPS_INEP11_REG_OFS,   0x200);
	USB_SETREG(USB_DEVMAXPS_INEP12_REG_OFS,   0x200);
	USB_SETREG(USB_DEVMAXPS_INEP13_REG_OFS,   0x200);
	USB_SETREG(USB_DEVMAXPS_INEP14_REG_OFS,   0x200);
	USB_SETREG(USB_DEVMAXPS_INEP15_REG_OFS,   0x200);
	USB_SETREG(USB_DEVMAXPS_OUTEP1_REG_OFS,   0x200);
	USB_SETREG(USB_DEVMAXPS_OUTEP2_REG_OFS,   0x200);
	USB_SETREG(USB_DEVMAXPS_OUTEP3_REG_OFS,   0x200);
	USB_SETREG(USB_DEVMAXPS_OUTEP4_REG_OFS,   0x200);
	USB_SETREG(USB_DEVMAXPS_OUTEP5_REG_OFS,   0x200);
	USB_SETREG(USB_DEVMAXPS_OUTEP6_REG_OFS,   0x200);
	USB_SETREG(USB_DEVMAXPS_OUTEP7_REG_OFS,   0x200);
	USB_SETREG(USB_DEVMAXPS_OUTEP8_REG_OFS,   0x200);
	USB_SETREG(USB_DEVMAXPS_OUTEP9_REG_OFS,   0x200);
	USB_SETREG(USB_DEVMAXPS_OUTEP10_REG_OFS,  0x200);
	USB_SETREG(USB_DEVMAXPS_OUTEP11_REG_OFS,  0x200);
	USB_SETREG(USB_DEVMAXPS_OUTEP12_REG_OFS,  0x200);
	USB_SETREG(USB_DEVMAXPS_OUTEP13_REG_OFS,  0x200);
	USB_SETREG(USB_DEVMAXPS_OUTEP14_REG_OFS,  0x200);
	USB_SETREG(USB_DEVMAXPS_OUTEP15_REG_OFS,  0x200);

	USB_SETREG(USB_DEVEPMAP1_4_REG_OFS,		  0xFFFFFFFF);
	USB_SETREG(USB_DEVEPMAP5_8_REG_OFS,	      0xFFFFFFFF);
	USB_SETREG(USB_DEVEPMAP9_12_REG_OFS,	  0xFFFFFFFF);
	USB_SETREG(USB_DEVEPMAP13_15_REG_OFS,	  0x00FFFFFF);

	USB_SETREG(USB_DEVFIFOMAP_REG_OFS,        0x0F0F0F0F);
	USB_SETREG(USB_DEVFIFOCFG_REG_OFS,	      0x00000000);
	USB_SETREG(USB_DEVFIFOMAP1_REG_OFS,       0x0F0F0F0F);
	USB_SETREG(USB_DEVFIFOCFG1_REG_OFS,	      0x00000000);
	USB_SETREG(USB_DEVFIFOMAP2_REG_OFS,       0x0F0F0F0F);
	USB_SETREG(USB_DEVFIFOCFG2_REG_OFS,	      0x00000000);
	USB_SETREG(USB_DEVFIFOMAP3_REG_OFS,       0x0F0F0F0F);
	USB_SETREG(USB_DEVFIFOCFG3_REG_OFS,	      0x00000000);

}


/*
    Clear all endpoint toggle bit and reset

    @return void
*/
static void usb_clrAllEP(void)
{
	UINT32                      EPn;
	T_USB_DEVMAXPS_INEP_REG     devEPMaxPS;
	T_USB_DEVMAXPS_OUTEP_REG	devEPMaxPSOut;

	// Clear All EPx Toggle bit
	for (EPn = USB_EP1; EPn <= USB_EP8; EPn++) {
		// Reset Toggle sequence for IN EP
		devEPMaxPS.reg = USB_GETREG(USB_DEVMAXPS_INEP1_REG_OFS + ((EPn - USB_EP1) << 2));
		devEPMaxPS.bit.RSTG_IEP = 1;
		USB_SETREG((USB_DEVMAXPS_INEP1_REG_OFS + ((EPn - USB_EP1) << 2)), devEPMaxPS.reg);
		devEPMaxPS.bit.RSTG_IEP = 0;
		USB_SETREG((USB_DEVMAXPS_INEP1_REG_OFS + ((EPn - USB_EP1) << 2)), devEPMaxPS.reg);


		devEPMaxPSOut.reg = USB_GETREG(USB_DEVMAXPS_OUTEP1_REG_OFS + ((EPn - USB_EP1) << 2));
		devEPMaxPSOut.bit.RSTG_OEP = 1;
		USB_SETREG((USB_DEVMAXPS_OUTEP1_REG_OFS + ((EPn - USB_EP1) << 2)), devEPMaxPSOut.reg);
		devEPMaxPSOut.bit.RSTG_OEP = 0;
		USB_SETREG((USB_DEVMAXPS_OUTEP1_REG_OFS + ((EPn - USB_EP1) << 2)), devEPMaxPSOut.reg);

	}

	for (EPn = USB_EP9; EPn <= USB_EP15; EPn++) {
		// Reset Toggle sequence for IN EP
		devEPMaxPS.reg = USB_GETREG(USB_DEVMAXPS_INEP9_REG_OFS + ((EPn - USB_EP9) << 2));
		devEPMaxPS.bit.RSTG_IEP = 1;
		USB_SETREG((USB_DEVMAXPS_INEP9_REG_OFS + ((EPn - USB_EP9) << 2)), devEPMaxPS.reg);
		devEPMaxPS.bit.RSTG_IEP = 0;
		USB_SETREG((USB_DEVMAXPS_INEP9_REG_OFS + ((EPn - USB_EP9) << 2)), devEPMaxPS.reg);


		devEPMaxPSOut.reg = USB_GETREG(USB_DEVMAXPS_OUTEP9_REG_OFS + ((EPn - USB_EP9) << 2));
		devEPMaxPSOut.bit.RSTG_OEP = 1;
		USB_SETREG((USB_DEVMAXPS_OUTEP9_REG_OFS + ((EPn - USB_EP9) << 2)), devEPMaxPSOut.reg);
		devEPMaxPSOut.bit.RSTG_OEP = 0;
		USB_SETREG((USB_DEVMAXPS_OUTEP9_REG_OFS + ((EPn - USB_EP9) << 2)), devEPMaxPSOut.reg);

	}


}


/*
    Get endpoint stall status.

    @param[in] EPn : endpoint number (USB_EP1~USB_EP15)

    @return
        - @b TRUE: endpoint is stall
        - @b FALSE: endpoint is not stall
*/
BOOL usb_EPStallStatus(USB_EP EPn)
{
	T_USB_DEVMAXPS_INEP_REG   devEPMaxPS;
	T_USB_DEVMAXPS_OUTEP_REG   devEPMaxPSOut;

	if(EPn <= USB_EP8) {
		devEPMaxPS.reg    = USB_GETREG(USB_DEVMAXPS_INEP1_REG_OFS + ((EPn - USB_EP1) << 2));
		devEPMaxPSOut.reg = USB_GETREG(USB_DEVMAXPS_OUTEP1_REG_OFS + ((EPn - USB_EP1) << 2));
	} else {
		devEPMaxPS.reg    = USB_GETREG(USB_DEVMAXPS_INEP9_REG_OFS + ((EPn - USB_EP9) << 2));
		devEPMaxPSOut.reg = USB_GETREG(USB_DEVMAXPS_OUTEP9_REG_OFS + ((EPn - USB_EP9) << 2));
	}

	return devEPMaxPS.bit.STL_IEP || devEPMaxPSOut.bit.STL_OEP;
}


/*
    Check if FIFO is enabled

    @param[in] FIFOn : FIFO number (0~3)
    @return
        - @b TRUE: FIFO is enabled
        - @b FALSE: FIFO is disabled
*/
static BOOL usb_isFIFOEnabled(UINT32 FIFOn)
{
	T_USB_DEVFIFOCFG_REG FIFOCfgReg;
	T_USB_DEVFIFOCFG1_REG FIFOCfg1Reg;
	T_USB_DEVFIFOCFG2_REG FIFOCfg2Reg;
	T_USB_DEVFIFOCFG3_REG FIFOCfg3Reg;

	FIFOCfgReg.reg = USB_GETREG(USB_DEVFIFOCFG_REG_OFS);
	FIFOCfg1Reg.reg = USB_GETREG(USB_DEVFIFOCFG1_REG_OFS);
	FIFOCfg2Reg.reg = USB_GETREG(USB_DEVFIFOCFG2_REG_OFS);
	FIFOCfg3Reg.reg = USB_GETREG(USB_DEVFIFOCFG3_REG_OFS);

	switch (FIFOn) {
	case 0:
		return (BOOL)(FIFOCfgReg.bit.EN_F0);
	case 1:
		return (BOOL)(FIFOCfgReg.bit.EN_F1);
	case 2:
		return (BOOL)(FIFOCfgReg.bit.EN_F2);
	case 3:
		return (BOOL)(FIFOCfgReg.bit.EN_F3);
	case 4:
		return (BOOL)(FIFOCfg1Reg.bit.EN_F4);
	case 5:
		return (BOOL)(FIFOCfg1Reg.bit.EN_F5);
	case 6:
		return (BOOL)(FIFOCfg1Reg.bit.EN_F6);
	case 7:
		return (BOOL)(FIFOCfg1Reg.bit.EN_F7);
	case 8:
		return (BOOL)(FIFOCfg2Reg.bit.EN_F8);
	case 9:
		return (BOOL)(FIFOCfg2Reg.bit.EN_F9);
	case 10:
		return (BOOL)(FIFOCfg2Reg.bit.EN_F10);
	case 11:
		return (BOOL)(FIFOCfg2Reg.bit.EN_F11);
	case 12:
		return (BOOL)(FIFOCfg3Reg.bit.EN_F12);
	case 13:
		return (BOOL)(FIFOCfg3Reg.bit.EN_F13);
	case 14:
		return (BOOL)(FIFOCfg3Reg.bit.EN_F14);
	case 15:
		return (BOOL)(FIFOCfg3Reg.bit.EN_F15);
	default:
		USB_PutString(" FIFO num parameter error ! \r\n");
	}
	return FALSE;
}

/*
    Clear IN endpoint stall

    @param[in] EPn : endpoint number (USB_EP1~USB_EP4)
    @return void
*/
static void usb_clrEPStall(USB_EP EPn)
{
	T_USB_DEVMAXPS_INEP_REG   devEPMaxPS;
	T_USB_DEVMAXPS_OUTEP_REG   devEPMaxPSOut;

	if (EPn <= USB_EP8) {
		if (usb_getEpDirection(EPn) == EP_DIR_IN) {
			devEPMaxPS.reg = USB_GETREG(USB_DEVMAXPS_INEP1_REG_OFS + ((EPn - USB_EP1) << 2));
			devEPMaxPS.bit.STL_IEP = 0;
			USB_SETREG((USB_DEVMAXPS_INEP1_REG_OFS + ((EPn - USB_EP1) << 2)), devEPMaxPS.reg);
		} else if (usb_getEpDirection(EPn) == EP_DIR_OUT){
			devEPMaxPSOut.reg = USB_GETREG(USB_DEVMAXPS_OUTEP1_REG_OFS + ((EPn - USB_EP1) << 2));
			devEPMaxPSOut.bit.STL_OEP = 0;
			USB_SETREG((USB_DEVMAXPS_OUTEP1_REG_OFS + ((EPn - USB_EP1) << 2)), devEPMaxPSOut.reg);
		}
	} else if (EPn <= USB_EP15) {
		if (usb_getEpDirection(EPn) == EP_DIR_IN) {
			devEPMaxPS.reg = USB_GETREG(USB_DEVMAXPS_INEP9_REG_OFS + ((EPn - USB_EP9) << 2));
			devEPMaxPS.bit.STL_IEP = 0;
			USB_SETREG((USB_DEVMAXPS_INEP9_REG_OFS + ((EPn - USB_EP9) << 2)), devEPMaxPS.reg);
		} else if (usb_getEpDirection(EPn) == EP_DIR_OUT){
			devEPMaxPSOut.reg = USB_GETREG(USB_DEVMAXPS_OUTEP9_REG_OFS + ((EPn - USB_EP9) << 2));
			devEPMaxPSOut.bit.STL_OEP = 0;
			USB_SETREG((USB_DEVMAXPS_OUTEP9_REG_OFS + ((EPn - USB_EP9) << 2)), devEPMaxPSOut.reg);
		}
	}
}

/*
    Reset IN endpoint data toggle sequence

    @param[in] EPn : endpoint number (USB_EP1~USB_EP4)
    @return void
*/
static void usb_rstEPDataToggle(USB_EP EPn)
{
	T_USB_DEVMAXPS_INEP_REG   devEPMaxPS;
	T_USB_DEVMAXPS_OUTEP_REG   devEPMaxPSOut;

	if (EPn <= USB_EP8) {
		devEPMaxPS.reg = USB_GETREG(USB_DEVMAXPS_INEP1_REG_OFS + ((EPn - USB_EP1) << 2));
		devEPMaxPS.bit.RSTG_IEP = 1;
		USB_SETREG((USB_DEVMAXPS_INEP1_REG_OFS + ((EPn - USB_EP1) << 2)), devEPMaxPS.reg);
		devEPMaxPS.bit.RSTG_IEP = 0;
		USB_SETREG((USB_DEVMAXPS_INEP1_REG_OFS + ((EPn - USB_EP1) << 2)), devEPMaxPS.reg);

		devEPMaxPSOut.reg = USB_GETREG(USB_DEVMAXPS_OUTEP1_REG_OFS + ((EPn - USB_EP1) << 2));
		devEPMaxPSOut.bit.RSTG_OEP = 1;
		USB_SETREG((USB_DEVMAXPS_OUTEP1_REG_OFS + ((EPn - USB_EP1) << 2)), devEPMaxPSOut.reg);
		devEPMaxPSOut.bit.RSTG_OEP = 0;
		USB_SETREG((USB_DEVMAXPS_OUTEP1_REG_OFS + ((EPn - USB_EP1) << 2)), devEPMaxPSOut.reg);
	} else {
		devEPMaxPS.reg = USB_GETREG(USB_DEVMAXPS_INEP9_REG_OFS + ((EPn - USB_EP9) << 2));
		devEPMaxPS.bit.RSTG_IEP = 1;
		USB_SETREG((USB_DEVMAXPS_INEP9_REG_OFS + ((EPn - USB_EP9) << 2)), devEPMaxPS.reg);
		devEPMaxPS.bit.RSTG_IEP = 0;
		USB_SETREG((USB_DEVMAXPS_INEP9_REG_OFS + ((EPn - USB_EP9) << 2)), devEPMaxPS.reg);

		devEPMaxPSOut.reg = USB_GETREG(USB_DEVMAXPS_OUTEP9_REG_OFS + ((EPn - USB_EP9) << 2));
		devEPMaxPSOut.bit.RSTG_OEP = 1;
		USB_SETREG((USB_DEVMAXPS_OUTEP9_REG_OFS + ((EPn - USB_EP9) << 2)), devEPMaxPSOut.reg);
		devEPMaxPSOut.bit.RSTG_OEP = 0;
		USB_SETREG((USB_DEVMAXPS_OUTEP9_REG_OFS + ((EPn - USB_EP9) << 2)), devEPMaxPSOut.reg);
	}
}
#endif
//@}

