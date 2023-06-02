#ifndef _MTR_DRV_MS41949_H_
#define _MTR_DRV_MS41949_H_

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
#define REVERSE_IRIS_GOHOME_DIR     1    // *


//=============================================================================
// constant define
//=============================================================================
#define MOVE_LOOP_FDELAY_TIME        31       // unit: msec
#define MOVE_LOOP_ZDELAY_TIME        62       // unit: msec
#define MOVE_LOOP_IDELAY_TIME        62       // unit: msec

#define STANDBY_FZ_PWR_PULSE_WIDTH      0x3737    // set to 33%  (vary with PWMMOD=21) 0.33*21*8
#define STANDBY_IRIS_PWR_PULSE_WIDTH    0x1111    // set to 11%  (vary with PWMMOD=21) 0.10*21*8


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
MOTOR_REG_DEF_BEGIN(PULSE_FREQUENCY_CD_00)
	MOTOR_REG_DEF_BIT(DT1CD,     8)
	MOTOR_REG_DEF_BIT(PWMMODECD, 5)
	MOTOR_REG_DEF_BIT(PWMRESCD,  2)
MOTOR_REG_DEF_END(PULSE_FREQUENCY_CD_00)

// REG_01
MOTOR_REG_DEF_BEGIN(PLS_OUTPUT_CD_01)
	MOTOR_REG_DEF_BIT(FZTESTCD,  5)
	MOTOR_REG_DEF_BIT(RESERVED,  2)
	MOTOR_REG_DEF_BIT(TESTEN2CD, 1)
MOTOR_REG_DEF_END(PLS_OUTPUT_CD_01)

// REG_02
MOTOR_REG_DEF_BEGIN(PULSE3_PHASE_CORR_02)
	MOTOR_REG_DEF_BIT(DT2C,   8)
	MOTOR_REG_DEF_BIT(PHMODC, 6)
MOTOR_REG_DEF_END(PULSE3_PHASE_CORR_02)

// REG_03
MOTOR_REG_DEF_BEGIN(PULSE3_MAX_DUTY_WIDTH_03)
	MOTOR_REG_DEF_BIT(PPWC1, 8)
	MOTOR_REG_DEF_BIT(PPWC2, 8)
MOTOR_REG_DEF_END(PULSE3_MAX_DUTY_WIDTH_03)

// REG_04
MOTOR_REG_DEF_BEGIN(PULSE3_MOVE_STEP_04)
	MOTOR_REG_DEF_BIT(PSUMC,    8)
	MOTOR_REG_DEF_BIT(CCWCWC,   1)
	MOTOR_REG_DEF_BIT(BRAKEC,   1)
	MOTOR_REG_DEF_BIT(ENDISC,   1)
	MOTOR_REG_DEF_BIT(RESERVED, 1)
	MOTOR_REG_DEF_BIT(MICROC,   2)
MOTOR_REG_DEF_END(PULSE3_MOVE_STEP_04)

// REG_05
MOTOR_REG_DEF_BEGIN(PULSE3_STEP_CYCLE_05)
	MOTOR_REG_DEF_BIT(INTCTC, 16)
MOTOR_REG_DEF_END(PULSE3_STEP_CYCLE_05)

// REG_07
MOTOR_REG_DEF_BEGIN(PULSE4_PHASE_CORR_07)
	MOTOR_REG_DEF_BIT(DT2D,   8)
	MOTOR_REG_DEF_BIT(PHMODD, 6)
MOTOR_REG_DEF_END(PULSE4_PHASE_CORR_07)

// REG_08
MOTOR_REG_DEF_BEGIN(PULSE4_MAX_DUTY_WIDTH_08)
	MOTOR_REG_DEF_BIT(PPWD1, 8)
	MOTOR_REG_DEF_BIT(PPWD2, 8)
MOTOR_REG_DEF_END(PULSE4_MAX_DUTY_WIDTH_08)

// REG_09
MOTOR_REG_DEF_BEGIN(PULSE4_MOVE_STEP_09)
	MOTOR_REG_DEF_BIT(PSUMD,    8)
	MOTOR_REG_DEF_BIT(CCWCWD,   1)
	MOTOR_REG_DEF_BIT(BRAKED,   1)
	MOTOR_REG_DEF_BIT(ENDISD,   1)
	MOTOR_REG_DEF_BIT(RESERVED, 1)
	MOTOR_REG_DEF_BIT(MICROD,   2)
MOTOR_REG_DEF_END(PULSE4_MOVE_STEP_09)

// REG_0A
MOTOR_REG_DEF_BEGIN(PULSE4_STEP_CYCLE_0A)
	MOTOR_REG_DEF_BIT(INTCTD, 16)
MOTOR_REG_DEF_END(PULSE4_STEP_CYCLE_0A)

// REG_0B
MOTOR_REG_DEF_BEGIN(VD_FZ_POLARITY_0B)
	MOTOR_REG_DEF_BIT(RESERVED0,  9)
	MOTOR_REG_DEF_BIT(MODESEL_FZ, 1)
	MOTOR_REG_DEF_BIT(RESERVED1,  6)
MOTOR_REG_DEF_END(VD_FZ_POLARITY_0B)

// REG_20
MOTOR_REG_DEF_BEGIN(PULSE_FREQUENCY_AB_20)
	MOTOR_REG_DEF_BIT(DT1AB,     8)
	MOTOR_REG_DEF_BIT(PWMMODEAB, 5)
	MOTOR_REG_DEF_BIT(PWMRESAB,  2)
MOTOR_REG_DEF_END(PULSE_FREQUENCY_AB_20)

// REG_21
MOTOR_REG_DEF_BEGIN(PLS_OUTPUT_AB_21)
	MOTOR_REG_DEF_BIT(FZTESTAB,  5)
	MOTOR_REG_DEF_BIT(RESERVED,  2)
	MOTOR_REG_DEF_BIT(TESTEN2AB, 1)
MOTOR_REG_DEF_END(PLS_OUTPUT_AB_21)

// REG_22
MOTOR_REG_DEF_BEGIN(PULSE1_PHASE_CORR_22)
	MOTOR_REG_DEF_BIT(DT2A,   8)
	MOTOR_REG_DEF_BIT(PHMODA, 6)
MOTOR_REG_DEF_END(PULSE1_PHASE_CORR_22)

// REG_23
MOTOR_REG_DEF_BEGIN(PULSE1_MAX_DUTY_WIDTH_23)
	MOTOR_REG_DEF_BIT(PPWA1, 8)
	MOTOR_REG_DEF_BIT(PPWA2, 8)
MOTOR_REG_DEF_END(PULSE1_MAX_DUTY_WIDTH_23)

// REG_24
MOTOR_REG_DEF_BEGIN(PULSE1_MOVE_STEP_24)
	MOTOR_REG_DEF_BIT(PSUMA,    8)
	MOTOR_REG_DEF_BIT(CCWCWA,   1)
	MOTOR_REG_DEF_BIT(BRAKEA,   1)
	MOTOR_REG_DEF_BIT(ENDISA,   1)
	MOTOR_REG_DEF_BIT(RESERVED, 1)
	MOTOR_REG_DEF_BIT(MICROA,   2)
MOTOR_REG_DEF_END(PULSE1_MOVE_STEP_24)

// REG_25
MOTOR_REG_DEF_BEGIN(PULSE1_STEP_CYCLE_25)
	MOTOR_REG_DEF_BIT(INTCTA, 16)
MOTOR_REG_DEF_END(PULSE1_STEP_CYCLE_25)

// REG_27
MOTOR_REG_DEF_BEGIN(PULSE2_PHASE_CORR_27)
	MOTOR_REG_DEF_BIT(DT2B,   8)
	MOTOR_REG_DEF_BIT(PHMODB, 6)
MOTOR_REG_DEF_END(PULSE2_PHASE_CORR_27)

// REG_28
MOTOR_REG_DEF_BEGIN(PULSE2_MAX_DUTY_WIDTH_28)
	MOTOR_REG_DEF_BIT(PPWB1, 8)
	MOTOR_REG_DEF_BIT(PPWB2, 8)
MOTOR_REG_DEF_END(PULSE2_MAX_DUTY_WIDTH_28)

// REG_29
MOTOR_REG_DEF_BEGIN(PULSE2_MOVE_STEP_29)
	MOTOR_REG_DEF_BIT(PSUMB,    8)
	MOTOR_REG_DEF_BIT(CCWCWB,   1)
	MOTOR_REG_DEF_BIT(BRAKEB,   1)
	MOTOR_REG_DEF_BIT(ENDISB,   1)
	MOTOR_REG_DEF_BIT(RESERVED, 1)
	MOTOR_REG_DEF_BIT(MICROB,   2)
MOTOR_REG_DEF_END(PULSE2_MOVE_STEP_29)

// REG_2A
MOTOR_REG_DEF_BEGIN(PULSE2_STEP_CYCLE_2A)
	MOTOR_REG_DEF_BIT(INTCTB, 16)
MOTOR_REG_DEF_END(PULSE2_STEP_CYCLE_2A)

// REG_2C
MOTOR_REG_DEF_BEGIN(IRCUT_CONTROL_2C)
	MOTOR_REG_DEF_BIT(IR2,     1)
	MOTOR_REG_DEF_BIT(IR1,     1)
	MOTOR_REG_DEF_BIT(IRSWICH, 1)
MOTOR_REG_DEF_END(IRCUT_CONTROL_2C)

// REG
MOTOR_REG_DEF_BEGIN(MS41949_REG)
	MOTOR_REG_DEF_BIT(BIT0_15, 16)
MOTOR_REG_DEF_END(MS41949_REG)


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
	MS41949_PULSE_FREQUENCY_CD     = 0x00,    // 1
	MS41949_PLS_CD_OUTPUT          = 0x01,    // 2
	MS41949_PULSE3_PHASE_CORR      = 0x02,    // 3
	MS41949_PULSE3_MAX_DUTY_WIDTH  = 0x03,    // 4
	MS41949_PULSE3_MOVE_STEP       = 0x04,    // 5
	MS41949_PULSE3_STEP_CYCLE      = 0x05,    // 6
	MS41949_PULSE4_PHASE_CORR      = 0x07,    // 7
	MS41949_PULSE4_MAX_DUTY_WIDTH  = 0x08,    // 8
	MS41949_PULSE4_MOVE_STEP       = 0x09,    // 9
	MS41949_PULSE4_STEP_CYCLE      = 0x0A,    // 10
	MS41949_VD_FZ_POLARITY         = 0x0B,    // 11
	MS41949_PULSE_FREQUENCY_AB     = 0x20,    // 12
	MS41949_PLS_AB_OUTPUT          = 0x21,    // 13
	MS41949_PULSE1_PHASE_CORR      = 0x22,    // 14
	MS41949_PULSE1_MAX_DUTY_WIDTH  = 0x23,    // 15
	MS41949_PULSE1_MOVE_STEP       = 0x24,    // 16
	MS41949_PULSE1_STEP_CYCLE      = 0x25,    // 17
	MS41949_PULSE2_PHASE_CORR      = 0x27,    // 18
	MS41949_PULSE2_MAX_DUTY_WIDTH  = 0x28,    // 19
	MS41949_PULSE2_MOVE_STEP       = 0x29,    // 20
	MS41949_PULSE2_STEP_CYCLE      = 0x2A,    // 21
	MS41949_IRCUT_CTL              = 0x2C,    // 22
	MS41949_REG_UNKNOWN            = 0x3F,
	ENUM_DUMMY4WORD(MS41949_REG_CMD)
} MS41949_REG_CMD;

typedef enum {
	MS41949_SET_CFG = 0,
	MS41949_SET_SPD,
	MS41949_SET_STEP,
	MS41949_SET_BOTH_MOVE,
	ENUM_DUMMY4WORD(MS41949_CMD_TYPE)
} MS41949_CMD_TYPE;

typedef enum {
	VD_FZ_SIGNAL = 0,
	VD_NONE_SIGNAL,
	ENUM_DUMMY4WORD(VD_SIGNAL)
} VD_SIGNAL;


//=============================================================================
// extern functions
//=============================================================================
extern MOTOR_DRV_INFO* mdrv_get_motor_info(UINT32 id);

extern void   ms41949_open(UINT32 id);
extern void   ms41949_close(UINT32 id);

extern INT32  ms41949_init_focus(UINT32 id);
extern UINT32 ms41949_get_focus_speed(UINT32 id);
extern void   ms41949_set_focus_speed(UINT32 id, MOTOR_SPEED_CATEGORY spd_category);
extern void   ms41949_assign_focus_position(UINT32 id, INT32 focus_pos, INT32 dir);
extern UINT32 ms41949_get_focus_focal_length(UINT32 id, UINT32 zoom_sec_pos);
extern void   ms41949_set_focus_focal_length(UINT32 id, UINT32 zoom_sec_pos, UINT32 focal_length);
extern INT32  ms41949_get_focus_range(UINT32 id);
extern INT32  ms41949_get_focus_position(UINT32 id);
extern INT32  ms41949_set_focus_position(UINT32 id, INT32 focus_pos, BOOL limit_range);
extern INT32  ms41949_press_focus_move(UINT32 id, BOOL move_dir);
extern void   ms41949_release_focus_move(UINT32 id);

extern INT32  ms41949_init_zoom(UINT32 id);
extern UINT32 ms41949_get_zoom_speed(UINT32 id);
extern void   ms41949_set_zoom_speed(UINT32 id, MOTOR_SPEED_CATEGORY spd_category);
extern void   ms41949_assign_zoom_position(UINT32 id, INT32 zoom_pos, INT32 dir);
extern UINT32 ms41949_get_zoom_section_pos(UINT32 id);
extern UINT32 ms41949_get_zoom_max_section_pos(UINT32 id);
extern INT32  ms41949_set_zoom_section_pos(UINT32 id, UINT32 zoom_sec_pos);
extern INT32  ms41949_get_zoom_position(UINT32 id);
extern INT32  ms41949_set_zoom_position(UINT32 id, INT32 zoom_pos);
extern INT32  ms41949_press_zoom_move(UINT32 id, BOOL move_dir);
extern void   ms41949_release_zoom_move(UINT32 id);

extern UINT32 ms41949_get_ircut_state(UINT32 id);
extern void   ms41949_set_ircut_state(UINT32 id, IRCUT_ACT state, UINT32 param);

extern void   ms41949_init_aperture(UINT32 id);
extern UINT32 ms41949_get_aperture_fno(UINT32 id, UINT32 zoom_sec_pos, UINT32 iris_pos);
extern void   ms41949_assign_aperture_position(UINT32 id, INT32 iris_pos, INT32 dir);
extern APERTURE_SEC_POS ms41949_get_aperture_section_pos(UINT32 id);
extern void   ms41949_set_aperture_section_pos(UINT32 id, APERTURE_SEC_POS iris_sec_pos);
extern UINT32 ms41949_get_aperture_position(UINT32 id);
extern void   ms41949_set_aperture_position(UINT32 id, INT32 iris_pos);
extern UINT32 ms41949_get_aperture_flux_ratio(UINT32 id);
extern void   ms41949_set_aperture_flux_ratio(UINT32 id, INT32 iris_ratio);

extern SHUTTER_ACT ms41949_get_shutter_state(UINT32 id);
extern void   ms41949_set_shutter_state(UINT32 id, SHUTTER_ACT state, UINT32 param);

extern UINT32 ms41949_get_capability_info(UINT32 id);
extern UINT32 ms41949_get_busy_status(UINT32 id);
extern UINT32 ms41949_get_zf_range_table(UINT32 id, UINT32 tab_idx, LENS_ZOOM_FOCUS_TAB *zf_rtab);
extern UINT32 ms41949_set_zf_range_table(UINT32 id, UINT32 tab_idx, LENS_ZOOM_FOCUS_TAB *zf_rtab);
extern UINT32 ms41949_get_zf_curve_table(UINT32 id, UINT32 tab_idx, LENS_FOCUS_DISTANCE_TAB *zf_ctab);
extern UINT32 ms41949_set_zf_curve_table(UINT32 id, UINT32 tab_idx, LENS_FOCUS_DISTANCE_TAB *zf_ctab);
extern UINT32 ms41949_get_register(UINT32 id, UINT32 reg_addr);
extern void   ms41949_set_register(UINT32 id, UINT32 reg_addr, UINT32 reg_value);
extern void   ms41949_get_fz_backlash_value(UINT32 id, INT32 *focus_backlash, INT32 *zoom_backlash);
extern void   ms41949_set_fz_backlash_value(UINT32 id, INT32 focus_backlash, INT32 zoom_backlash);


#endif

