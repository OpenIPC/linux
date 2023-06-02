/**
    @file       pwm.h
    @ingroup    mIDrvIO_PWM

    @brief      Header file for PWM module
				This file is the header file that define the API for PWM module

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.

*/
#ifndef _PWM_H
#define _PWM_H

#ifdef __KERNEL__
#include "kwrap/type.h"
#elif defined(__FREERTOS)
#include "kwrap/type.h"
#else
#include "type.h"
#endif


/**
    @addtogroup mIDrvIO_PWM
*/
//@{
/**
    PWM micro step direction

    Micro step direction configuration

    @note for pwm_mstep_config() pwm_mstep_config_set()
*/
typedef enum {
	MS_DIR_INCREASE = 0x0,          ///< CounterClockwise
	MS_DIR_DECREASE,                ///< Clockwise
	MS_DIR_CNT,
	ENUM_DUMMY4WORD(PWM_MS_DIR)
} PWM_MS_DIR;


/**
    PWM type (PWM or Micro step)
*/
typedef enum {
	PWM_TYPE_PWM = 0x0,             ///< PWM
	PWM_TYPE_MICROSTEP,             ///< Micro step
	PWM_TYPE_CCNT,                  ///< CCNT
	PWM_TYPE_CNT,
	ENUM_DUMMY4WORD(PWM_TYPE)
} PWM_TYPE;


/**
    PWM clock divid

    @note for pwm_pwm_config_clock_div()
*/
typedef enum {
	PWM0_3_CLKDIV   = 0x0,          ///< PWM0~PWM3 clock divid
	PWM4_7_CLKDIV,                  ///< PWM4~PWM7 clock divid
	PWM8_CLKDIV    = 8,             ///< PWM8 clock divid
	PWM9_CLKDIV,                    ///< PWM9 clock divid
	PWM10_CLKDIV,                   ///< PWM10 clock divid
	PWM11_CLKDIV,                   ///< PWM11 clock divid
	ENUM_DUMMY4WORD(PWM_CLOCK_DIV)
} PWM_CLOCK_DIV;


/**
    PWM micro step phase type (1-2 or 2-2 phase)

    @note for pwm_mstep_config() pwm_mstep_config_set()
*/
typedef enum {
	PWM_MS_1_2_PHASE_TYPE = 0x0,    ///< 1 unit each operation
	PWM_MS_2_2_PHASE_TYPE,          ///< 2 unit each operation
	PWM_MS_PHASE_TYPE_CNT,
	ENUM_DUMMY4WORD(PWM_MS_PHASE_TYPE)
} PWM_MS_PHASE_TYPE;


/**
    PWM Micro step step per phase

    PWM Micro step step per phase configuration

    @note for pwm_mstep_config() pwm_mstep_config_set()
*/
typedef enum {
	TOTAL_08_STEP_PER_PHASE = 8,    ///< 8  steps each phase
	TOTAL_16_STEP_PER_PHASE = 16,   ///< 16 steps each phase
	TOTAL_32_STEP_PER_PHASE = 32,   ///< 32 steps each phase
	TOTAL_64_STEP_PER_PHASE = 64,   ///< 64 steps each phase
	ENUM_DUMMY4WORD(PWM_MS_STEP_PER_PHASE)
} PWM_MS_STEP_PER_PHASE;


/**
    PWM Micro step channel level

    PWM Micro step channel level configuation
*/
typedef enum {
	PWM_MS_CHANNEL_LEVEL_LOW = 0x0, ///< PWM MS channel level low
	PWM_MS_CHANNEL_LEVEL_HIGH,      ///< PWM MS channel level high

	ENUM_DUMMY4WORD(PWM_MS_CHANNEL_LEVEL)
} PWM_MS_CHANNEL_LEVEL;



/**
    PWM Micro step channel set

    PWM Micro step channel set definition

    @note for pwm_open_set, pwm_close_set, pwm_wait_set(), pwm_configClockDivSet(), pwm_mstep_config_set(),\n
				pwm_pwm_enable_set(), pwm_pwm_disable_set(), pwm_mstep_enable_set(), pwm_mstep_disable_set()
*/
typedef enum {
	PWM_MS_SET_0 = 0x0,             ///< PWM MS channel set 0, including PWM0-3
	PWM_MS_SET_1,                   ///< PWM MS channel set 1, including PWM4-7
	PWM_MS_SET_TOTAL,

	ENUM_DUMMY4WORD(PWM_MS_CHANNEL_SET)
} PWM_MS_CHANNEL_SET;

/**
    Micro step target count set(s)

    @note for pwm_mstep_config_target_count_enable()
*/
typedef enum {
	PWM_00_03_TGT_CNT   = 0x0,          ///< PWM0~PWM3 target count
	PWM_04_07_TGT_CNT,                  ///< PWM4~PWM7 target count

	PWM_TGT_CNT_NUM,

	ENUM_DUMMY4WORD(PWM_TGT_COUNT)
} PWM_TGT_COUNT;

/**
    PWM CCNT count down policy

    @note   for pwm_ccnt_config()
*/
typedef enum {
	PWM_CCNT_COUNT_INCREASE = 0x0,  ///< PWM cycle count increase
	PWM_CCNT_COUNT_DECREASE,        ///< PWM cycle count decrease
	PWM_CCNT_COUNT_POLICY_CNT,
	ENUM_DUMMY4WORD(PWM_CCNT_COUNTDOWN)
} PWM_CCNT_COUNTDOWN;


/**
    PWM CCNT signal source

    @note   for pwm_ccnt_config()
*/
typedef enum {
	PWM_CCNT_SIGNAL_GPIO = 0x0,                 ///< Signal from GPIO (default value)
	PWM_CCNT_SIGNAL_ADC,                        ///< Signal from ADC channel( CCNT0 & 2 from ADC ch1 / CCNT1 & 3 from ADC ch2)
	PWM_CCNT_SIGNAL_CNT,
	ENUM_DUMMY4WORD(PWM_CCNT_SIGNAL_SOURCE)
} PWM_CCNT_SIGNAL_SOURCE;

/**
    PWM CCNT interrupt trigger event

    @note   for pwm_ccnt_config()
*/
typedef enum {
	PWM_CCNT_EDGE_TRIG_INTEN            = 0x1,  ///< CCNT target value arrived interrupt
	PWM_CCNT_TAGT_TRIG_INTEN            = 0x2,  ///< CCNT edge triggerd interrupt\n
	///< (depend on ui_count_mode set PWM_CCNT_MODE_PULSE or PWM_CCNT_MODE_EDGE)
	///< @note PWM_CCNT_MODE_PULSE, PWM_CCNT_MODE_EDGE
	PWM_CCNT_TRIG_INTEN_CNT,
	ENUM_DUMMY4WORD(PWM_CCNT_TRIG_INTERRUPT)
} PWM_CCNT_TRIG_INTERRUPT;

/**
    PWM CCNT interrupt trigger event

    @note   for pwm_ccnt_config()
*/
typedef enum {
	PWM_CCNT_EDGE_TRIG                  = 0x1,  ///< CCNT target value arrived trigger
	PWM_CCNT_TAGT_TRIG                  = 0x2,  ///< CCNT edge trigger
	///< @note PWM_CCNT_MODE_PULSE,PWM_CCNT_MODE_EDGE
	PWM_CCNT_TOUT_TRIG                  = 0x4,  ///< CCNT time out trigger
	PWM_CCNT_TRIG_CNT,
	ENUM_DUMMY4WORD(PWM_CCNT_TRIG_TYPE)
} PWM_CCNT_TRIG_TYPE;

/**
    PWM destination

    @note   for pwm_set_destination()
*/
typedef enum {
	PWM_DEST_TO_CPU1                    = 0x1,  ///< PWM interrupt destination to CPU1
	PWM_DEST_TO_CPU2                    = 0x2,  ///< PWM interrupt destination to CPU2
	ENUM_DUMMY4WORD(PWM_DEST)
} PWM_DEST;

/**
    PWM configuration identifier

    @note For pwm_set_config()
*/
typedef enum {
	PWM_CONFIG_ID_AUTOPINMUX,                   ///< Context can be one of the following:
	///< - @b TRUE  : disable pinmux when pwm driver close
	///< - @b FALSE : not disable pinmux when pwm driver close

	PWM_CONFIG_ID_PWM_DEBUG,                    ///< Enable/Disable pwm debug messages

	PWM_CONFIG_ID_PWM_OPEN_DESTINATION,         ///< The pwm channel will set this destination when pwm_open;
	                                            ///< PWM_DEST_TO_CPU1 : destination to CPU1 (defalut)
	                                            ///< PWM_DEST_TO_CPU2 : destination to CPU2

	PWM_CONFIG_ID_DISABLE_CCNT_AFTER_TARGET,    ///< Disable ccnt after target value trigger

#if defined(__FREERTOS)
	PWM_CONFIG_ID_PWM_REQUEST_IRQ,              ///< Force to request_irq;
	                                            ///< PWM_DEST_TO_CPU1 : request pwm_isr
	                                            ///< PWM_DEST_TO_CPU2 : request pwm_isr2

	PWM_CONFIG_ID_PWM_FREE_IRQ,                 ///< Force to free_irq;
	                                            ///< PWM_DEST_TO_CPU1 : free pwm_isr
	                                            ///< PWM_DEST_TO_CPU2 : free pwm_isr2
#endif

	ENUM_DUMMY4WORD(PWM_CONFIG_ID)
} PWM_CONFIG_ID;


/**
    PWM configuration structure

    @note for pwm_set()
*/
typedef struct {
#if 0
	UINT32  ui_div;                              ///< Clock divider, PWM clock = 120MHZ / (ui_div + 1), ui_div = 3 ~ 16383, min value = 3
	///< @note 1. ui_div MUST >= 3, if ui_div < 3, driver will force set as 3
	///< \n2. PWM0-3, PWM4-7 and PWM8-11 are use same clock source respectively
	///< \n3. Others(PWM12-19) use dedicate clock source respectively
#endif
	UINT32  ui_prd;                              ///< Base period, how many PWM clock per period, 2 ~ 255
	///< @note ui_rise <= ui_fall <= ui_prd
	UINT32  ui_rise;                             ///< Rising at which clock
	///< @note ui_rise <= ui_fall <= ui_prd
	UINT32  ui_fall;                             ///< Falling at which clock
	///< @note ui_rise <= ui_fall <= ui_prd
	UINT32  ui_on_cycle;                          ///< Output cycle, 0 ~ 65535
	///< - @b PWM_FREE_RUN: Free Run
	///< - @b Others: How many cycles (PWM will stop after output the cycle counts)
	UINT32  ui_inv;                              ///< Invert PWM output signal or not
	///< - @b PWM_SIGNAL_NORMAL: Don't invert PWM output signal
	///< - @b PWM_SIGNAL_INVERT: Invert PWM output signal
} PWM_CFG, *PPWM_CFG;


/**
    PWM CCNT configuration structure
*/
typedef struct {
	UINT32  ui_start_value;                       ///< CCNT start count value, 0 ~ 0xFFFF (Starting count value)
	UINT32  ui_trigger_value;                     ///< CCNT target trigger value, 0 ~ 0xFFFF (trigger count value)
	UINT32  ui_filter;                           ///<  Filter glitch of input signal, value is 0 ~ 255. If the glitch width is smaller than (CCNTx_FILTER * 8) + 1 cycles, then the glitch will be filtered out.
	UINT32  ui_inv;                              ///< Invert CCNT input signal or not
	///< - @b PWM_CCNT_SIGNAL_NORMAL: Don't invert CCNT input signal
	///< - @b PWM_CCNT_SIGNAL_INVERT: Invert CCNT input signal
	UINT32  ui_mode;                             ///< CCNT mode
	///< - @b PWM_CCNT_MODE_PULSE: Counting pulse only
	///< - @b PWM_CCNT_MODE_EDGE: Counting edge, both rising and falling edges
	PWM_CCNT_SIGNAL_SOURCE  ui_sig_src;           ///< CCNT signal source
	///< - @b PWM_CCNT_SIGNAL_GPIO  : Signal from GPIO (default value)
	///< - @b PWM_CCNT_SIGNAL_ADC   : Signal from ADC channel( CCNT0 & 3 from ADC ch1 / CCNT1 from ADC ch2 / CCNT2 from ADC ch6)
	PWM_CCNT_COUNTDOWN      ui_count_mode;        ///< CCNT count mode
	///< - @b PWM_CCNT_COUNT_INCREASE: The value of CCNT0_CURRENT_VAL will be increased by one when detecting one pulse or edge( by CCNT0_MODE)
	///< - @b PWM_CCNT_COUNT_DECREASE: The value of CCNT0_CURRENT_VAL will be decreased by one when detecting one pulse or edge( by CCNT0_MODE)
	PWM_CCNT_TRIG_INTERRUPT ui_trig_int_en;        ///< CCNT interrupt issue condition (can occurred mutiple condition at the same time)
	///<   @note PWM_CCNT_TRIG_INTERRUPT
	///< - @b TRUE  : Trigger at each edge(rising or both rasing & faling depend on ui_mode configuration
	///< - @b FALSE : Trigger at target value arrived.
} PWM_CCNT_CFG, *PPWM_CCNT_CFG;


/**
    PWM CCNT time out configuration structure

    @note for pwm_ccnt_enable()
*/
typedef struct {
	BOOL    ub_tout_en;                           ///< CCNT timeout enable
	UINT32  ui_tout_value;                        ///< CCNT timeout value
} PWM_CCNT_TOUT_CFG, *PPWM_CCNT_TOUT_CFG;


/**
    MSTP configuration structure

    @note for pwm_mstep_config()
*/
typedef struct {
	UINT32  ui_ph;                               ///< Micro step starting phase, phase 0 ~ 7
	UINT32  ui_dir;                              ///< Micro step moving direction ,
	///< - @b MS_DIR_INCREASE
	///< - @b MS_DIR_DECREASE
	UINT32  ui_on_cycle;                          ///< Number of phase for Micro step mode
	UINT32  ui_step_per_phase;                     ///< Step per phase
	///< - @b TOTAL_08_STEP_PER_PHASE (8 steps each phase)
	///< - @b TOTAL_16_STEP_PER_PHASE (16 steps each phase)
	///< - @b TOTAL_32_STEP_PER_PHASE (32 steps each phase)
	///< - @b TOTAL_64_STEP_PER_PHASE (64 steps each phase)
	UINT32  ui_phase_type;                        ///< 1-2 or 2-2 phase
	///< - @b PWM_MS_1_2_PHASE_TYPE (1 unit each operation)
	///< - @b PWM_MS_2_2_PHASE_TYPE (2 unit each operation)
	BOOL    ui_threshold_en;                      ///< Enable threshold filter(always set as FALSE), TRUE for test only
	UINT32  ui_threshold;                        ///< Max 0x63
    BOOL    is_square_wave;                      ///< square wave or micro stepping
	///< - @b TRUE (square)
	///< - @b FALSE (micro-stepping)
} MSTP_CFG, *PMSTP_CFG;


/**
    Micro step channels (within a set) phase configuration

    @note for pwm_mstep_config_set()
*/
typedef struct {
	UINT32  ui_ch0_phase;                       ///< Specific MS set channel 0 start phase
	UINT32  ui_ch1_phase;                       ///< Specific MS set channel 1 start phase
	UINT32  ui_ch2_phase;                       ///< Specific MS set channel 2 start phase
	UINT32  ui_ch3_phase;                       ///< Specific MS set channel 3 start phase

} MS_CH_PHASE_CFG, *PMS_CH_PHASE_CFG;

/**
    Micro step channels (within a set) level configuration

    @note for PWM_MS_CHANNEL_LEVEL_LOW or PWM_MS_CHANNEL_LEVEL_HIGH,
*/
typedef struct {
	UINT32  ui_ch0_level;                       ///< Specific MS channel 0 level
	UINT32  ui_ch1_level;                       ///< Specific MS channel 1 level
	UINT32  ui_ch2_level;                       ///< Specific MS channel 2 level
	UINT32  ui_ch3_level;                       ///< Specific MS channel 3 level

} MS_CH_LEVEL_CFG, *PMS_CH_LEVEL_CFG;


/**
    MSTP configuration structure

    @note for pwm_mstep_config_set()
*/
typedef struct {
	UINT32  ui_dir;                              ///< Micro step moving direction ,
	///< - @b MS_DIR_INCREASE
	///< - @b MS_DIR_DECREASE
	UINT32  ui_on_cycle;                          ///< Number of phase for Micro step mode
	UINT32  ui_step_per_phase;                     ///< Step per phase
	///< - @b TOTAL_08_STEP_PER_PHASE (8 steps each phase)
	///< - @b TOTAL_16_STEP_PER_PHASE (16 steps each phase)
	///< - @b TOTAL_32_STEP_PER_PHASE (32 steps each phase)
	///< - @b TOTAL_64_STEP_PER_PHASE (64 steps each phase)
	UINT32  ui_phase_type;                        ///< 1-2 or 2-2 phase
	///< - @b PWM_MS_1_2_PHASE_TYPE (1 unit each operation)
	///< - @b PWM_MS_2_2_PHASE_TYPE (2 unit each operation)
	BOOL    ui_threshold_en;                      ///< Enable threshold filter
	UINT32  ui_threshold;                        ///< Max 0x63
    BOOL    is_square_wave;                      ///< square wave or micro stepping
	///< - @b TRUE (square)
	///< - @b FALSE (micro-stepping)
} MSCOMMON_CFG, *PMSCOMMON_CFG;


/**
     @name PWM cycle type free run configuratopn
     @note pwm_set(), pwm_ms_set()
*/
//@{
#define PWM_FREE_RUN            0               ///< For pwm_set() API, output cycle is free run
//@}

/**
    @name PWM output signal mode

    Invert PWM output signal or not for pwm_set() API
*/
//@{
#define PWM_SIGNAL_NORMAL       0               ///< Don't invert PWM output signal
#define PWM_SIGNAL_INVERT       1               ///< Invert PWM output signal
//@}

/**
    @name CCNT input signal mode

    Invert CCNT input signal or not for pwm_setCCNT() API
*/
//@{
#define PWM_CCNT_SIGNAL_NORMAL  0               ///< Don't invert CCNT input signal
#define PWM_CCNT_SIGNAL_INVERT  1               ///< Invert CCNT input signal
//@}

/**
    @name PWM CCNT mode

    PWM CCNT mode for pwm_ccnt_config() API
*/
//@{
#define PWM_CCNT_MODE_PULSE     0               ///< Counting pulse only
///< Example:
///< @code
///  {
///        ____
///     ___|  |___
///     ==> count only raising edge => result is 1
///  }
///  @endcode
#define PWM_CCNT_MODE_EDGE      1               ///< Counting edge, both rising and falling edges
///< Example:
///< @code
///  {
///        ____
///     ___|  |___
///     ==> count both raising & faling edge => result is 2
///  }
///  @endcode
//@}

/**
    @name PWM ID

    PWM ID for PWM driver API

    @note for pwm_open(), pwm_set(), pwm_setCCNT(), pwm_setCCNTToutEN(), pwm_wait(), pwm_stop(), pwm_en(), pwm_reload(),\n
				pwm_ms_set(), pwm_ms_stop(), pwm_ms_en(), pwm_getCycleNumber(), pwm_ccnt_get_current_val(), pwm_ccnt_get_current_val(),\n
				pwm_ccnt_enable()
*/
//@{
#define PWMID_0                 0x00000001  ///< PWM ID 0
#define PWMID_1                 0x00000002  ///< PWM ID 1
#define PWMID_2                 0x00000004  ///< PWM ID 2
#define PWMID_3                 0x00000008  ///< PWM ID 3
#define PWMID_4                 0x00000010  ///< PWM ID 4
#define PWMID_5                 0x00000020  ///< PWM ID 5
#define PWMID_6                 0x00000040  ///< PWM ID 6
#define PWMID_7                 0x00000080  ///< PWM ID 7
#define PWMID_8                 0x00000100  ///< PWM ID 8
#define PWMID_9                 0x00000200  ///< PWM ID 9
#define PWMID_10                0x00000400  ///< PWM ID 10
#define PWMID_11                0x00000800  ///< PWM ID 11

#define PWMID_CCNT0             0x00001000  ///< PWM ID CCNT0
#define PWMID_CCNT1             0x00002000  ///< PWM ID CCNT1
#define PWMID_CCNT2             0x00004000  ///< PWM ID CCNT2
//@}

// PWM Driver API
extern ER       pwm_open(UINT32 ui_pwm_id);
extern ER       pwm_open_set(PWM_MS_CHANNEL_SET ui_ms_set);
extern UINT32   pwm_pwm_get_cycle_number(UINT32 ui_pwm_id);
extern ER       pwm_close(UINT32 ui_pwm_id, BOOL b_wait_auto_disable_done);
extern ER       pwm_close_set(PWM_MS_CHANNEL_SET ui_ms_set, BOOL b_wait_auto_disable_done);
extern ER       pwm_wait(UINT32 ui_pwm_id, PWM_TYPE ui_pwm_type);
extern ER       pwm_wait_set(PWM_MS_CHANNEL_SET ui_ms_set);

extern ER       pwm_pwm_reload(UINT32 ui_pwm_id);
extern ER       pwm_pwm_enable(UINT32 ui_pwm_id);
extern ER       pwm_pwm_disable(UINT32 ui_pwm_id);
extern ER       pwm_pwm_enable_set(PWM_MS_CHANNEL_SET ui_ms_set);
extern ER       pwm_pwm_disable_set(PWM_MS_CHANNEL_SET ui_ms_set);
extern ER       pwm_pwm_config(UINT32 ui_pwm_id, PPWM_CFG p_pwm_cfg);
extern ER       pwm_pwm_config_clock_div(PWM_CLOCK_DIV ui_clk_src, UINT32 ui_div);
extern ER       pwm_pwm_reload_config(UINT32 ui_pwm_id, INT32 i_rise, INT32 i_fall, INT32 i_base_period);

extern ER       pwm_mstep_enable(UINT32 ui_pwm_id);
extern ER       pwm_mstep_disable(UINT32 ui_pwm_id, BOOL b_wait);
extern ER       pwm_mstep_enable_set(PWM_MS_CHANNEL_SET ui_ms_set);
extern ER       pwm_mstep_disable_set(PWM_MS_CHANNEL_SET ui_ms_set);
extern ER       pwm_mstep_clock_div_reload(PWM_CLOCK_DIV ui_pwm_clk_div, BOOL  b_wait_done);
extern ER       pwm_mstep_config(UINT32 ui_pwm_id, PMSTP_CFG p_mstp_cfg);
extern ER       pwm_mstep_target_count_wait_done(PWM_TGT_COUNT ui_target_cnt);
extern ER       pwm_mstep_clk_div_wait_load_done(PWM_CLOCK_DIV ui_pwm_clk_div_src);
extern ER       pwm_mstep_config_clock_div(PWM_MS_CHANNEL_SET ui_ms_set, UINT32 ui_div);
extern ER       pwm_mstep_config_target_count_enable(PWM_TGT_COUNT ui_target_cnt_set, UINT32 ui_target_cnt, BOOL b_enable_target_cnt);
extern ER       pwm_mstep_config_set(PWM_MS_CHANNEL_SET ui_ms_set, PMS_CH_PHASE_CFG p_ms_set_ch_ph_cfg, PMSCOMMON_CFG p_ms_common_cfg);


extern ER       pwm_ccnt_enable(UINT32 ui_pwm_id);
extern ER       pwm_ccnt_reload(UINT32 ui_pwm_id);
extern UINT32   pwm_ccnt_get_current_val(UINT32 ui_pwm_id);
extern UINT32   pwm_ccnt_get_start_value(UINT32 ui_pwm_id);
extern UINT32   pwm_ccnt_get_edge_interval(UINT32 ui_pwm_id);
extern ER       pwm_ccnt_config(UINT32 ui_pwm_id, PPWM_CCNT_CFG p_ccnt_cfg);
extern ER       pwm_set_config(PWM_CONFIG_ID config_id, UINT32 ui_config);
extern ER       pwm_ccnt_config_timeout_enable(UINT32 ui_pwm_id, PPWM_CCNT_TOUT_CFG p_ccnt_tout_cfg);

//@}

#endif
