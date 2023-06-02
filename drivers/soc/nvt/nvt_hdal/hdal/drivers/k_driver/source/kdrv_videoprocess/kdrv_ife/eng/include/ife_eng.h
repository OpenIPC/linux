/**
    Public header file for IFE module.

    @file       ife_eng.h
    @ingroup    mIIPPIFE

    @brief

    Copyright   Novatek Microelectronics Corp. 2014.  All rights reserved.
*/


#ifndef _IFE_ENG_H_
#define _IFE_ENG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kdrv_type.h"

#include "ife_eng_base.h"


#define IFE_ENG_REG_NUM		470

#define IFE_LOADTYPE_IFE_START_LOAD		0
#define IFE_LOADTYPE_FRAME_END_LOAD		1
#define IFE_LOADTYPE_FRAME_START_LOAD	2
#define IFE_LOADTYPE_GLOBAL_LOAD	3


/*****************************************************************************/
typedef void (*IFE_ISR_CB)(void *eng, UINT32 status, void *reserve);

typedef enum  {
	IFE_ENG_INTERRUPT_FMD             = 0x00000001,  ///< enable interrupt: frame end
	IFE_ENG_INTERRUPT_DEC1_ERR        = 0x00000002,  ///< enable interrupt: rde decode error interrupt
	IFE_ENG_INTERRUPT_DEC2_ERR        = 0x00000004,  ///< enable interrupt: rde decode error interrupt
	IFE_ENG_INTERRUPT_LLEND           = 0x00000008,  ///< enable interrupt: LinkedList end interrupt
	IFE_ENG_INTERRUPT_LLERR           = 0x00000010,  ///< enable interrupt: LinkedList error  interrupt
	IFE_ENG_INTERRUPT_LLERR2          = 0x00000020,  ///< enable interrupt: LinkedList error2 interrupt
	IFE_ENG_INTERRUPT_LLJOBEND        = 0x00000040,  ///< enable interrupt: LinkedList job end interrupt
	IFE_ENG_INTERRUPT_BUFOVFL         = 0x00000080,  ///< enable interrupt: buffer overflow interrupt
	IFE_ENG_INTERRUPT_RING_BUF_ERR    = 0x00000100,  ///< enable interrupt: ring buffer error interrupt
	IFE_ENG_INTERRUPT_FRAME_ERR       = 0x00000200,  ///< enable interrupt: frame error interrupt (for direct mode)
	IFE_ENG_INTERRUPT_SIE_FRAME_START = 0x00001000,  ///< enable interrupt: SIE frame start interrupt (for direct mode)
	IFE_ENG_INTERRUPT_ALL             = (IFE_ENG_INTERRUPT_FMD | IFE_ENG_INTERRUPT_DEC1_ERR | IFE_ENG_INTERRUPT_DEC2_ERR | IFE_ENG_INTERRUPT_LLEND | IFE_ENG_INTERRUPT_LLERR |
										IFE_ENG_INTERRUPT_LLERR2 | IFE_ENG_INTERRUPT_LLJOBEND | IFE_ENG_INTERRUPT_BUFOVFL | IFE_ENG_INTERRUPT_RING_BUF_ERR | IFE_ENG_INTERRUPT_FRAME_ERR |
										IFE_ENG_INTERRUPT_SIE_FRAME_START)
} IFE_ENG_INTERRUPT;

typedef struct {
	IFE_OPMODE mode;
	IFE_BITDEPTH in_bits;
	IFE_BITDEPTH out_bits;
	IFE_BAYERFMTSEL bayer_fmt;
	UINT32 cfa_pat;
	UINT32 binning;
	UINT32 fusion_num;
} IFE_ENG_CONTROL_INFO;

typedef struct {
	UINT32 chip_id;
	UINT32 eng_id;
	CHAR name[16];

	UINT32 clock_rate;
	void *pclk;			/* linux struct clk*, not used in freertos */

	UINT32 reg_io_base;
	UINT32 irq_id;

	IFE_ISR_CB isr_cb;
} IFE_ENG_HANDLE;

typedef struct {
	UINT32 chip_num;
	UINT32 eng_num;
	UINT32 total_ch;
	IFE_ENG_HANDLE *p_eng;
} IFE_ENG_CTL;

typedef struct {
	UINT32 ofs;
	UINT32 val;
} IFE_ENG_REG;
/*****************************************************************************/
#if 0


IFE_ENG_REG ife_eng_gen_ctl_reg(IFE_ENG_CONTROL_INFO *info);
IFE_ENG_REG ife_eng_gen_inte_en_reg(UINT32 inte);
IFE_ENG_REG ife_eng_gen_stripe_reg(UINT32 hn, UINT32 hl, UINT32 hm);
IFE_ENG_REG ife_eng_gen_size_reg0(UINT32 width, UINT32 height);
IFE_ENG_REG ife_eng_gen_size_reg1(UINT32 crop_width, UINT32 crop_height);
IFE_ENG_REG ife_eng_gen_size_reg2(UINT32 crop_width, UINT32 crop_height);
IFE_ENG_REG ife_eng_gen_dma_in_reg0(UINT32 addr);
IFE_ENG_REG ife_eng_gen_lofs_reg0(UINT32 lofs, UINT32 h_start_shift);
IFE_ENG_REG ife_eng_gen_dma_in_reg1(UINT32 addr);
IFE_ENG_REG ife_eng_gen_in_lofs_reg1(UINT32 lofs);
IFE_ENG_REG ife_eng_gen_dma_out_reg0(UINT32 addr);
IFE_ENG_REG ife_eng_gen_out_lofs_reg(UINT32 lofs);
IFE_ENG_REG ife_eng_gen_dram_setting_reg(UINT32 input_burst_mode, UINT32 output_burst_mode, UINT32 loop_line, UINT32 loop_en);
#endif

extern INT32 ife_eng_init(UINT32 chip_num, UINT32 eng_num);
extern INT32 ife_eng_release(void);
extern INT32 ife_eng_init_resource(IFE_ENG_HANDLE *p_eng);
extern IFE_ENG_HANDLE* ife_eng_get_handle(UINT32 chip_id, UINT32 eng_id);
extern void ife_eng_reg_isr_callback(IFE_ENG_HANDLE *p_eng, IFE_ISR_CB cb);
extern INT32 ife_eng_open(IFE_ENG_HANDLE *p_eng);
extern INT32 ife_eng_close(IFE_ENG_HANDLE *p_eng);
extern void ife_eng_trig_single_hw_reg(IFE_ENG_HANDLE *p_eng);
extern void ife_eng_trig_ll_hw_reg(IFE_ENG_HANDLE *p_eng, UINT32 ll_addr);
extern void ife_eng_write_hw_reg(IFE_ENG_HANDLE *p_eng, UINT32 reg_ofs, UINT32 val);
extern void ife_eng_hard_reset_hw_reg(IFE_ENG_HANDLE *p_eng);
extern void ife_eng_isr_hw_reg(IFE_ENG_HANDLE *p_eng);

extern void ife_eng_set_load_hw_reg(IFE_ENG_HANDLE *p_eng, UINT32 load_type);
extern void ife_eng_stop_single_hw_reg(IFE_ENG_HANDLE *p_eng);

extern UINT32 ife_eng_get_reg_base_buf_size(VOID);  // unit: byte
extern UINT32 ife_eng_get_reg_flag_buf_size(VOID);   // unit: byte
extern VOID ife_eng_set_reg_buf(UINT32 reg_base_addr, UINT32 reg_flag_addr);


extern VOID ife_eng_wait_flag_frame_end(BOOL is_clear_flag);
extern VOID ife_eng_wait_flag_linked_list_end(BOOL is_clear_flag);
extern VOID ife_eng_wait_flag_linked_list_job_end(BOOL is_clear_flag);
extern VOID ife_eng_clear_flag_frame_end(VOID);
extern VOID ife_eng_clear_flag_linked_list_end(VOID);
extern VOID ife_eng_clear_flag_linked_list_job_end(VOID);

extern INT32 ife_eng_chk_limitation(UINT32 reg_base_addr, UINT32 reg_flag_addr);

extern VOID ife_eng_dma_channel_enable_hw_reg(IFE_ENG_HANDLE *p_eng, BOOL set_en);
extern UINT8 ife_eng_get_dma_channel_status_hw_reg(IFE_ENG_HANDLE *p_eng);

#if (defined(_NVT_EMULATION_) == ON)

extern UINT32 ife_get_dram_in_addr(void);

extern UINT32 ife_get_dram_in_addr2(void);

extern UINT32 ife_get_dram_in_lofs(void);

extern UINT32 ife_get_dram_in_lofs2(void);

extern UINT32 ife_get_dram_out_addr(void);

extern UINT32 ife_get_dram_out_lofs(void);

extern UINT32 ife_get_in_vsize(void);

extern UINT32 ife_get_in_hsize(void);

extern BOOL ife_end_time_out_status;

#endif




#endif
