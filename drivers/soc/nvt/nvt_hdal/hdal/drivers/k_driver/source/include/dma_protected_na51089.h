/*
    Protected header file of NT96510's DMA driver.

    The header file for protected APIs of NT96510's DMA driver.

    @file       dma_protected.h
    @ingroup    mIDrvDMA_DMA
    @note       For Novatek internal reference, don't export to agent or customer.

    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.
*/

#ifndef _DMA_PROTECTED_H
#define _DMA_PROTECTED_H

#include "plat/dma.h"
#include "comm/ddr_arb.h"

#define DMA_SUPPORT_PRECATED_DEFINE     (ENABLE)

/*
    @addtogroup mIDrvDMA_DMA
*/
//@{

/**
    @name DMA generic call back prototype

    @param[in] UINT32   event for this call back

    @return void
*/
//@{
typedef void (*DMA_GENERIC_CB)(UINT32);
//@}

typedef void (*DRV_CB)(UINT32 event);


/*
    DMA controller channel group number

    Each group represent 32 bit for 32 channel, there are 9x channel => need 4 group
*/
typedef enum {
	DMA_CH_GROUP0 = 0x0,    // represent channel 00-31
	DMA_CH_GROUP1,          // represent channel 32-63
	DMA_CH_GROUP2,          // represent channel 64-95
	DMA_CH_GROUP3,          // represent channel 96-127
    DMA_CH_GROUP4,          // represent channel 128-159
    DMA_CH_GROUP5,          // represent channel 160-191

	DMA_CH_GROUP_CNT,
	ENUM_DUMMY4WORD(DMA_CH_GROUP)
} DMA_CH_GROUP;

/*
    @name DMA channel encoding

    DMA channel setting

    @note Used in dma_setChannelPriority(), dma_getChannelPriority(),
                  dma_enableChannel(), dma_disableChannel()
*/
typedef enum {
	DMA_CH_RSV = 0x00,
	DMA_CH_FIRST_CHANNEL,
	DMA_CH_CPU = DMA_CH_FIRST_CHANNEL,
	DMA_CH_IP,
	DMA_CH_DCE_0,           // DCE input
	DMA_CH_DCE_1,           // DCE input
	DMA_CH_DCE_2,           // DCE output
	DMA_CH_DCE_3,           // DCE output
	DMA_CH_DCE_4,           // DCE output
	DMA_CH_DCE_5,           // DCE input
	DMA_CH_DCE_6,           // DCE output
	DMA_CH_DCE_7,           // DCE output
	DMA_CH_GRA_0,           // Graphic Input 0
	DMA_CH_GRA_1,           // Graphic Input 1
	DMA_CH_GRA_2,           // Graphic Input 2
	DMA_CH_GRA_3,           // Graphic output
	DMA_CH_GRA_4,           // Graphic output
	// Ctrl 0

	DMA_CH_GRA2_0,          // Graphic2 input
	DMA_CH_GRA2_1,          // Graphic2 input
	DMA_CH_GRA2_2,          // Graphic2 output
	DMA_CH_JPG0,            // JPG IMG
	DMA_CH_JPG1,            // JPG BS
	DMA_CH_JPG2,            // JPG Encode mode DC output
	DMA_CH_JPG3,
	DMA_CH_IPE0,            // IPE input
	DMA_CH_IPE1,            // IPE input
	DMA_CH_IPE2,            // IPE output
	DMA_CH_IPE3,            // IPE output
	DMA_CH_IPE4,            // IPE output
	DMA_CH_IPE5,            // IPE output
	DMA_CH_IPE6,            // IPE input
	DMA_CH_SIE_0,           // SIE output
	DMA_CH_SIE_1,           // SIE output
	// Ctrl 1
	DMA_CH_SIE_2,           // SIE output
	DMA_CH_SIE_3,           // SIE input
	DMA_CH_SIE2_0,          // SIE2 output
	DMA_CH_SIE2_1,          // SIE2 output
	DMA_CH_SIE2_2,          // SIE2 input
	DMA_CH_SIE2_3,          // SIE2 input
	DMA_CH_SIE3_0,          // SIE3 output
	DMA_CH_SIE3_1,          // SIE3 output
	DMA_CH_DIS0,            // DIS
	DMA_CH_DIS1,            // DIS input
	DMA_CH_LARB,            // Local Arbit for SIF/BMC/I2C/UART/SPI
	DMA_CH_DAI,             // DAI
	DMA_CH_IFE_0,           // IFE input
	DMA_CH_IFE_1,           // IFE input
	DMA_CH_IFE_2,           // IFE output
	DMA_CH_IME_0,           // IME input
	// Ctrl 2
	DMA_CH_IME_1,           // IME input
	DMA_CH_IME_2,           // IME input
	DMA_CH_IME_3,           // IME output
	DMA_CH_IME_4,           // IME output
	DMA_CH_IME_5,           // IME output
	DMA_CH_IME_6,           // IME output
	DMA_CH_IME_7,           // IME output
	DMA_CH_IME_8,           // IME output
	DMA_CH_IME_9,           // IME output
	DMA_CH_IME_A,           // IME output
	DMA_CH_IME_B,           // IME output
	DMA_CH_IME_C,           // IME input
	DMA_CH_IME_D,           // IME output
	DMA_CH_IME_E,           // IME input
	DMA_CH_IME_F,           // IME input
	DMA_CH_IME_10,          // IME input

	// Ctrl 3
	DMA_CH_IME_11,          // IME input
	DMA_CH_IME_12,          // IME output
	DMA_CH_IME_13,          // IME output
	DMA_CH_IME_14,          // IME input
	DMA_CH_IME_15,          // IME output
	DMA_CH_IME_16,          // IME
	DMA_CH_IME_17,          // IME output
	DMA_CH_ISE_a0,          // ISE input
	DMA_CH_ISE_a1,          // ISE output
	DMA_CH_ISE_a2,          // ISE output
	DMA_CH_IDE_a0,          // IDE V1 Y (in)
	DMA_CH_IDE_b0,          // IDE V1 C (in)
	DMA_CH_IDE_a1,          // IDE V2 Y (in/out)
	DMA_CH_IDE_b1,          // IDE V2 C (in/out)
	DMA_CH_IDE_6,           // IDE O1 PAL/A (in)
	DMA_CH_IDE_7,           // IDE O1 RGB (in)
	// Ctrl 4
	DMA_CH_SDIO,            // SDIO
	DMA_CH_SDIO2,           // SDIO2
	DMA_CH_SDIO3,           // SDIO3
	DMA_CH_NAND,            // NAND
	DMA_CH_H264_0,          // H.264 (input)
	DMA_CH_H264_1,          // H.264 (input)
	DMA_CH_H264_3,          // H.264 (input/output)
	DMA_CH_H264_4,          // H.264 (input)
	DMA_CH_H264_5,          // H.264 (output)
	DMA_CH_H264_6,          // H.264 (input)
	DMA_CH_H264_8,          // H.264 (output)
	DMA_CH_H264_9,          // H.264 (COE input)
	DMA_CH_IFE2_0,          // IFE2 input
	DMA_CH_IFE2_1,          // IFE2 output
	DMA_CH_TSE_0,           // TSE input
	DMA_CH_TSE_1,           // TSE output
	// Ctrl 5

	DMA_CH_CRYPTO,          // CRYPTO
	DMA_CH_HASH,            // Hash
	DMA_CH_CNN_0,           // CNN input
	DMA_CH_CNN_1,           // CNN input
	DMA_CH_CNN_2,           // CNN input
	DMA_CH_CNN_3,           // CNN input
	DMA_CH_CNN_4,           // CNN output
	DMA_CH_CNN_5,           // CNN output
	DMA_CH_CNN_6,           // CNN output
	DMA_CH_NUE2_0,          // NUE input
	DMA_CH_NUE2_1,          // NUE input
	DMA_CH_NUE2_2,          // NUE input
	DMA_CH_NUE2_3,          // NUE output
	DMA_CH_NUE2_4,			// NUE output
	DMA_CH_NUE2_5,			// NUE output
	DMA_CH_NUE2_6,			// NUE output

	// Ctrl 6

	DMA_CH_MDBC_0,          // MDBC input
	DMA_CH_MDBC_1,			// MDBC input
	DMA_CH_MDBC_2,			// MDBC input
	DMA_CH_MDBC_3,			// MDBC input
	DMA_CH_MDBC_4,			// MDBC input
	DMA_CH_MDBC_5,			// MDBC input
	DMA_CH_MDBC_6,			// MDBC output
	DMA_CH_MDBC_7,			// MDBC output
	DMA_CH_MDBC_8,			// MDBC input
	DMA_CH_MDBC_9,			// MDBC output
	DMA_CH_HLOAD_0,
	DMA_CH_HLOAD_1,
	DMA_CH_HLOAD_2,
	DMA_CH_AFN_0,
	DMA_CH_AFN_1,
	DMA_CH_IVE_0,
	// Ctrl7
	DMA_CH_IVE_1,
	DMA_CH_UVC_0,
	DMA_CH_UVC_1,

  DMA_CH_COUNT,

  DMA_CH_CPU_NS,

	DMA_CH_ALL = DMA_CH_COUNT,
	ENUM_DUMMY4WORD(DMA_CH)
} DMA_CH;


#if (DMA_SUPPORT_PRECATED_DEFINE == ENABLE)
static const DMA_CH DMA_CH_CPU2 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_SPI = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_SIE_4 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_RHE_0 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_RHE_1 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_RHE_2 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_RHE_3 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_RHE_4 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_RHE_5 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_RHE_6 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_RHE_7 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_ISE2_0 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_ISE2_1 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_FSD0 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_FSD1 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_FSD2 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_DRE_0 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_DRE_1 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_DRE_2 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_DRE_3 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_DRE_4 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_DRE_5 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_DRE_6 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_DRE_7 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_SVM_0 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_SVM_1 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_LARB_2 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_SIE4_0 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_SIE4_1 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_SIE4_2 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_SIE4_3 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_SIE5_0 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_SIE5_1 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_SIE5_2 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_SIE5_3 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_ISE_2 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_SDE_0 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_SDE_1 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_SDE_2 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_SDE_3 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_SDE_4 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_VPE_0 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_VPE_1 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_VPE_2 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_VPE_3 = DMA_CH_ALL + 1;
static const DMA_CH DMA_CH_VPE_4 = DMA_CH_ALL + 1;
#endif

/*
    Channel group

    Use DMA_CH_GROUP_CNT of UINT32 to represent DMA_CH_GROUP_CNT x 32 channel
*/
//typedef struct {
//	UINT32  uiChannelGroup[DMA_CH_GROUP_CNT];   // Description
//} DMA_CHANNEL, *PDMA_CHANNEL;
#if 0
/*
    DMA channel mask

    Indicate which DMA channels are required to protect/detect

    @note For DMA_WRITEPROT_ATTR
*/
typedef struct {
	// ch 0
	UINT32 bReserved0: 1;                       //< bit0: reserved (auto refresh)
	UINT32 bCPU_NS: 1;                          //< CPU_NS
	UINT32 bUSB: 1;                             //< USB_0
	UINT32 bUSB_1: 1;                           //< USB_1
	UINT32 bDCE_0: 1;                           //< DCE_0 (in)
	UINT32 bDCE_1: 1;                           //< DCE_1 (in)
	UINT32 bDCE_2: 1;                           //< DCE_2 (out)
	UINT32 bDCE_3: 1;                           //< DCE_3 (out)
	UINT32 bDCE_4: 1;                           //< DCE_4 (out)
	UINT32 bDCE_5: 1;                           //< DCE_5 (in)
	UINT32 bDCE_6: 1;                           //< DCE_6 (out)
	UINT32 bGRA_0: 1;                           //< GRA_0 (in)
	UINT32 bGRA_1: 1;                           //< GRA_1 (in)
	UINT32 bGRA_2: 1;                           //< GRA_2 (in)
	UINT32 bGRA_3: 1;                           //< GRA_3 (out)
	UINT32 bGRA_4: 1;                           //< GRA_4 (out)
	// ch 16
	UINT32 bGRA2_0: 1;                          //< GRA2_0 (in)
	UINT32 bGRA2_1: 1;                          //< GRA2_1 (in)
	UINT32 bGRA2_2: 1;                          //< GRA2_2 (out)
	UINT32 bJPG_0: 1;                           //< JPG IMG
	UINT32 bJPG_1: 1;                           //< JPG BS
	UINT32 bJPG_2: 1;                           //< JPG Enc DC out
	UINT32 bIPE_0: 1;                           //< IPE_0 (in)
	UINT32 bIPE_1: 1;                           //< IPE_1 (in)
	UINT32 bIPE_2: 1;                           //< IPE_2 (out)
	UINT32 bIPE_3: 1;                           //< IPE_3 (out)
	UINT32 bIPE_4: 1;                           //< IPE_4 (out)
	UINT32 bIPE_5: 1;                           //< IPE_5 (out)
	UINT32 bIPE_6: 1;                           //< IPE_6 (in)
	UINT32 bSIE_0: 1;                           //< SIE_0 (out)
	UINT32 bSIE_1: 1;                           //< SIE_1 (out)
	UINT32 bSIE_2: 1;                           //< SIE_2 (in)
	// ch 32
	UINT32 bSIE_3: 1;                           //< SIE_3 (in)
	UINT32 bSIE2_0: 1;                          //< SIE2_0 (out)
	UINT32 bSIE2_1: 1;                          //< SIE2_1 (out)
	UINT32 bSIE2_2: 1;                          //< SIE2_2 (in)
	UINT32 bSIE2_3: 1;                          //< SIE2_3 (in)
	UINT32 bSIE3_0: 1;                          //< SIE3_0 (out)
	UINT32 bSIE3_1: 1;                          //< SIE3_1 (out)
	UINT32 bDIS_0: 1;                           //< DIS_0 (in/out)
	UINT32 bDIS_1: 1;                           //< DIS_1 (in)
	UINT32 bLARB: 1;                            //< Local arbiter (SIF/BMC/I2C/UART/SPI)
	UINT32 bDAI: 1;                             //< DAI
	UINT32 bIFE_0: 1;                           //< IFE_0 (in)
	UINT32 bIFE_1: 1;                           //< IFE_1 (in)
	UINT32 bIFE_2: 1;                           //< IFE_2 (out)
	UINT32 bIME_0: 1;                           //< IME_0 (in)
	UINT32 bIME_1: 1;                           //< IME_1 (in)
	// ch 48
	UINT32 bIME_2: 1;                           //< IME_2 (in)
	UINT32 bIME_3: 1;                           //< IME_3 (out)
	UINT32 bIME_4: 1;                           //< IME_4 (out)
	UINT32 bIME_5: 1;                           //< IME_5 (out)
	UINT32 bIME_6: 1;                           //< IME_6 (out)
	UINT32 bIME_7: 1;                           //< IME_7 (out)
	UINT32 bIME_8: 1;                           //< IME_8 (out)
	UINT32 bIME_9: 1;                           //< IME_9 (out)
	UINT32 bIME_A: 1;                           //< IME_A (out)
	UINT32 bIME_B: 1;                           //< IME_B (out)
	UINT32 bIME_C: 1;                           //< IME_C (in)
	UINT32 bIME_D: 1;                           //< IME_D (out)
	UINT32 bIME_E: 1;                           //< IME_E (in)
	UINT32 bIME_F: 1;                           //< IME_F (in)
	UINT32 bIME_10: 1;                          //< IME_10 (in)
	UINT32 bIME_11: 1;                          //< IME_11 (in)
	// ch 64
	UINT32 bIME_12: 1;                          //< IME_12 (out)
	UINT32 bIME_13: 1;                          //< IME_13 (out)
	UINT32 bIME_14: 1;                          //< IME_14 (in)
	UINT32 bIME_15: 1;                          //< IME_15 (out)
	UINT32 bIME_16: 1;                          //< IME_16 (in/out)
	UINT32 bIME_17: 1;                          //< IME_17 (out)
	UINT32 bISE_0: 1;                           //< ISE_0 (in)
	UINT32 bISE_1: 1;                           //< ISE_1 (out)
	UINT32 bIDE_0: 1;                           //< IDE_0 (in)
	UINT32 bIDE_1: 1;                           //< IDE_1 (in)
	UINT32 bIDE_2: 1;                           //< IDE_2 (in/out)
	UINT32 bIDE_3: 1;                           //< IDE_3 (in/out)
	UINT32 bIDE_4: 1;                           //< IDE_4 (in)
	UINT32 bIDE_5: 1;                           //< IDE_5 (in)
	UINT32 bSDIO: 1;                            //< SDIO
	UINT32 bSDIO2: 1;                           //< SDIO2
	// ch 80
	UINT32 bSDIO3: 1;                           //< SDIO3
	UINT32 bNAND: 1;                            //< NAND
	UINT32 bH264_0: 1;                          //< H.264_0
	UINT32 bH264_1: 1;                          //< H.264_1
	UINT32 bH264_3: 1;                          //< H.264_3
	UINT32 bH264_4: 1;                          //< H.264_4
	UINT32 bH264_5: 1;                          //< H.264_5
	UINT32 bH264_6: 1;                          //< H.264_6
	UINT32 bH264_7: 1;                          //< H.264_7
	UINT32 bH264_8: 1;                          //< H.264_8
	UINT32 bH264_9: 1;                          //< H.264_9 (COE)
	UINT32 bIFE2_0: 1;                          //< IFE2_0 (in)
	UINT32 bIFE2_1: 1;                          //< IFE2_1 (out)
	UINT32 bETHERNET: 1;                        //< Ethernet
	UINT32 bTSE: 1;                             //< TSE input
	UINT32 bTSE_1: 1;                           //< TSE output
	// ch 96
	UINT32 bCRYPTO: 1;                          //< CRYPTO (in/out)
	UINT32 bHASH: 1;                            //< Hash (in/out)
	UINT32 bCNN_0: 1;                           //< CNN_0 (in)
	UINT32 bCNN_1: 1;                           //< CNN_1 (in)
	UINT32 bCNN_2: 1;                           //< CNN_2 (in)
	UINT32 bCNN_3: 1;                           //< CNN_3 (in)
	UINT32 bCNN_4: 1;                           //< CNN_4 (out)
	UINT32 bCNN_5: 1;                           //< CNN_5 (out)
	UINT32 bNUE_0: 1;                           //< NUE_0 (in)
	UINT32 bNUE_1: 1;                           //< NUE_1 (in)
	UINT32 bNUE_2: 1;                           //< NUE_2 (out)
	UINT32 bNUE2_0: 1;                          //< NUE2_0 (in)
	UINT32 bNUE2_1: 1;                          //< NUE2_1 (in)
	UINT32 bNUE2_2: 1;                          //< NUE2_2 (in)
	UINT32 bNUE2_3: 1;                          //< NUE2_3 (out)
	UINT32 bNUE2_4: 1;							//< NUE2_4 (out)
	// ch 112
	UINT32 bNUE2_5: 1;							//< NUE2_5 (out)
	UINT32 bMDBC_0: 1;                          //< MDBC_0 (in)
	UINT32 bMDBC_1: 1;							//< MDBC_1 (in)
	UINT32 bMDBC_2: 1;							//< MDBC_2 (in)
	UINT32 bMDBC_3: 1;							//< MDBC_3 (in)
	UINT32 bMDBC_4: 1;							//< MDBC_4 (in)
	UINT32 bMDBC_5: 1;							//< MDBC_5 (in)
	UINT32 bMDBC_6: 1;							//< MDBC_6 (out)
	UINT32 bMDBC_7: 1;							//< MDBC_7 (out)
	UINT32 bMDBC_8: 1;							//< MDBC_8 (in)
	UINT32 bMDBC_9: 1;							//< MDBC_9 (out)
	UINT32 bHLOAD_0: 1;                         //< Heavy load
	UINT32 bHLOAD_1: 1;							//< Heavy load
	UINT32 bHLOAD_2: 1;							//< Heavy load
	UINT32 bCNN2_0: 1;
	UINT32 bCNN2_1: 1;
	// ch 128
	UINT32 bCNN2_2: 1;
	UINT32 bCNN2_3: 1;
	UINT32 bCNN2_4: 1;
	UINT32 bCNN2_5: 1;
	UINT32 bDCE_7: 1;
	UINT32 bAFN_0: 1;
	UINT32 bAFN_1: 1;
	UINT32 bIVE_0: 1;
	UINT32 bIVE_1: 1;
	UINT32 bReserved: 23;
	// ch 160
	UINT32 bCPU: 1;
	UINT32 bReserved2: 15;

	UINT32 bCPU2: 1;                           // for compatible
	UINT32 bDSP_0: 1;
	UINT32 bDSP_1: 1;
	UINT32 bDSP2_0: 1;
	UINT32 bDSP2_1: 1;

} DMA_CH_MSK, *PDMA_CH_MSK;
#endif

/*
    DMA priority mask

    Indicate each channel's DMA priority

    @note For dma_set_system_priority()
*/
typedef struct {
	// ch 0
	UINT32 RSV0: 2;                             //< bit0: reserved (auto refresh)
	UINT32 CPU: 2;                              //< CPU
	UINT32 IP_USB_ETH: 2;                            //< USB_0
	UINT32 DCE_0: 2;                            //< DCE_0 (in)
	UINT32 DCE_1: 2;                            //< DCE_1 (in)
	UINT32 DCE_2: 2;                            //< DCE_2 (out)
	UINT32 DCE_3: 2;                            //< DCE_3 (out)
	UINT32 DCE_4: 2;                            //< DCE_4 (out)
	UINT32 DCE_5: 2;                            //< DCE_5 (in)
	UINT32 DCE_6: 2;                            //< DCE_6 (out)
	UINT32 DCE_7: 2;                            //< DCE_6 (out)
	UINT32 GRA_0: 2;                            //< GRA_0 (in)
	UINT32 GRA_1: 2;                            //< GRA_1 (in)
	UINT32 GRA_2: 2;                            //< GRA_2 (in)
	UINT32 GRA_3: 2;                            //< GRA_3 (out)
	UINT32 GRA_4: 2;                            //< GRA_4 (out)
	// ch 16
	UINT32 GRA2_0: 2;                           //< GRA2_0 (in)
	UINT32 GRA2_1: 2;                           //< GRA2_1 (in)
	UINT32 GRA2_2: 2;                           //< GRA2_2 (out)
	UINT32 JPG_0: 2;                            //< JPG IMG
	UINT32 JPG_1: 2;                            //< JPG BS
	UINT32 JPG_2: 2;                            //< JPG Enc DC out
	UINT32 JPG_3: 2;                            //< JPG Enc DC out
	UINT32 IPE_0: 2;                            //< IPE_0 (in)
	UINT32 IPE_1: 2;                            //< IPE_1 (in)
	UINT32 IPE_2: 2;                            //< IPE_2 (out)
	UINT32 IPE_3: 2;                            //< IPE_3 (out)
	UINT32 IPE_4: 2;                            //< IPE_4 (out)
	UINT32 IPE_5: 2;                            //< IPE_5 (out)
	UINT32 IPE_6: 2;                            //< IPE_6 (in)
	UINT32 SIE_0: 2;                            //< SIE_0 (out)
	UINT32 SIE_1: 2;                            //< SIE_1 (out)
	// ch 32
	UINT32 SIE_2: 2;                            //< SIE_2 (in)
	UINT32 SIE_3: 2;                            //< SIE_3 (in)
	UINT32 SIE2_0: 2;                           //< SIE2_0 (out)
	UINT32 SIE2_1: 2;                           //< SIE2_1 (out)
	UINT32 SIE2_2: 2;                           //< SIE2_2 (in)
	UINT32 SIE2_3: 2;                           //< SIE2_3 (in)
	UINT32 SIE3_0: 2;                           //< SIE3_0 (out)
	UINT32 SIE3_1: 2;                           //< SIE3_1 (out)
	UINT32 DIS_0: 2;                            //< DIS_0 (in/out)
	UINT32 DIS_1: 2;                            //< DIS_1 (in)
	UINT32 LARB: 2;                             //< Local arbiter (SIF/BMC/I2C/UART/SPI)
	UINT32 DAI: 2;                              //< DAI
	UINT32 IFE_0: 2;                            //< IFE_0 (in)
	UINT32 IFE_1: 2;                            //< IFE_1 (in)
	UINT32 IFE_2: 2;                            //< IFE_2 (out)
	UINT32 IME_0: 2;                            //< IME_0 (in)
	// ch 48
	UINT32 IME_1: 2;                            //< IME_1 (in)
	UINT32 IME_2: 2;                            //< IME_2 (in)
	UINT32 IME_3: 2;                            //< IME_3 (out)
	UINT32 IME_4: 2;                            //< IME_4 (out)
	UINT32 IME_5: 2;                            //< IME_5 (out)
	UINT32 IME_6: 2;                            //< IME_6 (out)
	UINT32 IME_7: 2;                            //< IME_7 (out)
	UINT32 IME_8: 2;                            //< IME_8 (out)
	UINT32 IME_9: 2;                            //< IME_9 (out)
	UINT32 IME_A: 2;                            //< IME_A (out)
	UINT32 IME_B: 2;                            //< IME_B (out)
	UINT32 IME_C: 2;                            //< IME_C (in)
	UINT32 IME_D: 2;                            //< IME_D (out)
	UINT32 IME_E: 2;                            //< IME_E (in)
	UINT32 IME_F: 2;                            //< IME_F (in)
	UINT32 IME_10: 2;                           //< IME_10 (in)
	// ch 64
	UINT32 IME_11: 2;                           //< IME_11 (in)
	UINT32 IME_12: 2;                           //< IME_12 (out)
	UINT32 IME_13: 2;                           //< IME_13 (out)
	UINT32 IME_14: 2;                           //< IME_14 (in)
	UINT32 IME_15: 2;                           //< IME_15 (out)
	UINT32 IME_16: 2;                           //< IME_16 (in/out)
	UINT32 IME_17: 2;                           //< IME_17 (out)
	UINT32 ISE_0: 2;                            //< ISE_0 (in)
	UINT32 ISE_1: 2;                            //< ISE_1 (out)
	UINT32 ISE_2: 2;                            //< ISE_1 (out)
	UINT32 IDE_0: 2;                            //< IDE_0 (in)
	UINT32 IDE_1: 2;                            //< IDE_1 (in)
	UINT32 IDE_2: 2;                            //< IDE_2 (in/out)
	UINT32 IDE_3: 2;                            //< IDE_3 (in/out)
	UINT32 IDE_4: 2;                            //< IDE_4 (in)
	UINT32 IDE_5: 2;                            //< IDE_5 (in)
	// ch 80
	UINT32 SDIO: 2;                             //< SDIO
	UINT32 SDIO2: 2;                            //< SDIO2
	UINT32 SDIO3: 2;                            //< SDIO3
	UINT32 NAND: 2;                             //< NAND
	UINT32 H264_0: 2;                           //< H.264_0
	UINT32 H264_1: 2;                           //< H.264_1
	UINT32 H264_3: 2;                           //< H.264_3
	UINT32 H264_4: 2;                           //< H.264_4
	UINT32 H264_5: 2;                           //< H.264_5
	UINT32 H264_6: 2;                           //< H.264_6
	UINT32 H264_8: 2;                           //< H.264_8
	UINT32 H264_9: 2;                           //< H.264_9 (COE)
	UINT32 IFE2_0: 2;                           //< IFE2_0 (in)
	UINT32 IFE2_1: 2;                           //< IFE2_1 (out)
	UINT32 TSE: 2;                              //< TSE input
	UINT32 TSE_1: 2;                            //< TSE output
	// ch 96
	UINT32 CRYPTO: 2;                           //< CRYPTO (in/out)
	UINT32 HASH: 2;                             //< Hash (in/out)
	UINT32 CNN_0: 2;                            //< CNN_0 (in)
	UINT32 CNN_1: 2;                            //< CNN_1 (in)
	UINT32 CNN_2: 2;                            //< CNN_2 (in)
	UINT32 CNN_3: 2;                            //< CNN_3 (in)
	UINT32 CNN_4: 2;                            //< CNN_4 (out)
	UINT32 CNN_5: 2;                            //< CNN_5 (out)
	UINT32 CNN_6: 2;                            //< CNN_5 (out)
	UINT32 NUE2_0: 2;                           //< NUE2_0 (in)
	UINT32 NUE2_1: 2;                           //< NUE2_1 (in)
	UINT32 NUE2_2: 2;                           //< NUE2_2 (in)
	UINT32 NUE2_3: 2;                           //< NUE2_3 (out)
	UINT32 NUE2_4: 2;                           //< NUE2_4 (out)
	UINT32 NUE2_5: 2;                           //< NUE2_4 (out)
	UINT32 NUE2_6: 2;                           //< NUE2_4 (out)
	// ch 112
	UINT32 MDBC_0: 2;                           //< MDBC_0 (in)
	UINT32 MDBC_1: 2;                           //< MDBC_1 (in)
	UINT32 MDBC_2: 2;                           //< MDBC_2 (in)
	UINT32 MDBC_3: 2;                           //< MDBC_3 (in)
	UINT32 MDBC_4: 2;                           //< MDBC_4 (in)
	UINT32 MDBC_5: 2;                           //< MDBC_5 (in)
	UINT32 MDBC_6: 2;                           //< MDBC_6 (out)
	UINT32 MDBC_7: 2;                           //< MDBC_7 (out)
	UINT32 MDBC_8: 2;                           //< MDBC_8 (in)
	UINT32 MDBC_9: 2;                           //< MDBC_9 (out)
	UINT32 HLOAD_0: 2;
	UINT32 HLOAD_1: 2;
	UINT32 HLOAD_2: 2;
	UINT32 AFN_0: 2;
	UINT32 AFN_1: 2;
	UINT32 IVE_0: 2;
	// ch 128
	UINT32 IVE_1: 2;
	UINT32 UVC_0: 2;
	UINT32 UVC_1: 2;

} DMA_PRIO_MSK, *PDMA_PRIO_MSK;

#if 0
/*
    DRAM protect set

    DMA controller support max 3 write protect/detect set.
    Each set has individual protected/detected DRAM range
    and DMA channels to be protected/detected.

    @note Used in dma_chkDmaWR2ProtectAddr(), dma_clrWPStatus(), dma_getWPStatus(),
                  dma_configWPFunc(), dma_enableWPFunc(), dma_disableWPFunc()
*/
typedef enum {
	DMA_WPSET_0,            // Write protect function set 0
	DMA_WPSET_1,            // Write protect function set 1
	DMA_WPSET_2,            // Write protect function set 2
	DMA_WPSET_3,            // Write protect function set 3
	DMA_WPSET_4,            // Write protect function set 4
	DMA_WPSET_5,            // Write protect function set 5
	DMA_WPSET_TOTAL,
	ENUM_DUMMY4WORD(DMA_WRITEPROT_SET)
} DMA_WRITEPROT_SET;

/*
    DRAM protect region in a set
*/
typedef enum {
	DMA_PROT_RGN0,
	DMA_PROT_RGN1,
	DMA_PROT_RGN2,
	DMA_PROT_RGN3,
	DMA_PROT_RGN_TOTAL,
	ENUM_DUMMY4WORD(DMA_PROT_REGION)
} DMA_PROT_REGION;
#endif

/*
    DRAM write protect source
    (OBSOLETE)
*/
typedef enum {
	DMA_WPSRC_CPU,          // from CPU, including AMBA DMA, AHBC.
	DMA_WPSRC_DMA,          // DMA
	DMA_WPSRC_ALL,          // CPU+DMA

	ENUM_DUMMY4WORD(DMA_WRITEPROT_SRC)
} DMA_WRITEPROT_SRC;

#if 0
/*
    DRAM write protect detect level
*/
typedef enum {
	DMA_WPLEL_UNWRITE,      // Not only detect write action but also denial access.
	DMA_WPLEL_DETECT,       // Only detect write action but allow write access.
	DMA_RPLEL_UNREAD,       // Not only detect read action but also denial access.
	DMA_RWPLEL_UNRW,        // Not only detect read write action but also denial access.
	ENUM_DUMMY4WORD(DMA_WRITEPROT_LEVEL)
} DMA_WRITEPROT_LEVEL;

/*
    DRAM protect detect level(NEW)
*/
typedef enum {
	DMA_PROT_WP,            // Not only detect write action but also denial access.
	DMA_PROT_WD,            // Only detect write action but allow write access.
	DMA_PROT_RP,
	DMA_PROT_RWP,
	ENUM_DUMMY4WORD(DMA_PROT_LEVEL)
} DMA_PROT_LEVEL;

/*
    DRAM protect mode(NEW)
*/
typedef enum {
	DMA_PROT_IN,
	DMA_PROT_OUT,
	ENUM_DUMMY4WORD(DMA_PROT_MODE)
} DMA_PROT_MODE;
#endif


/*
    @name DMA protect channel name
*/
typedef enum {
	DMA_WPCH_FIRST_CHANNEL = 1,

	DMA_WPCH_CPU = DMA_WPCH_FIRST_CHANNEL,
	DMA_WPCH_IP,
	DMA_WPCH_DCE_0,           // DCE input
	DMA_WPCH_DCE_1,           // DCE input
	DMA_WPCH_DCE_2,           // DCE output
	DMA_WPCH_DCE_3,           // DCE output
	DMA_WPCH_DCE_4,           // DCE output
	DMA_WPCH_DCE_5,           // DCE input
	DMA_WPCH_DCE_6,           // DCE output
	DMA_WPCH_DCE_7,           // DCE output
	DMA_WPCH_GRA_0,           // Graphic Input 0
	DMA_WPCH_GRA_1,           // Graphic Input 1
	DMA_WPCH_GRA_2,           // Graphic Input 2
	DMA_WPCH_GRA_3,           // Graphic output
	DMA_WPCH_GRA_4,           // Graphic output
	// Ctrl 0

	DMA_WPCH_GRA2_0,          // Graphic2 input
	DMA_WPCH_GRA2_1,          // Graphic2 input
	DMA_WPCH_GRA2_2,          // Graphic2 output
	DMA_WPCH_JPG0,            // JPG IMG
	DMA_WPCH_JPG1,            // JPG BS
	DMA_WPCH_JPG2,            // JPG Encode mode DC output
	DMA_WPCH_JPG3,
	DMA_WPCH_IPE0,            // IPE input
	DMA_WPCH_IPE1,            // IPE input
	DMA_WPCH_IPE2,            // IPE output
	DMA_WPCH_IPE3,            // IPE output
	DMA_WPCH_IPE4,            // IPE output
	DMA_WPCH_IPE5,            // IPE output
	DMA_WPCH_IPE6,            // IPE input
	DMA_WPCH_SIE_0,           // SIE output
	DMA_WPCH_SIE_1,           // SIE output
	// Ctrl 1
	DMA_WPCH_SIE_2,           // SIE output
	DMA_WPCH_SIE_3,           // SIE input
	DMA_WPCH_SIE2_0,          // SIE2 output
	DMA_WPCH_SIE2_1,          // SIE2 output
	DMA_WPCH_SIE2_2,          // SIE2 input
	DMA_WPCH_SIE2_3,          // SIE2 input
	DMA_WPCH_SIE3_0,          // SIE3 output
	DMA_WPCH_SIE3_1,          // SIE3 output
	DMA_WPCH_DIS0,            // DIS
	DMA_WPCH_DIS1,            // DIS input
	DMA_WPCH_LARB,            // Local Arbit for SIF/BMC/I2C/UART/SPI
	DMA_WPCH_DAI,             // DAI
	DMA_WPCH_IFE_0,           // IFE input
	DMA_WPCH_IFE_1,           // IFE input
	DMA_WPCH_IFE_2,           // IFE output
	DMA_WPCH_IME_0,           // IME input
	// Ctrl 2
	DMA_WPCH_IME_1,           // IME input
	DMA_WPCH_IME_2,           // IME input
	DMA_WPCH_IME_3,           // IME output
	DMA_WPCH_IME_4,           // IME output
	DMA_WPCH_IME_5,           // IME output
	DMA_WPCH_IME_6,           // IME output
	DMA_WPCH_IME_7,           // IME output
	DMA_WPCH_IME_8,           // IME output
	DMA_WPCH_IME_9,           // IME output
	DMA_WPCH_IME_A,           // IME output
	DMA_WPCH_IME_B,           // IME output
	DMA_WPCH_IME_C,           // IME input
	DMA_WPCH_IME_D,           // IME output
	DMA_WPCH_IME_E,           // IME input
	DMA_WPCH_IME_F,           // IME input
	DMA_WPCH_IME_10,          // IME input

	// Ctrl 3
	DMA_WPCH_IME_11,          // IME input
	DMA_WPCH_IME_12,          // IME output
	DMA_WPCH_IME_13,          // IME output
	DMA_WPCH_IME_14,          // IME input
	DMA_WPCH_IME_15,          // IME output
	DMA_WPCH_IME_16,          // IME
	DMA_WPCH_IME_17,          // IME output
	DMA_WPCH_ISE_a0,          // ISE input
	DMA_WPCH_ISE_a1,          // ISE output
	DMA_WPCH_ISE_a2,          // ISE output
	DMA_WPCH_IDE_a0,          // IDE V1 Y (in)
	DMA_WPCH_IDE_b0,          // IDE V1 C (in)
	DMA_WPCH_IDE_a1,          // IDE V2 Y (in/out)
	DMA_WPCH_IDE_b1,          // IDE V2 C (in/out)
	DMA_WPCH_IDE_6,           // IDE O1 PAL/A (in)
	DMA_WPCH_IDE_7,           // IDE O1 RGB (in)
	// Ctrl 4
	DMA_WPCH_SDIO,            // SDIO
	DMA_WPCH_SDIO2,           // SDIO2
	DMA_WPCH_SDIO3,           // SDIO3
	DMA_WPCH_NAND,            // NAND
	DMA_WPCH_H264_0,          // H.264 (input)
	DMA_WPCH_H264_1,          // H.264 (input)
	DMA_WPCH_H264_3,          // H.264 (input/output)
	DMA_WPCH_H264_4,          // H.264 (input)
	DMA_WPCH_H264_5,          // H.264 (output)
	DMA_WPCH_H264_6,          // H.264 (input)
	DMA_WPCH_H264_8,          // H.264 (output)
	DMA_WPCH_H264_9,          // H.264 (COE input)
	DMA_WPCH_IFE2_0,          // IFE2 input
	DMA_WPCH_IFE2_1,          // IFE2 output
	DMA_WPCH_TSE_0,           // TSE input
	DMA_WPCH_TSE_1,           // TSE output
	// Ctrl 5

	DMA_WPCH_CRYPTO,          // CRYPTO
	DMA_WPCH_HASH,            // Hash
	DMA_WPCH_CNN_0,           // CNN input
	DMA_WPCH_CNN_1,           // CNN input
	DMA_WPCH_CNN_2,           // CNN input
	DMA_WPCH_CNN_3,           // CNN input
	DMA_WPCH_CNN_4,           // CNN output
	DMA_WPCH_CNN_5,           // CNN output
	DMA_WPCH_CNN_6,           // CNN output
	DMA_WPCH_NUE2_0,          // NUE input
	DMA_WPCH_NUE2_1,          // NUE input
	DMA_WPCH_NUE2_2,          // NUE input
	DMA_WPCH_NUE2_3,          // NUE output
	DMA_WPCH_NUE2_4,			// NUE output
	DMA_WPCH_NUE2_5,			// NUE output
	DMA_WPCH_NUE2_6,			// NUE output

	// Ctrl 6

	DMA_WPCH_MDBC_0,          // MDBC input
	DMA_WPCH_MDBC_1,			// MDBC input
	DMA_WPCH_MDBC_2,			// MDBC input
	DMA_WPCH_MDBC_3,			// MDBC input
	DMA_WPCH_MDBC_4,			// MDBC input
	DMA_WPCH_MDBC_5,			// MDBC input
	DMA_WPCH_MDBC_6,			// MDBC output
	DMA_WPCH_MDBC_7,			// MDBC output
	DMA_WPCH_MDBC_8,			// MDBC input
	DMA_WPCH_MDBC_9,			// MDBC output
	DMA_WPCH_HLOAD_0,
	DMA_WPCH_HLOAD_1,
	DMA_WPCH_HLOAD_2,
	DMA_WPCH_AFN_0,
	DMA_WPCH_AFN_1,
	DMA_WPCH_IVE_0,
	// Ctrl7
	DMA_WPCH_IVE_1,
	DMA_WPCH_UVC_0,
	DMA_WPCH_UVC_1,


  DMA_WPCH_CPU_NS,

	DMA_WPCH_COUNT,

	DMA_WPCH_ALL = DMA_WPCH_COUNT,
	ENUM_DUMMY4WORD(DMA_WRITEPROT_CH)
} DMA_WRITEPROT_CH;
//STATIC_ASSERT((DMA_WPCH_COUNT == DMA_CH_COUNT));

/*
    DRAM direction

    @note for dma_configMonitor()
*/
typedef enum {
	DMA_DIRECTION_READ,     // DMA read (DRAM -> Module)
	DMA_DIRECTION_WRITE,    // DMA write (DRAM <- Module)
	DMA_DIRECTION_BOTH,     // DMA read and write

	ENUM_DUMMY4WORD(DMA_DIRECTION)
} DMA_DIRECTION;

#if (DMA_SUPPORT_PRECATED_DEFINE == ENABLE)
#endif

#if 0
/*
    DMA controller write protect function configuration

    DMA controller write protect function configuration
*/
typedef struct {
	DMA_WRITEPROT_SRC   uiProtectSrc __attribute__((deprecated));   // (OBSOLETE) protect source
	DMA_CH_MSK          chEnMask;       // DMA channel masks to be protected/detected
	DMA_WRITEPROT_LEVEL uiProtectlel;   // protect level
	UINT32              uiStartingAddr; // DDR3:must be 4 words alignment
	UINT32              uiSize;         // DDR3:must be 4 words alignment
} DMA_WRITEPROT_ATTR, *PDMA_WRITEPROT_ATTR;

/*
    DMA controller protect function configuration(NEW)

    DMA controller protect function configuration
*/
typedef struct {
	BOOL                en;            // enable this region
	UINT32              starting_addr; // DDR3:must be 4 words alignment
	UINT32              size;          // DDR3:must be 4 words alignment
} DMA_PROT_RGN_ATTR, *PDMA_PROT_RGN_ATTR;

/*
    DMA controller protect function configuration(NEW)

    DMA controller protect function configuration
*/
typedef struct {
	DMA_CH_MSK          ch_en_mask;       // DMA channel masks to be protected/detected
	DMA_PROT_MODE       protect_mode;
	DMA_PROT_LEVEL      protect_level;    // protect level
	DMA_PROT_RGN_ATTR   protect_rgn_attr[DMA_PROT_RGN_TOTAL];
} DMA_PROT_ATTR, *PDMA_PROT_ATTR;
#endif

/*
    DMA controller write protect channel group

    DMA controller write protect channel group
*/
typedef struct {
	UINT32  uiChannelGroup[DMA_CH_GROUP_CNT];
} DMA_WP_STS_TYPE, *PDMA_WP_STS_TYPE;

/*
    DMA priority setting

    DMA priority setting (Super High > High > Middle > Normal)
*/
typedef enum {
	DMA_PRIORITY_LOW,           // Low priority (Default value)
	DMA_PRIORITY_MIDDLE,        // Middle priority
	DMA_PRIORITY_HIGH,          // High priority
	DMA_PRIORITY_SUPER_HIGH,    // Super high priority (Only DMA_CH_SIE_XX or DMA_CH_SIE2_XX are allowed)

	ENUM_DUMMY4WORD(DMA_PRIORITY)
} DMA_PRIORITY, *PDMA_PRIORITY;

/*
    DMA heavy loading channel number
*/
typedef enum {
	DMA_HEAVY_LOAD_CH0 = 0x0,                   // Heavy load channel 0
	DMA_HEAVY_LOAD_CH1,                         // Heavy load channel 1
	DMA_HEAVY_LOAD_CH2,                         // Heavy load channel 2
	DMA_HEAVY_LOAD_CH_CNT,
	ENUM_DUMMY4WORD(DMA_HEAVY_LOAD_CH)
} DMA_HEAVY_LOAD_CH;


/*
    DMA heavy loading test method
*/
typedef enum {
	DMA_HEAVY_LOAD_RW_BURST_INTERLEAVE = 0x0,   // Heavy load test method 0 => RW burst interleave
	DMA_HEAVY_LOAD_READ_AFTER_WRITE_ALL_BUF,    // Heavy load test method 1 => Read after write all buffer
	DMA_HEAVY_LOAD_READ_ONLY,                   // Heavy load test method 2 => Read only
	DMA_HEAVY_LOAD_WRITE_ONLY,                  // Heavy load test method 3 => Write only
	ENUM_DUMMY4WORD(DMA_HEAVY_LOAD_TEST_METHOD)
} DMA_HEAVY_LOAD_TEST_METHOD;

/*
    DMA heavy loading test pattern number
*/
typedef enum {
	DMA_HEAVY_LOAD_PATTERN0 = 0x0,              // Heavy load test pattern 0 => 1st test pattern (UINT32 number)
	DMA_HEAVY_LOAD_PATTERN1,                    // Heavy load test pattern 1 => 2nd test pattern (UINT32 number)
	DMA_HEAVY_LOAD_PATTERN2,                    // Heavy load test pattern 2 => 3rd test pattern (UINT32 number)
	DMA_HEAVY_LOAD_PATTERN3,                    // Heavy load test pattern 3 => 4th test pattern (UINT32 number)
	DMA_HEAVY_LOAD_PATTERN_CNT,
	ENUM_DUMMY4WORD(DMA_HEAVY_LOAD_TEST_PATTERN)
} DMA_HEAVY_LOAD_TEST_PATTERN;

/*
    Module for DMA utilization

    @note for dma_getModuleUtilization()
*/
typedef enum {
	DMA_UTILIZATION_MODULE_ALL,                 //< Get utilization for all module

	DMA_UTILIZATION_MODULE_MIPS1,               //< Get utilization for MIPS1 (not accurate, just for reference)
	DMA_UTILIZATION_MODULE_MIPS2,               //< Get utilization for MIPS2 (not accurate, just for reference)

	DMA_UTILIZATION_MODULE_DSP1,                //< Get utilization for DSP1 (not accurate, just for reference)
	DMA_UTILIZATION_MODULE_DSP2,                //< Get utilization for DSP2 (not accurate, just for reference)

	ENUM_DUMMY4WORD(DMA_UTILIZATION_MODULE)
} DMA_UTILIZATION_MODULE;

/*
    DMA monitor channel
*/
typedef enum {
	DMA_MONITOR_CH0,                            //< monitor channel 0
	DMA_MONITOR_CH1,                            //< monitor channel 1
	DMA_MONITOR_CH2,                            //< monitor channel 2
	DMA_MONITOR_CH3,                            //< monitor channel 3
	DMA_MONITOR_CH4,                            //< monitor channel 4
	DMA_MONITOR_CH5,                            //< monitor channel 5
	DMA_MONITOR_CH6,                            //< monitor channel 6
	DMA_MONITOR_CH7,                            //< monitor channel 7

	DMA_MONITOR_ALL,                            //< monitor channel of all DMA traffic, similar to dma_getBWUtilization()

	DMA_MONITOR_CH_COUNT,

	ENUM_DUMMY4WORD(DMA_MONITOR_CH)
} DMA_MONITOR_CH;

/*
    DMA configuration ID

    @note For dma_setConfig()
*/
typedef enum {
	DMA_CONFIG_ID_MONITOR_PERIOD,               ///< DMA monitor period (unit: ms)

	ENUM_DUMMY4WORD(DMA_CONFIG_ID)
} DMA_CONFIG_ID;

/*
    DMA call back list

    @note For dma_setCallBack()
*/
typedef enum {
	DMA_CALLBACK_MONITOR_UPDATE,            ///< DMA usage monitor update callback
	///< (SUGGESTED to be invoked in system init phase)


	ENUM_DUMMY4WORD(DMA_CALLBACK)
} DMA_CALLBACK;

/*
    DMA monitor type
*/
typedef enum {
	DMA_MONITOR_TYPE_SIZE,              // size monitor (unit: word)
	DMA_MONITOR_TYPE_REQ,               // requests monitor

	DMA_MONITOR_TYPE_COUNT,

	ENUM_DUMMY4WORD(DMA_MONITOR_TYPE)
} DMA_MONITOR_TYPE;

/*
    DMA QoS channel
*/
typedef enum {
	DMA_QOS_CH_CA9,                             //< QoS channel: CA9
	DMA_QOS_CH_GRPH = 4,                        //< QoS channel: GRPH
	DMA_QOS_CH_GRPH2,                           //< QoS channel: GRPH2
	DMA_QOS_CH_IP,                              //< QoS channel: ethernet and USB
	DMA_QOS_CH_SDIO,                            //< QoS channel: SDIO
	DMA_QOS_CH_SDIO2,                           //< QoS channel: SDIO2
	DMA_QOS_CH_SDIO3,                           //< QoS channel: SDIO3
	DMA_QOS_CH_CRYPTO,                          //< QoS channel: CRYPTO

	DMA_QOS_CH_COUNT,



	ENUM_DUMMY4WORD(DMA_QOS_CH)
} DMA_QOS_CH;

/*
    DMA OCP channel
*/
typedef enum {
	DMA_OCP_CH0,                                //< OCP channel 0 (CA9)
	DMA_OCP_CH1,                                //< OCP channel 0 (USB, ETH)

	DMA_OCP_CH_COUNT,

	ENUM_DUMMY4WORD(DMA_OCP_CH)
} DMA_OCP_CH;

#if defined(_NVT_EMULATION_)
/*
    DMA test configuration ID

    @note For MT
*/
typedef enum {
	DMATEST_CONFIG_ID_DQ0_PHASE,                ///< DQ[7..0]
	DMATEST_CONFIG_ID_DQ1_PHASE,                ///< DQ[15..8]
	DMATEST_CONFIG_ID_DQ2_PHASE,                ///< DQ[23..16]
	DMATEST_CONFIG_ID_DQ3_PHASE,                ///< DQ[31..24]

	DMATEST_CONFIG_ID_DQS0_PHASE,               ///< DQS0
	DMATEST_CONFIG_ID_DQS1_PHASE,               ///< DQS1
	DMATEST_CONFIG_ID_DQS2_PHASE,               ///< DQS2
	DMATEST_CONFIG_ID_DQS3_PHASE,               ///< DQS3

	DMATEST_CONFIG_ID_RDQS0_PHASE,              ///< DQS0 (read path)
	DMATEST_CONFIG_ID_RDQS1_PHASE,              ///< DQS1 (read path)
	DMATEST_CONFIG_ID_RDQS2_PHASE,              ///< DQS2 (read path)
	DMATEST_CONFIG_ID_RDQS3_PHASE,              ///< DQS3 (read path)

	DMATEST_CONFIG_ID_CLK_PHASE,                ///< clock
	DMATEST_CONFIG_ID_CMD_PHASE,                ///< CMD/ADDR

	DMATEST_CONFIG_ID_CNT,
	ENUM_DUMMY4WORD(DMATEST_CONFIG_ID)
} DMATEST_CONFIG_ID;
#endif

/*
    DMA test pattern
*/
typedef struct {
	UINT32                      uiPattern0;
	UINT32                      uiPattern1;
	UINT32                      uiPattern2;
	UINT32                      uiPattern3;
} DMA_TEST_PATTERN;

/*
    @name DMA heavy load configuration struct

    @note for dma_enableHeavyLoad()
*/
typedef struct {
	UINT32                      test_times;    // Heavy load test times, 0 is not available
	DMA_HEAVY_LOAD_TEST_METHOD  test_method;   // Heavy load test method, 0 is not available
	UINT32                      burst_len;     // Heavy load pattern burst length(0~127)
	UINT32                      start_addr;    // Heavy load start address(word alignment)
	UINT32                      dma_size;      // Heavy load test dma size(word alignment)
} DMA_HEAVY_LOAD_PARAM, *PDMA_HEAVY_LOAD_PARAM;

/*
    @name Heavy loading call back function

    @note for dma_waitHeavyLoadDonePolling()
*/
//typedef void (*DMA_HVYLoadCallbackFunc)(void);
typedef void (*dma_hvyload_callback_func)(void);

/*
    Translate DRAM address to physical address.

    Translate DRAM address to physical address.

    @param[in] addr     DRAM address

    @return physical DRAM address
*/
#define         dma_getPhyAddr(addr)            ((((UINT32)(addr))>=0x60000000UL)?((UINT32)(addr)-0x60000000UL):(UINT32)(addr))

/*
    Translate DRAM address to non-cacheable address.

    Translate DRAM address to non-cacheable address.

    @param[in] addr     DRAM address

    @return non-cacheable DRAM address
*/
#define         dma_getNonCacheAddr(addr)       ((((UINT32)(addr))<0x60000000UL)?((UINT32)(addr)+0x60000000UL):(UINT32)(addr))

/*
    Flush read (Device to CPU) cache with checking address is cacheable or not.

    In DMA operation, if buffer is cacheable, we have to flush read buffer before
    DMA operation to make sure CPU will read correct data.

    @param[in] uiAddr   DRAM address
    @param[in] uiSize   DRAM size
*/
#define dma_flushReadCache(uiAddr, uiSize) \
	{ \
		if (dma_isCacheAddr((uiAddr)) == TRUE) \
		{ \
			dma_flushReadCacheWithoutCheck((uiAddr), (uiSize)); \
		} \
	}

/*
    Flush read (Device to CPU) cache with checking address is cacheable or not.

    This API is required to be invoked after DMA complete.

    @param[in] uiAddr   DRAM address
    @param[in] uiSize   DRAM size
*/
#define dma_flushReadCacheDmaEnd(uiAddr, uiSize) \
	{ \
		if (dma_isCacheAddr((uiAddr)) == TRUE) \
		{ \
			dma_flushReadCacheDmaEndWithoutCheck((uiAddr), (uiSize)); \
		} \
	}


/*
    Flush write (CPU to Device) cache with checking address is cacheable or not.

    In DMA operation, if buffer is cacheable, we have to flush write buffer before
    DMA operation to make sure DMA will send correct data.

    @param[in] uiAddr   DRAM address
    @param[in] uiSize   DRAM size
*/
#define dma_flushWriteCache(uiAddr, uiSize) \
	{ \
		if (dma_isCacheAddr((uiAddr)) == TRUE) \
		{ \
			dma_flushWriteCacheWithoutCheck((uiAddr), (uiSize)); \
		} \
		else \
		{ \
			__asm__ __volatile__("dsb\n\t");\
		}\
		(*((volatile UINT32*)(0x60000000)));\
	}


/*
    Flush read (Device to CPU) cache where lineoffset not equal to width

    Once if case like these two condition as show below, need call
    dma_flushReadCacheWidthNEQLineOffset() instead of dma_flushReadCache()

    Example:
    @code
    {
        case(1): image lineoffset != width
         image width
         <-------------->
        start
         +--------------+----+
         |--------------|xxxx|
         |--------------|xxxx|
         |--------------|xxxx|
         |--------------|xxxx|
         +--------------+----+<--end
         <------------------->
         image line offset

         case(2):
            (a) CPU put data into _addr
            (b) Engine read from _addr and process
            (c) Engine write(output) data back to _addr
            (d) CPU read back from _addr
    }
    @endcode

    @param[in] uiAddr   DRAM address
    @param[in] uiSize   DRAM size
*/
#define dma_flushReadCacheWidthNEQLineOffset(uiAddr, uiSize) \
	{ \
		if (dma_isCacheAddr((uiAddr)) == TRUE) \
		{ \
			dma_flushReadCacheWidthNEQLineOffsetWithoutCheck((uiAddr), (uiSize)); \
		} \
	}

/*
    Open DMA driver

    Open DMA driver

    @return
        - @b E_OK   : success
        - @b E_SYS  : error
*/
extern ER       dma_open(void);

/*
    Close DMA driver

    Close DMA driver

    @return
        - @b E_OK   : success
        - @b E_SYS  : error
*/
extern ER       dma_close(void);

/*
    Set dma configuration

    @param[in] id               controller identifier
    @param[in] configID         configuration identifier
    @param[in] configContext    configuration context for configID

    @return
        - @b E_OK: set configuration success
        - @b E_NOSPT: configID is not supported
*/
extern ER dma_setConfig(DMA_ID id, DMA_CONFIG_ID configID, UINT32 configContext);

/*
    Set DMA call back routine

    This function provides a facility for upper layer to install callback routine.

    @param[in] id           controller identifier
    @param[in] callBackID   callback routine identifier
    @param[in] pCallBack    callback routine to be installed

    @return
        - @b E_OK: install callback success
        - @b E_NOSPT: callBackID is not valid
*/
extern ER dma_setCallBack(DMA_ID id, DMA_CALLBACK callBackID, DMA_GENERIC_CB pCallBack);

/*
    Set priority of DMA channel of specific DMA controller.

    Set DMA priority of specific channel.


    @param[in] id           DMA controller ID
    @param[in] Channel      DMA channel
    @param[in] Priority     DMA priority
        - @b DMA_PRIORITY_LOW       : Low priority (Default)
        - @b DMA_PRIORITY_MIDDLE    : Middle priority
        - @b DMA_PRIORITY_HIGH      : High priority
        - @b DMA_PRIORITY_SUPER_HIGH: Spuer High priority

    @return Set priority status
        - @b E_OK   : Set priority success
        - @b E_PAR  : Channel or priority is not correct
*/
extern ER dma_setCtrlPriority(DMA_ID id, DMA_CH Channel, DMA_PRIORITY Priority);

/*
    Get priority of DMA channel of specific DMA controller.

    Get DMA priority of specific channel.

    @param[in] id           DMA controller ID
    @param[in] Channel      DMA channel
    @param[out] pPriority   Returned DMA priority, could be one of the following
        - @b DMA_PRIORITY_LOW       : Low priority (Default)
        - @b DMA_PRIORITY_MIDDLE    : Middle priority
        - @b DMA_PRIORITY_HIGH      : High priority
        - @b DMA_PRIORITY_SUPER_HIGH: Spuer High priority

    @return Get priority status
        - @b E_OK   : Get priority success
        - @b E_PAR  : Channel is not correct or pPriority is NULL
*/
extern ER dma_getCtrlPriority(DMA_ID id, DMA_CH Channel, PDMA_PRIORITY pPriority);

/*
    Set priority of DMA channel.



    @param[in] Channel      DMA channel
    @param[in] Priority     DMA priority
        - @b DMA_PRIORITY_LOW       : Low priority (Default)
        - @b DMA_PRIORITY_MIDDLE    : Middle priority
        - @b DMA_PRIORITY_HIGH      : High priority
        - @b DMA_PRIORITY_SUPER_HIGH: Spuer High priority
    @return Set priority status
        - @b E_OK   : Set priority success
        - @b E_PAR  : Channel or priority is not correct
*/
extern ER       dma_setChannelPriority(DMA_CH Channel, DMA_PRIORITY Priority);

/*
    Get priority of DMA channel.

    Get DMA priority of specific channel.


    @param[in] Channel      DMA channel
    @param[out] pPriority   Returned DMA priority, could be one of the following
        - @b DMA_PRIORITY_LOW       : Low priority (Default)
        - @b DMA_PRIORITY_MIDDLE    : Middle priority
        - @b DMA_PRIORITY_HIGH      : High priority
        - @b DMA_PRIORITY_SUPER_HIGH: Spuer High priority

    @return Get priority status
        - @b E_OK   : Get priority success
        - @b E_PAR  : Channel is not correct or pPriority is NULL
*/
extern ER       dma_getChannelPriority(DMA_CH Channel, PDMA_PRIORITY pPriority);

/*
    Set system priority of DMA channel.

    Set priority of whole system


    @param[in] priorities   global priority setting

    @return Set priority status
        - @b E_OK   : Set priority success
        - @b E_PAR  : Channel or priority is not correct
*/
extern ER dma_set_system_priority(BOOL is_direct);
//extern ER dma_set_system_priority(PDMA_PRIO_MSK priorities);

extern ER dma_enableChannel(DMA_CH Channel);
extern ER dma_disableChannel(DMA_CH Channel);

/*
    Get system priority of DMA channel.

    Get priority of whole system


    @param[in] p_priorities  pointer to global priority setting

    @return Get priority status
        - @b E_OK   : Set priority success
        - @b E_PAR  : Channel or priority is not correct
*/
extern ER dma_get_system_priority(PDMA_PRIO_MSK p_priorities);

/*
    Get DRAM capacity

    Get DRAM capacity of DMA controller configuration

    @return DRAM capacity
*/
extern UINT32   dma_getDramCapacity(DMA_ID id);


/*
    Get DRAM utilization

    Get DRAM bandwidth utilization

    @return DRAM utilization X  %
*/
extern UINT32 dma_get_utilization(DDR_ARB id);
#define dma_getBWUtilization(id) dma_get_utilization(id)

/*
    Get DRAM efficiency

    Get DRAM ecfficiency

    @return DRAM ecfficiency X  %
*/
extern UINT32 dma_get_efficiency(DDR_ARB id);

/*
    Get Module DRAM utilization

    Get Module DRAM bandwidth utilization

    @return DRAM utilization X / 255 %
*/
extern UINT32   dma_getModuleUtilization(DMA_UTILIZATION_MODULE module);

/*
    Get DMA monitor information

    Get DMA monitor information (size, request counts)

    @param[in] id       DMA controller ID
    @param[in] ch       select monitor channel
    @param[in] type     select which info need to get
                        - @b DMA_MONITOR_TYPE_SIZE: get transfer size (unit: word) of this channel
                        - @b DMA_MONITOR_TYPE_REQ: get DMA request count

    @return selected information
*/
extern UINT32 dma_getMonitorInfo(DMA_ID id, DMA_MONITOR_CH ch, DMA_MONITOR_TYPE type);

/*
    Config DMA monitor

    Config DMA monitor

    @param[in] id       DMA controller ID
    @param[in] chMon    select monitor channel
    @param[in] chDma    select DMA channel to be monitored by chMon
    @param[in] direction    select which direction to be monitored
                        - @b DMA_DIRECTION_READ: DRAM -> Module
                        - @b DMA_DIRECTION_WRITE: Module <- DRAM
                        - @b DMA_DIRECTION_BOTH: both direction

    @return
        - @b E_OK: success
        - @b Else: input parameter not valid
*/
extern ER dma_configMonitor(DMA_ID id, DMA_MONITOR_CH chMon, DMA_CH chDma, DMA_DIRECTION direction);

/*
    Get DMA monitor config

    Config DMA monitor

    @param[in] id       DMA controller ID
    @param[in] chMon    select monitor channel
    @param[out] pChDma      return DMA channel to be monitored by chMon
    @param[out] pDirection  return which direction to be monitored
                        - @b DMA_DIRECTION_READ: DRAM -> Module
                        - @b DMA_DIRECTION_WRITE: Module <- DRAM
                        - @b DMA_DIRECTION_BOTH: both direction

    @return
        - @b E_OK: success
        - @b Else: input parameter not valid
*/
extern ER dma_getMonitorconfig(DMA_ID id, DMA_MONITOR_CH chMon, DMA_CH *pChDma, DMA_DIRECTION *pDirection);

/*
    Show illigal DMA channel request

    Show illigal DMA channel request

    @return void
*/
extern void     dma_showRequestChannel(void);

/*
    Clear DMA channel request and ack flag

    Clear DMA channel request and ack flag

    @return void
*/
extern void     dma_clearRequestChannel(void);

/*
    Get specific DMA channel name

    Get specific DMA channel name provided by DMA driver layer

    @param[in] uiCh     Specific DMA channel number

    @return string of DMA channel name
*/
extern const CHAR    *dma_getChannelName(DMA_CH uiCh);

/*
    Enable DMA write protect function.

    Enable DMA write protect function.

    @param[in] WpSet  Write protect function set

    @return void
*/
extern void     dma_enableWPFunc(DMA_WRITEPROT_SET WpSet);

/*
    Disable specific set of DMA write protect function.

    Disable specific set of DMA write protect function.

    @param[in] WpSet  Write protect function set

    @return void
*/
extern void     dma_disableWPFunc(DMA_WRITEPROT_SET WpSet);

/*
    Default API to show write protect channel detected

    Call dma_getWPStatus and send status into dma_showWPChannel, after this
    function called, write protect status will be cleared.

    @param[in] WpSet  Write protect function set
    @param[in] WpSts  Write protect function status

    @return void
*/
extern void     dma_show_wp_channel(DMA_WRITEPROT_SET wp_set, DMA_PROT_REGION region, DMA_WP_STS_TYPE wp_sts);
#define dma_showWPChannel(WpSet, WpSts) dma_show_wp_channel(WpSet, DMA_PROT_RGN0, WpSts)
/*
    get the  write protect status.

    get the  status which DMA channel has been write data to protecting address.

    @param[in] WpSet  Write protect function set
    @param[out] pWpSts          High word (bit32~63)status of write protect. Each bit indicate one DMA channel.(Refer to DMA_WRITEPROT_CH enum)
                                Bit32~ Bit34 is mapping to DMA channel DMA_WPCH_IFE0~DMA_WPCH_GRPH_OUTPUT.
                                low word status of write protect.user Each bit indicate one DMA channel.(Refer to DMA_WRITEPROT_CH enum)
                                (Bit2~ Bit31) is mapping to DMA channel DMA_WPCH_SIE0~DMA_WPCH_DAI1.
                                Combination to UINT64
    @return void
*/
extern void     dma_get_wp_status(DMA_WRITEPROT_SET wp_set, DMA_PROT_REGION region, DMA_WP_STS_TYPE *p_wp_sts);
#define dma_getWPStatus(WpSet, pWpSts) dma_get_wp_status(WpSet, DMA_PROT_RGN0, pWpSts)
/*
    Clear the  status of specified DMA channel.

    Clear the  status of specified DMA channel.

    @param[in] WpSet        Write protect function set
    @param[in] chkChannel   To be clear status of the specified DMA channel.

    @return void
*/
extern void     dma_clr_wp_status(DMA_WRITEPROT_SET wp_set, DMA_PROT_REGION region, DMA_WRITEPROT_CH chk_channel);
#define dma_clrWPStatus(WpSet, chkChannel) dma_clr_wp_status(WpSet, DMA_PROT_RGN0, chkChannel)
/*
    Check the  specified DMA channel whether write data to protecting address or not

    Check the  specified DMA channel whether write data to protecting address or not

    @param[in] WpSet        Write protect function set
    @param[in] chkChannel   To be check DMA channel

    @return BOOL
        - @b TRUE:  Checked DMA chanel has written data to protecting area.
        - @b FALSE: Checked DMA chanel isn't writting data to protecting area yet.
*/
extern BOOL     dma_chk_dma_wr2_protect_addr(DMA_WRITEPROT_SET wp_set, DMA_PROT_REGION region, DMA_WRITEPROT_CH chk_channel);
#define dma_chkDmaWR2ProtectAddr(WpSet, chkChannel) dma_chk_dma_wr2_protect_addr(WpSet, DMA_PROT_RGN0, chkChannel)
/*
    Config DMA write protection function.

    This function is used to config DMA write protection function.
    The using right of write protect function must have been gotten before calling this function,

    @param[in] WpSet        Write protect function set
    @param[in] PprotectAttr Configuration for write protect function
    @param[in] pDrvcb       callback handler for write protect function

    @return void
*/
//extern void     dma_configWPFunc(DMA_WRITEPROT_SET WpSet, PDMA_WRITEPROT_ATTR PprotectAttr, DRV_CB pDrvcb);
//extern void     dma_config_wp_func(DMA_WRITEPROT_SET wp_set, PDMA_PROT_ATTR p_protect_attr, DRV_CB p_drv_cb);
/*
    Get write protect channel No.

    Get write protect channel number

    @param[in] WpSts    Point of dma write protect channel number array

    @return WP channel number

*/
extern UINT32   dma_getWPStsChannel(DMA_WP_STS_TYPE WpSts);

extern void dma_trig_heavyload(DDR_ARB id, UINT32 channel);
/*
    Polling status of heavy load

    Polling status bit of heavy load controller, done or error occurred

    @note : If more than 1 channel, done status will assert until all channel operation done
            and error status will occurred once if any channel encounter error

    @param[in] id               DMA controller ID
    @param[in] pCallBackHdl     Heavy Loading callback function during polling heavy load done,
                                Upper layer can display utilization there, if NULL, driver will
                                display utilization automatically
    @return BOOL
        - @b TRUE: Test success
        - @b TRUE: Test error occurred
*/
extern BOOL     dma_wait_heavyload_done_polling(DDR_ARB id, dma_hvyload_callback_func call_back_hdl);
#define     dma_waitHeavyLoadDonePolling(id, call_back_hdl) dma_wait_heavyload_done_polling(id, call_back_hdl)

/*
    Enable heavy loading operation

    Start heavy loading operation in specific channel

    @param[in] id           DMA controller ID
    @param[in] uiCh         3 channel are available for configure
    @param[in] hvyParam     configuration information

    @return ER
        - @b E_OK: Enable heavy load success
        - @b E_PAR:Param error
*/
extern ER       dma_enable_heavyload(DDR_ARB id, DMA_HEAVY_LOAD_CH channel, PDMA_HEAVY_LOAD_PARAM hvy_param);
#define       dma_enableHeavyLoad(id, channel, hvy_param) dma_enable_heavyload(id, channel, hvy_param)

/*
    Set heavy loading test pattern

    Set heavy loading test pattern, total 4 sets

    @param[in] id           DMA controller ID
    @param[in] uiPattern0   Pattern0
    @param[in] uiPattern1   Pattern1
    @param[in] uiPattern2   Pattern2
    @param[in] uiPattern3   Pattern3
*/
extern void     dma_setHeavyLoadPattern(DMA_ID id, UINT32 uiPattern0, UINT32 uiPattern1, UINT32 uiPattern2, UINT32 uiPattern3);

/*
    Get heavy load checksum

    Enable heavy load to calculate 16 bit checksum and get result

    @param[in] id           DMA controller ID
    @param[in] uiCh         3 channel are available for configure
    @param[in] uiBurstLen   0 ~ 127
    @param[in] uiAddr       DRAM buffer address (should word align)
    @param[in] uiLength     buffer length (should word align)

    @return checksum
*/
extern UINT32   dma_getHeavyLoadChkSum(DMA_ID id, DMA_HEAVY_LOAD_CH uiCh, UINT32 uiBurstLen, UINT32 uiAddr, UINT32 uiLength);

/*
    Dump DDR phy information.

    Dump DDR phy information.

    @return void
*/
extern void     dma_dumpPhyInfo(void);

/*
    Enable / Disable precharge power down

    @param[in] id       select DMA controller
    @param[in] flag     enable precharge power down
        - @b TRUE:  enable precharge power down
        - @b FALSE: disable precharge power down
    @return void
*/
extern void dma_setPrechargePowerDown(DMA_ID id, BOOL flag);

/*

    Flush read (Device to CPU) cache without checking buffer is cacheable or not


    In DMA operation, if buffer is cacheable, we have to flush read buffer before
    DMA operation to make sure CPU will read correct data.



    @param[in] uiStartAddr  Buffer starting address
    @param[in] uiLength     Buffer length
    @return Use clean and invalidate data cache all or not (cpu_cleanInvalidateDCacheAll)
        - @b TRUE:   Use cpu_cleanInvalidateDCacheAll
        - @b FALSE:  Not use cpu_cleanInvalidateDCacheAll
*/
extern BOOL             dma_flushReadCacheWithoutCheck(UINT32 uiStartAddr, UINT32 uiLength);

/*

    Flush read (Device to CPU) cache without checking buffer is cacheable or not


    In DMA operation, if buffer is cacheable, we also have to flush read buffer after
    DMA operation to make sure CPU will read correct data.



    @param[in] uiStartAddr  Buffer starting address
    @param[in] uiLength     Buffer length
    @return void
*/
extern void 			dma_flushReadCacheDmaEndWithoutCheck(UINT32 uiStartAddr, UINT32 uiLength);


/*
    Flush write (CPU to Device) cache without checking buffer is cacheable or not

    In DMA operation, if buffer is cacheable, we have to flush write buffer before
    DMA operation to make sure DMA will send correct data.

    @note   Depend on performance measurement
        clean data cache 016K = 00015 us
        clean data cache 032K = 00020 us
        clean data cache 064K = 00031 us
        clean data cache 128K = 00056 us
        clean data cache 256K = 00102 us
        clean data cache 512K = 00198 us

        fatch 16K data cache  = 00024 us

        So, once if (uiEndAddr - uiStartAddr) > 32K
        calling cpu_cleanInvalidateDCacheAll

    @param[in] uiStartAddr  Buffer starting address
    @param[in] uiLength     Buffer length
    @return Use clean and invalidate data cache all or not (cpu_cleanInvalidateDCacheAll)
        - @b TRUE:   Use cpu_cleanInvalidateDCacheAll
        - @b FALSE:  Not use cpu_cleanInvalidateDCacheAll
*/
extern BOOL             dma_flushWriteCacheWithoutCheck(UINT32 uiStartAddr, UINT32 uiLength);

/*
    Flush read (Device to CPU) cache where lineoffset not equal to width
    Flush read (Device to CPU) cache where lineoffset not equal to width

    @param[in] uiStartAddr  Buffer starting address
    @param[in] uiLength     Buffer length
    @return Use clean and invalidate data cache all or not (cpu_cleanInvalidateDCacheAll)
        - @b TRUE:   Use cpu_cleanInvalidateDCacheAll
        - @b FALSE:  Not use cpu_cleanInvalidateDCacheAll

*/
extern BOOL             dma_flushReadCacheWidthNEQLineOffsetWithoutCheck(UINT32 uiStartAddr, UINT32 uiLength);

/*
    Config QOS setting

    DMA channels that support QOS can temporarily raise priority from low to
    high if their DMA requests are not serviced within specific ARB cycles

    @param[in] channel      QOS channel
    @param[in] enable       enable/disable QOS function for this DMA channel
                            - TRUE: enable QOS
                            - FALSE: disable QOS
    @param[in] timeout      how many ARB clock cycle to raise channel priority (unit: ARB clock)

    @return
        - @b E_OK: success
        - @b E_NOSPT: input parameter not valid
*/
extern ER               dma_config_qos(DMA_QOS_CH channel, BOOL enable, UINT32 timeout);

/*
    Set OCP request count.

    set OCP request count for each processor cycle.

	@param[in] id       controller ID
    @param[in] channel  OCP channel
    @param[in] count    request count for each cycle
                        - @b 1: 1 request
                        - @b 2: 2 requests
                        - @b 3: 3 requests
                        - @b 4: 4 requests (max value for DMA_OCP_CH0/DMA_OCP_CH2/DMA_OCP_CH3)
                        - @b 5: 5 request
                        - @b 6: 6 requests
                        - @b 7: 7 requests
                        - @b 8: 8 requests (max value for DMA_OCP_CH1)
                        - @b others: invalid

    @return
        - @b E_ID: channel is not valid
        - @b E_OK: success
*/
extern ER               dma_setOCPRequestCount(DMA_ID id, DMA_OCP_CH channel, UINT32 count);

/*
    Set channel outstanding.

    Set channel outstanding


    @param[in] outstanding channel

    @return Set outstanding
        - @b E_OK   : Set outstanding success
        - @b E_PAR  : Channel is not correct
*/
//extern ER dma_set_channel_outstanding(DMA_CH_OUTSTANDING channel, BOOL enable);

/*
    Get channel outstanding.

    Get channel outstanding


    @param[in] outstanding channel

    @return Get outstanding
        - @b E_OK   : Set outstanding success
        - @b E_PAR  : Channel is not correct
        - @b E_OBJ  : DMA outstanding in DMA_ID_1 and DMA_ID_2 is not the same
*/
//extern ER dma_get_channel_outstanding(DMA_CH_OUTSTANDING channel, BOOL *enable);

extern ER       dma_enableChannel(DMA_CH Channel);
extern ER       dma_disableChannel(DMA_CH Channel);


#if defined(_NVT_EMULATION_)
/*
    Get dma test configuration

    @param[in] id               controller ID
    @param[in] configID         Configuration identifier
    @param[in] pConfigContext   returning context for configID

    @return
        - @b E_OK: set configuration success
        - @b E_NOSPT: configID is not supported
*/
extern ER dmaTest_getConfig(DMA_ID id, DMATEST_CONFIG_ID configID, UINT32 *pConfigContext);

/*
    Set dma test configuration

    @param[in] id               controller ID
    @param[in] configID         Configuration identifier
    @param[in] configContext    context for configID

    @return
        - @b E_OK: set configuration success
        - @b E_NOSPT: configID is not supported
*/
extern ER dmaTest_setConfig(DMA_ID id, DMATEST_CONFIG_ID configID, UINT32 configContext);
#endif

//@}

#endif
