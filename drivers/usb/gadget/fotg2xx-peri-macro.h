
#ifndef __FOTG200_M_H
#define __FOTG200_M_H

#define FIFOCX	         0xFF
#define DMA_CHANEL_FREE  0XFF

#define MAX_EP_NUM	8
#define MAX_FIFO_NUM	4

#define CPE_FOTG200_BASE(x) (x+0x100)
#define IRQ_USBDEV 	USB_FOTG2XX_IRQ
#define FUSB220_DMA_IS_IDLE_NOW DMA_CHANEL_FREE

#define FUSB220_BASE_ADDRESS(s)		CPE_FOTG200_BASE(x)
#define FOTG200_FIFO_BASE(bOffset)	((CPE_FOTG200_BASE(x))+0xC0+(bOffset<<2))

#define bFOTGPeri_Port(x,bOffset)	*((volatile u8 *) ((u32)x|(u32)(bOffset)))
#define hwFOTGPeri_Port(x,bOffset)	*((volatile u16 *) ((u32)x|(u32)(bOffset)))
#define wFOTGPeri_Port(x,bOffset)	*((volatile u32 *) ((u32)x|(u32)(bOffset)))

// Main control register(0x100)
#define mUsbRmWkupST(x)			(wFOTGPeri_Port(x,0x100) & BIT0)
#define mUsbRmWkupSet(x)		        (wFOTGPeri_Port(x,0x100) |= BIT0)
#define mUsbRmWkupClr(x)			(wFOTGPeri_Port(x,0x100) &= ~BIT0)

#define mUsbTstHalfSpeedEn(x)		(wFOTGPeri_Port(x,0x100) |= BIT1)
#define mUsbTstHalfSpeedDis(x)	        (wFOTGPeri_Port(x,0x100) &= ~BIT1)

#define mUsbGlobIntEnRd(x)		(wFOTGPeri_Port(x,0x100) & BIT2)
#define mUsbGlobIntEnSet(x)		(wFOTGPeri_Port(x,0x100) |= BIT2)
#define mUsbGlobIntDis(x)		(wFOTGPeri_Port(x,0x100) &= ~BIT2)

#define mUsbGoSuspend(x)			(wFOTGPeri_Port(x,0x100) |=  BIT3)

#define mUsbSoftRstSet(x)		(wFOTGPeri_Port(x,0x100) |=  BIT4)
#define mUsbSoftRstClr(x)		(wFOTGPeri_Port(x,0x100) &= ~BIT4)

#define mUsbChipEnSet(x)			(wFOTGPeri_Port(x,0x100) |= BIT5)
#define mUsbHighSpeedST(x)		(wFOTGPeri_Port(x,0x100) & BIT6)
#define mUsbDMAResetSet(x)		(wFOTGPeri_Port(x,0x100) |= BIT8)

#define mUsbForceFSSet(x)		(wFOTGPeri_Port(x,0x100) |=  BIT9)
#define mUsbForceFSClr(x)		(wFOTGPeri_Port(x,0x100) &= ~BIT9)

// Device address register(0x104)
#define mUsbDevAddrSet(x,Value)	        (wFOTGPeri_Port(x,0x104) = (u32)Value)
#define mUsbCfgST(x)			(wFOTGPeri_Port(x,0x104) & BIT7)
#define mUsbCfgSet(x)			(wFOTGPeri_Port(x,0x104) |= BIT7)
#define mUsbCfgClr(x)			(wFOTGPeri_Port(x,0x104) &= ~BIT7)

// Test register(0x108)
#define mUsbClrAllFIFOSet(x)		(wFOTGPeri_Port(x,0x108) |= BIT0)
#define mUsbClrAllFIFOClr(x)		(wFOTGPeri_Port(x,0x108) &= ~BIT0)

// SOF Frame Number register(0x10C)
#define mUsbFrameNo(x)			(u16)(wFOTGPeri_Port(x,0x10C) & 0x7FF)
#define mUsbMicroFrameNo(x)		(u8)((wFOTGPeri_Port(x,0x10C) & 0x3800)>>11)
#define mUsbFrameNoLow(x)		(mUsbFrameNo(x) & 0xff)
#define mUsbFrameNoHigh(x)		(mUsbFrameNo(x) >> 8)

// SOF Mask register(0x110)
#define mUsbSOFMaskHS(x)		        (wFOTGPeri_Port(x,0x110) = 0x44c)
#define mUsbSOFMaskFS(x)		        (wFOTGPeri_Port(x,0x110) = 0x2710)

// PHY Test Mode Selector register(0x114)
#define mUsbTsMdWr(x,item)		(wFOTGPeri_Port(x,0x114) = (u32)item)
#define mUsbUnPLGClr(x)			(wFOTGPeri_Port(x,0x114) &= ~BIT0)
#define mUsbUnPLGSet(x)			(wFOTGPeri_Port(x,0x114) |= BIT0)
// Vendor Specific IO Control register(0x118)

// Cx configuration and status register(0x11C)

// Cx configuration and FIFO Empty Status register(0x120)
#define mUsbEP0DoneSet(x)		(wFOTGPeri_Port(x,0x120) |= BIT0)
#define mUsbTsPkDoneSet(x)		(wFOTGPeri_Port(x,0x120) |= BIT1)
#define mUsbEP0StallSet(x)		(wFOTGPeri_Port(x,0x120) |= BIT2)
#define mUsbCxFClr(x)			(wFOTGPeri_Port(x,0x120) |= BIT3)

#define mUsbCxFFull(x)			(wFOTGPeri_Port(x,0x120) & BIT4)
#define mUsbCxFEmpty(x)			(wFOTGPeri_Port(x,0x120) & BIT5)
#define mUsbCxFByteCnt(x)		(u8)((wFOTGPeri_Port(x,0x120) & 0x7F000000)>>24)
#define mUsbEP0ClearFIFO(x)              (bFOTGPeri_Port(x,0x0120) |= (u8)BIT3)  //john
#define mUsbEP0EMPFIFO(x)                (bFOTGPeri_Port(x,0x0120) &= (u8)BIT5)  //john

// IDLE Counter register(0x124)
#define mUsbIdleCnt(x,time)		(wFOTGPeri_Port(x,0x124) = (u32)time)

// Mask of interrupt group(0x130)
#define mUsbIntGrp0Dis(x)		(wFOTGPeri_Port(x,0x130) |= BIT0)
#define mUsbIntGrp1Dis(x)		(wFOTGPeri_Port(x,0x130) |= BIT1)
#define mUsbIntGrp2Dis(x)		(wFOTGPeri_Port(x,0x130) |= BIT2)

#define mUsbIntGroupMaskRd(x)	        (wFOTGPeri_Port(x,0x130))

// Mask of interrupt source group 0(0x134)
#define mUsbIntEP0SetupDis(x)		(wFOTGPeri_Port(x,0x134) |= BIT0)
#define mUsbIntEP0InDis(x)		(wFOTGPeri_Port(x,0x134) |= BIT1)
#define mUsbIntEP0OutDis(x)		(wFOTGPeri_Port(x,0x134) |= BIT2)
#define mUsbIntEP0EndDis(x)		(wFOTGPeri_Port(x,0x134) |= BIT3)
#define mUsbIntEP0FailDis(x)		(wFOTGPeri_Port(x,0x134) |= BIT4)

#define mUsbIntEP0SetupEn(x)		(wFOTGPeri_Port(x,0x134) &= ~(BIT0))
#define mUsbIntEP0InEn(x)		(wFOTGPeri_Port(x,0x134) &= ~(BIT1))
#define mUsbIntEP0OutEn(x)		(wFOTGPeri_Port(x,0x134) &= ~(BIT2))
#define mUsbIntEP0EndEn(x)		(wFOTGPeri_Port(x,0x134) &= ~(BIT3))
#define mUsbIntEP0FailEn(x)		(wFOTGPeri_Port(x,0x134) &= ~(BIT4))

#define mUsbIntSrc0MaskRd(x)		(wFOTGPeri_Port(x,0x134))

// Mask of interrupt source group 1(0x138)
#define mUsbIntFIFO0_3OUTDis(x)	        (wFOTGPeri_Port(x,0x138) |= 0xFF)
#define mUsbIntFIFO0_3INDis(x)	        (wFOTGPeri_Port(x,0x138) |= 0xF0000)
#define mUsbIntFIFO0_3Set(x,wTemp)	(wFOTGPeri_Port(x,0x138)|= wTemp)
#define mUsbIntFIFO0_3Dis(x,wTemp)	(wFOTGPeri_Port(x,0x138)&= ~wTemp)

#define mUsbIntF0OUTEn(x)		(wFOTGPeri_Port(x,0x138) &= ~(BIT1 | BIT0))
#define mUsbIntF0OUTDis(x)		(wFOTGPeri_Port(x,0x138) |= (BIT1 | BIT0))
#define mUsbIntF1OUTEn(x)		(wFOTGPeri_Port(x,0x138) &= ~(BIT3 | BIT2))
#define mUsbIntF1OUTDis(x)		(wFOTGPeri_Port(x,0x138) |= (BIT3 | BIT2))
#define mUsbIntF2OUTEn(x)		(wFOTGPeri_Port(x,0x138) &= ~(BIT5 | BIT4))
#define mUsbIntF2OUTDis(x)		(wFOTGPeri_Port(x,0x138) |= (BIT5 | BIT4))
#define mUsbIntF3OUTEn(x)		(wFOTGPeri_Port(x,0x138) &= ~(BIT7 | BIT6))
#define mUsbIntF3OUTDis(x)		(wFOTGPeri_Port(x,0x138) |= (BIT7 | BIT6))

#define mUsbIntFXOUTEn(x,bnum)	        (wFOTGPeri_Port(x,0x138) &= ~((BIT0<<((bnum)*2+1)) | (BIT0<<((bnum)*2))))
#define mUsbIntFXOUTDis(x,bnum)     	(wFOTGPeri_Port(x,0x138) |= ((BIT0<<((bnum)*2+1)) | (BIT0<<((bnum)*2))))

#define mUsbIntF0INEn(x)			(wFOTGPeri_Port(x,0x138) &= ~BIT16)
#define mUsbIntF0INDis(x)		(wFOTGPeri_Port(x,0x138) |= BIT16)
#define mUsbIntF1INEn(x)			(wFOTGPeri_Port(x,0x138) &= ~BIT17)
#define mUsbIntF1INDis(x)		(wFOTGPeri_Port(x,0x138) |= BIT17)
#define mUsbIntF2INEn(x)			(wFOTGPeri_Port(x,0x138) &= ~BIT18)
#define mUsbIntF2INDis(x)		(wFOTGPeri_Port(x,0x138) |= BIT18)
#define mUsbIntF3INEn(x)			(wFOTGPeri_Port(x,0x138) &= ~BIT19)
#define mUsbIntF3INDis(x)		(wFOTGPeri_Port(x,0x138) |= BIT19)

#define mUsbIntFXINEn(x,bnum)		(wFOTGPeri_Port(x,0x138) &= ~(BIT0<<(bnum+16)))
#define mUsbIntFXINDis(x,bnum)	        (wFOTGPeri_Port(x,0x138) |= (BIT0<<(bnum+16)))

#define mUsbIntSrc1MaskRd(x)		(wFOTGPeri_Port(x,0x138))

// Mask of interrupt source group 2(DMA int mask)(0x13C)
#define mUsbIntSuspDis(x)		(wFOTGPeri_Port(x,0x13C) |= BIT1)
#define mUsbIntDmaErrDis(x)		(wFOTGPeri_Port(x,0x13C) |= BIT8)
#define mUsbIntDmaFinishDis(x)	        (wFOTGPeri_Port(x,0x13C) |= BIT7)

#define mUsbIntSuspEn(x)			(wFOTGPeri_Port(x,0x13C) &= ~(BIT1))
#define mUsbIntDmaErrEn(x)		(wFOTGPeri_Port(x,0x13C) &= ~(BIT8))
#define mUsbIntDmaFinishEn(x)	        (wFOTGPeri_Port(x,0x13C) &= ~(BIT7))

#define mUsbIntSrc2MaskRd(x)		(wFOTGPeri_Port(x,0x13C))

// Interrupt group (0x140)
#define mUsbIntGroupRegRd(x)		(wFOTGPeri_Port(x,0x140))
#define mUsbIntGroupRegSet(x,wValue)	(wFOTGPeri_Port(x,0x140) |= wValue)

// Interrupt source group 0(0x144)
#define mUsbIntSrc0Rd(x)			(wFOTGPeri_Port(x,0x144))
#if !defined(CONFIG_PLATFORM_GM8126)
#define mUsbIntEP0AbortClr(x)           (wFOTGPeri_Port(x,0x144) = BIT5)
#else
#define mUsbIntEP0AbortClr(x)	        (wFOTGPeri_Port(x,0x144) &= ~(BIT5))
#endif
#if !defined(CONFIG_PLATFORM_GM8126)
#define mUsbIntSrc0Clr(x)		(wFOTGPeri_Port(x,0x144) = ~0)
#else
#define mUsbIntSrc0Clr(x)		(wFOTGPeri_Port(x,0x144) = 0)
#endif
#define mUsbIntSrc0Set(x,wValue)	        (wFOTGPeri_Port(x,0x144) |= wValue)

// Interrupt source group 1(0x148)
#define mUsbIntSrc1Rd(x)			(wFOTGPeri_Port(x,0x148))
#define mUsbIntSrc1Set(x,wValue)	        (wFOTGPeri_Port(x,0x148) |= wValue)

// Interrupt source group 2(0x14C)
#define mUsbIntSrc2Rd(x)			(wFOTGPeri_Port(x,0x14C))
#define mUsbIntSrc2Set(x,wValue)	        (wFOTGPeri_Port(x,0x14C) |= wValue)


#if !defined(CONFIG_PLATFORM_GM8126)
#define mUsbIntBusRstClr(x)		(wFOTGPeri_Port(x,0x14C) = BIT0)
#define mUsbIntSuspClr(x)		(wFOTGPeri_Port(x,0x14C) = BIT1)
#define mUsbIntResmClr(x)		(wFOTGPeri_Port(x,0x14C) = BIT2)
#define mUsbIntIsoSeqErrClr(x)	        (wFOTGPeri_Port(x,0x14C) = BIT3)
#define mUsbIntIsoSeqAbortClr(x)	(wFOTGPeri_Port(x,0x14C) = BIT4)
#define mUsbIntTX0ByteClr(x)		(wFOTGPeri_Port(x,0x14C) = BIT5)
#define mUsbIntRX0ByteClr(x)		(wFOTGPeri_Port(x,0x14C) = BIT6)
#define mUsbIntDmaFinishClr(x)	        (wFOTGPeri_Port(x,0x14C) = BIT7)
#define mUsbIntDmaErrClr(x)		(wFOTGPeri_Port(x,0x14C) = BIT8)
#else
#define mUsbIntBusRstClr(x)		(wFOTGPeri_Port(x,0x14C) &= ~BIT0)
#define mUsbIntSuspClr(x)		(wFOTGPeri_Port(x,0x14C) &= ~BIT1)
#define mUsbIntResmClr(x)		(wFOTGPeri_Port(x,0x14C) &= ~BIT2)
#define mUsbIntIsoSeqErrClr(x)	        (wFOTGPeri_Port(x,0x14C) &= ~BIT3)
#define mUsbIntIsoSeqAbortClr(x)	(wFOTGPeri_Port(x,0x14C) &= ~BIT4)
#define mUsbIntTX0ByteClr(x)		(wFOTGPeri_Port(x,0x14C) &= ~BIT5)
#define mUsbIntRX0ByteClr(x)		(wFOTGPeri_Port(x,0x14C) &= ~BIT6)
#define mUsbIntDmaFinishClr(x)	        (wFOTGPeri_Port(x,0x14C) &= ~BIT7)
#define mUsbIntDmaErrClr(x)		(wFOTGPeri_Port(x,0x14C) &= ~BIT8)
#endif

#define mUsbIntDmaFinishRd(x)		(wFOTGPeri_Port(x,0x14C) & BIT7)
#define mUsbIntDmaErrRd(x)		(wFOTGPeri_Port(x,0x14C) & BIT8)

// Rx 0 byte packet register(0x150)
#define mUsbIntRX0ByteRd(x)		(u8)(wFOTGPeri_Port(x,0x150))
#define mUsbIntRX0ByteSetClr(x,set)	(wFOTGPeri_Port(x,0x150) &= ~((u32)set))

// Tx 0 byte packet register(0x154)
#define mUsbIntTX0ByteRd(x)		(u8)(wFOTGPeri_Port(x,0x154))
#define mUsbIntTX0ByteSetClr(x,data)	(wFOTGPeri_Port(x,0x154) &= ~((u32)data))

// ISO sequential Error/Abort register(0x158)
#define mUsbIntIsoSeqErrRd(x)		(u8)((wFOTGPeri_Port(x,0x158) & 0xff0000)>>16)
#define mUsbIntIsoSeqErrSetClr(x,data)	(wFOTGPeri_Port(x,0x158) &= ~(((u32)data)<<16))

#define mUsbIntIsoSeqAbortRd(x)	        (u8)(wFOTGPeri_Port(x,0x158) & 0xff)
#define mUsbIntIsoSeqAbortSetClr(x,data)	(wFOTGPeri_Port(x,0x158) &= ~((u32)data))

// IN Endpoint MaxPacketSize register(0x160,0x164,...,0x17C)
#define mUsbEPinHighBandSet(x,EPn, dir , size )	(wFOTGPeri_Port(x,0x160 + ((EPn - 1) << 2)) &= ~(BIT14 |BIT13));  (wFOTGPeri_Port(x,0x160 + ((EPn - 1) << 2)) |= ((((u8)(size >> 11)+1) << 13)* dir) )

#define mUsbEPMxPtSz(x,EPn, dir, size) (wFOTGPeri_Port(x,0x160 + ((1-dir) * 0x20) + ((EPn - 1) << 2)) = (u16)(size))

#define mUsbEPMxPtSzClr(x,EPn, dir)    (wFOTGPeri_Port(x,0x160 + ((1-dir) * 0x20) + ((EPn - 1) << 2)) = 0)

#define mUsbEPMxPtSzRd(x,EPn, dir)     ((wFOTGPeri_Port(x,0x160 + ((1-dir) * 0x20) + ((EPn - 1) << 2))) & 0x7ff)

#define mUsbEPinMxPtSz(x,EPn)		(wFOTGPeri_Port(x,0x160 + ((EPn - 1) << 2)) & 0x7ff)
#define mUsbEPinStallST(x,EPn)		((wFOTGPeri_Port(x,0x160 + ((EPn - 1) << 2)) & BIT11) >> 11)
#define mUsbEPinStallClr(x,EPn)		(wFOTGPeri_Port(x,0x160 + ((EPn - 1) << 2)) &= ~BIT11)
#define mUsbEPinStallSet(x,EPn)		(wFOTGPeri_Port(x,0x160 + ((EPn - 1) << 2)) |=  BIT11)
#define mUsbEPinRsTgClr(x,EPn)		(wFOTGPeri_Port(x,0x160 + ((EPn - 1) << 2)) &= ~BIT12)
#define mUsbEPinRsTgSet(x,EPn)	        (wFOTGPeri_Port(x,0x160 + ((EPn - 1) << 2)) |=  BIT12)
#define mUsbEPinZeroSet(x,EPn)	        (wFOTGPeri_Port(x,0x160 + ((EPn - 1) << 2)) |=  BIT15)

// OUT Endpoint MaxPacketSize register(0x180,0x164,...,0x19C)
#define mUsbEPoutMxPtSz(x,EPn)	        ((wFOTGPeri_Port(x,0x180 + ((EPn - 1) << 2))) & 0x7ff)
#define mUsbEPoutStallST(x,EPn)	        ((wFOTGPeri_Port(x,0x180 + ((EPn - 1) << 2)) & BIT11) >> 11)
#define mUsbEPoutStallClr(x,EPn)	        (wFOTGPeri_Port(x,0x180 + ((EPn - 1) << 2)) &= ~BIT11)
#define mUsbEPoutStallSet(x,EPn)	        (wFOTGPeri_Port(x,0x180 + ((EPn - 1) << 2)) |=  BIT11)
#define mUsbEPoutRsTgClr(x,EPn)	        (wFOTGPeri_Port(x,0x180 + ((EPn - 1) << 2)) &= ~BIT12)
#define mUsbEPoutRsTgSet(x,EPn)	        (wFOTGPeri_Port(x,0x180 + ((EPn - 1) << 2)) |=  BIT12)

// Endpoint & FIFO Configuration
// Endpoint 1~4 Map register(0x1a0), Endpoint 5~8 Map register(0x1a4)
#define mUsbEPMap(x,EPn, MAP)	        (bFOTGPeri_Port(x,0x1a0 + (EPn-1)) = MAP)
#define mUsbEPMapRd(x,EPn)		(bFOTGPeri_Port(x,0x1a0+ (EPn-1)))
#define mUsbEPMapAllClr(x)		(wFOTGPeri_Port(x,0x1a0) = 0);(wFOTGPeri_Port(x,0x1a4) = 0)
#define mUsbEPMap1_4Rd(x)                (wFOTGPeri_Port(x,0x1a0))

// FIFO Map register(0x1a8)
#define mUsbFIFOMap(x,FIFOn, MAP)	        (bFOTGPeri_Port(x,0x1a8 + FIFOn) = MAP)
#define mUsbFIFOMapRd(x,FIFOn)	        (bFOTGPeri_Port(x,0x1a8 + FIFOn))
#define mUsbFIFOMapAllClr(x)		(wFOTGPeri_Port(x,0x1a8) = 0)
#define mUsbFIFOMapAllRd(x)              (wFOTGPeri_Port(x,0x1a8))
// FIFO Configuration register(0x1ac)
#define mUsbFIFOConfig(x,FIFOn, CONFIG)	(bFOTGPeri_Port(x,0x1ac + FIFOn) = CONFIG)
#define mUsbFIFOConfigRd(x,FIFOn)		(bFOTGPeri_Port(x,0x1ac + FIFOn))
#define mUsbFIFOConfigAllClr(x)		(bFOTGPeri_Port(x,0x1ac) = 0)
#define FIFOEnBit			0x20
#define mUsbFIFOConfigAllRd(x)          (wFOTGPeri_Port(x,0x1ac))
// FIFO byte count register(0x1b0)
#define mUsbFIFOOutByteCount(x,fifo_num)	(((wFOTGPeri_Port(x,0x1b0+(fifo_num)*4)&0x7ff)))
#define mUsbFIFODone(x,fifo_num)		(wFOTGPeri_Port(x,0x1b0+(fifo_num)*4) |= BIT11)
#define mUsbFIFOReset(x,fifo_num)		(wFOTGPeri_Port(x,0x1b0+(fifo_num)*4) |=  BIT12) //john
#define mUsbFIFOResetOK(x,fifo_num)		(wFOTGPeri_Port(x,0x1b0+(fifo_num)*4) &= ~BIT12) //john

// DMA target FIFO register(0x1c0)
#define FOTG200_DMA2FIFO_Non 		0
#define FOTG200_DMA2FIFO0 		BIT0
#define FOTG200_DMA2FIFO1 		BIT1
#define FOTG200_DMA2FIFO2 		BIT2
#define FOTG200_DMA2FIFO3 		BIT3
#define FOTG200_DMA2CxFIFO 		BIT4

#define mUsbDMA2FIFOSel(x,sel)		(wFOTGPeri_Port(x,0x1c0) = sel)
#define mUsbDMA2FIFORd(x)		(wFOTGPeri_Port(x,0x1c0))

// DMA parameter set 1 (0x1c8)
#define mUsbDmaConfig(x,len,Dir)	(wFOTGPeri_Port(x,0x1c8) = (((u32)len)<<8)|(Dir<<1))
#define mUsbDmaLenRd(x)			((wFOTGPeri_Port(x,0x1c8) & 0x1ffff00) >> 8)
#define mUsbDmaConfigRd(x)		(wFOTGPeri_Port(x,0x1c8))
#define mUsbDmaConfigSet(set)		(wFOTGPeri_Port(x,0x1c8) = set)

#define mUsbDmaStart(x)			(wFOTGPeri_Port(x,0x1c8) |= BIT0)
#define mUsbDmaStop(x)			(wFOTGPeri_Port(x,0x1c8) &= ~BIT0)
#define mUsbDmaAbort(x)			(wFOTGPeri_Port(x,0x1c8) |= (BIT3 | BIT4))

// DMA parameter set 2 (0x1cc)
#define mUsbDmaAddr(x,addr)		(wFOTGPeri_Port(x,0x1cc) = addr)
#define mUsbDmaAddrRd(x)			(wFOTGPeri_Port(x,0x1cc))

// 8 byte command data port(0x1d0)
#define mUsbEP0CmdDataRdDWord(x)	        (wFOTGPeri_Port(x,0x1d0))

//For OTG Definition;;0x80
#define mUsb_OTGC_Control_B_BUS_REQ_Rd(x)	(wFOTGPeri_Port(x,0x80)& BIT0)
#define mUsb_OTGC_Control_B_BUS_REQ_Set(x)  	(wFOTGPeri_Port(x,0x80) |=  BIT0)
#define mUsb_OTGC_Control_B_BUS_REQ_Clr(x)  	(wFOTGPeri_Port(x,0x80) &=  (~BIT0))

#define mUsb_OTGC_Control_B_HNP_EN_Rd(x)		(wFOTGPeri_Port(x,0x80)& BIT1)
#define mUsb_OTGC_Control_B_HNP_EN_Set(x)	(wFOTGPeri_Port(x,0x80) |=  BIT1)
#define mUsb_OTGC_Control_B_HNP_EN_Clr(x)  	(wFOTGPeri_Port(x,0x80) &=  (~BIT1))

#define mUsb_OTGC_Control_B_DSCHG_VBUS_Rd(x)	(wFOTGPeri_Port(x,0x80)& BIT2)
#define mUsb_OTGC_Control_B_DSCHG_VBUS_Set(x)	(wFOTGPeri_Port(x,0x80) |=  BIT2)
#define mUsb_OTGC_Control_B_DSCHG_VBUS_Clr(x) 	(wFOTGPeri_Port(x,0x80) &=  (~BIT2))

#define mUsb_OTGC_Control_B_SESS_END_Rd(x)	(wFOTGPeri_Port(x,0x80)& BIT16)
#define mUsb_TGC_Control_B_SESS_VLD_Rd(x)	(wFOTGPeri_Port(x,0x80)& BIT17)
#define mUsb_TGC_Control_A_SESS_VLD_Rd(x)	(wFOTGPeri_Port(x,0x80)& BIT18)
#define mUsb_TGC_Control_A_VBUS_VLD_Rd(x)	(wFOTGPeri_Port(x,0x80)& BIT19)

#define mUsb_OTGC_Control_CROLE_Rd(x)	        (wFOTGPeri_Port(x,0x80)& BIT20) //0:Host 1:Peripheral

#define mUsb_dwOTGC_INT_STS_ROLE_CHG_Rd(x)       (wFOTGPeri_Port(x,0x84)& BIT8)

//For Host Port Reset setting (Timing Critical)
#define mUsb_mwHost20_PORTSC_PortReset_Rd(x)	(wFOTGPeri_Port(x,0x30) &=  BIT8)
#define mUsb_mwHost20_PORTSC_PortReset_Set(x)	(wFOTGPeri_Port(x,0x30) |=  BIT8)
#define mUsb_mwHost20_PORTSC_PortReset_Clr(x)	(wFOTGPeri_Port(x,0x30) &=  ~BIT8)

#define mUsbIntFIFOEn(x,off,val)  (bFOTGPeri_Port(x,CPE_FOTG200_BASE(x) | off) &= ~(val))        //john
#define mUsbIntFIFODis(x,off,val) (bFOTGPeri_Port(x,CPE_FOTG200_BASE(x) | off) |=  (val))        //john

#endif /* __FOTG200_M_H  */
