/*
    USB device Endpoint setting functions

    Setting Endpoint and FIFO configuration etc...

    @file       usb_EPConfig.c
    @ingroup    mIDrvUSB_Device
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2014.  All rights reserved.
*/

#include "usb2dev.h"
#include "usb2dev_reg.h"
#include "usb2dev_int.h"

static  VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags) vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags) vk_spin_unlock_irqrestore(&my_lock, flags)

BOOL    bAbortDMA       = FALSE;

/* saving EP & FIFO mapping information */

// record EP maps to which FIFO
UINT32  gEPMap[USB_EP_MAX - 1] = {
	USB_FIFO_NOT_USE, USB_FIFO_NOT_USE, USB_FIFO_NOT_USE, USB_FIFO_NOT_USE, USB_FIFO_NOT_USE, USB_FIFO_NOT_USE, USB_FIFO_NOT_USE, USB_FIFO_NOT_USE,
	USB_FIFO_NOT_USE, USB_FIFO_NOT_USE, USB_FIFO_NOT_USE, USB_FIFO_NOT_USE, USB_FIFO_NOT_USE, USB_FIFO_NOT_USE, USB_FIFO_NOT_USE
};

// record IN FIFO maps to which EP
UINT32  gFIFOInMap[USB_FIFO_MAX] = {
	USB_EP_NOT_USE, USB_EP_NOT_USE, USB_EP_NOT_USE, USB_EP_NOT_USE, USB_EP_NOT_USE, USB_EP_NOT_USE, USB_EP_NOT_USE, USB_EP_NOT_USE
};

// record OUT FIFO maps to which EP
UINT32  gFIFOOutMap[USB_FIFO_MAX] = {
	USB_EP_NOT_USE, USB_EP_NOT_USE, USB_EP_NOT_USE, USB_EP_NOT_USE, USB_EP_NOT_USE, USB_EP_NOT_USE, USB_EP_NOT_USE, USB_EP_NOT_USE
};

/**
    @addtogroup mIDrvUSB_Device
*/
//@{


/*
    Set endpoint and FIFO mapping

    This function is internal FIFO setting functions

    @param[in] EPn      endpoint number (1~8)
    @param[in] Dir      endpoint direction
                        - @b EP_DIR_IN: endpoint direction IN
                        - @b EP_DIR_OUT: endpoint direction OUT
    @param[in] FIFOn    FIFO number (0~3)

    @return void
*/
static void usb_setEPMap(USB_EP EPn, USB_EP_DIR Dir, USB_FIFO_NUM FIFOn)
{
	if (EPn <= USB_EP4) {
		T_USB_DEVEPMAP1_4_REG   DevEpMAP1_4Reg;
		DevEpMAP1_4Reg.reg = USB_GETREG(USB_DEVEPMAP1_4_REG_OFS);
		switch (EPn) {
			case USB_EP1:
				if (Dir == EP_DIR_IN)
					DevEpMAP1_4Reg.bit.FNO_IEP1 = FIFOn;
				else
					DevEpMAP1_4Reg.bit.FNO_OEP1 = FIFOn;
				break;
			case USB_EP2:
				if (Dir == EP_DIR_IN)
					DevEpMAP1_4Reg.bit.FNO_IEP2 = FIFOn;
				else
					DevEpMAP1_4Reg.bit.FNO_OEP2 = FIFOn;
				break;
			case USB_EP3:
				if (Dir == EP_DIR_IN)
					DevEpMAP1_4Reg.bit.FNO_IEP3 = FIFOn;
				else
					DevEpMAP1_4Reg.bit.FNO_OEP3 = FIFOn;
				break;
			default:
				if (Dir == EP_DIR_IN)
					DevEpMAP1_4Reg.bit.FNO_IEP4 = FIFOn;
				else
					DevEpMAP1_4Reg.bit.FNO_OEP4 = FIFOn;
				break;
		}
		USB_SETREG(USB_DEVEPMAP1_4_REG_OFS, DevEpMAP1_4Reg.reg);
	} else if (EPn <= USB_EP8) {
		T_USB_DEVEPMAP5_8_REG   DevEpMAP5_8Reg;
		DevEpMAP5_8Reg.reg = USB_GETREG(USB_DEVEPMAP5_8_REG_OFS);
		switch (EPn) {
			case USB_EP5:
				if (Dir == EP_DIR_IN)
					DevEpMAP5_8Reg.bit.FNO_IEP5 = FIFOn;
				else
					DevEpMAP5_8Reg.bit.FNO_OEP5 = FIFOn;
				break;
			case USB_EP6:
				if (Dir == EP_DIR_IN)
					DevEpMAP5_8Reg.bit.FNO_IEP6 = FIFOn;
				else
					DevEpMAP5_8Reg.bit.FNO_OEP6 = FIFOn;
				break;
			case USB_EP7:
				if (Dir == EP_DIR_IN)
					DevEpMAP5_8Reg.bit.FNO_IEP7 = FIFOn;
				else
					DevEpMAP5_8Reg.bit.FNO_OEP7 = FIFOn;
				break;
			default:
				if (Dir == EP_DIR_IN)
					DevEpMAP5_8Reg.bit.FNO_IEP8 = FIFOn;
				else
					DevEpMAP5_8Reg.bit.FNO_OEP8 = FIFOn;
				break;
		}
		USB_SETREG(USB_DEVEPMAP5_8_REG_OFS, DevEpMAP5_8Reg.reg);
	} else if (EPn <= USB_EP12) {
		T_USB_DEVEPMAP9_12_REG DevEpMAP9_12Reg;

		DevEpMAP9_12Reg.reg = USB_GETREG(USB_DEVEPMAP9_12_REG_OFS);
		switch (EPn) {
			case USB_EP9:
				if (Dir == EP_DIR_IN)
					DevEpMAP9_12Reg.bit.FNO_IEP9 = FIFOn;
				else
					DevEpMAP9_12Reg.bit.FNO_OEP9 = FIFOn;
				break;
			case USB_EP10:
				if (Dir == EP_DIR_IN)
					DevEpMAP9_12Reg.bit.FNO_IEP10 = FIFOn;
				else
					DevEpMAP9_12Reg.bit.FNO_OEP10 = FIFOn;
				break;
			case USB_EP11:
				if (Dir == EP_DIR_IN)
					DevEpMAP9_12Reg.bit.FNO_IEP11 = FIFOn;
				else
					DevEpMAP9_12Reg.bit.FNO_OEP11 = FIFOn;
				break;
			case USB_EP12:
				if (Dir == EP_DIR_IN)
					DevEpMAP9_12Reg.bit.FNO_IEP12 = FIFOn;
				else
					DevEpMAP9_12Reg.bit.FNO_OEP12 = FIFOn;
				break;
			default:
				break;
		}
		USB_SETREG(USB_DEVEPMAP9_12_REG_OFS, DevEpMAP9_12Reg.reg);

	} else {
		T_USB_DEVEPMAP13_15_REG DevEpMAP13_15Reg;

		DevEpMAP13_15Reg.reg = USB_GETREG(USB_DEVEPMAP13_15_REG_OFS);
		switch (EPn) {
			case USB_EP13:
				if (Dir == EP_DIR_IN)
					DevEpMAP13_15Reg.bit.FNO_IEP13 = FIFOn;
				else
					DevEpMAP13_15Reg.bit.FNO_OEP13 = FIFOn;
				break;
			case USB_EP14:
				if (Dir == EP_DIR_IN)
					DevEpMAP13_15Reg.bit.FNO_IEP14 = FIFOn;
				else
					DevEpMAP13_15Reg.bit.FNO_OEP14 = FIFOn;
				break;
			case USB_EP15:
				if (Dir == EP_DIR_IN)
					DevEpMAP13_15Reg.bit.FNO_IEP15 = FIFOn;
				else
					DevEpMAP13_15Reg.bit.FNO_OEP15 = FIFOn;
				break;
			default:
				break;
		}
		USB_SETREG(USB_DEVEPMAP13_15_REG_OFS, DevEpMAP13_15Reg.reg);

	}
}

/*
    Set FIFO mapping

    This function is internal FIFO setting functions

    @param[in] FIFOn    FIFO number (0~3)
    @param[in] Dir      FIFO direction
                        - @b USB_FIFO_OUT: FIFO direction OUT
                        - @b USB_FIFO_IN: FIFO direction IN
                        - @b USB_FIFO_BI: FIFO bi-direction
    @param[in] EPn      endpoint number (1~8)

    @return void
*/
static void USB_setFIFOMap(USB_FIFO_NUM FIFOn, USB_FIFO_DIR Dir, USB_EP EPn)
{
	T_USB_DEVFIFOMAP_REG FIFOmapReg;
	T_USB_DEVFIFOMAP1_REG FIFOmap1Reg;
	T_USB_DEVFIFOMAP2_REG FIFOmap2Reg;
	T_USB_DEVFIFOMAP3_REG FIFOmap3Reg;

	FIFOmapReg.reg = USB_GETREG(USB_DEVFIFOMAP_REG_OFS);
	FIFOmap1Reg.reg = USB_GETREG(USB_DEVFIFOMAP1_REG_OFS);
	FIFOmap2Reg.reg = USB_GETREG(USB_DEVFIFOMAP2_REG_OFS);
	FIFOmap3Reg.reg = USB_GETREG(USB_DEVFIFOMAP3_REG_OFS);

	switch (FIFOn) {
		case USB_FIFO0:
			FIFOmapReg.bit.EPNO_FIFO0 = EPn;
			FIFOmapReg.bit.Dir_FIFO0 = Dir;
			break;
		case USB_FIFO1:
			FIFOmapReg.bit.EPNO_FIFO1 = EPn;
			FIFOmapReg.bit.Dir_FIFO1 = Dir;
			break;
		case USB_FIFO2:
			FIFOmapReg.bit.EPNO_FIFO2 = EPn;
			FIFOmapReg.bit.Dir_FIFO2 = Dir;
			break;
		case USB_FIFO3:
			FIFOmapReg.bit.EPNO_FIFO3 = EPn;
			FIFOmapReg.bit.Dir_FIFO3 = Dir;
			break;
		case USB_FIFO4:
			FIFOmap1Reg.bit.EPNO_FIFO4 = EPn;
			FIFOmap1Reg.bit.Dir_FIFO4 = Dir;
			break;
		case USB_FIFO5:
			FIFOmap1Reg.bit.EPNO_FIFO5 = EPn;
			FIFOmap1Reg.bit.Dir_FIFO5 = Dir;
			break;
		case USB_FIFO6:
			FIFOmap1Reg.bit.EPNO_FIFO6 = EPn;
			FIFOmap1Reg.bit.Dir_FIFO6 = Dir;
			break;
		case USB_FIFO7:
			FIFOmap1Reg.bit.EPNO_FIFO7 = EPn;
			FIFOmap1Reg.bit.Dir_FIFO7 = Dir;
			break;
		case USB_FIFO8:
			FIFOmap2Reg.bit.EPNO_FIFO8 = EPn;
			FIFOmap2Reg.bit.Dir_FIFO8 = Dir;
			break;
		case USB_FIFO9:
			FIFOmap2Reg.bit.EPNO_FIFO9 = EPn;
			FIFOmap2Reg.bit.Dir_FIFO9 = Dir;
			break;
		case USB_FIFO10:
			FIFOmap2Reg.bit.EPNO_FIFO10 = EPn;
			FIFOmap2Reg.bit.Dir_FIFO10 = Dir;
			break;
		case USB_FIFO11:
			FIFOmap2Reg.bit.EPNO_FIFO11 = EPn;
			FIFOmap2Reg.bit.Dir_FIFO11 = Dir;
			break;
		case USB_FIFO12:
			FIFOmap3Reg.bit.EPNO_FIFO12 = EPn;
			FIFOmap3Reg.bit.Dir_FIFO12 = Dir;
			break;
		case USB_FIFO13:
			FIFOmap3Reg.bit.EPNO_FIFO13 = EPn;
			FIFOmap3Reg.bit.Dir_FIFO13 = Dir;
			break;
		case USB_FIFO14:
			FIFOmap3Reg.bit.EPNO_FIFO14 = EPn;
			FIFOmap3Reg.bit.Dir_FIFO14 = Dir;
			break;
		case USB_FIFO15:
			FIFOmap3Reg.bit.EPNO_FIFO15 = EPn;
			FIFOmap3Reg.bit.Dir_FIFO15 = Dir;
			break;

		default:
			break;
	}

	USB_SETREG(USB_DEVFIFOMAP_REG_OFS,  FIFOmapReg.reg);
	USB_SETREG(USB_DEVFIFOMAP1_REG_OFS, FIFOmap1Reg.reg);
	USB_SETREG(USB_DEVFIFOMAP2_REG_OFS, FIFOmap2Reg.reg);
	USB_SETREG(USB_DEVFIFOMAP3_REG_OFS, FIFOmap3Reg.reg);

}


/*
    Set FIFO configuration

    This function is internal FIFO setting functions

    @param[in] FIFOn        FIFO number (0~3)
    @param[in] BLK_TYP      transfer type
                            - @b EP_TYPE_ISOCHRONOUS: Endpoint transfer type is isochronous
                            - @b EP_TYPE_BULK: Endpoint transfer type is bulk
                            - @b EP_TYPE_INTERRUPT: Endpoint transfer type is interrupt
    @param[in] BLKNO        block number
                            - @b BLKNUM_SINGLE: use single block
                            - @b BLKNUM_DOUBLE: use double block
                            - @b BLKNUM_TRIPLE: use triple block
    @param[in] BLKSZ        block size
                            - @b TRUE: 512~1024 bytes
                            - @b FALSE: <=512 bytes

    @return void
*/
static void USB_setFIFOCfg(USB_FIFO_NUM FIFOn, USB_EP_TYPE BLK_TYP, USB_EP_BLKNUM BLKNO, BOOL BLKSZ)
{
	T_USB_DEVFIFOCFG_REG	FIFOCfgReg;
	T_USB_DEVFIFOCFG1_REG	FIFOCfg1Reg;
	T_USB_DEVFIFOCFG2_REG	FIFOCfg2Reg;
	T_USB_DEVFIFOCFG3_REG	FIFOCfg3Reg;

	DBG_DUMP("FIFO=%d BLK_TYP=%d BLKNO=%d BLKSZ=%d\r\n",(int)FIFOn,(int)BLK_TYP,(int)BLKNO,(int)BLKSZ);

	FIFOCfgReg.reg = USB_GETREG(USB_DEVFIFOCFG_REG_OFS);
	FIFOCfg1Reg.reg = USB_GETREG(USB_DEVFIFOCFG1_REG_OFS);
	FIFOCfg2Reg.reg = USB_GETREG(USB_DEVFIFOCFG2_REG_OFS);
	FIFOCfg3Reg.reg = USB_GETREG(USB_DEVFIFOCFG3_REG_OFS);

	switch (FIFOn) {
	case USB_FIFO0: {
			FIFOCfgReg.bit.BLK_TYP_F0           = BLK_TYP;
			FIFOCfgReg.bit.BLKNO_F0             = BLKNO - 1;
			FIFOCfgReg.bit.BLKSZ_F0             = BLKSZ;
			FIFOCfgReg.bit.EN_F0                = 1;

			if ((BLKNO == BLKNUM_SINGLE) && (BLKSZ == TRUE)) {
				FIFOCfgReg.bit.BLK_TYP_F1     = BLK_TYP;
				FIFOCfgReg.bit.BLKNO_F1       = BLKNO - 1;
				FIFOCfgReg.bit.BLKSZ_F1       = BLKSZ;
				FIFOCfgReg.bit.EN_F1          = 0;
			} else if (BLKNO == BLKNUM_DOUBLE) {
				FIFOCfgReg.bit.BLK_TYP_F1     = BLK_TYP;
				FIFOCfgReg.bit.BLKNO_F1       = BLKNO - 1;
				FIFOCfgReg.bit.BLKSZ_F1       = BLKSZ;
				FIFOCfgReg.bit.EN_F1          = 0;

				if (BLKSZ == TRUE) {
					// Block size : 512~1024 bytes
					FIFOCfgReg.bit.BLK_TYP_F2     = BLK_TYP;
					FIFOCfgReg.bit.BLKNO_F2       = BLKNO - 1;
					FIFOCfgReg.bit.BLKSZ_F2       = BLKSZ;
					FIFOCfgReg.bit.EN_F2          = 0;

					FIFOCfgReg.bit.BLK_TYP_F3     = BLK_TYP;
					FIFOCfgReg.bit.BLKNO_F3       = BLKNO - 1;
					FIFOCfgReg.bit.BLKSZ_F3       = BLKSZ;
					FIFOCfgReg.bit.EN_F3          = 0;
				}
			} else if (BLKNO == BLKNUM_TRIPLE) {
				FIFOCfgReg.bit.BLK_TYP_F1     = BLK_TYP;
				FIFOCfgReg.bit.BLKNO_F1       = BLKNO - 1;
				FIFOCfgReg.bit.BLKSZ_F1       = BLKSZ;
				FIFOCfgReg.bit.EN_F1          = 0;

				FIFOCfgReg.bit.BLK_TYP_F2     = BLK_TYP;
				FIFOCfgReg.bit.BLKNO_F2       = BLKNO - 1;
				FIFOCfgReg.bit.BLKSZ_F2       = BLKSZ;
				FIFOCfgReg.bit.EN_F2          = 0;

				if (BLKSZ == TRUE) {

					FIFOCfgReg.bit.BLK_TYP_F0     = BLK_TYP;
					FIFOCfgReg.bit.BLKNO_F0       = BLKNO - 2;
					FIFOCfgReg.bit.BLKSZ_F0       = BLKSZ;
					FIFOCfgReg.bit.EN_F0          = 1;
					FIFOCfgReg.bit.PINGPONG0      = 1;

					//BLKSZ = 0;

					FIFOCfgReg.bit.BLK_TYP_F1     = BLK_TYP;
					FIFOCfgReg.bit.BLKNO_F1       = BLKNO - 2;
					FIFOCfgReg.bit.BLKSZ_F1       = BLKSZ;
					FIFOCfgReg.bit.EN_F1          = 0;
					FIFOCfgReg.bit.PINGPONG1      = 1;

					FIFOCfgReg.bit.BLK_TYP_F2     = BLK_TYP;
					FIFOCfgReg.bit.BLKNO_F2       = BLKNO - 2;
					FIFOCfgReg.bit.BLKSZ_F2       = BLKSZ;
					FIFOCfgReg.bit.EN_F2          = 0;
					FIFOCfgReg.bit.PINGPONG2      = 1;


					FIFOCfgReg.bit.BLK_TYP_F3      = BLK_TYP;
					FIFOCfgReg.bit.BLKNO_F3        = BLKNO - 2;
					FIFOCfgReg.bit.BLKSZ_F3        = BLKSZ;
					FIFOCfgReg.bit.EN_F3           = 0;
					FIFOCfgReg.bit.PINGPONG3       = 1;

					FIFOCfg1Reg.bit.BLK_TYP_F4     = BLK_TYP;
					FIFOCfg1Reg.bit.BLKNO_F4       = BLKNO - 2;
					FIFOCfg1Reg.bit.BLKSZ_F4       = BLKSZ;
					FIFOCfg1Reg.bit.EN_F4          = 0;
					FIFOCfg1Reg.bit.PINGPONG4      = 1;

					FIFOCfg1Reg.bit.BLK_TYP_F5     = BLK_TYP;
					FIFOCfg1Reg.bit.BLKNO_F5       = BLKNO - 2;
					FIFOCfg1Reg.bit.BLKSZ_F5       = BLKSZ;
					FIFOCfg1Reg.bit.EN_F5          = 0;
					FIFOCfg1Reg.bit.PINGPONG5      = 1;

					FIFOCfg1Reg.bit.BLK_TYP_F6     = BLK_TYP;
					FIFOCfg1Reg.bit.BLKNO_F6       = BLKNO - 2;
					FIFOCfg1Reg.bit.BLKSZ_F6       = BLKSZ;
					FIFOCfg1Reg.bit.EN_F6          = 0;
					FIFOCfg1Reg.bit.PINGPONG6      = 1;

					FIFOCfg1Reg.bit.BLK_TYP_F7     = BLK_TYP;
					FIFOCfg1Reg.bit.BLKNO_F7       = BLKNO - 2;
					FIFOCfg1Reg.bit.BLKSZ_F7       = BLKSZ;
					FIFOCfg1Reg.bit.EN_F7          = 0;
					FIFOCfg1Reg.bit.PINGPONG7      = 1;

					FIFOCfg2Reg.bit.BLK_TYP_F8     = BLK_TYP;
					FIFOCfg2Reg.bit.BLKNO_F8       = BLKNO - 2;
					FIFOCfg2Reg.bit.BLKSZ_F8       = BLKSZ;
					FIFOCfg2Reg.bit.EN_F8          = 0;
					FIFOCfg2Reg.bit.PINGPONG8      = 1;

					FIFOCfg2Reg.bit.BLK_TYP_F9     = BLK_TYP;
					FIFOCfg2Reg.bit.BLKNO_F9       = BLKNO - 2;
					FIFOCfg2Reg.bit.BLKSZ_F9       = BLKSZ;
					FIFOCfg2Reg.bit.EN_F9          = 0;
					FIFOCfg2Reg.bit.PINGPONG9      = 1;

					FIFOCfg2Reg.bit.BLK_TYP_F10     = BLK_TYP;
					FIFOCfg2Reg.bit.BLKNO_F10       = BLKNO - 2;
					FIFOCfg2Reg.bit.BLKSZ_F10       = BLKSZ;
					FIFOCfg2Reg.bit.EN_F10          = 0;
					FIFOCfg2Reg.bit.PINGPONG10      = 1;

					FIFOCfg2Reg.bit.BLK_TYP_F11     = BLK_TYP;
					FIFOCfg2Reg.bit.BLKNO_F11       = BLKNO - 2;
					FIFOCfg2Reg.bit.BLKSZ_F11       = BLKSZ;
					FIFOCfg2Reg.bit.EN_F11          = 0;
					FIFOCfg2Reg.bit.PINGPONG11      = 1;

				}

			}
		}
		break;

	case USB_FIFO1: {
			FIFOCfgReg.bit.BLK_TYP_F1     = BLK_TYP;
			FIFOCfgReg.bit.BLKNO_F1       = BLKNO - 1;
			FIFOCfgReg.bit.BLKSZ_F1       = BLKSZ;
			FIFOCfgReg.bit.EN_F1          = 1;

			if ((BLKNO == BLKNUM_SINGLE) && (BLKSZ == TRUE)) {
				FIFOCfgReg.bit.BLK_TYP_F2     = BLK_TYP;
				FIFOCfgReg.bit.BLKNO_F2       = BLKNO - 1;
				FIFOCfgReg.bit.BLKSZ_F2       = BLKSZ;
				FIFOCfgReg.bit.EN_F2          = 0;
			} else if (BLKNO == BLKNUM_DOUBLE) {
				FIFOCfgReg.bit.BLK_TYP_F2     = BLK_TYP;
				FIFOCfgReg.bit.BLKNO_F2       = BLKNO - 1;
				FIFOCfgReg.bit.BLKSZ_F2       = BLKSZ;
				FIFOCfgReg.bit.EN_F2          = 0;

				if (BLKSZ == TRUE) {
					// Block size : 512~1024 bytes
					FIFOCfgReg.bit.BLK_TYP_F3     = BLK_TYP;
					FIFOCfgReg.bit.BLKNO_F3       = BLKNO - 1;
					FIFOCfgReg.bit.BLKSZ_F3       = BLKSZ;
					FIFOCfgReg.bit.EN_F3          = 0;

					FIFOCfg1Reg.bit.BLK_TYP_F4     = BLK_TYP;
					FIFOCfg1Reg.bit.BLKNO_F4       = BLKNO - 1;
					FIFOCfg1Reg.bit.BLKSZ_F4       = BLKSZ;
					FIFOCfg1Reg.bit.EN_F4          = 0;
				}
			} else if (BLKNO == BLKNUM_TRIPLE) {
				FIFOCfgReg.bit.BLK_TYP_F2     = BLK_TYP;
				FIFOCfgReg.bit.BLKNO_F2       = BLKNO - 1;
				FIFOCfgReg.bit.BLKSZ_F2       = BLKSZ;
				FIFOCfgReg.bit.EN_F2          = 0;

				FIFOCfgReg.bit.BLK_TYP_F3     = BLK_TYP;
				FIFOCfgReg.bit.BLKNO_F3       = BLKNO - 1;
				FIFOCfgReg.bit.BLKSZ_F3       = BLKSZ;
				FIFOCfgReg.bit.EN_F3          = 0;

				if (BLKSZ == TRUE) {

					FIFOCfg1Reg.bit.BLK_TYP_F4     = BLK_TYP;
					FIFOCfg1Reg.bit.BLKNO_F4       = BLKNO - 1;
					FIFOCfg1Reg.bit.BLKSZ_F4       = BLKSZ;
					FIFOCfg1Reg.bit.EN_F4          = 0;

					FIFOCfg1Reg.bit.BLK_TYP_F5     = BLK_TYP;
					FIFOCfg1Reg.bit.BLKNO_F5       = BLKNO - 1;
					FIFOCfg1Reg.bit.BLKSZ_F5       = BLKSZ;
					FIFOCfg1Reg.bit.EN_F5          = 0;

					FIFOCfg1Reg.bit.BLK_TYP_F6     = BLK_TYP;
					FIFOCfg1Reg.bit.BLKNO_F6       = BLKNO - 1;
					FIFOCfg1Reg.bit.BLKSZ_F6       = BLKSZ;
					FIFOCfg1Reg.bit.EN_F6          = 0;
				}
			}
		}
		break;

	case USB_FIFO2: {
			FIFOCfgReg.bit.BLK_TYP_F2     = BLK_TYP;
			FIFOCfgReg.bit.BLKNO_F2       = BLKNO - 1;
			FIFOCfgReg.bit.BLKSZ_F2       = BLKSZ;
			FIFOCfgReg.bit.EN_F2          = 1;


			if ((BLKNO == BLKNUM_SINGLE) && (BLKSZ == TRUE)) {
				FIFOCfgReg.bit.BLK_TYP_F3     = BLK_TYP;
				FIFOCfgReg.bit.BLKNO_F3       = BLKNO - 1;
				FIFOCfgReg.bit.BLKSZ_F3       = BLKSZ;
				FIFOCfgReg.bit.EN_F3          = 0;
			} else if (BLKNO == BLKNUM_DOUBLE) {
				FIFOCfgReg.bit.BLK_TYP_F3     = BLK_TYP;
				FIFOCfgReg.bit.BLKNO_F3       = BLKNO - 1;
				FIFOCfgReg.bit.BLKSZ_F3       = BLKSZ;
				FIFOCfgReg.bit.EN_F3          = 0;

				if (BLKSZ == TRUE) {
					// Block size : 512~1024 bytes
					FIFOCfg1Reg.bit.BLK_TYP_F4     = BLK_TYP;
					FIFOCfg1Reg.bit.BLKNO_F4       = BLKNO - 1;
					FIFOCfg1Reg.bit.BLKSZ_F4       = BLKSZ;
					FIFOCfg1Reg.bit.EN_F4          = 0;

					FIFOCfg1Reg.bit.BLK_TYP_F5     = BLK_TYP;
					FIFOCfg1Reg.bit.BLKNO_F5       = BLKNO - 1;
					FIFOCfg1Reg.bit.BLKSZ_F5       = BLKSZ;
					FIFOCfg1Reg.bit.EN_F5          = 0;
				}
			} else if (BLKNO == BLKNUM_TRIPLE) {
				FIFOCfgReg.bit.BLK_TYP_F3     = BLK_TYP;
				FIFOCfgReg.bit.BLKNO_F3       = BLKNO - 1;
				FIFOCfgReg.bit.BLKSZ_F3       = BLKSZ;
				FIFOCfgReg.bit.EN_F3          = 0;

				FIFOCfg1Reg.bit.BLK_TYP_F4     = BLK_TYP;
				FIFOCfg1Reg.bit.BLKNO_F4       = BLKNO - 1;
				FIFOCfg1Reg.bit.BLKSZ_F4       = BLKSZ;
				FIFOCfg1Reg.bit.EN_F4          = 0;

				if (BLKSZ == TRUE) {

					FIFOCfg1Reg.bit.BLK_TYP_F5     = BLK_TYP;
					FIFOCfg1Reg.bit.BLKNO_F5       = BLKNO - 1;
					FIFOCfg1Reg.bit.BLKSZ_F5       = BLKSZ;
					FIFOCfg1Reg.bit.EN_F5          = 0;

					FIFOCfg1Reg.bit.BLK_TYP_F6     = BLK_TYP;
					FIFOCfg1Reg.bit.BLKNO_F6       = BLKNO - 1;
					FIFOCfg1Reg.bit.BLKSZ_F6       = BLKSZ;
					FIFOCfg1Reg.bit.EN_F6          = 0;

					FIFOCfg1Reg.bit.BLK_TYP_F7     = BLK_TYP;
					FIFOCfg1Reg.bit.BLKNO_F7       = BLKNO - 1;
					FIFOCfg1Reg.bit.BLKSZ_F7       = BLKSZ;
					FIFOCfg1Reg.bit.EN_F7          = 0;
				}
			}
		}
		break;

	case USB_FIFO3: {
			FIFOCfgReg.bit.BLK_TYP_F3     = BLK_TYP;
			FIFOCfgReg.bit.BLKNO_F3       = BLKNO - 1;
			FIFOCfgReg.bit.BLKSZ_F3       = BLKSZ;
			FIFOCfgReg.bit.EN_F3          = 1;


			if ((BLKNO == BLKNUM_SINGLE) && (BLKSZ == TRUE)) {
				FIFOCfg1Reg.bit.BLK_TYP_F4     = BLK_TYP;
				FIFOCfg1Reg.bit.BLKNO_F4       = BLKNO - 1;
				FIFOCfg1Reg.bit.BLKSZ_F4       = BLKSZ;
				FIFOCfg1Reg.bit.EN_F4          = 0;
			} else if (BLKNO == BLKNUM_DOUBLE) {
				FIFOCfg1Reg.bit.BLK_TYP_F4     = BLK_TYP;
				FIFOCfg1Reg.bit.BLKNO_F4       = BLKNO - 1;
				FIFOCfg1Reg.bit.BLKSZ_F4       = BLKSZ;
				FIFOCfg1Reg.bit.EN_F4          = 0;

				if (BLKSZ == TRUE) {
					// Block size : 512~1024 bytes
					FIFOCfg1Reg.bit.BLK_TYP_F5     = BLK_TYP;
					FIFOCfg1Reg.bit.BLKNO_F5       = BLKNO - 1;
					FIFOCfg1Reg.bit.BLKSZ_F5       = BLKSZ;
					FIFOCfg1Reg.bit.EN_F5          = 0;

					FIFOCfg1Reg.bit.BLK_TYP_F6     = BLK_TYP;
					FIFOCfg1Reg.bit.BLKNO_F6       = BLKNO - 1;
					FIFOCfg1Reg.bit.BLKSZ_F6       = BLKSZ;
					FIFOCfg1Reg.bit.EN_F6          = 0;
				}
			} else if (BLKNO == BLKNUM_TRIPLE) {
				FIFOCfg1Reg.bit.BLK_TYP_F4     = BLK_TYP;
				FIFOCfg1Reg.bit.BLKNO_F4       = BLKNO - 1;
				FIFOCfg1Reg.bit.BLKSZ_F4       = BLKSZ;
				FIFOCfg1Reg.bit.EN_F4          = 0;

				FIFOCfg1Reg.bit.BLK_TYP_F5     = BLK_TYP;
				FIFOCfg1Reg.bit.BLKNO_F5       = BLKNO - 1;
				FIFOCfg1Reg.bit.BLKSZ_F5       = BLKSZ;
				FIFOCfg1Reg.bit.EN_F5          = 0;

				if (BLKSZ == TRUE) {

					FIFOCfg1Reg.bit.BLK_TYP_F6     = BLK_TYP;
					FIFOCfg1Reg.bit.BLKNO_F6       = BLKNO - 1;
					FIFOCfg1Reg.bit.BLKSZ_F6       = BLKSZ;
					FIFOCfg1Reg.bit.EN_F6          = 0;

					FIFOCfg1Reg.bit.BLK_TYP_F7     = BLK_TYP;
					FIFOCfg1Reg.bit.BLKNO_F7       = BLKNO - 1;
					FIFOCfg1Reg.bit.BLKSZ_F7       = BLKSZ;
					FIFOCfg1Reg.bit.EN_F7          = 0;

					FIFOCfg2Reg.bit.BLK_TYP_F8     = BLK_TYP;
					FIFOCfg2Reg.bit.BLKNO_F8       = BLKNO - 1;
					FIFOCfg2Reg.bit.BLKSZ_F8       = BLKSZ;
					FIFOCfg2Reg.bit.EN_F8          = 0;
				}
			}
		}
		break;



	case USB_FIFO4: {
			FIFOCfg1Reg.bit.BLK_TYP_F4           = BLK_TYP;
			FIFOCfg1Reg.bit.BLKNO_F4             = BLKNO - 1;
			FIFOCfg1Reg.bit.BLKSZ_F4             = BLKSZ;
			FIFOCfg1Reg.bit.EN_F4                = 1;

			if ((BLKNO == BLKNUM_SINGLE) && (BLKSZ == TRUE)) {
				FIFOCfg1Reg.bit.BLK_TYP_F5     = BLK_TYP;
				FIFOCfg1Reg.bit.BLKNO_F5       = BLKNO - 1;
				FIFOCfg1Reg.bit.BLKSZ_F5       = BLKSZ;
				FIFOCfg1Reg.bit.EN_F5          = 0;
			} else if (BLKNO == BLKNUM_DOUBLE) {
				FIFOCfg1Reg.bit.BLK_TYP_F5     = BLK_TYP;
				FIFOCfg1Reg.bit.BLKNO_F5       = BLKNO - 1;
				FIFOCfg1Reg.bit.BLKSZ_F5       = BLKSZ;
				FIFOCfg1Reg.bit.EN_F5          = 0;

				if (BLKSZ == TRUE) {
					// Block size : 512~1024 bytes
					FIFOCfg1Reg.bit.BLK_TYP_F6     = BLK_TYP;
					FIFOCfg1Reg.bit.BLKNO_F6       = BLKNO - 1;
					FIFOCfg1Reg.bit.BLKSZ_F6       = BLKSZ;
					FIFOCfg1Reg.bit.EN_F6          = 0;

					FIFOCfg1Reg.bit.BLK_TYP_F7     = BLK_TYP;
					FIFOCfg1Reg.bit.BLKNO_F7       = BLKNO - 1;
					FIFOCfg1Reg.bit.BLKSZ_F7       = BLKSZ;
					FIFOCfg1Reg.bit.EN_F7          = 0;
				}
			} else if (BLKNO == BLKNUM_TRIPLE) {
				FIFOCfg1Reg.bit.BLK_TYP_F5     = BLK_TYP;
				FIFOCfg1Reg.bit.BLKNO_F5       = BLKNO - 1;
				FIFOCfg1Reg.bit.BLKSZ_F5       = BLKSZ;
				FIFOCfg1Reg.bit.EN_F5          = 0;

				FIFOCfg1Reg.bit.BLK_TYP_F6     = BLK_TYP;
				FIFOCfg1Reg.bit.BLKNO_F6       = BLKNO - 1;
				FIFOCfg1Reg.bit.BLKSZ_F6       = BLKSZ;
				FIFOCfg1Reg.bit.EN_F6          = 0;

				if (BLKSZ == TRUE) {
					FIFOCfg1Reg.bit.BLK_TYP_F7     = BLK_TYP;
					FIFOCfg1Reg.bit.BLKNO_F7       = BLKNO - 1;
					FIFOCfg1Reg.bit.BLKSZ_F7       = BLKSZ;
					FIFOCfg1Reg.bit.EN_F7          = 0;

					FIFOCfg2Reg.bit.BLK_TYP_F8     = BLK_TYP;
					FIFOCfg2Reg.bit.BLKNO_F8       = BLKNO - 1;
					FIFOCfg2Reg.bit.BLKSZ_F8       = BLKSZ;
					FIFOCfg2Reg.bit.EN_F8          = 0;

					FIFOCfg2Reg.bit.BLK_TYP_F9     = BLK_TYP;
					FIFOCfg2Reg.bit.BLKNO_F9       = BLKNO - 1;
					FIFOCfg2Reg.bit.BLKSZ_F9       = BLKSZ;
					FIFOCfg2Reg.bit.EN_F9          = 0;
				}
			}
		}
		break;

	case USB_FIFO5: {
			FIFOCfg1Reg.bit.BLK_TYP_F5     = BLK_TYP;
			FIFOCfg1Reg.bit.BLKNO_F5       = BLKNO - 1;
			FIFOCfg1Reg.bit.BLKSZ_F5       = BLKSZ;
			FIFOCfg1Reg.bit.EN_F5          = 1;

			if ((BLKNO == BLKNUM_SINGLE) && (BLKSZ == TRUE)) {
				FIFOCfg1Reg.bit.BLK_TYP_F6     = BLK_TYP;
				FIFOCfg1Reg.bit.BLKNO_F6       = BLKNO - 1;
				FIFOCfg1Reg.bit.BLKSZ_F6       = BLKSZ;
				FIFOCfg1Reg.bit.EN_F6          = 0;
			} else if (BLKNO == BLKNUM_DOUBLE) {
				FIFOCfg1Reg.bit.BLK_TYP_F6     = BLK_TYP;
				FIFOCfg1Reg.bit.BLKNO_F6       = BLKNO - 1;
				FIFOCfg1Reg.bit.BLKSZ_F6       = BLKSZ;
				FIFOCfg1Reg.bit.EN_F6          = 0;

				if (BLKSZ == TRUE) {
					FIFOCfg1Reg.bit.BLK_TYP_F7     = BLK_TYP;
					FIFOCfg1Reg.bit.BLKNO_F7       = BLKNO - 1;
					FIFOCfg1Reg.bit.BLKSZ_F7       = BLKSZ;
					FIFOCfg1Reg.bit.EN_F7          = 0;

					FIFOCfg2Reg.bit.BLK_TYP_F8     = BLK_TYP;
					FIFOCfg2Reg.bit.BLKNO_F8       = BLKNO - 1;
					FIFOCfg2Reg.bit.BLKSZ_F8       = BLKSZ;
					FIFOCfg2Reg.bit.EN_F8          = 0;
				}
			} else if (BLKNO == BLKNUM_TRIPLE) {
				FIFOCfg1Reg.bit.BLK_TYP_F6     = BLK_TYP;
				FIFOCfg1Reg.bit.BLKNO_F6       = BLKNO - 1;
				FIFOCfg1Reg.bit.BLKSZ_F6       = BLKSZ;
				FIFOCfg1Reg.bit.EN_F6          = 0;

				FIFOCfg1Reg.bit.BLK_TYP_F7     = BLK_TYP;
				FIFOCfg1Reg.bit.BLKNO_F7       = BLKNO - 1;
				FIFOCfg1Reg.bit.BLKSZ_F7       = BLKSZ;
				FIFOCfg1Reg.bit.EN_F7          = 0;

				if (BLKSZ == TRUE) {
					FIFOCfg2Reg.bit.BLK_TYP_F8     = BLK_TYP;
					FIFOCfg2Reg.bit.BLKNO_F8       = BLKNO - 1;
					FIFOCfg2Reg.bit.BLKSZ_F8       = BLKSZ;
					FIFOCfg2Reg.bit.EN_F8          = 0;

					FIFOCfg2Reg.bit.BLK_TYP_F9     = BLK_TYP;
					FIFOCfg2Reg.bit.BLKNO_F9       = BLKNO - 1;
					FIFOCfg2Reg.bit.BLKSZ_F9       = BLKSZ;
					FIFOCfg2Reg.bit.EN_F9          = 0;

					FIFOCfg2Reg.bit.BLK_TYP_F10    = BLK_TYP;
					FIFOCfg2Reg.bit.BLKNO_F10      = BLKNO - 1;
					FIFOCfg2Reg.bit.BLKSZ_F10      = BLKSZ;
					FIFOCfg2Reg.bit.EN_F10         = 0;
				}
			}
		}
		break;

	case USB_FIFO6: {
			FIFOCfg1Reg.bit.BLK_TYP_F6           = BLK_TYP;
			FIFOCfg1Reg.bit.BLKNO_F6             = BLKNO - 1;
			FIFOCfg1Reg.bit.BLKSZ_F6             = BLKSZ;
			FIFOCfg1Reg.bit.EN_F6                = 1;

			if ((BLKNO == BLKNUM_SINGLE) && (BLKSZ == TRUE)) {
				FIFOCfg1Reg.bit.BLK_TYP_F7     = BLK_TYP;
				FIFOCfg1Reg.bit.BLKNO_F7       = BLKNO - 1;
				FIFOCfg1Reg.bit.BLKSZ_F7       = BLKSZ;
				FIFOCfg1Reg.bit.EN_F7          = 0;
			} else if (BLKNO == BLKNUM_DOUBLE) {
				FIFOCfg1Reg.bit.BLK_TYP_F7     = BLK_TYP;
				FIFOCfg1Reg.bit.BLKNO_F7       = BLKNO - 1;
				FIFOCfg1Reg.bit.BLKSZ_F7       = BLKSZ;
				FIFOCfg1Reg.bit.EN_F7          = 0;

				if (BLKSZ == TRUE) {
					// Block size : 512~1024 bytes
					FIFOCfg2Reg.bit.BLK_TYP_F8     = BLK_TYP;
					FIFOCfg2Reg.bit.BLKNO_F8       = BLKNO - 1;
					FIFOCfg2Reg.bit.BLKSZ_F8       = BLKSZ;
					FIFOCfg2Reg.bit.EN_F8          = 0;

					FIFOCfg2Reg.bit.BLK_TYP_F9     = BLK_TYP;
					FIFOCfg2Reg.bit.BLKNO_F9       = BLKNO - 1;
					FIFOCfg2Reg.bit.BLKSZ_F9       = BLKSZ;
					FIFOCfg2Reg.bit.EN_F9          = 0;
				}
			} else if (BLKNO == BLKNUM_TRIPLE) {
				FIFOCfg1Reg.bit.BLK_TYP_F7     = BLK_TYP;
				FIFOCfg1Reg.bit.BLKNO_F7       = BLKNO - 1;
				FIFOCfg1Reg.bit.BLKSZ_F7       = BLKSZ;
				FIFOCfg1Reg.bit.EN_F7          = 0;

				FIFOCfg2Reg.bit.BLK_TYP_F8     = BLK_TYP;
				FIFOCfg2Reg.bit.BLKNO_F8       = BLKNO - 1;
				FIFOCfg2Reg.bit.BLKSZ_F8       = BLKSZ;
				FIFOCfg2Reg.bit.EN_F8          = 0;

				if (BLKSZ == TRUE) {

					FIFOCfg2Reg.bit.BLK_TYP_F9      = BLK_TYP;
					FIFOCfg2Reg.bit.BLKNO_F9        = BLKNO - 1;
					FIFOCfg2Reg.bit.BLKSZ_F9        = BLKSZ;
					FIFOCfg2Reg.bit.EN_F9           = 0;

					FIFOCfg2Reg.bit.BLK_TYP_F10     = BLK_TYP;
					FIFOCfg2Reg.bit.BLKNO_F10       = BLKNO - 1;
					FIFOCfg2Reg.bit.BLKSZ_F10       = BLKSZ;
					FIFOCfg2Reg.bit.EN_F10          = 0;

					FIFOCfg2Reg.bit.BLK_TYP_F11     = BLK_TYP;
					FIFOCfg2Reg.bit.BLKNO_F11       = BLKNO - 1;
					FIFOCfg2Reg.bit.BLKSZ_F11       = BLKSZ;
					FIFOCfg2Reg.bit.EN_F11          = 0;
				}
			}
		}
		break;

	case USB_FIFO7: {
			FIFOCfg1Reg.bit.BLK_TYP_F7     = BLK_TYP;
			FIFOCfg1Reg.bit.BLKNO_F7       = BLKNO - 1;
			FIFOCfg1Reg.bit.BLKSZ_F7       = BLKSZ;
			FIFOCfg1Reg.bit.EN_F7          = 1;

			if ((BLKNO == BLKNUM_SINGLE) && (BLKSZ == TRUE)) {
				FIFOCfg2Reg.bit.BLK_TYP_F8     = BLK_TYP;
				FIFOCfg2Reg.bit.BLKNO_F8       = BLKNO - 1;
				FIFOCfg2Reg.bit.BLKSZ_F8       = BLKSZ;
				FIFOCfg2Reg.bit.EN_F8          = 0;
			} else if (BLKNO == BLKNUM_DOUBLE) {
				FIFOCfg2Reg.bit.BLK_TYP_F8     = BLK_TYP;
				FIFOCfg2Reg.bit.BLKNO_F8       = BLKNO - 1;
				FIFOCfg2Reg.bit.BLKSZ_F8       = BLKSZ;
				FIFOCfg2Reg.bit.EN_F8          = 0;

				if (BLKSZ == TRUE) {
					// Block size : 512~1024 bytes
					FIFOCfg2Reg.bit.BLK_TYP_F9     = BLK_TYP;
					FIFOCfg2Reg.bit.BLKNO_F9       = BLKNO - 1;
					FIFOCfg2Reg.bit.BLKSZ_F9       = BLKSZ;
					FIFOCfg2Reg.bit.EN_F9          = 0;

					FIFOCfg2Reg.bit.BLK_TYP_F10     = BLK_TYP;
					FIFOCfg2Reg.bit.BLKNO_F10       = BLKNO - 1;
					FIFOCfg2Reg.bit.BLKSZ_F10       = BLKSZ;
					FIFOCfg2Reg.bit.EN_F10          = 0;
				}
			} else if (BLKNO == BLKNUM_TRIPLE) {
				FIFOCfg2Reg.bit.BLK_TYP_F8     = BLK_TYP;
				FIFOCfg2Reg.bit.BLKNO_F8       = BLKNO - 1;
				FIFOCfg2Reg.bit.BLKSZ_F8       = BLKSZ;
				FIFOCfg2Reg.bit.EN_F8          = 0;

				FIFOCfg2Reg.bit.BLK_TYP_F9     = BLK_TYP;
				FIFOCfg2Reg.bit.BLKNO_F9       = BLKNO - 1;
				FIFOCfg2Reg.bit.BLKSZ_F9       = BLKSZ;
				FIFOCfg2Reg.bit.EN_F9          = 0;

				if (BLKSZ == TRUE) {
					FIFOCfg2Reg.bit.BLK_TYP_F10     = BLK_TYP;
					FIFOCfg2Reg.bit.BLKNO_F10       = BLKNO - 1;
					FIFOCfg2Reg.bit.BLKSZ_F10       = BLKSZ;
					FIFOCfg2Reg.bit.EN_F10          = 0;

					FIFOCfg2Reg.bit.BLK_TYP_F11     = BLK_TYP;
					FIFOCfg2Reg.bit.BLKNO_F11       = BLKNO - 1;
					FIFOCfg2Reg.bit.BLKSZ_F11       = BLKSZ;
					FIFOCfg2Reg.bit.EN_F11          = 0;

					FIFOCfg3Reg.bit.BLK_TYP_F12     = BLK_TYP;
					FIFOCfg3Reg.bit.BLKNO_F12       = BLKNO - 1;
					FIFOCfg3Reg.bit.BLKSZ_F12       = BLKSZ;
					FIFOCfg3Reg.bit.EN_F12          = 0;
				}
			}
		}
		break;


	case USB_FIFO8: {
			FIFOCfg2Reg.bit.BLK_TYP_F8           = BLK_TYP;
			FIFOCfg2Reg.bit.BLKNO_F8             = BLKNO - 1;
			FIFOCfg2Reg.bit.BLKSZ_F8             = BLKSZ;
			FIFOCfg2Reg.bit.EN_F8                = 1;

			if ((BLKNO == BLKNUM_SINGLE) && (BLKSZ == TRUE)) {
				FIFOCfg2Reg.bit.BLK_TYP_F9     = BLK_TYP;
				FIFOCfg2Reg.bit.BLKNO_F9       = BLKNO - 1;
				FIFOCfg2Reg.bit.BLKSZ_F9       = BLKSZ;
				FIFOCfg2Reg.bit.EN_F9          = 0;
			} else if (BLKNO == BLKNUM_DOUBLE) {
				FIFOCfg2Reg.bit.BLK_TYP_F9     = BLK_TYP;
				FIFOCfg2Reg.bit.BLKNO_F9       = BLKNO - 1;
				FIFOCfg2Reg.bit.BLKSZ_F9       = BLKSZ;
				FIFOCfg2Reg.bit.EN_F9          = 0;

				if (BLKSZ == TRUE) {
					// Block size : 512~1024 bytes
					FIFOCfg2Reg.bit.BLK_TYP_F10     = BLK_TYP;
					FIFOCfg2Reg.bit.BLKNO_F10       = BLKNO - 1;
					FIFOCfg2Reg.bit.BLKSZ_F10       = BLKSZ;
					FIFOCfg2Reg.bit.EN_F10          = 0;

					FIFOCfg2Reg.bit.BLK_TYP_F11     = BLK_TYP;
					FIFOCfg2Reg.bit.BLKNO_F11       = BLKNO - 1;
					FIFOCfg2Reg.bit.BLKSZ_F11       = BLKSZ;
					FIFOCfg2Reg.bit.EN_F11          = 0;
				}
			} else if (BLKNO == BLKNUM_TRIPLE) {
				FIFOCfg2Reg.bit.BLK_TYP_F9     = BLK_TYP;
				FIFOCfg2Reg.bit.BLKNO_F9       = BLKNO - 1;
				FIFOCfg2Reg.bit.BLKSZ_F9       = BLKSZ;
				FIFOCfg2Reg.bit.EN_F9          = 0;

				FIFOCfg2Reg.bit.BLK_TYP_F10     = BLK_TYP;
				FIFOCfg2Reg.bit.BLKNO_F10       = BLKNO - 1;
				FIFOCfg2Reg.bit.BLKSZ_F10       = BLKSZ;
				FIFOCfg2Reg.bit.EN_F10          = 0;

				// Block size : 512~1024 bytes. Exceed boundary.
				if (BLKSZ == TRUE) {
					FIFOCfg2Reg.bit.BLK_TYP_F11     = BLK_TYP;
					FIFOCfg2Reg.bit.BLKNO_F11       = BLKNO - 1;
					FIFOCfg2Reg.bit.BLKSZ_F11       = BLKSZ;
					FIFOCfg2Reg.bit.EN_F11          = 0;

					FIFOCfg3Reg.bit.BLK_TYP_F12     = BLK_TYP;
					FIFOCfg3Reg.bit.BLKNO_F12       = BLKNO - 1;
					FIFOCfg3Reg.bit.BLKSZ_F12       = BLKSZ;
					FIFOCfg3Reg.bit.EN_F12          = 0;

					FIFOCfg3Reg.bit.BLK_TYP_F13     = BLK_TYP;
					FIFOCfg3Reg.bit.BLKNO_F13       = BLKNO - 1;
					FIFOCfg3Reg.bit.BLKSZ_F13       = BLKSZ;
					FIFOCfg3Reg.bit.EN_F13          = 0;
				}
			}
		}
		break;

	case USB_FIFO9: {
			FIFOCfg2Reg.bit.BLK_TYP_F9     = BLK_TYP;
			FIFOCfg2Reg.bit.BLKNO_F9       = BLKNO - 1;
			FIFOCfg2Reg.bit.BLKSZ_F9       = BLKSZ;
			FIFOCfg2Reg.bit.EN_F9          = 1;

			if ((BLKNO == BLKNUM_SINGLE) && (BLKSZ == TRUE)) {
				FIFOCfg2Reg.bit.BLK_TYP_F10     = BLK_TYP;
				FIFOCfg2Reg.bit.BLKNO_F10       = BLKNO - 1;
				FIFOCfg2Reg.bit.BLKSZ_F10       = BLKSZ;
				FIFOCfg2Reg.bit.EN_F10          = 0;
			} else if (BLKNO == BLKNUM_DOUBLE) {
				FIFOCfg2Reg.bit.BLK_TYP_F10     = BLK_TYP;
				FIFOCfg2Reg.bit.BLKNO_F10       = BLKNO - 1;
				FIFOCfg2Reg.bit.BLKSZ_F10       = BLKSZ;
				FIFOCfg2Reg.bit.EN_F10          = 0;

				// Block size : 512~1024 bytes. Exceed boundary.
				if (BLKSZ == TRUE) {
					FIFOCfg2Reg.bit.BLK_TYP_F11     = BLK_TYP;
					FIFOCfg2Reg.bit.BLKNO_F11       = BLKNO - 1;
					FIFOCfg2Reg.bit.BLKSZ_F11       = BLKSZ;
					FIFOCfg2Reg.bit.EN_F11          = 0;

					FIFOCfg3Reg.bit.BLK_TYP_F12     = BLK_TYP;
					FIFOCfg3Reg.bit.BLKNO_F12       = BLKNO - 1;
					FIFOCfg3Reg.bit.BLKSZ_F12       = BLKSZ;
					FIFOCfg3Reg.bit.EN_F12          = 0;
				}
			} else if (BLKNO == BLKNUM_TRIPLE) {
				FIFOCfg2Reg.bit.BLK_TYP_F10     = BLK_TYP;
				FIFOCfg2Reg.bit.BLKNO_F10       = BLKNO - 1;
				FIFOCfg2Reg.bit.BLKSZ_F10       = BLKSZ;
				FIFOCfg2Reg.bit.EN_F10          = 0;

				FIFOCfg2Reg.bit.BLK_TYP_F11     = BLK_TYP;
				FIFOCfg2Reg.bit.BLKNO_F11       = BLKNO - 1;
				FIFOCfg2Reg.bit.BLKSZ_F11       = BLKSZ;
				FIFOCfg2Reg.bit.EN_F11          = 0;

				// Block size : 512~1024 bytes. Exceed boundary.
				if (BLKSZ == TRUE) {
					FIFOCfg3Reg.bit.BLK_TYP_F12     = BLK_TYP;
					FIFOCfg3Reg.bit.BLKNO_F12       = BLKNO - 1;
					FIFOCfg3Reg.bit.BLKSZ_F12       = BLKSZ;
					FIFOCfg3Reg.bit.EN_F12          = 0;

					FIFOCfg3Reg.bit.BLK_TYP_F13     = BLK_TYP;
					FIFOCfg3Reg.bit.BLKNO_F13       = BLKNO - 1;
					FIFOCfg3Reg.bit.BLKSZ_F13       = BLKSZ;
					FIFOCfg3Reg.bit.EN_F13          = 0;

					FIFOCfg3Reg.bit.BLK_TYP_F14    = BLK_TYP;
					FIFOCfg3Reg.bit.BLKNO_F14      = BLKNO - 1;
					FIFOCfg3Reg.bit.BLKSZ_F14      = BLKSZ;
					FIFOCfg3Reg.bit.EN_F14         = 0;
				}
			}
		}
		break;

	case USB_FIFO10: {
			FIFOCfg2Reg.bit.BLK_TYP_F10     = BLK_TYP;
			FIFOCfg2Reg.bit.BLKNO_F10       = BLKNO - 1;
			FIFOCfg2Reg.bit.BLKSZ_F10       = BLKSZ;
			FIFOCfg2Reg.bit.EN_F10          = 1;

			if ((BLKNO == BLKNUM_SINGLE) && (BLKSZ == TRUE)) {
				FIFOCfg2Reg.bit.BLK_TYP_F11     = BLK_TYP;
				FIFOCfg2Reg.bit.BLKNO_F11       = BLKNO - 1;
				FIFOCfg2Reg.bit.BLKSZ_F11       = BLKSZ;
				FIFOCfg2Reg.bit.EN_F11          = 0;
			} else if (BLKNO == BLKNUM_DOUBLE) {
				FIFOCfg2Reg.bit.BLK_TYP_F11     = BLK_TYP;
				FIFOCfg2Reg.bit.BLKNO_F11       = BLKNO - 1;
				FIFOCfg2Reg.bit.BLKSZ_F11       = BLKSZ;
				FIFOCfg2Reg.bit.EN_F11          = 0;

				// Block size : 512~1024 bytes
				if (BLKSZ == TRUE) {
					// Block size : 512~1024 bytes
					FIFOCfg3Reg.bit.BLK_TYP_F12     = BLK_TYP;
					FIFOCfg3Reg.bit.BLKNO_F12       = BLKNO - 1;
					FIFOCfg3Reg.bit.BLKSZ_F12       = BLKSZ;
					FIFOCfg3Reg.bit.EN_F12          = 0;

					FIFOCfg3Reg.bit.BLK_TYP_F13     = BLK_TYP;
					FIFOCfg3Reg.bit.BLKNO_F13       = BLKNO - 1;
					FIFOCfg3Reg.bit.BLKSZ_F13       = BLKSZ;
					FIFOCfg3Reg.bit.EN_F13          = 0;
				}
			} else if (BLKNO == BLKNUM_TRIPLE) {
				FIFOCfg2Reg.bit.BLK_TYP_F11     = BLK_TYP;
				FIFOCfg2Reg.bit.BLKNO_F11       = BLKNO - 1;
				FIFOCfg2Reg.bit.BLKSZ_F11       = BLKSZ;
				FIFOCfg2Reg.bit.EN_F11          = 0;

				FIFOCfg3Reg.bit.BLK_TYP_F12     = BLK_TYP;
				FIFOCfg3Reg.bit.BLKNO_F12       = BLKNO - 1;
				FIFOCfg3Reg.bit.BLKSZ_F12       = BLKSZ;
				FIFOCfg3Reg.bit.EN_F12          = 0;

				if (BLKSZ == TRUE) {
					FIFOCfg3Reg.bit.BLK_TYP_F13      = BLK_TYP;
					FIFOCfg3Reg.bit.BLKNO_F13        = BLKNO - 1;
					FIFOCfg3Reg.bit.BLKSZ_F13        = BLKSZ;
					FIFOCfg3Reg.bit.EN_F13           = 0;

					FIFOCfg3Reg.bit.BLK_TYP_F14     = BLK_TYP;
					FIFOCfg3Reg.bit.BLKNO_F14       = BLKNO - 1;
					FIFOCfg3Reg.bit.BLKSZ_F14       = BLKSZ;
					FIFOCfg3Reg.bit.EN_F14          = 0;

					FIFOCfg3Reg.bit.BLK_TYP_F15     = BLK_TYP;
					FIFOCfg3Reg.bit.BLKNO_F15       = BLKNO - 1;
					FIFOCfg3Reg.bit.BLKSZ_F15       = BLKSZ;
					FIFOCfg3Reg.bit.EN_F15          = 0;
				}
			}
		}
		break;

	case USB_FIFO11: {
			FIFOCfg2Reg.bit.BLK_TYP_F11     = BLK_TYP;
			FIFOCfg2Reg.bit.BLKNO_F11       = BLKNO - 1;
			FIFOCfg2Reg.bit.BLKSZ_F11       = BLKSZ;
			FIFOCfg2Reg.bit.EN_F11          = 1;

			if ((BLKNO == BLKNUM_SINGLE) && (BLKSZ == TRUE)) {
				FIFOCfg3Reg.bit.BLK_TYP_F12     = BLK_TYP;
				FIFOCfg3Reg.bit.BLKNO_F12       = BLKNO - 1;
				FIFOCfg3Reg.bit.BLKSZ_F12       = BLKSZ;
				FIFOCfg3Reg.bit.EN_F12          = 0;
			} else if (BLKNO == BLKNUM_DOUBLE) {
				FIFOCfg3Reg.bit.BLK_TYP_F12     = BLK_TYP;
				FIFOCfg3Reg.bit.BLKNO_F12       = BLKNO - 1;
				FIFOCfg3Reg.bit.BLKSZ_F12       = BLKSZ;
				FIFOCfg3Reg.bit.EN_F12          = 0;

				if (BLKSZ == TRUE) {
					// Block size : 512~1024 bytes
					FIFOCfg3Reg.bit.BLK_TYP_F13     = BLK_TYP;
					FIFOCfg3Reg.bit.BLKNO_F13       = BLKNO - 1;
					FIFOCfg3Reg.bit.BLKSZ_F13       = BLKSZ;
					FIFOCfg3Reg.bit.EN_F13          = 0;

					FIFOCfg3Reg.bit.BLK_TYP_F14     = BLK_TYP;
					FIFOCfg3Reg.bit.BLKNO_F14       = BLKNO - 1;
					FIFOCfg3Reg.bit.BLKSZ_F14       = BLKSZ;
					FIFOCfg3Reg.bit.EN_F14          = 0;
				}
			} else if (BLKNO == BLKNUM_TRIPLE) {
				FIFOCfg3Reg.bit.BLK_TYP_F12     = BLK_TYP;
				FIFOCfg3Reg.bit.BLKNO_F12       = BLKNO - 1;
				FIFOCfg3Reg.bit.BLKSZ_F12       = BLKSZ;
				FIFOCfg3Reg.bit.EN_F12          = 0;

				FIFOCfg3Reg.bit.BLK_TYP_F13     = BLK_TYP;
				FIFOCfg3Reg.bit.BLKNO_F13       = BLKNO - 1;
				FIFOCfg3Reg.bit.BLKSZ_F13       = BLKSZ;
				FIFOCfg3Reg.bit.EN_F13          = 0;

				if (BLKSZ == TRUE) {
					DBG_ERR("BLKNO %d, BLKSZ %d exceed resource\r\n", BLKNO, BLKSZ);
				}
			}
		}
		break;



	case USB_FIFO12: {
			FIFOCfg3Reg.bit.BLK_TYP_F12           = BLK_TYP;
			FIFOCfg3Reg.bit.BLKNO_F12             = BLKNO - 1;
			FIFOCfg3Reg.bit.BLKSZ_F12             = BLKSZ;
			FIFOCfg3Reg.bit.EN_F12                = 1;

			if ((BLKNO == BLKNUM_SINGLE) && (BLKSZ == TRUE)) {
				FIFOCfg3Reg.bit.BLK_TYP_F13     = BLK_TYP;
				FIFOCfg3Reg.bit.BLKNO_F13       = BLKNO - 1;
				FIFOCfg3Reg.bit.BLKSZ_F13       = BLKSZ;
				FIFOCfg3Reg.bit.EN_F13          = 0;
			} else if (BLKNO == BLKNUM_DOUBLE) {
				FIFOCfg3Reg.bit.BLK_TYP_F13     = BLK_TYP;
				FIFOCfg3Reg.bit.BLKNO_F13       = BLKNO - 1;
				FIFOCfg3Reg.bit.BLKSZ_F13       = BLKSZ;
				FIFOCfg3Reg.bit.EN_F13          = 0;

				if (BLKSZ == TRUE) {
					// Block size : 512~1024 bytes
					FIFOCfg3Reg.bit.BLK_TYP_F14     = BLK_TYP;
					FIFOCfg3Reg.bit.BLKNO_F14       = BLKNO - 1;
					FIFOCfg3Reg.bit.BLKSZ_F14       = BLKSZ;
					FIFOCfg3Reg.bit.EN_F14          = 0;

					FIFOCfg3Reg.bit.BLK_TYP_F15     = BLK_TYP;
					FIFOCfg3Reg.bit.BLKNO_F15       = BLKNO - 1;
					FIFOCfg3Reg.bit.BLKSZ_F15       = BLKSZ;
					FIFOCfg3Reg.bit.EN_F15          = 0;
				}
			} else if (BLKNO == BLKNUM_TRIPLE) {
				FIFOCfg3Reg.bit.BLK_TYP_F13     = BLK_TYP;
				FIFOCfg3Reg.bit.BLKNO_F13       = BLKNO - 1;
				FIFOCfg3Reg.bit.BLKSZ_F13       = BLKSZ;
				FIFOCfg3Reg.bit.EN_F13          = 0;

				FIFOCfg3Reg.bit.BLK_TYP_F14     = BLK_TYP;
				FIFOCfg3Reg.bit.BLKNO_F14       = BLKNO - 1;
				FIFOCfg3Reg.bit.BLKSZ_F14       = BLKSZ;
				FIFOCfg3Reg.bit.EN_F14          = 0;

				// Block size : 512~1024 bytes. Exceed boundary.
				if (BLKSZ == TRUE) {
					DBG_ERR("BLKNO %d, BLKSZ %d exceed resource\r\n", BLKNO, BLKSZ);
				}
			}
		}
		break;

	case USB_FIFO13: {
			FIFOCfg3Reg.bit.BLK_TYP_F13     = BLK_TYP;
			FIFOCfg3Reg.bit.BLKNO_F13       = BLKNO - 1;
			FIFOCfg3Reg.bit.BLKSZ_F13       = BLKSZ;
			FIFOCfg3Reg.bit.EN_F13          = 1;

			if ((BLKNO == BLKNUM_SINGLE) && (BLKSZ == TRUE)) {
				FIFOCfg3Reg.bit.BLK_TYP_F14     = BLK_TYP;
				FIFOCfg3Reg.bit.BLKNO_F14       = BLKNO - 1;
				FIFOCfg3Reg.bit.BLKSZ_F14       = BLKSZ;
				FIFOCfg3Reg.bit.EN_F14          = 0;
			} else if (BLKNO == BLKNUM_DOUBLE) {
				FIFOCfg3Reg.bit.BLK_TYP_F14     = BLK_TYP;
				FIFOCfg3Reg.bit.BLKNO_F14       = BLKNO - 1;
				FIFOCfg3Reg.bit.BLKSZ_F14       = BLKSZ;
				FIFOCfg3Reg.bit.EN_F14          = 0;

				// Block size : 512~1024 bytes. Exceed boundary.
				if (BLKSZ == TRUE) {
					DBG_ERR("FIFOn %d, BLKNO %d, BLKSZ %d exceed resource\r\n", FIFOn, BLKNO, BLKSZ);
				}
			} else if (BLKNO == BLKNUM_TRIPLE) {
				FIFOCfg3Reg.bit.BLK_TYP_F14     = BLK_TYP;
				FIFOCfg3Reg.bit.BLKNO_F14       = BLKNO - 1;
				FIFOCfg3Reg.bit.BLKSZ_F14       = BLKSZ;
				FIFOCfg3Reg.bit.EN_F14          = 0;

				FIFOCfg3Reg.bit.BLK_TYP_F15     = BLK_TYP;
				FIFOCfg3Reg.bit.BLKNO_F15       = BLKNO - 1;
				FIFOCfg3Reg.bit.BLKSZ_F15       = BLKSZ;
				FIFOCfg3Reg.bit.EN_F15          = 0;

				// Block size : 512~1024 bytes. Exceed boundary.
				if (BLKSZ == TRUE) {
					DBG_ERR("FIFOn %d, BLKNO %d, BLKSZ %d exceed resource\r\n", FIFOn, BLKNO, BLKSZ);
				}
			}
		}
		break;

	case USB_FIFO14: {
			FIFOCfg3Reg.bit.BLK_TYP_F14     = BLK_TYP;
			FIFOCfg3Reg.bit.BLKNO_F14       = BLKNO - 1;
			FIFOCfg3Reg.bit.BLKSZ_F14       = BLKSZ;
			FIFOCfg3Reg.bit.EN_F14          = 1;

			if ((BLKNO == BLKNUM_SINGLE) && (BLKSZ == TRUE)) {
				FIFOCfg3Reg.bit.BLK_TYP_F15     = BLK_TYP;
				FIFOCfg3Reg.bit.BLKNO_F15       = BLKNO - 1;
				FIFOCfg3Reg.bit.BLKSZ_F15       = BLKSZ;
				FIFOCfg3Reg.bit.EN_F15          = 0;
			} else if (BLKNO == BLKNUM_DOUBLE) {
				FIFOCfg3Reg.bit.BLK_TYP_F15     = BLK_TYP;
				FIFOCfg3Reg.bit.BLKNO_F15       = BLKNO - 1;
				FIFOCfg3Reg.bit.BLKSZ_F15       = BLKSZ;
				FIFOCfg3Reg.bit.EN_F15          = 0;

				// Block size : 512~1024 bytes
				if (BLKSZ == TRUE) {
					DBG_ERR("FIFOn %d, BLKNO %d, BLKSZ %d exceed resource\r\n", FIFOn, BLKNO, BLKSZ);
				}
			} else if (BLKNO == BLKNUM_TRIPLE) {
				DBG_ERR("FIFOn %d, BLKNO %d, BLKSZ %d exceed resource\r\n", FIFOn, BLKNO, BLKSZ);
			}
		}
		break;

	case USB_FIFO15: {
			FIFOCfg3Reg.bit.BLK_TYP_F15     = BLK_TYP;
			FIFOCfg3Reg.bit.BLKNO_F15       = BLKNO - 1;
			FIFOCfg3Reg.bit.BLKSZ_F15       = BLKSZ;
			FIFOCfg3Reg.bit.EN_F15          = 1;

			if ((BLKNO > BLKNUM_SINGLE) || (BLKSZ == TRUE)) {
				DBG_ERR("FIFOn %d, BLKNO %d, BLKSZ %d exceed resource\r\n", FIFOn, BLKNO, BLKSZ);
			}
		}
		break;




	default:
		break;
	}

	USB_SETREG(USB_DEVFIFOCFG_REG_OFS, FIFOCfgReg.reg);
	USB_SETREG(USB_DEVFIFOCFG1_REG_OFS, FIFOCfg1Reg.reg);
	USB_SETREG(USB_DEVFIFOCFG2_REG_OFS, FIFOCfg2Reg.reg);
	USB_SETREG(USB_DEVFIFOCFG3_REG_OFS, FIFOCfg3Reg.reg);
}


/*
    Set endpoint max packet size

    This function is internal FIFO setting functions

    @param[in] EPn      endpoint number (1~8)
    @param[in] Dir      endpoint direction
     - @b EP_DIR_IN: endpoint direction IN
     - @b EP_DIR_OUT: endpoint direction OUT
    @param[in] MaxPksz  max packet size (0~0x1ffff)

    @return void
*/
static void usb_setEPMaxpktsize(USB_EP EPn, USB_EP_DIR Dir, UINT32 MaxPksz)
{
	T_USB_DEVMAXPS_INEP_REG  DevINEPMaxPS;
	T_USB_DEVMAXPS_OUTEP_REG  DevOUTEPMaxPS;

	DBG_DUMP("EP%d Dir=%d MaxPkt=%d\r\n",(int)EPn,(int)Dir,(int)MaxPksz);

	if (EPn <= USB_EP8) {
		if (Dir == EP_DIR_IN) {
			DevINEPMaxPS.reg = USB_GETREG(USB_DEVMAXPS_INEP1_REG_OFS + ((EPn - USB_EP1) << 2));
			DevINEPMaxPS.bit.MAXPS_IEP = MaxPksz;
			USB_SETREG((USB_DEVMAXPS_INEP1_REG_OFS + ((EPn - 1) << 2)), DevINEPMaxPS.reg);
		} else {
			DevOUTEPMaxPS.reg = USB_GETREG(USB_DEVMAXPS_OUTEP1_REG_OFS + ((EPn - 1) << 2));
			DevOUTEPMaxPS.bit.MAXPS_OEP = MaxPksz;
			USB_SETREG(USB_DEVMAXPS_OUTEP1_REG_OFS + ((EPn - 1) << 2), DevOUTEPMaxPS.reg);
		}
	} else if (EPn <= USB_EP15) {
		if (Dir == EP_DIR_IN) {
			DevINEPMaxPS.reg = USB_GETREG(USB_DEVMAXPS_INEP9_REG_OFS + ((EPn - USB_EP9) << 2));
			DevINEPMaxPS.bit.MAXPS_IEP = MaxPksz;
			USB_SETREG((USB_DEVMAXPS_INEP9_REG_OFS + ((EPn - USB_EP9) << 2)), DevINEPMaxPS.reg);
		} else {
			DevOUTEPMaxPS.reg = USB_GETREG(USB_DEVMAXPS_OUTEP9_REG_OFS + ((EPn - USB_EP9) << 2));
			DevOUTEPMaxPS.bit.MAXPS_OEP = MaxPksz;
			USB_SETREG(USB_DEVMAXPS_OUTEP9_REG_OFS + ((EPn - USB_EP9) << 2), DevOUTEPMaxPS.reg);
		}

	}
}

/*
    Set transaction number for high band width endpoint

    This function is internal FIFO setting functions

    @param[in] EPn      endpoint number (1~8)
    @param[in] HBW      high band width setting
                        - @b HBW_NOT: not high bandwidth
                        - @b HBW_1: one transaction
                        - @b HBW_2: two transaction
                        - @b HBW_3: three transaction

    @return void
*/
static void usb_setEPHighBandWidth(USB_EP EPn, USB_EP_HBW HBW)
{
	T_USB_DEVMAXPS_INEP_REG  DevEPMaxPS;

	if (EPn <= USB_EP8) {
		DevEPMaxPS.reg = USB_GETREG(USB_DEVMAXPS_INEP1_REG_OFS + ((EPn - USB_EP1) << 2));
		DevEPMaxPS.bit.TX_NUM_HBW_IEP = HBW;
		USB_SETREG((USB_DEVMAXPS_INEP1_REG_OFS + ((EPn - USB_EP1) << 2)), DevEPMaxPS.reg);
	} else {
		DevEPMaxPS.reg = USB_GETREG(USB_DEVMAXPS_INEP9_REG_OFS + ((EPn - USB_EP9) << 2));
		DevEPMaxPS.bit.TX_NUM_HBW_IEP = HBW;
		USB_SETREG((USB_DEVMAXPS_INEP9_REG_OFS + ((EPn - USB_EP9) << 2)), DevEPMaxPS.reg);
	}
}

/*
    Set the default FIFO and endpoint configuration

    This function is internal FIFO setting functions.

    @return void
*/
void usb_FIFO_EPxCfg(void)
{
	UINT32  k, EPi, FIFOn = 0, FIFO_Dir = 0;
	BOOL    bUseBiDir = FALSE;
	BOOL    bHS_En;

	// clear all EP & FIFO setting. -1 because excluding the EP0.
	for (EPi = 0 ; EPi < (USB_EP_MAX - 1) ; EPi++) {
		gEPMap[EPi] = USB_FIFO_NOT_USE;
	}

	for (FIFOn = 0 ; FIFOn < USB_FIFO_MAX ; FIFOn++) {
		gFIFOInMap[FIFOn]  = USB_EP_NOT_USE;
		gFIFOOutMap[FIFOn] = USB_EP_NOT_USE;
	}

	#if _EMULATION_
	FIFOn   = gTestFifoOffset;
	#else
	FIFOn   = USB_FIFO0;
	#endif

	bHS_En  = usb2dev_is_highspeed_enabled();

	// Clear FIFO Cfg
	USB_SETREG(USB_DEVEPMAP1_4_REG_OFS,	0xFFFFFFFF);
	USB_SETREG(USB_DEVEPMAP5_8_REG_OFS,	0xFFFFFFFF);
	USB_SETREG(USB_DEVEPMAP9_12_REG_OFS,0xFFFFFFFF);
	USB_SETREG(USB_DEVEPMAP13_15_REG_OFS,0xFFFFFFFF);

	USB_SETREG(USB_DEVFIFOMAP_REG_OFS,  0x0F0F0F0F);
	USB_SETREG(USB_DEVFIFOCFG_REG_OFS,	0x00000000);
	USB_SETREG(USB_DEVFIFOMAP1_REG_OFS, 0x0F0F0F0F);
	USB_SETREG(USB_DEVFIFOCFG1_REG_OFS,	0x00000000);
	USB_SETREG(USB_DEVFIFOMAP2_REG_OFS, 0x0F0F0F0F);
	USB_SETREG(USB_DEVFIFOCFG2_REG_OFS,	0x00000000);
	USB_SETREG(USB_DEVFIFOMAP3_REG_OFS, 0x0F0F0F0F);
	USB_SETREG(USB_DEVFIFOCFG3_REG_OFS,	0x00000000);

	// check for different g_Alternating & g_Interface if needed...,
	// set FIFO for EP1 ~ EP8 (ep_config_hs[0] ~ ep_config_hs[7])
	for (EPi = 0 ; EPi < (USB_EP_MAX - 1) ; EPi++) {
		USB_ENP_CONFIG *pEnpCfg, *pNextEnpCfg;

		pNextEnpCfg = NULL;

		if (bHS_En) {
			pEnpCfg     = &(gUSBManage.ep_config_hs[EPi]);

			for (k = EPi + 1; k < (USB_EP_MAX - 1); k++) {
				if (gUSBManage.ep_config_hs[k].enable) {
					pNextEnpCfg = &(gUSBManage.ep_config_hs[k]);
					break;
				}
			}
		} else {
			pEnpCfg     = &(gUSBManage.ep_config_fs[EPi]);

			for (k = EPi + 1; k < (USB_EP_MAX - 1); k++) {
				if (gUSBManage.ep_config_fs[k].enable) {
					pNextEnpCfg = &(gUSBManage.ep_config_fs[k]);
					break;
				}
			}
		}

		if (pEnpCfg->enable) {
			if (FIFOn >= USB_FIFO_MAX) {
				DBG_ERR("FIFO count %d exceed resource\r\n", FIFOn);
				return;
			}

			// set EP mapping
			usb_setEPMap((EPi + 1), pEnpCfg->direction, FIFOn);
			gEPMap[EPi] = FIFOn;
			//DBG_DUMP("EP%d map to FIFO-%d\r\n", EPi, FIFOn);

			// set FIFO mapping
			if (pEnpCfg->direction == EP_DIR_IN) {
				FIFO_Dir = USB_FIFO_IN;
				gFIFOInMap[FIFOn] = EPi + 1;
			} else {
				FIFO_Dir = USB_FIFO_OUT;
				gFIFOOutMap[FIFOn] = EPi + 1;
			}

			if (bUseBiDir) {
				FIFO_Dir = USB_FIFO_BI;
				USB_setFIFOMap(FIFOn, FIFO_Dir, EPi);
			} else {
				USB_setFIFOMap(FIFOn, FIFO_Dir, EPi + 1);
				USB_setFIFOCfg(FIFOn, pEnpCfg->trnsfer_type, pEnpCfg->blk_num, (pEnpCfg->blk_size > 512));
			}

			// set EP max packet size
			usb_setEPMaxpktsize((EPi + 1), pEnpCfg->direction, pEnpCfg->max_pkt_size);

			// set high bandwidth
			if (pEnpCfg->direction == EP_DIR_IN) {
				usb_setEPHighBandWidth((EPi + 1), pEnpCfg->high_bandwidth);
			}

			// check for sharing FIFO
			// may not enough for complicated config...
			if ((gUsbUseShareFIFO)                                          &&
				(pNextEnpCfg != NULL)                                       &&
				((bUseBiDir == FALSE)                                &&
				 (pNextEnpCfg->blk_num        == pEnpCfg->blk_num)     &&
				 (pNextEnpCfg->blk_size       == pEnpCfg->blk_size)    &&
				 (pNextEnpCfg->direction     != pEnpCfg->direction)  &&
				 (pNextEnpCfg->trnsfer_type   == pEnpCfg->trnsfer_type)&&
				 (pNextEnpCfg->trnsfer_type   != EP_TYPE_ISOCHRONOUS)))

			{
				bUseBiDir = TRUE;
			} else {
				UINT32  vFIFOn;

				bUseBiDir = FALSE;

				//////////////////
				if ((FIFOn == 0) && (pEnpCfg->blk_num == BLKNUM_TRIPLE) && (pEnpCfg->blk_size > 512) && (pEnpCfg->trnsfer_type == EP_TYPE_ISOCHRONOUS)) {
					FIFOn += 6;
				}
				/////////////////

				if ((pEnpCfg->blk_num) <= BLKNUM_TRIPLE) {
					FIFOn += pEnpCfg->blk_num;

					// If max_pkt_size larger than 512. FIFO number is doubled.
					if (pEnpCfg->blk_size > 512) {
						FIFOn += pEnpCfg->blk_num;
					}
				}

				#if 0
				if(pEnpCfg->trnsfer_type == EP_TYPE_BULK) {
					FIFOn += 2;
				}
				#endif

				vFIFOn = FIFOn;

				if (pNextEnpCfg != NULL) {
					if ((pNextEnpCfg->blk_num) <= BLKNUM_TRIPLE) {
						vFIFOn += pNextEnpCfg->blk_num;

						// If max_pkt_size larger than 512. FIFO number is doubled.
						if (pNextEnpCfg->blk_size > 512) {
							vFIFOn += pNextEnpCfg->blk_num;
						}
					}
				}


				//if ((vFIFOn > USB_FIFO4) && (FIFOn <= USB_FIFO3)) {
				//	FIFOn = USB_FIFO4;
				//} else if ((vFIFOn > USB_FIFO8) && (FIFOn <= USB_FIFO7)) {
				//	FIFOn = USB_FIFO8;
				//}

				#if defined(_NVT_EMULATION_)
				if (FIFOn >= USB_FIFO_MAX) {
					FIFOn = 0;
				}
				#else
				if (FIFOn >= USB_FIFO_MAX) {
					break;
				}
				#endif
			}
		}
	} // end of for loop

}

/*
    Get EP is IN or OUT
*/
USB_EP_DIR usb_getEpDirection(USB_EP EPn)
{
	USB_FIFO_NUM fifono;
	T_USB_DEVEPMAP1_4_REG devmap1;
	T_USB_DEVEPMAP5_8_REG devmap2;
	T_USB_DEVEPMAP9_12_REG devmap3;
	T_USB_DEVEPMAP13_15_REG devmap4;
	UINT32 temp;

	fifono = usb_getFifoFromEp(EPn);

	devmap1.reg = USB_GETREG(USB_DEVEPMAP1_4_REG_OFS);
	devmap2.reg = USB_GETREG(USB_DEVEPMAP5_8_REG_OFS);
	devmap3.reg = USB_GETREG(USB_DEVEPMAP9_12_REG_OFS);
	devmap4.reg = USB_GETREG(USB_DEVEPMAP13_15_REG_OFS);

	if (EPn < USB_EP5) {
		temp = (devmap1.reg >> ((EPn-1)*8))&0xFF;
	} else if (EPn < USB_EP9) {
		temp = (devmap2.reg >> ((EPn-5)*8))&0xFF;
	} else if (EPn < USB_EP13){
		temp = (devmap3.reg >> ((EPn-9)*8))&0xFF;
	} else {
		temp = (devmap4.reg >> ((EPn-13)*8))&0xFF;
	}

	if((temp & 0xF) == fifono) {
		return EP_DIR_IN;
	} else if (((temp>>4) & 0xF) == fifono) {
		return EP_DIR_OUT;
	} else {
		return EP_DIR_NOT_USE;
	}
}

/*
    Get IN EP from FIFO

    This function is used to get EP mapped by input FIFO

    @param[in] FIFOn

    @return EP number
*/
USB_EP usb_getInEpFromFifo(USB_FIFO_NUM FIFOn)
{
	return gFIFOInMap[FIFOn];
}

/*
    Get OUT EP from FIFO

    This function is used to get EP mapped by output FIFO

    @param[in] FIFOn

    @return EP number
*/
USB_EP usb_getOutEpFromFifo(USB_FIFO_NUM FIFOn)
{
	return gFIFOOutMap[FIFOn];
}

/*
    Get FIFO from EP

    This function is used to get FIFO mapped by input EP

    @param[in] EPn

    @return FIFO number
*/
USB_FIFO_NUM usb_getFifoFromEp(USB_EP EPn)
{
	return gEPMap[EPn - 1];
}

BOOL usb_get_fifo_in_int_en(USB_FIFO_NUM FIFOn)
{
	T_USB_DEVINTMASKGROUP1_REG intGroup1Mask;

	if (FIFOn >= USB_FIFO_MAX) {
		return 0;
	}

	intGroup1Mask.reg  = USB_GETREG(USB_DEVINTMASKGROUP1_REG_OFS);
	intGroup1Mask.reg &= (0x1 << (16 + FIFOn));

	return !(intGroup1Mask.reg > 0);
}


BOOL usb_get_fifo_out_int_en(USB_FIFO_NUM FIFOn)
{
	T_USB_DEVINTMASKGROUP1_REG intGroup1Mask;
	T_USB_DEVINTMASKGROUP2_REG intGroup2Mask;

	if (FIFOn >= USB_FIFO_MAX) {
		return 0;
	}

	if (FIFOn < USB_FIFO8) {
		intGroup1Mask.reg  = USB_GETREG(USB_DEVINTMASKGROUP1_REG_OFS);
		intGroup1Mask.reg &= (0x3 << (FIFOn << 1));
		return !(intGroup1Mask.reg > 0);

	} else {
		intGroup2Mask.reg  = USB_GETREG(USB_DEVINTMASKGROUP2_REG_OFS);
		intGroup2Mask.reg &= (0x3 << (((FIFOn-USB_FIFO8) << 1)+16));
		return !(intGroup2Mask.reg > 0);
	}

}


/*
    Mask(Disable) FIFO IN interrupt

    @param[in] FIFOn    FIFO number
    @return void
*/
static void usb_maskFIFOININT(USB_FIFO_NUM FIFOn)
{
	T_USB_DEVINTMASKGROUP1_REG intGroup1Mask;
	unsigned long      flags;

	if (FIFOn >= USB_FIFO_MAX) {
		return;
	}

	loc_cpu(flags);
	intGroup1Mask.reg  = USB_GETREG(USB_DEVINTMASKGROUP1_REG_OFS);
	intGroup1Mask.reg |= (0x1 << (16 + FIFOn));
	USB_SETREG(USB_DEVINTMASKGROUP1_REG_OFS, intGroup1Mask.reg);
	unl_cpu(flags);
}

/*
    Unmask(Enable) FIFO IN interrupt

    @param[in] FIFOn    FIFO number
    @return void
*/
static void usb_unmaskFIFOININT(USB_FIFO_NUM FIFOn)
{
	T_USB_DEVINTMASKGROUP1_REG intGroup1Mask;
	unsigned long      flags;

	if (FIFOn >= USB_FIFO_MAX) {
		return;
	}

	loc_cpu(flags);
	intGroup1Mask.reg  = USB_GETREG(USB_DEVINTMASKGROUP1_REG_OFS);
	intGroup1Mask.reg &= ~(0x1 << (16 + FIFOn));
	USB_SETREG(USB_DEVINTMASKGROUP1_REG_OFS, intGroup1Mask.reg);
	unl_cpu(flags);
}

/*
    Mask(Disable) FIFO OUT interrupt

    @param[in] FIFOn    FIFO number
    @return void
*/
static void usb_maskFIFOOUTINT(USB_FIFO_NUM FIFOn)
{
	T_USB_DEVINTMASKGROUP1_REG intGroup1Mask;
	T_USB_DEVINTMASKGROUP2_REG intGroup2Mask;
	unsigned long      flags;

	if (FIFOn >= USB_FIFO_MAX) {
		return;
	}

	loc_cpu(flags);
	if (FIFOn < USB_FIFO8) {
		intGroup1Mask.reg  = USB_GETREG(USB_DEVINTMASKGROUP1_REG_OFS);
		intGroup1Mask.reg |= (0x3 << (FIFOn << 1));
		USB_SETREG(USB_DEVINTMASKGROUP1_REG_OFS, intGroup1Mask.reg);
	} else {
		intGroup2Mask.reg  = USB_GETREG(USB_DEVINTMASKGROUP2_REG_OFS);
		intGroup2Mask.reg |= (0x3 << (((FIFOn-USB_FIFO8) << 1)+16));
		USB_SETREG(USB_DEVINTMASKGROUP2_REG_OFS, intGroup2Mask.reg);
	}
	unl_cpu(flags);
}

/*
    Unmask(Enable) FIFO OUT interrupt

    @param[in] FIFOn    FIFO number
    @return void
*/
static void usb_unmaskFIFOOUTINT(USB_FIFO_NUM FIFOn)
{
	T_USB_DEVINTMASKGROUP1_REG intGroup1Mask;
	T_USB_DEVINTMASKGROUP2_REG intGroup2Mask;
	unsigned long      flags;

	if (FIFOn >= USB_FIFO_MAX) {
		return;
	}

	loc_cpu(flags);
	if (FIFOn < USB_FIFO8) {
		intGroup1Mask.reg  = USB_GETREG(USB_DEVINTMASKGROUP1_REG_OFS);
		intGroup1Mask.reg &= ~(0x3 << (FIFOn << 1));
		USB_SETREG(USB_DEVINTMASKGROUP1_REG_OFS, intGroup1Mask.reg);
	} else {
		intGroup2Mask.reg  = USB_GETREG(USB_DEVINTMASKGROUP2_REG_OFS);
		intGroup2Mask.reg &= ~(0x3 << (((FIFOn-USB_FIFO8) << 1)+16));
		USB_SETREG(USB_DEVINTMASKGROUP2_REG_OFS, intGroup2Mask.reg);
	}
	unl_cpu(flags);
}

/*
    Set DMA target FIFO

    This function is internal DMA target FIFO setting functions

    @param[in] EPn      endpoint number

    @return
        - @b E_OK: success
        - @b Else: fail
*/
#if (_USE_VDMA_FIFO_ == 0)
static ER usb_setDmaTargetFifo(USB_EP EPn)
{
	USB_FIFO_NUM            fifoNum;
	T_USB_DEVACCFIFO_REG    devAccFifo = {0};

	if ((EPn > USB_EP0) && (EPn < USB_EP_MAX)) {
		fifoNum = usb_getFifoFromEp(EPn);

		devAccFifo.bit.ACC_F0_3 = (0x1 << fifoNum);
		USB_SETREG(USB_DEVACCFIFO_REG_OFS, devAccFifo.reg);
		//DBG_DUMP("ACC=0x%02X 0x%02X\r\n",(int)devAccFifo.reg,(int)USB_GETREG(USB_DEVACCFIFO_REG_OFS));
	} else if (EPn == USB_EP0) {
		devAccFifo.bit.ACC_F0_3 = (0x1 << 16);
		USB_SETREG(USB_DEVACCFIFO_REG_OFS, devAccFifo.reg);
	}
	return E_OK;
}
#endif
/*
    If need not clear fifo. EP number cam set to USB_EP_NOT_USE
*/
BOOL usb_abortDMA(USB_EP EPn, USB_DMACH DmaCH)
{
	T_USB_DEVVIRTUAL_DMACXF1_REG  devDMACtrl;
	UINT32                        uiRegOfs, i, loop_cnt = 0;
	FLGPTN                        uiFlag = 0;

	#if 1
	// CONTROL
	{
		devDMACtrl.reg = USB_GETREG(USB_DEVVIRTUAL_DMACXF1_REG_OFS);
		if ((DmaCH == USB_DMACH_ALL) || ((DmaCH & USB_DMACH_CONTROL) &&devDMACtrl.bit.VDMA_START_CXF)) {
		//if (1) {

			// set DMA_ABORT & CLRFIFO
			devDMACtrl.bit.VDMA_ABORT_FIFO    = 1;
			devDMACtrl.bit.VDMA_ABORT_CLRFIFO = 0;
			USB_SETREG(USB_DEVVIRTUAL_DMACXF1_REG_OFS, devDMACtrl.reg);

			#ifdef _NVT_EMULATION_
			DBG_WRN("ABORT CTL   0x%08X\r\n",  devDMACtrl.reg);
			#endif

			/*
			loop_cnt = 0;
			do {
				devDMACtrl.reg = USB_GETREG(USB_DEVVIRTUAL_DMACXF1_REG_OFS);

				loop_cnt++;
				if (loop_cnt > 0x1800000) {
					devDMACtrl.reg = USB_GETREG(USB_DEVVIRTUAL_DMACXF1_REG_OFS);
					DBG_ERR("DMA-ctl Abort Fail!! 0x%08X\r\n", devDMACtrl.reg);
					break;
				}
				#ifdef _NVT_EMULATION_
				DBG_DUMP("u\r\n");
				#endif
			} while (devDMACtrl.bit.VDMA_START_CXF);
			*/

			// disable CLRFIFO
			//devDMACtrl.bit.VDMA_ABORT_FIFO    = 0;
			//devDMACtrl.bit.VDMA_ABORT_CLRFIFO = 0;
			//USB_SETREG(USB_DEVVIRTUAL_DMACXF1_REG_OFS, devDMACtrl.reg);

			loop_cnt = 0;

			do {
				devDMACtrl.reg = USB_GETREG(USB_DEVVIRTUAL_DMACXF1_REG_OFS);
				loop_cnt++;

				if(loop_cnt > 1000) {
					DBG_ERR("ABORT ctl FAILED 0x%08X\r\n", devDMACtrl.reg);
					break;
				}

			} while(devDMACtrl.bit.VDMA_ABORT_FIFO || devDMACtrl.bit.VDMA_ABORT_CLRFIFO);


		}

	}
	#endif

	for(i = 0; i < 4; i++) {
		uiRegOfs = i << 3;

		devDMACtrl.reg = USB_GETREG(USB_DEVVIRTUAL_DMAFIFO0_PAR1_REG_OFS+uiRegOfs);
		if ((DmaCH & (0x1 << i)) && devDMACtrl.bit.VDMA_START_CXF) {

			// set DMA_ABORT & CLRFIFO
			devDMACtrl.bit.VDMA_ABORT_FIFO    = 1;
			devDMACtrl.bit.VDMA_ABORT_CLRFIFO = 0;
			USB_SETREG(USB_DEVVIRTUAL_DMAFIFO0_PAR1_REG_OFS+uiRegOfs, devDMACtrl.reg);

			/*
			loop_cnt = 0;
			do {
				devDMACtrl.reg = USB_GETREG(USB_DEVVIRTUAL_DMAFIFO0_PAR1_REG_OFS+uiRegOfs);

				loop_cnt++;
				if (loop_cnt > 0x1800000) {
					devDMACtrl.reg = USB_GETREG(USB_DEVVIRTUAL_DMAFIFO0_PAR1_REG_OFS+uiRegOfs);
					DBG_ERR("DMA-%d Abort Fail!! 0x%08X\r\n", (UINT)i, devDMACtrl.reg);
					return 1;
				}
				#ifdef _NVT_EMULATION_
				DBG_DUMP("x\r\n");
				#endif
			} while (devDMACtrl.bit.VDMA_START_CXF);
			*/

			#ifdef _NVT_EMULATION_
			DBG_WRN("ABORT DMA-%d   0x%08X\r\n", i,  devDMACtrl.reg);
			#endif

			// disable CLRFIFO
			//devDMACtrl.bit.VDMA_ABORT_FIFO    = 0;
			//devDMACtrl.bit.VDMA_ABORT_CLRFIFO = 0;
			//devDMACtrl.bit.VDMA_START_CXF     = 0;
			//USB_SETREG(USB_DEVVIRTUAL_DMAFIFO0_PAR1_REG_OFS + uiRegOfs, devDMACtrl.reg);

			loop_cnt = 0;

			do {
				devDMACtrl.reg = USB_GETREG(USB_DEVVIRTUAL_DMAFIFO0_PAR1_REG_OFS+uiRegOfs);
				loop_cnt++;

				if(loop_cnt > 1000) {
					DBG_ERR("ABORT FAILED 0x%08X\r\n", devDMACtrl.reg);
					return 1;
				}

			} while(devDMACtrl.bit.VDMA_ABORT_FIFO || devDMACtrl.bit.VDMA_ABORT_CLRFIFO);

			{
				T_USB_DEVFIFOBYTECNT_REG        devFIFOByteCnt;

				devFIFOByteCnt.reg  = USB_GETREG(USB_DEVFIFO0BYTECNT_REG_OFS + (i << 2));
				devFIFOByteCnt.bit.FFRST = 0x1;
				USB_SETREG(USB_DEVFIFO0BYTECNT_REG_OFS + (i << 2), devFIFOByteCnt.reg);

			}

			uiFlag |= (FLGUSB_DMA0DONE << i);
		}

	}


	for(i = 4; i < 16; i++) {
		uiRegOfs = (i-4) << 3;

		devDMACtrl.reg = USB_GETREG(USB_DEVVIRTUAL_DMAFIFO4_PAR1_REG_OFS+uiRegOfs);
		if ((DmaCH & (0x1 << i)) && devDMACtrl.bit.VDMA_START_CXF) {
			// set DMA_ABORT & CLRFIFO
			devDMACtrl.bit.VDMA_ABORT_FIFO    = 1;
			devDMACtrl.bit.VDMA_ABORT_CLRFIFO = 0;
			USB_SETREG(USB_DEVVIRTUAL_DMAFIFO4_PAR1_REG_OFS+uiRegOfs, devDMACtrl.reg);

			/*
			loop_cnt = 0;
			do {
				devDMACtrl.reg = USB_GETREG(USB_DEVVIRTUAL_DMAFIFO4_PAR1_REG_OFS+uiRegOfs);

				loop_cnt++;
				if (loop_cnt > 0x1800000) {
					devDMACtrl.reg = USB_GETREG(USB_DEVVIRTUAL_DMAFIFO4_PAR1_REG_OFS+uiRegOfs);
					DBG_ERR("DMA-%d Abort Fail!! 0x%08X\r\n", (UINT)i, devDMACtrl.reg);
					return 1;
				}
				DBG_DUMP("o\r\n");
			} while (devDMACtrl.bit.VDMA_START_CXF);
			*/

			#ifdef _NVT_EMULATION_
			DBG_WRN("ABORT DMA-%d   0x%08X\r\n", i,  devDMACtrl.reg);
			#endif

			// disable CLRFIFO
			//devDMACtrl.bit.VDMA_ABORT_FIFO    = 0;
			//devDMACtrl.bit.VDMA_ABORT_CLRFIFO = 0;
			//devDMACtrl.bit.VDMA_START_CXF     = 0;
			//USB_SETREG(USB_DEVVIRTUAL_DMAFIFO4_PAR1_REG_OFS + uiRegOfs, devDMACtrl.reg);

			do {
				devDMACtrl.reg = USB_GETREG(USB_DEVVIRTUAL_DMAFIFO4_PAR1_REG_OFS+uiRegOfs);
				loop_cnt++;

				if(loop_cnt > 1000) {
					DBG_ERR("ABORT FAILED 0x%08X\r\n", devDMACtrl.reg);
					return 1;
				}

			} while(devDMACtrl.bit.VDMA_ABORT_FIFO || devDMACtrl.bit.VDMA_ABORT_CLRFIFO);

			{
				T_USB_DEVFIFOBYTECNT_REG        devFIFOByteCnt;

				devFIFOByteCnt.reg  = USB_GETREG(USB_DEVFIFO4BYTECNT_REG_OFS + ((i - USB_FIFO4) << 2));
				devFIFOByteCnt.bit.FFRST = 0x1;
				USB_SETREG(USB_DEVFIFO4BYTECNT_REG_OFS + ((i - USB_FIFO4) << 2), devFIFOByteCnt.reg);
			}

			uiFlag |= (FLGUSB_DMA0DONE << i);

		}

	}

	//if (DmaCH == USB_DMACH_ALL) {
	//	// set DMA_ABORT & CLRFIFO
	//	devDMACtrl.reg = USB_GETREG(USB_DEVVIRTUAL_DMACXF1_REG_OFS);
	//	devDMACtrl.bit.VDMA_ABORT_FIFO    = 1;
	//	devDMACtrl.bit.VDMA_ABORT_CLRFIFO = 0;
	//	USB_SETREG(USB_DEVVIRTUAL_DMACXF1_REG_OFS, devDMACtrl.reg);
	//}

	if (uiFlag) {
		bAbortDMA = TRUE;
		set_flg(FLG_ID_USBTSK, uiFlag);
	}

	return 0;
}

#if 1
/**
    @name USB read /write functions
*/
//@{


/**
    Return setup data to host.

    Class library can use this API to return CX IN data after receive a setup packet.

    @note Before invoke this API, usb2dev_control_data.p_data should point to return data\n
            and usb2dev_control_data.w_length should be filled with length of return data.

    @return void
*/
void usb2dev_reture_setup_data(void)
{
	UINT32  RetBytes;

	//if (usb2dev_control_data.w_length < EP0_PACKET_SIZE) {
		RetBytes = usb2dev_control_data.w_length;  // this is the last packet to return host....
		usb2dev_control_data.w_length = 0;
	//} else {
	//	T_USB_DEVINTMASKGROUP0_REG    devIntMskGrp0;

	//	RetBytes = EP0_PACKET_SIZE;
	//	usb2dev_control_data.w_length -= EP0_PACKET_SIZE;

		// CX_IN interrupt is default DISABLED
		// Only enable if still has more data to be transfered to host
	//	devIntMskGrp0.reg = USB_GETREG(USB_DEVINTMASKGROUP0_REG_OFS);
	//	devIntMskGrp0.bit.MCX_IN_INT = 0;
	//	USB_SETREG(USB_DEVINTMASKGROUP0_REG_OFS, devIntMskGrp0.reg);
	//}

	usb2dev_write_endpoint(USB_EP0, usb2dev_control_data.p_data, &RetBytes);

	usb2dev_control_data.p_data += RetBytes;

	// only set CX_DONE at the last cx transfer
	if (usb2dev_control_data.w_length == 0) {
		gUSBCxFinishAction = USB_ACT_DONE;
	} else {
		gUSBCxFinishAction = USB_ACT_IDLE;
	}

}


/**
    Read the buffer content from FIFO

    This function is blocking mode. It will return until DMA done or
    some error occured. Check the return ER and pDMALen for actual
    transfered length.

    @param[in] EPn          endpoint number (USB_EP0~USB_EP15)
    @param[in] pBuffer      buffer pointer
    @param[in,out] pDMALen  input length to read, output actual transfered length

    @return
        - @b E_OACV: USB driver is not opened
        - @b E_PAR: input DMA length or EP number incorrect
        - @b E_OBJ: FIFO is busy
        - @b E_SYS: USB is SUSPENDED
        - @b E_OK: read done (maybe aborted, check return pDMALen)
*/
ER usb2dev_read_endpoint(USB_EP EPn, UINT8 *pBuffer, UINT32 *pDMALen)
{
	ER erReturn;

	//DBG_DUMP("usb2dev_read_endpoint EP%d Len=%d\r\n", EPn, *pDMALen);

	erReturn = usb2dev_set_ep_read(EPn, pBuffer, pDMALen);
	if (erReturn != E_OK) {
		return erReturn;
	}

	erReturn = usb2dev_wait_ep_done(EPn, pDMALen);

	dma_flushReadCache((UINT32)pBuffer, *pDMALen);

	return erReturn;
}

ER usb2dev_read_endpoint_timeout(USB_EP EPn, UINT8 *pBuffer, UINT32 *pDMALen, UINT32 timeout_ms)
{
	ER erReturn;

	erReturn = usb2dev_set_ep_read(EPn, pBuffer, pDMALen);
	if (erReturn != E_OK) {
		return erReturn;
	}

	erReturn = usb2dev_wait_ep_done_timeout(EPn, pDMALen, timeout_ms);

	dma_flushReadCache((UINT32)pBuffer, *pDMALen);

	return erReturn;
}

/**
    Write the buffer content to FIFO

    This function is blocking mode. It will return until DMA done or
    some error occured. Check the return ER and pDMALen for actual
    transfered length.

    @param[in] EPn          endpoint number (USB_EP0~USB_EP15)
    @param[in] pBuffer      buffer pointer
    @param[in,out] pDMALen  input length to write, output actual transfered length

    @return
        - @b E_OACV: USB driver is not opened
        - @b E_PAR: input DMA length or EP number incorrect
        - @b E_OBJ: FIFO is busy
        - @b E_SYS: USB is SUSPENDED
        - @b E_OK: read done (maybe aborted, check return pDMALen)
*/
ER usb2dev_write_endpoint(USB_EP EPn, UINT8 *pBuffer, UINT32 *pDMALen)
{
	ER erReturn;

	//DBG_DUMP("usb2dev_write_endpoint EP%d Len=%d\r\n", EPn, *pDMALen);

	erReturn = usb2dev_set_ep_write(EPn, pBuffer, pDMALen);
	if (erReturn != E_OK) {
		return erReturn;
	}

	return usb2dev_wait_ep_done(EPn, pDMALen);
}

ER usb2dev_write_endpoint_timeout(USB_EP EPn, UINT8 *pBuffer, UINT32 *pDMALen, UINT32 timeout_ms)
{
	ER erReturn;

	erReturn = usb2dev_set_ep_write(EPn, pBuffer, pDMALen);
	if (erReturn != E_OK) {
		return erReturn;
	}

	return usb2dev_wait_ep_done_timeout(EPn, pDMALen, timeout_ms);
}

/**
    Abort the endpoint transfer

    @param[in] EPn  endpoint number (1~15)

    @return
        - @b E_OK: abort success
        - @b Else: abort fail
*/
ER usb2dev_abort_endpoint(USB_EP EPn)
{
	USB_FIFO_NUM                fifonum;

	if (!gUSBOpen) {
		DBG_ERR("driver not opened\r\n");
		return E_OACV;
	}

	if (EPn == USB_EP0) {
		usb_abortDMA(EPn, USB_DMACH_CONTROL);
	} else {
		fifonum = usb_getFifoFromEp(EPn);
		usb_abortDMA(EPn, (USB_DMACH_CH1 << fifonum));
	}

	usb2dev_clear_ep_fifo(EPn);

	return E_OK;
}

/**
    Read the buffer content from FIFO

    This function is non-blocking mode and will return immediately.
    Used with usb2dev_wait_ep_done() to wait for DMA complete.

    @param[in] EPn      endpoint number (USB_EP0~USB_EP15)
    @param[in] pBuffer  buffer pointer
    @param[in] pDMALen  DMA length to be read.
                        Valid length 0x1 to 0x40 for EP0.
                        Valid length 0x1 to 0x7fffff for EP1~EP15.

    @return
        - @b E_OACV: USB driver is not opened
        - @b E_PAR: input DMA length or EP number incorrect
        - @b E_OBJ: FIFO is busy
        - @b E_SYS: USB is SUSPENDED
        - @b E_OK: start to transfer
*/
ER usb2dev_set_ep_read(USB_EP EPn, UINT8 *pBuffer, UINT32 *pDMALen)
{
#if _USE_VDMA_FIFO_
	T_USB_DEVVIRTUAL_DMACXF1_REG  devVDMACXF1;
	T_USB_DEVVIRTUAL_DMACXF2_REG  devVDMACXF2;
#else
	T_USB_DEVDMACTRL1_REG         devDMACtl1;
	T_USB_DEVDMACTRL2_REG         devDMACtl2;
#endif
	USB_FIFO_NUM                  fifonum;
#if _USE_CXF_PIO_
	T_USB_DEVCXCFGFIFOSTATUS_REG  devCXCFG;
	INT32                         i, Ep0DMASize, Ep0RxSize;
	UINT32                        *pBUF;
#else

#endif
	unsigned long                 flags;
	FLGPTN                        flags2;

	if (!gUSBOpen) {
		DBG_ERR("driver is not opened !\r\n");
		return E_OACV;
	}

	// We must avoid 0Byte DMAtrigger. This may cause mal-functioned in MSDC application.
	if ((*pDMALen == 0) || (*pDMALen > USB_MAX_DMA_LENGTH)) {
		return E_PAR;
	}

	// check if under SUSPEND state
	if (usb2dev_get_controller_state() == USB_CONTROLLER_STATE_SUSPEND) {
		// In SUSPEND state, UCLK is stopped and USB controller will not do jobs of DMA
		// Block DMA request when usb is SUSPENDED
		DBG_ERR("usb under SUSPENDED\r\n");
		return E_SYS;
	}

	if (EPn == USB_EP0) {
#if _USE_VDMA_FIFO_

		// check if DMA being acccessed
		devVDMACXF1.reg = USB_GETREG(USB_DEVVIRTUAL_DMACXF1_REG_OFS);
		if (devVDMACXF1.bit.VDMA_START_CXF) {
			DBG_ERR("EP%d: DMA not available\r\n", EPn);
			return E_OBJ;
		}

		//DBG_DUMP("r");
		clr_flg(FLG_ID_USBTSK, FLGUSB_EP0_DMADONE);

		// Enter critical section
		loc_cpu(flags);

		// Config DMA direction & length
		devVDMACXF1.reg = USB_GETREG(USB_DEVVIRTUAL_DMACXF1_REG_OFS);
		devVDMACXF1.bit.VDMA_TYPE_CXF = 0; // FIFO to memory
		devVDMACXF1.bit.VDMA_LEN_CXF  = *pDMALen;
		USB_SETREG(USB_DEVVIRTUAL_DMACXF1_REG_OFS, devVDMACXF1.reg);


		// set Dma Addr
		//if ((((UINT32)pBuffer)&0xFFF00000) == 0xF0700000)
		//	devVDMACXF2.reg = (UINT32)pBuffer;
		//else
			devVDMACXF2.reg = dma_getPhyAddr2((UINT32)pBuffer);

		USB_SETREG(USB_DEVVIRTUAL_DMACXF2_REG_OFS, devVDMACXF2.reg);

		dma_flushReadCache2((UINT32)pBuffer, *pDMALen);
		dma_flushWriteCache2((UINT32)pBuffer, *pDMALen);

		//bAbortDMA = FALSE;

		// start Dma
		devVDMACXF1.reg = USB_GETREG(USB_DEVVIRTUAL_DMACXF1_REG_OFS);
		devVDMACXF1.bit.VDMA_START_CXF = 1;
		USB_SETREG(USB_DEVVIRTUAL_DMACXF1_REG_OFS, devVDMACXF1.reg);

		// Exit critical section
		unl_cpu(flags);
#else
		wai_flg(&flags2, FLG_ID_USBTSK, FLGPTN_USB_DMA_RDY, TWF_CLR | TWF_ORW);

		// check if DMA being acccessed
		devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
		if (devDMACtl1.bit.DMA_START) {
			DBG_ERR("EP%d: DMA not available 0x%x\r\n", EPn, USB_GETREG(USB_DEVACCFIFO_REG_OFS));
			return E_OBJ;
		}

		clr_flg(FLG_ID_USBTSK, FLGUSB_DMADONE);

		// Enter critical section
		loc_cpu(flags);

		// Config DMA direction & length
		devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
		devDMACtl1.bit.DMA_TYPE = 0; // FIFO to memory
		devDMACtl1.bit.DMA_LEN  = *pDMALen;
		USB_SETREG(USB_DEVDMACTRL1_REG_OFS, devDMACtl1.reg);

		usb_setDmaTargetFifo(USB_EP0);

		// set Dma Addr
		//if ((((UINT32)pBuffer)&0xFFF00000) == 0xF0700000)
		//	devDMACtl2.reg = (UINT32)pBuffer;
		//else
			devDMACtl2.reg = dma_getPhyAddr((UINT32)pBuffer);
		USB_SETREG(USB_DEVDMACTRL2_REG_OFS, devDMACtl2.reg);

		//dma_flushWriteCache((UINT32)pBuffer, *pDMALen);
		dma_flushReadCache((UINT32)pBuffer, *pDMALen);

		bAbortDMA = FALSE;

		// start Dma
		devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
		devDMACtl1.bit.DMA_START = 1;
		USB_SETREG(USB_DEVDMACTRL1_REG_OFS, devDMACtl1.reg);

		// Exit critical section
		unl_cpu(flags);
#endif

		return E_OK;
	}

	fifonum = usb_getFifoFromEp(EPn);

	if(usb_get_fifo_out_int_en(fifonum)) {
		DBG_DUMP("FIFO%d INT Not DISABLE\r\n", fifonum);
	}

	if (fifonum < USB_FIFO_MAX) {

#if _USE_VDMA_FIFO_
		//wai_flg(&flags2, FLG_ID_USB, FLGPTN_USB_DMA_RDY, TWF_CLR | TWF_ORW);
		flags2 = flags2;

		if (fifonum < USB_FIFO4) {

			// check if DMA being acccessed
			devVDMACXF1.reg = USB_GETREG(USB_DEVVIRTUAL_DMAFIFO0_PAR1_REG_OFS + fifonum * 0x8);
			if (devVDMACXF1.bit.VDMA_START_CXF) {
				DBG_ERR("EP%d: DMA not available 0x%x\r\n", EPn, fifonum);
				return E_OBJ;
			}

			clr_flg(FLG_ID_USBTSK, (FLGUSB_DMA0DONE << fifonum));

			// Enter critical section
			loc_cpu(flags);

			// Config DMA direction & length
			devVDMACXF1.reg = USB_GETREG(USB_DEVVIRTUAL_DMAFIFO0_PAR1_REG_OFS + fifonum * 0x8);
			devVDMACXF1.bit.VDMA_TYPE_CXF = 0; // FIFO to memory
			devVDMACXF1.bit.VDMA_LEN_CXF  = *pDMALen;
			USB_SETREG(USB_DEVVIRTUAL_DMAFIFO0_PAR1_REG_OFS + fifonum * 0x8, devVDMACXF1.reg);

			// set Dma Addr
			if ((((UINT32)pBuffer)&0xFFF00000) == 0xF0700000)
				devVDMACXF2.reg = (UINT32)pBuffer;
			else
				devVDMACXF2.reg = dma_getPhyAddr((UINT32)pBuffer);

			USB_SETREG(USB_DEVVIRTUAL_DMAFIFO0_PAR2_REG_OFS + fifonum * 0x8, devVDMACXF2.reg);

			//dma_flushWriteCache((UINT32)pBuffer, *pDMALen);
			dma_flushReadCache((UINT32)pBuffer, *pDMALen);

			bAbortDMA = FALSE;

			// start Dma
			devVDMACXF1.reg = USB_GETREG(USB_DEVVIRTUAL_DMAFIFO0_PAR1_REG_OFS + fifonum * 0x8);
			devVDMACXF1.bit.VDMA_START_CXF = 1;
			USB_SETREG(USB_DEVVIRTUAL_DMAFIFO0_PAR1_REG_OFS + fifonum * 0x8, devVDMACXF1.reg);

		} else if (fifonum < USB_FIFO8) {

			// check if DMA being acccessed
			devVDMACXF1.reg = USB_GETREG(USB_DEVVIRTUAL_DMAFIFO4_PAR1_REG_OFS + (fifonum-USB_FIFO4) * 0x8);
			if (devVDMACXF1.bit.VDMA_START_CXF) {
				DBG_ERR("EP%d: DMA not available 0x%x\r\n", EPn, fifonum);
				return E_OBJ;
			}

			clr_flg(FLG_ID_USBTSK, (FLGUSB_DMA0DONE << fifonum));

			// Enter critical section
			loc_cpu(flags);

			// Config DMA direction & length
			devVDMACXF1.reg = USB_GETREG(USB_DEVVIRTUAL_DMAFIFO4_PAR1_REG_OFS + (fifonum-USB_FIFO4) * 0x8);
			devVDMACXF1.bit.VDMA_TYPE_CXF = 0; // FIFO to memory
			devVDMACXF1.bit.VDMA_LEN_CXF  = *pDMALen;
			USB_SETREG(USB_DEVVIRTUAL_DMAFIFO4_PAR1_REG_OFS + (fifonum-USB_FIFO4) * 0x8, devVDMACXF1.reg);

			// set Dma Addr
			if ((((UINT32)pBuffer)&0xFFF00000) == 0xF0700000)
				devVDMACXF2.reg = (UINT32)pBuffer;
			else
				devVDMACXF2.reg = dma_getPhyAddr((UINT32)pBuffer);
			USB_SETREG(USB_DEVVIRTUAL_DMAFIFO4_PAR2_REG_OFS + (fifonum-USB_FIFO4) * 0x8, devVDMACXF2.reg);

			//dma_flushWriteCache((UINT32)pBuffer, *pDMALen);
			dma_flushReadCache((UINT32)pBuffer, *pDMALen);

			bAbortDMA = FALSE;

			// start Dma
			devVDMACXF1.reg = USB_GETREG(USB_DEVVIRTUAL_DMAFIFO4_PAR1_REG_OFS + (fifonum-USB_FIFO4) * 0x8);
			devVDMACXF1.bit.VDMA_START_CXF = 1;
			USB_SETREG(USB_DEVVIRTUAL_DMAFIFO4_PAR1_REG_OFS + (fifonum-USB_FIFO4) * 0x8, devVDMACXF1.reg);

		}else {

			// check if DMA being acccessed
			devVDMACXF1.reg = USB_GETREG(USB_DEVVIRTUAL_DMAFIFO8_PAR1_REG_OFS + (fifonum-USB_FIFO8) * 0x8);
			if (devVDMACXF1.bit.VDMA_START_CXF) {
				DBG_ERR("EP%d: DMA not available 0x%x\r\n", EPn, fifonum);
				return E_OBJ;
			}

			clr_flg(FLG_ID_USBTSK, (FLGUSB_DMA0DONE << fifonum));

			// Enter critical section
			loc_cpu(flags);

			// Config DMA direction & length
			devVDMACXF1.reg = USB_GETREG(USB_DEVVIRTUAL_DMAFIFO8_PAR1_REG_OFS + (fifonum-USB_FIFO8) * 0x8);
			devVDMACXF1.bit.VDMA_TYPE_CXF = 0; // FIFO to memory
			devVDMACXF1.bit.VDMA_LEN_CXF  = *pDMALen;
			USB_SETREG(USB_DEVVIRTUAL_DMAFIFO8_PAR1_REG_OFS + (fifonum-USB_FIFO8) * 0x8, devVDMACXF1.reg);

			// set Dma Addr
			if ((((UINT32)pBuffer)&0xFFF00000) == 0xF0700000)
				devVDMACXF2.reg = (UINT32)pBuffer;
			else
				devVDMACXF2.reg = dma_getPhyAddr((UINT32)pBuffer);
			USB_SETREG(USB_DEVVIRTUAL_DMAFIFO8_PAR2_REG_OFS + (fifonum-USB_FIFO8) * 0x8, devVDMACXF2.reg);

			//dma_flushWriteCache((UINT32)pBuffer, *pDMALen);
			dma_flushReadCache((UINT32)pBuffer, *pDMALen);

			bAbortDMA = FALSE;

			// start Dma
			devVDMACXF1.reg = USB_GETREG(USB_DEVVIRTUAL_DMAFIFO8_PAR1_REG_OFS + (fifonum-USB_FIFO8) * 0x8);
			devVDMACXF1.bit.VDMA_START_CXF = 1;
			USB_SETREG(USB_DEVVIRTUAL_DMAFIFO8_PAR1_REG_OFS + (fifonum-USB_FIFO8) * 0x8, devVDMACXF1.reg);

		}

#else
		wai_flg(&flags2, FLG_ID_USBTSK, FLGPTN_USB_DMA_RDY, TWF_CLR | TWF_ORW);

		// check if DMA being acccessed
		devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
		if (devDMACtl1.bit.DMA_START) {
			DBG_ERR("EP%d: DMA-1 not available 0x%x\r\n", EPn, USB_GETREG(USB_DEVACCFIFO_REG_OFS));
			return E_OBJ;
		}

		clr_flg(FLG_ID_USBTSK, FLGUSB_DMADONE);

		// Enter critical section
		loc_cpu(flags);

		// Config DMA direction & length
		devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
		devDMACtl1.bit.DMA_TYPE = 0; // FIFO to memory
		devDMACtl1.bit.DMA_LEN  = *pDMALen;
		USB_SETREG(USB_DEVDMACTRL1_REG_OFS, devDMACtl1.reg);

		usb_setDmaTargetFifo(EPn);

		// set Dma Addr
		if ((((UINT32)pBuffer)&0xFFF00000) == 0xF0700000)
			devDMACtl2.reg = (UINT32)pBuffer;
		else
			devDMACtl2.reg = dma_getPhyAddr((UINT32)pBuffer);
		USB_SETREG(USB_DEVDMACTRL2_REG_OFS, devDMACtl2.reg);

		//dma_flushWriteCache((UINT32)pBuffer, *pDMALen);
		dma_flushReadCache((UINT32)pBuffer, *pDMALen);

		bAbortDMA = FALSE;


		// start Dma
		devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
		devDMACtl1.bit.DMA_START = 1;
		USB_SETREG(USB_DEVDMACTRL1_REG_OFS, devDMACtl1.reg);
#endif
		// Exit critical section
		unl_cpu(flags);
	}

	return E_OK;
}


/**
    Write the buffer content to FIFO

    This function is non-blocking mode and will return immediately.
    Used with usb2dev_wait_ep_done() to wait for DMA complete.

    @param[in] EPn      endpoint number (USB_EP0~USB_EP15)
    @param[in] pBuffer  buffer pointer
    @param[in] pDMALen  DMA length to be written.
                        Valid length 0x1 to 0x40 for EP0.
                        Valid length 0x1 to 0x7fffff for EP1~EP15.

    @return
        - @b E_OACV: USB driver is not opened
        - @b E_PAR: input DMA length or EP number incorrect
        - @b E_OBJ: FIFO is busy
        - @b E_SYS: USB is SUSPENDED
        - @b E_OK: start to transfer
*/
ER usb2dev_set_ep_write(USB_EP EPn, UINT8 *pBuffer, UINT32 *pDMALen)
{
#if _USE_VDMA_FIFO_
	T_USB_DEVVIRTUAL_DMACXF1_REG  devVDMACXF1;
	T_USB_DEVVIRTUAL_DMACXF2_REG  devVDMACXF2;
#else
	T_USB_DEVDMACTRL1_REG         devDMACtl1;
	T_USB_DEVDMACTRL2_REG         devDMACtl2;
#endif
	USB_FIFO_NUM                  fifonum;
#if _USE_CXF_PIO_
	T_USB_DEVCXCFGFIFOSTATUS_REG  devCXCFG;
	INT32                         i, Ep0DMASize;
	UINT32                        *pBUF;
#endif
	unsigned long                 flags;
	FLGPTN                        flags2;

	if (!gUSBOpen) {
		DBG_ERR("driver not opened\r\n");
		return E_OACV;
	}

	// We must avoid 0Byte DMAtrigger. This may cause mal-functioned in MSDC application.
	if ((*pDMALen == 0) || (*pDMALen > USB_MAX_DMA_LENGTH)) {
		return E_PAR;
	}

	// check if under SUSPEND state
	if (usb2dev_get_controller_state() == USB_CONTROLLER_STATE_SUSPEND) {
		// In SUSPEND state, UCLK is stopped and USB controller will not do jobs of DMA
		// Block DMA request when usb is SUSPENDED
		DBG_ERR("usb under SUSPENDED\r\n");
		return E_SYS;
	}

	if (EPn == USB_EP0) {
#if _USE_VDMA_FIFO_

		// check if DMA being acccessed
		devVDMACXF1.reg = USB_GETREG(USB_DEVVIRTUAL_DMACXF1_REG_OFS);
		if (devVDMACXF1.bit.VDMA_START_CXF) {
			DBG_ERR("EP%d: DMA not available\r\n", EPn);
			return E_OBJ;
		}

		//DBG_DUMP("w");
		clr_flg(FLG_ID_USBTSK, FLGUSB_EP0_DMADONE);

		// Enter critical section
		loc_cpu(flags);

		// Config DMA direction & length
		devVDMACXF1.reg = USB_GETREG(USB_DEVVIRTUAL_DMACXF1_REG_OFS);
		devVDMACXF1.bit.VDMA_TYPE_CXF = 1; // memory to FIFO
		devVDMACXF1.bit.VDMA_LEN_CXF  = *pDMALen;
		USB_SETREG(USB_DEVVIRTUAL_DMACXF1_REG_OFS, devVDMACXF1.reg);

		// set Dma Addr
		//if ((((UINT32)pBuffer)&0xFFF00000) == 0xF0700000)
		//	devVDMACXF2.reg = (UINT32)pBuffer;
		//else
			devVDMACXF2.reg = dma_getPhyAddr2((UINT32)pBuffer);
		USB_SETREG(USB_DEVVIRTUAL_DMACXF2_REG_OFS, devVDMACXF2.reg);

		dma_flushWriteCache2((UINT32)pBuffer, *pDMALen);
		dma_flushReadCache2((UINT32)pBuffer, *pDMALen);

		//bAbortDMA = FALSE;

		// start Dma
		devVDMACXF1.reg = USB_GETREG(USB_DEVVIRTUAL_DMACXF1_REG_OFS);
		devVDMACXF1.bit.VDMA_START_CXF = 1;
		USB_SETREG(USB_DEVVIRTUAL_DMACXF1_REG_OFS, devVDMACXF1.reg);

		// Exit critical section
		unl_cpu(flags);
#else

		wai_flg(&flags2, FLG_ID_USBTSK, FLGPTN_USB_DMA_RDY, TWF_CLR | TWF_ORW);

		// check if DMA being acccessed
		devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
		if (devDMACtl1.bit.DMA_START) {
			DBG_ERR("EP%d: DMA-1 not available 0x%lx\r\n", EPn, USB_GETREG(USB_DEVACCFIFO_REG_OFS));
			return E_OBJ;
		}

		clr_flg(FLG_ID_USBTSK, FLGUSB_DMADONE);

		// Enter critical section
		loc_cpu(flags);

		// Config DMA direction & length
		devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
		devDMACtl1.bit.DMA_TYPE = 1; // memory to FIFO
		devDMACtl1.bit.DMA_LEN  = *pDMALen;
		USB_SETREG(USB_DEVDMACTRL1_REG_OFS, devDMACtl1.reg);

		usb_setDmaTargetFifo(USB_EP0);

		// set Dma Addr
		//if ((((UINT32)pBuffer)&0xFFF00000) == 0xF0700000)
		//	devDMACtl2.reg = (UINT32)pBuffer;
		//else
			devDMACtl2.reg = dma_getPhyAddr((UINT32)pBuffer);
		USB_SETREG(USB_DEVDMACTRL2_REG_OFS, devDMACtl2.reg);

		dma_flushWriteCache((UINT32)pBuffer, *pDMALen);

		bAbortDMA = FALSE;

		// start Dma
		devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
		devDMACtl1.bit.DMA_START = 1;
		USB_SETREG(USB_DEVDMACTRL1_REG_OFS, devDMACtl1.reg);

		// Exit critical section
		unl_cpu(flags);
#endif
		return E_OK;
	}

	fifonum = usb_getFifoFromEp(EPn);

	if(usb_get_fifo_in_int_en(fifonum)) {
		DBG_DUMP("FIFO%d INT Not DISABLE\r\n", fifonum);
	}

	if (fifonum < USB_FIFO_MAX) {

#if _USE_VDMA_FIFO_
		//wai_flg(&flags2, FLG_ID_USB, FLGPTN_USB_DMA_RDY, TWF_CLR | TWF_ORW);
		flags2 = flags2;

		if (fifonum < USB_FIFO4) {

			// check if DMA being acccessed
			devVDMACXF1.reg = USB_GETREG(USB_DEVVIRTUAL_DMAFIFO0_PAR1_REG_OFS + fifonum * 0x8);
			if (devVDMACXF1.bit.VDMA_START_CXF) {
				DBG_ERR("EP%d: DMA-1 not available 0x%x\r\n", EPn, fifonum);
				return E_OBJ;
			}

			// 660 ISO-IN: 512x3  Size 1536+1 and 1536+2 continuously trigger.
			// For F0-F3, pre-trigger DMA is not allowed.
			// We must wait FIFo empty and then trigger next dma.
			while (usb2dev_check_ep_busy(EPn)) {
				USB_DELAY_US(50);
				//DBG_DUMP(".");
			}

			clr_flg(FLG_ID_USBTSK, (FLGUSB_DMA0DONE << fifonum));

			// Enter critical section
			loc_cpu(flags);

			// Config DMA direction & length
			devVDMACXF1.reg = USB_GETREG(USB_DEVVIRTUAL_DMAFIFO0_PAR1_REG_OFS + fifonum * 0x8);
			devVDMACXF1.bit.VDMA_TYPE_CXF = 1; // memory to FIFO
			devVDMACXF1.bit.VDMA_LEN_CXF  = *pDMALen;
			USB_SETREG(USB_DEVVIRTUAL_DMAFIFO0_PAR1_REG_OFS + fifonum * 0x8, devVDMACXF1.reg);

			// set Dma Addr
			if ((((UINT32)pBuffer)&0xFFF00000) == 0xF0700000)
				devVDMACXF2.reg = (UINT32)pBuffer;
			else
				devVDMACXF2.reg = dma_getPhyAddr((UINT32)pBuffer);
			USB_SETREG(USB_DEVVIRTUAL_DMAFIFO0_PAR2_REG_OFS + fifonum * 0x8, devVDMACXF2.reg);

			dma_flushWriteCache((UINT32)pBuffer, *pDMALen);
			//dma_flushReadCache((UINT32)pBuffer, *pDMALen);

			bAbortDMA = FALSE;

			// start Dma
			devVDMACXF1.reg = USB_GETREG(USB_DEVVIRTUAL_DMAFIFO0_PAR1_REG_OFS + fifonum * 0x8);
			devVDMACXF1.bit.VDMA_START_CXF = 1;
			USB_SETREG(USB_DEVVIRTUAL_DMAFIFO0_PAR1_REG_OFS + fifonum * 0x8, devVDMACXF1.reg);

		} else if (fifonum < USB_FIFO8) {

			// check if DMA being acccessed
			devVDMACXF1.reg = USB_GETREG(USB_DEVVIRTUAL_DMAFIFO4_PAR1_REG_OFS + (fifonum - USB_FIFO4) * 0x8);
			if (devVDMACXF1.bit.VDMA_START_CXF) {
				DBG_ERR("EP%d: DMA not available 0x%x\r\n", EPn, fifonum);
				return E_OBJ;
			}

			// 660 ISO-IN: 512x3  Size 1536+1 and 1536+2 continuously trigger.
			// For F0-F3, pre-trigger DMA is not allowed.
			// We must wait FIFo empty and then trigger next dma.
			while (usb2dev_check_ep_busy(EPn)) {
				USB_DELAY_US(50);
			}

			clr_flg(FLG_ID_USBTSK, (FLGUSB_DMA0DONE << fifonum));

			// Enter critical section
			loc_cpu(flags);

			// Config DMA direction & length
			devVDMACXF1.reg = USB_GETREG(USB_DEVVIRTUAL_DMAFIFO4_PAR1_REG_OFS + (fifonum - USB_FIFO4) * 0x8);
			devVDMACXF1.bit.VDMA_TYPE_CXF = 1; // memory to FIFO
			devVDMACXF1.bit.VDMA_LEN_CXF  = *pDMALen;
			USB_SETREG(USB_DEVVIRTUAL_DMAFIFO4_PAR1_REG_OFS + (fifonum - USB_FIFO4) * 0x8, devVDMACXF1.reg);

			// set Dma Addr
			if ((((UINT32)pBuffer)&0xFFF00000) == 0xF0700000)
				devVDMACXF2.reg = (UINT32)pBuffer;
			else
				devVDMACXF2.reg = dma_getPhyAddr((UINT32)pBuffer);
			USB_SETREG(USB_DEVVIRTUAL_DMAFIFO4_PAR2_REG_OFS + (fifonum - USB_FIFO4) * 0x8, devVDMACXF2.reg);

			dma_flushWriteCache((UINT32)pBuffer, *pDMALen);
			//dma_flushReadCache((UINT32)pBuffer, *pDMALen);

			bAbortDMA = FALSE;

			// start Dma
			devVDMACXF1.reg = USB_GETREG(USB_DEVVIRTUAL_DMAFIFO4_PAR1_REG_OFS + (fifonum - USB_FIFO4) * 0x8);
			devVDMACXF1.bit.VDMA_START_CXF = 1;
			USB_SETREG(USB_DEVVIRTUAL_DMAFIFO4_PAR1_REG_OFS + (fifonum - USB_FIFO4) * 0x8, devVDMACXF1.reg);

		} else {

			// check if DMA being acccessed
			devVDMACXF1.reg = USB_GETREG(USB_DEVVIRTUAL_DMAFIFO8_PAR1_REG_OFS + (fifonum - USB_FIFO8) * 0x8);
			if (devVDMACXF1.bit.VDMA_START_CXF) {
				DBG_ERR("EP%d: DMA not available 0x%x\r\n", EPn, fifonum);
				return E_OBJ;
			}

			// 660 ISO-IN: 512x3  Size 1536+1 and 1536+2 continuously trigger.
			// For F0-F3, pre-trigger DMA is not allowed.
			// We must wait FIFo empty and then trigger next dma.
			while (usb2dev_check_ep_busy(EPn)) {
				USB_DELAY_US(50);
			}

			clr_flg(FLG_ID_USBTSK, (FLGUSB_DMA0DONE << fifonum));

			// Enter critical section
			loc_cpu(flags);

			// Config DMA direction & length
			devVDMACXF1.reg = USB_GETREG(USB_DEVVIRTUAL_DMAFIFO8_PAR1_REG_OFS + (fifonum - USB_FIFO8) * 0x8);
			devVDMACXF1.bit.VDMA_TYPE_CXF = 1; // memory to FIFO
			devVDMACXF1.bit.VDMA_LEN_CXF  = *pDMALen;
			USB_SETREG(USB_DEVVIRTUAL_DMAFIFO8_PAR1_REG_OFS + (fifonum - USB_FIFO8) * 0x8, devVDMACXF1.reg);

			// set Dma Addr
			if ((((UINT32)pBuffer)&0xFFF00000) == 0xF0700000)
				devVDMACXF2.reg = (UINT32)pBuffer;
			else
				devVDMACXF2.reg = dma_getPhyAddr((UINT32)pBuffer);

			USB_SETREG(USB_DEVVIRTUAL_DMAFIFO8_PAR2_REG_OFS + (fifonum - USB_FIFO8) * 0x8, devVDMACXF2.reg);

			dma_flushWriteCache((UINT32)pBuffer, *pDMALen);
			//dma_flushReadCache((UINT32)pBuffer, *pDMALen);

			bAbortDMA = FALSE;

			// start Dma
			devVDMACXF1.reg = USB_GETREG(USB_DEVVIRTUAL_DMAFIFO8_PAR1_REG_OFS + (fifonum - USB_FIFO8) * 0x8);
			devVDMACXF1.bit.VDMA_START_CXF = 1;
			USB_SETREG(USB_DEVVIRTUAL_DMAFIFO8_PAR1_REG_OFS + (fifonum - USB_FIFO8) * 0x8, devVDMACXF1.reg);
		}

#else
		wai_flg(&flags2, FLG_ID_USBTSK, FLGPTN_USB_DMA_RDY, TWF_CLR | TWF_ORW);

		// check if DMA being acccessed
		devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
		if (devDMACtl1.bit.DMA_START) {
			DBG_ERR("EP%d: DMA-1 not available 0x%lx\r\n", EPn, USB_GETREG(USB_DEVACCFIFO_REG_OFS));
			return E_OBJ;
		}

		// 660 ISO-IN: 512x3  Size 1536+1 and 1536+2 continuously trigger.
		// For F0-F3, pre-trigger DMA is not allowed.
		// We must wait FIFo empty and then trigger next dma.
		while (usb2dev_check_ep_busy(EPn)) {
			USB_DELAY_US(50);
		}

		clr_flg(FLG_ID_USBTSK, FLGUSB_DMADONE);

		// Enter critical section
		loc_cpu(flags);

		// Config DMA direction & length
		devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
		devDMACtl1.bit.DMA_TYPE = 1; // memory to FIFO
		devDMACtl1.bit.DMA_LEN  = *pDMALen;
		USB_SETREG(USB_DEVDMACTRL1_REG_OFS, devDMACtl1.reg);

		usb_setDmaTargetFifo(EPn);

		// set Dma Addr
		if ((((UINT32)pBuffer)&0xFFF00000) == 0xF0700000)
			devDMACtl2.reg = (UINT32)pBuffer;
		else
			devDMACtl2.reg = dma_getPhyAddr((UINT32)pBuffer);
		USB_SETREG(USB_DEVDMACTRL2_REG_OFS, devDMACtl2.reg);

		dma_flushWriteCache((UINT32)pBuffer, *pDMALen);

		bAbortDMA = FALSE;

		// start Dma
		devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
		devDMACtl1.bit.DMA_START = 1;
		USB_SETREG(USB_DEVDMACTRL1_REG_OFS, devDMACtl1.reg);
#endif
		// Exit critical section
		unl_cpu(flags);
	}

	return E_OK;
}


/**
    Wait for endpoint DMA done

    This function is used with usb2dev_set_ep_read() or usb2dev_set_ep_write().
    It will Wait until DMA done and return actual transfered length

    @param[in] EPn          endpoint number (USB_EP0~USB_EP8)
    @param[in,out] pDMALen  input DMA length (1~0x7fffff), output actual transfered length

    @return
        - @b E_OACV: USB driver is not opened
        - @b E_OK: DMA done (maybe aborted, check return pDMALen)
*/
ER usb2dev_wait_ep_done(USB_EP EPn, UINT32 *pDMALen)
{
	USB_FIFO_NUM                fifonum;
	FLGPTN                      uiFlag;
#if 0
	UINT32                      uiChOfs;
	BOOL                        bLocalAbort;
#endif

	if (!gUSBOpen) {
		DBG_ERR("driver not opened\r\n");
		return E_OACV;
	}

	if (EPn == USB_EP0) {
		#if _USE_VDMA_FIFO_
		wai_flg(&uiFlag, FLG_ID_USBTSK, FLGUSB_EP0_DMADONE, TWF_CLR | TWF_ORW);
		#else
		wai_flg(&uiFlag, FLG_ID_USBTSK, FLGUSB_DMADONE, TWF_CLR | TWF_ORW);
		set_flg(FLG_ID_USBTSK, FLGPTN_USB_DMA_RDY);
		#endif
		//DBG_DUMP("o");

		return E_OK;
	}

	fifonum = usb_getFifoFromEp(EPn);

	if (fifonum < USB_FIFO_MAX) {
		// wait for DMA done
		#if _USE_VDMA_FIFO_
		wai_flg(&uiFlag, FLG_ID_USBTSK, (FLGUSB_DMA0DONE << fifonum), TWF_CLR | TWF_ORW);
		#else
		wai_flg(&uiFlag, FLG_ID_USBTSK, FLGUSB_DMADONE, TWF_CLR | TWF_ORW);
		#endif

		if (bAbortDMA) {
			USB_PutString1("\r\n DMA abort : EP %d, ", EPn);
			USB_PutString1("  total 0x%x , ", *pDMALen);

			*pDMALen -= USB_GETREG(USB_DEVDMACTRLSTATUS_REG_OFS);

			USB_PutString1("  received 0x%x \r\n", *pDMALen);

			if (gFIFOInMap[gEPMap[EPn - 1]] == EPn) {
				// This is a IN EP
				usb2dev_set_tx0byte(EPn);
			}
		}

#if _USE_VDMA_FIFO_
		// check if VDMA_ERROR
		if (gUSBCauseGroup[3] & (fifonum << 17)) {
			USB_PutString(" Abort VDMA transfer.. \r\n");

			// clear FIFO
			usb2dev_clear_ep_fifo(EPn);
		}
#else
		// check if DMA_ERROR/Resume/Suspend/Reset
		if (gUSBCauseGroup[2] & (BIT8 | BIT2 | BIT1 | BIT0)) {
			USB_PutString(" Abort DMA transfer.. \r\n");

			// total - remain = actual transfer size
			*pDMALen -= USB_GETREG(USB_DEVDMACTRLSTATUS_REG_OFS);

			// clear FIFO
			usb2dev_clear_ep_fifo(EPn);
		}
#endif
		// signal USB DMA channel is ready again
		set_flg(FLG_ID_USBTSK, FLGPTN_USB_DMA_RDY);
	}

	return E_OK;
}


ER usb2dev_wait_ep_done_timeout(USB_EP EPn, UINT32 *pDMALen, UINT32 timeout_ms)
{
	USB_FIFO_NUM                fifonum;
	FLGPTN                      uiFlag;
#if 0
	UINT32                      uiChOfs;
	BOOL                        bLocalAbort;
#endif

	if (!gUSBOpen) {
		DBG_ERR("driver not opened\r\n");
		return E_OACV;
	}

	if (EPn == USB_EP0) {
		#if _USE_VDMA_FIFO_
		vos_flag_wait_timeout(&uiFlag, FLG_ID_USBTSK, FLGUSB_EP0_DMADONE, TWF_CLR | TWF_ORW, vos_util_msec_to_tick(timeout_ms));
		#else
		vos_flag_wait_timeout(&uiFlag, FLG_ID_USBTSK, FLGUSB_DMADONE, TWF_CLR | TWF_ORW, vos_util_msec_to_tick(timeout_ms));
		set_flg(FLG_ID_USBTSK, FLGPTN_USB_DMA_RDY);
		#endif
		//DBG_DUMP("o");

		return E_OK;
	}

	fifonum = usb_getFifoFromEp(EPn);

	if (fifonum < USB_FIFO_MAX) {
		// wait for DMA done
		#if _USE_VDMA_FIFO_
		vos_flag_wait_timeout(&uiFlag, FLG_ID_USBTSK, (FLGUSB_DMA0DONE << fifonum), TWF_CLR | TWF_ORW, vos_util_msec_to_tick(timeout_ms));
		#else
		vos_flag_wait_timeout(&uiFlag, FLG_ID_USBTSK, FLGUSB_DMADONE, TWF_CLR | TWF_ORW, vos_util_msec_to_tick(timeout_ms));
		#endif

		if (uiFlag == 0) {
			//timeout
			*pDMALen = 0;
			return E_TMOUT;
		}

		if (bAbortDMA) {
			USB_PutString1("\r\n DMA abort : EP %d, ", EPn);
			USB_PutString1("  total 0x%x , ", *pDMALen);

			*pDMALen -= USB_GETREG(USB_DEVDMACTRLSTATUS_REG_OFS);

			USB_PutString1("  received 0x%x \r\n", *pDMALen);

			if (gFIFOInMap[gEPMap[EPn - 1]] == EPn) {
				// This is a IN EP
				usb2dev_set_tx0byte(EPn);
			}
		}

#if _USE_VDMA_FIFO_
		// check if VDMA_ERROR
		if (gUSBCauseGroup[3] & (fifonum << 17)) {
			USB_PutString(" Abort VDMA transfer.. \r\n");

			// clear FIFO
			usb2dev_clear_ep_fifo(EPn);
		}
#else
		// check if DMA_ERROR/Resume/Suspend/Reset
		if (gUSBCauseGroup[2] & (BIT8 | BIT2 | BIT1 | BIT0)) {
			USB_PutString(" Abort DMA transfer.. \r\n");

			// total - remain = actual transfer size
			*pDMALen -= USB_GETREG(USB_DEVDMACTRLSTATUS_REG_OFS);

			// clear FIFO
			usb2dev_clear_ep_fifo(EPn);
		}
#endif
		// signal USB DMA channel is ready again
		set_flg(FLG_ID_USBTSK, FLGPTN_USB_DMA_RDY);
	}

	return E_OK;
}


//@}

#endif
#if 1

/**
    @name USB endpoint and FIFO setting functions
*/
//@{

/**
    Set Configuration value to dedicate Endpoint

    Set Configuration value to dedicate Endpoint

    @param[in] EPn          Endpoint number.
    @param[in] CfgID        Configuration Select ID.
    @param[in] uiCfgValue   Configuration value.

    @return void
*/
void usb2dev_set_ep_config(USB_EP EPn, USB_EPCFG_ID CfgID, UINT32 uiCfgValue)
{
	switch (CfgID) {
	default:
		DBG_ERR("EP%d No Sprt ID %d\r\n", EPn, CfgID);
		break;
	}

}

/**
    Get Configuration value from dedicate Endpoint

    Get Configuration value from dedicate Endpoint

    @param[in] EPn      Endpoint number.
    @param[in] CfgID    Configuration Select ID.

    @return Configuration value
*/
UINT32 usb2dev_get_ep_config(USB_EP EPn, USB_EPCFG_ID CfgID)
{
	UINT32                      Ret = 0xABCD1238;

	switch (CfgID) {
	default:
		break;
	}

	return Ret;

}

/**
    Mask(Disable) the endpoint interrupt

    @param[in] EPn  endpoint number (USB_EP0~USB_EP8)

    @return void
*/
void usb2dev_mask_ep_interrupt(USB_EP EPn)
{
	if (EPn == USB_EP0) {
		T_USB_DEVINTMASKGROUP0_REG devIntMskGrp0;
		unsigned long      flags;

		loc_cpu(flags);
		devIntMskGrp0.reg = USB_GETREG(USB_DEVINTMASKGROUP0_REG_OFS);
		devIntMskGrp0.bit.MCX_IN_INT  = 1;
		devIntMskGrp0.bit.MCX_OUT_INT = 1;
		USB_SETREG(USB_DEVINTMASKGROUP0_REG_OFS, devIntMskGrp0.reg);
		unl_cpu(flags);
		return;
	}

	if (EPn >= USB_EP_MAX) {
		DBG_ERR("EP%d out of range\r\n", EPn);
		return;
	}

	if (gEPMap[EPn - 1] == USB_FIFO_NOT_USE) {
		DBG_ERR("no FIFO maps to EP%d\r\n", EPn);
		return;
	}

	if (gEPMap[EPn - 1] >= USB_FIFO_MAX) {
		DBG_ERR("EP%d maps to FIFO%d\r\n", EPn, (UINT)gEPMap[EPn - 1]);
		return;
	}

	if (gFIFOInMap[gEPMap[EPn-1]] == EPn)
		usb_maskFIFOININT(gEPMap[EPn - 1]);
	else if (gFIFOOutMap[gEPMap[EPn-1]] == EPn)
		usb_maskFIFOOUTINT(gEPMap[EPn - 1]);
}


/**
    Unmask(Enable) the endpoint interrupt

    @param[in] EPn  endpoint number (USB_EP0~USB_EP8)

    @return void
*/
void usb2dev_unmask_ep_interrupt(USB_EP EPn)
{
	if (EPn == USB_EP0) {
		T_USB_DEVINTMASKGROUP0_REG devIntMskGrp0;
		unsigned long      flags;

		loc_cpu(flags);
		devIntMskGrp0.reg = USB_GETREG(USB_DEVINTMASKGROUP0_REG_OFS);
		devIntMskGrp0.bit.MCX_IN_INT  = 1;
		devIntMskGrp0.bit.MCX_OUT_INT = 0;
		USB_SETREG(USB_DEVINTMASKGROUP0_REG_OFS, devIntMskGrp0.reg);
		unl_cpu(flags);
		return;
	}

	if (EPn >= USB_EP_MAX) {
		DBG_ERR("EP%d out of range\r\n", EPn);
		return;
	}

	if (gEPMap[EPn - 1] == USB_FIFO_NOT_USE) {
		DBG_ERR("no FIFO maps to EP%d\r\n", EPn);
		return;
	}

	if (gEPMap[EPn - 1] >= USB_FIFO_MAX) {
		DBG_ERR("EP%d maps to FIFO%d\r\n", EPn, (UINT)gEPMap[EPn - 1]);
		return;
	}

	if (gFIFOInMap[gEPMap[EPn-1]] == EPn)
		usb_unmaskFIFOININT(gEPMap[EPn - 1]);
	else if (gFIFOOutMap[gEPMap[EPn-1]] == EPn)
		usb_unmaskFIFOOUTINT(gEPMap[EPn - 1]);
}

/**
    Set endpoint0 CX_DONE

    When class library completes transfers of CX IN/OUT, it should invoke this API to enter status stage.

    @return void
*/
void usb2dev_set_ep0_done(void)
{
	T_USB_DEVCXCFGFIFOSTATUS_REG devCXCFG;

	devCXCFG.reg = USB_GETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS);
	devCXCFG.bit.CX_DONE = 1;
	USB_SETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS, devCXCFG.reg);

	//do {
	//	devCXCFG.reg = USB_GETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS);
	//	USB_DELAY_US_POLL(150);
	//}while(devCXCFG.bit.CX_DONE == 1);
}

/**
    Set endpoint stall

    When class library finds somethins wrong, it can use this API to make a function stall for this endpoint.

    @param[in] EPn  endpoint number (USB_EP0~USB_EP15)

    @return void
*/
void usb2dev_set_ep_stall(USB_EP EPn)
{
	T_USB_DEVMAXPS_INEP_REG         devEPMaxPS;
	T_USB_DEVMAXPS_OUTEP_REG		devEPMaxPSOut;
	T_USB_DEVCXCFGFIFOSTATUS_REG    devCXCFG;

	//DBG_DUMP("usb2dev_set_ep_stall EP%d\r\n", EPn);

	if (EPn == USB_EP0) {
		devCXCFG.reg = USB_GETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS);
		devCXCFG.bit.CX_STL = 1;
		USB_SETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS, devCXCFG.reg);
	} else if (EPn <= USB_EP8) {
		if (usb_getEpDirection(EPn) == EP_DIR_IN) {
			devEPMaxPS.reg = USB_GETREG(USB_DEVMAXPS_INEP1_REG_OFS + ((EPn - USB_EP1) << 2));
			devEPMaxPS.bit.STL_IEP = 1;
			USB_SETREG((USB_DEVMAXPS_INEP1_REG_OFS + ((EPn - USB_EP1) << 2)), devEPMaxPS.reg);
		} else if (usb_getEpDirection(EPn) == EP_DIR_OUT) {
			devEPMaxPSOut.reg = USB_GETREG(USB_DEVMAXPS_OUTEP1_REG_OFS + ((EPn - USB_EP1) << 2));
			devEPMaxPSOut.bit.STL_OEP = 1;
			USB_SETREG((USB_DEVMAXPS_OUTEP1_REG_OFS + ((EPn - USB_EP1) << 2)), devEPMaxPSOut.reg);
		}
	} else {
		if (usb_getEpDirection(EPn) == EP_DIR_IN) {
			devEPMaxPS.reg = USB_GETREG(USB_DEVMAXPS_INEP9_REG_OFS + ((EPn - USB_EP9) << 2));
			devEPMaxPS.bit.STL_IEP = 1;
			USB_SETREG((USB_DEVMAXPS_INEP9_REG_OFS + ((EPn - USB_EP9) << 2)), devEPMaxPS.reg);
		} else if (usb_getEpDirection(EPn) == EP_DIR_OUT) {
			devEPMaxPSOut.reg = USB_GETREG(USB_DEVMAXPS_OUTEP9_REG_OFS + ((EPn - USB_EP9) << 2));
			devEPMaxPSOut.bit.STL_OEP = 1;
			USB_SETREG((USB_DEVMAXPS_OUTEP9_REG_OFS + ((EPn - USB_EP9) << 2)), devEPMaxPSOut.reg);
		}
	}
}

/**
    clear FIFO

    @return void
*/
void usb2dev_clear_fifo(void)
{
	T_USB_DEVTEST_REG devTest;

	devTest.reg = USB_GETREG(USB_DEVTEST_REG_OFS);
	devTest.bit.TST_CLRFF = 1;
	USB_SETREG(USB_DEVTEST_REG_OFS, devTest.reg);

	usb2dev_clear_ep_fifo(USB_EP0);
}


/**
    Clear Endpoint FIFO

    Clear FIFO of related endpoint.

    @param[in] EPn  endpoint number (USB_EP0~USB_EP15)
    @return void
*/
void usb2dev_clear_ep_fifo(USB_EP EPn)
{
	UINT32                          fifo_num;
	UINT32                          uiBufNum;
	T_USB_DEVTEST_REG    			devTest;
	T_USB_DEVCXCFGFIFOSTATUS_REG    devCXCFG;
	T_USB_DEVFIFOBYTECNT_REG        devFIFOByteCnt;

	// clear FIFO
	switch (EPn) {
	case USB_EP0 : { // CX FIFO
			devCXCFG.reg = USB_GETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS);
			devCXCFG.bit.CX_CLR = 1;
			USB_SETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS, devCXCFG.reg);

			devTest.reg = USB_GETREG(USB_DEVTEST_REG_OFS);
			devTest.bit.TST_CLREA = 1;
			USB_SETREG(USB_DEVTEST_REG_OFS, devTest.reg);
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
			if (gEPMap[EPn - 1] == USB_FIFO_NOT_USE) {
				DBG_ERR("no FIFO maps to EP%d\r\n", EPn);
				return;
			}

			if (gEPMap[EPn - 1] >= USB_FIFO_MAX) {
				DBG_ERR("EP%d maps to FIFO%d\r\n", EPn, (UINT)gEPMap[EPn - 1]);
				return;
			}

			if (usb2dev_is_highspeed_enabled()) {
				uiBufNum = gUSBManage.ep_config_hs[EPn - 1].blk_num;

				if (gUSBManage.ep_config_hs[EPn - 1].blk_size > 512) {
					uiBufNum = uiBufNum << 1;
				}

				if ((uiBufNum == 6) && (gEPMap[EPn - 1] == USB_FIFO0)) {
					uiBufNum = 12;
				}
			} else {
				uiBufNum = gUSBManage.ep_config_fs[EPn - 1].blk_num;

				if (gUSBManage.ep_config_fs[EPn - 1].blk_size > 512) {
					uiBufNum = uiBufNum << 1;
				}
			}

			//DBG_DUMP("Clr-Ep%d-FIFO%d-Number=%d\r\n", (int)EPn, (int)gEPMap[EPn - 1], (int)uiBufNum);

			// All FIFO mapped to this EP should be reset.
			for (fifo_num = gEPMap[EPn - 1]; fifo_num < (gEPMap[EPn - 1] + uiBufNum); fifo_num++) {

				if(fifo_num < USB_FIFO4) {
					devFIFOByteCnt.reg  = USB_GETREG(USB_DEVFIFO0BYTECNT_REG_OFS + (fifo_num << 2));
					devFIFOByteCnt.bit.FFRST = 0x1;
					USB_SETREG(USB_DEVFIFO0BYTECNT_REG_OFS + (fifo_num << 2), devFIFOByteCnt.reg);
				} else {
					devFIFOByteCnt.reg  = USB_GETREG(USB_DEVFIFO4BYTECNT_REG_OFS + ((fifo_num - USB_FIFO4) << 2));
					devFIFOByteCnt.bit.FFRST = 0x1;
					USB_SETREG(USB_DEVFIFO4BYTECNT_REG_OFS + ((fifo_num - USB_FIFO4) << 2), devFIFOByteCnt.reg);
				}
			}
		}
		break;

	default :
		break;
	}
}


/**
    Set the endpoint to send a 0 byte data

    @param[in] EPn      endpoint number (USB_EP1~USB_EP15)

    @return void
*/
void usb2dev_set_tx0byte(USB_EP EPn)
{
	T_USB_DEVMAXPS_INEP_REG DevEPMaxPS;

	if (EPn == USB_EP0) {
		return;
	}

	if (EPn < USB_EP9) {
		DevEPMaxPS.reg = USB_GETREG(USB_DEVMAXPS_INEP1_REG_OFS + ((EPn - USB_EP1) << 2));
		DevEPMaxPS.bit.TX0BYTE_IEP = 1;
		USB_SETREG((USB_DEVMAXPS_INEP1_REG_OFS + ((EPn - USB_EP1) << 2)), DevEPMaxPS.reg);
	} else {
		DevEPMaxPS.reg = USB_GETREG(USB_DEVMAXPS_INEP9_REG_OFS + ((EPn - USB_EP9) << 2));
		DevEPMaxPS.bit.TX0BYTE_IEP = 1;
		USB_SETREG((USB_DEVMAXPS_INEP9_REG_OFS + ((EPn - USB_EP9) << 2)), DevEPMaxPS.reg);
	}
}

/**
    Get endpoint byte count.

    Some application may not know the exactly transfering size at
    the beginning. Use this function to get the FIFO byte counts.
    This API should be used after OUT interrupt issued.

    @param[in] EPn      endpoint number (USB_EP0~USB_EP15)

    @return endpoint byte count
*/
UINT32 usb2dev_get_ep_bytecount(USB_EP EPn)
{
	T_USB_DEVFIFOBYTECNT_REG        devFIFOByteCnt;
	T_USB_DEVCXCFGFIFOSTATUS_REG    devCXCFG;
	UINT32                          fifo_num;

	if (EPn >= USB_EP_MAX) {
		DBG_ERR("EP%d out of range\r\n", EPn);
		return 0;
	}

	if (EPn) {
		if (gEPMap[EPn - 1] == USB_FIFO_NOT_USE) {
			DBG_ERR("no FIFO maps to EP%d\r\n", EPn);
			return 0;
		}

		if (gEPMap[EPn - 1] >= USB_FIFO_MAX) {
			DBG_ERR("EP%d maps to FIFO%d\r\n", EPn, (UINT)gEPMap[EPn - 1]);
			return 0;
		}
	}

	if (EPn == USB_EP0) {
		devCXCFG.reg = USB_GETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS);

		return (UINT32)devCXCFG.bit.CX_FNT_OUT;
	} else {
		fifo_num = gEPMap[EPn - 1];

		if(fifo_num < 4) {
			devFIFOByteCnt.reg = USB_GETREG(USB_DEVFIFO0BYTECNT_REG_OFS + ((fifo_num) << 2));
		} else {
			devFIFOByteCnt.reg = USB_GETREG(USB_DEVFIFO4BYTECNT_REG_OFS + ((fifo_num-4) << 2));
		}

		return (UINT32)devFIFOByteCnt.bit.BC_F;
	}
}

/**
    Check endpoint FIFO empty

    @param[in] EPn      endpoint number (USB_EP1~USB_EP8)

    @return
        - @b TRUE: FIFO for this endpoint is empty
        - @b FALSE: FIFO for this endpoint is NOT empty
*/
BOOL usb2dev_check_ep_empty(USB_EP EPn)
{
	T_USB_DEVCXCFGFIFOSTATUS_REG fifoStsReg;
	T_USB_DEVIDLECNT_REG		 fifoSts2Reg;
	UINT32 uiBufNum;
	UINT32 uiMask;
	UINT32 empty_sts;

	fifoStsReg.reg = USB_GETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS);
	fifoSts2Reg.reg = USB_GETREG(USB_DEVIDLECNT_REG_OFS);

	empty_sts = (fifoStsReg.bit.F_EMP & 0xFF) + ((fifoSts2Reg.bit.F_EMP8 & 0xFF) << 8);

	switch (EPn) {
	case USB_EP0:
		return fifoStsReg.bit.CX_EMP;

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
			if (gEPMap[EPn - 1] == USB_FIFO_NOT_USE) {
				DBG_ERR("no FIFO for input EP %d\r\n", EPn);
				return FALSE;
			}
			if (gEPMap[EPn - 1] >= USB_FIFO_MAX) {
				DBG_ERR("FIFO %d for EP %d is not valid\r\n", (UINT)gEPMap[EPn - 1], EPn);
				return FALSE;
			}
			if (usb2dev_is_highspeed_enabled()) {
				uiBufNum = gUSBManage.ep_config_hs[EPn - 1].blk_num;

				if (gUSBManage.ep_config_hs[EPn - 1].blk_size > 512) {
					uiBufNum = uiBufNum << 1;
				}
			} else {
				uiBufNum = gUSBManage.ep_config_fs[EPn - 1].blk_num;

				if (gUSBManage.ep_config_fs[EPn - 1].blk_size > 512) {
					uiBufNum = uiBufNum << 1;
				}
			}

			uiMask = ((1 << uiBufNum) - 1) << gEPMap[EPn - 1];

			if ((uiMask & empty_sts) == uiMask) {
				// All FIFO for this EP are empty
				return TRUE;
			} else {
				// some FIFO for this EP are not empty
				return FALSE;
			}
		}
		break;

	default: {
			DBG_ERR("input EP %d out of range\r\n", EPn);
		}
		return FALSE;
	}
}


/**
    Check endpoint is Busy or not.


    @param[in] EPn      endpoint number (USB_EP1~USB_EP15)

    @return
        - @b FALSE:      FIFO for this endpoint is empty and DMA is not working.
        - @b TRUE:     FIFO for this endpoint is busy.
*/
BOOL usb2dev_check_ep_busy(USB_EP EPn)
{
	T_USB_DEVCXCFGFIFOSTATUS_REG    fifoStsReg;
	T_USB_DEVIDLECNT_REG		    fifoSts2Reg;
	UINT32                          uiBufNum;
	UINT32                          uiMask;
	USB_FIFO_NUM                    fifonum;
	UINT32							empty_sts;
#if _USE_VDMA_FIFO_
	T_USB_DEVVIRTUAL_DMACXF1_REG    devVDMACXF1;
#else
	T_USB_DEVDMACTRL1_REG           devDMACtl1;
#endif


	fifoStsReg.reg = USB_GETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS);
	fifoSts2Reg.reg = USB_GETREG(USB_DEVIDLECNT_REG_OFS);

	empty_sts = (fifoStsReg.bit.F_EMP & 0xFF) + ((fifoSts2Reg.bit.F_EMP8 & 0xFF) << 8);

	switch (EPn) {
	case USB_EP0:
		return fifoStsReg.bit.CX_EMP;

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
			if (gEPMap[EPn - 1] == USB_FIFO_NOT_USE) {
				DBG_ERR("no FIFO for input EP %d\r\n", EPn);
				return FALSE;
			}
			if (gEPMap[EPn - 1] >= USB_FIFO_MAX) {
				DBG_ERR("FIFO %d for EP %d is not valid\r\n", (UINT)gEPMap[EPn - 1], EPn);
				return FALSE;
			}

			fifonum = usb_getFifoFromEp(EPn);
			if (fifonum < USB_FIFO_MAX) {
				// check if DMA being acccessed
#if _USE_VDMA_FIFO_
				if(fifonum < 4) {
					devVDMACXF1.reg = USB_GETREG(USB_DEVVIRTUAL_DMAFIFO0_PAR1_REG_OFS + fifonum * 0x8);
				} else {
					devVDMACXF1.reg = USB_GETREG(USB_DEVVIRTUAL_DMAFIFO4_PAR1_REG_OFS + fifonum * 0x8);
				}
				if (devVDMACXF1.bit.VDMA_START_CXF) {
					return TRUE;
				}

#else
				devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
				if (devDMACtl1.bit.DMA_START == 1) {
					return TRUE;
				}
#endif
			}

			// If is OUT EP, the EP is available if DMA_START=0,
			// Because the fifo may be written by host.
			if (usb_getEpDirection(EPn) == EP_DIR_OUT) {
				return FALSE;
			}

			if (usb2dev_is_highspeed_enabled()) {
				uiBufNum = gUSBManage.ep_config_hs[EPn - 1].blk_num;

				if (gUSBManage.ep_config_hs[EPn - 1].blk_size > 512) {
					uiBufNum = uiBufNum << 1;
				}
			} else {
				uiBufNum = gUSBManage.ep_config_fs[EPn - 1].blk_num;

				if (gUSBManage.ep_config_fs[EPn - 1].blk_size > 512) {
					uiBufNum = uiBufNum << 1;
				}
			}

			uiMask = ((1 << uiBufNum) - 1) << gEPMap[EPn - 1];

			if ((uiMask & empty_sts) == uiMask) {
				// All FIFO for this EP are empty
				return FALSE; // Not busy
			} else {
				// some FIFO for this EP are not empty
				return TRUE;  // TRUE
			}
		}
		break;

	default: {
			DBG_ERR("input EP %d out of range\r\n", EPn);
		}
		return FALSE;
	}
}


/**
    Get SOF

    Return current SOF value

    @return SOF value
*/
UINT32 usb2dev_get_sof_number(void)
{
	return (USB_GETREG(USB_DEVSOFNUM_REG_OFS) & 0x7FF);
}
#endif
//@}

#if _EMULATION_
// Hidden API for IC test
UINT32 usb_pauseResumeDmaTest(USB_EP EPn);

UINT32 usb_pauseResumeDmaTest(USB_EP EPn)
{
#if 0
	T_USB_DEVDMA2CTRL0_REG      devDMAxCtl;
#endif
	T_USB_DEVINTGROUP2_REG      devIntGrp2;
	T_USB_DEVINTMASKGROUP2_REG  devIntMskGrp2;
	T_USB_DEVDMACTRL1_REG       devDMACtl1;
	T_USB_DEVDMACTRLSTATUS_REG  RegRemain1;
	UINT32                      loop_cnt = 0;
	BOOL                        bIntDis;
#if 0
	UINT32                      i, uiRegOfs = 0;
#endif
	USB_FIFO_NUM                fifonum;

	fifonum = usb_getFifoFromEp(EPn);

	//
	//  DMA CH1
	//
	if ((fifonum >= USB_FIFO0) && (fifonum <= USB_FIFO3)) {
		devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
		if (devDMACtl1.bit.DMA_START == 1) {
			// mask DMA_CMPLT interrupt
			devIntMskGrp2.reg = USB_GETREG(USB_DEVINTMASKGROUP2_REG_OFS);
			bIntDis = devIntMskGrp2.bit.MDMA_CMPLT;
			if (!bIntDis) {
				devIntMskGrp2.bit.MDMA_CMPLT = 1;
				USB_SETREG(USB_DEVINTMASKGROUP2_REG_OFS, devIntMskGrp2.reg);
			}

			// set DMA_ABORT & CLRFIFO
			devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
			devDMACtl1.bit.DMA_ABORT         = 1;
			//devDMACtl0.bit.CLRFIFO_DMA_ABORT = 1;
			USB_SETREG(USB_DEVDMACTRL1_REG_OFS, devDMACtl1.reg);

			// wait DMA_CMPLT and clear it
			do {
				devIntGrp2.reg = USB_GETREG(USB_DEVINTGROUP2_REG_OFS);

				loop_cnt++;
				//  check if DMA abort fail...
				if (loop_cnt > 0x20) {
					USB_PutString("\r\nDMA-1 abort fail !!!\r\n");

					// force DMA abort if fail happen...
					// devMainCtl.reg = USB_GETREG(USB_DEVMAINCTRL_REG_OFS);
					// devMainCtl.bit.FORCE_DMA_ABORT = 1;
					// USB_SETREG(USB_DEVMAINCTRL_REG_OFS, devMainCtl.reg);

					do {
						devIntGrp2.reg = USB_GETREG(USB_DEVINTGROUP2_REG_OFS);
						DBG_WRN(" @ ");

					} while (devIntGrp2.bit.DMA_CMPLT == 0);

					loop_cnt = 0;
					break;
				}
			} while (devIntGrp2.bit.DMA_CMPLT == 0);

			devIntGrp2.bit.DMA_CMPLT = 1;
			USB_SETREG(USB_DEVINTGROUP2_REG_OFS, devIntGrp2.reg);

			// disable CLRFIFO
			devDMACtl1.bit.CLRFIFO_DMA_ABORT = 0;
			USB_SETREG(USB_DEVDMACTRL1_REG_OFS, devDMACtl1.reg);

			// Pause / Resume Delay
			USB_DELAY_US(200);

			// unmask DMA_CMPLT interrupt
			devIntMskGrp2.reg = USB_GETREG(USB_DEVINTMASKGROUP2_REG_OFS);
			devIntMskGrp2.bit.MDMA_CMPLT = 0;
			USB_SETREG(USB_DEVINTMASKGROUP2_REG_OFS, devIntMskGrp2.reg);

			// Resume DMA
			{
				UINT32 Txed;

				RegRemain1.reg = USB_GETREG(USB_DEVDMACTRLSTATUS_REG_OFS);
				Txed = devDMACtl1.bit.DMA_LEN - RegRemain1.reg;

				USB_SETREG(USB_DEVDMACTRL2_REG_OFS, USB_GETREG(USB_DEVDMACTRL2_REG_OFS) + Txed);

				devDMACtl1.bit.DMA_START = 0;
				devDMACtl1.bit.DMA_LEN   = RegRemain1.reg;
				USB_SETREG(USB_DEVDMACTRL1_REG_OFS, devDMACtl1.reg);

				devDMACtl1.bit.DMA_START = 1;
				USB_SETREG(USB_DEVDMACTRL1_REG_OFS, devDMACtl1.reg);
			}
			DBG_WRN("USB Remain Size = %d; ", RegRemain1.reg);
		}

	}
#if 0
	if (fifonum >= USB_FIFO4) {
		i = fifonum - USB_FIFO4;
		uiRegOfs = i << 3;
		loop_cnt = 0;

		devDMAxCtl.reg = USB_GETREG(USB_DEVDMA2CTRL0_REG_OFS + uiRegOfs);
		if (devDMAxCtl.bit.DMA_START == 1) {
			// mask DMA_CMPLT interrupt
			devIntMskGrp2.reg = USB_GETREG(USB_DEVINTMASKGROUP2_REG_OFS);
			bIntDis = (devIntMskGrp2.reg >> (9 + i)) & 0x1;
			if (!bIntDis) {
				devIntMskGrp2.reg |= (0x1 << (9 + i));
				USB_SETREG(USB_DEVINTMASKGROUP2_REG_OFS, devIntMskGrp2.reg);
			}

			// set DMA_ABORT & CLRFIFO
			devDMAxCtl.bit.DMA_ABORT        = 1;
			devDMAxCtl.bit.DMA_START        = 0;
			//devDMAxCtl.bit.DMA_CLRFIFO    = 1;
			devIntGrp2.reg = USB_GETREG(USB_DEVINTGROUP2_REG_OFS);
			if ((devIntGrp2.reg  & (0x1 << (9 + i))) == 0) {
				USB_SETREG(USB_DEVDMA2CTRL0_REG_OFS + uiRegOfs, devDMAxCtl.reg);
				devDMAxCtl.bit.DMA_ABORT   = 0;

				// wait DMA_CMPLT and clear it
				do {
					devIntGrp2.reg = USB_GETREG(USB_DEVINTGROUP2_REG_OFS);

					loop_cnt++;
					if (loop_cnt > 0x1800000) {
						DBG_ERR("DMA-%d Abort Fail!!\r\n", i + 2);
						break;
					}
				} while ((devIntGrp2.reg & (0x1 << (9 + i))) == 0);

				// Clear Int Sts
				devIntGrp2.reg |= (0x1 << (9 + i));
				USB_SETREG(USB_DEVINTGROUP2_REG_OFS, devIntGrp2.reg);

				// disable CLRFIFO
				devDMAxCtl.bit.DMA_CLRFIFO = 0;
				USB_SETREG(USB_DEVDMA2CTRL0_REG_OFS + uiRegOfs, devDMAxCtl.reg);
			}

			// unmask DMA_CMPLT interrupt
			devIntMskGrp2.reg = USB_GETREG(USB_DEVINTMASKGROUP2_REG_OFS);
			if (!bIntDis) {
				devIntMskGrp2.reg &= ~(0x1 << (9 + i));
				USB_SETREG(USB_DEVINTMASKGROUP2_REG_OFS, devIntMskGrp2.reg);
			}

			// Pause / Resume Delay
			USB_DELAY_US(200);

			// Resume DMA
			{
				UINT32 Txed;

				RegRemain1.reg = USB_GETREG(USB_DEVDMA2CTRLSTATUS_REG_OFS + (i << 2));
				if (RegRemain1.reg > 0) {
					Txed = devDMAxCtl.bit.DMA_LEN - RegRemain1.reg;

					USB_SETREG(USB_DEVDMA2CTRL1_REG_OFS + uiRegOfs, USB_GETREG(USB_DEVDMA2CTRL1_REG_OFS + uiRegOfs) + Txed);

					devDMAxCtl.bit.DMA_START = 0;
					devDMAxCtl.bit.DMA_LEN   = RegRemain1.reg;
					USB_SETREG(USB_DEVDMA2CTRL0_REG_OFS + uiRegOfs, devDMAxCtl.reg);

					devDMAxCtl.bit.DMA_START = 1;
					USB_SETREG(USB_DEVDMA2CTRL0_REG_OFS + uiRegOfs, devDMAxCtl.reg);

					DBG_WRN("USB D%d Remain Size = %d; ", i + 2, RegRemain1.reg);
					DBG_WRN("USB D%d Txed Size = 0x%X; ", i + 2, Txed);
				} else {
					DBG_WRN("FLG ");
					set_flg(FLG_ID_USBTSK, (FLGUSB_DMA2DONE << i));
				}
			}


		}
	}
#endif
	return 0;

}

#endif
//@}


