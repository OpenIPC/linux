/*
    @file       sdp_int.h

    @ingroup    mIDrvIO_SDP

    @brief      SDP internal header file

    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.

*/

#ifndef _SDP_INT_H
#define _SDP_INT_H

//#include <stdlib.h>
#include <kwrap/type.h>
#include <kwrap/nvt_type.h>
#include <kwrap/semaphore.h>
#include <kwrap/flag.h>
#include <kwrap/spinlock.h>
#include <kwrap/task.h>
#include <kwrap/cpu.h>
#include <io_address.h>
//#include "plat/interrupt.h"
//#include "pll.h"
#include "pll_protected.h"

//#include "DrvCommon.h"
//#include "top.h"
#include "sdp_reg.h"

#define DRV_SUPPORT_IST             (ENABLE)
#define _EMULATION_ON_CPU2_	        (0)

/*
#ifdef _NVT_EMULATION_
#define _EMULATION_                 (ENABLE)
#else
#define _EMULATION_                 (DISABLE)
#endif

#ifdef _NVT_FPGA_
#define _FPGA_EMULATION_            (ENABLE)
#else
#define _FPGA_EMULATION_            (DISABLE)
#endif
*/
//#define SDP_SETREG(ofs,value)  OUTW(IOADDR_SDP_REG_BASE+(ofs),(value))
//#define SDP_GETREG(ofs)        INW(IOADDR_SDP_REG_BASE+(ofs))


/*
    SDP register default value

    SDP register default value.
*/
typedef struct _SDP_REG_DEFAULT {
	UINT32  offset;
	UINT32  value;
	CHAR    *p_name;
} SDP_REG_DEFAULT;

#endif
