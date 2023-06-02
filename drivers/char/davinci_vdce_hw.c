/* *
 * Copyright (C) 2007 Texas Instruments	Inc
 *
 * This	program	is free	software; you can redistribute it and/or modify
 * it under the	terms of the GNU General Public	License	as published by
 * the Free Software Foundation; either	version	2 of the License, or
 * (at your option) any	later version.
 *
 * This	program	is distributed in the hope that	it will	be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A	PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59	Temple Place, Suite 330, Boston, MA 02111-1307	USA
 */
/* davinci_vdce_hw.c	file */

/*Header files*/
#include "davinci_vdce_hw.h"
#include <linux/davinci_vdce.h>
#include <linux/kernel.h>	/* printk()     */
#include <linux/device.h>

extern struct device *vdce_device;

/*
 * vdce_hardware_setup :Function to set GE hardware configuration registers
 */
void vdce_hw_setup(vdce_hw_config_t * register_config)
{
	/* all devdbg will be removed after UT */
	int status;
	dev_dbg(vdce_device, " vdce_hw_setup E\n");

	/* setting the hardware register GE ctrl */
	regw(register_config->vdce_ctrl, VDCE_CTRL);
	dev_dbg(vdce_device, "GE CTRL : %x	regr = %x \n",
		register_config->vdce_ctrl, regr(VDCE_CTRL));

	regw(register_config->vdce_emulation_ctrl, VDCE_EMULATION_CTRL);
	dev_dbg(vdce_device, "VDCE_EMULATION_CTRL	%x regr	= %x\n",
		register_config->vdce_emulation_ctrl,
		regr(VDCE_EMULATION_CTRL));

	/* setting the hardware register sdr_fmt */
	regw(register_config->vdce_sdr_fmt, VDCE_SDR_FMT);
	dev_dbg(vdce_device, "VDCE_SDR_FMT %x regr = %x\n",
		register_config->vdce_sdr_fmt, regr(VDCE_SDR_FMT));

	/* setting the hardware register req_sz */
	regw(register_config->vdce_req_sz, VDCE_REQ_SZ);
	dev_dbg(vdce_device, "VDCE_REQ_SZ %x	regr = %x\n",
		register_config->vdce_req_sz, regr(VDCE_REQ_SZ));

	regw(register_config->vdce_prcs_unit_size, VDCE_PRCS_UNIT_SIZE);
	dev_dbg(vdce_device, "VDCE_PRCS_UNIT_SIZE :	%x	regr = %x \n",
		register_config->vdce_prcs_unit_size,
		regr(VDCE_PRCS_UNIT_SIZE));

	regw(register_config->src_Y_strt_ps, SRC_Y_STRT_PS);
	dev_dbg(vdce_device, "SRC_Y_STRT_PS : %x	regr = %x \n",
		register_config->src_Y_strt_ps, regr(SRC_Y_STRT_PS));

	regw(register_config->src_Y_sz, SRC_Y_SZ);
	dev_dbg(vdce_device, "SRC_Y_SZ : %x	regr = %x \n",
		register_config->src_Y_sz, regr(SRC_Y_SZ));

	regw(register_config->src_C_strt_ps, SRC_C_STRT_PS);
	dev_dbg(vdce_device, "SRC_C_STRT_PS : %x	regr = %x \n",
		register_config->src_C_strt_ps, regr(SRC_C_STRT_PS));

	regw(register_config->src_C_sz, SRC_C_SZ);
	dev_dbg(vdce_device, "SRC_C_SZ : %x	regr = %x \n",
		register_config->src_C_sz, regr(SRC_C_SZ));

	regw(register_config->src_bmp_strt_ps, SRC_BMP_STRT_PS);
	dev_dbg(vdce_device, "SRC_BMP_STRT_PS : %x	regr = %x \n",
		register_config->src_bmp_strt_ps, regr(SRC_BMP_STRT_PS));

	regw(register_config->src_bmp_sz, SRC_BMP_SZ);
	dev_dbg(vdce_device, "SRC_BMP_SZ : %x	regr = %x \n",
		register_config->src_bmp_sz, regr(SRC_BMP_SZ));

	regw(register_config->res_Y_strt_ps, RES_Y_STRT_PS);
	dev_dbg(vdce_device, "RES_Y_STRT_PS : %x	regr = %x \n",
		register_config->res_Y_strt_ps, regr(RES_Y_STRT_PS));

	regw(register_config->res_Y_sz, RES_Y_SZ);
	dev_dbg(vdce_device, "RES_Y_SZ : %x	regr = %x \n",
		register_config->res_Y_sz, regr(RES_Y_SZ));

	regw(register_config->res_C_strt_ps, RES_C_STRT_PS);
	dev_dbg(vdce_device, "RES_C_STRT_PS : %x	regr = %x \n",
		register_config->res_C_strt_ps, regr(RES_C_STRT_PS));

	regw(register_config->res_C_sz, RES_C_SZ);
	dev_dbg(vdce_device, "RES_C_SZ : %x	regr = %x \n",
		register_config->res_C_sz, regr(RES_C_SZ));

	regw(register_config->res_bmp_strt_ps, RES_BMP_STRT_PS);
	dev_dbg(vdce_device, "RES_BMP_STRT_PS : %x	regr = %x \n",
		register_config->res_bmp_strt_ps, regr(RES_BMP_STRT_PS));

	regw(register_config->rsz_mode, RSZ_MODE);
	dev_dbg(vdce_device, "RSZ_MODE : %x	regr = %x \n",
		register_config->rsz_mode, regr(RSZ_MODE));

	regw(register_config->rsz_h_mag, RSZ_H_MAG);
	dev_dbg(vdce_device, "RSZ_H_MAG :	%x	regr = %x \n",
		register_config->rsz_h_mag, regr(RSZ_H_MAG));

	regw(register_config->rsz_v_mag, RSZ_V_MAG);
	dev_dbg(vdce_device, "RSZ_V_MAG :	%x	regr = %x \n",
		register_config->rsz_v_mag, regr(RSZ_V_MAG));

	regw(register_config->rsz_h_phase, RSZ_H_PHASE);
	dev_dbg(vdce_device, "RSZ_H_PHASE	: %x	regr = %x \n",
		register_config->rsz_h_phase, regr(RSZ_H_PHASE));

	regw(register_config->rsz_v_phase, RSZ_V_PHASE);
	dev_dbg(vdce_device, "RSZ_V_PHASE	: %x	regr = %x \n",
		register_config->rsz_v_phase, regr(RSZ_V_PHASE));

	regw(register_config->rsz_alf_intensity, RSZ_ALF_INTENSITY);
	dev_dbg(vdce_device, "RSZ_ALF_INTENSITY :	%x	regr = %x \n",
		register_config->rsz_alf_intensity, regr(RSZ_ALF_INTENSITY));

	regw(register_config->ccv_mode, CCV_MODE);
	dev_dbg(vdce_device, "CCV_MODE : %x	regr = %x \n",
		register_config->ccv_mode, regr(CCV_MODE));

	regw(register_config->bld_lut[0], BLD_LUT_00);
	dev_dbg(vdce_device, "BLD_LUT_00 : %x	regr = %x \n",
		register_config->bld_lut[0], regr(BLD_LUT_00));

	regw(register_config->bld_lut[1], BLD_LUT_01);
	dev_dbg(vdce_device, "BLD_LUT_01 : %x	regr = %x \n",
		register_config->bld_lut[1], regr(BLD_LUT_01));

	regw(register_config->bld_lut[2], BLD_LUT_02);
	dev_dbg(vdce_device, "BLD_LUT_02 : %x	regr = %x \n",
		register_config->bld_lut[2], regr(BLD_LUT_02));

	regw(register_config->bld_lut[3], BLD_LUT_03);
	dev_dbg(vdce_device, "BLD_LUT_03 : %x	regr = %x \n",
		register_config->bld_lut[3], regr(BLD_LUT_03));

	regw(register_config->rgmp_ctrl, RGMP_CTRL);
	dev_dbg(vdce_device, "RGMP_CTRL :	%x	regr = %x \n",
		register_config->rgmp_ctrl, regr(RGMP_CTRL));

	regw(register_config->epd_luma_width, EPD_LUMA_WIDTH);
	dev_dbg(vdce_device, "EPD_LUMA_WIDTH : %x	regr = %x \n",
		register_config->epd_luma_width, regr(EPD_LUMA_WIDTH));

	regw(register_config->epd_chroma_width, EPD_CHROMA_WIDTH);
	dev_dbg(vdce_device, "EPD_CHROMA_WIDTH : %x	regr = %x \n",
		register_config->epd_chroma_width, regr(EPD_CHROMA_WIDTH));

	/* source addres for luma and chroma */
	regw(register_config->vdce_src_strt_add_ytop, VDCE_SRC_STRT_ADD_YTOP);

	dev_dbg(vdce_device, "VDCE_SRC_STRT_ADD_YTOP %x	regr = %x\n",
		register_config->vdce_src_strt_add_ytop,
		regr(VDCE_SRC_STRT_ADD_YTOP));

	regw(register_config->vdce_src_strt_add_ctop, VDCE_SRC_STRT_ADD_CTOP);

	dev_dbg(vdce_device, "VDCE_SRC_STRT_ADD_CTOP %x	regr = %x\n",
		register_config->vdce_src_strt_add_ctop,
		regr(VDCE_SRC_STRT_ADD_CTOP));

	regw(register_config->vdce_src_strt_add_ybot, VDCE_SRC_STRT_ADD_YBOT);

	dev_dbg(vdce_device, "VDCE_SRC_STRT_ADD_YBOT %x	regr = %x\n",
		register_config->vdce_src_strt_add_ybot,
		regr(VDCE_SRC_STRT_ADD_YBOT));

	regw(register_config->vdce_src_strt_add_cbot, VDCE_SRC_STRT_ADD_CBOT);

	dev_dbg(vdce_device, "VDCE_SRC_STRT_ADD_CBOT %x	regr = %x\n",
		register_config->vdce_src_strt_add_cbot,
		regr(VDCE_SRC_STRT_ADD_CBOT));

	/* result address for luma and chroma */
	regw(register_config->vdce_res_strt_add_ytop, VDCE_RES_STRT_ADD_YTOP);

	dev_dbg(vdce_device, "VDCE_RES_STRT_ADD_YTOP %x	regr = %x\n",
		register_config->vdce_res_strt_add_ytop,
		regr(VDCE_RES_STRT_ADD_YTOP));

	regw(register_config->vdce_res_strt_add_ctop, VDCE_RES_STRT_ADD_CTOP);
	dev_dbg(vdce_device, "VDCE_RES_STRT_ADD_CTOP %x	regr = %x\n",
		register_config->vdce_res_strt_add_ctop,
		regr(VDCE_RES_STRT_ADD_CTOP));

	regw(register_config->vdce_res_strt_add_ybot, VDCE_RES_STRT_ADD_YBOT);

	dev_dbg(vdce_device, "VDCE_RES_STRT_ADD_YTOP %x	regr = %x\n",
		register_config->vdce_res_strt_add_ybot,
		regr(VDCE_RES_STRT_ADD_YBOT));

	regw(register_config->vdce_res_strt_add_cbot, VDCE_RES_STRT_ADD_CBOT);
	dev_dbg(vdce_device, "VDCE_RES_STRT_ADD_CBOT %x	regr = %x\n",
		register_config->vdce_res_strt_add_cbot,
		regr(VDCE_RES_STRT_ADD_CBOT));

	/* source offset for luma and chroma */
	regw(register_config->vdce_src_add_ofst_ytop, VDCE_SRC_STRT_OFF_YTOP);
	dev_dbg(vdce_device, "VDCE_SRC_STRT_OFF_YTOP %x	regr = %x\n",
		register_config->vdce_src_add_ofst_ytop,
		regr(VDCE_SRC_STRT_OFF_YTOP));

	regw(register_config->vdce_src_add_ofst_ctop, VDCE_SRC_STRT_OFF_CTOP);
	dev_dbg(vdce_device, "VDCE_SRC_STRT_OFF_CTOP %x	regr = %x\n",
		register_config->vdce_src_add_ofst_ctop,
		regr(VDCE_SRC_STRT_OFF_CTOP));

	regw(register_config->vdce_src_add_ofst_ybot, VDCE_SRC_STRT_OFF_YBOT);
	dev_dbg(vdce_device, "VDCE_SRC_STRT_OFF_YBOT %x	regr = %x\n",
		register_config->vdce_src_add_ofst_ybot,
		regr(VDCE_SRC_STRT_OFF_YBOT));

	regw(register_config->vdce_src_add_ofst_cbot, VDCE_SRC_STRT_OFF_CBOT);
	dev_dbg(vdce_device, "VDCE_SRC_STRT_OFF_CBOT %x	regr = %x\n",
		register_config->vdce_src_add_ofst_cbot,
		regr(VDCE_SRC_STRT_OFF_CBOT));

	/* result offset for luma and chroma */
	regw(register_config->vdce_res_strt_off_ytop, VDCE_RES_STRT_OFF_YTOP);
	dev_dbg(vdce_device, "VDCE_RES_STRT_OFF_YTOP %x	regr = %x\n",
		register_config->vdce_res_strt_off_ytop,
		regr(VDCE_RES_STRT_OFF_YTOP));

	regw(register_config->vdce_res_strt_off_ctop, VDCE_RES_STRT_OFF_CTOP);
	dev_dbg(vdce_device, "VDCE_RES_STRT_OFF_CTOP %x	regr = %x\n",
		register_config->vdce_res_strt_off_ctop,
		regr(VDCE_RES_STRT_OFF_CTOP));

	regw(register_config->vdce_res_strt_off_ybot, VDCE_RES_STRT_OFF_YBOT);
	dev_dbg(vdce_device, "VDCE_RES_STRT_OFF_YBOT %x	regr = %x\n",
		register_config->vdce_res_strt_off_ybot,
		regr(VDCE_RES_STRT_OFF_YBOT));

	regw(register_config->vdce_res_strt_off_cbot, VDCE_RES_STRT_OFF_CBOT);
	dev_dbg(vdce_device, "VDCE_RES_STRT_OFF_CBOT %x	regr = %x\n",
		register_config->vdce_res_strt_off_cbot,
		regr(VDCE_RES_STRT_OFF_CBOT));

	/* bitmap address and offset for luma and chroma */
	regw(register_config->vdce_src_strt_add_bmp_top,
	     VDCE_SRC_STRT_ADD_BMP_TOP);
	dev_dbg(vdce_device, "VDCE_SRC_STRT_ADD_BMP_TOP %x	regr = %x\n",
		register_config->vdce_src_strt_add_bmp_top,
		regr(VDCE_SRC_STRT_ADD_BMP_TOP));

	regw(register_config->vdce_src_strt_add_bmp_bot,
	     VDCE_SRC_STRT_ADD_BMP_BOT);
	dev_dbg(vdce_device, "VDCE_SRC_STRT_ADD_BMP_BOT %x	regr = %x\n",
		register_config->vdce_src_strt_add_bmp_bot,
		regr(VDCE_SRC_STRT_ADD_BMP_BOT));

	regw(register_config->vdce_src_strt_off_bmp_top,
	     VDCE_SRC_STRT_OFF_BMP_TOP);
	dev_dbg(vdce_device, "VDCE_SRC_STRT_OFF_BMP_TOP %x	regr = %x\n",
		register_config->vdce_src_strt_off_bmp_top,
		regr(VDCE_SRC_STRT_OFF_BMP_TOP));

	regw(register_config->vdce_src_strt_off_bmp_bot,
	     VDCE_SRC_STRT_OFF_BMP_BOT);

	status = regr(VDCE_STATUS);
	dev_dbg(vdce_device, "VDCE_SRC_STRT_OFF_BMP_BOT %x	regr = %x\n",
		register_config->vdce_src_strt_off_bmp_bot,
		regr(VDCE_SRC_STRT_OFF_BMP_BOT));
	dev_dbg(vdce_device, "vdce_hw_setup L\n");
}

/*
 * vdce_enable : Function to enable the	ge module
 */
int vdce_enable(vdce_hw_config_t * register_config)
{
	dev_dbg(vdce_device, " vdce_enable E\n");
	/* enabling the resizer the setting enable bit */
	register_config->vdce_ctrl =
	    BITSET(register_config->vdce_ctrl, VDCE_ENABLE);

	regw(register_config->vdce_ctrl, VDCE_CTRL);

	register_config->vdce_ctrl =
	    BITRESET(register_config->vdce_ctrl, VDCE_ENABLE);

	dev_dbg(vdce_device, "vdce enable L\n");

	return 0;
}

/*
 * vdce_interrupt set : Function to set interrupt
 */
inline int vdce_enable_int(void)
{
	regw(1, VDCE_INTEN);
	regw(1, VDCE_INTEN_SET);
	/* Disable emulation control signal */
	regw(1, VDCE_EMULATION_CTRL);
	return 0;
}

/*
 * vdce_clear_status : Function to clear the status
 */
inline int vdce_clear_status(void)
{
	regw(1, VDCE_STATUS_CLR);
	return 0;
}
