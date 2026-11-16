#ifndef __CORE_BASE_H__
#define __CORE_BASE_H__

/* for xburst2 INTC base 0x12300000*/
#ifdef CONFIG_SOC_T41
#include <soc/base.h>
#else
#define	INTC_IOBASE	0x12300000
#endif
/* CCU IO base address */
//#define CCU_IO_BASE		(read_c0_config7 & 0xffff0000)
#define CCU_IO_BASE			0x12200000

/* core system ost interrupte timer. core system ost. xburst2 */
#define    CORE_OST_IOBASE	0x12100000
/* global system ost timer for jiffies. xburst2 */
#define    G_OST_IOBASE		0x12000000

#endif /* __CORE_BASE_H__ */
