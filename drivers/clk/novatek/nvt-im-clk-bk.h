/**
    NVT clock header file
    This file will provide NVT clock related structure & API
    @file       nvt-im-clk.h
    @ingroup
    @note
    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/
#ifndef __NVT_IM_CLK_BK_H__
#define __NVT_IM_CLK_BK_H__

#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/clk-provider.h>

#define CLK_NAME_STR_SIZE 32

/* Reset or not during clk_prepare() */
#define DO_RESET	1
#define NOT_RESET	0

/* Enable or not during clock tree construction */
#define DO_ENABLE	1
#define NOT_ENABLE	0

/* PCLK Gating offset */
#define PGATING_OFS	0x10

/* SRAM Shutdown offset */
#define SRAM_OFS	0x80

/* PLL calculator factor*/
#define pll_cal_factor  0x20000

/* Bit index */
#define BIT0	0
#define BIT1	1
#define BIT2	2
#define BIT3	3
#define BIT4	4
#define BIT5	5
#define BIT6	6
#define BIT7	7
#define BIT8	8
#define BIT9	9
#define BIT10	10
#define BIT11	11
#define BIT12	12
#define BIT13	13
#define BIT14	14
#define BIT15	15
#define BIT16	16
#define BIT17	17
#define BIT18	18
#define BIT19	19
#define BIT20	20
#define BIT21	21
#define BIT22	22
#define BIT23	23
#define BIT24	24
#define BIT25	25
#define BIT26	26
#define BIT27	27
#define BIT28	28
#define BIT29	29
#define BIT30	30
#define BIT31	31

/* Bit width */
#define WID0	0
#define WID1	1
#define WID2	2
#define WID3	3
#define WID4	4
#define WID5	5
#define WID6	6
#define WID7	7
#define WID8	8
#define WID9	9
#define WID10	10
#define WID11	11
#define WID12	12
#define WID13	13
#define WID14	14
#define WID15	15
#define WID16	16
#define WID17	17
#define WID18	18
#define WID19	19
#define WID20	20
#define WID21	21
#define WID22	22
#define WID23	23
#define WID24	24
#define WID25	25
#define WID26	26
#define WID27	27
#define WID28	28
#define WID29	29
#define WID30	30
#define WID31	31

/* SWDIVX to correct software value */
#define SWDIVX  0x80
#define SWDIV2  0x82
#define SWDIV4  0x84
#define SWDIV8  0x88

struct nvt_fixed_rate_clk {
	char name[CLK_NAME_STR_SIZE];
	unsigned long fixed_rate;	/* HZ */
};

#define FIXED_RATE_CONF(_name, _fixed_rate)\
{\
	.name				=	_name,\
	.fixed_rate			=	_fixed_rate,\
}

struct nvt_pll_clk {
	struct clk_hw pll_hw;

	char name[CLK_NAME_STR_SIZE];
	unsigned long long pll_ratio;
	unsigned long rate_reg_offset;
	unsigned long gate_reg_offset;
	unsigned char gate_bit_idx;

	unsigned char rate_bit_idx;
	unsigned char rate_bit_width;
	unsigned long status_reg_offset;
	unsigned char status_bit_idx;

	unsigned long current_rate;
	unsigned long parent_rate;
	struct clk_hw *fixed_factor;

	spinlock_t *lock;
};

#define PLL_CONF(_name, _pll_ratio, _rate_reg_offset, _gate_reg_offset, \
			_gate_bit_idx)\
{\
	.name			=	_name,\
	.pll_ratio		=	_pll_ratio,\
	.gate_reg_offset	=	_gate_reg_offset,\
	.gate_bit_idx		=	_gate_bit_idx,\
	.rate_reg_offset	=	_rate_reg_offset,\
	.rate_bit_idx		=	0,\
	.rate_bit_width		=	8,\
	.status_reg_offset	=	_gate_reg_offset+0x04,\
	.status_bit_idx		=	_gate_bit_idx,\
}

struct nvt_composite_mux_clk {
	struct clk_mux mux;

	char name[CLK_NAME_STR_SIZE];
	const char **parent_names;
	unsigned long current_rate;	/* HZ */
	unsigned char parent_idx;
	unsigned long mux_reg_offset;
	unsigned char mux_bit_idx;
	unsigned char mux_bit_width;
	unsigned long div_reg_offset;
	unsigned char div_bit_idx;
	unsigned char div_bit_width;
	unsigned long gate_reg_offset;
	unsigned char gate_bit_idx;
	unsigned long apb_gate_reg_offset;
	unsigned char apb_gate_bit_idx;
	bool do_enable;
	unsigned long reset_reg_offset;
	unsigned char reset_bit_idx;
	unsigned long apb_reset_reg_offset;
	unsigned char apb_reset_bit_idx;
	bool do_reset;

	unsigned char num_parents;
	unsigned char mux_flags;

	unsigned long parent_rate;	/* HZ */

	spinlock_t *lock;
};

#define COMP_MUX_CONF(_name, _parent_names, _current_rate, _parent_idx, \
		_mux_reg_offset, _mux_bit_idx, _mux_bit_width, \
		_div_reg_offset, _div_bit_idx, _div_bit_width, \
		_gate_reg_offset, _gate_bit_idx, _apb_gate_reg_offset, _apb_gate_bit_idx, _do_enable, \
		_reset_reg_offset, _reset_bit_idx, _apb_reset_reg_offset, _apb_reset_bit_idx, _do_reset) \
{\
	.name			=	_name,\
	.parent_names		=	_parent_names,\
	.current_rate		=	_current_rate,\
	.parent_idx		=	_parent_idx,\
	.mux_reg_offset		=	_mux_reg_offset,\
	.mux_bit_idx		=	_mux_bit_idx,\
	.mux_bit_width		=	_mux_bit_width,\
	.div_reg_offset		=	_div_reg_offset,\
	.div_bit_idx		=	_div_bit_idx,\
	.div_bit_width		=	_div_bit_width,\
	.gate_reg_offset	=	_gate_reg_offset,\
	.gate_bit_idx		=	_gate_bit_idx,\
	.apb_gate_reg_offset	=	_apb_gate_reg_offset,\
	.apb_gate_bit_idx	=	_apb_gate_bit_idx,\
	.do_enable		=	_do_enable,\
	.reset_reg_offset	=	_reset_reg_offset,\
	.reset_bit_idx		=	_reset_bit_idx,\
	.apb_reset_reg_offset	=	_apb_reset_reg_offset,\
	.apb_reset_bit_idx	=	_apb_reset_bit_idx,\
	.do_reset		=	_do_reset,\
	.num_parents		=	ARRAY_SIZE(_parent_names),\
	.mux_flags		=	CLK_SET_RATE_PARENT\
					| CLK_SET_RATE_NO_REPARENT \
					| CLK_GET_RATE_NOCACHE,\
}

void nvt_cg_base_remap(void __iomem *);
int nvt_fixed_rate_clk_register(struct nvt_fixed_rate_clk[], int);
int nvt_pll_clk_register(struct nvt_pll_clk[], int, unsigned long,
			 spinlock_t *);
int nvt_composite_mux_clk_register(struct nvt_composite_mux_clk[], int,
				   spinlock_t *);

#endif
