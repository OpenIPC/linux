/*!
*****************************************************************************
** \file        arch/arm/mach-gk7101/include/plat/mdma.h
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

#ifndef __PLAT_MDMA_H__
#define __PLAT_MDMA_H__


/****************************************************/
/* Capabilities based on chip revision              */
/****************************************************/


#define NUMBERS_MDMA_INSTANCES      4


#define MDMA_ON_AHB                 1


#define MDMA_SUPPORT_ALPHA_BLEND    0

/****************************************************/
/* Controller registers definitions                 */
/****************************************************/

#if (MDMA_ON_AHB == 1)
#define MDMA_SRC_1_BASE_OFFSET      0x00
#define MDMA_SRC_1_PITCH_OFFSET     0x04
#define MDMA_SRC_2_BASE_OFFSET      0x08
#define MDMA_SRC_2_PITCH_OFFSET     0x0c
#define MDMA_DST_BASE_OFFSET        0x10
#define MDMA_DST_PITCH_OFFSET       0x14
#define MDMA_WIDTH_OFFSET           0x18
#define MDMA_HIGHT_OFFSET           0x1c
#define MDMA_TRANSPARENT_OFFSET     0x20
#define MDMA_OPCODE_OFFSET          0x24
#define MDMA_PENDING_OPS_OFFSET     0x28

#if (MDMA_SUPPORT_ALPHA_BLEND == 1)
#define MDMA_PIXELFORMAT_OFFSET     0x2c
#define MDMA_ALPHA_OFFSET           0x30
#define MDMA_CLUT_BASE_OFFSET       0x400
#endif

#define MDMA_SRC_1_BASE_REG         GRAPHICS_DMA_REG(MDMA_SRC_1_BASE_OFFSET)
#define MDMA_SRC_1_PITCH_REG        GRAPHICS_DMA_REG(MDMA_SRC_1_PITCH_OFFSET)
#define MDMA_SRC_2_BASE_REG         GRAPHICS_DMA_REG(MDMA_SRC_2_BASE_OFFSET)
#define MDMA_SRC_2_PITCH_REG        GRAPHICS_DMA_REG(MDMA_SRC_2_PITCH_OFFSET)
#define MDMA_DST_BASE_REG           GRAPHICS_DMA_REG(MDMA_DST_BASE_OFFSET)
#define MDMA_DST_PITCH_REG          GRAPHICS_DMA_REG(MDMA_DST_PITCH_OFFSET)
#define MDMA_WIDTH_REG              GRAPHICS_DMA_REG(MDMA_WIDTH_OFFSET)
#define MDMA_HEIGHT_REG             GRAPHICS_DMA_REG(MDMA_HIGHT_OFFSET)
#define MDMA_TRANSPARENT_REG        GRAPHICS_DMA_REG(MDMA_TRANSPARENT_OFFSET)
#define MDMA_OPCODE_REG             GRAPHICS_DMA_REG(MDMA_OPCODE_OFFSET)
#define MDMA_PENDING_OPS_REG        GRAPHICS_DMA_REG(MDMA_PENDING_OPS_OFFSET)

#if (MDMA_SUPPORT_ALPHA_BLEND == 1)
#define MDMA_PIXELFORMAT_REG        GRAPHICS_DMA_REG(MDMA_PIXELFORMAT_OFFSET)
#define MDMA_ALPHA_REG              GRAPHICS_DMA_REG(MDMA_ALPHA_OFFSET)
#define MDMA_CLUT_BASE_REG          GRAPHICS_DMA_REG(MDMA_CLUT_BASE_OFFSET)

/* MDMA_PIXELFORMAT_REG */
#define MDMA_PIXELFORMAT_THROTTLE_DRAM  (1L << 11)

#endif /* (MDMA_SUPPORT_ALPHA_BLEND == 1)*/
#endif /*    MDMA_ON_AHB == 1    */

#endif

