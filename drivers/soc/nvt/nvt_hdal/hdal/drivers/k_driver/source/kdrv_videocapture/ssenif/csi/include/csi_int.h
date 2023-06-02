/*
    MIPI CSI Controller internal header

    MIPI CSI Controller internal header

    @file       csi_int.h
    @ingroup    mIDrvIO_CSI
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#ifndef _CSI_INT_H
#define _CSI_INT_H

#define _FPGA_EMULATION_ 0

#ifdef __KERNEL__
#include "csi_reg.h"
#include "csi_rule.h"
#include <mach/rcw_macro.h>
#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "csi_dbg.h"
#include "../../senphy.h"
#else
#if defined(__FREERTOS)
#include "csi_reg.h"
#include "csi_rule.h"
#include "../../senphy.h"
#include "kwrap/error_no.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"

//#include "rcw_macro.h"
#include "io_address.h"
#include "interrupt.h"
#include "pll.h"
#include "pll_protected.h"
//#include "../../../../../../../../lib/include/Utility/SwTimer.h"
#include "Utility/SwTimer.h"
#else
#include "DrvCommon.h"
#include "pll.h"
#include "pll_protected.h"
#include "../SENPHY/senphy_int.h"
#include "SwTimer.h"
#endif
#endif


#define CSI_FORCE_OFF               DISABLE
#define CSI_DEBUG                   DISABLE
#define CSI_ECO_ENABLE              DISABLE
#define CSI_ECO_HWRST               DISABLE
#define CSI_ENABLE_DESKEW           ENABLE

//
//  MIPI-CSI register access definition
//
#ifndef __KERNEL__
#define CSI_REG_ADDR(ofs)           (IOADDR_CSI_REG_BASE+(ofs))
#define CSI_GETREG(ofs)             INW(IOADDR_CSI_REG_BASE+(ofs))
#define CSI_SETREG(ofs, value)       OUTW(IOADDR_CSI_REG_BASE+(ofs), (value))

#define CSI2_REG_ADDR(ofs)          (IOADDR_CSI2_REG_BASE+(ofs))
#define CSI2_GETREG(ofs)            INW(IOADDR_CSI2_REG_BASE+(ofs))
#define CSI2_SETREG(ofs, value)      OUTW(IOADDR_CSI2_REG_BASE+(ofs), (value))

#define CSI3_REG_ADDR(ofs)          (IOADDR_CSI3_REG_BASE+(ofs))
#define CSI3_GETREG(ofs)            INW(IOADDR_CSI3_REG_BASE+(ofs))
#define CSI3_SETREG(ofs, value)      OUTW(IOADDR_CSI3_REG_BASE+(ofs), (value))

#define CSI4_REG_ADDR(ofs)          (IOADDR_CSI4_REG_BASE+(ofs))
#define CSI4_GETREG(ofs)            INW(IOADDR_CSI4_REG_BASE+(ofs))
#define CSI4_SETREG(ofs, value)      OUTW(IOADDR_CSI4_REG_BASE+(ofs), (value))

#define CSI5_REG_ADDR(ofs)          (IOADDR_CSI5_REG_BASE+(ofs))
#define CSI5_GETREG(ofs)            INW(IOADDR_CSI5_REG_BASE+(ofs))
#define CSI5_SETREG(ofs, value)      OUTW(IOADDR_CSI5_REG_BASE+(ofs), (value))

#define CSI6_REG_ADDR(ofs)          (IOADDR_CSI6_REG_BASE+(ofs))
#define CSI6_GETREG(ofs)            INW(IOADDR_CSI6_REG_BASE+(ofs))
#define CSI6_SETREG(ofs, value)      OUTW(IOADDR_CSI6_REG_BASE+(ofs), (value))

#define CSI7_REG_ADDR(ofs)          (IOADDR_CSI7_REG_BASE+(ofs))
#define CSI7_GETREG(ofs)            INW(IOADDR_CSI7_REG_BASE+(ofs))
#define CSI7_SETREG(ofs, value)      OUTW(IOADDR_CSI7_REG_BASE+(ofs), (value))

#define CSI8_REG_ADDR(ofs)          (IOADDR_CSI8_REG_BASE+(ofs))
#define CSI8_GETREG(ofs)            INW(IOADDR_CSI8_REG_BASE+(ofs))
#define CSI8_SETREG(ofs, value)      OUTW(IOADDR_CSI8_REG_BASE+(ofs), (value))

#define CSI_SIE_REG_ADDR(ofs)       (IOADDR_SIE_REG_BASE+(ofs))
#define CSI_SIE_GETREG(ofs)         INW(IOADDR_SIE_REG_BASE+(ofs))
#define CSI_SIE_SETREG(ofs, value)   OUTW(IOADDR_SIE_REG_BASE+(ofs), (value))
#define FLGPTN_CSI				FLGPTN_BIT(0)
#define FLGPTN_CSI2				FLGPTN_BIT(0)
#else
extern UINT32 _CSI_REG_BASE_ADDR;
#define CSI_REG_ADDR(ofs)           (_CSI_REG_BASE_ADDR+(ofs))
#define CSI_GETREG(ofs)             INW(_CSI_REG_BASE_ADDR+(ofs))
#define CSI_SETREG(ofs, value)       OUTW(_CSI_REG_BASE_ADDR+(ofs), (value))

extern UINT32 _CSI2_REG_BASE_ADDR;
#define CSI2_REG_ADDR(ofs)          (_CSI2_REG_BASE_ADDR+(ofs))
#define CSI2_GETREG(ofs)            INW(_CSI2_REG_BASE_ADDR+(ofs))
#define CSI2_SETREG(ofs, value)      OUTW(_CSI2_REG_BASE_ADDR+(ofs), (value))

extern UINT32 _CSI3_REG_BASE_ADDR;
#define CSI3_REG_ADDR(ofs)          (_CSI3_REG_BASE_ADDR+(ofs))
#define CSI3_GETREG(ofs)            INW(_CSI3_REG_BASE_ADDR+(ofs))
#define CSI3_SETREG(ofs, value)      OUTW(_CSI3_REG_BASE_ADDR+(ofs), (value))

extern UINT32 _CSI4_REG_BASE_ADDR;
#define CSI4_REG_ADDR(ofs)          (_CSI4_REG_BASE_ADDR+(ofs))
#define CSI4_GETREG(ofs)            INW(_CSI4_REG_BASE_ADDR+(ofs))
#define CSI4_SETREG(ofs, value)      OUTW(_CSI4_REG_BASE_ADDR+(ofs), (value))

extern UINT32 _CSI5_REG_BASE_ADDR;
#define CSI5_REG_ADDR(ofs)          (_CSI5_REG_BASE_ADDR+(ofs))
#define CSI5_GETREG(ofs)            INW(_CSI5_REG_BASE_ADDR+(ofs))
#define CSI5_SETREG(ofs, value)      OUTW(_CSI5_REG_BASE_ADDR+(ofs), (value))

extern UINT32 _CSI6_REG_BASE_ADDR;
#define CSI6_REG_ADDR(ofs)          (_CSI6_REG_BASE_ADDR+(ofs))
#define CSI6_GETREG(ofs)            INW(_CSI6_REG_BASE_ADDR+(ofs))
#define CSI6_SETREG(ofs, value)      OUTW(_CSI6_REG_BASE_ADDR+(ofs), (value))

extern UINT32 _CSI7_REG_BASE_ADDR;
#define CSI7_REG_ADDR(ofs)          (_CSI7_REG_BASE_ADDR+(ofs))
#define CSI7_GETREG(ofs)            INW(_CSI7_REG_BASE_ADDR+(ofs))
#define CSI7_SETREG(ofs, value)      OUTW(_CSI7_REG_BASE_ADDR+(ofs), (value))

extern UINT32 _CSI8_REG_BASE_ADDR;
#define CSI8_REG_ADDR(ofs)          (_CSI8_REG_BASE_ADDR+(ofs))
#define CSI8_GETREG(ofs)            INW(_CSI8_REG_BASE_ADDR+(ofs))
#define CSI8_SETREG(ofs, value)      OUTW(_CSI8_REG_BASE_ADDR+(ofs), (value))

extern UINT32 _SIE_CSI_REG_BASE_ADDR;
#define CSI_SIE_REG_ADDR(ofs)       (0xFDC00000+(ofs))
#define CSI_SIE_GETREG(ofs)         INW(0xFDC00000+(ofs))
#define CSI_SIE_SETREG(ofs, value)   OUTW(0xFDC00000+(ofs), (value))
#endif


#if _FPGA_EMULATION_
#define CSI_DEFAULT_INT0            (0)
#define CSI_DEFAULT_INT1            (0)
#else
#define CSI_DEFAULT_INT0            (0x40F0F203)
#define CSI_DEFAULT_INT1            (0x000000FF)
#define CSI_DEFAULT_INT0_FS      (0x00000040)
#endif


#if CSI_DEBUG
#define CSI_OPTION_MSG_PUT          DISABLE
#define CSI2_OPTION_MSG_PUT         DISABLE
#endif

#define CSI_INT_BANK0               0
#define CSI_INT_BANK1               1


// CSI Internal Usage Definition
#define CSI_SWAP_MSK                0x3
#define CSI_SWAP_MSK7               0x7
#define CSI_SWAP_DATALANE0_OFS      0
#define CSI_SWAP_DATALANE1_OFS      4
#define CSI_SWAP_DATALANE2_OFS      8
#define CSI_SWAP_DATALANE3_OFS      12

#define CSI_PINSWAP_OFS             16
#define CSI_LPSWAP_OFS             0

#define CSI_ANALOG_DLY_MSK          0x7

//void csi_install_cmd(void);
BOOL csi_print_info_to_uart(CHAR *strcmd);
BOOL csi2_print_info_to_uart(CHAR *strcmd);
BOOL csi3_print_info_to_uart(CHAR *strcmd);
BOOL csi4_print_info_to_uart(CHAR *strcmd);
BOOL csi5_print_info_to_uart(CHAR *strcmd);
BOOL csi6_print_info_to_uart(CHAR *strcmd);
BOOL csi7_print_info_to_uart(CHAR *strcmd);
BOOL csi8_print_info_to_uart(CHAR *strcmd);

void csi_error_parser(CSI_ID csi_id, UINT32 sts0, UINT32 sts1, UINT32 line_sta1);

#endif
