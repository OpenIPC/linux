#ifndef _MTR_DRV_AN41908_H_
#define _MTR_DRV_AN41908_H_

#ifdef __KERNEL__
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/gpio.h>
#include <plat/nvt-gpio.h>
#else
#include "plat/gpio.h"
#endif
#include "mtr_common.h"


//=============================================================================
// compile switch
//=============================================================================
#define USE_GPIO_SIMULATION_SPI     1    // * if set to 0, need enable SPI driver in kernel config (also modify pinmux)
#define REVERSE_FOCUS_GOHOME_DIR    0    // *
#define REVERSE_ZOOM_GOHOME_DIR     0    // *


//=============================================================================
// constant define
//=============================================================================
#define MOVE_LOOP_FDELAY_TIME        31       // unit: msec
#define MOVE_LOOP_ZDELAY_TIME        62       // unit: msec

#define STANDBY_PWR_PULSE_WIDTH     0x3737    // set to 33%  (vary with PWMMOD=21) 0.33*21*8


//=============================================================================
// register definition
//=============================================================================
#define MOTOR_REG_DEF_BEGIN(name)    \
typedef union                        \
{                                    \
	UINT16 Reg;    \
	struct         \
	{              \

#define MOTOR_REG_DEF_BIT(field, bits)\
		unsigned field : bits;

#define MOTOR_REG_DEF_END(name)    \
	} Bit;                     \
} T_##name;


// REG_00
MOTOR_REG_DEF_BEGIN(IRIS_TARGET_00)
	MOTOR_REG_DEF_BIT(IRS_TGT, 10)
MOTOR_REG_DEF_END(IRIS_TARGET_00)

// REG_01
MOTOR_REG_DEF_BEGIN(IRIS_LPF_FC_CTL_01)
	MOTOR_REG_DEF_BIT(OVER_LPF_FC_1ST, 2)
	MOTOR_REG_DEF_BIT(OVER_LPF_FC_2ND, 2)
	MOTOR_REG_DEF_BIT(DEC_AVE,         1)
	MOTOR_REG_DEF_BIT(AS_FLT_OFF,      1)
	MOTOR_REG_DEF_BIT(ASOUND_LPF_FC,   3)
	MOTOR_REG_DEF_BIT(DGAIN,           7)
MOTOR_REG_DEF_END(IRIS_LPF_FC_CTL_01)

// REG_02
MOTOR_REG_DEF_BEGIN(IRIS_POLE_ZERO_CTL_02)
	MOTOR_REG_DEF_BIT(IRIS_CALC_NR, 4)
	MOTOR_REG_DEF_BIT(IRIS_ROUND,   4)
	MOTOR_REG_DEF_BIT(PID_ZERO,     4)
	MOTOR_REG_DEF_BIT(PID_POLE,     4)
MOTOR_REG_DEF_END(IRIS_POLE_ZERO_CTL_02)

// REG_03
MOTOR_REG_DEF_BEGIN(IRIS_PWM_CTL_03)
	MOTOR_REG_DEF_BIT(ARW,         4)
	MOTOR_REG_DEF_BIT(LMT_ENB,     1)
	MOTOR_REG_DEF_BIT(PWM_FLT_OFF, 1)
	MOTOR_REG_DEF_BIT(PWM_LPF_FC,  3)
	MOTOR_REG_DEF_BIT(PWM_IRIS,    3)
	MOTOR_REG_DEF_BIT(DT_ADJ_IRIS, 2)
MOTOR_REG_DEF_END(IRIS_PWM_CTL_03)

// REG_04
MOTOR_REG_DEF_BEGIN(IRIS_HALL_SENSOR_04)
	MOTOR_REG_DEF_BIT(HALL_BIAS_DAC,   8)
	MOTOR_REG_DEF_BIT(HALL_OFFSET_DAC, 8)
MOTOR_REG_DEF_END(IRIS_HALL_SENSOR_04)

// REG_05
MOTOR_REG_DEF_BEGIN(IRIS_HALL_SENSOR_GAIN_05)
	MOTOR_REG_DEF_BIT(TGT_LPF_FC,  4)
	MOTOR_REG_DEF_BIT(TGT_FLT_OFF, 1)
	MOTOR_REG_DEF_BIT(PID_INV,     1)
	MOTOR_REG_DEF_BIT(RESERVED,    2)
	MOTOR_REG_DEF_BIT(HALL_GAIN,   4)
	MOTOR_REG_DEF_BIT(AAF_FC,      1)
MOTOR_REG_DEF_END(IRIS_HALL_SENSOR_GAIN_05)

// REG_06
MOTOR_REG_DEF_BEGIN(PULSE1_START_TIME_06)
	MOTOR_REG_DEF_BIT(START1_TIME, 10)
MOTOR_REG_DEF_END(PULSE1_START_TIME_06)

// REG_07
MOTOR_REG_DEF_BEGIN(PULSE1_WIDTH_07)
	MOTOR_REG_DEF_BIT(WIDTH1,   12)
	MOTOR_REG_DEF_BIT(RESERVED, 3)
	MOTOR_REG_DEF_BIT(P1EN,     1)
MOTOR_REG_DEF_END(PULSE1_WIDTH_07)

// REG_08
MOTOR_REG_DEF_BEGIN(PULSE2_START_TIME_08)
	MOTOR_REG_DEF_BIT(START2_TIME, 10)
MOTOR_REG_DEF_END(PULSE2_START_TIME_08)

// REG_09
MOTOR_REG_DEF_BEGIN(PULSE2_WIDTH_09)
	MOTOR_REG_DEF_BIT(WIDTH2,   6)
	MOTOR_REG_DEF_BIT(RESERVED, 9)
	MOTOR_REG_DEF_BIT(P2EN,     1)
MOTOR_REG_DEF_END(PULSE2_WIDTH_09)

// REG_0A
MOTOR_REG_DEF_BEGIN(IRIS_TEST_0A)
	MOTOR_REG_DEF_BIT(TGT_IN_TEST, 10)
	MOTOR_REG_DEF_BIT(DUTY_TEST,   1)
MOTOR_REG_DEF_END(IRIS_TEST_0A)

// REG_0B
MOTOR_REG_DEF_BEGIN(VD_FZ_POLARITY_0B)
	MOTOR_REG_DEF_BIT(RESERVED0,    3)
	MOTOR_REG_DEF_BIT(ASWMODE,      2)
	MOTOR_REG_DEF_BIT(RESERVED1,    2)
	MOTOR_REG_DEF_BIT(TESTEN1,      1)
	MOTOR_REG_DEF_BIT(MODESEL_IRIS, 1)
	MOTOR_REG_DEF_BIT(MODESEL_FZ,   1)
	MOTOR_REG_DEF_BIT(PDWNB,        1)
	MOTOR_REG_DEF_BIT(ADC_TEST,     1)
	MOTOR_REG_DEF_BIT(PID_CLIP,     4)
MOTOR_REG_DEF_END(VD_FZ_POLARITY_0B)

// REG_0C
MOTOR_REG_DEF_BEGIN(IRIS_ADC_OUTPUT_0C)
	MOTOR_REG_DEF_BIT(IRSAD, 10)
MOTOR_REG_DEF_END(IRIS_ADC_OUTPUT_0C)

// REG_0E
MOTOR_REG_DEF_BEGIN(IRIS_TARGET_AVG_SPEED_0E)
	MOTOR_REG_DEF_BIT(TGT_UPDATE, 8)
	MOTOR_REG_DEF_BIT(AVE_SPEED,  5)
MOTOR_REG_DEF_END(IRIS_TARGET_AVG_SPEED_0E)

// REG_20
MOTOR_REG_DEF_BEGIN(PULSE_FREQUENCY_20)
	MOTOR_REG_DEF_BIT(DT1,     8)
	MOTOR_REG_DEF_BIT(PWMMODE, 5)
	MOTOR_REG_DEF_BIT(PWMRES,  2)
MOTOR_REG_DEF_END(PULSE_FREQUENCY_20)

// REG_21
MOTOR_REG_DEF_BEGIN(PLS_OUTPUT_21)
	MOTOR_REG_DEF_BIT(FZTEST,   5)
	MOTOR_REG_DEF_BIT(RESERVED, 2)
	MOTOR_REG_DEF_BIT(TESTEN2,  1)
MOTOR_REG_DEF_END(PLS_OUTPUT_21)

// REG_22
MOTOR_REG_DEF_BEGIN(PULSE1_PHASE_CORR_22)
	MOTOR_REG_DEF_BIT(DT2A,    8)
	MOTOR_REG_DEF_BIT(PHMODAB, 6)
MOTOR_REG_DEF_END(PULSE1_PHASE_CORR_22)

// REG_23
MOTOR_REG_DEF_BEGIN(PULSE1_MAX_DUTY_WIDTH_23)
	MOTOR_REG_DEF_BIT(PPWA, 8)
	MOTOR_REG_DEF_BIT(PPWB, 8)
MOTOR_REG_DEF_END(PULSE1_MAX_DUTY_WIDTH_23)

// REG_24
MOTOR_REG_DEF_BEGIN(PULSE1_MOVE_STEP_24)
	MOTOR_REG_DEF_BIT(PSUMAB,  8)
	MOTOR_REG_DEF_BIT(CCWCWAB, 1)
	MOTOR_REG_DEF_BIT(BRAKEAB, 1)
	MOTOR_REG_DEF_BIT(ENDISAB, 1)
	MOTOR_REG_DEF_BIT(LEDB,    1)
	MOTOR_REG_DEF_BIT(MICROAB, 2)
MOTOR_REG_DEF_END(PULSE1_MOVE_STEP_24)

// REG_25
MOTOR_REG_DEF_BEGIN(PULSE1_STEP_CYCLE_25)
	MOTOR_REG_DEF_BIT(INTCTAB, 16)
MOTOR_REG_DEF_END(PULSE1_STEP_CYCLE_25)

// REG_27
MOTOR_REG_DEF_BEGIN(PULSE2_PHASE_CORR_27)
	MOTOR_REG_DEF_BIT(DT2B,    8)
	MOTOR_REG_DEF_BIT(PHMODCD, 6)
MOTOR_REG_DEF_END(PULSE2_PHASE_CORR_27)

// REG_28
MOTOR_REG_DEF_BEGIN(PULSE2_MAX_DUTY_WIDTH_28)
	MOTOR_REG_DEF_BIT(PPWC, 8)
	MOTOR_REG_DEF_BIT(PPWD, 8)
MOTOR_REG_DEF_END(PULSE2_MAX_DUTY_WIDTH_28)

// REG_29
MOTOR_REG_DEF_BEGIN(PULSE2_MOVE_STEP_29)
	MOTOR_REG_DEF_BIT(PSUMCD,  8)
	MOTOR_REG_DEF_BIT(CCWCWCD, 1)
	MOTOR_REG_DEF_BIT(BRAKECD, 1)
	MOTOR_REG_DEF_BIT(ENDISCD, 1)
	MOTOR_REG_DEF_BIT(LEDA,    1)
	MOTOR_REG_DEF_BIT(MICROCD, 2)
MOTOR_REG_DEF_END(PULSE2_MOVE_STEP_29)

// REG_2A
MOTOR_REG_DEF_BEGIN(PULSE2_STEP_CYCLE_2A)
	MOTOR_REG_DEF_BIT(INTCTCD, 16)
MOTOR_REG_DEF_END(PULSE2_STEP_CYCLE_2A)

// REG_2C
MOTOR_REG_DEF_BEGIN(IRCUT_CONTROL_2C)
	MOTOR_REG_DEF_BIT(IR2, 1)
	MOTOR_REG_DEF_BIT(IR1, 1)
	MOTOR_REG_DEF_BIT(IRSWICH, 1)
MOTOR_REG_DEF_END(IRCUT_CONTROL_2C)

// REG
MOTOR_REG_DEF_BEGIN(AN41908_REG)
	MOTOR_REG_DEF_BIT(BIT0_15, 16)
MOTOR_REG_DEF_END(AN41908_REG)


//=============================================================================
// struct & definition
//=============================================================================
typedef struct {
	union {
		struct {
			UINT8 a0:1;
			UINT8 a1:1;
			UINT8 a2:1;
			UINT8 a3:1;
			UINT8 a4:1;
			UINT8 a5:1;
		} A;

		UINT8 addr;        // register address
	};

	union {
		struct {
			UINT8 r_w:1;    // 0: write / 1: read
			UINT8 c1 :1;    // un-used
		} M;

		UINT8 rw;
	};

	union {
		struct {
			UINT8 b0:1;
			UINT8 b1:1;
			UINT8 b2:1;
			UINT8 b3:1;
			UINT8 b4:1;
			UINT8 b5:1;
			UINT8 b6:1;
			UINT8 b7:1;
			UINT8 b8:1;
			UINT8 b9:1;
			UINT8 b10:1;
			UINT8 b11:1;
			UINT8 b12:1;
			UINT8 b13:1;
			UINT8 b14:1;
			UINT8 b15:1;
		} B;

		UINT16 data;       // written data
	};
} SPI_DATA_FMT;

typedef enum {
	AN41908_IRIS_TARGET            = 0x00,    //  0
	AN41908_IRIS_LPF_FC_CTL        = 0x01,    //  1
	AN41908_IRIS_POLE_ZERO_CTL     = 0x02,    //  2
	AN41908_IRIS_PWM_CTL           = 0x03,    //  3
	AN41908_IRIS_HALL_SENSOR       = 0x04,    //  4
	AN41908_IRIS_HALL_SENSOR_GAIN  = 0x05,    //  5
	AN41908_PULSE1_START_TIME      = 0x06,    //  6
	AN41908_PULSE1_WIDTH           = 0x07,    //  7
	AN41908_PULSE2_START_TIME      = 0x08,    //  8
	AN41908_PULSE2_WIDTH           = 0x09,    //  9
	AN41908_IRIS_TEST              = 0x0A,    // 10
	AN41908_VD_FZ_POLARITY         = 0x0B,    // 11
	AN41908_IRIS_ADC_OUTPUT        = 0x0C,    // 12
	AN41908_IRIS_TARGET_AVG_SPEED  = 0x0E,    // 13
	AN41908_PULSE_FREQUENCY        = 0x20,    // 14
	AN41908_PLS_OUTPUT             = 0x21,    // 15
	AN41908_PULSE1_PHASE_CORR      = 0x22,    // 16
	AN41908_PULSE1_MAX_DUTY_WIDTH  = 0x23,    // 17
	AN41908_PULSE1_MOVE_STEP       = 0x24,    // 18
	AN41908_PULSE1_STEP_CYCLE      = 0x25,    // 19
	AN41908_PULSE2_PHASE_CORR      = 0x27,    // 20
	AN41908_PULSE2_MAX_DUTY_WIDTH  = 0x28,    // 21
	AN41908_PULSE2_MOVE_STEP       = 0x29,    // 22
	AN41908_PULSE2_STEP_CYCLE      = 0x2A,    // 23
	AN41908_IRCUT_CTL              = 0x2C,    // 24
	AN41908_REG_UNKNOWN            = 0x3F,
	ENUM_DUMMY4WORD(AN41908_REG_CMD)
} AN41908_REG_CMD;

typedef enum {
	AN41908_SET_CFG = 0,
	AN41908_SET_SPD,
	AN41908_SET_STEP,
	AN41908_SET_BOTH_MOVE,
	ENUM_DUMMY4WORD(AN41908_CMD_TYPE)
} AN41908_CMD_TYPE;

typedef enum {
	VD_FZ_SIGNAL = 0,
	VD_IS_SIGNAL,
	VD_NONE_SIGNAL,
	ENUM_DUMMY4WORD(VD_SIGNAL)
} VD_SIGNAL;


//=============================================================================
// extern functions
//=============================================================================
extern MOTOR_DRV_INFO* mdrv_get_motor_info(UINT32 id);

extern void   an41908_open(UINT32 id);
extern void   an41908_close(UINT32 id);

extern INT32  an41908_init_focus(UINT32 id);
extern UINT32 an41908_get_focus_speed(UINT32 id);
extern void   an41908_set_focus_speed(UINT32 id, MOTOR_SPEED_CATEGORY spd_category);
extern void   an41908_assign_focus_position(UINT32 id, INT32 focus_pos, INT32 dir);
extern UINT32 an41908_get_focus_focal_length(UINT32 id, UINT32 zoom_sec_pos);
extern void   an41908_set_focus_focal_length(UINT32 id, UINT32 zoom_sec_pos, UINT32 focal_length);
extern INT32  an41908_get_focus_range(UINT32 id);
extern INT32  an41908_get_focus_position(UINT32 id);
extern INT32  an41908_set_focus_position(UINT32 id, INT32 focus_pos, BOOL limit_range);
extern INT32  an41908_press_focus_move(UINT32 id, BOOL move_dir);
extern void   an41908_release_focus_move(UINT32 id);

extern INT32  an41908_init_zoom(UINT32 id);
extern UINT32 an41908_get_zoom_speed(UINT32 id);
extern void   an41908_set_zoom_speed(UINT32 id, MOTOR_SPEED_CATEGORY spd_category);
extern void   an41908_assign_zoom_position(UINT32 id, INT32 zoom_pos, INT32 dir);
extern UINT32 an41908_get_zoom_section_pos(UINT32 id);
extern UINT32 an41908_get_zoom_max_section_pos(UINT32 id);
extern INT32  an41908_set_zoom_section_pos(UINT32 id, UINT32 zoom_sec_pos);
extern INT32  an41908_get_zoom_position(UINT32 id);
extern INT32  an41908_set_zoom_position(UINT32 id, INT32 zoom_pos);
extern INT32  an41908_press_zoom_move(UINT32 id, BOOL move_dir);
extern void   an41908_release_zoom_move(UINT32 id);

extern UINT32 an41908_get_ircut_state(UINT32 id);
extern void   an41908_set_ircut_state(UINT32 id, IRCUT_ACT state, UINT32 param);

extern void   an41908_init_aperture(UINT32 id);
extern UINT32 an41908_get_aperture_fno(UINT32 id, UINT32 zoom_sec_pos, UINT32 iris_pos);
extern APERTURE_SEC_POS an41908_get_aperture_section_pos(UINT32 id);
extern void   an41908_set_aperture_section_pos(UINT32 id, APERTURE_SEC_POS iris_sec_pos);
extern UINT32 an41908_get_aperture_position(UINT32 id);
extern void   an41908_set_aperture_position(UINT32 id, UINT32 iris_pos);

extern SHUTTER_ACT an41908_get_shutter_state(UINT32 id);
extern void   an41908_set_shutter_state(UINT32 id, SHUTTER_ACT state, UINT32 param);

extern UINT32 an41908_get_capability_info(UINT32 id);
extern UINT32 an41908_get_busy_status(UINT32 id);
extern UINT32 an41908_get_zf_range_table(UINT32 id, UINT32 tab_idx, LENS_ZOOM_FOCUS_TAB *zf_rtab);
extern UINT32 an41908_set_zf_range_table(UINT32 id, UINT32 tab_idx, LENS_ZOOM_FOCUS_TAB *zf_rtab);
extern UINT32 an41908_get_zf_curve_table(UINT32 id, UINT32 tab_idx, LENS_FOCUS_DISTANCE_TAB *zf_ctab);
extern UINT32 an41908_set_zf_curve_table(UINT32 id, UINT32 tab_idx, LENS_FOCUS_DISTANCE_TAB *zf_ctab);
extern UINT32 an41908_get_register(UINT32 id, UINT32 reg_addr);
extern void   an41908_set_register(UINT32 id, UINT32 reg_addr, UINT32 reg_value);
extern void   an41908_get_fz_backlash_value(UINT32 id, INT32 *focus_backlash, INT32 *zoom_backlash);
extern void   an41908_set_fz_backlash_value(UINT32 id, INT32 focus_backlash, INT32 zoom_backlash);


#endif

