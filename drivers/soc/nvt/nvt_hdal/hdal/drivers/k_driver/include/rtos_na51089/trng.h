/*
    Trng driver header file

    Trng driver header file.

    @file       trng.h
    @ingroup    mIDrvMisc_Trng
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.
*/

#ifndef _TRNG_H
#define _TRNG_H

#include <kwrap/nvt_type.h>

/**
    SEED
*/
typedef enum {
	TRNG_SEED_LOW,           ///< TRNG SEED LOW
	TRNG_SEED_HIGH,          ///< TRNG SEED HIGH

	ENUM_DUMMY4WORD(TRNG_SEED)
} TRNG_SEED;

typedef enum {

	TRNG_CONFIG_CLKSRC,
	///< Context can be one of the following:
	///< - @b 160   : 160 MHz (Default value)
	///< - @b 240   : 240 MHz
	TRNG_CONFIG_ROSC_CLKSRC,
	TRNG_CONFIG_RO_DELAY,
	TRNG_CONFIG_RO_CLKDIV,
    TRNG_CONFIG_CLKDIV,

	ENUM_DUMMY4WORD(TRNG_CONFIG_ID)
} TRNG_CONFIG_ID;

typedef enum {

	TRNGRO_CLKSRC_RO,
	TRNGRO_CLKSRC_PLL4,

	ENUM_DUMMY4WORD(TRNGRO_CLKSRC)
} TRNGRO_CLKSRC;

extern ER trng_open(void);
extern ER trng_close(void);
extern ER trng_setConfig(TRNG_CONFIG_ID ConfigID, UINT32 uiConfig);
extern void trng_set_casr(BOOL b_en);
extern void trng_set_postproc(BOOL b_en);
extern void trng_set_lock(BOOL b_lock);
extern void trng_update_seed(BOOL b_update);
extern void trng_set_seed(TRNG_SEED seedhl, UINT32 data);
extern UINT32 trng_get_Control(void);
extern UINT32 trng_get_seed(TRNG_SEED seedhl);
extern BOOL trng_get_tdes_status(void);
extern UINT32 trng_get_randnum(void);
extern UINT32 randomUINT32(void);


#if defined(_NVT_EMULATION_)
BOOL trng_resetvalue_check(void);
#endif


//@}

#endif // _TRNG_H



