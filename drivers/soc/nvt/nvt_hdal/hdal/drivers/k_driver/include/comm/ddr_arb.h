/**
    Public header file for arbiter module.

    This file is the header file that define the API and data type for arbiter
    module.

    @file       ddr_arb.h
    @ingroup    miDrvComm_Arb
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _DRAM_ARB_H
#define _DRAM_ARB_H

#if defined __UITRON || defined __ECOS
#include "Driver.h"
#else
#include "comm/driver.h"
#endif


/**
    @addtogroup miDrvComm_Arb
*/
//@{

/*
    DMA channel mask

    Indicate which DMA channels are required to protect/detect

    @note For DMA_WRITEPROT_ATTR
*/
typedef struct _DMA_CH_MSK {
	// ch 0
	UINT32 reserved0: 1;                       //< bit0: reserved (auto refresh)
	UINT32 CPU_NS: 1;                          //< CPU_NS
	UINT32 IP_USB_ETH: 1;                             //< USB_0
	UINT32 DCE_0: 1;                           //< DCE_0 (in)
	UINT32 DCE_1: 1;                           //< DCE_1 (in)
	UINT32 DCE_2: 1;                           //< DCE_2 (out)
	UINT32 DCE_3: 1;                           //< DCE_3 (out)
	UINT32 DCE_4: 1;                           //< DCE_4 (out)
	UINT32 DCE_5: 1;                           //< DCE_5 (in)
	UINT32 DCE_6: 1;                           //< DCE_6 (out)
	UINT32 DCE_7: 1;                           //< DCE_6 (out)
	UINT32 GRA_0: 1;                           //< GRA_0 (in)
	UINT32 GRA_1: 1;                           //< GRA_1 (in)
	UINT32 GRA_2: 1;                           //< GRA_2 (in)
	UINT32 GRA_3: 1;                           //< GRA_3 (out)
	UINT32 GRA_4: 1;                           //< GRA_4 (out)
	// ch 16
	UINT32 GRA2_0: 1;                          //< GRA2_0 (in)
	UINT32 GRA2_1: 1;                          //< GRA2_1 (in)
	UINT32 GRA2_2: 1;                          //< GRA2_2 (out)
	UINT32 JPG_0: 1;                           //< JPG IMG
	UINT32 JPG_1: 1;                           //< JPG BS
	UINT32 JPG_2: 1;                           //< JPG Enc DC out
	UINT32 JPG_3: 1;                           //< JPG Enc DC out
	UINT32 IPE_0: 1;                           //< IPE_0 (in)
	UINT32 IPE_1: 1;                           //< IPE_1 (in)
	UINT32 IPE_2: 1;                           //< IPE_2 (out)
	UINT32 IPE_3: 1;                           //< IPE_3 (out)
	UINT32 IPE_4: 1;                           //< IPE_4 (out)
	UINT32 IPE_5: 1;                           //< IPE_5 (out)
	UINT32 IPE_6: 1;                           //< IPE_6 (in)
	UINT32 SIE_0: 1;                           //< SIE_0 (out)
	UINT32 SIE_1: 1;                           //< SIE_1 (out)
	// ch 32
	UINT32 SIE_2: 1;                           //< SIE_2 (in)
	UINT32 SIE_3: 1;                           //< SIE_3 (in)
	UINT32 SIE2_0: 1;                          //< SIE2_0 (out)
	UINT32 SIE2_1: 1;                          //< SIE2_1 (out)
	UINT32 SIE2_2: 1;                          //< SIE2_2 (in)
	UINT32 SIE2_3: 1;                          //< SIE2_3 (in)
	UINT32 SIE3_0: 1;                          //< SIE3_0 (out)
	UINT32 SIE3_1: 1;                          //< SIE3_1 (out)
	UINT32 bDIS_0: 1;                           //< DIS_0 (in/out)
	UINT32 DIS_1: 1;                           //< DIS_1 (in)
	UINT32 LARB: 1;                            //< Local arbiter (SIF/BMC/I2C/UART/SPI)
	UINT32 DAI: 1;                             //< DAI
	UINT32 IFE_0: 1;                           //< IFE_0 (in)
	UINT32 IFE_1: 1;                           //< IFE_1 (in)
	UINT32 IFE_2: 1;                           //< IFE_2 (out)
	UINT32 IME_0: 1;                           //< IME_0 (in)
	// ch 48
	UINT32 IME_1: 1;                           //< IME_1 (in)
	UINT32 IME_2: 1;                           //< IME_2 (in)
	UINT32 IME_3: 1;                           //< IME_3 (out)
	UINT32 IME_4: 1;                           //< IME_4 (out)
	UINT32 IME_5: 1;                           //< IME_5 (out)
	UINT32 IME_6: 1;                           //< IME_6 (out)
	UINT32 IME_7: 1;                           //< IME_7 (out)
	UINT32 IME_8: 1;                           //< IME_8 (out)
	UINT32 IME_9: 1;                           //< IME_9 (out)
	UINT32 IME_A: 1;                           //< IME_A (out)
	UINT32 IME_B: 1;                           //< IME_B (out)
	UINT32 IME_C: 1;                           //< IME_C (in)
	UINT32 IME_D: 1;                           //< IME_D (out)
	UINT32 IME_E: 1;                           //< IME_E (in)
	UINT32 IME_F: 1;                           //< IME_F (in)
	UINT32 IME_10: 1;                          //< IME_10 (in)
	// ch 64
	UINT32 IME_11: 1;                          //< IME_11 (in)
	UINT32 IME_12: 1;                          //< IME_12 (out)
	UINT32 IME_13: 1;                          //< IME_13 (out)
	UINT32 IME_14: 1;                          //< IME_14 (in)
	UINT32 IME_15: 1;                          //< IME_15 (out)
	UINT32 IME_16: 1;                          //< IME_16 (in/out)
	UINT32 IME_17: 1;                          //< IME_17 (out)
	UINT32 ISE_0: 1;                           //< ISE_0 (in)
	UINT32 ISE_1: 1;                           //< ISE_1 (out)
	UINT32 ISE_2: 1;
	UINT32 IDE_0: 1;                           //< IDE_0 (in)
	UINT32 IDE_1: 1;                           //< IDE_1 (in)
	UINT32 IDE_2: 1;                           //< IDE_2 (in/out)
	UINT32 IDE_3: 1;                           //< IDE_3 (in/out)
	UINT32 IDE_4: 1;                           //< IDE_4 (in)
	UINT32 IDE_5: 1;                           //< IDE_5 (in)
	// ch 80
	UINT32 SDIO: 1;                            //< SDIO
	UINT32 SDIO2: 1;                           //< SDIO2
	UINT32 SDIO3: 1;                           //< SDIO3
	UINT32 NAND: 1;                            //< NAND
	UINT32 H264_0: 1;                          //< H.264_0
	UINT32 H264_1: 1;                          //< H.264_1
	UINT32 H264_3: 1;                          //< H.264_3
	UINT32 H264_4: 1;                          //< H.264_4
	UINT32 H264_5: 1;                          //< H.264_5
	UINT32 H264_6: 1;                          //< H.264_6
	UINT32 H264_8: 1;                          //< H.264_8
	UINT32 H264_9: 1;                          //< H.264_9 (COE)
	UINT32 IFE2_0: 1;                          //< IFE2_0 (in)
	UINT32 IFE2_1: 1;                          //< IFE2_1 (out)
	UINT32 TSE: 1;                             //< TSE input
	UINT32 TSE_1: 1;                           //< TSE output
	// ch 96
	UINT32 CRYPTO: 1;                          //< CRYPTO (in/out)
	UINT32 HASH: 1;                            //< Hash (in/out)
	UINT32 CNN_0: 1;                           //< CNN_0 (in)
	UINT32 CNN_1: 1;                           //< CNN_1 (in)
	UINT32 CNN_2: 1;                           //< CNN_2 (in)
	UINT32 CNN_3: 1;                           //< CNN_3 (in)
	UINT32 CNN_4: 1;                           //< CNN_4 (out)
	UINT32 CNN_5: 1;                           //< CNN_5 (out)
	UINT32 CNN_6: 1;                           //< CNN_5 (out)
	UINT32 NUE2_0: 1;                          //< NUE2_0 (in)
	UINT32 NUE2_1: 1;                          //< NUE2_1 (in)
	UINT32 NUE2_2: 1;                          //< NUE2_2 (in)
	UINT32 NUE2_3: 1;                          //< NUE2_3 (out)
	UINT32 NUE2_4: 1;							//< NUE2_4 (out)
	UINT32 NUE2_5: 1;							//< NUE2_5 (out)
	UINT32 NUE2_6: 1;							//< NUE2_5 (out)
	// ch 112
	UINT32 MDBC_0: 1;                          //< MDBC_0 (in)
	UINT32 MDBC_1: 1;							//< MDBC_1 (in)
	UINT32 MDBC_2: 1;							//< MDBC_2 (in)
	UINT32 MDBC_3: 1;							//< MDBC_3 (in)
	UINT32 MDBC_4: 1;							//< MDBC_4 (in)
	UINT32 MDBC_5: 1;							//< MDBC_5 (in)
	UINT32 MDBC_6: 1;							//< MDBC_6 (out)
	UINT32 MDBC_7: 1;							//< MDBC_7 (out)
	UINT32 MDBC_8: 1;							//< MDBC_8 (in)
	UINT32 MDBC_9: 1;							//< MDBC_9 (out)
	UINT32 HLOAD_0: 1;                         //< Heavy load
	UINT32 HLOAD_1: 1;							//< Heavy load
	UINT32 HLOAD_2: 1;							//< Heavy load
	UINT32 AFN_0: 1;
	UINT32 AFN_1: 1;
	UINT32 IVE_0: 1;
	// ch 128
	UINT32 IVE_1: 1;
	UINT32 UVC_0: 1;
	UINT32 UVC_1: 1;
	UINT32 reserved1: 29;
	// ch 160
	UINT32 CPU: 1;
	UINT32 reserved2: 31;
} DMA_CH_MSK, *PDMA_CH_MSK;

/**
    DDR Arbiter ID

*/
typedef enum _DDR_ARB {
	DDR_ARB_1,                           ///< DDR Arbiter

	DDR_ARB_COUNT,                       //< Arbiter count

	ENUM_DUMMY4WORD(DDR_ARB)
} DDR_ARB;

typedef enum _DMA_WRITEPROT_SET {
	WPSET_0,            // Write protect function set 0
	WPSET_1,            // Write protect function set 1
	WPSET_2,            // Write protect function set 2
	WPSET_3,            // Write protect function set 3
	WPSET_4,            // Write protect function set 4
	WPSET_5,            // Write protect function set 5
	WPSET_COUNT,
	ENUM_DUMMY4WORD(DMA_WRITEPROT_SET)
} DMA_WRITEPROT_SET;

typedef enum _DMA_PROT_REGION {
	DMA_PROT_RGN0,
	DMA_PROT_RGN1,
	DMA_PROT_RGN2,
	DMA_PROT_RGN3,
	DMA_PROT_RGN_TOTAL,
	ENUM_DUMMY4WORD(DMA_PROT_REGION)
} DMA_PROT_REGION;

typedef enum _DMA_WRITEPROT_LEVEL {
	DMA_WPLEL_UNWRITE,      // Not only detect write action but also denial access.
	DMA_WPLEL_DETECT,       // Only detect write action but allow write access.
	DMA_RPLEL_UNREAD,       // Not only detect read action but also denial access.
	DMA_RWPLEL_UNRW,        // Not only detect read write action but also denial access.
	ENUM_DUMMY4WORD(DMA_WRITEPROT_LEVEL)
} DMA_WRITEPROT_LEVEL;

typedef enum _DMA_PROT_MODE {
	DMA_PROT_IN,
	DMA_PROT_OUT,
	ENUM_DUMMY4WORD(DMA_PROT_MODE)
} DMA_PROT_MODE;

typedef struct _DMA_PROT_RGN_ATTR {
	BOOL                en;            // enable this region
	UINT32              starting_addr; // DDR3:must be 4 words alignment
	UINT32              size;          // DDR3:must be 4 words alignment
} DMA_PROT_RGN_ATTR, *PDMA_PROT_RGN_ATTR;

typedef struct _DMA_WRITEPROT_ATTR {
	DMA_CH_MSK          mask;       // DMA channel masks to be protected/detected
	DMA_WRITEPROT_LEVEL level;	    // protect level
	DMA_PROT_MODE       protect_mode; // in or out region
	DMA_PROT_RGN_ATTR   protect_rgn_attr[DMA_PROT_RGN_TOTAL];
} DMA_WRITEPROT_ATTR, *PDMA_WRITEPROT_ATTR;

/*
    @name DMA outstanding

    DMA outstanding setting

    @note Used in dma_set_channel_outstanding()
*/
typedef enum {
	DMA_CH_OUTSTANDING_CNN_0,           // CNN input
	DMA_CH_OUTSTANDING_CNN_1,           // CNN input
	DMA_CH_OUTSTANDING_CNN_2,           // CNN input
	DMA_CH_OUTSTANDING_CNN_3,           // CNN input
	DMA_CH_OUTSTANDING_CNN_4,           // CNN output
	DMA_CH_OUTSTANDING_CNN_5,           // CNN output

	DMA_CH_OUTSTANDING_CNN2_0,
	DMA_CH_OUTSTANDING_CNN2_1,
	DMA_CH_OUTSTANDING_CNN2_2,
	DMA_CH_OUTSTANDING_CNN2_3,
	DMA_CH_OUTSTANDING_CNN2_4,
	DMA_CH_OUTSTANDING_CNN2_5,

	DMA_CH_OUTSTANDING_NUE_0,           // NUE input
	DMA_CH_OUTSTANDING_NUE_1,           // NUE input
	DMA_CH_OUTSTANDING_NUE_2,           // NUE output

	DMA_CH_OUTSTANDING_CNN_ALL,
	DMA_CH_OUTSTANDING_CNN2_ALL,
	DMA_CH_OUTSTANDING_NUE_ALL,
	ENUM_DUMMY4WORD(DMA_CH_OUTSTANDING)
} DMA_CH_OUTSTANDING;

#define DRAM_CONSUMETSK_CHANNEL_SET_OPERATION_BIT(ch, chGroup)       ((chGroup).channel_group[((ch)>>5)] |= (1<<(ch&0x1F)))

typedef enum _DRAM_CONSUME_BW_DEGREE {
	DRAM_CONSUME_EASY_LOADING = 0x0,
	DRAM_CONSUME_NORMAL_LOADING,
	DRAM_CONSUME_HEAVY_LOADING,

	DRAM_CONSUME_CH_DISABLE,

	ENUM_DUMMY4WORD(DRAM_CONSUME_BW_DEGREE)
} DRAM_CONSUME_BW_DEGREE;

typedef struct {
	UINT32  channel_group[6];   // Description
} DMA_CHANNEL, *PDMA_CHANNEL;


typedef struct _DRAM_CONSUME_ATTR {
	DRAM_CONSUME_BW_DEGREE  load_degree;
	UINT32                  addr;
	UINT32                  size;
	DMA_CHANNEL             dma_channel;
	BOOL					is_start;
} DRAM_CONSUME_ATTR, *PDRAM_CONSUME_ATTR;

// Arbiter Driver API
extern void arb_enable_wp(DDR_ARB id, DMA_WRITEPROT_SET set,
                DMA_WRITEPROT_ATTR *p_attr);
extern void arb_disable_wp(DDR_ARB id, DMA_WRITEPROT_SET set);
extern ER arb_init(void);
extern void arb_set_priority(BOOL is_direct);
extern ER dma_get_channel_outstanding(DMA_CH_OUTSTANDING channel, BOOL *enable);
extern ER dma_set_channel_outstanding(DMA_CH_OUTSTANDING channel, BOOL enable);

extern int dram_consume_cfg(PDRAM_CONSUME_ATTR attr);
extern int dram_consume_start(void);
extern int dram_consume_stop(void);
extern int dram2_consume_cfg(PDRAM_CONSUME_ATTR attr);
extern int dram2_consume_start(void);
extern int dram2_consume_stop(void);

extern int ddr_data_monitor_start(DDR_ARB id);
extern int ddr_data_monitor_stop(DDR_ARB id);
extern void ddr_data_monitor_reset(DDR_ARB id);
extern void ddr_data_monitor_get(DDR_ARB id, UINT64 *cnt, UINT64 *byte);







/* @this function is used to perform hardware checksum
 * @parm: id:
 *      memory address on which ddr
 * @parm: phy_addr:
 *		memory physical address (must be word alignment)
 * @parm: len:
 * 		memory length (must be word alignment)
 * @return:
 *		checksum value
 *
 */
extern unsigned short arb_chksum(DDR_ARB id, unsigned int phy_addr, unsigned int len);
//@}


//ch: 0: CPU, 1: CNN, 2: CNN2, 3: NUE, 4: NUE2, 5: ISE
//rw: 0: write, 1: read, 2: both
//dram: 0: DRAM1, 1: DRAM2
extern int mau_ch_mon_start(int ch, int rw, int dram);
extern UINT64 mau_ch_mon_stop(int ch, int dram);

#endif
