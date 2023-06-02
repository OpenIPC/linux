/*
    Embedded Audio Codec Driver Internal Header File

    This file is the internal header file for Embedded Audio Codec.

    @file       eac_int.h
    @ingroup    mIDrvAud_EAC
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#ifndef _EAC_INT_H
#define _EAC_INT_H

#ifdef __KERNEL__
//#include "eac_dbg.h"
//#include "eac_drv.h"

extern UINT32 _EAC_REG_BASE_ADDR[1];
#define EAC_SETREG(ofs, value)      iowrite32(value, (void *)(_EAC_REG_BASE_ADDR[0] + (ofs)))
#define EAC_GETREG(ofs)             ioread32((void *)(_EAC_REG_BASE_ADDR[0] + (ofs)))
#elif defined(__FREERTOS)
#define EAC_SETREG(ofs, value)      OUTW(IOADDR_EAC_REG_BASE+(ofs), (value))
#define EAC_GETREG(ofs)             INW(IOADDR_EAC_REG_BASE+(ofs))
#endif

//ADC Volume control
#define EAC_ADC_DGAIN_MAX           0xFF

//ALC control
#define EAC_ALC_ATTACK_MAX          10
#define EAC_ALC_DECAY_MAX           10
#define EAC_ALC_HOLD_MAX            15
#define EAC_ALC_STEP_MAX            31
#define EAC_ALC_TIME_RESO_MAX       0x7FFFFF

//ADC Noise Gate Config
#define EAC_NG_FACTOR_MAX           0x0F

//ADC DCCAN init value
#define EAC_ADC_DCINIT_MASK         0xFFFF
#define EAC_ADC_DCINIT_R_OFS        16

//DAC Volume control
#define EAC_DAC_PGAIN_HP_MAX        0x1F
#define EAC_DAC_PGAIN_SPK_MAX       0x1F

#define EAC_DAC_DGAIN_MAX           0xFF

extern void eac_debug(BOOL b_en, BOOL b_mode_ad);
extern void eac_debug_type(UINT32 dac_type);
#endif
