/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Verisilicon.                                    --
--                                                                            --
--                   (C) COPYRIGHT 2014 VERISILICON                           --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Description :  Encoder SW/HW interface register definitions
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------

    Table of contents

    1. Include headers
    2. External compiler flags
    3. Module defines

------------------------------------------------------------------------------*/
#ifndef ENC_SWHWREGISTERS_H
#define ENC_SWHWREGISTERS_H

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#include "base_type.h"


/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/


#define ASIC_SWREG_AMOUNT               (656/4+1)

#define ASIC_ID_BLUEBERRY               0x48321220
#define ASIC_ID_CLOUDBERRY              0x48321480
#define ASIC_ID_DRAGONFLY               0x48322170
#define ASIC_ID_EVERGREEN               0x48323590
#define ASIC_ID_EVERGREEN_PLUS          0x48324470
#define ASIC_ID_FOXTAIL                 0x48324780

#define ASIC_INPUT_YUV420PLANAR         0x00
#define ASIC_INPUT_YUV420SEMIPLANAR     0x01
#define ASIC_INPUT_YUYV422INTERLEAVED   0x02
#define ASIC_INPUT_UYVY422INTERLEAVED   0x03
#define ASIC_INPUT_RGB565               0x04
#define ASIC_INPUT_RGB555               0x05
#define ASIC_INPUT_RGB444               0x06
#define ASIC_INPUT_RGB888               0x07
#define ASIC_INPUT_RGB101010            0x08
#define ASIC_INPUT_I010                 0x09      //yuv420 10 bit, in 16 bit pixel,  low 10bits valid, high 6 bits invalid.
#define ASIC_INPUT_P010                 0x0a      //yuv420 10 bit, in 16 bit pixel,  high 10bits valid, low 6 bits invalid.
#define ASIC_INPUT_PACKED_10BIT_PLANAR  0x0b      //yuv420 10 bit, packed planar.
#define ASIC_INPUT_PACKED_10BIT_Y0L2    0x0c      //yuv420 10 bit, packed Y0L2.



/* Bytes of external memory for VP9 counters for probability updates,
 * 252 counters for dct coeff probs, 1 for skipped, 1 for intra type and
 * 2 * 11 for mv probs, each counter 2 bytes */
#define ASIC_VP9_PROB_COUNT_SIZE            244*2
#define ASIC_VP9_PROB_COUNT_MODE_OFFSET     220
#define ASIC_VP9_PROB_COUNT_MV_OFFSET       222
#define ASIC_REG_INDEX_STATUS           (20/4)

#define MAX_SLICE_NUM 128

/* HW Register field names */
typedef enum
{

//#include "registerenum.h"

  HEncRegisterAmount

} regName;

/* HW Register field descriptions */
typedef struct
{
  u32 name;               /* Register name and index  */
  i32 base;               /* Register base address  */
  u32 mask;               /* Bitmask for this field */
  i32 lsb;                /* LSB for this field [31..0] */
  i32 trace;              /* Enable/disable writing in swreg_params.trc */
  i32 rw;                 /* 1=Read-only 2=Write-only 3=Read-Write */
  char *description;      /* Field description */
} regField_s;

/* Flags for read-only, write-only and read-write */
#define RO 1
#define WO 2
#define RW 3

#define REGBASE(reg) (asicRegisterDesc[reg].base)

/* Description field only needed for system model build. */
#ifdef TEST_DATA
#define H2REG(name, base, mask, lsb, trace, rw, desc) \
        {name, base, mask, lsb, trace, rw, desc}
#else
#define H2REG(name, base, mask, lsb, trace, rw, desc) \
        {name, base, mask, lsb, trace, rw, ""}
#endif


/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/

void EncAsicSetRegisterValue(u32 *regMirror, regName name, u32 value);
u32 EncAsicGetRegisterValue(const void *ewl, u32 *regMirror, regName name);

#endif
