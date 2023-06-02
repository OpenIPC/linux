/*
    Novatek protected header file of NT96650's driver.

    The header file for Novatek protected APIs of NT96650's driver.

    @file       nand__protected.h
    @ingroup    mIDriver
    @note       For Novatek internal reference, don't export to agent or customer.

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#ifndef _NVT_NAND_PROTECTED_H
#define _NVT_NAND_PROTECTED_H

#include <kwrap/nvt_type.h>


// -----------------------------------------------------------------------------
// NAND driver support table list version code organization
// -----------------------------------------------------------------------------
#define     NAND_TBL_VERSION_MM_SFT             2
#define     NAND_TBL_VERSION_DD_SFT             6
#define     NAND_TBL_VERSION_CODE_SFT           11
#define     NAND_TBL_VERSION_TOTAL_SFT          16

#define     PARSE_NAND_TBL_VER_YY(m)            (2013+(m>>(NAND_TBL_VERSION_TOTAL_SFT)&0x3))                        //2bit
#define     PARSE_NAND_TBL_VER_MM(m)            ((m>>(NAND_TBL_VERSION_TOTAL_SFT+NAND_TBL_VERSION_MM_SFT))&0xF)     //4bit
#define     PARSE_NAND_TBL_VER_DD(m)            ((m>>(NAND_TBL_VERSION_TOTAL_SFT+NAND_TBL_VERSION_DD_SFT))&0x1F)    //5bit
#define     PARSE_NAND_TBL_VER_CODE(m)          ((m>>(NAND_TBL_VERSION_TOTAL_SFT+NAND_TBL_VERSION_CODE_SFT))&0x1F)  //5bit

#define     NAND_TBL_FLAG_SPI_FLASH             0x46495053     //'S''P''I''F' : represent internal flash is SPI
#define     NAND_TBL_FLAG_NAND_ID_MATCHED       (1)            // Table found
#define     NAND_TBL_FLAG_NAND_ID_NOT_MATCHED   (2)            // Table not found


/*
    Primary NAND ECC algorithm selection

    @note for SMHostECCEnable()
*/
typedef enum {
	NAND_PRIMARY_ECC_OFF = 0x0,                     //<ECC OFF
	NAND_PRIMARY_ECC_HAMMING,                       //<Hamming Code ECC
	NAND_PRIMARY_ECC_REEDSOLOMON,                   //<ReedSolomon  ECC
	NAND_PRIMARY_ECC_SPINAND_ONDIE_ECC,             //<Only available in SPI NAND
	NAND_PRIMARY_ECC_CODE_NUM,
	ENUM_DUMMY4WORD(NAND_PRIMARY_ECC_CODE)
} NAND_PRIMARY_ECC_CODE;

/*
    Secondary NAND ECC method selection

    @note for SMHostECCEnable()
*/
typedef enum {
	NAND_SEC_ECC_OFF = 0x0,                         //<ECC OFF
	NAND_SEC_ECC_METHOD1,                           //<For small page
	NAND_SEC_ECC_METHOD2,                           //<For large page
	NAND_SEC_ECC_NUM,
	ENUM_DUMMY4WORD(NAND_SEC_ECC_METHOD)
} NAND_SEC_ECC_METHOD;

typedef enum {
	NAND_EVA_VERSION,       //B07F0000
	NAND_EVA_INFO0,         //B07F0004
	//  bit[15..00] : Maker
	//  bit[31..16] : clock
	//  0x0: 48
	//  0x1: 60
	//  0x2: 96

	NAND_EVA_INFO1,         //B07F0008
	//  bit[15..00] : Page size
	//  bit[31..15] : Page per Block
	NAND_EVA_INFO2,         //B07F000C      : Capacity (Unit:MB)
	NAND_EVA_INFO3,         //B07F0010
	// Timing0
	NAND_EVA_INFO4,         //B07F0014
	// Timing1
	NAND_EVA_INFO5,         //B07F0018      : Block size
	NAND_EVA_INFO6,         //B07F001C
	// Driving0
	//  data0
	//  data1
	//  data2
	//  data3
	//  data4
	//  data5
	//  data6
	//  data7
	// Driving1
	//  cs0
	//  cs1
	//  we
	//  re
	//  cle
	//  ale
	//  wp
	//  rdy
	NAND_EVA_ERROR_TAG,     //B07F0020
	NAND_EVA_SUCCESS_TAG,   //B07F0024
	NAND_EVA_FW0_START,     //B07F0028
	NAND_EVA_FW0_END,       //B07F002C
	NAND_EVA_FW1_START,     //B07F0030
	NAND_EVA_FW1_END,       //B07F0034
	NAND_EVA_FW2_START,     //B07F0038
	NAND_EVA_FW2_END,       //B07F003C
	NAND_EVA_FAT0_START,    //B07F0040 bit[31..28] low bytes of bad block tolerance
	NAND_EVA_FAT0_END,      //B07F0044 bit[31..28] high bytes of bad block tolerance
	NAND_EVA_FAT1_START,    //B07F0048
	NAND_EVA_FAT1_END,      //B07F004C
	NAND_EVA_FAT2_START,    //B07F0050
	NAND_EVA_FAT2_END,      //B07F0054
	NAND_EVA_PS0_START,     //B07F0058
	NAND_EVA_PS0_END,       //B07F005C
	NAND_EVA_PS1_START,     //B07F0060
	NAND_EVA_PS1_END,       //B07F0064
	NAND_EVA_PS2_START,     //B07F0068
	NAND_EVA_PS2_END,       //B07F006C
	NAND_EVA_PS3_START,     //B07F0070
	NAND_EVA_PS3_END,       //B07F0074
	NAND_EVA_PS4_START,     //B07F0078
	NAND_EVA_PS4_END,       //B07F007C
	NAND_EVA_MAX_BUF_SIZE,  //B07F0080

	NAND_EVA_INFO_CNT,

	ENUM_DUMMY4WORD(NAND_EVA_INFO)
} NAND_EVA_INFO;

typedef enum {
	NAND_EVA_PERF_WRITE_PERF,
	NAND_EVA_PERF_READ_PERF,
	NAND_EVA_PERF_SIZE_PERF,
	NAND_EVA_PERF_RESULT,


	NAND_EVA_PERF_INFO_CNT,

	ENUM_DUMMY4WORD(NAND_EVA_PERF_INFO)
} NAND_EVA_PERF_INFO;

typedef enum {
	NAND_EVA_RW_ROBUST_START_SECTOR,
	NAND_EVA_RW_ROBUST_SECTOR_COUNT,
	NAND_EVA_RW_ROBUST_TEST_SIZE,
	NAND_EVA_RW_ROBUST_CURRENT_START_SECTOR,


	NAND_EVA_RW_ROBUST_CNT,

	ENUM_DUMMY4WORD(NAND_EVA_RW_ROBUST)
} NAND_EVA_RW_ROBUST;


/*
    @name   NAND driver error code

    Definition specific error code for nand driver, internal usage
*/
//@{
#define E_MARK_BAD                 (-100)
#define E_DEFECTIVE                (-101)
//@}

typedef enum {
	NANDCTRL_SDR_TYPE = 0x0,
	NANDCTRL_DTR_TYPE,

	DTR_TYPE_NUM,
	ENUM_DUMMY4WORD(DTR_TYPE_SEL)
} DTR_TYPE_SEL;

typedef enum {
	NANDCTRL_SPIFLASH_USE_INTERNAL_RS_ECC = 0x0,
	NANDCTRL_SPIFLASH_USE_ONDIE_ECC,

	ENUM_DUMMY4WORD(NAND_ECC_SEL)
} NAND_ECC_SEL;

typedef enum {
	NANDCTRL_SPIFLASH_USE_1_BIT = 0x0,
	NANDCTRL_SPIFLASH_USE_2_BIT,
	NANDCTRL_SPIFLASH_USE_4_BIT,

	ENUM_DUMMY4WORD(NAND_SPINAND_BIT_SEL)
} NAND_SPINAND_BIT_SEL;




typedef enum {
	NAND_DLL_SRC_CLK_FROM_PAD = 0x0,
	NAND_DLL_SRC_CLK_FROM_INTERNAL,

	NAND_DLL_SRC_CLK_NUM,
	ENUM_DUMMY4WORD(NAND_DLL_SRC_CLOCK_SEL)
} NAND_DLL_SRC_CLOCK_SEL;

typedef enum {
	NAND_DLL_SAMPLE_CLK_RISING = 0x0,
	NAND_DLL_SAMPLE_CLK_FALING,

	NAND_DLL_SAMPLE_CLK_NUM,
	ENUM_DUMMY4WORD(NAND_DLL_SAMPLE_CLOCK_SEL)
} NAND_DLL_SAMPLE_CLOCK_SEL;

#if defined(_NVT_EMULATION_)
typedef enum {
	NAND_CS0 = 0x0,
	NAND_CS1,

	NAND_CHIP_SEL_NUM,
	ENUM_DUMMY4WORD(NAND_CHIP_SEL)
} NAND_CHIP_SEL;

typedef enum {
	NAND_PIO_MODE = 0x0,
	NAND_DMA_MODE,

	NAND_TRANS_MODE_NUM,
	NAND_TRANS_MODE_INVALID,
	ENUM_DUMMY4WORD(NAND_TRANS_MODE)
} NAND_TRANS_MODE;

typedef enum {
	NAND_SINGLE_PAGE = 0x0,
	NAND_MULTIPLE_PAGE,

	NAND_TRANS_TYPE_NUM,
	NAND_TRANS_TYPE_INVALID,
	ENUM_DUMMY4WORD(NAND_TRANS_TYPE)
} NAND_TRANS_TYPE;



typedef struct {
	NAND_CHIP_SEL           uiCS;
	UINT32                  uiNANDClockRate;            //< NAND internal clock rate @note for pll.h
	NAND_PRIMARY_ECC_CODE   uiPriECCMethod;             //< NAND primary ECC method
	NAND_SEC_ECC_METHOD     uiSecECCMethod;             //< NAND secondary ECC method
	NAND_TRANS_TYPE         uiTransType;                //< NAND transfer type => single / multi page
	NAND_TRANS_MODE         uiTransMode;                //< NAND transfer => PIO / DMA mode
} EMUNANDCFG, *PEMUNANDCFG;

extern UINT32           nand_getTotalSize(void);
extern void             nand_setEmuNandConfig(PEMUNANDCFG gConfig);
extern ER               nand_emuReadOperation(INT8 *buffer, UINT32 pageAddress, UINT32 numPage);
extern ER               nand_emuWriteOperation_SINGLE(INT8 *buffer, UINT32 pageAddress, UINT32 numPage);
extern ER               nand_emuWriteOperation_MULTI(INT8 *buffer, UINT32 pageAddress, UINT32 numPage);
extern ER               nand_emuSetECCEnable(NAND_CHIP_SEL uiCS, NAND_PRIMARY_ECC_CODE uiPriECCSel, NAND_SEC_ECC_METHOD uiSecECCSel);
extern void             nand_emuProgramSpareSram(UINT16 usLogicalBlockNum, UINT8 ucBlockStatus, UINT8 ucReserved);
extern ER               nand_emuProgramSpareOperation(UINT32 pageAddress, BOOL bECCEN, BOOL bECCPat);
extern UINT32           nand_emuGetTmpBufferAddress(void);
extern void             emu_dump_open(char *pFilename);
extern void             emu_dump_write(UINT32 uiData, UINT32 uiSize);
extern void             emu_dump_close(void);
extern UINT32           nand_getBlockSize(void);
extern UINT32           nand_getSectorCnt(void);
extern UINT32           nand_getSectorSize(void);



#endif
extern UINT32           nand_getPageSize(void);
extern void             nand_hostSetTiming0(UINT32 uiTim0);
extern void             nand_hostSetTiming1(UINT32 uiTim1);
extern void             nand_hostSetTiming2(UINT32 uiTim2);



extern ER               nand_evaDumpInfo(void);
extern ER               nand_chkBlkDefective(UINT32 uiPhysicalBlockNum);

extern UINT32           nand_hostGetTiming0(void);
extern UINT32           nand_hostGetTiming1(void);

extern  ER              nand_host_config_checksum_enable(BOOL en);
extern  void            nand_host_clear_checksum_value(void);
extern  UINT16          nand_host_get_checksum_value(void);



#endif


