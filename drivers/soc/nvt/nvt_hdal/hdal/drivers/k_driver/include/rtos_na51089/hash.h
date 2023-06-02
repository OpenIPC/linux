/*
    HASH driver header file

    HASH driver header file.

    @file       hash.h
    @ingroup    mIDrvMisc_HASH
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2020.  All rights reserved.
*/

#ifndef _HASH_H
#define _HASH_H

#include <kwrap/nvt_type.h>

/**
    @addtogroup mIDrvMisc_HASH
*/
//@{
/**
    Hash engine clock rate
*/
typedef enum
{
    HASH_CLOCK_240MHz = 0x00,        ///< Select HASH clock rate 240 MHz
    HASH_CLOCK_320MHz,               ///< Select HASH clock rate 320 MHz
    HASH_CLOCK_Reserved,             ///< Select HASH clock rate Reserved
    HASH_CLOCK_PLL9,                 ///< Select HASH clock rate PLL9(for IDE/ETH)

    HASH_CLOCK_RATE_NUM,
    ENUM_DUMMY4WORD(HASH_CLOCK_RATE)
} HASH_CLOCK_RATE;


/**
    Hash engine Algorithm
*/
typedef enum
{
    HASH_MODE_SHA1 = 0x00,            ///< Select HASH mode SHA1
    HASH_MODE_SHA256,                 ///< Select HASH mode SHA256
    HASH_MODE_HMAC_SHA1,              ///< Select HASH mode HMAC_SHA1
    HASH_MODE_HMAC_SHA256,            ///< Select HASH mode HMAC_SHA256

    HASH_MODE_NUM,
    ENUM_DUMMY4WORD(HASH_MODE)
} HASH_MODE;


/**
    Hash engine Select Iniatial Vector source
*/
typedef enum
{
    HASH_IV_Default=0,                ///< Set HASH IV with default value of algorithm.
    HASH_IV_REG,                      ///< Configured by using register HASH_IV_0 to HASH_IV_7

    HASH_IV_NUM,
    ENUM_DUMMY4WORD(HASH_IV)
} HASH_IV;


/**
    Hash engine Access Mode
*/
typedef enum
{
    HASH_ACCMODE_PIO=0,               ///< Select HASH Access mode PIO
    HASH_ACCMODE_DMA,                 ///< Select HASH Access mode DMA

    HASH_ACCMODE_NUM,
    ENUM_DUMMY4WORD(HASH_ACCMODE)
} HASH_ACCMODE;


/**
    Hash engine Access Mode
*/
typedef enum
{
    HASH_DMA_PAD_DISABLE=0,               ///< Select HASH DMA mode Padding Disable
    HASH_DMA_PAD_ENABLE,                  ///< Select HASH DMA mode Padding Enable

    HASH_DMA_PAD_NUM,
    ENUM_DUMMY4WORD(HASH_DMA_PAD)
} HASH_DMA_PAD;

/**
    Hash funciotn bypass or not at DMA mode.
*/
typedef enum
{
    HASH_BYPASS_NORMAL=0,             ///< Set HASH engine as normal mode
    HASH_BYPASS_HwCopy,               ///< Set HASH engine as HwCopy

    HASH_BYPASS_NUM,
    ENUM_DUMMY4WORD(HASH_BYPASS)
} HASH_BYPASS;


/**
    Set Hash funciotn as initail state.
*/
typedef enum
{
    HASH_INI_NORMAL=0,                ///< Set HASH engine as normal state
    HASH_INI_STATE,                   ///< Set HASH engine as initail state

    HASH_INI_NUM,
    ENUM_DUMMY4WORD(HASH_INI)
} HASH_INI;


/**
    Set Hash funciotn as internal padding state.
*/
typedef enum
{
    HASH_IPAD_NORMAL=0,              ///< Set HASH engine as normal state
    HASH_IPAD_STATE,                 ///< Set HASH engine as internal padding state

    HASH_IPAD_NUM,
    ENUM_DUMMY4WORD(HASH_IPAD)
} HASH_IPAD;


/**
    Set Hash funciotn as external padding state.
*/
typedef enum
{
    HASH_OPAD_NORMAL=0,              ///< Set HASH engine as normal state
    HASH_OPAD_STATE,                  ///< Set HASH engine as internal padding state

    HASH_OPAD_NUM,
    ENUM_DUMMY4WORD(HASH_OPAD)
} HASH_OPAD;


/**
    Hash configuration identifier

    @note For hash_setConfig()
*/
typedef enum
{
    HASH_CONFIG_ID_CLOCK_RATE,          ///< Select HASH engine internal clock rate usage
                                        ///< Context can be any of:
                                        ///< - @b HASH_CLOCK_240MHz:     Internal clock rate 240MHz
                                        ///< - @b HASH_CLOCK_320MHz:     Internal clock rate 320MHz
                                        ///< - @b HASH_CLOCK_PLL9MHz:    Internal clock rate 280MHz
    HASH_CONFIG_ID_MODE,                ///< Select HASH engine Algorithm
                                        ///< Context can be any of:
                                        ///< - @b HASH_MODE_SHA1:        Select HASH mode SHA1
                                        ///< - @b HASH_MODE_SHA256:      Select HASH mode SHA256
                                        ///< - @b HASH_MODE_HMAC_SHA1:   Select HASH mode HMAC_SHA1
                                        ///< - @b HASH_MODE_HMAC_SHA256: Select HASH mode HMAC_SHA256
                                        ///< - @b HASH_MODE_HwCopy:      Select Bypass HASH function
    HASH_CONFIG_ID_IV_SEL,              ///< Select HASH engine Iniatial Vector source
                                        ///< Context can be any of:
                                        ///< - @b HASH_IV_Defaut:        Set HASH IV with default value of algorithm.
                                        ///< - @b HASH_IV_REG:           Configured by using register HASH_IV_0 to HASH_IV_7
    HASH_CONFIG_ID_ACCMODE,             ///< Select HASH engine Access Mode
                                        ///< Context can be any of:
                                        ///< - @b HASH_ACCMODE_PIO:      Select HASH Access mode PIO
                                        ///< - @b HASH_ACCMODE_DMA:      Select HASH Access mode DMA
    HASH_CONFIG_ID_DMA_PAD,             ///< Select HASH engine DMA mode Padding Enable
                                        ///< Context can be any of:
                                        ///< - @b HASH_DMA_PAD_DISABLE:  Select HASH DMA mode Padding Disable
                                        ///< - @b HASH_DMA_PAD_ENABLE:   Select HASH DMA mode Padding Enable
    HASH_CONFIG_ID_BYPASS,              ///< Select HASH engine DMA mode bypass or not for HwCopy.
                                        ///< Context can be any of:
                                        ///< - @b HASH_BYPASS_NORMAL:    Set HASH engine as normal mode
                                        ///< - @b HASH_BYPASS_HwCopy:    Set HASH engine as HwCopy
    HASH_CONFIG_ID_INI,                 ///< Set HASH engine DMA mode as initail state.
                                        ///< Context can be any of:
                                        ///< - @b HASH_INI_NORMAL:       Set HASH engine as normal state
                                        ///< - @b HASH_INI_STATE:        Set HASH engine as initail state
    HASH_CONFIG_ID_IPAD,                ///< Set HASH engine DMA mode as internal padding state.
                                        ///< Context can be any of:
                                        ///< - @b HASH_IPAD_NORMAL:      Set HASH engine as normal state
                                        ///< - @b HASH_IPAD_STATE:       Set HASH engine as internal padding state
    HASH_CONFIG_ID_OPAD,                ///< Set HASH engine DMA mode as external padding state.
                                        ///< Context can be any of:
                                        ///< - @b HASH_OPAD_NORMAL:      Set HASH engine as normal state
                                        ///< - @b HASH_OPAD_STATE:       Set HASH engine as external padding state
    ENUM_DUMMY4WORD(HASH_CONFIG_ID)
} HASH_CONFIG_ID;


extern ER   hash_setConfig(HASH_CONFIG_ID cfgid, UINT32 cfgvalue);
extern ER   hash_open(void);
extern ER   hash_close(void);
extern void hash_pio_enable(UINT8* ucInput, UINT32 len, BOOL bmac);
extern void hash_setKey(UINT8 * ucKey, UINT32 uiLen);
extern void hash_setIV(UINT8 * ucIV);
extern void hash_getOutput(UINT8 * Output, UINT32 len);
extern void hash_dma_enable(UINT32 addr, UINT32 size, UINT32 pad_len);
extern void hash_dma_HwCopy(UINT32 Srcaddr, UINT32 Dstaddr, UINT32 size);
extern BOOL hashpadding(UINT8* ucInput, UINT32 len);

#if defined(_NVT_FPGA_) || defined(_NVT_EMULATION_)
extern void hash_dma_HwCopy_without_flush(UINT32 Srcaddr, UINT32 Dstaddr, UINT32 size);
extern void hash_dma_enable_random_terminate(UINT32 addr, UINT32 size, UINT32 pad_len);
#endif

//@}

#endif /* _HASH_H */
