
#ifndef _H2V3_IOS_H_
#define _H2V3_IOS_H_

#include <mhve_ios.h>

typedef struct h2v3_mir h2v3_mir;
typedef struct h2v3_ios h2v3_ios;

#define H2V3_REG_SIZE       (165)
#define H2V3_MAX_REG_NUM    (137)

struct h2v3_mir
{
    mhve_job    mjob;
    uint        regs[H2V3_REG_SIZE];
};

struct h2v3_ios
{
    /* mhve_ios MUST be the first element */
    mhve_ios    mios;
    void*       bank;
    void*       conf;
    void*       pjob;
};

/* error code */
#define ASIC_SLICE_DONE        ( 2 )
#define ASIC_FRAME_DONE        ( 1 )
#define ASIC_OKAY              ( 0 )
#define ASIC_ERROR             (-1 )
#define ASIC_ERR_HWTIMEOUT     (-2 )
#define ASIC_ERR_HWRESET       (-3 )
#define ASIC_ERR_BUFF_FULL     (-4 )

/* specific registers */
#define ASIC_PRODUCT_REV       (  0)
/* trigger controller register */
#define ASIC_ENC               (  5)
#define ASIC_ENC_TRIGGER        0x001
/* status report register */
#define ASIC_STATUS            (  1)
#define ASIC_STATUS_ALL         0x3FD
#define ASIC_STATUS_FUSE        0x200
#define ASIC_STATUS_SLICE_DONE  0x100
#define ASIC_STATUS_HW_TIMEOUT  0x040
#define ASIC_STATUS_BUFF_FULL   0x020
#define ASIC_STATUS_HW_RESET    0x010
#define ASIC_STATUS_ERROR       0x008
#define ASIC_STATUS_FRAME_DONE  0x004
#define ASIC_STATUS_IRQ_LINE    0x001
/* slice-number report register */
#define ASIC_SLICE             (  7)
#define ASIC_SLICE_NR(reg)     (((reg)>>17)&0xFF)
/* tcnt/cycles report register */
#define ASIC_OUTPUT_SIZE       (  9)
#define ASIC_SUPPORT_CFG       ( 80)
#define ASIC_CYCLE_COUNT       ( 82)
#define ASIC_NZCOUNT_AVQ       (108)
#define ASIC_INTRACU8NUM       (111)
#define ASIC_SKIP_CU8NUM       (112)
#define ASIC_PBF4NRDCOST       (113)
#define ASIC_CTBBITS_BOUND     (119)
#define ASIC_TOTAL_LCUBITS     (120)
#define ASIC_SIGMA0            (135)
#define ASIC_SIGMA1            (136)
#define ASIC_SIGMA2            (137)
#define ASIC_QP_SUM            (159)
#define ASIC_QP_NUM            (160)
#define ASIC_PIC_COMPLEX       (161)

#endif/*_H2V3_IOS_H*/
