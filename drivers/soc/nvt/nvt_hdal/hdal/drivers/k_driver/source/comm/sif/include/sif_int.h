/*
    SIF module internal header file

    SIF module internal header file

    @file       sif_int.h
    @ingroup    mIDrvIO_SIF
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/

#ifndef _SIF_INT_H
#define _SIF_INT_H

#if defined __UITRON || defined __ECOS
#include "DrvCommon.h"
#elif defined __KERNEL__

#define CHKPNT                          printk(KERN_INFO  "\033[37mCHK: %d, %s\033[0m\r\n",__LINE__,__func__)

#define FLGPTN_SIF0             FLGPTN_BIT(0)
#define FLGPTN_SIF1             FLGPTN_BIT(1)
#define FLGPTN_SIF2             FLGPTN_BIT(2)
#define FLGPTN_SIF3             FLGPTN_BIT(3)
#define FLGPTN_SIFDMA0          FLGPTN_BIT(4)
#define FLGPTN_SIFDMA1          FLGPTN_BIT(5)
#define FLGPTN_SIFEMPTY0        FLGPTN_BIT(6)
#define FLGPTN_SIFEMPTY1        FLGPTN_BIT(7)
#define FLGPTN_SIFEMPTY2        FLGPTN_BIT(8)
#define FLGPTN_SIFEMPTY3        FLGPTN_BIT(9)
#endif

/**
    @addtogroup mIDrvIO_SIF
*/
//@{

#define SIF_FLGPTN_END_ALL    (FLGPTN_SIF0 | FLGPTN_SIF1 | FLGPTN_SIF2 | FLGPTN_SIF3 | \
							   FLGPTN_SIFDMA0 | FLGPTN_SIFDMA1)

// SIF INTERRUPT STATUS bit definition
#define SIF_INT_DMA(ch)             (0x01000000 << (ch))
#define SIF_INT_END(ch)             (0x00000010 << (ch))
#define SIF_INT_EMPTY(ch)           (0x00000001 << (ch))

// SIF SEN duration
#define SIF_SEND_MIN                1
#define SIF_SEND_MAX                16

// SIF SEN setup time
#define SIF_SENS_MIN                0
#define SIF_SENS_MAX                15

// SIF SEN hold time
#define SIF_SENH_MIN                0
#define SIF_SENH_MAX                7

// SIF transmission length
#define SIF_TRANLEN_MIN             1
#define SIF_TRANLEN_MAX             255
#define SIF_TRANLEN_DEFAULT         1

// SIF continuous mode address bits
#define SIF_CONTADDRBITS_MIN        1
#define SIF_CONTADDRBITS_MAX        255
#define SIF_CONTADDRBITS_DEFAULT    1

// SIF delay between successive transmission
#define SIF_DELAY_MIN               0
#define SIF_DELAY_MAX               255

// SIF clock divider
#define SIF_DIVIDER_MIN             0
#define SIF_DIVIDER_MAX             255

// Maximum data reigster bits
#define SIF_MAX_DATAREG_BITS        96      // 3 data registers, 32 bits per register

// SIF source clock
#if (_FPGA_EMULATION_ == ENABLE)
// (OSC * 2) * 96 / 480 --> OSC * 2 / 5
#define SIF_SOURCE_CLOCK            ((_FPGA_PLL_OSC_ << 1) / 5)
#else
#define SIF_SOURCE_CLOCK            96000000
#endif

// Data port selection
#define SIF_DPSEL_CH0               0
#define SIF_DPSEL_CH1               1
#define SIF_DPSEL_CH2               2
#define SIF_DPSEL_CH3               3


// SIF CONFIG bit definition
#define SIF_CONF_START_CH(ch)       (0x00000001 << (ch))
#define SIF_CONF_RESTART_CH(ch)     (0x00010000 << (ch))

// SIF Status bit definition
#define SIF_STATUS_CH(ch)           (0x00000001 << (ch))

// SIF DMA size
#define SIF_DMA_SIZE_MIN            0x4
#define SIF_DMA_SIZE_MAX            0xFFFC

// SIF maximum burst delay in ns
#define SIF_UI_BURST_DELAY_MAX      100000000

// SIF maximum burst delay
#define SIF_BURST_DELAY_MAX         0x927C00

// Register default value
#define SIF_MODE_REG_DEFAULT            0x00000000
#define SIF_CONF0_REG_DEFAULT           0x00000000
#define SIF_CONF1_REG_DEFAULT           0x00000000
#define SIF_CONF2_REG_DEFAULT           0x00000000
#define SIF_CLKCTRL0_REG_DEFAULT        0x00000000
#define SIF_CLKCTRL1_REG_DEFAULT        0x00000000
#define SIF_TXSIZE0_REG_DEFAULT         0x00000000
#define SIF_TXSIZE1_REG_DEFAULT         0x00000000
#define SIF_STS_REG_DEFAULT             0x00000000
#define SIF_DATA0_REG_DEFAULT           0x00000000
#define SIF_DATA1_REG_DEFAULT           0x00000000
#define SIF_DATA2_REG_DEFAULT           0x00000000
#define SIF_DATASEL_REG_DEFAULT         0x00000000
#define SIF_DELAY0_REG_DEFAULT          0x00000000
#define SIF_DELAY1_REG_DEFAULT          0x00000000
#define SIF_TIMING0_REG_DEFAULT         0x00000000
#define SIF_TIMING1_REG_DEFAULT         0x00000000
#define SIF_TIMING2_REG_DEFAULT         0x00000000
#define SIF_TIMING3_REG_DEFAULT         0x00000000
#define SIF_DMA0_CTRL0_REG_DEFAULT      0x00000000
#define SIF_DMA0_CTRL1_REG_DEFAULT      0x00000000
#define SIF_DMA0_CTRL2_REG_DEFAULT      0x00000000
#define SIF_DMA0_CTRL3_REG_DEFAULT      0x00000000
#define SIF_DMA0_STARTADDR_REG_DEFAULT  0x00000000
#define SIF_DMA1_CTRL0_REG_DEFAULT      0x00000000
#define SIF_DMA1_CTRL1_REG_DEFAULT      0x00000000
#define SIF_DMA1_CTRL2_REG_DEFAULT      0x00000000
#define SIF_DMA1_CTRL3_REG_DEFAULT      0x00000000
#define SIF_DMA1_STARTADDR_REG_DEFAULT  0x00000000
#define SIF_DMA2_CTRL0_REG_DEFAULT      0x00000000
#define SIF_DMA2_CTRL1_REG_DEFAULT      0x00000000
#define SIF_DMA2_CTRL2_REG_DEFAULT      0x00000000
#define SIF_DMA2_CTRL3_REG_DEFAULT      0x00000000
#define SIF_DMA2_STARTADDR_REG_DEFAULT  0x00000000
#define SIF_DMA3_CTRL0_REG_DEFAULT      0x00000000
#define SIF_DMA3_CTRL1_REG_DEFAULT      0x00000000
#define SIF_DMA3_CTRL2_REG_DEFAULT      0x00000000
#define SIF_DMA3_CTRL3_REG_DEFAULT      0x00000000
#define SIF_DMA3_STARTADDR_REG_DEFAULT  0x00000000
#define SIF_DMA4_CTRL0_REG_DEFAULT      0x00000000
#define SIF_DMA4_CTRL1_REG_DEFAULT      0x00000000
#define SIF_DMA4_CTRL2_REG_DEFAULT      0x00000000
#define SIF_DMA4_CTRL3_REG_DEFAULT      0x00000000
#define SIF_DMA4_STARTADDR_REG_DEFAULT  0x00000000
#define SIF_DST0_REG_DEFAULT            0x00000000
#define SIF_DST1_REG_DEFAULT            0x00000000
#define SIF_INTEN0_REG_DEFAULT          0x00000000
#define SIF_INTEN1_REG_DEFAULT          0x00000000
#define SIF_INTSTS0_REG_DEFAULT         0x00000000
#define SIF_INTSTS1_REG_DEFAULT         0x00000000

/*
    SIF register default value

    SIF register default value.
*/
typedef struct {
	UINT32  uiOffset;
	UINT32  uiValue;
	CHAR    *pName;
} SIF_REG_DEFAULT;

//@}

#endif
