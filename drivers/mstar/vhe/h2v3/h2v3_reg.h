
#ifndef _H2V3_REG_H_
#define _H2V3_REG_H_

enum H2REG_INDEX {
#include <h2v3_regtab.h>
};

int H2PutRegVal(uint* mir, enum H2REG_INDEX idx, uint  val);
int H2GetRegVal(uint* mir, enum H2REG_INDEX idx, uint* val);

/* ARM ARCH SWAP */
#define ENCH2_PIC_SWAP_YUV      0
#define ENCH2_DQM_SWAP64        1
#define ENCH2_DQM_SWAP32        1
#define ENCH2_DQM_SWAP16        1
#define ENCH2_DQM_SWAP_8        1
#define ENCH2_DQM_SWAP          (ENCH2_DQM_SWAP_8|(ENCH2_DQM_SWAP16<<1)|(ENCH2_DQM_SWAP32<<2)|(ENCH2_DQM_SWAP64<<3))
#define ENCH2_OUT_SWAP64        0
#define ENCH2_OUT_SWAP32        0
#define ENCH2_OUT_SWAP16        0
#define ENCH2_OUT_SWAP_8        0
#define ENCH2_OUT_SWAP          (ENCH2_OUT_SWAP_8|(ENCH2_OUT_SWAP16<<1)|(ENCH2_OUT_SWAP32<<2)|(ENCH2_OUT_SWAP64<<3))
#define ENCH2_RCM_SWAP64        0
#define ENCH2_RCM_SWAP32        0
#define ENCH2_RCM_SWAP16        0
#define ENCH2_RCM_SWAP_8        0
#define ENCH2_RCM_SWAP          (ENCH2_RCM_SWAP_8|(ENCH2_RCM_SWAP16<<1)|(ENCH2_RCM_SWAP32<<2)|(ENCH2_RCM_SWAP64<<3))
#define ENCH2_NAL_SWAP64        0
#define ENCH2_NAL_SWAP32        0
#define ENCH2_NAL_SWAP16        0
#define ENCH2_NAL_SWAP_8        0
#define ENCH2_NAL_SWAP          (ENCH2_NAL_SWAP_8|(ENCH2_NAL_SWAP16<<1)|(ENCH2_NAL_SWAP32<<2)|(ENCH2_NAL_SWAP64<<3))
#define ENCH2_SCA_SWAP          0

#define ENCH2_AXI_RD_ID                     0
#define ENCH2_AXI_WR_ID                     0
#define ENCH2_IRQ_DISABLE                   0
#define ENCH2_BURST_LENGTH                  16
#define ENCH2_BURST_SCMD_DISABLE            0
#define ENCH2_BURST_INCR_TYPE_ENABLED       0
#define ENCH2_BURST_DATA_DISCARD_ENABLED    0
#define ENCH2_ASIC_CLOCK_GATING_ENABLED     0
#define ENCH2_INPUT_READ_CHUNK              0
#define ENCH2_AXI_2CH_DISABLE               0

#define ENCH2_CODEC_HEVC                    1
#define ENCH2_CODEC_VP9                     2

#define ENCH2_AXI40_BURST_LENGTH            0x20
#define ENCH2_ASIC_TIMEOUT_OVERRIDE_ENABLE  0
#define ENCH2_ASIC_TIMEOUT_CYCLES           0
#define ENCH2_TIMEOUT_INTERRUPT             1

#endif/*_H2V3_REG_H_*/
