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
--  Abstract : H2 Encoder Wrapper Layer for OS services
--
------------------------------------------------------------------------------*/

#ifndef __EWL_H__
#define __EWL_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "base_type.h"

  /* Return values */
#define EWL_OK                      0
#define EWL_ERROR                  -1

#define EWL_HW_WAIT_OK              EWL_OK
#define EWL_HW_WAIT_ERROR           EWL_ERROR
#define EWL_HW_WAIT_TIMEOUT         1

  /* HW configuration values */
#define EWL_HW_BUS_TYPE_UNKNOWN     0
#define EWL_HW_BUS_TYPE_AHB         1
#define EWL_HW_BUS_TYPE_OCP         2
#define EWL_HW_BUS_TYPE_AXI         3
#define EWL_HW_BUS_TYPE_PCI         4

#define EWL_HW_BUS_WIDTH_UNKNOWN     0
#define EWL_HW_BUS_WIDTH_32BITS      1
#define EWL_HW_BUS_WIDTH_64BITS      2
#define EWL_HW_BUS_WIDTH_128BITS     3

#define EWL_HW_SYNTHESIS_LANGUAGE_UNKNOWN     0
#define EWL_HW_SYNTHESIS_LANGUAGE_VHDL        1
#define EWL_HW_SYNTHESIS_LANGUAGE_VERILOG     2

#define EWL_HW_CONFIG_NOT_SUPPORTED    0
#define EWL_HW_CONFIG_ENABLED          1

  /* Hardware configuration description */
//swreg80 bit             name                                      function

//    bit 30                 sw_enc_HWScalingSupport         Down-scaling supported by HW. 0=not supported. 1=supported
//    bit 29                 sw_enc_HWBFrameSupport         HW bframe support. 0=not support bframe. 1=support bframe
//    bit 28                 sw_enc_HWRgbSupport              RGB to YUV conversion supported by HW. 0=not supported. 1=supported
//    bit 27                 sw_enc_HWHevcSupport             HEVC encoding supported by HW. 0=not supported. 1=supported
//    bit 26                 sw_enc_HWVp9Support              VP9 encoding supported by HW. 0=not supported. 1=supported
//    bit 25                 sw_enc_HWDenoiseSupport        Denoise supported by HW, 0=not supported. 1=supported
//    bit [23:21]          sw_enc_HWBus                         Bus connection of HW. 1=AHB. 2=OCP. 3=AXI. 4=PCI. 5=AXIAHB. 6=AXIAPB.
//    bit [18:17]          sw_enc_HWSynthesisLan            Synthesis language. 1=vhdl. 2=verilog
//    bit [14:13]          sw_enc_HWBusWidth                 Bus width of HW. 0=32b. 1=64b. 2=128b
//    bit [12:00]          sw_enc_HWMaxVideoWidth         Maximum video width supported by HW (pixels)
  typedef struct EWLHwConfig
  {
    u32 scalingEnabled;  /*bit 30 *//* HW supports down-scaling */
    u32 bFrameEnabled; /*bit 29 *//* HW b frame enabled */
    u32 rgbEnabled;      /*bit 28 *//* HW supports RGB input */
    u32 hevcEnabled;     /*bit 27 *//* HW supports HEVC */
    u32 vp9Enabled;      /*bit 26 *//* HW supports VP9 */
    u32 deNoiseEnabled;      /*bit 25 *//* HW supports DeNoise */
    u32 main10Enabled;      /*bit 24 *//* HW supports Main10 */
    u32 busType;      /*bit 23:21 *//* HW bus type in use  1=AHB. 2=OCP. 3=AXI. 4=PCI. 5=AXIAHB. 6=AXIAPB.*/

    u32 synthesisLanguage; /*bit 18:17 *//* HW synthesisLanguage. 1=vhdl. 2=verilog */

    u32 busWidth;           /*bit 14:13 *//* bus width  */
    u32 maxEncodedWidth; /*bit 12:0 *//* Maximum supported width for video encoding  */
  } EWLHwConfig_t;


  /* Allocated linear memory area information */
  typedef struct EWLLinearMem
  {
    u32 *virtualAddress;
    u32 busAddress;
    u32 size;
  } EWLLinearMem_t;

  /* EWLInitParam is used to pass parameters when initializing the EWL */
  typedef struct EWLInitParam
  {
    u32 clientType;
  } EWLInitParam_t;

#define EWL_CLIENT_TYPE_HEVC_ENC         1U
#define EWL_CLIENT_TYPE_VP9_ENC          2U

  /*------------------------------------------------------------------------------
      4.  Function prototypes
  ------------------------------------------------------------------------------*/

  /* Read and return the HW ID register value, static implementation */
  u32 EWLReadAsicID(void);

  /* Read and return HW configuration info, static implementation */
  EWLHwConfig_t EWLReadAsicConfig(void);

  /* Initialize the EWL instance
   * Returns a wrapper instance or NULL for error
   * EWLInit is called when the encoder instance is initialized */
  const void *EWLInit(EWLInitParam_t *param);

  /* Release the EWL instance
   * Returns EWL_OK or EWL_ERROR
   * EWLRelease is called when the encoder instance is released */
  i32 EWLRelease(const void *inst);

  /* Reserve the HW resource for one codec instance
   * EWLReserveHw is called when beginning a frame encoding
   * The function may block until the resource is available.
   * Returns EWL_OK if the resource was successfully reserved for this instance
   * or EWL_ERROR if unable to reserve the resource. */
  i32 EWLReserveHw(const void *inst);

  /* Release the HW resource
   * EWLReleaseHw is called when the HW has finished the frame encoding.
   * The codec SW will continue the frame encoding but the HW can
   * be used by another codec.*/
  void EWLReleaseHw(const void *inst);

  /* Frame buffers memory */
  i32 EWLMallocRefFrm(const void *inst, u32 size, EWLLinearMem_t *info);
  void EWLFreeRefFrm(const void *inst, EWLLinearMem_t *info);

  /* SW/HW shared memory */
  i32 EWLMallocLinear(const void *inst, u32 size, EWLLinearMem_t *info);
  void EWLFreeLinear(const void *inst, EWLLinearMem_t *info);

  /* D-Cache coherence *//* Not in use currently */
  void EWLDCacheRangeFlush(const void *instance, EWLLinearMem_t *info);
  void EWLDCacheRangeRefresh(const void *instance, EWLLinearMem_t *info);

  /* Write value to a HW register
   * All registers are written at once at the beginning of frame encoding
   * Offset is relative to the the HW ID register (#0) in bytes
   * Enable indicates when the HW is enabled. If shadow registers are used then
   * they must be flushed to the HW registers when enable is '1' before
   * writing the register that enables the HW */
  void EWLWriteReg(const void *inst, u32 offset, u32 val);

  /* Read and return the value of a HW register
   * The status register is read after every macroblock encoding by SW
   * The other registers which may be updated by the HW are read after
   * BUFFER_FULL or FRAME_READY interrupt
   * Offset is relative to the the HW ID register (#0) in bytes */
  u32 EWLReadReg(const void *inst, u32 offset);

  /* Writing all registers in one call *//*Not in use currently */
  void EWLWriteRegAll(const void *inst, const u32 *table, u32 size);
  /* Reading all registers in one call *//*Not in use currently */
  void EWLReadRegAll(const void *inst, u32 *table, u32 size);

  /* HW enable/disable. This will write <val> to register <offset> and by */
  /* this enablig/disabling the hardware. */
  void EWLEnableHW(const void *inst, u32 offset, u32 val);
  void EWLDisableHW(const void *inst, u32 offset, u32 val);

  /* Synchronize SW with HW
   * Returns EWL_HW_WAIT_OK, EWL_HW_WAIT_ERROR or EWL_HW_WAIT_TIMEOUT
   * EWLWaitHwRdy is called after enabling the HW to wait for IRQ from HW.
   * If slicesReady pointer is given, at input it should contain the number
   * of slicesReady received. The function will return when the HW has finished
   * encoding next slice. Upon return the slicesReady pointer will contain
   * the number of slices that are ready and available in the HW output buffer.
   */

  i32 EWLWaitHwRdy(const void *inst, u32 *slicesReady,u32 totalsliceNumber,u32* status_register);


  /* SW/SW shared memory handling */
  void *EWLmalloc(u32 n);
  void *EWLcalloc(u32 n, u32 s);
  void EWLfree(void *p);
  void *EWLmemcpy(void *d, const void *s, u32 n);
  void *EWLmemset(void *d, i32 c, u32 n);
  int EWLmemcmp(const void *s1, const void *s2, u32 n);

#ifdef __cplusplus
}
#endif
#endif /*__EWL_H__*/