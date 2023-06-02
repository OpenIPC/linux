/**
    Public header file for ISE module.

    @file       ise_eng.h
    @ingroup    mIIPPISE

    @brief

    Copyright   Novatek Microelectronics Corp. 2014.  All rights reserved.
*/


#ifndef _ISE_ENG_H_
#define _ISE_ENG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kdrv_type.h"

/*****************************************************************************/
typedef void (*ISE_ISR_CB)(void *eng, UINT32 status, void *reserve);

typedef struct {
	UINT32 chip_id;
	UINT32 eng_id;
	CHAR name[16];

	SEM_HANDLE sem;
	ID flg_id;
	UINT32 clock_rate;
	void *pclk;			/* linux struct clk*, not used in freertos */

	UINT32 reg_io_base;
	UINT32 sram_id;

	ISE_ISR_CB isr_cb;
} ISE_ENG_HANDLE;

typedef struct {
	UINT32 chip_num;
	UINT32 eng_num;
	UINT32 total_ch;
	ISE_ENG_HANDLE *p_eng;
} ISE_ENG_CTL;

INT32 ise_eng_init(ISE_ENG_CTL *p_eng_ctl);
INT32 ise_eng_release(ISE_ENG_CTL *p_eng_ctl);

ISE_ENG_HANDLE* ise_eng_get_handle(UINT32 chip_id, UINT32 eng_id);
void ise_eng_reg_isr_callback(ISE_ENG_HANDLE *p_eng, ISE_ISR_CB cb);
INT32 ise_eng_open(ISE_ENG_HANDLE *p_eng);
INT32 ise_eng_close(ISE_ENG_HANDLE *p_eng);
void ise_eng_trig_single(ISE_ENG_HANDLE *p_eng);
void ise_eng_trig_ll(ISE_ENG_HANDLE *p_eng, UINT32 ll_addr);
void ise_eng_write_reg(ISE_ENG_HANDLE *p_eng, UINT32 reg_ofs, UINT32 val);
void ise_eng_isr(ISE_ENG_HANDLE *p_eng);
void ise_eng_dump(void);
void ise_eng_dma_channel_enable_hw_reg(ISE_ENG_HANDLE *p_eng, BOOL set_en);
UINT8 ise_eng_get_dma_channel_status_hw_reg(ISE_ENG_HANDLE *p_eng);

INT32 ise_eng_chk_limitation(UINT32 reg_base_addr, UINT32 reg_flag_addr);

void ise_eng_wait_flag_frame_end(ISE_ENG_HANDLE *p_eng);
void ise_eng_wait_flag_linked_list_end(ISE_ENG_HANDLE *p_eng);
VOID ise_set_linked_list_fire_reg(ISE_ENG_HANDLE *p_eng, UINT32 set_fire);
VOID ise_set_linked_list_terminate_reg(ISE_ENG_HANDLE *p_eng, UINT32 set_terminate);
VOID ise_set_linked_list_addr_reg(ISE_ENG_HANDLE *p_eng, UINT32 set_addr);
ER ise_chg_linked_list_addr(UINT32 addr);
ER ise_set_linked_list_fire(VOID);
VOID ise_set_linked_list_terminate(VOID);
UINT32 ise_get_linked_list_buf_size(UINT32 job_nums);

/*****************************************************************************/
#define ISE_ENG_STRIPE_SIZE_MAX (0x100)

typedef enum {
	ISE_ENG_INTERRUPT_LLEND = 0x00000001,
	ISE_ENG_INTERRUPT_LLERR = 0x00000002,
	ISE_ENG_INTERRUPT_FMD = 0x80000000,
} ISE_ENG_INTERRUPT;

typedef enum {
	ISE_ENG_SCALE_BILINEAR = 	1,
	ISE_ENG_SCALE_NEAREST  = 	2,
	ISE_ENG_SCALE_INTEGRATION = 3,
	ENUM_DUMMY4WORD(ISE_ENG_SCALE_METHOD)
} ISE_ENG_SCALE_METHOD;

typedef enum {
	ISE_ENG_SCALE_DOWN  = 0,
	ISE_ENG_SCALE_UP	= 1,
	ENUM_DUMMY4WORD(ISE_ENG_SCALE_UD)
} ISE_ENG_SCALE_UD;

typedef enum {
	ISE_ENG_IOFMT_Y8          = 0,    ///< Y-8bit packing
	ISE_ENG_IOFMT_Y4          = 1,    ///< Y-4bit packing
	ISE_ENG_IOFMT_Y1          = 2,    ///< Y-1bit packing
	ISE_ENG_IOFMT_UVP         = 3,    ///< UV-Packed
	ISE_ENG_IOFMT_RGB565      = 4,    ///< RGB565 packing
	ISE_ENG_IOFMT_ARGB8888    = 5,    ///< ARGB8888
	ISE_ENG_IOFMT_ARGB1555    = 6,    ///< ARGB1555
	ISE_ENG_IOFMT_ARGB4444    = 7,    ///< ARGB4444
	ISE_ENG_IOFMT_YUVP        = 8,    ///< YUV-Packed
	ENUM_DUMMY4WORD(ISE_ENG_IO_FMT)
} ISE_ENG_IO_FMT;

typedef enum {
	ISE_ENG_OUTMODE_ARGB8888 = 0, ///< Output ARGB8888
	ISE_ENG_OUTMODE_RGB8888 = 1,  ///< Output RGB888
	ISE_ENG_OUTMODE_A8 = 2,       ///< Output A8
	ENUM_DUMMY4WORD(ISE_ENG_ARGB_OUTMODE)
} ISE_ENG_ARGB_OUTMODE;

typedef enum {
	ISE_ENG_BURST_32W = 0,  ///< burst 32 word
	ISE_ENG_BURST_16W = 1,  ///< burst 16 word
	ENUM_DUMMY4WORD(ISE_ENG_BURST_LENGTH)
} ISE_ENG_BURST_LENGTH;

typedef struct {
	UINT32 ofs;
	UINT32 val;
} ISE_ENG_REG;

typedef struct {
	UINT32 h_scl_ud;
	UINT32 h_scl_down_rate;
	UINT32 h_scl_factor;
	UINT32 v_scl_ud;
	UINT32 v_scl_down_rate;
	UINT32 v_scl_factor;

	UINT32 isd_coef_mode;
	UINT32 isd_scl_coef_num_h;
	UINT32 isd_scl_base_h;
	UINT32 isd_scl_factor_h[3];
	INT32 isd_user_coef_h[17];
	INT32 isd_coef_sum_all_h;
	INT32 isd_coef_sum_half_h;

	UINT32 isd_scl_coef_num_v;
	UINT32 isd_scl_base_v;
	UINT32 isd_scl_factor_v[3];
	INT32 isd_user_coef_v[17];
	INT32 isd_coef_sum_all_v;
	INT32 isd_coef_sum_half_v;

	UINT32 h_scl_ofs;
	UINT32 v_scl_ofs;
} ISE_ENG_SCALE_FACTOR_INFO;

typedef struct {
	BOOL h_filt_en;
	UINT32 h_filt_coef;
	BOOL v_filt_en;
	UINT32 v_filt_coef;
} ISE_ENG_SCALE_FILTER_INFO;

typedef struct {
	UINT32 overlap_mode;
	UINT32 overlap_size_sel;
	UINT32 stripe_size;
} ISE_ENG_STRIPE_INFO;

INT32 ise_eng_cal_scale_factor(UINT32 in_width, UINT32 in_height, UINT32 out_width, UINT32 out_height, UINT32 scl_method, ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor);
INT32 ise_eng_cal_scale_filter(UINT32 in_width, UINT32 in_height, UINT32 out_width, UINT32 out_height, ISE_ENG_SCALE_FILTER_INFO *p_scl_filter);
INT32 ise_eng_cal_stripe(UINT32 in_width, UINT32 out_width, UINT32 scl_method, ISE_ENG_STRIPE_INFO *p_stripe);

ISE_ENG_REG ise_eng_gen_in_ctrl_reg(UINT8 io_fmt, UINT8 scl_method, UINT8 argb_out_mode, UINT8 in_burst, UINT8 out_burst, UINT8 ovlp_mode);
ISE_ENG_REG ise_eng_gen_int_en_reg(UINT32 inte_en);
ISE_ENG_REG ise_eng_gen_in_size_reg(UINT32 width, UINT32 height);
ISE_ENG_REG ise_eng_gen_in_strp_reg(UINT32 st_size, UINT32 ovlp_size);
ISE_ENG_REG ise_eng_gen_in_lofs_reg(UINT32 lofs);
ISE_ENG_REG ise_eng_gen_in_addr_reg(UINT32 phy_addr);
ISE_ENG_REG ise_eng_gen_out_ctrl_reg0(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor, ISE_ENG_SCALE_FILTER_INFO *p_scl_filt);
ISE_ENG_REG ise_eng_gen_out_ctrl_reg1(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor);
ISE_ENG_REG ise_eng_gen_out_size_reg(UINT32 width, UINT32 height);
ISE_ENG_REG ise_eng_gen_out_lofs_reg(UINT32 lofs);
ISE_ENG_REG ise_eng_gen_out_addr_reg(UINT32 phy_addr);
ISE_ENG_REG ise_eng_gen_scl_ofs_reg0(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor);
ISE_ENG_REG ise_eng_gen_scl_ofs_reg1(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor);
ISE_ENG_REG ise_eng_gen_scl_isd_base_reg(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor);
ISE_ENG_REG ise_eng_gen_scl_isd_fact_reg0(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor);
ISE_ENG_REG ise_eng_gen_scl_isd_fact_reg1(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor);
ISE_ENG_REG ise_eng_gen_scl_isd_fact_reg2(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor);
ISE_ENG_REG ise_eng_gen_scl_isd_coef_ctrl_reg(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor);
ISE_ENG_REG ise_eng_gen_scl_isd_coef_h_reg0(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor);
ISE_ENG_REG ise_eng_gen_scl_isd_coef_h_reg1(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor);
ISE_ENG_REG ise_eng_gen_scl_isd_coef_h_reg2(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor);
ISE_ENG_REG ise_eng_gen_scl_isd_coef_h_reg3(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor);
ISE_ENG_REG ise_eng_gen_scl_isd_coef_h_reg4(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor);
ISE_ENG_REG ise_eng_gen_scl_isd_coef_h_reg5(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor);
ISE_ENG_REG ise_eng_gen_scl_isd_coef_h_reg6(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor);
ISE_ENG_REG ise_eng_gen_scl_isd_coef_v_reg0(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor);
ISE_ENG_REG ise_eng_gen_scl_isd_coef_v_reg1(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor);
ISE_ENG_REG ise_eng_gen_scl_isd_coef_v_reg2(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor);
ISE_ENG_REG ise_eng_gen_scl_isd_coef_v_reg3(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor);
ISE_ENG_REG ise_eng_gen_scl_isd_coef_v_reg4(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor);
ISE_ENG_REG ise_eng_gen_scl_isd_coef_v_reg5(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor);
ISE_ENG_REG ise_eng_gen_scl_isd_coef_v_reg6(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor);
ISE_ENG_REG ise_eng_gen_scl_isd_coef_h_all(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor);
ISE_ENG_REG ise_eng_gen_scl_isd_coef_h_half(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor);
ISE_ENG_REG ise_eng_gen_scl_isd_coef_v_all(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor);
ISE_ENG_REG ise_eng_gen_scl_isd_coef_v_half(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor);


/*****************************************************************************/
#ifdef __cplusplus
}
#endif

#endif //_ISE_ENG_H_
