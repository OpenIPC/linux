
#ifndef _VHE_ASIC_H_
#define _VHE_ASIC_H_

#include <mhve_ios.h>

typedef struct vhe_mirr {
    mhve_job    mjob;
    uint        swregs[VHE_REGS_MAX_SIZE];
} vhe_mirr;

typedef struct vhe_asic {
    mhve_ios    mios;
    void*       p_bank;
    void*       p_conf;
    void*       p_mirr;
} vhe_asic;

/* error code */
#define ASIC_SLICE_READY        2
#define ASIC_FRAME_READY        1
#define ASIC_OKAY               0
#define ASIC_ERROR             -1
#define ASIC_ERR_HWTIMEOUT     -2
#define ASIC_ERR_HWRESET       -3
#define ASIC_ERR_BUFF_FULL     -4

/* specific registers */
#define ASIC_ENC                5
#define ASIC_ENC_TRIGGER        0x001

#define ASIC_STATUS             1
#define ASIC_STATUS_ALL         0x3FD
#define ASIC_STATUS_FUSE        0x200
#define ASIC_STATUS_SLICE_READY 0x100
#define ASIC_STATUS_HW_TIMEOUT  0x040
#define ASIC_STATUS_BUFFER_FULL 0x020
#define ASIC_STATUS_HW_RESET    0x010
#define ASIC_STATUS_ERROR       0x008
#define ASIC_STATUS_FRAME_READY 0x004
#define ASIC_STATUS_IRQ_LINE    0x001

#define ASIC_SLICE              7
#define ASIC_NUM_SLICES(reg)    (((reg)>>17)&0xFF)

#define ASIC_OUTPUT_SIZE        9
#define ASIC_CYCLE_COUNT        82

#endif//_VHE_ASIC_H_
