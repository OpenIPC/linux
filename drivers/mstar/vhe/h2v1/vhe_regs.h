
#ifndef _VHE_REGS_H_
#define _VHE_REGS_H_

typedef enum vhe_field {
#include "vhe_regs_table.h"
} vhe_field;

#define VHE_REGS_MAX_SIZE   83

int vhe_put_regs(uint* regs, vhe_field fld, uint  val);
int vhe_get_regs(uint* regs, vhe_field fld, uint* val);

/* codec type */
#define ENCH2_ASIC_HEVC                     1
#define ENCH2_ASIC_VP9                      2 /* Not support */
/* setting */
#define ENCH2_IRQ_DISABLE                   0
#define ENCH2_INPUT_READ_CHUNK              0
#define ENCH2_AXI_READ_ID                   0
#define ENCH2_AXI_WRITE_ID                  0
#define ENCH2_BURST_LENGTH                  16
#define ENCH2_BURST_SCMD_DISABLE            0
#define ENCH2_BURST_INCR_TYPE_ENABLED       0
#define ENCH2_BURST_DATA_DISCARD_ENABLED    0
#define ENCH2_ASIC_CLOCK_GATING_ENABLED     0
#define ENCH2_AXI_2CH_DISABLE               0
/* ARM ARCH SWAP */
#define ENCH2_INPUT_SWAP_64_YUV             1
#define ENCH2_INPUT_SWAP_32_YUV             1
#define ENCH2_INPUT_SWAP_16_YUV             1
#define ENCH2_INPUT_SWAP_8_YUV              1
#define ENCH2_INPUT_SWAP_64_RGB16           1
#define ENCH2_INPUT_SWAP_32_RGB16           1
#define ENCH2_INPUT_SWAP_16_RGB16           1
#define ENCH2_INPUT_SWAP_8_RGB16            0
#define ENCH2_INPUT_SWAP_64_RGB32           1
#define ENCH2_INPUT_SWAP_32_RGB32           1
#define ENCH2_INPUT_SWAP_16_RGB32           0
#define ENCH2_INPUT_SWAP_8_RGB32            0
#define ENCH2_OUTPUT_SWAP_64                0
#define ENCH2_OUTPUT_SWAP_32                0
#define ENCH2_OUTPUT_SWAP_16                0
#define ENCH2_OUTPUT_SWAP_8                 0

#define ENCH2_AXI40_BURST_LENGTH            0x10
#define ENCH2_ASIC_TIMEOUT_OVERRIDE_ENABLE  0
#define ENCH2_ASIC_TIMEOUT_CYCLES           0
#define ENCH2_TIMEOUT_INTERRUPT             1

#endif//_VHE_REGS_H_
