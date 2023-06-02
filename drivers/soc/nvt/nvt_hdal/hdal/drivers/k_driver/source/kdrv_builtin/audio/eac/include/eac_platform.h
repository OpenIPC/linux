#ifndef _DAI_PLATFORM_H
#define _DAI_PLATFORM_H
#include "kwrap/type.h"

#define EAC_REG_NUM             1


extern void _eac_enableclk_platform(BOOL b_en);
extern void _eac_enableadcclk_platform(BOOL b_en);
extern void _eac_enabledacclk_platform(BOOL b_en);
extern void _eac_setdacclkrate_platform(unsigned long clkrate);

#endif
