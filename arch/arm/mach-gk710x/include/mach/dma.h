/*!
*****************************************************************************
** \file        arch/arm/mach-gk7101/include/hal/dma.h
**
** \version
**
** \brief
**
** \attention   THIS SAMPLE CODE IS PROVIDED AS IS. GOKE MICROELECTRONICS
**              ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**              OMMISSIONS
**
** (C) Copyright 2012-2015 by GOKE MICROELECTRONICS CO.,LTD
**
*****************************************************************************
*/

#ifndef __HAL_DMA_H__
#define __HAL_DMA_H__
#include <mach/hardware.h>


#define DMA_BASE                (GK_VA_DMAC)
#define DMA_REG(x)              (DMA_BASE + (x))


/****************************************************/
/* Capabilities based on chip revision              */
/****************************************************/

#define DMA_SUPPORT_DMA_FIOS    0    /* DMA_FIOS */

/****************************/
/* DMA Channel Assignments  */
/****************************/

#if (DMA_SUPPORT_DMA_FIOS == 0)
/* DMA instance channel */
#define FIO_DMA_CHAN            0
#define I2S_RX_DMA_CHAN         1
#define I2S_TX_DMA_CHAN         2
#define HOST_RX_DMA_CHAN        3
#define HOST_TX_DMA_CHAN        4
#endif

#define MS_DMA_CHAN             3
/* No AHB MS controller */

/* Max number of channel */
#define NUM_DMA_FIOS_CHANNELS   DMA_SUPPORT_DMA_FIOS

#define NUM_DMA_CHANNELS        4

#if defined(CONFIG_GK_MUSB_CON_V1_00) 
#define DMA_CHAN_TX        3
#define DMA_CHAN_RX        3
#elif defined(CONFIG_GK_MUSB_CON_V1_10) 
#define DMA_CHAN_TX        0
#define DMA_CHAN_RX        3
#endif

/****************************************************/
/* Controller registers definitions                 */
/****************************************************/
#define DMA_CHAN_CTR_REG(x)		DMA_REG((0x300 + ((x) << 4)))
#define DMA_CHAN_SRC_REG(x)     DMA_REG((0x304 + ((x) << 4)))
#define DMA_CHAN_DST_REG(x)     DMA_REG((0x308 + ((x) << 4)))
#define DMA_CHAN_STA_REG(x)     DMA_REG((0x30C + ((x) << 4)))
#define DMA_CHAN_DA_REG(x)      DMA_REG((0x380 + ((x) << 2)))
#define DMA_INT_REG             DMA_REG(0x3F0)

/* DMA_CHANX_CTR_REG */
#define DMA_CHANX_CTR_EN        0x80000000
#define DMA_CHANX_CTR_D         0x40000000
#define DMA_CHANX_CTR_WM        0x20000000
#define DMA_CHANX_CTR_RM        0x10000000
#define DMA_CHANX_CTR_NI        0x08000000
#define DMA_CHANX_CTR_BLK_1024B 0x07000000
#define DMA_CHANX_CTR_BLK_512B  0x06000000
#define DMA_CHANX_CTR_BLK_256B  0x05000000
#define DMA_CHANX_CTR_BLK_128B  0x04000000
#define DMA_CHANX_CTR_BLK_64B   0x03000000
#define DMA_CHANX_CTR_BLK_32B   0x02000000
#define DMA_CHANX_CTR_BLK_16B   0x01000000
#define DMA_CHANX_CTR_BLK_8B    0x00000000
#define DMA_CHANX_CTR_TS_8B     0x00C00000
#define DMA_CHANX_CTR_TS_4B     0x00800000
#define DMA_CHANX_CTR_TS_2B     0x00400000
#define DMA_CHANX_CTR_TS_1B     0x00000000

/* DMA descriptor bit fields */
#define DMA_DESC_EOC            0x01000000
#define DMA_DESC_WM             0x00800000
#define DMA_DESC_RM             0x00400000
#define DMA_DESC_NI             0x00200000
#define DMA_DESC_TS_8B          0x00180000
#define DMA_DESC_TS_4B          0x00100000
#define DMA_DESC_TS_2B          0x00080000
#define DMA_DESC_TS_1B          0x00000000
#define DMA_DESC_BLK_1024B      0x00070000
#define DMA_DESC_BLK_512B       0x00060000
#define DMA_DESC_BLK_256B       0x00050000
#define DMA_DESC_BLK_128B       0x00040000
#define DMA_DESC_BLK_64B        0x00030000
#define DMA_DESC_BLK_32B        0x00020000
#define DMA_DESC_BLK_16B        0x00010000
#define DMA_DESC_BLK_8B         0x00000000
#define DMA_DESC_ID             0x00000004
#define DMA_DESC_IE             0x00000002
#define DMA_DESC_ST             0x00000001

/* DMA_CHANX_STA_REG */
#define DMA_CHANX_STA_DM        0x80000000
#define DMA_CHANX_STA_OE        0x40000000
#define DMA_CHANX_STA_DA        0x20000000
#define DMA_CHANX_STA_DD        0x10000000
#define DMA_CHANX_STA_OD        0x08000000
#define DMA_CHANX_STA_ME        0x04000000
#define DMA_CHANX_STA_BE        0x02000000
#define DMA_CHANX_STA_RWE       0x01000000
#define DMA_CHANX_STA_AE        0x00800000
#define DMA_CHANX_STA_DN        0x00400000

/* DMA_INT_REG */
#define DMA_INT_CHAN(x)         (0x1 << (x))

#if    defined(__FPGA__)
#define DMA_INT_CHAN7            0x00000080
#define DMA_INT_CHAN6            0x00000040
#define DMA_INT_CHAN5            0x00000020
#endif

#define DMA_INT_CHAN4            0x00000010
#define DMA_INT_CHAN3            0x00000008
#define DMA_INT_CHAN2            0x00000004
#define DMA_INT_CHAN1            0x00000002
#define DMA_INT_CHAN0            0x00000001


/*********************************/
/* FIO/DMA Burst Setup           */
/*  - descriptor, non-descriptor */
/*  - main, spare                */
/*********************************/

#if defined(__FPGA__)

#define DMA_NODC_MN_BURST_SIZE  (DMA_CHANX_CTR_BLK_32B | DMA_CHANX_CTR_TS_4B)
#define DMA_NODC_SP_BURST_SIZE  (DMA_CHANX_CTR_BLK_32B | DMA_CHANX_CTR_TS_4B)
#define DMA_DESC_MN_BURST_SIZE  (DMA_DESC_BLK_32B | DMA_DESC_TS_4B)
#define DMA_DESC_SP_BURST_SIZE  (DMA_DESC_BLK_32B | DMA_DESC_TS_4B)
#define FIO_MN_BURST_SIZE       (FIO_DMACTR_BLK_32B | FIO_DMACTR_TS4B)
#define FIO_SP_BURST_SIZE       (FIO_DMACTR_BLK_32B | FIO_DMACTR_TS4B)

#else

#if (DMA_SUPPORT_DMA_FIOS == 0)
#define DMA_NODC_MN_BURST_SIZE  (DMA_CHANX_CTR_BLK_512B | DMA_CHANX_CTR_TS_4B)
#define DMA_NODC_SP_BURST_SIZE  (DMA_CHANX_CTR_BLK_512B | DMA_CHANX_CTR_TS_4B)
#define DMA_DESC_MN_BURST_SIZE  (DMA_DESC_BLK_512B | DMA_DESC_TS_4B)
#define DMA_DESC_SP_BURST_SIZE  (DMA_DESC_BLK_512B | DMA_DESC_TS_4B)
#define FIO_MN_BURST_SIZE       (FIO_DMACTR_BLK_512B | FIO_DMACTR_TS4B)
#define FIO_SP_BURST_SIZE       (FIO_DMACTR_BLK_512B | FIO_DMACTR_TS4B)
#else

#endif

#endif

#endif /* __HAL_DMA_H__ */
