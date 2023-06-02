#ifndef _DAI_PLATFORM_H
#define _DAI_PLATFORM_H

#define DAI_REG_NUM             1


extern void _dai_enableclk_platform(void);
extern void _dai_disableclk_platform(void);
extern void _dai_setclkrate_platform(unsigned long clkrate);

#endif
