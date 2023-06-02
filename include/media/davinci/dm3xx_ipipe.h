/*
 * Copyright (C) 2008 Texas Instruments Inc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Contains all ipipeif specific types used by applications and driver
 */

#ifndef _DM3XX_IPIPE_H
#define _DM3XX_IPIPE_H
/* Used to shift input image data based on the data lines connected
 * to parallel port
 */
/* IPIPE base specific types */
enum ipipeif_data_shift {
	IPIPEIF_BITS15_2,
	IPIPEIF_BITS14_1,
	IPIPEIF_BITS13_0,
	IPIPEIF_BITS12_0,
	IPIPEIF_BITS11_0,
	IPIPEIF_BITS10_0,
	IPIPEIF_BITS9_0
};

enum ipipeif_clkdiv {
	IPIPEIF_DIVIDE_HALF,
	IPIPEIF_DIVIDE_THIRD,
	IPIPEIF_DIVIDE_FOURTH,
	IPIPEIF_DIVIDE_FIFTH,
	IPIPEIF_DIVIDE_SIXTH,
	IPIPEIF_DIVIDE_EIGHTH,
	IPIPEIF_DIVIDE_SIXTEENTH,
	IPIPEIF_DIVIDE_THIRTY
};

/* IPIPE 5.1 interface types */
/* dpcm predicator for IPIPE 5.1 */
enum ipipeif_dpcm_pred {
	DPCM_SIMPLE_PRED,
	DPCM_ADV_PRED
};
/* data shift for IPIPE 5.1 */
enum ipipeif_5_1_data_shift {
	IPIPEIF_5_1_BITS11_0,
	IPIPEIF_5_1_BITS10_0,
	IPIPEIF_5_1_BITS9_0,
	IPIPEIF_5_1_BITS8_0,
	IPIPEIF_5_1_BITS7_0,
	IPIPEIF_5_1_BITS15_4,
};

/* clockdiv for IPIPE 5.1 */
struct ipipeif_5_1_clkdiv {
	unsigned char m;
	unsigned char n;
};

/* DPC at the if for IPIPE 5.1 */
struct ipipeif_dpc {
	/* 0 - disable, 1 - enable */
	unsigned char en;
	/* threshold */
	unsigned short thr;
};

enum ipipeif_decimation {
	IPIPEIF_DECIMATION_OFF,
	IPIPEIF_DECIMATION_ON
};

enum 	ipipeif_pixel_order {
	IPIPEIF_CBCR_Y,
	IPIPEIF_Y_CBCR
};

#ifdef __KERNEL__
#include <linux/kernel.h>
//#include <asm/arch/cpu.h>
#include <mach/hardware.h>
#include <asm/io.h>
#include <media/davinci/vpss.h>
#include <media/davinci/vpfe_types.h>

enum ipipeif_clock {
	PIXCEL_CLK,
	SDRAM_CLK
};

enum ipipeif_pack_mode  {
	IPIPEIF_PACK_16_BIT,
	IPIPEIF_PACK_8_BIT
};

enum ipipe_oper_mode {
	CONTINUOUS,
	ONE_SHOT
};

enum ipipeif_5_1_pack_mode  {
	IPIPEIF_5_1_PACK_16_BIT,
	IPIPEIF_5_1_PACK_8_BIT,
	IPIPEIF_5_1_PACK_8_BIT_A_LAW,
	IPIPEIF_5_1_PACK_12_BIT
};

enum  ipipeif_avg_filter {
	AVG_OFF,
	AVG_ON
};

enum  ipipeif_input_source {
	CCDC,
	SDRAM_RAW,
	CCDC_DARKFM,
	SDRAM_YUV
};

enum ipipeif_ialaw {
	ALAW_OFF,
	ALAW_ON
};

struct ipipeif_base {
	enum ipipeif_ialaw ialaw;
	enum ipipeif_pack_mode pack_mode;
	enum ipipeif_data_shift data_shift;
	enum ipipeif_clkdiv clk_div;
};

enum  ipipeif_input_src1 {
	SRC1_PARALLEL_PORT,
	SRC1_SDRAM_RAW,
	SRC1_ISIF_DARKFM,
	SRC1_SDRAM_YUV
};

enum ipipeif_dpcm_type {
	DPCM_8BIT_10BIT,
	DPCM_8BIT_12BIT
};

struct ipipeif_dpcm_decomp {
	unsigned char en;
	enum ipipeif_dpcm_type type;
	enum ipipeif_dpcm_pred pred;
};

enum ipipeif_dfs_dir {
	IPIPEIF_PORT_MINUS_SDRAM,
	IPIPEIF_SDRAM_MINUS_PORT
};

struct ipipeif_5_1 {
	enum ipipeif_5_1_pack_mode pack_mode;
	enum ipipeif_5_1_data_shift data_shift;
	enum ipipeif_input_src1 source1;
	struct ipipeif_5_1_clkdiv clk_div;
	/* Defect pixel correction */
	struct ipipeif_dpc dpc;
	/* DPCM decompression */
	struct ipipeif_dpcm_decomp dpcm;
	/* ISIF port pixel order */
	enum ipipeif_pixel_order pix_order;
	/* interface parameters from isif */
	struct vpfe_hw_if_param isif_port;
	/* clipped to this value */
	unsigned short clip;
	/* Align HSync and VSync to rsz_start */
	unsigned char align_sync;
	/* resizer start position */
	unsigned int rsz_start;
	/* DF gain enable */
	unsigned char df_gain_en;
	/* DF gain value */
	unsigned short df_gain;
	/* DF gain threshold value */
	unsigned short df_gain_thr;
};

/* ipipeif structures common to DM350 and DM365 used by ipipeif API */
struct ipipeif {
	enum ipipe_oper_mode mode;
	enum ipipeif_input_source source;
	enum ipipeif_clock clock_select;
	unsigned int glob_hor_size;
	unsigned int glob_ver_size;
	unsigned int hnum;
	unsigned int vnum;
	unsigned int adofs;
	unsigned char rsz;
	enum ipipeif_decimation decimation;
	enum ipipeif_avg_filter avg_filter;
	unsigned short gain;
	/* IPIPE 5.1 */
	union var_part {
		struct ipipeif_base if_base;
		struct ipipeif_5_1  if_5_1;
	} var;
};

/* IPIPEIF Register Offsets from the base address */
#define IPIPEIF_ENABLE 			(0x00)
#define IPIPEIF_GFG1			(0x04)
#define IPIPEIF_PPLN			(0x08)
#define IPIPEIF_LPFR			(0x0C)
#define IPIPEIF_HNUM 			(0x10)
#define IPIPEIF_VNUM    		(0x14)
#define IPIPEIF_ADDRU   		(0x18)
#define IPIPEIF_ADDRL   		(0x1C)
#define IPIPEIF_ADOFS   		(0x20)
#define IPIPEIF_RSZ   			(0x24)
#define IPIPEIF_GAIN  	 		(0x28)

/* Below registers are available only on IPIPE 5.1 */
#define IPIPEIF_DPCM   			(0x2C)
#define IPIPEIF_CFG2   			(0x30)
#define IPIPEIF_INIRSZ 			(0x34)
#define IPIPEIF_OCLIP  			(0x38)
#define IPIPEIF_DTUDF  			(0x3C)
#define IPIPEIF_CLKDIV			(0x40)
#define IPIPEIF_DPC1  			(0x44)
#define IPIPEIF_DPC2  			(0x48)
#define IPIPEIF_DFSGVL			(0x4C)
#define IPIPEIF_DFSGTH			(0x50)
#define IPIPEIF_RSZ3A 			(0x54)
#define IPIPEIF_INIRSZ3A		(0x58)
#define IPIPEIF_RSZ_MIN			(16)
#define IPIPEIF_RSZ_MAX			(112)
#define IPIPEIF_RSZ_CONST		(16)
#define SETBIT(reg, bit)   (reg = ((reg) | ((0x00000001)<<(bit))))
#define RESETBIT(reg, bit) (reg = ((reg) & (~(0x00000001<<(bit)))))

#define IPIPEIF_ADOFS_LSB_MASK 		(0x1FF)
#define IPIPEIF_ADOFS_LSB_SHIFT		(5)
#define IPIPEIF_ADOFS_MSB_MASK 		(0x200)
#define IPIPEIF_ADDRU_MASK 		(0x7FF)
#define IPIPEIF_ADDRL_SHIFT		(5)
#define IPIPEIF_ADDRL_MASK		(0xFFFF)
#define IPIPEIF_ADDRU_SHIFT		(21)
#define IPIPEIF_ADDRMSB_SHIFT		(31)
#define IPIPEIF_ADDRMSB_LEFT_SHIFT 	(10)

/* CFG1 Masks and shifts */
#define ONESHOT_SHIFT			(0)
#define DECIM_SHIFT			(1)
#define INPSRC_SHIFT			(2)
#define CLKDIV_SHIFT			(4)
#define AVGFILT_SHIFT			(7)
#define PACK8IN_SHIFT			(8)
#define IALAW_SHIFT			(9)
#define CLKSEL_SHIFT			(10)
#define DATASFT_SHIFT			(11)
#define INPSRC1_SHIFT			(14)

/* DPC2 */
#define IPIPEIF_DPC2_EN_SHIFT		(12)
#define IPIPEIF_DPC2_THR_MASK		(0xFFF)
#define IPIPEIF_DF_GAIN_EN_SHIFT	(10)
#define IPIPEIF_DF_GAIN_MASK		(0x3FF)
#define IPIPEIF_DF_GAIN_THR_MASK	(0xFFF)
/* DPCM */
#define IPIPEIF_DPCM_BITS_SHIFT		(2)
#define IPIPEIF_DPCM_PRED_SHIFT		(1)
/* CFG2 */
#define IPIPEIF_CFG2_HDPOL_SHIFT	(1)
#define IPIPEIF_CFG2_VDPOL_SHIFT	(2)
#define IPIPEIF_CFG2_YUV8_SHIFT		(6)
#define	IPIPEIF_CFG2_YUV16_SHIFT	(3)
#define	IPIPEIF_CFG2_YUV8P_SHIFT	(7)

/* INIRSZ */
#define IPIPEIF_INIRSZ_ALNSYNC_SHIFT	(13)
#define IPIPEIF_INIRSZ_MASK		(0x1FFF)

/* CLKDIV */
#define IPIPEIF_CLKDIV_M_SHIFT		8

int ipipeif_hw_setup(struct ipipeif *if_params);
int ipipeif_set_address(struct ipipeif *if_params, unsigned int address);
void ipipeif_set_enable(char en, unsigned int mode);
u32 ipipeif_get_enable(void);
void ipipeif_dump_register(void);

#endif
#endif

