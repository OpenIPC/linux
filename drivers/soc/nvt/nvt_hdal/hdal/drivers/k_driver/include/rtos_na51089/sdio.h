/**
    Header file for SDIO module driver

    @file       SDIO.h
    @ingroup    mIDrvStg_SDIO

    @brief      SDIO Card driver header

    Copyright   Novatek Microelectronics Corp. 2009.  All rights reserved.
*/

#ifndef _SDIO_H
#define _SDIO_H

#include "strg_def.h"
#include "sdmmc_desc.h"

/**
    @addtogroup mIDrvStg_SDIO
*/
//@{

/*
    @name SDIO card detect function prototype

    SDIO card insert/protect detection call back prototype

    @note for sdio_setDetectCardExistHdl(), sdio2_setDetectCardExistHdl(), sdio_setDetectCardProtectHdl(), sdio2_setDetectCardProtectHdl()
*/
//@{
typedef BOOL (*SDIO_CARD_DETECT_CB)(void);  ///< card insert/detect function prototype
//@}

/*
    @name SDIO IO pad control function prototype

    SDIO IO pad control call back prototype

    @note for sdio_setCardIOHdl(), sdio2_setCardIOHdl()
*/
//@{
typedef void (*SDIO_SET_CARDIO_CB)(void);   ///< SDIO IO pad call back function prototype
//@}

/*
    @name SDIO power control call back prototype

    @param[in] BOOL power on/off control
                    - @b TRUE: power on
                    - @b FALSE: power off

    @return void
*/
//@{
typedef void (*SDIO_CARD_PWR_CTRL_CB)(BOOL);
//@}

/**
    @name SDIO general call back prototype

    @param[in] BOOL

    @return BOOL
*/
typedef BOOL (*SDIO_CALLBACK_HDL)(BOOL);

//------------------------------------------------------------
// Definitions for exported interface
//------------------------------------------------------------

/**
    @name SDIO source clock definition

    @note for sdioHost_setSrcClock()/sdioHost_getSrcClock()
*/
//@{
#define SDIO_SRC_CLK_20MHZ      0   ///< No use
#define SDIO_SRC_CLK_24MHZ      1   ///< No use
#define SDIO_SRC_CLK_40MHZ      2   ///< No use
#define SDIO_SRC_CLK_48MHZ      3   ///< No use
#define SDIO_SRC_CLK_60MHZ      4   ///< No use
#define SDIO_SRC_CLK_80MHZ      5   ///< No use
#define SDIO_SRC_CLK_SSPLLMHZ   6   ///< No use
#define SDIO_SRC_CLK_SSPLL2MHZ  7   ///< No use
#define SDIO_SRC_CLK_SSPLL3MHZ  8   ///< No use
#define SDIO_SRC_CLK_SSPLL4MHZ  9   ///< No use
#define SDIO_SRC_CLK_SSPLL5MHZ 10   ///< No use
#define SDIO_SRC_CLK_96MHZ     11   ///< No use
#define SDIO_SRC_CLK_PLL4      12   ///< Source clock PLL4
#define SDIO_SRC_CLK_PLL2      13   ///< Source clock PLL2
#define SDIO_SRC_CLK_480MHZ    14   ///< Source clock 480MHz
#define SDIO_SRC_CLK_192MHZ    15   ///< Source clock 192MHz
#define SDIO_SRC_CLK_320MHZ    16   ///< Source clock 320MHz
#define SDIO_SRC_CLK_PLL9      17   ///< Source clock PLL9
#define SDIO_SRC_CLK_END       SDIO_SRC_CLK_PLL9
//@}

/**
    @name SDIO IO RW flag definition.

    SDIO IO Read Write flag.

    SDIO IO Read Write flag.

    @note for SDIO_CMD52_STRUCT, SDIO_CMD53_STRUCT

*/
//@{
#define SDIO_IO_R                           0x0         ///< IO Read flag
#define SDIO_IO_W                           0x1         ///< IO Write flag
//@}

/**
    @name SDIO IO RAW (Read After Write) definition.

    @note for SDIO_CMD52_STRUCT

*/
//@{
#define SDIO_IO_DIS_RAW                     0x0         ///< Disable IO RAW function
#define SDIO_IO_EN_RAW                      0x1         ///< Enable IO RAW function
//@}

/**
    @name SDIO IO byte/block access definition.

    @note for SDIO_CMD53_STRUCT

*/
//@{
#define SDIO_IO_BYTEMODE                    0x0         ///< IO byte access mode
#define SDIO_IO_BLOCKMODE                   0x1         ///< IO block access mode
//@}

/**
    @name SDIO IO OP code definition.

    @note for SDIO_CMD53_STRUCT

*/
//@{
#define SDIO_IO_OPCODE_FIX_ADDR             0           ///< OP code: multi byte R/W to fixed address
#define SDIO_IO_OPCODE_INCREMENT_ADDR       1           ///< OP code: multi byte R/W to incrementing address
//@}

/**
    @name SDIO IO function select definition.

    @note for SDIO_CMD52_STRUCT, SDIO_CMD53_STRUCT

*/
//@{
#define SDIO_IO_FUNNUM0                     0           ///< Function 0 on IO device
#define SDIO_IO_FUNNUM1                     1           ///< Function 1 on IO device
#define SDIO_IO_FUNNUM2                     2           ///< Function 2 on IO device
#define SDIO_IO_FUNNUM3                     3           ///< Function 3 on IO device
#define SDIO_IO_FUNNUM4                     4           ///< Function 4 on IO device
#define SDIO_IO_FUNNUM5                     5           ///< Function 5 on IO device
#define SDIO_IO_FUNNUM6                     6           ///< Function 6 on IO device
#define SDIO_IO_FUNNUM7                     7           ///< Function 7 on IO device
//@}

/**
    @name SDIO input delay config

    @note for SDIO_CONFIG_ID_XXX_INPUT_DELAY in SDIO_CONFIG_ID
*/
//@{
#define SDIO_IN_DLY_CLK_SD                  (1<<9)
#define SDIO_IN_DLY_ACT                     (1<<12)
#define SDIO_IN_DLY_POLARITY                (1<<16)
#define SDIO_IN_DLY_SEL(val)                ((val)<<24)
//@}

/**
    SDIO card type

    @note For SDIO_MISC_INFORMATION
*/
typedef enum {
	SDIO_CARD_UNKNOWN,          ///< unknown card type
	SDIO_CARD_SD,               ///< SD card
	SDIO_CARD_SDHC,             ///< SDHC card
	SDIO_CARD_SDXC,             ///< SDXC card
	SDIO_CARD_MMC,              ///< MMC card
	SDIO_CARD_SDIO,             ///< SDIO card
	ENUM_DUMMY4WORD(SDIO_CARD_TYPE)
} SDIO_CARD_TYPE;

/**
    SDIO call back list
*/
typedef enum {
	SDIO_CALLBACK_CARD_DETECT,      ///< card detect callback
	SDIO_CALLBACK_WRITE_PROTECT,    ///< write protect detect callback
	SDIO_CALLBACK_IO_PAD,           ///< IO pad control callback
	SDIO_CALLBACK_POWER_CONTROL,    ///< power control callback
	SDIO_CALLBACK_IO_INT,           ///< Card IO interrupt callback
	ENUM_DUMMY4WORD(SDIO_CALLBACK)
} SDIO_CALLBACK;

/**
    SDIO configuration identifier

    @note For sdio_setConfig() and sdio_setInfo()
*/
typedef enum {
	SDIO_CONFIG_ID_BUS_SPEED_MODE,          ///< Select Bus Speed Mode (DS, HS, SDR12, SDR25, SDR50, DDR50)
	///< Context can be ORed of:
	///< - @b SDIO_BUS_SPEED_MODE_DS (Defalut value)
	///< - @b SDIO_BUS_SPEED_MODE_HS (Defalut value)
	///< - @b SDIO_BUS_SPEED_MODE_SDR12
	///< - @b SDIO_BUS_SPEED_MODE_SDR25
	///< - @b SDIO_BUS_SPEED_MODE_SDR50
	///< - @b SDIO_BUS_SPEED_MODE_DDR50
	SDIO_CONFIG_ID_DRIVING_SINK_EN,         ///< Enable adjust driving/sink
	///< - @b TRUE: enable SD driver to adjust driving/sink of clock pin according to SDIO_CONFIG_ID_BUS_SPEED_MODE
	///< - @b FALSE: SD driver will NEVER adjust driving/sink of clock pin (Default value)
	SDIO_CONFIG_ID_BUS_WIDTH,               ///< Set max bus width of SDIO host
	///< Context can be any of:
	///< - @b SD_HOST_BUS_1_BIT
	///< - @b SD_HOST_BUS_4_BITS (Default value)
	///< - @b SD_HOST_BUS_8_BITS
	SDIO_CONFIG_ID_FORMAD_OEMID,            ///< OEM ID to format SD card (valid context: ASCII String with length <= 8)
	SDIO_CONFIG_ID_FORMAD_VOL_LABLE,        ///< Volume lable to format SD card (valid context: ASCII String with length <= 11)
	SDIO_CONFIG_ID_FORMAD_VOL_ID,           ///< Volume ID to format SD card (valid context: 0x00000000 ~ 0xFFFFFFFF)
	SDIO_CONFIG_ID_FORMAD_RAND_VOL_ID_EN,   ///< Enable format SD card with randomized Volume ID
	///< Context can be any of:
	///< - @b TRUE: enable format SD card with randomized volume ID
	///< - @b FALSE: use volume ID specified by SDIO_CONFIG_ID_FORMAD_VOL_ID
	SDIO_CONFIG_ID_IMPROVE_COMP_EN,         ///< Enable improved compatibility mode
	///< Context can be any of:
	///< - @b TRUE: enable improved compatibility mode (i.e. no high speed for SD card <= 512MB)
	///< - @b FALSE: disable improved compatibility mode
	SDIO_CONFIG_ID_IO_EN,                   ///< Enable SDIO IO function
	///< Context can be any of:
	///< - @b TRUE: enable support of SDIO IO device
	///< - @b FALSE: disable support of SDIO IO device
	SDIO_CONFIG_ID_MMC_EN,                  ///< Enable support of MMC card
	///< Context can be any of:
	///< - @b TRUE: enable support of MMC card
	///< - @b FALSE: disable support of MMC card
	SDIO_CONFIG_ID_CLK_GATING,              ///< Set clock output high or low when auto clock gating
	///< - @b SDIO_CLK_AUTOGATING_LOW (Defalut value)
	///< - @b SDIO_CLK_AUTOGATING_HIGH
	SDIO_CONFIG_ID_SRCLK,                   ///< Set source clock for SDIO host
	///< Context can be any of:
	///< - @b SDIO_SRC_CLK_192MHZ (Default value)
	///< - @b SDIO_SRC_CLK_480MHZ
	///< - @b SDIO_SRC_CLK_PLL2
	///< - @b SDIO_SRC_CLK_PLL4
	SDIO_CONFIG_ID_DS_MAX_CLK,              ///< Set maximum clock frequency in Default Speed Mode
	///< Valid value: 1~25000000, unit: Hz
	///< Valid when SDIO_CONFIG_ID_BUS_SPEED_MODE has SDIO_BUS_SPEED_MODE_DS
	SDIO_CONFIG_ID_DS_DRIVING,              ///< Set driving/sink of clock pin when card is operated in Default Speed Mode
	///< valid context: (5, 10, 15, 20) * 10, unit: mA. Default 10 mA
	///< (Valid when SDIO_CONFIG_ID_DRIVING_SINK_EN is set to TRUE)
	SDIO_CONFIG_ID_DS_INPUT_DELAY,          ///< Set input delay parameter in Default Speed Mode
	SDIO_CONFIG_ID_DS_OUTPUT_DELAY,         ///< Set output delay parameter in Default Speed Mode
	SDIO_CONFIG_ID_HS_MAX_CLK,              ///< Set maximum clock frequency in High Speed Mode
	///< Valid value: 1~50000000, unit: Hz
	///< Valid when SDIO_CONFIG_ID_BUS_SPEED_MODE has SDIO_BUS_SPEED_MODE_HS
	SDIO_CONFIG_ID_HS_DRIVING,              ///< Set driving/sink of clock pin when card is operated in High Speed Mode
	///< valid context: (5, 10, 15, 20) * 10, unit: mA. Default 10 mA
	///< (Valid when SDIO_CONFIG_ID_DRIVING_SINK_EN is set set to TRUE)
	SDIO_CONFIG_ID_HS_INPUT_DELAY,          ///< Set input delay parameter in High Speed Mode
	SDIO_CONFIG_ID_HS_OUTPUT_DELAY,         ///< Set output delay parameter in High Speed Mode
	SDIO_CONFIG_ID_SDR12_MAX_CLK,           ///< Set maximum clock frequency in UHS-I SDR12 Mode
	///< (Valid value: 1~25000000, unit: Hz)
	///< (Valid when SDIO_CONFIG_ID_BUS_SPEED_MODE has SDIO_BUS_SPEED_MODE_SDR12)
	SDIO_CONFIG_ID_SDR12_DRIVING,           ///< Set driving/sink when card is operated in UHS-I SDR12 Mode
	///< (valid context: (5, 10, 15, 20) * 10, unit: mA. Default 10 mA)
	///< (Valid when SDIO_CONFIG_ID_DRIVING_SINK_EN is set set to TRUE)
	SDIO_CONFIG_ID_SDR12_INPUT_DELAY,       ///< Set input delay parameter in UHS-I SDR12 Mode
	SDIO_CONFIG_ID_SDR12_OUTPUT_DELAY,      ///< Set output delay parameter in UHS-I SDR12 Mode
	SDIO_CONFIG_ID_SDR25_MAX_CLK,           ///< Set maximum clock frequency in UHS-I SDR25 Mode
	///< (Valid value: 1~50000000, unit: Hz)
	///< (Valid when SDIO_CONFIG_ID_BUS_SPEED_MODE has SDIO_BUS_SPEED_MODE_SDR25)
	SDIO_CONFIG_ID_SDR25_DRIVING,           ///< Set driving/sink when card is operated in UHS-I SDR25 Mode
	///< (valid context: (5, 10, 15, 20) * 10, unit: mA. Default 10 mA)
	///< (Valid when SDIO_CONFIG_ID_DRIVING_SINK_EN is set set to TRUE)
	SDIO_CONFIG_ID_SDR25_INPUT_DELAY,       ///< Set input delay parameter in UHS-I SDR25 Mode
	SDIO_CONFIG_ID_SDR25_OUTPUT_DELAY,      ///< Set output delay parameter in UHS-I SDR25 Mode
	SDIO_CONFIG_ID_SDR50_MAX_CLK,           ///< Set maximum clock frequency in UHS-I SDR50 Mode
	///< (Valid value: 1~100000000, unit: Hz)
	///< (Valid when SDIO_CONFIG_ID_BUS_SPEED_MODE has SDIO_BUS_SPEED_MODE_SDR50)
	SDIO_CONFIG_ID_SDR50_DRIVING,           ///< Set driving/sink when card is operated in UHS-I SDR50 Mode
	///< (valid context: (5, 10, 15, 20) * 10, unit: mA. Default 10 mA)
	///< (Valid when SDIO_CONFIG_ID_DRIVING_SINK_EN is set set to TRUE)
	SDIO_CONFIG_ID_SDR50_INPUT_DELAY,       ///< Set input delay parameter in UHS-I SDR50 Mode
	SDIO_CONFIG_ID_SDR50_OUTPUT_DELAY,      ///< Set output delay parameter in UHS-I SDR50 Mode
	SDIO_CONFIG_ID_DDR50_MAX_CLK,           ///< Set maximum clock frequency in UHS-I DDR50 Mode
	///< (Valid value: 1~50000000, unit: Hz)
	///< (Valid when SDIO_CONFIG_ID_BUS_SPEED_MODE has SDIO_BUS_SPEED_MODE_DDR50)
	SDIO_CONFIG_ID_DDR50_DRIVING,           ///< Set driving/sink when card is operated in UHS-I DDR50 Mode
	///< (valid context: (5, 10, 15, 20) * 10, unit: mA. Default 10 mA)
	///< (Valid when SDIO_CONFIG_ID_DRIVING_SINK_EN is set set to TRUE)
	SDIO_CONFIG_ID_DDR50_INPUT_DELAY,       ///< Set input delay parameter in UHS-I DDR50 Mode
	SDIO_CONFIG_ID_DDR50_OUTPUT_DELAY,      ///< Set output delay parameter in UHS-I DDR50 Mode
	SDIO_CONFIG_ID_SDR104_MAX_CLK,           ///< Set maximum clock frequency in UHS-I SDR104 Mode
	///< (Valid value: 1~208000000, unit: Hz)
	///< (Valid when SDIO_CONFIG_ID_BUS_SPEED_MODE has SDIO_BUS_SPEED_MODE_SDR104)
	SDIO_CONFIG_ID_SDR104_DRIVING,           ///< Set driving/sink when card is operated in UHS-I SDR104 Mode
	///< (valid context: (5, 10, 15, 20) * 10, unit: mA. Default 10 mA)
	///< (Valid when SDIO_CONFIG_ID_DRIVING_SINK_EN is set set to TRUE)
	SDIO_CONFIG_ID_SDR104_INPUT_DELAY,      ///< Set input delay parameter in UHS-I SDR104 Mode
	SDIO_CONFIG_ID_SDR104_OUTPUT_DELAY,     ///< Set output delay parameter in UHS-I SDR104 Mode

	SDIO_CONFIG_ID_HS200_MAX_CLK,           ///< Set maximum clock frequency in HS200
	///< (Valid value: 1~96000000, unit: Hz)
	///< (Valid when SDIO_CONFIG_ID_BUS_SPEED_MODE has SDIO_BUS_SPEED_MODE_HS200)
	SDIO_CONFIG_ID_HS200_DRIVING,           ///< Set driving/sink when card is operated in HS200
	///< (valid context: (10, 16) * 10, unit: mA. Default 10 mA)
	///< (Valid when SDIO_CONFIG_ID_DRIVING_SINK_EN is set set to TRUE)
	SDIO_CONFIG_ID_HS200_INPUT_DELAY,      ///< Set input delay parameter in HS200
	SDIO_CONFIG_ID_HS200_OUTPUT_DELAY,     ///< Set output delay parameter in HS200

	SDIO_CONFIG_ID_AUTO_CALIBRATION,        ///< Enable/Disable input delay auto calibration (valid with UHS-I SDR card)

	SDIO_CONFIG_ID_DIS_CMD17_RANGE_CHECK,   ///< Disable Out_Of_Range Check after CMD17 read last block on card
	///< - @b TRUE: disable check. Can work with some non-standard card
	///< - @b FALSE: enable check. Follow SD spec. (default value)
	SDIO_CONFIG_ID_UHSI_EN,                 ///< Enable/Disable UHSI.

	SDIO_CONFIG_ID_AUTOPINMUX,              ///< Auto pinmux to gpio when close
	///< - @b TRUE: enable set pinmux to gpio when close
	///< - @b FALSE: not set pinmux to gpio when close

	SDIO_CONFIG_EMMC_BOOT_ACK,              ///< Set EMMC BOOT ACK
	SDIO_CONFIG_EMMC_BOOT_ALT,              ///< Set EMMC BOOT Alternative
	SDIO_CONFIG_EMMC_BOOT_CLK,              ///< Set EMMC BOOT CLK selection
	SDIO_CONFIG_EMMC_BOOT_BUS_WIDTH,        ///< Set EMMC BOOT BUS WIDTH
	SDIO_CONFIG_EMMC_BOOT_SPEED_MODE,       ///< Set EMMC BOOT SPEED MODE

	SDIO_CONFIG_ID_DRIVING_SINK_CMDDATA_EN, ///< Enable adjust cmd+data driving/sink
	///< - @b TRUE: enable SD driver to adjust driving/sink of cmd+data pin according to SDIO_CONFIG_ID_BUS_SPEED_MODE
	///< - @b FALSE: SD driver will keep the driving/sink setting of cmd+data pin (for backword compatible)
	SDIO_CONFIG_ID_DS_CMD_DRIVING,          ///< Set driving/sink of cmd pin when card is operated in Default Speed Mode
	///< valid context: (2.5, 5, 7.5, 10) * 10, unit: mA. Default 10 mA
	///< (Valid when SDIO_CONFIG_ID_DRIVING_SINK_EN & SDIO_CONFIG_ID_DRIVING_SINK_CMDDATA_EN  is set to TRUE)
	SDIO_CONFIG_ID_DS_DATA_DRIVING,         ///< Set driving/sink of data pin when card is operated in Default Speed Mode
	///< valid context: (2.5, 5, 7.5, 10) * 10, unit: mA. Default 10 mA
	///< (Valid when SDIO_CONFIG_ID_DRIVING_SINK_EN & SDIO_CONFIG_ID_DRIVING_SINK_CMDDATA_EN  is set to TRUE)
	SDIO_CONFIG_ID_HS_CMD_DRIVING,          ///< Set driving/sink of cmd pin when card is operated in High Speed Mode
	///< valid context: (2.5, 5, 7.5, 10) * 10, unit: mA. Default 10 mA
	///< (Valid when SDIO_CONFIG_ID_DRIVING_SINK_EN & SDIO_CONFIG_ID_DRIVING_SINK_CMDDATA_EN  is set to TRUE)
	SDIO_CONFIG_ID_HS_DATA_DRIVING,         ///< Set driving/sink of data pin when card is operated in High Speed Mode
	///< valid context: (2.5, 5, 7.5, 10) * 10, unit: mA. Default 10 mA
	///< (Valid when SDIO_CONFIG_ID_DRIVING_SINK_EN & SDIO_CONFIG_ID_DRIVING_SINK_CMDDATA_EN  is set to TRUE)
	SDIO_CONFIG_ID_SDR12_CMD_DRIVING,       ///< Set driving/sink of cmd pin when card is operated in UHS-I SDR12 Mode
	///< valid context: (2.5, 5, 7.5, 10) * 10, unit: mA. Default 10 mA
	///< (Valid when SDIO_CONFIG_ID_DRIVING_SINK_EN & SDIO_CONFIG_ID_DRIVING_SINK_CMDDATA_EN is set to TRUE)
	SDIO_CONFIG_ID_SDR12_DATA_DRIVING,      ///< Set driving/sink of data pin when card is operated in UHS-I SDR12 Mode
	///< valid context: (2.5, 5, 7.5, 10) * 10, unit: mA. Default 10 mA
	///< (Valid when SDIO_CONFIG_ID_DRIVING_SINK_EN & SDIO_CONFIG_ID_DRIVING_SINK_CMDDATA_EN is set to TRUE)
	SDIO_CONFIG_ID_SDR25_CMD_DRIVING,       ///< Set driving/sink of cmd pin when card is operated in UHS-I SDR25 Mode
	///< valid context: (2.5, 5, 7.5, 10) * 10, unit: mA. Default 10 mA
	///< (Valid when SDIO_CONFIG_ID_DRIVING_SINK_EN & SDIO_CONFIG_ID_DRIVING_SINK_CMDDATA_EN  is set to TRUE)
	SDIO_CONFIG_ID_SDR25_DATA_DRIVING,      ///< Set driving/sink of data pin when card is operated in UHS-I SDR25 Mode
	///< valid context: (2.5, 5, 7.5, 10) * 10, unit: mA. Default 10 mA
	///< (Valid when SDIO_CONFIG_ID_DRIVING_SINK_EN & SDIO_CONFIG_ID_DRIVING_SINK_CMDDATA_EN  is set to TRUE)
	SDIO_CONFIG_ID_SDR50_CMD_DRIVING,       ///< Set driving/sink of cmd pin when card is operated in UHS-I SDR50 Mode
	///< valid context: (2.5, 5, 7.5, 10) * 10, unit: mA. Default 10 mA
	///< (Valid when SDIO_CONFIG_ID_DRIVING_SINK_EN & SDIO_CONFIG_ID_DRIVING_SINK_CMDDATA_EN  is set to TRUE)
	SDIO_CONFIG_ID_SDR50_DATA_DRIVING,      ///< Set driving/sink of data pin when card is operated in UHS-I SDR50 Mode
	///< valid context: (2.5, 5, 7.5, 10) * 10, unit: mA. Default 10 mA
	///< (Valid when SDIO_CONFIG_ID_DRIVING_SINK_EN & SDIO_CONFIG_ID_DRIVING_SINK_CMDDATA_EN  is set to TRUE)
	SDIO_CONFIG_ID_DDR50_CMD_DRIVING,       ///< Set driving/sink of cmd pin when card is operated in UHS-I DDR50 Mode
	///< valid context: (2.5, 5, 7.5, 10) * 10, unit: mA. Default 10 mA
	///< (Valid when SDIO_CONFIG_ID_DRIVING_SINK_EN & SDIO_CONFIG_ID_DRIVING_SINK_CMDDATA_EN  is set to TRUE)
	SDIO_CONFIG_ID_DDR50_DATA_DRIVING,      ///< Set driving/sink of data pin when card is operated in UHS-I DDR50 Mode
	///< valid context: (2.5, 5, 7.5, 10) * 10, unit: mA. Default 10 mA
	///< (Valid when SDIO_CONFIG_ID_DRIVING_SINK_EN & SDIO_CONFIG_ID_DRIVING_SINK_CMDDATA_EN  is set to TRUE)
	SDIO_CONFIG_ID_CMD23_EN,                ///< Enable CMD23 (set block count) support
	///< - @b TRUE: support CMD23 if card support this command (default value)
	///< - @b FALSE: don't issue CMD23 even card support this command
	SDIO_CONFIG_ID_SDR104_CMD_DRIVING,       ///< Set driving/sink of cmd pin when card is operated in UHS-I SDR104 Mode
	///< valid context: (2.5, 5, 7.5, 10) * 10, unit: mA. Default 10 mA
	///< (Valid when SDIO_CONFIG_ID_DRIVING_SINK_EN & SDIO_CONFIG_ID_DRIVING_SINK_CMDDATA_EN  is set to TRUE)
	SDIO_CONFIG_ID_SDR104_DATA_DRIVING,      ///< Set driving/sink of data pin when card is operated in UHS-I SDR104 Mode
	///< valid context: (2.5, 5, 7.5, 10) * 10, unit: mA. Default 10 mA
	///< (Valid when SDIO_CONFIG_ID_DRIVING_SINK_EN & SDIO_CONFIG_ID_DRIVING_SINK_CMDDATA_EN  is set to TRUE)
	SDIO_CONFIG_ID_HS200_CMD_DRIVING,       ///< Set driving/sink of cmd pin when card is operated in HS200
	///< valid context: (4, 10) * 10, unit: mA. Default 10 mA
	///< (Valid when SDIO_CONFIG_ID_DRIVING_SINK_EN & SDIO_CONFIG_ID_DRIVING_SINK_CMDDATA_EN  is set to TRUE)
	SDIO_CONFIG_ID_HS200_DATA_DRIVING,      ///< Set driving/sink of data pin when card is operated in HS200
	///< valid context: (4, 10) * 10, unit: mA. Default 10 mA
	///< (Valid when SDIO_CONFIG_ID_DRIVING_SINK_EN & SDIO_CONFIG_ID_DRIVING_SINK_CMDDATA_EN  is set to TRUE)

	SDIO_CONFIG_ID_DIRECT_MODE,             ///< Direct mode has higher priority than DMA/PIO mode
	SDIO_CONFIG_SDIO_PROTOCOL_DEBUG,

	SDIO_SET_INFO_VER = 0x00010001,         ///< START Tag for sdio_setInfo()
	SDIO_INFOR_END_TAG = 0x444E4549,        ///< END Tag for sdio_setInfo() ('I''E''N''D')
	ENUM_DUMMY4WORD(SDIO_CONFIG_ID)
} SDIO_CONFIG_ID;

/**
    SDIO Bus Speed Mode

    @note For sdio_setConfig(SDIO_CONFIG_ID_BUS_SPEED_MODE, XXX)
*/
typedef enum {
	SDIO_BUS_SPEED_MODE_DS = 0x01,      ///< Default Speed
	SDIO_BUS_SPEED_MODE_HS = 0x02,      ///< High Seed
	SDIO_BUS_SPEED_MODE_SDR12 = 0x04,   ///< UHS-I SDR12
	SDIO_BUS_SPEED_MODE_SDR25 = 0x08,   ///< UHS-I SDR25
	SDIO_BUS_SPEED_MODE_SDR50 = 0x10,   ///< UHS-I SDR50
	SDIO_BUS_SPEED_MODE_DDR50 = 0x20,   ///< UHS-I DDR50
	SDIO_BUS_SPEED_MODE_SDR104 = 0x40,  ///< UHS-I SDR104

	SDIO_BUS_SPEED_MODE_HS200 = 0x80,   ///< HS200-SDR

	ENUM_DUMMY4WORD(SDIO_BUS_SPEED_MODE)
} SDIO_BUS_SPEED_MODE;

/**
    EMMC BOOT ACK configuration

    @note For sdio_setConfig(SDIO_CONFIG_EMMC_BOOT_ACK, XXX)
*/
typedef enum {
	SDIO_EMMC_BOOT_ACK_DIS = 0,         ///< EMMC BOOT ACK DISALBE
	SDIO_EMMC_BOOT_ACK_EN,              ///< EMMC BOOT ACK ENABLE

	ENUM_DUMMY4WORD(SDIO_EMMC_BOOT_ACK)
} SDIO_EMMC_BOOT_ACK;

/**
    EMMC BOOT BUS WIDTH configuration

    @note For sdio_setConfig(SDIO_CONFIG_EMMC_BOOT_BUS_WIDTH, XXX)
*/
typedef enum {
	SDIO_EMMC_BOOT_BUS_WIDTH_1 = 0,    ///< EMMC BOOT BUS WIDTH 1 bit
	SDIO_EMMC_BOOT_BUS_WIDTH_4,        ///< EMMC BOOT BUS WIDTH 4 bits
	SDIO_EMMC_BOOT_BUS_WIDTH_8,        ///< EMMC BOOT BUS WIDTH 8 bits

	ENUM_DUMMY4WORD(SDIO_EMMC_BOOT_BUS_WIDTH)
} SDIO_EMMC_BOOT_BUS_WIDTH;

/**
    EMMC BOOT speed mode configuration

    @note For sdio_setConfig(SDIO_CONFIG_EMMC_BOOT_SPEED_MODE, XXX)
*/
typedef enum {
	SDIO_EMMC_BOOT_SPEED_MODE_SINGLE_BACK = 0, ///< EMMC BOOT MODE: single backward
	SDIO_EMMC_BOOT_SPEED_MODE_SINGLE_HIGH,     ///< EMMC BOOT MODE: single high speed
	SDIO_EMMC_BOOT_SPEED_MODE_DUAL,            ///< EMMC BOOT MODE: dual

	ENUM_DUMMY4WORD(SDIO_EMMC_BOOT_MODE)
} SDIO_EMMC_BOOT_SPEED_MODE;

typedef enum {
	SDIO_EMMC_BOOT_CLASS6_WP = 0,
	SDIO_EMMC_BOOT_CLASS6_RESERVED = 2,

	ENUM_DUMMY4WORD(SDIO_EMMC_BOOT_CLASS6)
} SDIO_EMMC_BOOT_CLASS6;

typedef enum {
	SDIO_EMMC_USER_WP_PWR_WP_EN = 0,

	ENUM_DUMMY4WORD(SDIO_EMMC_USER_WP)
} SDIO_EMMC_USER_WP;

/**
    EMMC BOOT cmd low or alternativeconfiguration

    @note For sdio_setConfig(SDIO_CONFIG_EMMC_BOOT_ALT, XXX)
*/
typedef enum {
	SDIO_EMMC_BOOT_CMDLOW = 0,                ///< EMMC BOOT CMD LOW
	SDIO_EMMC_BOOT_ALT,                       ///< EMMC BOOT ALTERNATIVE

	ENUM_DUMMY4WORD(SDIO_EMMC_BOOT_ALT_SEL)
} SDIO_EMMC_BOOT_ALT_SEL;

/**
    EMMC BOOT speed mode configuration

    @note For sdio_setConfig(SDIO_CONFIG_EMMC_BOOT_CLK, XXX)
*/
typedef enum {
	SDIO_EMMC_BOOT_CLKALWAYS = 0,            ///< EMMC BOOT CLK always output
	SDIO_EMMC_BOOT_CLKPAUSE,                 ///< EMMC BOOT CLK pause when dma transfer end

	ENUM_DUMMY4WORD(SDIO_EMMC_BOOT_CLK_SEL)
} SDIO_EMMC_BOOT_CLK_SEL;


/**
    SDIO IO device CMD52 request structure.

    Structure definition for IO device CMD52 operation.

    For more detail descriptions, please refer to SDIO spec.
*/
typedef struct {
	UINT8   *pData;                         ///< pointer to read/write buffer address
	UINT32  Reg_Addr;                       ///< register address in SDIO IO device
	UINT8   RAW;                            ///< Read After Write flag
	///< - @b SDIO_IO_DIS_RAW: disable RAW function
	///< - @b SDIO_IO_EN_RAW: enable RAW function
	UINT8   Function_Num;                   ///< function number in SDIO IO device
	///< - @b SDIO_IO_FUNNUM0: function 0 in SDIO IO device
	///< - @b SDIO_IO_FUNNUM1: function 1 in SDIO IO device
	///< - @b SDIO_IO_FUNNUM2: function 2 in SDIO IO device
	///< - @b SDIO_IO_FUNNUM3: function 3 in SDIO IO device
	///< - @b SDIO_IO_FUNNUM4: function 4 in SDIO IO device
	///< - @b SDIO_IO_FUNNUM5: function 5 in SDIO IO device
	///< - @b SDIO_IO_FUNNUM6: function 6 in SDIO IO device
	///< - @b SDIO_IO_FUNNUM7: function 7 in SDIO IO device
	UINT8   RW_flag;                        ///< R/W flag
	///< - @b SDIO_IO_R: IO read
	///< - @b SDIO_IO_W: IO write
	UINT16  ByteCount;                      ///< Transfer byte count
	UINT16  Stuff;                          ///< Write Data/Stuff bits
} SDIO_CMD52_STRUCT, *PSDIO_CMD52_STRUCT;

/**
    SDIO IO device CMD53 request structure.

    Structure definition for IO device CMD53 operation.

    For more detail descriptions, please refer to SDIO spec.
*/
typedef struct {
	UINT8   *pData;                         ///< pointer to read/write buffer address
	UINT32  Reg_Addr;                       ///< register address in SDIO IO device
	UINT8   OpCode;                         ///< IO OP code:
	///< - @b SDIO_IO_OPCODE_FIX_ADDR: R/W to fixed address
	///< - @b SDIO_IO_OPCODE_INCREMENT_ADDR: R/W to incrementing address
	UINT8   Block;                          ///< Block/Byte IO mode
	///< - @b SDIO_IO_BYTEMODE: byte access mode
	///< - @b SDIO_IO_BLOCKMODE: block access mode
	UINT8   Function_Num;                   ///< function number in SDIO IO device
	///< - @b SDIO_IO_FUNNUM0: function 0 in SDIO IO device
	///< - @b SDIO_IO_FUNNUM1: function 1 in SDIO IO device
	///< - @b SDIO_IO_FUNNUM2: function 2 in SDIO IO device
	///< - @b SDIO_IO_FUNNUM3: function 3 in SDIO IO device
	///< - @b SDIO_IO_FUNNUM4: function 4 in SDIO IO device
	///< - @b SDIO_IO_FUNNUM5: function 5 in SDIO IO device
	///< - @b SDIO_IO_FUNNUM6: function 6 in SDIO IO device
	///< - @b SDIO_IO_FUNNUM7: function 7 in SDIO IO device
	UINT8   RW_flag;                        ///< R/W flag
	///< - @b SDIO_IO_R: IO read
	///< - @b SDIO_IO_W: IO write
	UINT16  ByteCount;                      ///< Transfer byte count
	UINT8   Stuff;                          ///< Write Data/Stuff bits
} SDIO_CMD53_STRUCT, *PSDIO_CMD53_STRUCT;

/**
    SD card information structure

    @note for sdio_getMiscInfo(), sdio2_getMiscInfo(), sdio3_getMiscInfo().
*/
typedef struct {
	SDIO_CARD_TYPE  cardType;               ///< card type
	UINT16          uiVersion;              ///< SD/MMC spec version
	UINT32          uiWriteRate;            ///< Write performance. unit: MB/s
	SD_CID_STRUCT   sdCID;                  ///< SD CID (valid when cardType = SDIO_CARD_SD or SDIO_CARD_SDHC or SDIO_CARD_SDXC)
	MMC_CID_STRUCT  mmcCID;                 ///< MMC CID (valid when cardType = SDIO_CARD_MMC)
} SDIO_MISC_INFORMATION, *PSDIO_MISC_INFORMATION;

//------------------------------------------------------------
// The general api for the SDMMC device driver
//------------------------------------------------------------

/*
    Public OS level functions
*/
/*
// Functions exported from SDIO
extern void                 sdio_init(void);
extern ER                   sdio_open(void);
extern ER                   sdio_close(void);
extern BOOL                 sdio_isOpened(void);
extern ER                   sdio_lock(void);
extern ER                   sdio_unlock(void);

// Functions exported from SDIO2
extern void                 sdio2_init(void);
extern ER                   sdio2_open(void);
extern ER                   sdio2_close(void);
extern BOOL                 sdio2_isOpened(void);
extern ER                   sdio2_lock(void);
extern ER                   sdio2_unlock(void);
*/

/*
    Public driver specific operating functions
*/

//#NT#2010/06/14#HH Chuang -begin
//#NT#merge single call back install API
/**
    @name Backward compatible APIs

    For old version SDIO driver
*/
//@{
#define sdio_setDetectCardExistHdl(pHdl)    sdio_setCallBack(SDIO_CALLBACK_CARD_DETECT, (SDIO_CALLBACK_HDL)pHdl)
#define sdio_setDetectCardProtectHdl(pHdl)  sdio_setCallBack(SDIO_CALLBACK_WRITE_PROTECT, (SDIO_CALLBACK_HDL)pHdl)
#define sdio_setCardIOHdl(pHdl)             sdio_setCallBack(SDIO_CALLBACK_IO_PAD, (SDIO_CALLBACK_HDL)pHdl)
#define sdio_setCardPowerHdl(pHdl)          sdio_setCallBack(SDIO_CALLBACK_POWER_CONTROL, (SDIO_CALLBACK_HDL)pHdl)
#define sdio2_setDetectCardExistHdl(pHdl)   sdio2_setCallBack(SDIO_CALLBACK_CARD_DETECT, (SDIO_CALLBACK_HDL)pHdl)
#define sdio2_setDetectCardProtectHdl(pHdl) sdio2_setCallBack(SDIO_CALLBACK_WRITE_PROTECT, (SDIO_CALLBACK_HDL)pHdl)
#define sdio2_setCardIOHdl(pHdl)            sdio2_setCallBack(SDIO_CALLBACK_IO_PAD, (SDIO_CALLBACK_HDL)pHdl)
#define sdio2_setCardPowerHdl(pHdl)         sdio2_setCallBack(SDIO_CALLBACK_POWER_CONTROL, (SDIO_CALLBACK_HDL)pHdl)
#define sdio3_setDetectCardExistHdl(pHdl)   sdio3_setCallBack(SDIO_CALLBACK_CARD_DETECT, (SDIO_CALLBACK_HDL)pHdl)
#define sdio3_setDetectCardProtectHdl(pHdl) sdio3_setCallBack(SDIO_CALLBACK_WRITE_PROTECT, (SDIO_CALLBACK_HDL)pHdl)
#define sdio3_setCardIOHdl(pHdl)            sdio3_setCallBack(SDIO_CALLBACK_IO_PAD, (SDIO_CALLBACK_HDL)pHdl)
#define sdio3_setCardPowerHdl(pHdl)         sdio3_setCallBack(SDIO_CALLBACK_POWER_CONTROL, (SDIO_CALLBACK_HDL)pHdl)

//@}
//#NT#2010/06/14#HH Chuang -end

// Functions exported from SDIO
#if(NEW_STORAGE_OBJECT  == ENABLE)
extern PSTRG_TAB                sdio_getStorageObject(STRG_OBJ_ID objID);
#else
extern PSTRG_TAB                sdio_getStorageObject(void);
#endif
extern UINT32                   sdio_getCardExist(void);
extern UINT32                   sdio_getCardWriteProt(void);
extern ER                       sdio_setConfig(SDIO_CONFIG_ID configID, UINT32 configContext);
extern ER                       sdio_setInfo(UINT32 *pInfo);

extern void                     sdio_printCardInformation(void);
extern PSDMMC_INFO_STRUCT       sdio_getCardInformation(void);
extern PSDIO_MISC_INFORMATION   sdio_getMiscInfo(void);
extern ER                       sdio_setCallBack(SDIO_CALLBACK callBackID, SDIO_CALLBACK_HDL pCallBack);
extern void                     sdio_setIOEnable(BOOL IOEn);
extern ER                       sdio_sendIOCMD52(PSDIO_CMD52_STRUCT pSDIO_CMD52);
extern ER                       sdio_sendIOCMD53(PSDIO_CMD53_STRUCT pSDIO_CMD53);

//extern PMMC_EXT_CSD_STRUCT      sdio_getExtCSD(void);

// Functions exported from SDIO2
#if (NEW_STORAGE_OBJECT  == ENABLE)
extern PSTORAGE_OBJ             sdio2_getStorageObject(STRG_OBJ_ID objID);
#else
extern PSTRG_TAB                sdio2_getStorageObject(void);
#endif
extern UINT32                   sdio2_getCardExist(void);
extern UINT32                   sdio2_getCardWriteProt(void);
extern ER                       sdio2_setConfig(SDIO_CONFIG_ID configID, UINT32 configContext);
extern ER                       sdio2_setInfo(UINT32 *pInfo);

extern void                     sdio2_printCardInformation(void);
extern PSDMMC_INFO_STRUCT       sdio2_getCardInformation(void);
extern PSDIO_MISC_INFORMATION   sdio2_getMiscInfo(void);
extern ER                       sdio2_setCallBack(SDIO_CALLBACK callBackID, SDIO_CALLBACK_HDL pCallBack);
extern void                     sdio2_setIOEnable(BOOL IOEn);
extern ER                       sdio2_sendIOCMD52(PSDIO_CMD52_STRUCT pSDIO_CMD52);
extern ER                       sdio2_sendIOCMD53(PSDIO_CMD53_STRUCT pSDIO_CMD53);

//extern PMMC_EXT_CSD_STRUCT      sdio2_getExtCSD(void);

// Functions exported from SDIO3
#if (NEW_STORAGE_OBJECT  == ENABLE)
extern PSTORAGE_OBJ             sdio3_getStorageObject(STRG_OBJ_ID objID);
#else
extern PSTRG_TAB                sdio3_getStorageObject(void);
#endif
extern UINT32                   sdio3_getCardExist(void);
extern UINT32                   sdio3_getCardWriteProt(void);
extern ER                       sdio3_setConfig(SDIO_CONFIG_ID configID, UINT32 configContext);
extern ER                       sdio3_setInfo(UINT32 *pInfo);

extern void                     sdio3_printCardInformation(void);
extern PSDMMC_INFO_STRUCT       sdio3_getCardInformation(void);
extern PSDIO_MISC_INFORMATION   sdio3_getMiscInfo(void);
extern ER                       sdio3_setCallBack(SDIO_CALLBACK callBackID, SDIO_CALLBACK_HDL pCallBack);
extern void                     sdio3_setIOEnable(BOOL IOEn);
extern ER                       sdio3_sendIOCMD52(PSDIO_CMD52_STRUCT pSDIO_CMD52);
extern ER                       sdio3_sendIOCMD53(PSDIO_CMD53_STRUCT pSDIO_CMD53);

//extern PMMC_EXT_CSD_STRUCT      sdio3_getExtCSD(void);

extern ER                       sdio3_send_power_on_wp(UINT32 curr_sector, UINT32 num_sector);
extern UINT8                    sdio3_get_wp_status(UINT32 addr);

extern void                     sdio_platform_init(void);
extern void                     sdio_platform_uninit(void);

//@}

#endif // _SDIO_H




