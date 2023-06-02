/**
    @file       NAND.h

    @ingroup    mIDrvStg_NAND

    @brief      Nand flash driver export variables and function prototypes.

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.

*/

#ifndef _NAND_H
#define _NAND_H

#ifndef _NAND2_SUPPORT_
#define _NAND2_SUPPORT_ 1
#endif

#include "strg_def.h"

/**
    @addtogroup mIDrvStg_NAND
*/
//@{

//------------------------------------------------------------------------------
// compatible with earily code base
//------------------------------------------------------------------------------
/**
    @name Backward compatible APIs

    For old version NAND driver
*/
//@{
#define EraseAllNANDBlockMaping()           nand_eraseAllPartitionBlockMapping(FALSE, FALSE, FALSE)
//@}
#if defined(_NVT_FPGA_)
#define NAND_SRC_CLK 48000000
#else
#define NAND_SRC_CLK 480000000
#endif

#define PLL_CLKSEL_NAND_48 0x9
//#define PLL_CLKSEL_NAND_60 0x7
#define PLL_CLKSEL_NAND_80 0x5
#define PLL_CLKSEL_NAND_96 0x4
#define PLL_CLKSEL_NAND_120 0x3


// -----------------------------------------------------------------------------
// NAND driver error code
// -----------------------------------------------------------------------------
//
#define NAND_ERROR_OK                       0x00000000
#define NAND_ERROR_INSUFFICIENT_FREE_BLOCK  0x00010000
#define NAND_ERROR_TOO_MANY_BAD_UNITS       0x00080000 //Too many bad blocks result in write protection rasen.(write and erase is prohibited)
#define NAND_ERROR_DATA_STATUS              0x00000001 //Data status is corrupt
#define NAND_ERROR_ECC_FIELD1               0x00000002
#define NAND_ERROR_ECC_FIELD2               0x00000004
#define NAND_ERROR_DOUBLE_MAPPING           0x00000008
#define NAND_ERROR_UNCORRECT_ECC            0x00000080
#define NAND_ERROR_READ_ERROR               0x00000010
#define NAND_ERROR_WRITE_ERROR              0x00000020
#define NAND_ERROR_DEVICE_ERROR             0x00000040


/**
    NAND write one reserved block param
*/
typedef struct {
	UINT32  uiLogicBlock;
	UINT32  uiPhysicalBlock;
} NAND_WRITERSVCFG, *PNAND_WRITERSVCFG;

/**
    NOR read non-blocking param
*/
typedef struct {
	UINT32  uiSectorAddr;
	UINT32  uiSectorSize;
    UINT8 	*pBuf;
} NOR_READ_NONBLK_CFG, *PNOR_READ_NONBLK_CFG;

/*
    NAND specific operation condition definition
\n  Some condition can not process anytime. Need turn on/off flag

    @note for nand_setFeature(), nand_getFeature()
*/
typedef enum {
	NAND_ERASE_LOADER,      //< TRUE: valid for erasing loader area
	NAND_ERASE_FW,          //< TRUE: valid for erasing FW area
	NAND_ERASE_PSTORE,      //< TRUE: valid for erasing Pstore area
	NAND_MARK_PSTORE,       //< TRUE: valid for mark bad block at Pstore area
	ENUM_DUMMY4WORD(NAND_FEATRUE)
} NAND_FEATRUE;

/**
    SPIFLASH operation

    @note For SPIFLASH_INVALID_WRITE_CB()
*/
typedef enum {
	SPIFLASH_OPERATION_READ,        ///< read operation
	SPIFLASH_OPERATION_WRITE,       ///< write operation
	SPIFLASH_OPERATION_ERASE,       ///< erase operation

	ENUM_DUMMY4WORD(SPIFLASH_OPERATION)
} SPIFLASH_OPERATION;

/**
    SPIFLASH invalid write callback

    Callback routine to be invoked when invalid erase/write access is detected by driver.

    @note STRG_EXT_CMD_SPI_INVALID_WRITE_CB

    @param[in] first parameter  Operating type
                                - @b SPIFLASH_OPERATION_ERASE: erase
                                - @b SPIFLASH_OPERATION_WRITE: write
    @param[in] second parameter Starting address
    @param[in] third parameter  Access length

    @return void
*/
typedef void (*SPIFLASH_INVALID_WRITE_CB)(SPIFLASH_OPERATION, unsigned int, unsigned int);

extern PSTORAGE_OBJ spiflash_getStorageObject(STRG_OBJ_ID strgObjID);


/**
    NAND configuration identifier

    @note For nand_setConfig()
*/
typedef enum {
	NAND_CONFIG_ID_FREQ,                    ///< NAND module clock (Unit: MHz), MUST config before storage object open, active after open
	///< Context can be one of the following:
	///< - @b 48    : 48 MHz (Default value)
	///< - @b 60    : 60 MHz
	///< - @b 96    : 96 MHz
	NAND_CONFIG_ID_TIMING0,                 ///< NAND controller timing0 register, MUST config before storage object open and need config timing1 at the same time
	///< Context is UINT32 value
	NAND_CONFIG_ID_TIMING1,                 ///< NAND controller timing1 register, MUST config before storage object open and need config timing0 at the same time
	///< Context is UINT32 value

	NAND_CONFIG_ID_DELAY_LATCH,             ///< Context can be one of the following:
	///< - @b TRUE  : 0.5T delay latch
	///< - @b FALSE : 0.0T delay latch
	///<   @note: TRUE equal to 0.5 tRP

	NAND_CONFIG_ID_SET_INTEN,               ///< Context can be one of the following:
	///< - @b TRUE  : Enable interrupt mode
	///< - @b FALSE : Disable interrupt mode
	///<   @note: Need config before any access of storage object hook on NAND device open\n
	///<          such as STRG_SET_PARTITION_SIZE, STRG_SET_PARTITION_RSV_SIZE, \n
	///<          STRG_GET_BEST_ACCESS_SIZE, STRG_GET_SECTOR_SIZE, STRG_GET_DEVICE_PHY_SIZE,\n
	///<          STRG_GET_MEMORY_SIZE_REQ
	///< @code
	///     if(nand_setConfig(NAND_CONFIG_ID_SET_INTEN, TRUE) != E_OK)
	///     {
	///         ERROR;
	///     }
	///     //Enable Interrupt mode
	//      if(nand_setConfig(NAND_CONFIG_ID_SET_INTEN, FALSE) != E_OK)
	///     {
	///         ERROR;
	///     }
	///     //Disable Interrupt mode
	///  }
	///  @endcode
	NAND_CONFIG_ID_AUTOPINMUX,              ///< Context can be one of the following:
	///< - @b TRUE  : disable pinmux when NAND driver close
	///< - @b FALSE : not disable pinmux when NAND driver close

	NAND_CONFIG_ID_NAND_TYPE,               ///< Context can be one of the following:
	///< - @b NANDCTRL_ONFI_NAND_TYPE   : ONFI NAND via NAND controller
	///< - @b NANDCTRL_SPI_NAND_TYPE    :  SPI NAND via NAND controller
	///< - @b NANDCTRL_SPI_NOR_TYPE     :  SPI NOR  via NAND controller
	///< - @b SPICTRL_SPI_NAND_TYPE     :  SPI NAND via  SPI controller

	NAND_CONFIG_ID_SPI_ECC_TYPE,            ///< Context can be one of the following(only available when device is SPI NAND flash):
	///< - @b NANDCTRL_SPIFLASH_USE_INTERNAL_RS_ECC : SPI NAND use nand controller reedsolomon ecc
	///< - @b NANDCTRL_SPIFLASH_USE_ONDIE_ECC       : SPI NAND use nand on die ecc(depend on each spi nand flash)

	NAND_CONFIG_ID_SPI_SEC_ECC_EN,          ///< Context can be one of the following(only available when device is SPI NAND flash):
	///< - @b TRUE  : Secondary ECC enable(only available when NAND_CONFIG_ID_SPI_ECC_TYPE = NANDCTRL_SPIFLASH_USE_INTERNAL_RS_ECC)
	///< - @b FALSE : Secondary ECC disable

	NAND_CONFIG_ID_SPI_OPERATION_BIT_MODE,  ///< Context can be one of the following(only available when device is SPI NAND flash):
	///< - @b NANDCTRL_SPIFLASH_USE_1_BIT : SPI NAND 1 bit operation mode
	///< - @b NANDCTRL_SPIFLASH_USE_2_BIT : SPI NAND 2 bit operation mode(dual mode)
	///< - @b NANDCTRL_SPIFLASH_USE_4_BIT : SPI NAND 4 bit operation mode(quad mode)

	NAND_CONFIG_ID_NOR_TYPE,                ///< Context can be one of the following:
	///< - @b NANDCTRL_SDR_TYPE   : SDR mode for SPI NOR
	///< - @b NANDCTRL_DRR_TYPE   : DTR mode for SPI NOR
	ENUM_DUMMY4WORD(NAND_CONFIG_ID)
} NAND_CONFIG_ID;

typedef enum {
	NANDCTRL_ONFI_NAND_TYPE = 0x0,
	NANDCTRL_SPI_NAND_TYPE,
	NANDCTRL_SPI_NOR_TYPE,
	SPICTRL_SPI_NAND_TYPE,

	NAND_TYPE_NUM,
	ENUM_DUMMY4WORD(NAND_TYPE_SEL)
} NAND_TYPE_SEL;


typedef enum {
	NAND_ERASE_UNCOND,      ///< unconditional erase block
	NAND_ERASE_EMPTY,       ///< erase block if this block is empty
	NAND_ERASE_LOGICALNUM   ///< erase block if this block is selected logic block number
} NAND_ERASE_CONDITION;
/**
    SPI Flash Wide Bus Capability
*/
typedef enum {
	SPI_FLASH_BUSWIDTH_NORMAL       = 0,        //< Normal 1bit full duplex flash
	SPI_FLASH_BUSWIDTH_DUAL         = 0x01,     //< Support dual read (0x3B)
	SPI_FLASH_BUSWIDTH_QUAD_TYPE1   = 0x10,     //< Support quad read (0xEB)
	                                            //< But QE (Quad Enable) bit is in Status Register[bit 6]
	                                            //<  and 0xEB command requires 4 dummy clocks
	SPI_FLASH_BUSWIDTH_QUAD_TYPE2   = 0x20,     //< Support quad read (0xEB)
	                                            //< But QE (Quad Enable) bit is in Status Register[bit 9]
	                                            //< and  and 0xEB command requires 4 dummy clocks
	SPI_FLASH_BUSWIDTH_QUAD_TYPE3   = 0x40,     //< Support quad read (0xEB)
	                                            //< But not require QE (Quad Enable) bit and 0xEB command requires 8 dummy clocks
	SPI_FLASH_BUSWIDTH_QUAD_TYPE4   = 0x80,     //< Support quad read, QE(Quad Enable) bit  is in Status Register[bit 9] and 0xEB command requires 4 dummy clocks.
	                                            //< But QE should be modified by 0x31 command

	SPI_FLASH_BUSWIDTH_QUAD_TYPE1_CMD31 = 0x100,        //< Write status 0x31 command
                                                        //< But QE should be modified by 0x31 command

	SPI_FLASH_BUSWIDTH_QUAD_TYPE5   = 0x200,            //< Support quad read (0xEB)
                                                        //< But QE should be modified by 0x31 command
	SPI_FLASH_BUSWIDTH_QUAD_DUMMY_6CYCLES = 0x400,      //< Require  6 dummy cycles in quad DTR mode
	SPI_FLASH_BUSWIDTH_QUAD_DUMMY_10CYCLES = 0x800,     //< Require 10 dummy cycles in quad DTR mode

    SPI_FLASH_BUSWIDTH_SINGLG_DUMMY_4CYCLES = 0x1000,   //< Require  4 dummy cycles in single DTR mode
    SPI_FLASH_BUSWIDTH_SINGLG_DUMMY_8CYCLES = 0x2000,   //< Require  8 dummy cycles in single DTR mode

	ENUM_DUMMY4WORD(SPI_FLASH_BUSWIDTH)
} SPI_FLASH_BUSWIDTH;

/**
    SPI flash identification structure

    @note For spiflash_open()
*/
typedef struct {
	UINT32      uiMfgID;            //< manufacture ID
	UINT32      uiMemType;          //< memory type
	UINT32      uiMemCapacity;      //< memory capacity
	UINT32      uiTotalSize;        //< total size (unit: byte)
	UINT32      uiSectorSize;       //< sector size (unit: byte)
	UINT32      uiBlockSize;        //< block size (unit: byte)
	UINT32      uiSectorCnt;        //< sectr count (unit: sector)
	BOOL        bSupportEWSR;       //< support EWSR command
	BOOL        bSupportAAI;        //< support AAI command
	BOOL        bSupportSecErase;   //< support SECTOR_ERASE command (0x20)
	UINT32      uiChipEraseTime;    //< CHIP erase time (unit: ms)
	UINT32      uiBlockEraseTime;   //< Block erase time (unit: ms)
	UINT32      uiSectorEraseTime;  //< Sector erase time (unit: ms)
	UINT32      uiPageProgramTime;  //< page program time (unit: ms)
	SPI_FLASH_BUSWIDTH  flashWidth; //< Wide bus (dual/quad) supported by this flash
} SPI_FLASH_INFO, *PSPI_FLASH_INFO;

/**
    SPI flash operation interface
*/
typedef struct {
	ER(*open)(void);                                    //< open
	ER(*close)(void);                                   //< close
	ER(*readID)(UINT8 *, UINT32);                       //< Read JEDEC ID from SPI nand
	//< Context are :
	//< - @b UINT8 * : Buffer to pass ID
	//< - @b UINT32 : buffer size (i.e. need to read how many bytes)
	ER(*eraseBlock)(UINT32);                            //< erase block
	//< Context are :
	//< - @b UINT32 : row address. Should be block alignment
	ER(*programPage)(UINT32, UINT32, UINT8 *, UINT32, UINT8 *, UINT32);                     // page program
	//< Context are:
	//< - @b UINT32: row address
	//< - @b UINT32: column address (valid range: 0 ~ 2111)
	//< - @b UINT8 * : buffer to be written to flash
	//< - @b UINT32 : buffer size
	//< - @b UINT8 * : buffer to write to spare aprea
	//< - @b UINT32 : spare area buffer size
	ER(*readPage)(UINT32, UINT32, UINT8 *, UINT32);                         // read page
	//< Context are:
	//< - @b UINT32: row address
	//< - @b UINT32: column address (valid range: 0 ~ 2111)
	//< - @b UINT8 * : buffer to be stored on DRAM
	//< - @b UINT32 : buffer size
	ER(*readMultiPage)(UINT32, UINT8 *, UINT32);                        // read whole block (skip spare area)
	//< Context are:
	//< - @b UINT32: row address
	//< - @b UINT8 * : buffer to be stored on DRAM
	//< - @b UINT32: page count
	ER(*setFreq)(UINT32);                               //< set operating frequency
	//< Context are:
	//< - @b UINT32 : frequency (unit: Hz)

	ER(*getStatus)(UINT32, UINT8 *);
	//< get status
	//< Context are:
	//< - @b UINT32 : SPI status set
	//< - @b UINT8 * : buffer to be stored on DRAM
	ER(*programBuffer)(UINT32, UINT8 *, UINT32);                      // page program into SPI NAND buffer
	//< Context are:
	//< - @b UINT32: column address (valid range: 0 ~ 2111)
	//< - @b UINT8 * : buffer to be written to flash
	//< - @b UINT32 : buffer size
	ER(*readBuffer)(UINT32, UINT8 *, UINT32);                         // read page from buffer
	//< Context are:
	//< - @b UINT32: column address (valid range: 0 ~ 2111)
	//< - @b UINT8 * : buffer to be stored on DRAM
	//< - @b UINT32 : buffer size
	ER(*setStatus)(UINT32, UINT8);
	//< set status frequency
	//< Context are:
	//< - @b UINT32 : SPI status set
	//< - @b UINT32 : SPI status to be write

	ER(*eraseNorSector)(UINT32);                        //< erase sector
	//< Context are :
	//< - @b UINT32 : address. Should be sector alignment (0x1000)

	ER(*norProgramPage)(UINT32, UINT32, UINT8 *);
	//< Nor flash page program(256 bytes)
	//< Context are :
	//< - @b UINT32 : address. Should be page alignment (256 bytes)
	//< - @b UINT32 : size Should be multiple of 256
	//< - @b UINT8 * : buffer to be stored on NAND


	ER(*norReadData)(UINT32, UINT32, UINT8 *);
	//< Nor flash read data(256 bytes)
	//< Context are :
	//< - @b UINT32 : address. Should be page alignment (256 bytes)
	//< - @b UINT32 : size Should be multiple of 256
	//< - @b UINT8 *: buffer to be stored on NAND


} NAND_SPI_OBJ, *PNAND_SPI_OBJ;

/**
    NAND host operation interface
*/
typedef struct {
	ER(*reset)(void);
	ER(*readID)(UINT8 *);
	ER(*eraseBlock)(UINT32);
	void (*nand_programSpare)(UINT16, UINT8, UINT8);
	ER(*nand_readOperation)(INT8 *, UINT32, UINT32);
	ER(*nand_writeOperation)(INT8 *, UINT32, UINT32);
	ER(*nand_condEraseBlock)(NAND_ERASE_CONDITION, UINT32, UINT32);
	ER(*nand_writeOperation_single)(INT8 *, UINT32, UINT32);
	ER(*nand_readPageSpareDataTotal)(UINT32 *, UINT32 *, UINT32 *, UINT32 *, UINT32);
	ER(*nand_readOperation_single)(INT8 *, UINT32, UINT32);
	ER(*nand_programBuffer)(UINT32, UINT8 *, UINT32);
	ER(*nand_readBuffer)(UINT32, UINT8 *, UINT32);
	ER(*nand_eraseSector)(UINT32);
	ER(*nand_readByBytes)(UINT32, UINT32, UINT8 *);
	ER(*nand_programSector)(UINT32, UINT32, UINT8 *);

} NAND_HostCmdOps, *PNAND_HostCmdOps;

/**
    SPI flash identification structure

    @note For SPIFLASH_IDENTIFY_CB
*/
typedef struct {
	UINT32      uiTotalSize;        ///< total size (unit: byte)
	UINT32      uiSectorSize;       ///< sector size (unit: byte)
	UINT32      uiBlockSize;        ///< block size (unit: byte)
	UINT32      uiSectorCnt;        ///< sectr count (unit: sector)
	BOOL        bSupportEWSR;       ///< support EWSR command
	BOOL        bSupportAAI;        ///< support AAI command
	BOOL        bSupportSecErase;   ///< support SECTOR_ERASE command (0x20)
	UINT32      uiChipEraseTime;    ///< CHIP erase time (unit: ms)
	UINT32      uiBlockEraseTime;   ///< Block erase time (unit: ms)
	UINT32      uiSectorEraseTime;  ///< Sector erase timeout (unit: ms)
	UINT32      uiPageProgramTime;  ///< page program timeout (unit: ms)
	SPI_FLASH_BUSWIDTH  flashWidth; ///< Wide bus (dual/quad) supported by this flash
} SPIFLASH_IDENTIFY, *PSPIFLASH_IDENTIFY;


/**
    SPIFLASH identify callback

    Callback routine to be invoked after JEDEC ID is read from spi flash.
    Callback routine should check if read ID is supported.

    @note STRG_EXT_CMD_SPI_IDENTIFY_CB

    @param[in] first parameter  (JEDEC) manufacturer ID read from spi flash
    @param[in] second parameter (JEDEC) type ID read from spi flash
    @param[in] third parameter  (JEDEC) capacity ID read from spi flash
    @param[out] forth parameter flash identification returned to spi flash driver

    @return
        - @b TRUE: call back will handle identification of this flash. and PSPI_IDENTIFY will fill identifed information
        - @b FALSE: input ID is NOT supported/identified by call back
*/
typedef BOOL (*SPIFLASH_IDENTIFY_CB)(UINT32, UINT32, UINT32, PSPIFLASH_IDENTIFY);

extern ER        nand_emuReadPageSpareDataTotal(UINT32 *spare0, UINT32 *spare1, UINT32 *spare2, UINT32 *spare3, UINT32 pageAddress);
extern ER        nand_setFeature(NAND_FEATRUE Nand_Feature, UINT32 uiParameter);
#if _NAND2_SUPPORT_
extern PSTRG_TAB nand2_getStorageObject(void);
extern ER        nand2_writeSectors(INT8 *pcBuf, UINT32 ulSecNum, UINT32 ulSctCnt);
extern ER        nand2_readSectors(INT8 *pcBuf, UINT32 ulSecNum, UINT32 ulSctCnt);
extern void      nand2_setSize(UINT32 size);
ER               nand2_format(void);
void             nand2_FormatEraseNAND(void);
#endif
extern ER           nand_setConfig(NAND_CONFIG_ID ConfigID, UINT32 uiConfig);
extern INT32        nand_getConfig(NAND_CONFIG_ID ConfigID);
extern void         nand_dumpInfo(void);
extern void         nand_dumpAllData(UINT32 uiBuf, UINT32  uiSize);
extern PSTORAGE_OBJ nand_getStorageObject(STRG_OBJ_ID strgObjID);
extern void         nand_init_fat_storage_object(void);
extern void         nand_eraseAllPartitionBlockMapping(BOOL bForceErMarked, BOOL bForceErDefected, BOOL force_erase_all);

extern PNAND_SPI_OBJ    nand_ctrl_spi_nor_getObject(void);
extern ER               nand_attach(void);
extern void             nand_detach(void);
extern ER               nand_identify(void);
extern BOOL             spiflash_isInfoReady(void);
extern ER               spiflash_getAttachFlashInfo(PSPI_FLASH_INFO *pFlashInfo);

//@}

#endif // _NAND_H


