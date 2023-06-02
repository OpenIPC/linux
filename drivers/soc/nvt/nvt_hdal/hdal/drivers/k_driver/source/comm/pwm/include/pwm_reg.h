#ifndef __PWM_REG_H__
#define __PWM_REG_H__
/*
    Register definition header file for PWM module.

    This file is the header file that define the address offset and bit
    definition of registers of PWM module.

    @file       pwm_reg.h
    @ingroup    mIDrvIO_PWM
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
#include "pwm_platform.h"

#define PWM_PWM0_CTRL_OFS       0x00  /* PWM0 Control Register */
#define PWM_PWM0_PRD_OFS        0x04  /* PWM0 Period Register */
#define PWM_PWM0_MS_EXT_OFS     0x04  /* PWM0 Micro step extend Register */

#define PWM_PWM1_CTRL_OFS       0x08  /* PWM1 Control Register */
#define PWM_PWM1_PRD_OFS        0x0C  /* PWM1 Period Register */
#define PWM_PWM1_MS_EXT_OFS     0x0C  /* PWM1 Micro step extend Register */

#define PWM_PWM2_CTRL_OFS       0x10  /* PWM2 Control Register */
#define PWM_PWM2_PRD_OFS        0x14  /* PWM2 Period Register */
#define PWM_PWM2_MS_EXT_OFS     0x14  /* PWM2 Micro step extend Register */

#define PWM_PWM3_CTRL_OFS       0x18  /* PWM3 Control Register */
#define PWM_PWM3_PRD_OFS        0x1C  /* PWM3 Period Register */
#define PWM_PWM3_MS_EXT_OFS     0x1C  /* PWM3 Micro step extend Register */

#define PWM_PWM4_CTRL_OFS       0x20  /* PWM4 Control Register */
#define PWM_PWM4_PRD_OFS        0x24  /* PWM4 Period Register */
#define PWM_PWM4_MS_EXT_OFS     0x24  /* PWM4 Micro step extend Register */

#define PWM_PWM5_CTRL_OFS       0x28  /* PWM5 Control Register */
#define PWM_PWM5_PRD_OFS        0x2C  /* PWM5 Period Register */
#define PWM_PWM5_MS_EXT_OFS     0x2C  /* PWM5 Micro step extend Register */

#define PWM_PWM6_CTRL_OFS       0x30  /* PWM6 Control Register */
#define PWM_PWM6_PRD_OFS        0x34  /* PWM6 Period Register */
#define PWM_PWM6_MS_EXT_OFS     0x34  /* PWM6 Micro step extend Register */

#define PWM_PWM7_CTRL_OFS       0x38  /* PWM7 Control Register */
#define PWM_PWM7_PRD_OFS        0x3C  /* PWM7 Period Register */
#define PWM_PWM7_MS_EXT_OFS     0x3C  /* PWM7 Micro step extend Register */

#define PWM_PWM8_CTRL_OFS       0x40  /* PWM8 Control Register */
#define PWM_PWM8_PRD_OFS        0x44  /* PWM8 Period Register */

#define PWM_PWM9_CTRL_OFS       0x48  /* PWM9 Control Register */
#define PWM_PWM9_PRD_OFS        0x4C  /* PWM9 Period Register */

#define PWM_PWM10_CTRL_OFS      0x50  /* PWM10 Control Register */
#define PWM_PWM10_PRD_OFS       0x54  /* PWM10 Period Register */

#define PWM_PWM11_CTRL_OFS      0x58  /* PWM11 Control Register */
#define PWM_PWM11_PRD_OFS       0x5C  /* PWM11 Period Register */

#define PWM_CTRL_REG_BUF_OFS    0x00
REGDEF_BEGIN(PWM_CTRL_REG_BUF)                  /* PWM Control Register Buf */
REGDEF_BIT(pwm_on, 16)                      /* PWM on cycle */
REGDEF_BIT(pwm_type, 1)                     /* 0: PWM ,1: Mirco step mode */
REGDEF_BIT(pwm_ms_dir, 1)                   /* Micro step moving direction 0: Counterclockwise 1: Clockwise */
REGDEF_BIT(, 2)                             /* Reserved bit 18~19 */
REGDEF_BIT(pwm_ms_threshold_en, 1)          /* Mirco step degree threshold enable */
REGDEF_BIT(, 3)                             /* Reserved bit 21~23 */
REGDEF_BIT(pwm_ms_threshold, 7)             /* Mirco step degree threshold value */
REGDEF_BIT(, 1)                             /* Reserved bit 30~31 */
REGDEF_END(PWM_CTRL_REG_BUF)                    /* PWM Control Register Buf end */

#define PWM_PERIOD_REG_BUF_OFS    0x04
REGDEF_BEGIN(PWM_PERIOD_REG_BUF)                /* PWM Period Register Buf */
REGDEF_BIT(pwm_r, 8)                        /* PWM rising time, the number of base clock cycles. */
REGDEF_BIT(pwm_f, 8)                        /* PWM falling time, the number of base clock cycles. */
REGDEF_BIT(pwm_prd, 8)                      /* PWM basis period, the number of clock cycles.  */
REGDEF_BIT(, 4)                             /* Reserved bit 24~27 */
REGDEF_BIT(pwm_inv, 1)                      /* Only used in PWM mode.0: Not invert; 1: PWM0 invert. */
REGDEF_BIT(, 3)                             /* Reserved bit 29~31 */
REGDEF_END(PWM_PERIOD_REG_BUF)                  /* PWM Period Register Buf end */

#define PWM_MS_EXT_REG_BUF_OFS    0x04
REGDEF_BEGIN(PWM_MS_EXT_REG_BUF)                /* PWM micro step extern control register */
REGDEF_BIT(pwm_ms_start_grads, 9)           /* Micro step starting graduation */
REGDEF_BIT(, 3)                             /* Reserved bit 9~11 */
REGDEF_BIT(pwm_ms_step_per_unit, 3)         /* Micro step step per unit */
REGDEF_BIT(, 1)                             /* Reserved bit 15 */

REGDEF_BIT(pwm_ms_grads_per_step, 2)        /* Micro step graduations per step */
REGDEF_BIT(, 14)                            /* Reserved bit 17~31 */
REGDEF_END(PWM_MS_EXT_REG_BUF)                  /* PWM Period Register Buf end */

#define PWM_CTRL_INTEN_OFS0  0xE0
REGDEF_BEGIN(PWM_CTRL_INTEN)                /* PWM interrupt Control Register, control PWMx_DONE_INTEN */
REGDEF_BIT(pwm0_done_inten, 1)
REGDEF_BIT(pwm1_done_inten, 1)
REGDEF_BIT(pwm2_done_inten, 1)
REGDEF_BIT(pwm3_done_inten, 1)

REGDEF_BIT(pwm4_done_inten, 1)
REGDEF_BIT(pwm5_done_inten, 1)
REGDEF_BIT(pwm6_done_inten, 1)
REGDEF_BIT(pwm7_done_inten, 1)

REGDEF_BIT(pwm8_done_inten, 1)
REGDEF_BIT(pwm9_done_inten, 1)
REGDEF_BIT(pwm10_done_inten, 1)
REGDEF_BIT(pwm11_done_inten, 1)

REGDEF_BIT(, 20)
REGDEF_END(PWM_CTRL_INTEN)                     /* PWM interrupt Control Register0, control PWMx_DONE_INTEN end*/

#define PWM_MS_CTRL_INTEN_OFS0  0xE4
REGDEF_BEGIN(PWM_MS_CTRL_INTEN)            /* Micro step interrupt Control Register, control MSx_DONE_INTEN */
REGDEF_BIT(ms0_done_inten, 1)
REGDEF_BIT(ms1_done_inten, 1)
REGDEF_BIT(ms2_done_inten, 1)
REGDEF_BIT(ms3_done_inten, 1)

REGDEF_BIT(ms4_done_inten, 1)
REGDEF_BIT(ms5_done_inten, 1)
REGDEF_BIT(ms6_done_inten, 1)
REGDEF_BIT(ms7_done_inten, 1)

REGDEF_BIT(, 24)
REGDEF_END(PWM_MS_CTRL_INTEN)                   /* Micro step interrupt Control Register, control MSx_DONE_INTEN end */

#define PWM_CCNT_CTRL_INTEN_OFS0  0xE8
REGDEF_BEGIN(PWM_CCNT_CTRL_INTEN)           /* CCNT  interrupt Control Register */
REGDEF_BIT(ccnt0_edge_inten, 1)
REGDEF_BIT(ccnt0_trig_inten, 1)
REGDEF_BIT(ccnt0_tout_inten, 1)
REGDEF_BIT(, 1)

REGDEF_BIT(ccnt1_edge_inten, 1)
REGDEF_BIT(ccnt1_trig_inten, 1)
REGDEF_BIT(ccnt1_tout_inten, 1)
REGDEF_BIT(, 1)

REGDEF_BIT(ccnt2_edge_inten, 1)
REGDEF_BIT(ccnt2_trig_inten, 1)
REGDEF_BIT(ccnt2_tout_inten, 1)
REGDEF_BIT(, 1)

REGDEF_BIT(, 20)
REGDEF_END(PWM_CCNT_CTRL_INTEN)                 /* CCNT  interrupt Control Register end */

#define PWM_CLKDIV_LOAD_INTEN_OFS0  0xEC
REGDEF_BEGIN(PWM_CLKDIV_LOAD_INTEN)         /* CLKDIV load done interrupt Control Register */
REGDEF_BIT(clkdiv_00_03_load_inten,    1)
REGDEF_BIT(clkdiv_04_07_load_inten,    1)
REGDEF_BIT(, 14)
REGDEF_BIT(clkdiv_00_03_tgt_cnt_inten, 1)
REGDEF_BIT(clkdiv_04_07_tgt_cnt_inten, 1)
REGDEF_BIT(, 14)
REGDEF_END(PWM_CLKDIV_LOAD_INTEN)               /* CLKDIV load done interrupt Control Register end */

#define PWM_CTRL_INTSTS_OFS0  0xF0
REGDEF_BEGIN(PWM_CTRL_INTSTS)               /* PWM interrupt status Register, PWMx_DONE_INTSTS */
REGDEF_BIT(pwm0_done_intsts, 1)
REGDEF_BIT(pwm1_done_intsts, 1)
REGDEF_BIT(pwm2_done_intsts, 1)
REGDEF_BIT(pwm3_done_intsts, 1)

REGDEF_BIT(pwm4_done_intsts, 1)
REGDEF_BIT(pwm5_done_intsts, 1)
REGDEF_BIT(pwm6_done_intsts, 1)
REGDEF_BIT(pwm7_done_intsts, 1)

REGDEF_BIT(pwm8_done_intsts, 1)
REGDEF_BIT(pwm9_done_intsts, 1)
REGDEF_BIT(pwm10_done_intsts, 1)
REGDEF_BIT(pwm11_done_intsts, 1)

REGDEF_BIT(, 20)
REGDEF_END(PWM_CTRL_INTSTS)                    /* PWM interrupt status Register, PWMx_DONE_INTSTS end */

#define PWM_MS_CTRL_INTSTS_OFS0  0xF4
REGDEF_BEGIN(PWM_MS_CTRL_INTSTS)           /* Micro step interrupt status Register, control MSx_DONE_INTSTS */
REGDEF_BIT(ms0_done_intsts, 1)
REGDEF_BIT(ms1_done_intsts, 1)
REGDEF_BIT(ms2_done_intsts, 1)
REGDEF_BIT(ms3_done_intsts, 1)

REGDEF_BIT(ms4_done_intsts, 1)
REGDEF_BIT(ms5_done_intsts, 1)
REGDEF_BIT(ms6_done_intsts, 1)
REGDEF_BIT(ms7_done_intsts, 1)

REGDEF_BIT(, 24)
REGDEF_END(PWM_MS_CTRL_INTSTS)                  /* Micro step interrupt Control Register, control MSx_DONE_INTSTS end */

#define PWM_CCNT_CTRL_INTSTS_OFS0  0xF8
REGDEF_BEGIN(PWM_CCNT_CTRL_INTSTS)          /* CCNT  interrupt status Register */
REGDEF_BIT(ccnt0_edge_intsts, 1)
REGDEF_BIT(ccnt0_trig_intsts, 1)
REGDEF_BIT(ccnt0_tout_intsts, 1)
REGDEF_BIT(, 1)

REGDEF_BIT(ccnt1_edge_intsts, 1)
REGDEF_BIT(ccnt1_trig_intsts, 1)
REGDEF_BIT(ccnt1_tout_intsts, 1)
REGDEF_BIT(, 1)

REGDEF_BIT(ccnt2_edge_intsts, 1)
REGDEF_BIT(ccnt2_trig_intsts, 1)
REGDEF_BIT(ccnt2_tout_intsts, 1)
REGDEF_BIT(, 1)

REGDEF_BIT(, 20)
REGDEF_END(PWM_CCNT_CTRL_INTSTS)                /* CCNT  interrupt status Register end */

#define PWM_CLKDIV_LOAD_STS_OFS0  0xFC
REGDEF_BEGIN(PWM_CLKDIV_LOAD_STS)          /* Clock load done status */
REGDEF_BIT(pwm_00_03_clkdiv_load_done_intsts, 1)
REGDEF_BIT(pwm_04_07_clkdiv_load_done_intsts, 1)
REGDEF_BIT(, 14)
REGDEF_BIT(pwm_00_03_tgt_cnt_done_intsts, 1)
REGDEF_BIT(pwm_04_07_tgt_cnt_done_intsts, 1)
REGDEF_BIT(, 14)

REGDEF_END(PWM_CLKDIV_LOAD_STS)                /* Clock load done status end */



#define PWM_ENABLE_OFS  0x100
REGDEF_BEGIN(PWM_ENABLE)                        /* PWM enable  register */
REGDEF_BIT(pwm0_en, 1)
REGDEF_BIT(pwm1_en, 1)
REGDEF_BIT(pwm2_en, 1)
REGDEF_BIT(pwm3_en, 1)

REGDEF_BIT(pwm4_en, 1)
REGDEF_BIT(pwm5_en, 1)
REGDEF_BIT(pwm6_en, 1)
REGDEF_BIT(pwm7_en, 1)

REGDEF_BIT(pwm8_en, 1)
REGDEF_BIT(pwm9_en, 1)
REGDEF_BIT(pwm10_en, 1)
REGDEF_BIT(pwm11_en, 1)

REGDEF_BIT(, 20)
REGDEF_END(PWM_ENABLE)                          /* PWM enable  register end */

#define PWM_DISABLE_OFS  0x104
REGDEF_BEGIN(PWM_DISABLE)                       /* PWM disable  register */
REGDEF_BIT(pwm0_dis, 1)
REGDEF_BIT(pwm1_dis, 1)
REGDEF_BIT(pwm2_dis, 1)
REGDEF_BIT(pwm3_dis, 1)

REGDEF_BIT(pwm4_dis, 1)
REGDEF_BIT(pwm5_dis, 1)
REGDEF_BIT(pwm6_dis, 1)
REGDEF_BIT(pwm7_dis, 1)

REGDEF_BIT(pwm8_dis, 1)
REGDEF_BIT(pwm9_dis, 1)
REGDEF_BIT(pwm10_dis, 1)
REGDEF_BIT(pwm11_dis, 1)

REGDEF_BIT(, 20)
REGDEF_END(PWM_DISABLE)                         /* PWM disable  register end */

#define PWM_LOAD_OFS  0x108
REGDEF_BEGIN(PWM_LOAD)                          /* PWM load  register */
REGDEF_BIT(pwm0_load, 1)
REGDEF_BIT(pwm1_load, 1)
REGDEF_BIT(pwm2_load, 1)
REGDEF_BIT(pwm3_load, 1)

REGDEF_BIT(pwm4_load, 1)
REGDEF_BIT(pwm5_load, 1)
REGDEF_BIT(pwm6_load, 1)
REGDEF_BIT(pwm7_load, 1)

REGDEF_BIT(pwm8_load, 1)
REGDEF_BIT(pwm9_load, 1)
REGDEF_BIT(pwm10_load, 1)
REGDEF_BIT(pwm11_load, 1)

REGDEF_BIT(, 20)
REGDEF_END(PWM_LOAD)                            /* PWM load register end */

#define PWM_CLKDIV_LOAD_OFS  0x10C
REGDEF_BEGIN(PWM_CLKDIV_LOAD)                   /* PWM clock div load  register */
REGDEF_BIT(pwm00_03_clkdiv_load, 1)
REGDEF_BIT(pwm04_07_clkdiv_load, 1)
REGDEF_BIT(, 30)
REGDEF_END(PWM_CLKDIV_LOAD)

#define PWM_MS_START_OFS  0x110
REGDEF_BEGIN(PWM_MS_START)                      /* PWM micro step start register */
REGDEF_BIT(ms0_start, 1)
REGDEF_BIT(ms1_start, 1)
REGDEF_BIT(ms2_start, 1)
REGDEF_BIT(ms3_start, 1)

REGDEF_BIT(ms4_start, 1)
REGDEF_BIT(ms5_start, 1)
REGDEF_BIT(ms6_start, 1)
REGDEF_BIT(ms7_start, 1)

REGDEF_BIT(, 24)
REGDEF_END(PWM_MS_START)                       /* PWM micro step start register end */

#define PWM_MS_STOP_OFS  0x114
REGDEF_BEGIN(PWM_MS_STOP)                      /* PWM micro step stop register */
REGDEF_BIT(ms0_stop, 1)
REGDEF_BIT(ms1_stop, 1)
REGDEF_BIT(ms2_stop, 1)
REGDEF_BIT(ms3_stop, 1)

REGDEF_BIT(ms4_stop, 1)
REGDEF_BIT(ms5_stop, 1)
REGDEF_BIT(ms6_stop, 1)
REGDEF_BIT(ms7_stop, 1)

REGDEF_BIT(, 24)
REGDEF_END(PWM_MS_STOP)                         /* PWM micro step stop register end */

#define PWM_TGT_CNT_ENABLE_OFS  0x118
REGDEF_BEGIN(PWM_TGT_CNT_ENABLE)                /* PWM target count enable register */
REGDEF_BIT(pwm00_03_target_cnt_en, 1)
REGDEF_BIT(pwm04_07_target_cnt_en, 1)
REGDEF_BIT(, 30)
REGDEF_END(PWM_TGT_CNT_ENABLE)                  /* PWM target count enable register end */

#define PWM_TGT_CNT_DISABLE_OFS  0x11C
REGDEF_BEGIN(PWM_TGT_CNT_DISABLE)               /* PWM target count disable register */
REGDEF_BIT(pwm00_03_target_cnt_dis, 1)
REGDEF_BIT(pwm04_07_target_cnt_dis, 1)
REGDEF_BIT(, 30)
REGDEF_END(PWM_TGT_CNT_DISABLE)                 /* PWM target count disable register end */


#define PWM_CCNT0_CONFIG_OFS             0x120
#define PWM_CCNT0_START_COUNT_VAL_OFS    0x124
#define PWM_CCNT0_TRIGGER_VAL_OFS        0x128
#define PWM_CCNT0_CONTROL_OFS            0x12C
#define PWM_CCNT0_CURRENT_VAL_OFS        0x130
#define PWM_CCNT0_EDGE_INTERVAL_OFS      0x134

#define PWM_CCNT1_CONFIG_OFS             0x140
#define PWM_CCNT1_START_COUNT_VAL_OFS    0x144
#define PWM_CCNT1_TRIGGER_VAL_OFS        0x148
#define PWM_CCNT1_CONTROL_OFS            0x14C
#define PWM_CCNT1_CURRENT_VAL_OFS        0x150
#define PWM_CCNT1_EDGE_INTERVAL_OFS      0x154

#define PWM_CCNT2_CONFIG_OFS             0x160
#define PWM_CCNT2_START_COUNT_VAL_OFS    0x164
#define PWM_CCNT2_TRIGGER_VAL_OFS        0x168
#define PWM_CCNT2_CONTROL_OFS            0x16C
#define PWM_CCNT2_CURRENT_VAL_OFS        0x170
#define PWM_CCNT2_EDGE_INTERVAL_OFS      0x174

#define PWM_CCNT_CONFIG_OFS  0x120
REGDEF_BEGIN(PWM_CCNT_CONFIG)                   /* Cycle Count Calculator */
REGDEF_BIT(ccnt_filter, 8)                  /* Cycle Count glitch filter register */
REGDEF_BIT(, 8)                             /* Reserved bit 8~15 */
REGDEF_BIT(ccnt_decrease, 1)                /* Cycle Count increase or decrease */
REGDEF_BIT(ccnt_inv, 1)                     /* Cycle Count invert source signal */
REGDEF_BIT(ccnt_mode, 1)                    /* Cycle Count mode selection */
REGDEF_BIT(, 1)                             /* Reserved bit 19 */
REGDEF_BIT(ccnt_signal_source, 1)           /* Cycle Count signal source selection */
REGDEF_BIT(, 11)                            /* Reserved bit 21~31 */
REGDEF_END(PWM_CCNT_CONFIG)                     /* Cycle Count Calculator end */

#define PWM_CCNT_START_COUNT_VAL_OFS  0x124
REGDEF_BEGIN(PWM_CCNT_START_COUNT_VAL)          /* Cycle Count starting count value register */
REGDEF_BIT(ccnt_sta_val, 16)                /* Cycle Count starting count value */
REGDEF_BIT(, 16)                            /* Reserved bit 16~31 */
REGDEF_END(PWM_CCNT_START_COUNT_VAL)            /* Cycle Count starting count value register end */

#define PWM_CCNT_TRIGGER_VAL_OFS  0x128
REGDEF_BEGIN(PWM_CCNT_TRIGGER_VAL)              /* Cycle Count trigger value register */
REGDEF_BIT(ccnt_trig_val, 16)               /* Cycle Count trigger value register */
REGDEF_BIT(, 16)                            /* Reserved bit 16~31 */
REGDEF_END(PWM_CCNT_TRIGGER_VAL)                /* Cycle Count Calculator end */

#define PWM_CCNT_CONTROL_OFS  0x12C
REGDEF_BEGIN(PWM_CCNT_CONTROL)                  /* Cycle Count controller reigster */
REGDEF_BIT(ccnt_time_en, 1)                 /* Cycle Count timeout enable bit */
REGDEF_BIT(, 15)                            /* Reserved bit 1~15 */
REGDEF_BIT(ccnt_time_cnt, 16)               /* Cycle timeout count register  */
REGDEF_END(PWM_CCNT_CONTROL)                    /* Cycle Count controller reigster end */

#define PWM_CCNT_CURRENT_VAL_OFS  0x130
REGDEF_BEGIN(PWM_CCNT_CURRENT_VAL)              /* Cycle Count current value reigster */
REGDEF_BIT(ccnt_current_val, 16)            /* Cycle count current value */
REGDEF_BIT(, 16)                            /* Reserved bit 1~15 */
REGDEF_END(PWM_CCNT_CURRENT_VAL)                /* Cycle Count controller reigster end */

#define PWM_CCNT_EDGE_INTERVAL_OFS  0x134
REGDEF_BEGIN(PWM_CCNT_EDGE_INTERVAL)            /* Cycle Count edge interval register */
REGDEF_BIT(ccnt_current_val, 16)            /* Cycle count edge interval value */
REGDEF_BIT(, 16)                            /* Reserved bit 16~31 */
REGDEF_END(PWM_CCNT_EDGE_INTERVAL)              /* Cycle Count controller reigster end */


#define PWM_CCNT_ENABLE_OFS  0x200
REGDEF_BEGIN(PWM_CCNT_ENABLE)                   /* Cycle Count enable  register */
REGDEF_BIT(ccnt0_en, 1)
REGDEF_BIT(ccnt1_en, 1)
REGDEF_BIT(ccnt2_en, 1)
REGDEF_BIT(, 29)
REGDEF_END(PWM_CCNT_ENABLE)                     /* Cycle Count enable  register end*/

#define PWM_CCNT_LOAD_OFS  0x204
REGDEF_BEGIN(PWM_CCNT_LOAD)                     /* Cycle Count load  register */
REGDEF_BIT(ccnt0_load, 1)
REGDEF_BIT(ccnt1_load, 1)
REGDEF_BIT(ccnt2_load, 1)
REGDEF_BIT(, 29)
REGDEF_END(PWM_CCNT_LOAD)                       /* Cycle Count load  register end*/

#define PWM_TGT_CNT_REG0_OFS  0x210
REGDEF_BEGIN(PWM_TGT_CNT_REG0)                  /* PWM target count register 0*/
REGDEF_BIT(pwm00_03_target_cnt, 16)
REGDEF_BIT(, 16)
REGDEF_END(PWM_TGT_CNT_REG0)                    /* PWM target count register 0 end*/

#define PWM_TGT_CNT_REG1_OFS  0x214
REGDEF_BEGIN(PWM_TGT_CNT_REG1)                  /* PWM target count register 1*/
REGDEF_BIT(pwm04_07_target_cnt, 16)
REGDEF_BIT(, 16)
REGDEF_END(PWM_TGT_CNT_REG1)                    /* PWM target count register 1end*/

#define PWM0_EXPEND_PERIOD_REG_BUF_OFS  0x230
#define PWM1_EXPEND_PERIOD_REG_BUF_OFS  0x234
#define PWM2_EXPEND_PERIOD_REG_BUF_OFS  0x238
#define PWM3_EXPEND_PERIOD_REG_BUF_OFS  0x23C
#define PWM4_EXPEND_PERIOD_REG_BUF_OFS  0x240
#define PWM5_EXPEND_PERIOD_REG_BUF_OFS  0x244
#define PWM6_EXPEND_PERIOD_REG_BUF_OFS  0x248
#define PWM7_EXPEND_PERIOD_REG_BUF_OFS  0x24C
REGDEF_BEGIN(PWM_EXPEBD_PERIOD_REG_BUF)         /* PWM Period Register Buf */
REGDEF_BIT(pwm_r, 8)                        /* PWM rising time, the number of base clock cycles. */
REGDEF_BIT(pwm_f, 8)                        /* PWM falling time, the number of base clock cycles. */
REGDEF_BIT(pwm_prd, 8)                      /* PWM basis period, the number of clock cycles.  */
REGDEF_BIT(, 8)

REGDEF_END(PWM_EXPEBD_PERIOD_REG_BUF)           /* PWM Period Register Buf end */

#define PWM_DEBUG_PORT_OFS  0x3FC
REGDEF_BEGIN(PWM_DEBUG_PORT)
REGDEF_BIT(debug_sel, 4)
REGDEF_BIT(, 28)
REGDEF_END(PWM_DEBUG_PORT)


//-------------------------------------------------------
//  PWM registers for NA51084
//-------------------------------------------------------
/* PWM Interrupt Destination Register0 */
#define PWM_TO_CPU1_OFS  0x250
REGDEF_BEGIN(PWM_TO_CPU1)
REGDEF_BIT(pwm0_cpu1, 1)
REGDEF_BIT(pwm1_cpu1, 1)
REGDEF_BIT(pwm2_cpu1, 1)
REGDEF_BIT(pwm3_cpu1, 1)
REGDEF_BIT(pwm4_cpu1, 1)
REGDEF_BIT(pwm5_cpu1, 1)
REGDEF_BIT(pwm6_cpu1, 1)
REGDEF_BIT(pwm7_cpu1, 1)
REGDEF_BIT(pwm8_cpu1, 1)
REGDEF_BIT(pwm9_cpu1, 1)
REGDEF_BIT(pwm10_cpu1, 1)
REGDEF_BIT(pwm11_cpu1, 1)
REGDEF_BIT(pwm12_cpu1, 1)
REGDEF_BIT(pwm13_cpu1, 1)
REGDEF_BIT(pwm14_cpu1, 1)
REGDEF_BIT(, 17)
REGDEF_END(PWM_TO_CPU1)

/* PWM Interrupt Destination Register1 */
#define PWM_TO_CPU2_OFS  0x254
REGDEF_BEGIN(PWM_TO_CPU2)
REGDEF_BIT(pwm0_cpu2, 1)
REGDEF_BIT(pwm1_cpu2, 1)
REGDEF_BIT(pwm2_cpu2, 1)
REGDEF_BIT(pwm3_cpu2, 1)
REGDEF_BIT(pwm4_cpu2, 1)
REGDEF_BIT(pwm5_cpu2, 1)
REGDEF_BIT(pwm6_cpu2, 1)
REGDEF_BIT(pwm7_cpu2, 1)
REGDEF_BIT(pwm8_cpu2, 1)
REGDEF_BIT(pwm9_cpu2, 1)
REGDEF_BIT(pwm10_cpu2, 1)
REGDEF_BIT(pwm11_cpu2, 1)
REGDEF_BIT(pwm12_cpu2, 1)
REGDEF_BIT(pwm13_cpu2, 1)
REGDEF_BIT(pwm14_cpu2, 1)
REGDEF_BIT(, 17)
REGDEF_END(PWM_TO_CPU2)

/* PWM Interrupt Control Register1, control PWMx_DONE_INTEN */
#define PWM_CTRL_INTEN_OFS1  0x258
REGDEF_BEGIN(PWM_CTRL_INTEN1)
REGDEF_BIT(pwm0_done_inten, 1)
REGDEF_BIT(pwm1_done_inten, 1)
REGDEF_BIT(pwm2_done_inten, 1)
REGDEF_BIT(pwm3_done_inten, 1)

REGDEF_BIT(pwm4_done_inten, 1)
REGDEF_BIT(pwm5_done_inten, 1)
REGDEF_BIT(pwm6_done_inten, 1)
REGDEF_BIT(pwm7_done_inten, 1)

REGDEF_BIT(pwm8_done_inten, 1)
REGDEF_BIT(pwm9_done_inten, 1)
REGDEF_BIT(pwm10_done_inten, 1)
REGDEF_BIT(pwm11_done_inten, 1)

REGDEF_BIT(, 20)
REGDEF_END(PWM_CTRL_INTEN1)

/* Micro step Interrupt Control Register1, control MSx_DONE_INTEN */
#define PWM_MS_CTRL_INTEN_OFS1  0x25C
REGDEF_BEGIN(PWM_MS_CTRL_INTEN1)
REGDEF_BIT(ms0_done_inten, 1)
REGDEF_BIT(ms1_done_inten, 1)
REGDEF_BIT(ms2_done_inten, 1)
REGDEF_BIT(ms3_done_inten, 1)

REGDEF_BIT(ms4_done_inten, 1)
REGDEF_BIT(ms5_done_inten, 1)
REGDEF_BIT(ms6_done_inten, 1)
REGDEF_BIT(ms7_done_inten, 1)

REGDEF_BIT(, 24)
REGDEF_END(PWM_MS_CTRL_INTEN1)

/* CCNT Interrupt Control Register1 */
#define PWM_CCNT_CTRL_INTEN_OFS1  0x260
REGDEF_BEGIN(PWM_CCNT_CTRL_INTEN1)
REGDEF_BIT(ccnt0_edge_inten, 1)
REGDEF_BIT(ccnt0_trig_inten, 1)
REGDEF_BIT(ccnt0_tout_inten, 1)
REGDEF_BIT(, 1)

REGDEF_BIT(ccnt1_edge_inten, 1)
REGDEF_BIT(ccnt1_trig_inten, 1)
REGDEF_BIT(ccnt1_tout_inten, 1)
REGDEF_BIT(, 1)

REGDEF_BIT(ccnt2_edge_inten, 1)
REGDEF_BIT(ccnt2_trig_inten, 1)
REGDEF_BIT(ccnt2_tout_inten, 1)
REGDEF_BIT(, 1)

REGDEF_BIT(, 20)
REGDEF_END(PWM_CCNT_CTRL_INTEN1)

/* CLKDIV load done Interrupt Control Register1 */
#define PWM_CLKDIV_LOAD_INTEN_OFS1  0x264
REGDEF_BEGIN(PWM_CLKDIV_LOAD_INTEN1)
REGDEF_BIT(clkdiv_00_03_load_inten,    1)
REGDEF_BIT(clkdiv_04_07_load_inten,    1)
REGDEF_BIT(, 14)
REGDEF_BIT(clkdiv_00_03_tgt_cnt_inten, 1)
REGDEF_BIT(clkdiv_04_07_tgt_cnt_inten, 1)
REGDEF_BIT(, 14)
REGDEF_END(PWM_CLKDIV_LOAD_INTEN1)

/* PWM Interrupt Status Register1, control PWMx_DONE_INTSTS */
#define PWM_CTRL_INTSTS_OFS1  0x268
REGDEF_BEGIN(PWM_CTRL_INTSTS1)
REGDEF_BIT(pwm0_done_intsts, 1)
REGDEF_BIT(pwm1_done_intsts, 1)
REGDEF_BIT(pwm2_done_intsts, 1)
REGDEF_BIT(pwm3_done_intsts, 1)

REGDEF_BIT(pwm4_done_intsts, 1)
REGDEF_BIT(pwm5_done_intsts, 1)
REGDEF_BIT(pwm6_done_intsts, 1)
REGDEF_BIT(pwm7_done_intsts, 1)

REGDEF_BIT(pwm8_done_intsts, 1)
REGDEF_BIT(pwm9_done_intsts, 1)
REGDEF_BIT(pwm10_done_intsts, 1)
REGDEF_BIT(pwm11_done_intsts, 1)

REGDEF_BIT(, 20)
REGDEF_END(PWM_CTRL_INTSTS1)

/* Micro step Interrupt Status Register1, control MSx_DONE_INTSTS */
#define PWM_MS_CTRL_INTSTS_OFS1  0x26C
REGDEF_BEGIN(PWM_MS_CTRL_INTSTS1)
REGDEF_BIT(ms0_done_intsts, 1)
REGDEF_BIT(ms1_done_intsts, 1)
REGDEF_BIT(ms2_done_intsts, 1)
REGDEF_BIT(ms3_done_intsts, 1)

REGDEF_BIT(ms4_done_intsts, 1)
REGDEF_BIT(ms5_done_intsts, 1)
REGDEF_BIT(ms6_done_intsts, 1)
REGDEF_BIT(ms7_done_intsts, 1)

REGDEF_BIT(, 24)
REGDEF_END(PWM_MS_CTRL_INTSTS1)

/* CCNT Interrupt Status Register1 */
#define PWM_CCNT_CTRL_INTSTS_OFS1  0x270
REGDEF_BEGIN(PWM_CCNT_CTRL_INTSTS1)
REGDEF_BIT(ccnt0_edge_intsts, 1)
REGDEF_BIT(ccnt0_trig_intsts, 1)
REGDEF_BIT(ccnt0_tout_intsts, 1)
REGDEF_BIT(, 1)

REGDEF_BIT(ccnt1_edge_intsts, 1)
REGDEF_BIT(ccnt1_trig_intsts, 1)
REGDEF_BIT(ccnt1_tout_intsts, 1)
REGDEF_BIT(, 1)

REGDEF_BIT(ccnt2_edge_intsts, 1)
REGDEF_BIT(ccnt2_trig_intsts, 1)
REGDEF_BIT(ccnt2_tout_intsts, 1)
REGDEF_BIT(, 1)

REGDEF_BIT(, 20)
REGDEF_END(PWM_CCNT_CTRL_INTSTS1)

/* CLKDIV load done Interrupt Status Register1 */
#define PWM_CLKDIV_LOAD_STS_OFS1  0x274
REGDEF_BEGIN(PWM_CLKDIV_LOAD_STS1)
REGDEF_BIT(pwm_00_03_clkdiv_load_done_intsts, 1)
REGDEF_BIT(pwm_04_07_clkdiv_load_done_intsts, 1)
REGDEF_BIT(, 14)
REGDEF_BIT(pwm_00_03_tgt_cnt_done_intsts, 1)
REGDEF_BIT(pwm_04_07_tgt_cnt_done_intsts, 1)
REGDEF_BIT(, 14)
REGDEF_END(PWM_CLKDIV_LOAD_STS1)

#endif
