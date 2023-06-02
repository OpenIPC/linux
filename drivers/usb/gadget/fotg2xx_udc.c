/*******************************************************************************
 * Module name: fotg2xx_udc.c
 *
 * Copyright 2006 GM as an unpublished work.
 * All Rights Reserved.
 *
 * The Information contained herein is confidential property of Company.
 * The user, copying, transfer or disclosure of such Information is
 * prohibited except by express written agreement with Company.
 *
 * Written on 2006/6 by Elliot Hou Au-ping.
 *
 * Module Description:
 *  This OTG UDC dirver for GM FOTG220 controller
 *
 ******************************************************************************/
u8 cx_in_int_assert = 0, cx_out_int_assert = 0, bulk_out_int_assert = 0;

/*-------------------------------------------------------------------------*/
//*******************************************************************
// Name:Get_FIFO_Num
// Description:get the FIFO number from ep
//             FIFO0=0
//             FIFO1=1
//             FIFO2=2
//             FIFO3=3
//             FIFO-CX = 0xFF
//*******************************************************************
static u8 Get_FIFO_Num(struct FTC_ep *ep)
{
	u8 u8fifo_n;

	if (ep->num == 0) {
		u8fifo_n = FIFOCX;
		return (u8fifo_n);
	}

	u8fifo_n = mUsbEPMapRd(ep->dev->va_base,ep->num);	// get the relatived FIFO number
	if (ep->is_in)
		u8fifo_n &= 0x0F;
	else
		u8fifo_n >>= 4;

	if (u8fifo_n >= MAX_FIFO_NUM)	// over the Max. fifo count ?
		printk("??? Error ep > FUSB200_MAX_FIFO \n");

	return (u8fifo_n);
}

/*-------------------------------------------------------------------------*/
//****************************************************
// Name:CX_dma_Directly
// Description: Start DMA directly
//              <1>.For Control Command - Get Stayus (Only for ep0)
// Input:<1>.For Control Command - Get Stayus
//       <2>.status
// Output:none
//****************************************************
static int CX_dma_Directly(struct FTC_udc *dev, u8 * pu8Buffer, u32 u8Num, u8 bdir)
{
	u32 FIFO_Sel, wTemp, wDMABuffer, temp;
	u8 u8fifo_n;

	DBG_FUNCC("+CX_dma_Directly, start addr = 0x%x len=0x%x, dir =%x\n",
		  (u32) pu8Buffer, u8Num, bdir);

	//<1>.Get the FIFO Select
	u8fifo_n = 0;
	FIFO_Sel = FOTG200_DMA2CxFIFO;

	//<2>.Map the DMA Buffer
	temp = (u8Num + 15) / 16;
	temp = temp * 16;
	wDMABuffer = dma_map_single((void *)dev, pu8Buffer, temp,	//USB_EPX_BUFSIZ
				    bdir ? DMA_TO_DEVICE : DMA_FROM_DEVICE);

	//<3>.Init DMA start register
	// EP=0,1,2,3,4
	if (bdir)
		mUsbDmaConfig(dev->va_base, u8Num, DIRECTION_OUT);
	else
		mUsbDmaConfig(dev->va_base, u8Num, DIRECTION_IN);

	mUsbDMA2FIFOSel(dev->va_base,FIFO_Sel);
	mUsbDmaAddr(dev->va_base,(u32) wDMABuffer);

	//<4>.Enable the DMA
	mUsbDmaStart(dev->va_base);

	//<5>.Waiting for DMA complete
	while (1) {
		wTemp = mUsbIntSrc2Rd(dev->va_base);
		if (wTemp & BIT8) {
			mUsbIntDmaErrClr(dev->va_base);
			printk("??? Cx IN DMA error..");
			break;
		}
		if (wTemp & BIT7) {
			mUsbIntDmaFinishClr(dev->va_base);
			break;
		}
		if ((wTemp & 0x00000007) > 0) {	//If (Resume/Suspend/Reset) exit
			mUsbIntDmaFinishClr(dev->va_base);
			printk
			    ("???? Cx IN DMA stop because USB Resume/Suspend/Reset..");
			break;
		}
	}
	mUsbDMA2FIFOSel(dev->va_base,FOTG200_DMA2FIFO_Non);

	//<6>.Unmap the DMA
	dma_unmap_single((void *)dev, wDMABuffer, temp,	//USB_EPX_BUFSIZ,  //req->req.length+32,
			 bdir ? DMA_TO_DEVICE : DMA_FROM_DEVICE);
	return 0;
}

//****************************************************
// Name:start_dma
// Description: Start the DMA
//              For FOTG200-Peripheral HW:
//              <1>.Control-8 byte command => Only PIO
//              <2>.Others => Only DMA
//
// Input:<1>.ep structure point
//       <2>.status
// Output:none
//****************************************************
// return:  0 = q running, 1 = q stopped, negative = errno
static int start_dma(struct FTC_ep *ep, struct FTC_request *req)
{
	struct FTC_udc *dev = ep->dev;
	u32 FIFO_Sel = 0;
	u8 u8fifo_n;

    if ((dev->EPUseDMA != DMA_CHANEL_FREE) && (dev->EPUseDMA != ep->num))
        return 0;

	DBG_FUNCC("+start_dma, start addr = 0x%x len = 0x%x\n", req->req.dma, req->req.length);

	//<0>.
	//<1>.Get the FIFO Select
	u8fifo_n = Get_FIFO_Num(ep);
	if (u8fifo_n == FIFOCX)
		FIFO_Sel = FOTG200_DMA2CxFIFO;
	else
		FIFO_Sel = 1 << (u8fifo_n);

	//<2>.Init DMA start register
	// EP=0,1,2,3,4
	if (likely(ep->is_in)) {
		if (req->req.length == 0) {
			if ((ep->num) == 0)
				mUsbEP0DoneSet(dev->va_base);
			else
				mUsbEPinZeroSet(dev->va_base, ep->num);
            DBG_CTRLL("%s,%d: dma done(ep:%d)\n", __func__, __LINE__, ep->num);
			done(ep, req, 0);
			return 0;
        } else {
            if (ep->num==0) {
                u32 max_packet = 64;

                if (req->req.length <= max_packet)      //one packet is enough
                    ep->wDMA_Set_Length = req->req.length;
                else
                    ep->wDMA_Set_Length = max_packet;
            } else {
                // for EP 1/3, we will IN all data at one time
                ep->wDMA_Set_Length = req->req.length;
            }
            mUsbDmaConfig(dev->va_base, ep->wDMA_Set_Length, DIRECTION_IN);
        }
	} else {
		if (ep->num == 0) {
            u32 max_packet = 64;

            if (cx_out_int_assert) {
                cx_out_int_assert = 0;
            } else {
                ep->wDMA_Set_Length = 0;
                return 0;
            }

            if (req->req.length <= max_packet)
                ep->wDMA_Set_Length = req->req.length;
            else
                ep->wDMA_Set_Length = max_packet;
			mUsbDmaConfig(dev->va_base, ep->wDMA_Set_Length, DIRECTION_OUT);
		} else { //For EP1~EP4
		    unsigned int out_fifo_byte_cnt = 0, ep_max_pkt_sz = 0;

            out_fifo_byte_cnt = mUsbFIFOOutByteCount(dev->va_base, ((ep->num) - 1));
            ep_max_pkt_sz = mUsbEPMxPtSzRd(dev->va_base, ((ep->num)), DIRECTION_OUT);

			//Read the Byte Counter
			if (out_fifo_byte_cnt == 0)
				return 0;

			DBG_CTRLL(">>>start_dma ==>(mUsbFIFOOutByteCount=0x%x(ep->num=0x%x)(MaxPackSize=0x%x))\n",
				 out_fifo_byte_cnt, ep->num, ep_max_pkt_sz);

			if (out_fifo_byte_cnt < ep_max_pkt_sz)
				ep->wDMA_Set_Length = out_fifo_byte_cnt;
			else
				ep->wDMA_Set_Length = ep_max_pkt_sz;
            mUsbDmaConfig(dev->va_base, ep->wDMA_Set_Length, DIRECTION_OUT);
		}
	}

	mUsbDMA2FIFOSel(dev->va_base, FIFO_Sel);
	mUsbDmaAddr(dev->va_base, req->req.dma);
	DBG_CTRLL(">>>(mUsbDmaConfigRd=0x%x(Request Length=0x%x))\n",
		  mUsbDmaConfigRd(dev->va_base), req->req.length);
	DBG_CTRLL(">>>(mUsbDMA2FIFOSel=0x%x)\n", mUsbDMA2FIFORd(dev->va_base));
	DBG_CTRLL(">>>(mUsbDmaAddr=0x%x)\n", mUsbDmaAddrRd(dev->va_base));

	//<3>.Record who use the DMA chanel(In use)
	dev->EPUseDMA = ep->num;

	//<4>.Disable FIFO-Interrupt
	// If use DMA, no FIFO interrupt for FIFO
	//FIFO_Interrupt(ep,0);

    // fifo interrupt on/off
    if (ep->num == 0) {
        if (ep->is_in == 0)
            mUsbIntEP0OutDis(dev->va_base);
    } else {
        if (ep->is_in)
            mUsbIntFXINDis(dev->va_base,((ep->num) - 1));
        else
            mUsbIntFXOUTDis(dev->va_base,((ep->num) - 1));
    }

	//<5>.Enable the DMA Interrupt
	mUsbIntDmaErrEn(dev->va_base);
	mUsbIntDmaFinishEn(dev->va_base);

#if 0 // removed for PTP
    if ((ep->num)!=0)
        mUsbIntEP0SetupDis(dev->va_base);
#endif

	//<6>.Enable the DMA
	mUsbDmaStart(dev->va_base);

	DBG_FUNCC("-start_dma...\n");
	return 0;
}

//****************************************************
// Name:dma_advance
// Description: After finish DMA
// Input:<1>.dev structure pointer
//       <2>.ep structure pointer
// Output:none
//****************************************************
static void dma_advance(struct FTC_udc *dev, struct FTC_ep *ep)
{
	struct FTC_request *req;

	DBG_FUNCC("+dma_advance\n");

    if (unlikely(list_empty(&ep->queue))) {

stop://Disable DMA)
        mUsbDmaStop(dev->va_base);
        mUsbIntDmaErrDis(dev->va_base);
        mUsbIntDmaFinishDis(dev->va_base);
        mUsbDMA2FIFOSel(dev->va_base,FOTG200_DMA2FIFO_Non);
        dev->EPUseDMA = DMA_CHANEL_FREE;

        if (unlikely(ep->num)) {	//ep->num>0
            //Disable the FIFO-Interrupt (ep->Num>0)
            if (likely(ep->is_in))
                mUsbIntFXINDis(dev->va_base,((ep->num) - 1));
            else
                mUsbIntFXOUTDis(dev->va_base,((ep->num) - 1));
        }

        mUsbIntEP0SetupEn(dev->va_base);

        // fifo interrupt on/off
        if (ep->num == 0) {
            if (ep->is_in)
                mUsbIntEP0InEn(dev->va_base);
            else
                mUsbIntEP0OutEn(dev->va_base);
        }

        {
            int i;

            for (i = 1; i < FUSB220_CURRENT_SUPPORT_EP; i++)
            {
                struct FTC_ep *tp = &dev->ep[i];

                if (tp->num == ep->num)
                    continue;
                if (unlikely(!list_empty(&tp->queue))) {
                    if (likely(tp->is_in))
                        mUsbIntFXINEn(dev->va_base,(i - 1));
                    else
                        mUsbIntFXOUTEn(dev->va_base,(i - 1));
                }
            }
        }

        return;
    }

	req = list_entry(ep->queue.next, struct FTC_request, queue);

	//<2>.Get length
	/* normal hw dma completion (not abort) */
	if (mUsbIntDmaErrRd(dev->va_base) == 0) {
		req->req.actual += ep->wDMA_Set_Length;
		DBG_CTRLL(">>> dma_advance=>req->req.actual=0x%x  \n",
			  req->req.actual);

		// only control EP & IN will have such condition
		if (likely(ep->is_in)) {
            if (req->req.length > req->req.actual) {
                u32 max_packet;

                if (ep->num==0) {
                    if (cx_in_int_assert) {
                        cx_in_int_assert = 0;
                    } else {
                        mUsbIntEP0InEn(dev->va_base); // fifo interrupt on/off
                        ep->wDMA_Set_Length = 0; // avoid to double count when reentry after cx_in_int
                        return;
                    }
                    max_packet = 64;
                } else {
                    max_packet = mUsbEPMxPtSzRd(ep->dev->va_base, ep->num, DIRECTION_IN);
                }

                ep->wDMA_Set_Length = req->req.length - req->req.actual;
                if (ep->wDMA_Set_Length > max_packet)
                    ep->wDMA_Set_Length = max_packet;

                mUsbDmaConfig(dev->va_base,ep->wDMA_Set_Length,DIRECTION_IN);
                mUsbDmaAddr(dev->va_base,req->req.dma + req->req.actual);

               // fifo interrupt on/off
                if (ep->num == 0)
                    mUsbIntEP0InDis(dev->va_base);
                else
                    mUsbIntFXINDis(dev->va_base,((ep->num) - 1));

                mUsbIntDmaErrEn(dev->va_base);
                mUsbIntDmaFinishEn(dev->va_base);

                mUsbDmaStart(dev->va_base);
                return;
            }
		} else {
            if (ep->num == 0) {
                if (req->req.length > req->req.actual) {
                    // for control out ep and entered by cx_out_int triggered
                    u32 max_packet = 64;

                    if (cx_out_int_assert) {
                        cx_out_int_assert = 0;
                    } else {
                        mUsbIntEP0OutEn(dev->va_base); // fifo interrupt on/off
                        ep->wDMA_Set_Length = 0;
                        return;
                    }

                    ep->wDMA_Set_Length = req->req.length - req->req.actual;
                    if (ep->wDMA_Set_Length > max_packet)
                        ep->wDMA_Set_Length = max_packet;

                    mUsbDmaConfig(dev->va_base, ep->wDMA_Set_Length, DIRECTION_OUT);
                    mUsbDmaAddr(dev->va_base, req->req.dma + req->req.actual);

                    mUsbIntEP0OutDis(dev->va_base); // fifo interrupt on/off

                    mUsbIntDmaErrEn(dev->va_base);
                    mUsbIntDmaFinishEn(dev->va_base);

                    mUsbDmaStart(dev->va_base);
                    return;
                }
            } else {
                if (req->req.length > req->req.actual) {
                    unsigned int out_fifo_byte_cnt = 0, ep_max_pkt_sz = 0;

                    ep_max_pkt_sz = mUsbEPMxPtSzRd(ep->dev->va_base, ((ep->num)), DIRECTION_OUT);

                    if ((req->req.actual % ep_max_pkt_sz) == 0) {
                        if (bulk_out_int_assert) {
                            bulk_out_int_assert = 0;
                        } else {
                            mUsbIntFXOUTEn(dev->va_base,((ep->num) - 1)); // fifo interrupt on/off
                            ep->wDMA_Set_Length = 0;
                            return;
                        }

                        out_fifo_byte_cnt = mUsbFIFOOutByteCount(ep->dev->va_base, ((ep->num) - 1));

                        // continue transfer remains
                        if (out_fifo_byte_cnt < ep_max_pkt_sz)
                            ep->wDMA_Set_Length = out_fifo_byte_cnt;
                        else
                            ep->wDMA_Set_Length = ep_max_pkt_sz;

                        mUsbDmaConfig(ep->dev->va_base, ep->wDMA_Set_Length, DIRECTION_OUT);
                        mUsbDmaAddr(dev->va_base, req->req.dma + req->req.actual);

                        mUsbIntFXOUTDis(dev->va_base,((ep->num) - 1)); // fifo interrupt on/off

                        mUsbIntDmaErrEn(dev->va_base);
                        mUsbIntDmaFinishEn(dev->va_base);

                        mUsbDmaStart(dev->va_base);
                        return;
                    }
                }
            }
		}
	} else {
		printk("??? DMA Error...\n");
		req->req.actual = 0;
	}

#ifdef USB_TRACE
	VDBG(dev, "done %s %s dma, %u/%u bytes, req %p\n",
	     ep->ep.name, ep->is_in ? "IN" : "OUT",
	     req->req.actual, req->req.length, req);
#endif

	//<3>.Done the request
    DBG_CTRLL("%s,%d: dma done(ep:%d)\n", __func__, __LINE__, ep->num);
	done(ep, req, 0);

	if (list_empty(&ep->queue))
		goto stop;

	//<4>.Start another req DMA
	if (ep->num == 0) { // under test, nerver happened
		req = list_entry(ep->queue.next, struct FTC_request, queue);
		(void)start_dma(ep, req);
	} else {
		//<1>.Free the DMA resource => Waiting for next DMA-Start
		mUsbDmaStop(dev->va_base);
		mUsbIntDmaErrDis(dev->va_base);
		mUsbIntDmaFinishDis(dev->va_base);
		mUsbDMA2FIFOSel(dev->va_base,FOTG200_DMA2FIFO_Non);
		dev->EPUseDMA = DMA_CHANEL_FREE;

		//<2>.open the interrupt
		if (likely(ep->is_in))
			mUsbIntFXINEn(dev->va_base,((ep->num) - 1));	//Enable the Bulk-In
		else
			mUsbIntFXOUTEn(dev->va_base,((ep->num) - 1));	//Enable the Bulk-Out
        mUsbIntEP0SetupEn(dev->va_base);

        if ((ep->num == 1) || (ep->num == 2)) {
            int i;

            for (i = 3; i < FUSB220_CURRENT_SUPPORT_EP; i++) {
                struct FTC_ep *tp = &dev->ep[i];

                if (unlikely(!list_empty(&tp->queue))) {
                    if (likely(ep->is_in))
                        mUsbIntFXINDis(dev->va_base,((ep->num) - 1));    //Disable the Bulk-In
                    else
                        mUsbIntFXOUTDis(dev->va_base,((ep->num) - 1));   //Disable the Bulk-Out

                    if (likely(tp->is_in))
                        mUsbIntFXINEn(dev->va_base,(i - 1));    //Enable the Interrupt-In
                    else
                        mUsbIntFXOUTEn(dev->va_base,(i - 1));   //Enable the Interrupt-Out
                }
            }
        }
	}
}

//****************************************************
// Name:FTC_queue
// Description:
// Input:<1>.ep structure point
//       <2>.status
//       <3>.flag
// Output:none
//****************************************************
static int FTC_queue(struct usb_ep *_ep, struct usb_request *_req, unsigned gfp_flags)
{
	struct FTC_request *req;
	struct FTC_ep *ep;
	struct FTC_udc *dev;
	unsigned long flags;
	int status;
	u32 temp;

	DBG_FUNCC("+FTC_queue()\n");

	//<1>.Check request & ep & dev

	/* always require a cpu-view buffer so pio works */
	req = container_of(_req, struct FTC_request, req);
	if (unlikely(!_req || !_req->complete
		     || !_req->buf || !list_empty(&req->queue))) {
		printk("??? FTC_queue => return -EINVAL\n");
		return -EINVAL;
	}

	ep = container_of(_ep, struct FTC_ep, ep);
	if (unlikely(!_ep || (!ep->desc && ep->num != 0))) {
		printk("??? FTC_queue => return -EINVAL\n");
		return -EINVAL;
	}

	dev = ep->dev;
	if (unlikely(!dev->driver || dev->gadget.speed == USB_SPEED_UNKNOWN)) {
		printk("??? FTC_queue => return -ESHUTDOWN\n");
		return -ESHUTDOWN;
	}

	//Check CX 0 bytes
	if (req->req.length == 0) {
		if (ep->num == 0) {	//request Control Transfer 0 byte
			mUsbEP0DoneSet(dev->va_base);
			mUsbCfgSet(dev->va_base);//Temp Solution for Set Configuration
			DBG_CTRLL(">>> FTC_queue => return (set configuration)\n");
			return 0;
		}
		//else => Other ED 0 bytes
	}

	/* can't touch registers when suspended */
	if (dev->ep0state == EP0_SUSPEND) {
		printk("??? FTC_queue => return -EBUSY\n");
		return -EBUSY;
	}

	if (_req->dma == DMA_ADDR_INVALID) {
		DBG_CTRLL("....dma_map_single len = %x, dev = %x, buf = %x\n",
			  _req->length, dev, _req->buf);

		//important : DMA length will set as 16*n bytes
		temp = _req->length / 16;
		if (_req->length % 16)
			temp++;
		temp = temp * 16;
		_req->dma = dma_map_single((void *)dev, _req->buf, temp,	//USB_EPX_BUFSIZ,
					   ep->is_in ? DMA_TO_DEVICE :
					   DMA_FROM_DEVICE);
		req->mapped = 1;
	}
#ifdef USB_TRACE
	VDBG(dev, "%s queue req %p, len %u buf %p\n",
	     _ep->name, _req, _req->length, _req->buf);
#endif

	//<2>.Set the req's status ...
	spin_lock_irqsave(&dev->lock, flags);

	_req->status = -EINPROGRESS;
	_req->actual = 0;

	//<3>.For control-in => Fource short packet

	/* for ep0 IN without premature status, zlp is required and
	 * writing EOP starts the status stage (OUT).
	 */
	if (unlikely(ep->num == 0 && ep->is_in))
		_req->zero = 1;

	/* kickstart this i/o queue? */
	status = 0;

	//In  => Write data to the FIFO directly
	//Out => Only Enable the FIFO-Read interrupt
	if (list_empty(&ep->queue) && likely(!ep->stopped)) {
		//ep->num>0 ==> will add to queue until ed-FIFO-Interrupt be issue
		/* dma:  done after dma completion IRQ (or error)
		 * pio:  FOTG200 do not support
		 */
		DBG_CTRLL(">>> dev->EPUseDMA = 0x%x (ep->num=%x)\n",
			  dev->EPUseDMA, ep->num);

		if ((ep->num == 0) && (ep->is_in == 1))
			status = start_dma(ep, req);

		if (unlikely(status != 0)) {
			if (status > 0)
				status = 0;
			req = 0;
		}
	}
	/* else pio or dma irq handler advances the queue. */
	//Add request to queue
	if (likely(req != 0)) {
        /*
         * before add request to ep1-queue, check queue state and dma status
         */
        if ((ep->num == 1) && !list_empty(&ep->queue) && (dev->EPUseDMA == ep->num)) {
            while (mUsbDmaConfigRd(dev->va_base) & BIT0)
                ;
        }

		DBG_CTRLL(">>> add request to ep(%d) queue(0x%p) ...\n", ep->num, &ep->queue);
		list_add_tail(&req->queue, &ep->queue);
		if ((ep->num == 0) && (ep->is_in == 0) && (mUsbIntSrc0MaskRd(dev->va_base) & BIT2))
            mUsbIntEP0OutEn(dev->va_base);
	}
	//Enable the FIFO Interrupt
	if (likely((ep->num) > 0)) {	//Open the FIFO interrupt
        if (list_empty(&(dev->ep[0].queue))) {
    		if (likely((ep->is_in) == 1))	//For In-Ep
    			mUsbIntFXINEn(dev->va_base,((ep->num) - 1));
    		else		//For Out-Ep
    			mUsbIntFXOUTEn(dev->va_base,((ep->num) - 1));

     		DBG_CTRLL(">>> Enable EP-%x Interrupt (Register=0x%x)(Length=0x%x)...\n",
    		     ep->num, mUsbIntSrc1MaskRd(dev->va_base), req->req.length);
        }
	}

	spin_unlock_irqrestore(&dev->lock, flags);
	return status;
}

//****************************************************
// Name:ep_isr
// Description: For Ep-1 In
//             <1>.if queue have data start dma
//
//
// Input:dev
// Output:none
//****************************************************
void ep_isr(struct FTC_udc *dev, u8 epNum)
{
	struct FTC_request *req;
	struct FTC_ep *ep;

	DBG_FUNCC("+ep_isr(epNum=0x%x)\n", epNum);

    if ((dev->EPUseDMA != DMA_CHANEL_FREE) && (dev->EPUseDMA != epNum)) {
		if (likely(dev->ep[epNum].is_in))
			mUsbIntFXINDis(dev->va_base,(epNum - 1));
		else
			mUsbIntFXOUTDis(dev->va_base,(epNum - 1));
		return;
	}

	//<1>.Checking data in queue ?
	ep = &(dev->ep[epNum]);

	if (list_empty(&ep->queue)) {
		if (likely(ep->is_in))
			mUsbIntFXINDis(dev->va_base,((ep->num) - 1));//Disable the Bulk--In
		else
			mUsbIntFXOUTDis(dev->va_base,((ep->num) - 1));//Disable the Bulk-Out
		printk("Error ep_isr ==> ep(%d)'s LIST(0x%p) is empty\n", epNum, &ep->queue);
	} else {		//data in queue
		if ((dev->EPUseDMA == DMA_CHANEL_FREE) || (dev->EPUseDMA == epNum)) {
			//Start the DMA
			req = list_entry(ep->queue.next, struct FTC_request, queue);
            if (req->req.actual) {
                if (ep->num == 2)
                    bulk_out_int_assert = 1;
                dma_advance(dev, ep);
            } else
			    start_dma(ep, req);
		} else {
			printk("Error ep_isr ==> LIST is full and DMA used %x\n", dev->EPUseDMA);
		}
	}
}

/////////////////////////////////////////////////////
//              clrFIFORegister(void)
//              Description:
//              input: none
//              output: none
/////////////////////////////////////////////////////
static void vUsbClrFIFORegister(struct FTC_udc *dev)
{
	u32 u8ep;

	mUsbEPMapAllClr(dev->va_base);
	mUsbFIFOMapAllClr(dev->va_base);
	mUsbFIFOConfigAllClr(dev->va_base);

	for (u8ep = 1; u8ep <= MAX_FIFO_NUM; u8ep++) {
		mUsbEPMxPtSzClr(dev->va_base, u8ep, DIRECTION_IN);
		mUsbEPMxPtSzClr(dev->va_base, u8ep, DIRECTION_OUT);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// config FIFO
//-----------------------------------------------------------------------
/////////////////////////////////////////////////////
//              vUsbFIFO_EPxCfg_HS(struct FTC_udc *dev)
//              Description:
//                      1. Configure the FIFO and EPx map
//              input: none
//              output: none
/////////////////////////////////////////////////////
static void vUsbFIFO_EPxCfg_FS(struct FTC_udc *dev)
{
	u32 i;

	DBG_FUNCC("+vUsbFIFO_EPxCfg_FS()\n");

	DBG_CTRLL("FIFO-Start:0~3\n");
	DBG_CTRLL("Dir:Out=>1 / In =>0\n");
	DBG_CTRLL("BLKSize:1=>64bytes / 2 =>128 bytes\n");
	DBG_CTRLL("MaxPackSize:Max=64 bytes\n");
	DBG_CTRLL("IFO-Use-Num:1=>Single / 2=>Double / 3=>TRIBLE\n");
	DBG_CTRLL("FULL_ED4_bTYPE:0=>Control / 1=>ISO / 2=>Bulk / 3=>Interrupt\n");

	vUsbClrFIFORegister(dev);

	//EP4
	mUsbEPMap(dev->va_base, EP4, FULL_EP4_Map);
	mUsbFIFOMap(dev->va_base, FULL_ED4_FIFO_START, FULL_EP4_FIFO_Map);
	mUsbFIFOConfig(dev->va_base, FULL_ED4_FIFO_START, FULL_EP4_FIFO_Config);

	for (i = (FULL_ED4_FIFO_START+1);i < (FULL_ED4_FIFO_START+(FULL_ED4_bBLKNO*FULL_ED4_bBLKSIZE));i++) {
		mUsbFIFOConfig(dev->va_base, i, (FULL_EP4_FIFO_Config & (~BIT7)));
	}

	mUsbEPMxPtSz(dev->va_base, EP4, FULL_ED4_bDIRECTION, (FULL_ED4_MAXPACKET & 0x7ff));
	//mUsbEPinHighBandSet(dev->va_base, EP4, FULL_ED4_bDIRECTION, FULL_ED4_MAXPACKET);
	DBG_CTRLL("EP4 Config = (FIFO-Start=0x%x) (Dir=0x%x)(BLKSize=0x%x)(MaxPackSize=0x%x)\n",
		 FULL_ED4_FIFO_START, FULL_ED4_bDIRECTION, FULL_ED4_bBLKSIZE,
		 FULL_ED4_MAXPACKET);
	DBG_CTRLL("(FIFO-Use-Num=0x%x) (Type=0x%x)\n", FULL_ED4_bBLKNO, FULL_ED4_bTYPE);
	DBG_CTRLL("Register Dump (mUsbEPMap=0x%x) (mUsbFIFOMap=0x%x)(mUsbFIFOConfig=0x%x)\n",
		 mUsbEPMap1_4Rd(dev->va_base), mUsbFIFOMapAllRd(dev->va_base), mUsbFIFOConfigAllRd(dev->va_base));

	//EP3
	mUsbEPMap(dev->va_base, EP3, FULL_EP3_Map);
	mUsbFIFOMap(dev->va_base, FULL_ED3_FIFO_START, FULL_EP3_FIFO_Map);
	mUsbFIFOConfig(dev->va_base, FULL_ED3_FIFO_START, FULL_EP3_FIFO_Config);

	for (i = FULL_ED3_FIFO_START+1;i < FULL_ED3_FIFO_START+(FULL_ED3_bBLKNO*FULL_ED3_bBLKSIZE);i++) {
		mUsbFIFOConfig(dev->va_base, i, (FULL_EP3_FIFO_Config & (~BIT7)));
	}

	mUsbEPMxPtSz(dev->va_base, EP3, FULL_ED3_bDIRECTION, (FULL_ED3_MAXPACKET & 0x7ff));
	//mUsbEPinHighBandSet(dev->va_base, EP3, FULL_ED3_bDIRECTION, FULL_ED3_MAXPACKET);
	DBG_CTRLL("EP3 Config = (FIFO-Start=0x%x) (Dir=0x%x)(BLKSize=0x%x)(MaxPackSize=0x%x)\n",
		 FULL_ED3_FIFO_START, FULL_ED3_bDIRECTION, FULL_ED3_bBLKSIZE,
		 FULL_ED3_MAXPACKET);
	DBG_CTRLL("(FIFO-Use-Num=0x%x) (Type=0x%x)\n", FULL_ED3_bBLKNO, FULL_ED3_bTYPE);
	DBG_CTRLL("Register Dump (mUsbEPMap=0x%x) (mUsbFIFOMap=0x%x)(mUsbFIFOConfig=0x%x)\n",
		 mUsbEPMap1_4Rd(dev->va_base), mUsbFIFOMapAllRd(dev->va_base), mUsbFIFOConfigAllRd(dev->va_base));

	//EP2
	mUsbEPMap(dev->va_base, EP2, FULL_EP2_Map);
	mUsbFIFOMap(dev->va_base, FULL_ED2_FIFO_START, FULL_EP2_FIFO_Map);
	mUsbFIFOConfig(dev->va_base, FULL_ED2_FIFO_START, FULL_EP2_FIFO_Config);
	for (i = FULL_ED2_FIFO_START+1;i < FULL_ED2_FIFO_START+(FULL_ED2_bBLKNO*FULL_ED2_bBLKSIZE);i++) {
		mUsbFIFOConfig(dev->va_base, i, (FULL_EP2_FIFO_Config & (~BIT7)));
	}

	mUsbEPMxPtSz(dev->va_base, EP2, FULL_ED2_bDIRECTION, (FULL_ED2_MAXPACKET & 0x7ff));
	//mUsbEPinHighBandSet(dev->va_base, EP2, FULL_ED2_bDIRECTION, FULL_ED2_MAXPACKET);
	DBG_CTRLL("EP2 Config = (FIFO-Start=0x%x) (Dir=0x%x)(BLKSize=0x%x)(MaxPackSize=0x%x)\n",
		 FULL_ED2_FIFO_START, FULL_ED2_bDIRECTION, FULL_ED2_bBLKSIZE,
		 FULL_ED2_MAXPACKET);
	DBG_CTRLL("(FIFO-Use-Num=0x%x) (Type=0x%x)\n", FULL_ED2_bBLKNO, FULL_ED2_bTYPE);
	DBG_CTRLL("Register Dump (mUsbEPMap=0x%x) (mUsbFIFOMap=0x%x)(mUsbFIFOConfig=0x%x)\n",
		 mUsbEPMap1_4Rd(dev->va_base), mUsbFIFOMapAllRd(dev->va_base), mUsbFIFOConfigAllRd(dev->va_base));

	//EP1
	mUsbEPMap(dev->va_base, EP1, FULL_EP1_Map);
	mUsbFIFOMap(dev->va_base, FULL_ED1_FIFO_START, FULL_EP1_FIFO_Map);
	mUsbFIFOConfig(dev->va_base, FULL_ED1_FIFO_START, FULL_EP1_FIFO_Config);

	for (i = FULL_ED1_FIFO_START+1;i < FULL_ED1_FIFO_START+(FULL_ED1_bBLKNO*FULL_ED1_bBLKSIZE);i++) {
		mUsbFIFOConfig(dev->va_base, i, (FULL_EP1_FIFO_Config & (~BIT7)));
	}

	mUsbEPMxPtSz(dev->va_base, EP1, FULL_ED1_bDIRECTION, (FULL_ED1_MAXPACKET & 0x7ff));
	//mUsbEPinHighBandSet(dev->va_base, EP1, FULL_ED1_bDIRECTION, FULL_ED1_MAXPACKET);
	DBG_CTRLL("EP1 Config = (FIFO-Start=0x%x) (Dir=0x%x)(BLKSize=0x%x)(MaxPackSize=0x%x)\n",
		 FULL_ED1_FIFO_START, FULL_ED1_bDIRECTION, FULL_ED1_bBLKSIZE,
		 FULL_ED1_MAXPACKET);
	DBG_CTRLL("(FIFO-Use-Num=0x%x) (Type=0x%x)\n", FULL_ED1_bBLKNO, FULL_ED1_bTYPE);
	DBG_CTRLL("Register Dump (mUsbEPMap=0x%x) (mUsbFIFOMap=0x%x)(mUsbFIFOConfig=0x%x)\n",
		 mUsbEPMap1_4Rd(dev->va_base), mUsbFIFOMapAllRd(dev->va_base), mUsbFIFOConfigAllRd(dev->va_base));
}

static void vUsbFIFO_EPxCfg_HS(struct FTC_udc *dev)
{
	u32 i;

	DBG_FUNCC("+vUsbFIFO_EPxCfg_HS()\n");

	DBG_CTRLL("FIFO-Start:0~3\n");
	DBG_CTRLL("Dir:Out=>1 / In =>0\n");
	DBG_CTRLL("BLKSize:1=>512bytes / 2 =>1024 bytes\n");
	DBG_CTRLL("MaxPackSize:Max=1023 bytes\n");
	DBG_CTRLL("IFO-Use-Num:1=>Single / 2=>Double / 3=>TRIBLE\n");
	DBG_CTRLL("FULL_ED4_bTYPE:0=>Control / 1=>ISO / 2=>Bulk / 3=>Interrupt\n");

	vUsbClrFIFORegister(dev);

	//EP4
	mUsbEPMap(dev->va_base, EP4, HIGH_EP4_Map);
	mUsbFIFOMap(dev->va_base, HIGH_ED4_FIFO_START, HIGH_EP4_FIFO_Map);
	mUsbFIFOConfig(dev->va_base, HIGH_ED4_FIFO_START, HIGH_EP4_FIFO_Config);

	for (i = HIGH_ED4_FIFO_START + 1;
	     i < HIGH_ED4_FIFO_START + (HIGH_ED4_bBLKNO * HIGH_ED4_bBLKSIZE);
	     i++) {
		mUsbFIFOConfig(dev->va_base, i, (HIGH_EP4_FIFO_Config & (~BIT7)));
	}

	mUsbEPMxPtSz(dev->va_base, EP4, HIGH_ED4_bDIRECTION, (HIGH_ED4_MAXPACKET & 0x7ff));
	//mUsbEPinHighBandSet(dev->va_base, EP4, HIGH_ED4_bDIRECTION, HIGH_ED4_MAXPACKET);
	DBG_CTRLL
	    ("EP4 Config = (FIFO-Start=0x%x) (Dir=0x%x)(BLKSize=0x%x)(MaxPackSize=0x%x)\n",
	     HIGH_ED4_FIFO_START, HIGH_ED4_bDIRECTION, HIGH_ED4_bBLKSIZE,
	     HIGH_ED4_MAXPACKET);
	DBG_CTRLL("             (FIFO-Use-Num=0x%x) (Type=0x%x)\n",
		  HIGH_ED4_bBLKNO, HIGH_ED4_bTYPE);

	//EP3
	mUsbEPMap(dev->va_base, EP3, HIGH_EP3_Map);
	mUsbFIFOMap(dev->va_base, HIGH_ED3_FIFO_START, HIGH_EP3_FIFO_Map);
	mUsbFIFOConfig(dev->va_base, HIGH_ED3_FIFO_START, HIGH_EP3_FIFO_Config);

	for (i = HIGH_ED3_FIFO_START+1;i < HIGH_ED3_FIFO_START+(HIGH_ED3_bBLKNO*HIGH_ED3_bBLKSIZE);i++) {
		mUsbFIFOConfig(dev->va_base, i, (HIGH_EP3_FIFO_Config & (~BIT7)));
	}

	mUsbEPMxPtSz(dev->va_base, EP3, HIGH_ED3_bDIRECTION, (HIGH_ED3_MAXPACKET & 0x7ff));
	//mUsbEPinHighBandSet(dev->va_base, EP3, HIGH_ED3_bDIRECTION, HIGH_ED3_MAXPACKET);
	DBG_CTRLL("EP3 Config = (FIFO-Start=0x%x) (Dir=0x%x)(BLKSize=0x%x)(MaxPackSize=0x%x)\n",
	     HIGH_ED3_FIFO_START, HIGH_ED3_bDIRECTION, HIGH_ED3_bBLKSIZE,
	     HIGH_ED3_MAXPACKET);
	DBG_CTRLL("(FIFO-Use-Num=0x%x) (Type=0x%x)\n", HIGH_ED3_bBLKNO, HIGH_ED3_bTYPE);

	//EP2
	mUsbEPMap(dev->va_base, EP2, HIGH_EP2_Map);
	mUsbFIFOMap(dev->va_base, HIGH_ED2_FIFO_START, HIGH_EP2_FIFO_Map);
	mUsbFIFOConfig(dev->va_base, HIGH_ED2_FIFO_START, HIGH_EP2_FIFO_Config);

	for (i = HIGH_ED2_FIFO_START+1;i < HIGH_ED2_FIFO_START+(HIGH_ED2_bBLKNO*HIGH_ED2_bBLKSIZE);i++) {
		mUsbFIFOConfig(dev->va_base, i, (HIGH_EP2_FIFO_Config & (~BIT7)));
	}

	mUsbEPMxPtSz(dev->va_base, EP2, HIGH_ED2_bDIRECTION, (HIGH_ED2_MAXPACKET & 0x7ff));
	//mUsbEPinHighBandSet(dev->va_base, EP2, HIGH_ED2_bDIRECTION, HIGH_ED2_MAXPACKET);
	DBG_CTRLL("EP2 Config = (FIFO-Start=0x%x) (Dir=0x%x)(BLKSize=0x%x)(MaxPackSize=0x%x)\n",
	     HIGH_ED2_FIFO_START, HIGH_ED2_bDIRECTION, HIGH_ED2_bBLKSIZE,
	     HIGH_ED2_MAXPACKET);
	DBG_CTRLL("(FIFO-Use-Num=0x%x) (Type=0x%x)\n", HIGH_ED2_bBLKNO, HIGH_ED2_bTYPE);

	//EP1
	mUsbEPMap(dev->va_base, EP1, HIGH_EP1_Map);
	mUsbFIFOMap(dev->va_base, HIGH_ED1_FIFO_START, HIGH_EP1_FIFO_Map);
	mUsbFIFOConfig(dev->va_base, HIGH_ED1_FIFO_START, HIGH_EP1_FIFO_Config);

	for (i = HIGH_ED1_FIFO_START+1;i < HIGH_ED1_FIFO_START+(HIGH_ED1_bBLKNO*HIGH_ED1_bBLKSIZE);i++) {
		mUsbFIFOConfig(dev->va_base, i, (HIGH_EP1_FIFO_Config & (~BIT7)));
	}

	mUsbEPMxPtSz(dev->va_base, EP1, HIGH_ED1_bDIRECTION, (HIGH_ED1_MAXPACKET & 0x7ff));
	//mUsbEPinHighBandSet(dev->va_base, EP1, HIGH_ED1_bDIRECTION, HIGH_ED1_MAXPACKET);
	DBG_CTRLL("EP1 Config = (FIFO-Start=0x%x) (Dir=0x%x)(BLKSize=0x%x)(MaxPackSize=0x%x)\n",
	     HIGH_ED1_FIFO_START, HIGH_ED1_bDIRECTION, HIGH_ED1_bBLKSIZE,
	     HIGH_ED1_MAXPACKET);
	DBG_CTRLL("(FIFO-Use-Num=0x%x) (Type=0x%x)\n", HIGH_ED1_bBLKNO, HIGH_ED1_bTYPE);
	DBG_CTRLL("Register Dump (mUsbEPMap=0x%x) (mUsbFIFOMap=0x%x)(mUsbFIFOConfig=0x%x)\n",
	     mUsbEPMap1_4Rd(dev->va_base), mUsbFIFOMapAllRd(dev->va_base), mUsbFIFOConfigAllRd(dev->va_base));
}

/*-------------------------------------------------------------------------*/
// GM USB initial code

///////////////////////////////////////////////////////////////////////////////
//              vFOTG200_Dev_Init()
//              Description:
//                      1. Turn on the "Global Interrupt Enable" bit of FOTG200-P
//                      2. Turn on the "Chip Enable" bit of FOTG200
//              input: none
//              output: none
///////////////////////////////////////////////////////////////////////////////
static void vFOTG200_Dev_Init(struct FTC_udc *dev)
{
	DBG_FUNCC("+vFOTG200_Dev_Init()\n");

	// suspend counter
	//mUsbIdleCnt(dev->va_base,7);

	// Soft Reset
	mUsbSoftRstSet(dev->va_base);	// All circuit change to which state after Soft Reset?
    //mUsbTstHalfSpeedEn(dev->va_base);
	//mUsbSoftRstClr(dev->va_base);

	// Clear interrupt
	mUsbIntBusRstClr(dev->va_base);
	mUsbIntSuspClr(dev->va_base);
	mUsbIntResmClr(dev->va_base);

    // Mask interrupt (mask device-idle, wakeupbyVBUS)
    wFOTGPeri_Port(dev->va_base, 0x13C) = (BIT9 | BIT10);

	// Disable all fifo interrupt
	mUsbIntFIFO0_3OUTDis(dev->va_base);
	mUsbIntFIFO0_3INDis(dev->va_base);

	// Clear all fifo
	mUsbClrAllFIFOSet(dev->va_base);	// will be cleared after one cycle.

    // Mask CX_COMEND
	mUsbIntEP0EndDis(dev->va_base);

    mUsbChipEnSet(dev->va_base);
    mUsbGlobIntEnSet(dev->va_base);
    mUsbUnPLGClr(dev->va_base);
#if defined(CONFIG_PLATFORM_GM8136)
    if (CONFIG_GM_OTG_CHOOSE == 1)
        mUsbForceFSSet(dev->va_base);
#endif
}

///////////////////////////////////////////////////////////////////////////////
//              vUsbInit(struct FTC_udc *dev)
//              Description:
//                      1. Configure the FIFO and EPx map.
//                      2. Init FOTG200-Peripheral.
//                      3. Set the usb interrupt source as edge-trigger.
//                      4. Enable Usb interrupt.
//              input: none
//              output: none
///////////////////////////////////////////////////////////////////////////////
static void vUsbInit(struct FTC_udc *dev)
{
	DBG_FUNCC("+vUsbInit()\n");

	// init variables
	dev->u16TxRxCounter = 0;
	dev->eUsbCxCommand = CMD_VOID;
	dev->u8UsbConfigValue = 0;
	dev->u8UsbInterfaceValue = 0;
	dev->u8UsbInterfaceAlternateSetting = 0;

	// init hardware
	vFOTG200_Dev_Init(dev);
}

///////////////////////////////////////////////////////////////////////////////
//              vUsb_rst(struct FTC_udc *dev)
//              Description:
//                      1. Change descriptor table (High or Full speed).
//              input: none
//              output: none
///////////////////////////////////////////////////////////////////////////////
static void vUsb_rst(struct FTC_udc *dev)
{
	DBG_FUNCC("+vUsb_rst()\n");

	Info(dev, "L%x, Bus reset\n", dev->u8LineCount++);

	mUsbIntBusRstClr(dev->va_base);
	dev->gadget.speed = USB_SPEED_UNKNOWN;
}

///////////////////////////////////////////////////////////////////////////////
//              vUsb_suspend(dev)
//              Description:
//                      1. .
//              input: none
//              output: none
///////////////////////////////////////////////////////////////////////////////
static void vUsb_suspend(struct FTC_udc *dev)
{
	DBG_FUNCC("+vUsb_suspend()\n");

	Info(dev, "L%x, Bus suspend\n", dev->u8LineCount++);
	// uP must do-over everything it should handle and do before into the suspend mode
	// Go Suspend status
	mUsbIntSuspClr(dev->va_base);
	if (dev->gadget.b_hnp_enable) {
		mUsbGlobIntDis(dev->va_base);
		wFOTGPeri_Port(dev->va_base, 0x130) |= (BIT0 | BIT1 | BIT2);
		mUsb_OTGC_Control_B_HNP_EN_Set(dev->va_base);
	}
	//john, clear FIFO for suspend to avoid extra interrupt
	mUsbFIFODone(dev->va_base, 0);
	mUsbFIFODone(dev->va_base, 1);
	mUsbFIFODone(dev->va_base, 2);
	mUsbFIFODone(dev->va_base, 3);

	//Bruce;;mUsbGoSuspend();
	dev->gadget.b_hnp_enable = 0;
	dev->ep0state = EP0_SUSPEND;

    /* because the iddig detection of otg function is disabled, we can
     * ignore the suspend state on otg protocol.
     * we want to show the real status of current speed under the path
     * /sys/devices/fotg2xx_udc/udc/fotg2xx_udc/current_speed
     * after usb cable is un-plugged.
     */
	dev->gadget.speed = USB_SPEED_UNKNOWN;

#if defined(USB_NOTIFY)
	sys_kill(pid, SIGUSR2);
#endif
}

///////////////////////////////////////////////////////////////////////////////
//              vUsb_resm(struct FTC_udc        *dev)
//              Description:
//                      1. Change descriptor table (High or Full speed).
//              input: none
//              output: none
///////////////////////////////////////////////////////////////////////////////
static void vUsb_resm(struct FTC_udc *dev)
{
	DBG_FUNCC("+vUsb_resm()\n");

	Info(dev, "L%x, Bus resume\n", dev->u8LineCount++);
	// uP must do-over everything it should handle and do before into the suspend mode
	// uP must wakeup immediately
	mUsbIntResmClr(dev->va_base);

	dev->ep0state = EP0_IDLE;
}

void vUsbClrEPx(struct FTC_udc *dev)
{
	u32 u8ep;

	DBG_FUNCC("+vUsbClrEPx()\n");

	// Clear All EPx Toggle Bit
	for (u8ep = 1; u8ep <= MAX_EP_NUM; u8ep++) {
		mUsbEPinRsTgSet(dev->va_base,u8ep);
		mUsbEPinRsTgClr(dev->va_base,u8ep);
	}

	for (u8ep = 1; u8ep <= MAX_EP_NUM; u8ep++) {
		mUsbEPoutRsTgSet(dev->va_base,u8ep);
		mUsbEPoutRsTgClr(dev->va_base,u8ep);
	}
}

///////////////////////////////////////////////////////////////////////////////
//              bGet_status(struct FTC_udc *dev)
//              Description:
//                      1. Send 2 bytes status to host.
//              input: none
//              output: TRUE or FALSE (u8)
///////////////////////////////////////////////////////////////////////////////
static u8 bGet_status(struct FTC_udc *dev, const struct usb_ctrlrequest *ctrl)
{
	u8 u8ep_n, u8fifo_n, RecipientStatusLow, RecipientStatusHigh;
	u8 u8Tmp[2];
	u8 bdir;

	DBG_FUNCC("+bGet_status()  \n");

	RecipientStatusLow = 0;
	RecipientStatusHigh = 0;
	switch ((ctrl->bRequestType) & 0x3) {	// Judge which recipient type is at first
		case 0:		// Device
			// Return 2-byte's Device status (Bit1:Remote_Wakeup, Bit0:Self_Powered) to Host
			// Notice that the programe sequence of RecipientStatus
			RecipientStatusLow = mUsbRmWkupST(dev->va_base) << 1;
			// Bit0: Self_Powered--> DescriptorTable[0x23], D6(Bit 6)
			// Now we force device return data as self power. (Andrew)
			RecipientStatusLow |= ((USB_CONFIG_ATT_SELFPOWER >> 6) & 0x01);
			break;
		case 1:		// Interface
			// Return 2-byte ZEROs Interface status to Host
			break;
		case 2:		// Endpoint
			if (ctrl->wIndex == 0x00) {
				if (dev->ep0state == EP0_STALL)
					RecipientStatusLow = TRUE;
			} else {
				u8ep_n = (u8) ctrl->wIndex & 0x7F;	// which ep will be clear
				bdir = (u8) ctrl->wIndex >> 7;	// the direction of this ep
				if (u8ep_n > MAX_EP_NUM)	// over the Max. ep count ?
					return FALSE;
				else {
					u8fifo_n = mUsbEPMapRd(dev->va_base,u8ep_n);	// get the relatived FIFO number
					if (bdir == 1)
						u8fifo_n &= 0x0F;
					else
						u8fifo_n >>= 4;
					if (u8fifo_n >= MAX_FIFO_NUM)	// over the Max. fifo count ?
						return FALSE;
					// Check the FIFO had been enable ?
					if (bdir == 1)	// IN direction ?
						RecipientStatusLow = mUsbEPinStallST(dev->va_base,u8ep_n);
					else
						RecipientStatusLow = mUsbEPoutStallST(dev->va_base,u8ep_n);
				}
			}
			break;
		default:
			return FALSE;
	}

	// return RecipientStatus;
	u8Tmp[0] = RecipientStatusLow;
	u8Tmp[1] = RecipientStatusHigh;

	//Use DMA to transfer data
	CX_dma_Directly(dev, u8Tmp, 2, 1);

	mUsbEP0DoneSet(dev->va_base);

	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//              bClear_feature(struct FTC_udc *dev)
//              Description:
//                      1. Send 2 bytes status to host.
//              input: none
//              output: TRUE or FALSE (u8)
///////////////////////////////////////////////////////////////////////////////
static u8 bClear_feature(struct FTC_udc *dev, const struct usb_ctrlrequest *ctrl)
{
	u8 u8ep_n;
	u8 u8fifo_n;
	u8 bdir;

	DBG_FUNCC("+bClear_feature()\n");

	switch (ctrl->wValue)	// FeatureSelector
	{
	case 0:		// ENDPOINT_HALE
		// Clear "Endpoint_Halt", Turn off the "STALL" bit in Endpoint Control Function Register
		if (ctrl->wIndex == 0x00)
			u8ep_n = 0;	//Sp0 ed clear feature
		else {
			u8ep_n = ctrl->wIndex & 0x7F;	// which ep will be clear
			bdir = ctrl->wIndex >> 7;	// the direction of this ep
			if (u8ep_n > MAX_EP_NUM)	// over the Max. ep count ?
				return FALSE;
			else {
				u8fifo_n = Get_FIFO_Num(&dev->ep[u8ep_n]);	// get the relatived FIFO number
				if (u8fifo_n < MAX_FIFO_NUM)
					if ((mUsbFIFOConfigRd(dev->va_base, u8fifo_n) & FIFOEnBit) == 0)//FIFO enable?
						return FALSE;
			}
		}
		FTC_clear_halt(&dev->ep[u8ep_n]);
		break;
	case 1:		// Device Remote Wakeup
		// Clear "Device_Remote_Wakeup", Turn off the"RMWKUP" bit in Main Control Register
		mUsbRmWkupClr(dev->va_base);
		break;
	case 2:		// Test Mode
		return FALSE;
	default:
		return FALSE;
	}

	mUsbEP0DoneSet(dev->va_base);
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//              bSet_feature(struct FTC_udc *dev)
//              Description:
//                      1. Send 2 bytes status to host.
//              input: none
//              output: TRUE or FALSE (u8)
///////////////////////////////////////////////////////////////////////////////
static u8 bSet_feature(struct FTC_udc *dev, const struct usb_ctrlrequest *ctrl)
{
	u8 i;
	u8 u8ep_n;
	u8 u8fifo_n;
	u8 u8Tmp[55];
	u8 *pp;
	u8 bdir;

	DBG_FUNCC("+bSet_feature()\n");

	switch (ctrl->wValue) {	// FeatureSelector
		case 0:		// ENDPOINT_HALT
			// Set "Endpoint_Halt", Turn on the "STALL" bit in Endpoint Control Function Register
			if (ctrl->wIndex == 0x00)
				FTC_set_halt(dev->gadget.ep0, PROTO_STALL);	// Return EP0_Stall
			else {
				u8ep_n = ctrl->wIndex & 0x7F;	// which ep will be clear
				bdir = ctrl->wIndex >> 7;	// the direction of this ep
				u8fifo_n = Get_FIFO_Num(&dev->ep[u8ep_n]);	// get the relatived FIFO number
				if (u8fifo_n < MAX_FIFO_NUM)	// Check the FIFO had been enable ?
					if ((mUsbFIFOConfigRd(dev->va_base,u8fifo_n) & FIFOEnBit) ==	0)
						return FALSE;

				if (bdir == 1)	// IN direction ?
					mUsbEPinStallSet(dev->va_base,u8ep_n);	// Clear Stall Bit
				else
					mUsbEPoutStallSet(dev->va_base,u8ep_n);	// Set Stall Bit
			}
			break;
		case 1:		// Device Remote Wakeup
			// Set "Device_Remote_Wakeup", Turn on the"RMWKUP" bit in Mode Register
			mUsbRmWkupSet(dev->va_base);
			break;

		case 2:		// Test Mode
			switch ((ctrl->wIndex >> 8)) {	// TestSelector
				case 0x1:	// Test_J
					mUsbTsMdWr(dev->va_base, 1 << TEST_J);
					break;
				case 0x2:	// Test_K
					mUsbTsMdWr(dev->va_base, 1 << TEST_K);
					break;
				case 0x3:	// TEST_SE0_NAK
					mUsbTsMdWr(dev->va_base, 1 << TEST_SE0_NAK);
					break;
				case 0x4:	// Test_Packet
					mUsbTsMdWr(dev->va_base, 1 << TEST_PACKET);
					mUsbEP0DoneSet(dev->va_base);// special case: follow the test sequence
					pp = u8Tmp;
					for (i = 0; i < 9; i++)	// JKJKJKJK x 9
						(*pp++) = (0x00);

					//(*pp++) = (0xAA);
					//(*pp++) = (0x00);

					for (i = 0; i < 8; i++)	// JJKKJJKK *8, 8*AA
						(*pp++) = (0xAA);

					for (i = 0; i < 8; i++)	//JJJJKKKK *8, 8*EE
						(*pp++) = (0xEE);

					(*pp++) = (0xFE);	//JJJJJJJKKKKKKK *8,
					for (i = 0; i < 11; i++)	// 11*FF
						(*pp++) = (0xFF);

					(*pp++) = (0x7F);	// JJJJJJJK * 8
					(*pp++) = (0xBF);
					(*pp++) = (0xDF);
					(*pp++) = (0xEF);
					(*pp++) = (0xF7);
					(*pp++) = (0xFB);
					(*pp++) = (0xFD);
					(*pp++) = (0xFC);
					(*pp++) = (0x7E);	// {JKKKKKKK * 10}, JK
					(*pp++) = (0xBF);
					(*pp++) = (0xDF);
					(*pp++) = (0xEF);
					(*pp++) = (0xF7);
					(*pp++) = (0xFB);
					(*pp++) = (0xFD);
					(*pp) = (0x7E);
					CX_dma_Directly(dev, u8Tmp, 53, 1);
					mUsbEP0DoneSet(dev->va_base);

					// Turn on "r_test_packet_done" bit(flag) (Bit 5)
					mUsbTsPkDoneSet(dev->va_base);
					return TRUE;
					break;
				case 0x5:	// Test_Force_Enable
					//FUSBPort[0x08] = 0x20; //Start Test_Force_Enable
					break;
				default:
					return FALSE;
			}
			break;
		case 3:		//For OTG => b_hnp_enable
			dev->gadget.b_hnp_enable = 1;
			//<1>.Set b_Bus_Request
			mUsb_OTGC_Control_B_BUS_REQ_Set(dev->va_base);

			//<2>.Set the HNP enable
			mUsb_OTGC_Control_B_HNP_EN_Set(dev->va_base);
			printk("FOTG2XX Device set HNP ... HNP:%x B_BUS_REQ:%x\n",
			mUsb_OTGC_Control_B_HNP_EN_Rd(dev->va_base),mUsb_OTGC_Control_B_BUS_REQ_Rd(dev->va_base));
			break;
		case 4:		//For OTG => b_hnp_enable
			dev->gadget.a_hnp_support = 1;
			break;
		case 5:		//For OTG => b_hnp_enable
			dev->gadget.a_alt_hnp_support = 1;
			printk(">>> Please Connect to an alternate port on the A-device for HNP...\n");
			break;
		default:
			return FALSE;
	}

	mUsbEP0DoneSet(dev->va_base);
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//              bSynch_frame(struct FTC_udc *dev)
//              Description:
//                      1. If the EP is a Iso EP, then return the 2 bytes Frame number.
//                               else stall this command
//              input: none
//              output: TRUE or FALSE
///////////////////////////////////////////////////////////////////////////////
static u8 bSynch_frame(struct FTC_udc *dev, const struct usb_ctrlrequest *ctrl)
{
	DBG_FUNCC("+bSynch_frame()  ==> add by Andrew\n");

	if ((ctrl->wIndex == 0) || (ctrl->wIndex > 4))
		return FALSE;

	// Does the Endpoint support Isochronous transfer type?
	mUsbEP0DoneSet(dev->va_base);
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//              bSet_address(struct FTC_udc *dev)
//              Description:
//                      1. Set addr to FUSB200 register.
//              input: none
//              output: TRUE or FALSE (u8)
///////////////////////////////////////////////////////////////////////////////
static u8 bSet_address(struct FTC_udc *dev, const struct usb_ctrlrequest *ctrl)
{
	DBG_FUNCC("+bSet_address() = %d\n", ctrl->wValue);

	if (ctrl->wValue >= 0x0100)
		return FALSE;
	else {
		mUsbDevAddrSet(dev->va_base,ctrl->wValue);
		mUsbEP0DoneSet(dev->va_base);
		return TRUE;
	}
}

///////////////////////////////////////////////////////////////////////////////
//              vUsb_ep0setup(struct FTC_udc *dev)
//              Description:
//                      1. Read the speed
//                      2. Read 8-byte setup packet.
//          3. Process the standard command:
//             <1>.bSet_address
//
//
//              input: none
//              output: none
///////////////////////////////////////////////////////////////////////////////
static void vUsb_ep0setup(struct FTC_udc *dev)
{
	u8 u8UsbCmd[8];
	struct usb_ctrlrequest ctrl;
	int tmp;
	u32 u32UsbCmd[2];

	DBG_FUNCC("+vUsb_ep0setup()\n");

	//<1>.Read the speed
	if (dev->gadget.speed == USB_SPEED_UNKNOWN) {
		// first ep0 command after usb reset, means we can check usb speed right now.
		if (mUsbHighSpeedST(dev->va_base))	// First we should judge HS or FS
		{
			Info(dev, "L%x, high speed mode\n", dev->u8LineCount++);
			dev->gadget.speed = USB_SPEED_HIGH;
			vUsbFIFO_EPxCfg_HS(dev);//Set the FIFO Information
		} else {
			Info(dev, "L%x, full speed mode\n", dev->u8LineCount++);
			dev->gadget.speed = USB_SPEED_FULL;
			vUsbFIFO_EPxCfg_FS(dev);//Set the FIFO Information
		}
		dev->ep0state = EP0_IDLE;
	}
	//<2>.Dequeue ALL requests
	nuke(&dev->ep[0], 0);
	dev->ep[0].stopped = 0;

	//<3>.Read 8-byte setup packet from FIFO

	// Read 8-byte setup packet from FIFO
	mUsbDMA2FIFOSel(dev->va_base,FOTG200_DMA2CxFIFO);
	u32UsbCmd[0] = mUsbEP0CmdDataRdDWord(dev->va_base);
	u32UsbCmd[1] = mUsbEP0CmdDataRdDWord(dev->va_base);
	mUsbDMA2FIFOSel(dev->va_base,FOTG200_DMA2FIFO_Non);
	memcpy(u8UsbCmd, u32UsbCmd, 8);

	DBG_CTRLL("L%x, EP0Cmd:%02x %02x %02x %02x %02x %02x %02x %02x\n",
		  dev->u8LineCount++, u8UsbCmd[0], u8UsbCmd[1], u8UsbCmd[2],
		  u8UsbCmd[3], u8UsbCmd[4], u8UsbCmd[5], u8UsbCmd[6],
		  u8UsbCmd[7]);

	/* read SETUP packet and enter DATA stage */
	ctrl.bRequestType = u8UsbCmd[0];
	ctrl.bRequest = u8UsbCmd[1];
	ctrl.wValue = (u8UsbCmd[3] << 8) | u8UsbCmd[2];
	ctrl.wIndex = (u8UsbCmd[5] << 8) | u8UsbCmd[4];
	ctrl.wLength = (u8UsbCmd[7] << 8) | u8UsbCmd[6];

	if (likely(ctrl.bRequestType & USB_DIR_IN)) {
		dev->ep[0].is_in = 1;
		dev->ep0state = EP0_IN;
	} else {
		dev->ep[0].is_in = 0;
		dev->ep0state = EP0_OUT;
	}

	if ((ctrl.bRequestType & USB_TYPE_MASK) == USB_TYPE_STANDARD) {
    	//Parsing the Standard Command
    	switch (ctrl.bRequest)	// by Standard Request codes
    	{
    	case USB_REQ_CLEAR_FEATURE:	// clear feature
    		if (bClear_feature(dev, &(ctrl)) == FALSE)
    			goto stall;
    		break;
    	case USB_REQ_SET_ADDRESS:	// set address
    		if (dev->ep0state == EP0_STALL)
    			goto stall;
    		if (bSet_address(dev, &(ctrl)) == FALSE)
    			goto stall;
    		break;
    	case USB_REQ_SET_FEATURE:	// clear feature
    		if (bSet_feature(dev, &(ctrl)) == FALSE)
    			goto stall;
    		break;
    	case USB_REQ_GET_STATUS:	// clear feature
    		if (bGet_status(dev, &(ctrl)) == FALSE)
    			goto stall;
    		break;
    	case USB_REQ_SYNCH_FRAME:	// clear feature
    		if (dev->ep0state == EP0_STALL)
    			goto stall;
    		if (bSynch_frame(dev, &(ctrl)) == FALSE)
    			goto stall;
    		break;
    	default:		/* pass to gadget driver */
    		if (dev->ep0state == EP0_STALL)
    			goto stall;

    		spin_unlock(&dev->lock);
    		tmp = dev->driver->setup(&dev->gadget, &(ctrl));
    		spin_lock(&dev->lock);
    		DBG_CTRLL(">>>Exit Driver call back setup function...\n");
    		if (unlikely(tmp < 0))
    			goto stall;
    		break;
    	}
	} else if ((ctrl.bRequestType & USB_TYPE_MASK) == USB_TYPE_CLASS) {
        if (dev->ep0state == EP0_STALL)
            goto stall;

        spin_unlock(&dev->lock);
        tmp = dev->driver->setup(&dev->gadget, &(ctrl));
        spin_lock(&dev->lock);
        if (unlikely(tmp < 0))
            goto stall;
	} else if ((ctrl.bRequestType & USB_TYPE_MASK) == USB_TYPE_VENDOR) {
        printk(">>>EP0 Vendor Cmd:%02x %02x %02x %02x %02x %02x %02x %02x\n",
              u8UsbCmd[0], u8UsbCmd[1], u8UsbCmd[2], u8UsbCmd[3],
              u8UsbCmd[4], u8UsbCmd[5], u8UsbCmd[6], u8UsbCmd[7]);
	}
	//Normal Exit
	return;

	//Stall the command
stall:
#ifdef USB_TRACE
	VDBG(dev, "req %02x.%02x protocol STALL; err %d\n", ctrl.bRequestType,
	     ctrl.bRequest, tmp);
#endif
	Info(dev, "Set STALL in vUsb_ep0setup\n");
	FTC_set_halt(dev->gadget.ep0, PROTO_STALL);	// Return EP0_Stall
	dev->ep[0].stopped = 1;
	dev->ep0state = EP0_STALL;
}

///////////////////////////////////////////////////////////////////////////////
//              vUsb_ep0end(struct FTC_udc *dev)
//              Description:
//                      1. End this transfer.
//              input: none
//              output: none
///////////////////////////////////////////////////////////////////////////////
static void vUsb_ep0end(struct FTC_udc *dev)
{
	DBG_FUNCC("+vUsb_ep0end()\n");

	dev->eUsbCxCommand = CMD_VOID;
}

///////////////////////////////////////////////////////////////////////////////
//              vUsb_ep0fail(struct FTC_udc *dev)
//              Description:
//                      1. Stall this transfer.
//              input: none
//              output: none
///////////////////////////////////////////////////////////////////////////////
static void vUsb_ep0fail(struct FTC_udc *dev)
{
	DBG_FUNCC("+vUsb_ep0fail()\n");
	Info(dev, "L%x, EP0 fail\n", dev->u8LineCount++);

	FTC_set_halt(dev->gadget.ep0, PROTO_STALL);	// Return EP0_Stall
}

///////////////////////////////////////////////////////////////////////////////
//              vUsbHandler(struct FTC_udc      *dev)
//              Description:
//                      1. Service all Usb events
//                      2. ReEnable Usb interrupt.
//              input: none
//              output: none
///////////////////////////////////////////////////////////////////////////////
static void vUsbHandler(struct FTC_udc *dev)	//FOTG200.ok
{
	u32 usb_interrupt_level2;
	u32 usb_interrupt_Mask;
	u32 usb_interrupt_Origan;

	DBG_FUNCC("+vUsbHandler(dev->va_base)\n");
	DBG_CTRLL("usb_interrupt_level1:0x%x\n", dev->usb_interrupt_level1);

	//----- Group Byte 2 ---------
	if (dev->usb_interrupt_level1 & BIT2) {
		usb_interrupt_Origan = mUsbIntSrc2Rd(dev->va_base);
		usb_interrupt_Mask = mUsbIntSrc2MaskRd(dev->va_base);
		usb_interrupt_level2 =
		    usb_interrupt_Origan & ~usb_interrupt_Mask;
		DBG_CTRLL("usb_interrupt_Origan:0x%x\n", usb_interrupt_Origan);
		DBG_CTRLL("usb_interrupt_Mask:0x%x\n", usb_interrupt_Mask);
		DBG_CTRLL("usb_interrupt_level2:0x%x\n", usb_interrupt_level2);

		if (usb_interrupt_level2 & BIT0)
			vUsb_rst(dev);
		if (usb_interrupt_level2 & BIT1)
			vUsb_suspend(dev);
		if (usb_interrupt_level2 & BIT2)
			vUsb_resm(dev);
		if (usb_interrupt_level2 & BIT3) {
			mUsbIntIsoSeqErrClr(dev->va_base);
			printk("??? ISO sequence error...\n");
		}
		if (usb_interrupt_level2 & BIT4) {
			mUsbIntIsoSeqAbortClr(dev->va_base);
			printk("??? ISO sequence error...\n");
		}
		if (usb_interrupt_level2 & BIT5) {
			mUsbIntTX0ByteClr(dev->va_base);
			mUsbIntTX0ByteSetClr(dev->va_base,0x9);	// Clear all zero-legnth
		}

		if (usb_interrupt_level2 & BIT6)
			mUsbIntRX0ByteClr(dev->va_base);

		if (usb_interrupt_level2 & BIT7) {
			mUsbIntDmaFinishClr(dev->va_base);
			dma_advance(dev, &(dev->ep[dev->EPUseDMA]));
		}
		if (usb_interrupt_level2 & BIT8) {
			mUsbIntDmaErrClr(dev->va_base);
			printk("??? DMA error Interrupt \n");
		}
	}
	//----- Group Byte 0 ---------
	if (dev->usb_interrupt_level1 & BIT0) {
		usb_interrupt_Origan = mUsbIntSrc0Rd(dev->va_base);
		usb_interrupt_Mask = mUsbIntSrc0MaskRd(dev->va_base);
		usb_interrupt_level2 =
		    usb_interrupt_Origan & ~usb_interrupt_Mask;

		DBG_CTRLL("IntSCR0:0x%x\n", usb_interrupt_level2);
		dev->ep[0].irqs++;
		//        Stop APB DMA if DMA is still running
		//        record buffer counter, and clear buffer. Later
		//        will re-input data use DMA.
		if (usb_interrupt_level2 & BIT0) {
            if (dev->EPUseDMA != DMA_CHANEL_FREE) {
                struct FTC_request *req;
                struct FTC_ep *ep;

                if (dev->EPUseDMA == 1) {
                    mUsbDmaAbort(dev->va_base);
                    while (mUsbDmaConfigRd(dev->va_base) & BIT0)
                        ;

                    mUsbIntDmaErrDis(dev->va_base);
                    mUsbIntDmaFinishDis(dev->va_base);
                    mUsbDMA2FIFOSel(dev->va_base,FOTG200_DMA2FIFO_Non);
                    dev->EPUseDMA = DMA_CHANEL_FREE;
                } else if (dev->EPUseDMA == 2) {
                    // abort ep2's request first
                    ep = &(dev->ep[dev->EPUseDMA]);
                    while (!list_empty(&ep->queue))
                    {
                        req = list_entry(ep->queue.next, struct FTC_request, queue);
                        if (req->req.actual && (req->req.actual < req->req.length)) {
                            done(ep, req, 0);
                            //done(ep, req, -ESHUTDOWN);
                        } else {
                            break;
                        }
                    }

                    mUsbIntDmaErrDis(dev->va_base);
                    mUsbIntDmaFinishDis(dev->va_base);
                    mUsbDMA2FIFOSel(dev->va_base,FOTG200_DMA2FIFO_Non);
                    dev->EPUseDMA = DMA_CHANEL_FREE;
                }
            }

			DBG_CTRLL("USB ep0 Setup\n");
            mUsbIntFIFO0_3OUTDis(dev->va_base);
            mUsbIntFIFO0_3INDis(dev->va_base);
            vUsb_ep0setup(dev);
		} else if (usb_interrupt_level2 & BIT3) {
			DBG_CTRLL("USB ep0 end\n");
			vUsb_ep0end(dev);
		}

		if (usb_interrupt_level2 & BIT1) {
			DBG_CTRLL("USB ep0 TX\n");
            cx_in_int_assert = 1;
            dma_advance(dev, &(dev->ep[0]));
		}

		if (usb_interrupt_level2 & BIT2) {
            struct FTC_ep *ep = &(dev->ep[0]);
            struct FTC_request *req;

			DBG_CTRLL("USB ep0 RX\n");
            cx_out_int_assert = 1;
            if (list_empty(&ep->queue)) { // only happened at PTP function enabled
//                printk("Error %s ==> ep0's LIST(0x%p) is empty in cx out\n", __func__, &ep->queue);
                mUsbIntEP0OutDis(dev->va_base);
            } else {
                req = list_entry(ep->queue.next, struct FTC_request, queue);
                if (req->req.actual)
                    dma_advance(dev, &(dev->ep[0]));
                else
                    start_dma(ep, req);
            }
		}

		if (usb_interrupt_level2 & BIT4) {
			uwarn(dev, "USB ep0 fail\n");
			vUsb_ep0fail(dev);
		}

		if (usb_interrupt_level2 & BIT5) {
			mUsbIntEP0AbortClr(dev->va_base);
			printk("??? Command Abort Interrupt ...\n");
		}
	}
	//----- Group Byte 1 ---------
	if (dev->usb_interrupt_level1 & BIT1) {
		usb_interrupt_Origan = mUsbIntSrc1Rd(dev->va_base);
		usb_interrupt_Mask = mUsbIntSrc1MaskRd(dev->va_base);
		usb_interrupt_level2 =
		    usb_interrupt_Origan & (~usb_interrupt_Mask);

		DBG_CTRLL("(IntSCR1:0x%x)(Mask1:0x%x)(usb_interrupt_level2=0x%x)\n",
		     usb_interrupt_Origan, mUsbIntSrc1MaskRd(dev->va_base),
		     usb_interrupt_level2);

		// use FIFO1 for ep2( bulk out)
		if (usb_interrupt_level2 & BIT3)	// short packet
			ep_isr(dev, 2);
		else if (usb_interrupt_level2 & BIT2)	// full packet
			ep_isr(dev, 2);

		// use FIFO0 for ep1( bulk in)
		if (usb_interrupt_level2 & BIT16)
			ep_isr(dev, 1);

		// use FIFO3 for ep4( Interrupt out)
		if (usb_interrupt_level2 & BIT7)	// short packet
			ep_isr(dev, 4);
		else if (usb_interrupt_level2 & BIT6)	// full packet
			ep_isr(dev, 4);

		// use FIFO2 for ep3( Interrupt in)
		if (usb_interrupt_level2 & BIT18)
			ep_isr(dev, 3);
	}
}
