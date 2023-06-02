/*------------------------------------------------------------------------------
--                                                                                                                               --
--       This software is confidential and proprietary and may be used                                   --
--        only as expressly authorized by a licensing agreement from                                     --
--                                                                                                                               --
--                            Verisilicon.                                                                                    --
--                                                                                                                               --
--                   (C) COPYRIGHT 2014 VERISILICON                                                            --
--                            ALL RIGHTS RESERVED                                                                    --
--                                                                                                                               --
--                 The entire notice above must be reproduced                                                 --
--                  on all copies and should not be removed.                                                    --
--                                                                                                                               --
--------------------------------------------------------------------------------
--
--  Description : Encoder common configuration parameters
--
------------------------------------------------------------------------------*/
#ifndef __ENCCFG_H__
#define __ENCCFG_H__

#include "base_type.h"


/* Here is defined the default values for the encoder build-time configuration.
 * You can override these settings by defining the values as compiler flags
 * in the Makefile.
 */

#ifdef ARM_ARCH_SWAP
#define ENCH2_INPUT_SWAP_64_YUV                   1
#define ENCH2_INPUT_SWAP_32_YUV                   1
#define ENCH2_INPUT_SWAP_16_YUV                   1
#define ENCH2_INPUT_SWAP_8_YUV                    1
#define ENCH2_INPUT_SWAP_64_RGB16                 1
#define ENCH2_INPUT_SWAP_32_RGB16                 1
#define ENCH2_INPUT_SWAP_16_RGB16                 1
#define ENCH2_INPUT_SWAP_8_RGB16                  0
#define ENCH2_INPUT_SWAP_64_RGB32                 1
#define ENCH2_INPUT_SWAP_32_RGB32                 1
#define ENCH2_INPUT_SWAP_16_RGB32                 0
#define ENCH2_INPUT_SWAP_8_RGB32                  0
#define ENCH2_ROIMAP_QPDELTA_SWAP_64              1
#define ENCH2_ROIMAP_QPDELTA_SWAP_32              1
#define ENCH2_ROIMAP_QPDELTA_SWAP_16              1
#define ENCH2_ROIMAP_QPDELTA_SWAP_8               1
#define ENCH2_OUTPUT_SWAP_64                      0
#define ENCH2_OUTPUT_SWAP_32                      0
#define ENCH2_OUTPUT_SWAP_16                      0
#define ENCH2_OUTPUT_SWAP_8                       0
#define ENCH2_SCALE_OUTPUT_SWAP_64                0
#define ENCH2_SCALE_OUTPUT_SWAP_32                0
#define ENCH2_SCALE_OUTPUT_SWAP_16                0
#define ENCH2_SCALE_OUTPUT_SWAP_8                 0
#define ENCH2_CTBRC_OUTPUT_SWAP_64                0
#define ENCH2_CTBRC_OUTPUT_SWAP_32                0
#define ENCH2_CTBRC_OUTPUT_SWAP_16                0
#define ENCH2_CTBRC_OUTPUT_SWAP_8                 0
#define ENCH2_NALUNIT_SIZE_SWAP_64                0
#define ENCH2_NALUNIT_SIZE_SWAP_32                0
#define ENCH2_NALUNIT_SIZE_SWAP_16                0
#define ENCH2_NALUNIT_SIZE_SWAP_8                 0

#endif

/* The input image's 64-bit swap: 0 or 1
 * This defines the 64-bit endianess of the ASIC input YUV
 * 1 = 128-bit endianess */
#ifndef ENCH2_INPUT_SWAP_64_YUV
#define ENCH2_INPUT_SWAP_64_YUV                   0
#endif


/* The input image's 32-bit swap: 0 or 1
 * This defines the 32-bit endianess of the ASIC input YUV
 * 1 = 64-bit endianess */
#ifndef ENCH2_INPUT_SWAP_32_YUV
#define ENCH2_INPUT_SWAP_32_YUV                   0
#endif

/* The input image's 16-bit swap: 0 or 1
 * This defines the 16-bit endianess of the ASIC input YUV
 */
#ifndef ENCH2_INPUT_SWAP_16_YUV
#define ENCH2_INPUT_SWAP_16_YUV                   0
#endif

/* The input image's 8-bit swap: 0 or 1
 * This defines the byte endianess of the ASIC input YUV
 */
#ifndef ENCH2_INPUT_SWAP_8_YUV
#define ENCH2_INPUT_SWAP_8_YUV                    0
#endif

/* The input image's 64-bit swap: 0 or 1
 * This defines the 64-bit endianess of the ASIC input RGB16
 * 1 = 128-bit endianess */
#ifndef ENCH2_INPUT_SWAP_64_RGB16
#define ENCH2_INPUT_SWAP_64_RGB16                 0
#endif


/* The input image's 32-bit swap: 0 or 1
 * This defines the 32-bit endianess of the ASIC input RGB16
 * 1 = 64-bit endianess */
#ifndef ENCH2_INPUT_SWAP_32_RGB16
#define ENCH2_INPUT_SWAP_32_RGB16                 0
#endif

/* The input image's 16-bit swap: 0 or 1
 * This defines the 16-bit endianess of the ASIC input RGB16
 */
#ifndef ENCH2_INPUT_SWAP_16_RGB16
#define ENCH2_INPUT_SWAP_16_RGB16                 0
#endif

/* The input image's byte swap: 0 or 1
 * This defines the byte endianess of the ASIC input RGB16
 */
#ifndef ENCH2_INPUT_SWAP_8_RGB16
#define ENCH2_INPUT_SWAP_8_RGB16                  1
#endif

/* The input image's 64-bit swap: 0 or 1
 * This defines the 64-bit endianess of the ASIC input RGB32
 * 1 = 64-bit endianess */
#ifndef ENCH2_INPUT_SWAP_64_RGB32
#define ENCH2_INPUT_SWAP_64_RGB32                 0
#endif


/* The input image's 32-bit swap: 0 or 1
 * This defines the 32-bit endianess of the ASIC input RGB32
 * 1 = 64-bit endianess */
#ifndef ENCH2_INPUT_SWAP_32_RGB32
#define ENCH2_INPUT_SWAP_32_RGB32                 0
#endif

/* The input image's 16-bit swap: 0 or 1
 * This defines the 16-bit endianess of the ASIC input RGB32
 */
#ifndef ENCH2_INPUT_SWAP_16_RGB32
#define ENCH2_INPUT_SWAP_16_RGB32                 1
#endif

/* The input image's byte swap: 0 or 1
 * This defines the byte endianess of the ASIC input RGB32
 */
#ifndef ENCH2_INPUT_SWAP_8_RGB32
#define ENCH2_INPUT_SWAP_8_RGB32                  1
#endif

/* ENCH2_OUTPUT_SWAP_XX define the byte endianess of the ASIC output stream.
 * This MUST be configured to be the same as the native system endianess,
 * because the control software relies on system endianess when reading
 * the data from the memory. */

/* The output stream's 64-bit swap: 0 or 1
 * This defines the 64-bit endianess of the ASIC output stream
 * 1 = 128-bit endianess */
#ifndef ENCH2_OUTPUT_SWAP_64
#define ENCH2_OUTPUT_SWAP_64                      1
#endif


/* The output stream's 32-bit swap: 0 or 1
 * This defines the 32-bit endianess of the ASIC output stream
 * 1 = 64-bit endianess */
#ifndef ENCH2_OUTPUT_SWAP_32
#define ENCH2_OUTPUT_SWAP_32                      1
#endif

/* The output stream's 16-bit swap: 0 or 1
 * This defines the 16-bit endianess of the ASIC output stream.
 */
#ifndef ENCH2_OUTPUT_SWAP_16
#define ENCH2_OUTPUT_SWAP_16                      1
#endif

/* The output stream's 8-bit swap: 0 or 1
 * This defines the byte endianess of the ASIC output stream.
 */
#ifndef ENCH2_OUTPUT_SWAP_8
#define ENCH2_OUTPUT_SWAP_8                       1
#endif

/* The output down-scaled image's swapping is defined the same way as
 * output stream swapping. Typically these should be the same values
 * as for output stream. */
#ifndef ENCH2_SCALE_OUTPUT_SWAP_64
#define ENCH2_SCALE_OUTPUT_SWAP_64                  1
#endif
#ifndef ENCH2_SCALE_OUTPUT_SWAP_32
#define ENCH2_SCALE_OUTPUT_SWAP_32                  1
#endif
#ifndef ENCH2_SCALE_OUTPUT_SWAP_16
#define ENCH2_SCALE_OUTPUT_SWAP_16                  1
#endif
#ifndef ENCH2_SCALE_OUTPUT_SWAP_8
#define ENCH2_SCALE_OUTPUT_SWAP_8                   1
#endif
#ifndef ENCH2_CTBRC_OUTPUT_SWAP_64
#define ENCH2_CTBRC_OUTPUT_SWAP_64                  1
#endif
#ifndef ENCH2_CTBRC_OUTPUT_SWAP_32
#define ENCH2_CTBRC_OUTPUT_SWAP_32                  1
#endif
#ifndef ENCH2_CTBRC_OUTPUT_SWAP_16
#define ENCH2_CTBRC_OUTPUT_SWAP_16                  1
#endif
#ifndef ENCH2_CTBRC_OUTPUT_SWAP_8
#define ENCH2_CTBRC_OUTPUT_SWAP_8                   1
#endif

#ifndef ENCH2_NALUNIT_SIZE_SWAP_64
#define ENCH2_NALUNIT_SIZE_SWAP_64                  0
#endif
#ifndef ENCH2_NALUNIT_SIZE_SWAP_32
#define ENCH2_NALUNIT_SIZE_SWAP_32                  0
#endif
#ifndef ENCH2_NALUNIT_SIZE_SWAP_16
#define ENCH2_NALUNIT_SIZE_SWAP_16                  0
#endif
#ifndef ENCH2_NALUNIT_SIZE_SWAP_8
#define ENCH2_NALUNIT_SIZE_SWAP_8                   0
#endif


#ifndef ENCH2_ROIMAP_QPDELTA_SWAP_64
#define ENCH2_ROIMAP_QPDELTA_SWAP_64                0
#endif
#ifndef ENCH2_ROIMAP_QPDELTA_SWAP_32
#define ENCH2_ROIMAP_QPDELTA_SWAP_32                0
#endif
#ifndef ENCH2_ROIMAP_QPDELTA_SWAP_16
#define ENCH2_ROIMAP_QPDELTA_SWAP_16                0
#endif
#ifndef ENCH2_ROIMAP_QPDELTA_SWAP_8
#define ENCH2_ROIMAP_QPDELTA_SWAP_8                 0
#endif
/* The output MV and macroblock info swapping is defined the same way as
 * output stream swapping. Typically these should be the same values
 * as for output stream. */
#ifndef ENCH2_MV_OUTPUT_SWAP_32
#define ENCH2_MV_OUTPUT_SWAP_32                     ENCH2_OUTPUT_SWAP_32
#endif
#ifndef ENCH2_MV_OUTPUT_SWAP_16
#define ENCH2_MV_OUTPUT_SWAP_16                     ENCH2_OUTPUT_SWAP_16
#endif
#ifndef ENCH2_MV_OUTPUT_SWAP_8
#define ENCH2_MV_OUTPUT_SWAP_8                      ENCH2_OUTPUT_SWAP_8
#endif

/* ASIC interrupt enable.
 * This enables/disables the ASIC to generate interrupts
 * If this is '1', the EWL must poll the registers to find out
 * when the HW is ready.
 */
#ifndef ENCH2_IRQ_DISABLE
#define ENCH2_IRQ_DISABLE                         0
#endif

/* ASIC bus interface configuration values                  */
/* DO NOT CHANGE IF NOT FAMILIAR WITH THE CONCEPTS INVOLVED */

/* Burst length. This sets the maximum length of a single ASIC burst in addresses.
 * Allowed values are:
 *          AHB {0, 4, 8, 16} ( 0 means incremental burst type INCR)
 *          OCP [1,63]
 *          AXI [1,16]
 */
#ifndef ENCH2_BURST_LENGTH
#define ENCH2_BURST_LENGTH                               16
#endif

/* SCMD burst mode disable                                                    */
/* 0 - enable SCMD burst mode                                                 */
/* 1 - disable SCMD burst mode                                                */
#ifndef ENCH2_BURST_SCMD_DISABLE
#define ENCH2_BURST_SCMD_DISABLE                         0
#endif

/* INCR type burst mode                                                       */
/* 0 - enable INCR type bursts                                                */
/* 1 - disable INCR type and use SINGLE instead                               */
#ifndef ENCH2_BURST_INCR_TYPE_ENABLED
#define ENCH2_BURST_INCR_TYPE_ENABLED                    0
#endif

/* Data discard mode. When enabled read bursts of length 2 or 3 are converted */
/* to BURST4 and  useless data is discarded. Otherwise use INCR type for that */
/* kind  of read bursts */
/* 0 - disable data discard                                                   */
/* 1 - enable data discard                                                    */
#ifndef ENCH2_BURST_DATA_DISCARD_ENABLED
#define ENCH2_BURST_DATA_DISCARD_ENABLED                 0
#endif

/* AXI bus read and write ID values used by HW. 0 - 255 */
#ifndef ENCH2_AXI_READ_ID
#define ENCH2_AXI_READ_ID                                0
#endif

#ifndef ENCH2_AXI_WRITE_ID
#define ENCH2_AXI_WRITE_ID                               0
#endif

/* End of "ASIC bus interface configuration values"                           */

/* ASIC internal clock gating control. 0 - disabled, 1 - enabled              */
#ifndef ENCH2_ASIC_CLOCK_GATING_ENABLED
#define ENCH2_ASIC_CLOCK_GATING_ENABLED                  0
#endif

/* ASIC timeout interrupt enable/disable                                      */
#ifndef ENCH2_TIMEOUT_INTERRUPT
#define ENCH2_TIMEOUT_INTERRUPT                          1
#endif

/* Hevc slice ready interrupt enable/disable. When enabled the HW will raise */
/* interrupt after every completed slice creating several IRQ per frame.      */
/* When disabled the HW will raise interrupt only when the frame encoding is  */
/* finished.                                                                  */
#ifndef ENCH2_SLICE_READY_INTERRUPT
#define ENCH2_SLICE_READY_INTERRUPT                      1
#endif

/* ASIC input picture read chunk size, 0=4 MBs, 1=1 MB                        */
#ifndef ENCH2_INPUT_READ_CHUNK
#define ENCH2_INPUT_READ_CHUNK                           0
#endif

/* AXI bus dual channel disable, 0=use two channels, 1=use single channel.   */
#ifndef ENCH2_AXI_2CH_DISABLE
#define ENCH2_AXI_2CH_DISABLE                           0
#endif

/* Reconstructed picture output writing, 0=1 MB (write burst for every MB),
   1=4MBs (write burst for every fourth MB).                                 */
#ifndef ENCH2_REC_WRITE_BUFFER
#define ENCH2_REC_WRITE_BUFFER                          1
#endif
/* How often to update VP9 entropy probabilities, higher value lowers SW load.
   1=update for every frame, 2=update for every second frame, etc.           */
#ifndef ENCH2_VP9_ENTROPY_UPDATE_DISTANCE
#define ENCH2_VP9_ENTROPY_UPDATE_DISTANCE               1
#endif

#ifndef ENCH2_AXI40_BURST_LENGTH
#define ENCH2_AXI40_BURST_LENGTH               0x20
#endif

#ifndef ENCH2_ASIC_TIMEOUT_OVERRIDE_ENABLE
#define ENCH2_ASIC_TIMEOUT_OVERRIDE_ENABLE               0
#endif

#ifndef ENCH2_ASIC_TIMEOUT_CYCLES
#define ENCH2_ASIC_TIMEOUT_CYCLES               0
#endif


#endif
