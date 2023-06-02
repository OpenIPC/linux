/*!
*****************************************************************************
** \file        arch/arm/mach-gk/include/mach/idc.h
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
#ifndef __MACH_IDC_H__
#define __MACH_IDC_H__

#include <mach/hardware.h>

/****************************************************/
/*          Capabilities based on chip revision     */
/****************************************************/

#define IDC_INSTANCES               2
#define IDCS_INSTANCES              0

#define IDC_INTERNAL_DELAY_CLK      2

/****************************************************/
/*          Controller registers definitions        */
/****************************************************/
#define IDC_PSLL_OFFSET             0x00
#define IDC_PSLH_OFFSET             0x04
#define IDC_FMCTRL_OFFSET           0x08
#define IDC_CTRL_OFFSET             0x0c
#define IDC_STS_OFFSET              0x10
#define IDC_ENR_OFFSET              0x14
#define IDC_DATA_OFFSET             0x18
#define IDC_FMDATA_OFFSET           0x1c

#if (IDCS_INSTANCES >= 1)
#define IDCS_ENR_OFFSET             0x00
#define IDCS_CTRL_OFFSET            0x04
#define IDCS_DATA_OFFSET            0x08
#define IDCS_STS_OFFSET             0x0c
#define IDCS_FIFO_CNT_OFFSET        0x10
#define IDCS_RX_CNT_OFFSET          0x14
#define IDCS_TX_CNT_OFFSET          0x18
#define IDCS_HOLD_TIME_OFFSET       0x1c
#define IDCS_SLAVE_ADDR_OFFSET      0x20
#define IDCS_SCL_TIMER_OFFSET       0x24
#define IDCS_TIMEOUT_STS_OFFSET     0x28
#endif

#define IDC_REG(x)              (GK_PA_IDC + (x))

#define IDC_ENR_REG             IDC_REG(IDC_ENR_OFFSET)
#define IDC_CTRL_REG            IDC_REG(IDC_CTRL_OFFSET)
#define IDC_DATA_REG            IDC_REG(IDC_DATA_OFFSET)
#define IDC_STS_REG             IDC_REG(IDC_STS_OFFSET)
#define IDC_PSLL_REG            IDC_REG(IDC_PSLL_OFFSET)
#define IDC_PSLH_REG            IDC_REG(IDC_PSLH_OFFSET)
#define IDC_FMCTRL_REG          IDC_REG(IDC_FMCTRL_OFFSET)
#define IDC_FMDATA_REG          IDC_REG(IDC_FMDATA_OFFSET)

#if (IDC_INSTANCES >= 2)

#define IDC2_REG(x)             (GK_PA_IDC2 + (x))

#define IDC2_ENR_REG            IDC2_REG(IDC_ENR_OFFSET)
#define IDC2_CTRL_REG           IDC2_REG(IDC_CTRL_OFFSET)
#define IDC2_DATA_REG           IDC2_REG(IDC_DATA_OFFSET)
#define IDC2_STS_REG            IDC2_REG(IDC_STS_OFFSET)
#define IDC2_PSLL_REG           IDC2_REG(IDC_PSLL_OFFSET)
#define IDC2_PSLH_REG           IDC2_REG(IDC_PSLH_OFFSET)
#define IDC2_FMCTRL_REG         IDC2_REG(IDC_FMCTRL_OFFSET)
#define IDC2_FMDATA_REG         IDC2_REG(IDC_FMDATA_OFFSET)
#endif

#if (IDCS_INSTANCES >= 1)
#define IDCS_ENR_REG            IDCS_REG(IDCS_ENR_OFFSET)
#define IDCS_CTRL_REG           IDCS_REG(IDCS_CTRL_OFFSET)
#define IDCS_DATA_REG           IDCS_REG(IDCS_DATA_OFFSET)
#define IDCS_STS_REG            IDCS_REG(IDCS_STS_OFFSET)
#define IDCS_FIFO_CNT_REG       IDCS_REG(IDCS_FIFO_CNT_OFFSET)
#define IDCS_RX_CNT_REG         IDCS_REG(IDCS_RX_CNT_OFFSET)
#define IDCS_TX_CNT_REG         IDCS_REG(IDCS_TX_CNT_OFFSET)
#define IDCS_HOLD_TIME_REG      IDCS_REG(IDCS_HOLD_TIME_OFFSET)
#define IDCS_SLAVE_ADDR_REG     IDCS_REG(IDCS_SLAVE_ADDR_OFFSET)
#define IDCS_SCL_TIMER_REG      IDCS_REG(IDCS_SCL_TIMER_OFFSET)
#define IDCS_TIMEOUT_STS_REG    IDCS_REG(IDCS_TIMEOUT_STS_OFFSET)


/* Bit/format definition */

/* IDCS_CTRL_REG */
#define IDCS_IRQ_EN             0x02
#define IDCS_IRQ_P_SR_EN        0x04
#define IDCS_IRQ_FIFO_TH_EN     0x08
#define IDCS_IRQ_TIME_OUT_EN    0x10

/* IDCS_STS_REG */
#define IDCS_TIMEOUT            0x100
#define IDCS_STOP               0x080
#define IDCS_REPEATED_START     0x040
#define IDCS_FIFO_TH_VLD        0x020
#define IDCS_SEL                0x010
#define IDCS_GENERAL_CALL       0x008
#define IDCS_FIFO_FULL          0x004
#define IDCS_FIFO_EMPTY         0x002
#define IDCS_RX_TX_STATE        0x001

#define IDCS_TX_MODE            0x0
#define IDCS_RX_MODE            0x1

#endif

/* ==========================================================================*/
/* IDC_ENABLE_REG */
#define IDC_ENR_ENABLE          (0x01)//bit0
#define IDC_ENR_DISABLE         (0x00)

/* IDC_CTRL_REG */
#define IDC_CTRL_IF             (0x08)//bit3
#define IDC_CTRL_ACK            (0x04)//bit2
#define IDC_CTRL_START          (0x02)//bit1
#define IDC_CTRL_STOP           (0x01)//bit0
#define IDC_CTRL_CLS            (0x00)

#define IDC_FIFO_BUF_SIZE       (63)

/* IDC_STS_REG */
#define IDC_STS_MODE            (0x01)//1:master receiver; 0:master transmitter

/* IDC_FM_CTRL_REG */

#define IDC_FMCTRL_IF           (0x10)//bit4
#define IDC_FMCTRL_START        (0x02)//bit1
#define IDC_FMCTRL_STOP         (0x01)//bit0

/* ==========================================================================*/
#ifndef __ASSEMBLER__

struct gk_platform_i2c {
    int                 clk_limit;  //Hz
    int                 bulk_write_num;
    unsigned int        i2c_class;
    u32                 (*get_clock)(void);
};
#define IDC_PARAM_CALL(id, arg, perm) \
    module_param_cb(idc##id##_clk_limit, &param_ops_int, &(arg.clk_limit), perm); \
    module_param_cb(idc##id##_bulk_write_num, &param_ops_int, &(arg.bulk_write_num), perm)

/* ==========================================================================*/
extern struct platform_device   gk_idc;
extern struct platform_device   gk_idc_hdmi;
//extern struct platform_device   gk_i2cmux;

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif  /* __MACH_IDC_H__ */

