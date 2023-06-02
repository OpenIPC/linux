/*
 * Copyright (C) 2005-2009 Texas Instruments Inc
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
 */

#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/device.h>

#include <media/davinci/dm355_ipipe.h>
#include <media/davinci/vpss.h>

#include "dm355_ipipe_hw.h"

void ipipe_enable_reg_write(void)
{
	regw_ip(1, GCL_ARM);
	/*enable the clock wb,cfa,dfc,d2f,pre modules */
	regw_ip(0x06, GCL_CCD);
}

#ifdef CONFIG_IMP_DEBUG
void ipipe_hw_dump_config(void)
{
	u32 utemp;
	printk(KERN_NOTICE "IPIPEIF Registers\n");
	printk(KERN_NOTICE
	       "**************************************************\n");
	utemp = regr_if(IPIPEIF_ENABLE);
	printk(KERN_NOTICE "IPIPEIF ENABLE = 0x%x\n", utemp);
	utemp = regr_if(IPIPEIF_GFG);
	printk(KERN_NOTICE "IPIPEIF CFG = 0x%x\n", utemp);
	utemp = regr_if(IPIPEIF_PPLN);
	printk(KERN_NOTICE "IPIPEIF PPLN = 0x%x\n", utemp);
	utemp = regr_if(IPIPEIF_LPFR);
	printk(KERN_NOTICE "IPIPEIF LPFR = 0x%x\n", utemp);
	utemp = regr_if(IPIPEIF_HNUM);
	printk(KERN_NOTICE "IPIPEIF HNUM = 0x%x\n", utemp);
	utemp = regr_if(IPIPEIF_VNUM);
	printk(KERN_NOTICE "IPIPEIF VNUM = 0x%x\n", utemp);
	utemp = regr_if(IPIPEIF_ADDRU);
	printk(KERN_NOTICE "IPIPEIF ADDRU = 0x%x\n", utemp);
	utemp = regr_if(IPIPEIF_ADDRL);
	printk(KERN_NOTICE "IPIPEIF ADDRL = 0x%x\n", utemp);
	utemp = regr_if(IPIPEIF_ADOFS);
	printk(KERN_NOTICE "IPIPEIF ADOFS = 0x%x\n", utemp);
	utemp = regr_if(IPIPEIF_RSZ);
	printk(KERN_NOTICE "IPIPEIF RSZ = 0x%x\n", utemp);
	utemp = regr_if(IPIPEIF_GAIN);
	printk(KERN_NOTICE "IPIPEIF GAIN = 0x%x\n", utemp);
	printk(KERN_NOTICE "IPIPE Registers\n");
	printk(KERN_NOTICE
	       "**************************************************\n");
	utemp = regr_ip(IPIPE_EN);
	printk(KERN_NOTICE "IPIPE ENABLE = 0x%x\n", utemp);
	utemp = regr_ip(IPIPE_MODE);
	printk(KERN_NOTICE "IPIPE MODE = 0x%x\n", utemp);
	utemp = regr_ip(IPIPE_DPATHS);
	printk(KERN_NOTICE "IPIPE DPATHS = 0x%x\n", utemp);
	utemp = regr_ip(IPIPE_COLPAT);
	printk(KERN_NOTICE "IPIPE COLPAT = 0x%x\n", utemp);
	utemp = regr_ip(IPIPE_VST);
	printk(KERN_NOTICE "IPIPE VST = 0x%x\n", utemp);
	utemp = regr_ip(IPIPE_VSZ);
	printk(KERN_NOTICE "IPIPE VSZ = 0x%x\n", utemp);
	utemp = regr_ip(IPIPE_HST);
	printk(KERN_NOTICE "IPIPE HST = 0x%x\n", utemp);
	utemp = regr_ip(IPIPE_HSZ);
	printk(KERN_NOTICE "IPIPE HSZ = 0x%x\n", utemp);
	printk(KERN_NOTICE "Resizer Registers\n");
	printk(KERN_NOTICE
	       "**************************************************\n");
	utemp = regr_ip(RSZ_EN_0);
	printk(KERN_NOTICE "IPIPE RSZ 0 ENABLE = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_0 + RSZ_MODE));
	printk(KERN_NOTICE "IPIPE RSZ 0 MODE = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_0 + RSZ_I_VST));
	printk(KERN_NOTICE "IPIPE RSZ 0 I_VST = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_0 + RSZ_I_VSZ));
	printk(KERN_NOTICE "IPIPE RSZ 0 I_VSZ = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_0 + RSZ_I_HST));
	printk(KERN_NOTICE "IPIPE RSZ 0 I_HST = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_0 + RSZ_O_VSZ));
	printk(KERN_NOTICE "IPIPE RSZ 0 O_VSZ = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_0 + RSZ_O_HST));
	printk(KERN_NOTICE "IPIPE RSZ 0 O_HST = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_0 + RSZ_O_HSZ));
	printk(KERN_NOTICE "IPIPE RSZ 0 O_HSZ = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_0 + RSZ_V_PHS));
	printk(KERN_NOTICE "IPIPE RSZ 0 V_PHS = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_0 + RSZ_V_PHS_O));
	printk(KERN_NOTICE "IPIPE RSZ 0 V_PHS_O = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_0 + RSZ_V_DIF));
	printk(KERN_NOTICE "IPIPE RSZ 0 V_DIF = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_0 + RSZ_V_SIZ_O));
	printk(KERN_NOTICE "IPIPE RSZ 0 V_SIZ_O = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_0 + RSZ_H_PHS));
	printk(KERN_NOTICE "IPIPE RSZ 0 H_PHS = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_0 + RSZ_H_DIF));
	printk(KERN_NOTICE "IPIPE RSZ 0 H_DIF = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_0 + RSZ_H_TYP));
	printk(KERN_NOTICE "IPIPE RSZ 0 H_TYP = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_0 + RSZ_H_LSE));
	printk(KERN_NOTICE "IPIPE RSZ 0 H_LSE = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_0 + RSZ_H_LPF));
	printk(KERN_NOTICE "IPIPE RSZ 0 H_LPF = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_0 + RSZ_RGB_EN));
	printk(KERN_NOTICE "IPIPE RSZ 0 RGB_EN = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_0 + RSZ_RGB_TYP));
	printk(KERN_NOTICE "IPIPE RSZ 0 RGB_TYP = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_0 + RSZ_RGB_BLD));
	printk(KERN_NOTICE "IPIPE RSZ 0 RGB_BLD = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_0 + RSZ_SDR_BAD_H));
	printk(KERN_NOTICE "IPIPE RSZ 0 SDR_BAD_H = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_0 + RSZ_SDR_BAD_L));
	printk(KERN_NOTICE "IPIPE RSZ 0 SDR_BAD_L = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_0 + RSZ_SDR_SAD_H));
	printk(KERN_NOTICE "IPIPE RSZ 0 SDR_SAD_H = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_0 + RSZ_SDR_SAD_L));
	printk(KERN_NOTICE "IPIPE RSZ 0 SDR_SAD_L = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_0 + RSZ_SDR_OFT));
	printk(KERN_NOTICE "IPIPE RSZ 0 SDR_OFT = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_0 + RSZ_SDR_PTR_S));
	printk(KERN_NOTICE "IPIPE RSZ 0 SDR_PTR_S = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_0 + RSZ_SDR_PTR_E));
	printk(KERN_NOTICE "IPIPE RSZ 0 SDR_PTR_E = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_0 + RSZ_SDR_PTR_O));
	printk(KERN_NOTICE "IPIPE RSZ 0 SDR_PTR_O = 0x%x\n", utemp);
	utemp = regr_ip(RSZ_EN_1);
	printk(KERN_NOTICE
	       "**************************************************\n");
	printk(KERN_NOTICE "IPIPE RSZ 1 ENABLE = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_1 + RSZ_MODE));
	printk(KERN_NOTICE "IPIPE RSZ 1 MODE = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_1 + RSZ_I_VST));
	printk(KERN_NOTICE "IPIPE RSZ 1 I_VST = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_1 + RSZ_I_VSZ));
	printk(KERN_NOTICE "IPIPE RSZ 1 I_VSZ = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_1 + RSZ_I_HST));
	printk(KERN_NOTICE "IPIPE RSZ 1 I_HST = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_1 + RSZ_O_VSZ));
	printk(KERN_NOTICE "IPIPE RSZ 1 I_VSZ = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_1 + RSZ_O_HST));
	printk(KERN_NOTICE "IPIPE RSZ 1 O_HST = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_1 + RSZ_O_HSZ));
	printk(KERN_NOTICE "IPIPE RSZ 1 O_HSZ = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_1 + RSZ_V_PHS));
	printk(KERN_NOTICE "IPIPE RSZ 1 V_PHS = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_1 + RSZ_V_PHS_O));
	printk(KERN_NOTICE "IPIPE RSZ 1 V_PHS_O = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_1 + RSZ_V_DIF));
	printk(KERN_NOTICE "IPIPE RSZ 1 V_DIF = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_1 + RSZ_V_SIZ_O));
	printk(KERN_NOTICE "IPIPE RSZ 1 V_SIZ_O = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_1 + RSZ_H_PHS));
	printk(KERN_NOTICE "IPIPE RSZ 1 H_PHS = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_1 + RSZ_H_DIF));
	printk(KERN_NOTICE "IPIPE RSZ 1 H_DIF = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_1 + RSZ_H_TYP));
	printk(KERN_NOTICE "IPIPE RSZ 1 H_TYP = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_1 + RSZ_H_LSE));
	printk(KERN_NOTICE "IPIPE RSZ 1 H_LSE = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_1 + RSZ_H_LPF));
	printk(KERN_NOTICE "IPIPE RSZ 1 H_LPF = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_1 + RSZ_RGB_EN));
	printk(KERN_NOTICE "IPIPE RSZ 1 RGB_EN = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_1 + RSZ_RGB_TYP));
	printk(KERN_NOTICE "IPIPE RSZ 1 RGB_TYP = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_1 + RSZ_RGB_BLD));
	printk(KERN_NOTICE "IPIPE RSZ 1 RGB_BLD = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_1 + RSZ_SDR_BAD_H));
	printk(KERN_NOTICE "IPIPE RSZ 1 SDR_BAD_H = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_1 + RSZ_SDR_BAD_L));
	printk(KERN_NOTICE "IPIPE RSZ 1 SDR_BAD_L = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_1 + RSZ_SDR_SAD_H));
	printk(KERN_NOTICE "IPIPE RSZ 1 SDR_SAD_H = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_1 + RSZ_SDR_SAD_L));
	printk(KERN_NOTICE "IPIPE RSZ 1 SDR_SAD_L = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_1 + RSZ_SDR_OFT));
	printk(KERN_NOTICE "IPIPE RSZ 1 SDR_OFT = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_1 + RSZ_SDR_PTR_S));
	printk(KERN_NOTICE "IPIPE RSZ 1 SDR_PTR_S = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_1 + RSZ_SDR_PTR_E));
	printk(KERN_NOTICE "IPIPE RSZ 1 SDR_PTR_E = 0x%x\n", utemp);
	utemp = regr_ip((RSZ_EN_1 + RSZ_SDR_PTR_O));
	printk(KERN_NOTICE "IPIPE RSZ 1 SDR_PTR_O = 0x%x\n", utemp);
}
#else
void ipipe_hw_dump_config(void)
{
}
#endif

/* ipipe_hw_setup:It is used for Hardware Setup */
int ipipe_hw_setup(struct ipipe_params *config)
{
	u32 utemp = 0;
	u32 data_format;
	enum ipipeif_decimation decimation_en;
	if (!config)
		return -EINVAL;

	/*Combine all the fields to make CFG register of IPIPEIF */
	utemp = config->ipipeif_param.mode << 0;
	utemp |= config->ipipeif_param.decimation << 1;
	utemp |= config->ipipeif_param.source << 2;
	utemp |= config->ipipeif_param.clk_div << 4;
	utemp |= config->ipipeif_param.avg_filter << 7;
	utemp |= config->ipipeif_param.pack_mode << 8;
	utemp |= config->ipipeif_param.ialaw << 9;
	utemp |= config->ipipeif_param.clock_select << 10;
	utemp |= config->ipipeif_param.data_shift << 11;
	regw_if(utemp, IPIPEIF_GFG);
	switch (config->ipipeif_param.source) {
	case CCDC:
		regw_if(config->ipipeif_param.gain, IPIPEIF_GAIN);
		break;

	case SDRAM_RAW:
	case CCDC_DARKFM:
		regw_if(config->ipipeif_param.gain, IPIPEIF_GAIN);

	case SDRAM_YUV:
		regw_if(config->ipipeif_param.glob_hor_size, IPIPEIF_PPLN);
		regw_if(config->ipipeif_param.glob_ver_size, IPIPEIF_LPFR);
		regw_if(config->ipipeif_param.hnum, IPIPEIF_HNUM);
		regw_if(config->ipipeif_param.vnum, IPIPEIF_VNUM);
		vpss_dm355_assign_rblctrl_master(DM355_RBLCTRL_IPIPEIF);
		break;

	}
	/*check if decimation is enable or not */
	decimation_en = config->ipipeif_param.decimation;
	if (decimation_en)
		regw_if(config->ipipeif_param.rsz, IPIPEIF_RSZ);

	/*set GCL_ARM reg before writting to ipipe registers */
	ipipe_enable_reg_write();

	data_format =
	    (config->ipipe_dpaths_fmt | (config->ipipe_dpaths_bypass) << 2);

	vpss_dm355_assign_wblctrl_master(DM355_WBLCTRL_IPIPE);

	/*enable ipipe mode to either one shot or continuous */
	utemp = regr_ip(IPIPE_MODE);
	RESETBIT(utemp, 0);	/*first reset mode bit and then set it by
				   config->ipipe_mode */
	utemp |= config->ipipe_mode;
	regw_ip(utemp, IPIPE_MODE);
	regw_ip(data_format, IPIPE_DPATHS);
	/*set size */
	regw_ip(config->ipipe_vst, IPIPE_VST);
	regw_ip(config->ipipe_hst, IPIPE_HST);
	regw_ip(config->ipipe_vsz, IPIPE_VSZ);
	regw_ip(config->ipipe_hsz, IPIPE_HSZ);
	switch (data_format) {
	case IPIPE_RAW2YUV:

		/*Combine all the fields to make COLPAT register of IPIPE */
		utemp = (config->ipipe_colpat_elep << 0);
		utemp |= (config->ipipe_colpat_elop << 2);
		utemp |= (config->ipipe_colpat_olep << 4);
		utemp |= (config->ipipe_colpat_olop << 6);

		regw_ip(utemp, IPIPE_COLPAT);
		ipipe_set_rsz_regs(config);	/*set RSZ_SEQ registers */
		ipipe_set_aal_regs(config);	/*set RSZ_AAL registers */
		/*set the registers of either RSZ0 or RSZ1 */
		ipipe_set_rsz_structs(config);

		break;

	case IPIPE_RAW2RAW:
		ipipe_set_aal_regs(config);	/*set RSZ_AAL registers */
		/*set the registers of RSZ0 and RSZ1 */
		ipipe_set_rsz_structs(config);	/*...................CHECK */
		break;

	case IPIPE_YUV2YUV:
		ipipe_set_rsz_regs(config);	/*set RSZ_SEQ registers */
		ipipe_set_aal_regs(config);	/*set RSZ_AAL registers */
		/*set the registers of either RSZ0 or RSZ1 */
		ipipe_set_rsz_structs(config);
		break;
	}
	return 0;
}

int ipipe_set_resizer_address(struct ipipe_params *params,
			      int resize_no, unsigned int address)
{
	unsigned int utemp;
	unsigned int rsz_start_add;

	if (ISNULL(params)) {
		printk(KERN_ERR "null ptr for params\n");
		return -1;
	}

	if ((resize_no != 0) && (resize_no != 1)) {
		printk(KERN_ERR "invalid resizer number\n");
		return -1;
	}

	if (resize_no)
		rsz_start_add = RSZ_EN_1;
	else
		rsz_start_add = RSZ_EN_0;
	utemp = address & SET_LOW_ADD;

	/* resizer must be enabled */
	regw_ip(params->rsz_en[resize_no], rsz_start_add);
	if ((params->rsz_en[resize_no]) && (address == 0)) {
		printk(KERN_ERR "invalid address for resizer %d\n", resize_no);
		return -1;
	}
	regw_ip(params->ext_mem_param[resize_no].rsz_sdr_oft,
		rsz_start_add + RSZ_SDR_OFT);

	regw_ip(utemp, rsz_start_add + RSZ_SDR_BAD_L);
	regw_ip(utemp, rsz_start_add + RSZ_SDR_SAD_L);

	utemp = (address & SET_HIGH_ADD) >> 16;
	regw_ip(utemp, rsz_start_add + RSZ_SDR_BAD_H);
	regw_ip(utemp, rsz_start_add + RSZ_SDR_SAD_H);
	return 0;
}

int ipipe_set_ipipeif_address(struct ipipe_params *params, unsigned int address)
{
	unsigned int utemp, utemp_h, utemp_l;
	if (params->ipipeif_param.source != 0) {
		regw_if(((params->ipipeif_param.adofs) >> 5), IPIPEIF_ADOFS);

		/*lower sixteen bit */
		utemp = address;
		utemp_l = utemp >> 5;
		regw_if(utemp_l, IPIPEIF_ADDRL);
		/*upper next seven bit */
		utemp_h = utemp >> 21;
		regw_if(utemp_h, IPIPEIF_ADDRU);
	} else {
		return -1;
	}
	return 0;
}

/*default configuratins for RAW2RAW mode*/
int ipipe_default_raw2raw(struct ipipe_params *parameter)
{
	u32 utemp;
	u32 bright = 0;
	u32 contrast = 16;

	vpss_dm355_assign_dfc_memory_master(DM355_DFC_MEM_IPIPE);
	vpss_dm355_assign_int_memory_master(DM355_INT_MEM_IPIPE);
	vpss_dm355_ipipe_enable_any_address(1);
	regw_ip(1, GCL_SDR);

	/*set this to 0 for dafault config */
	regw_ip(0, FCS_EN);
	/*set this to 0 for dafault config */
	regw_ip(0, YEE_EN);
	/*set default brightness and contrast */
	utemp = ((contrast << 0) | (bright << 8));
	regw_ip(utemp, YUV_ADJ);

	/*set default luminance */
	regw_ip(0, YUV_Y_MIN);
	regw_ip(255, YUV_Y_MAX);

	/*set default chrominance */
	regw_ip(0, YUV_C_MIN);
	regw_ip(255, YUV_C_MAX);
	/*default config for resizer 1  registers */
	ipipe_default_bypass_resizer(parameter);
	return 0;
}

/*default configuratins for RAW2RAW mode*/
int ipipe_default_bypass_resizer(struct ipipe_params *parameter)
{
	u32 utemp;
	int seq_tmm = 0;

	regw_ip(1, GCL_SDR);

	/*set this to 0 for dafault config */
	utemp =
	    (parameter->rsz_seq_seq << 0) | (seq_tmm << 1) | (parameter->
							      rsz_seq_hrv << 2)
	    | (parameter->rsz_seq_vrv << 3) | (parameter->rsz_seq_crv << 4);
	regw_ip(utemp, RSZ_SEQ);
	/*set this to 0 for dafault config */
	regw_ip(1, RSZ_EN_0);
	regw_ip(0, RSZ_EN_0 + RSZ_I_HST);
	regw_ip(0, RSZ_EN_0 + RSZ_I_VST);
	regw_ip(0, RSZ_EN_0 + RSZ_O_HST);
	regw_ip(0, RSZ_EN_0 + RSZ_V_PHS);
	regw_ip(256, RSZ_EN_0 + RSZ_V_DIF);
	regw_ip(256, RSZ_EN_0 + RSZ_H_DIF);
	regw_ip(0, RSZ_EN_0 + RSZ_H_PHS);
	regw_ip(0, RSZ_EN_0 + RSZ_H_TYP);
	regw_ip(0, RSZ_EN_0 + RSZ_H_LSE);
	regw_ip(0, RSZ_EN_0 + RSZ_H_LPF);
	regw_ip(0, RSZ_EN_0 + RSZ_RGB_EN);
	/*disable resizer 0 in default mode */
	regw_ip(0, RSZ_EN_1);
	return 0;
}

/*default configuratins for RAW2RAW_bypass mode*/
int ipipe_default_bypass_ycbcr(struct ipipe_params *parameter)
{
	/*disable noise filter in default config */
	regw_ip(0, D2F_EN);
	/*disable defect coorection in default config */
	regw_ip(0, DFC_EN);
	/*disable prefilter filter in default config */
	regw_ip(0, PRE_EN);
	/*set default config for white balance */
	regw_ip(256, WB2_DGN);
	regw_ip(128, WB2_WG_R);
	regw_ip(128, WB2_WG_GR);
	regw_ip(128, WB2_WG_GB);
	regw_ip(128, WB2_WG_B);
	return 0;
}

int ipipe_set_dfc_regs(struct prev_dfc *dfc)
{
	u32 utemp;
	unsigned int count;

	ipipe_enable_reg_write();
	vpss_dm355_assign_wblctrl_master(DM355_WBLCTRL_IPIPE);
	regw_ip(dfc->en, DFC_EN);	/*writting to enable register */
	if (1 == dfc->en) {
		regw_ip(dfc->vert_copy_method, DFC_SEL);
		regw_ip(DEF_COR_START_ADDR, DFC_ADR);
		regw_ip(dfc->dfc_size, DFC_SIZE);
		vpss_dm355_assign_dfc_memory_master(DM355_DFC_MEM_IPIPE);

		/* set the auto increment,write only,dfc mode in RAM_MODE */
		regw_ip(0x0034, RAM_MODE);
		regw_ip(0x00, RAM_ADR);
		if (dfc->table != NULL) {
			count = 0;
			while (count < dfc->dfc_size) {
				regw_ip(dfc->table[count].horz_pos, RAM_WDT);
				printk(KERN_INFO "###RAM_WDT[%d] = %x\n", count,
				       regr_ip(RAM_WDT));
				/*write next fifteen bit */
				utemp =
				    (dfc->table[count].
				     vert_pos | (dfc->table[count].
						 method << 12));
				regw_ip(utemp, RAM_WDT);
				printk(KERN_INFO "RAM_WDT[%d] = %x\n", count,
				       regr_ip(RAM_WDT));
				count++;
			}
		}
	}
	return 0;
}

/* 2D Noise filter */
int ipipe_set_d2f_nf_regs(struct prev_nf *noise_filter)
{
	u32 utemp;
	int count = 0;

	ipipe_enable_reg_write();
	regw_ip(noise_filter->en, D2F_EN);
	if (1 == noise_filter->en) {
		/*Combine all the fields to make D2F_CFG register of IPIPE */
		utemp =
		    (noise_filter->spread_val << 0) |
		    (noise_filter->shft_val << 2) |
		    (noise_filter->gr_sample_meth << 4);
		regw_ip(utemp, D2F_CFG);
		count = 0;
		while (count < IPIPE_NF_STR_TABLE_SIZE) {
			regw_ip(noise_filter->str[count], D2F_STR + count * 4);
			count++;
		}
		count = 0;
		while (count < IPIPE_NF_THR_TABLE_SIZE) {
			regw_ip(noise_filter->thr[count], DFC_THR + count * 4);
			count++;
		}
	}
	return 0;
}

/* Pre filter */
int ipipe_set_pf_regs(struct prev_prefilter *pre_filter)
{
	u32 utemp;

	ipipe_enable_reg_write();
	regw_ip(pre_filter->en, PRE_EN);
	if (1 == pre_filter->en) {
		/*Combine all the fields to make PRE_EN register of IPIPE */
		utemp = ((pre_filter->aver_meth_gs2 << 0) |
			 (pre_filter->aver_meth_gs1 << 1) |
			 (pre_filter->en_adapt_prefilter << 2) |
			 (pre_filter->en_adapt_dotred << 3));
		regw_ip(utemp, PRE_TYP);
		regw_ip(pre_filter->pre_shf, PRE_SHF);
		regw_ip(pre_filter->pre_gain, PRE_GAIN);
		regw_ip(pre_filter->pre_thr_g, PRE_THR_G);
		regw_ip(pre_filter->pre_thr_b, PRE_THR_B);
		regw_ip(pre_filter->pre_thr_1, PRE_THR_1);
	}
	return 0;
}

/* White balance */
#define IPIPE_U10Q7(decimal,integer) \
	(((decimal & 0x7f) | ((integer & 0x7) << 7)))
#define IPIPE_U10Q8(decimal,integer) \
	(((decimal & 0xff) | ((integer & 0x3) << 8)))
int ipipe_set_wb_regs(struct prev_wb *wb)
{
	u32 utemp;

	/* U10Q7 */
	utemp = IPIPE_U10Q8((wb->dgn.decimal), (wb->dgn.integer));
	ipipe_enable_reg_write();
	regw_ip(utemp, WB2_DGN);
	utemp = IPIPE_U10Q7((wb->gain_r.decimal), (wb->gain_r.integer));
	regw_ip(utemp, WB2_WG_R);
	utemp = IPIPE_U10Q7((wb->gain_gr.decimal), (wb->gain_gr.integer));
	regw_ip(utemp, WB2_WG_GR);
	utemp = IPIPE_U10Q7((wb->gain_gb.decimal), (wb->gain_gb.integer));
	regw_ip(utemp, WB2_WG_GB);
	utemp = IPIPE_U10Q7((wb->gain_b.decimal), (wb->gain_b.integer));
	regw_ip(utemp, WB2_WG_B);
	return 0;
}

#define IPIPE_S10Q8(decimal,integer) \
	(((decimal & 0xff) | ((integer & 0x3) << 8)))
/* RGB2YUV */
int ipipe_set_rgb2ycbcr_regs(struct prev_rgb2yuv *yuv)
{
	u32 utemp;

	/* S10Q8 */
	ipipe_enable_reg_write();
	utemp = IPIPE_S10Q8((yuv->coef_ry.decimal), (yuv->coef_ry.integer));
	regw_ip(utemp, YUV_MUL_RY);
	utemp = IPIPE_S10Q8((yuv->coef_gy.decimal), (yuv->coef_gy.integer));
	regw_ip(utemp, YUV_MUL_GY);
	utemp = IPIPE_S10Q8((yuv->coef_by.decimal), (yuv->coef_by.integer));
	regw_ip(utemp, YUV_MUL_BY);
	utemp = IPIPE_S10Q8((yuv->coef_rcb.decimal), (yuv->coef_rcb.integer));
	regw_ip(utemp, YUV_MUL_RCB);
	utemp = IPIPE_S10Q8((yuv->coef_gcb.decimal), (yuv->coef_gcb.integer));
	regw_ip(utemp, YUV_MUL_GCB);
	utemp = IPIPE_S10Q8((yuv->coef_bcb.decimal), (yuv->coef_bcb.integer));
	regw_ip(utemp, YUV_MUL_BCB);
	utemp = IPIPE_S10Q8((yuv->coef_rcr.decimal), (yuv->coef_rcr.integer));
	regw_ip(utemp, YUV_MUL_RCR);
	utemp = IPIPE_S10Q8((yuv->coef_gcr.decimal), (yuv->coef_gcr.integer));
	regw_ip(utemp, YUV_MUL_GCR);
	utemp = IPIPE_S10Q8((yuv->coef_bcr.decimal), (yuv->coef_bcr.integer));
	regw_ip(utemp, YUV_MUL_BCR);
	regw_ip(yuv->out_ofst_y, YUV_OFT_Y);
	regw_ip(yuv->out_ofst_cb, YUV_OFT_CB);
	regw_ip(yuv->out_ofst_cr, YUV_OFT_CR);
	return 0;
}

/* Lumina adjustments */
int ipipe_set_lum_adj_regs(struct prev_lum_adj *lum_adj)
{
	u32 utemp;

	ipipe_enable_reg_write();
	/*combine fields of YUV_ADJ to set brightness and contrast */
	utemp = ((lum_adj->contast << 0) | (lum_adj->brightness << 8));
	regw_ip(utemp, YUV_ADJ);
	return 0;
}

/* YUV 422 conversion */
int ipipe_set_yuv422_conv_regs(struct prev_yuv422_conv *conv)
{
	u32 utemp;

	ipipe_enable_reg_write();
	regw_ip(conv->lum_min, YUV_Y_MIN);
	regw_ip(conv->lum_max, YUV_Y_MAX);
	regw_ip(conv->chrom_min, YUV_C_MIN);
	regw_ip(conv->chrom_max, YUV_C_MAX);
	/*Combine all the fields to make YUV_PHS register of IPIPE */
	utemp = ((conv->chrom_pos << 0) | (conv->en_chrom_lpf << 1));
	regw_ip(utemp, YUV_PHS);
	return 0;
}

/* rgb2rgb conversion */
#define IPIPE_S12Q8(decimal,integer) \
	(((decimal & 0xff) | ((integer & 0xf) << 8)))

int ipipe_set_rgb2rgb_regs(struct prev_rgb2rgb *rgb)
{
	u32 utemp;

	ipipe_enable_reg_write();
	utemp = IPIPE_S12Q8((rgb->coef_rr.decimal), (rgb->coef_rr.integer));
	regw_ip(utemp, RGB_MUL_RR);
	utemp = IPIPE_S12Q8((rgb->coef_gr.decimal), (rgb->coef_gr.integer));
	regw_ip(utemp, RGB_MUL_GR);
	utemp = IPIPE_S12Q8((rgb->coef_br.decimal), (rgb->coef_br.integer));
	regw_ip(utemp, RGB_MUL_BR);
	utemp = IPIPE_S12Q8((rgb->coef_rg.decimal), (rgb->coef_rg.integer));
	regw_ip(utemp, RGB_MUL_RG);
	utemp = IPIPE_S12Q8((rgb->coef_gg.decimal), (rgb->coef_gg.integer));
	regw_ip(utemp, RGB_MUL_GG);
	utemp = IPIPE_S12Q8((rgb->coef_bg.decimal), (rgb->coef_bg.integer));
	regw_ip(utemp, RGB_MUL_BG);
	utemp = IPIPE_S12Q8((rgb->coef_rb.decimal), (rgb->coef_rb.integer));
	regw_ip(utemp, RGB_MUL_RB);
	utemp = IPIPE_S12Q8((rgb->coef_gb.decimal), (rgb->coef_gb.integer));
	regw_ip(utemp, RGB_MUL_GB);
	utemp = IPIPE_S12Q8((rgb->coef_bb.decimal), (rgb->coef_bb.integer));
	regw_ip(utemp, RGB_MUL_BB);
	regw_ip(rgb->out_ofst_r, RGB_MUL_OR);
	regw_ip(rgb->out_ofst_g, RGB_MUL_OG);
	regw_ip(rgb->out_ofst_b, RGB_MUL_OB);
	return 0;
}

/* Gamma correction */
int ipipe_set_gamma_regs(struct prev_gamma *gamma)
{
	u32 utemp;
	int count, table_size = 0;

	ipipe_enable_reg_write();
	utemp = ((gamma->bypass_r << 0) |
		 (gamma->bypass_b << 1) |
		 (gamma->bypass_g << 2) |
		 (gamma->tbl_sel << 4) | (gamma->tbl_size << 5));

	regw_ip(utemp, GMM_CFG);

	vpss_dm355_assign_wblctrl_master(DM355_WBLCTRL_IPIPE);
	if (gamma->tbl_sel == IPIPE_GAMMA_TBL_RAM) {
		if (gamma->tbl_size == IPIPE_GAMMA_TBL_SZ_128)
			table_size = 128;
		else if (gamma->tbl_size == IPIPE_GAMMA_TBL_SZ_256)
			table_size = 256;
		else if (gamma->tbl_size == IPIPE_GAMMA_TBL_SZ_512)
			table_size = 512;
		if (!(gamma->bypass_r)) {
			if (gamma->table_r != NULL) {
				/*set the auto increment,write only, gamma
				   red mode in RAM_MODE */
				regw_ip(0x0035, RAM_MODE);
				/*set the starting address of gamma table */
				regw_ip(0x00, RAM_ADR);

				for (count = 0; count < table_size; count++) {
					regw_ip(gamma->table_r[count].slope,
						RAM_WDT);
					regw_ip(gamma->table_r[count].offset,
						RAM_WDT);
				}
			}
		}
		if (!(gamma->bypass_b)) {
			if (gamma->table_b != NULL) {
				/*set the auto increment,write only,
				   gamma red mode in RAM_MODE */
				regw_ip(0x0036, RAM_MODE);
				/*set the starting address of gamma table */
				regw_ip(0x00, RAM_ADR);
				for (count = 0; count < table_size; count++) {
					regw_ip(gamma->table_b[count].slope,
						RAM_WDT);
					regw_ip(gamma->table_b[count].offset,
						RAM_WDT);
				}
			}
		}
		if (!(gamma->bypass_g)) {
			if (gamma->table_g != NULL) {
				/*set the auto increment,write only, gamma red
				   mode in RAM_MODE */
				regw_ip(0x0037, RAM_MODE);
				/*set the starting address of gamma table */
				regw_ip(0x00, RAM_ADR);
				for (count = 0; count < table_size; count++) {
					regw_ip(gamma->table_g[count].slope,
						RAM_WDT);
					regw_ip(gamma->table_g[count].offset,
						RAM_WDT);
				}
			}
		}

		/*set the auto increment,write only, gamma red mode in
		   RAM_MODE */
		regw_ip(0x0038, RAM_MODE);
		/*set the starting address of gamma table */
		regw_ip(0x00, RAM_ADR);
		if (gamma->table_rgb_all != NULL) {
			for (count = 0; count < table_size; count++) {
				regw_ip(gamma->table_rgb_all[count].slope,
					RAM_WDT);
				regw_ip(gamma->table_rgb_all[count].offset,
					RAM_WDT);
			}
		}
		regw_ip(0x00, RAM_MODE);
	}
	return 0;
}

/* Edge Enhancement */
int ipipe_set_ee_regs(struct prev_yee *ee)
{
	unsigned int count;

	ipipe_enable_reg_write();
	vpss_dm355_assign_wblctrl_master(DM355_WBLCTRL_IPIPE);
	regw_ip(ee->en, YEE_EN);
	if (1 == ee->en) {
		regw_ip(ee->en_emf, YEE_EMF);
		regw_ip(ee->hpf_shft, YEE_SHF);
		regw_ip(ee->hpf_coef_00, YEE_MUL_00);
		regw_ip(ee->hpf_coef_01, YEE_MUL_01);
		regw_ip(ee->hpf_coef_02, YEE_MUL_02);
		regw_ip(ee->hpf_coef_10, YEE_MUL_10);
		regw_ip(ee->hpf_coef_11, YEE_MUL_11);
		regw_ip(ee->hpf_coef_12, YEE_MUL_12);
		regw_ip(ee->hpf_coef_20, YEE_MUL_20);
		regw_ip(ee->hpf_coef_21, YEE_MUL_21);
		regw_ip(ee->hpf_coef_22, YEE_MUL_22);

		/*set the auto increment,write only,ee mode in RAM_MODE */
		regw_ip(0x0039, RAM_MODE);
		regw_ip(0x000 /*0x1FF */ , RAM_ADR);
		if (ee->table != NULL) {
			for (count = 0; count < MAX_SIZE_EEC; count++)
				regw_ip(ee->table[count], RAM_WDT);
		}
	}
	return 0;
}

/* False color suppression */
int ipipe_set_fcs_regs(struct prev_fcs *fcs)
{
	ipipe_enable_reg_write();
	regw_ip(fcs->en, FCS_EN);
	if (1 == fcs->en) {
		regw_ip(fcs->type, FCS_TYP);
		regw_ip(fcs->hpf_shft_y, FCS_SHF_Y);
		regw_ip(fcs->gain_shft_c, FCS_SHF_C);
		regw_ip(fcs->thr, FCS_THR);
		regw_ip(fcs->sgn, FCS_SGN);
		regw_ip(fcs->lth, FCS_LTH);
	}
	return 0;
}

int ipipe_set_rsz_regs(struct ipipe_params *param_resize)
{
	u32 utemp;
	ipipe_enable_reg_write();
	/*Combine all the fields to make RSZ_SEQ register of IPIPE */
	utemp =
	    (param_resize->rsz_seq_seq << 0) |
	    (param_resize->rsz_seq_tmm << 1) |
	    (param_resize->rsz_seq_hrv << 2) |
	    (param_resize->rsz_seq_vrv << 3) | (param_resize->rsz_seq_crv << 4);
	regw_ip(utemp, RSZ_SEQ);
	return 0;
}

int ipipe_set_aal_regs(struct ipipe_params *param_resize)
{
	regw_ip(param_resize->rsz_aal, RSZ_AAL);
	return 0;
}

int ipipe_set_output_size(struct ipipe_params *params)
{
	vpss_dm355_assign_dfc_memory_master(DM355_DFC_MEM_IPIPE);
	vpss_dm355_assign_int_memory_master(DM355_INT_MEM_IPIPE);
	vpss_dm355_ipipe_enable_any_address(1);
	regw_ip(1, GCL_SDR);
	/*setting rescale parameters */
	regw_ip(params->rsz_rsc_param[0].rsz_o_vsz, RSZ_EN_0 + RSZ_O_VSZ);
	regw_ip(params->rsz_rsc_param[0].rsz_o_hsz, RSZ_EN_0 + RSZ_O_HSZ);
	regw_ip(params->rsz_rsc_param[0].rsz_v_dif, RSZ_EN_0 + RSZ_V_DIF);
	regw_ip(params->rsz_rsc_param[0].rsz_h_dif, RSZ_EN_0 + RSZ_H_DIF);
	regw_ip(params->rsz_rsc_param[1].rsz_o_vsz, RSZ_EN_1 + RSZ_O_VSZ);
	regw_ip(params->rsz_rsc_param[1].rsz_o_hsz, RSZ_EN_1 + RSZ_O_HSZ);
	regw_ip(params->rsz_rsc_param[1].rsz_v_dif, RSZ_EN_1 + RSZ_V_DIF);
	regw_ip(params->rsz_rsc_param[1].rsz_h_dif, RSZ_EN_1 + RSZ_H_DIF);
	return 0;
}

int ipipe_set_output_offsets(int resizer, struct ipipe_params *params)
{
	regw_ip(1, GCL_SDR);
	if (resizer == 0) {
		regw_ip(params->ext_mem_param[0].rsz_sdr_oft,
			RSZ_EN_0 + RSZ_SDR_OFT);
		regw_ip(params->ext_mem_param[0].rsz_sdr_ptr_s,
			RSZ_EN_0 + RSZ_SDR_PTR_S);
		regw_ip(params->ext_mem_param[0].rsz_sdr_ptr_e,
			RSZ_EN_0 + RSZ_SDR_PTR_E);

	} else {
		regw_ip(params->ext_mem_param[1].rsz_sdr_oft,
			RSZ_EN_0 + RSZ_SDR_OFT);
		regw_ip(params->ext_mem_param[0].rsz_sdr_ptr_s,
			RSZ_EN_0 + RSZ_SDR_PTR_S);
		regw_ip(params->ext_mem_param[0].rsz_sdr_ptr_e,
			RSZ_EN_0 + RSZ_SDR_PTR_E);
	}
	return 0;
}

int rsz_enable(int rsz_id, int enable)
{
	if (enable != 1)
		enable = 0;
	switch (rsz_id) {
	case 0:
		regw_ip(enable, RSZ_EN_0);
		break;
	case 1:
		regw_ip(enable, RSZ_EN_1);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

int ipipe_set_rsz_structs(struct ipipe_params *params)
{				/*set the registers of either RSZ0 or RSZ1 */
	u32 utemp;
	u32 rsz_seq, rsz_tmm;

	vpss_dm355_assign_dfc_memory_master(DM355_DFC_MEM_IPIPE);
	vpss_dm355_assign_int_memory_master(DM355_INT_MEM_IPIPE);
	vpss_dm355_ipipe_enable_any_address(1);
	regw_ip(params->rsz_en[0], RSZ_EN_0);
	if (params->rsz_en[0]) {
		printk(KERN_DEBUG
		       "ipipe_set_rsz_structs, resizer - 0 enabled\n");
		/*testing--- for register write */
		utemp = regr_ip(RSZ_EN_0);
		/*enable RSZ clock */
		regw_ip(1, GCL_SDR);
		/*setting rescale parameters */
		regw_ip(params->rsz_rsc_param[0].rsz_mode, RSZ_EN_0 + RSZ_MODE);
		regw_ip(params->rsz_rsc_param[0].rsz_i_vst,
			RSZ_EN_0 + RSZ_I_VST);
		regw_ip(params->rsz_rsc_param[0].rsz_i_vsz,
			RSZ_EN_0 + RSZ_I_VSZ);
		regw_ip(params->rsz_rsc_param[0].rsz_i_hst,
			RSZ_EN_0 + RSZ_I_HST);
		regw_ip(params->rsz_rsc_param[0].rsz_o_vsz,
			RSZ_EN_0 + RSZ_O_VSZ);
		regw_ip(params->rsz_rsc_param[0].rsz_o_hsz,
			RSZ_EN_0 + RSZ_O_HSZ);
		regw_ip(params->rsz_rsc_param[0].rsz_o_hst,
			RSZ_EN_0 + RSZ_O_HST);
		regw_ip(params->rsz_rsc_param[0].rsz_v_phs,
			RSZ_EN_0 + RSZ_V_PHS);
		regw_ip(params->rsz_rsc_param[0].rsz_v_dif,
			RSZ_EN_0 + RSZ_V_DIF);
		regw_ip(params->rsz_rsc_param[0].rsz_h_phs,
			RSZ_EN_0 + RSZ_H_PHS);
		regw_ip(params->rsz_rsc_param[0].rsz_h_dif,
			RSZ_EN_0 + RSZ_H_DIF);
		regw_ip(params->rsz_rsc_param[0].rsz_h_typ,
			RSZ_EN_0 + RSZ_H_TYP);
		regw_ip(params->rsz_rsc_param[0].rsz_h_lse_sel,
			RSZ_EN_0 + RSZ_H_LSE);
		regw_ip(params->rsz_rsc_param[0].rsz_h_lpf,
			RSZ_EN_0 + RSZ_H_LPF);

		/*seting rgb conversion parameters */
		regw_ip(params->rsz2rgb[0].rsz_rgb_en, RSZ_EN_0 + RSZ_RGB_EN);
		regw_ip(params->rsz2rgb[0].rsz_rgb_en, RSZ_EN_0 + RSZ_RGB_EN);
		utemp =
		    ((params->rsz2rgb[0].rsz_rgb_typ << 0) |
		     (params->rsz2rgb[0].rsz_rgb_msk0 << 1) |
		     (params->rsz2rgb[0].rsz_rgb_msk1) << 2);
		regw_ip(utemp, RSZ_RGB_TYP);
		regw_ip(params->rsz2rgb[0].rsz_rgb_alpha_val,
			RSZ_EN_0 + RSZ_RGB_BLD);

		/*setting external memory parameters */
		regw_ip(params->ext_mem_param[0].rsz_sdr_oft,
			RSZ_EN_0 + RSZ_SDR_OFT);
		regw_ip(params->ext_mem_param[0].rsz_sdr_ptr_s,
			RSZ_EN_0 + RSZ_SDR_PTR_S);
		regw_ip(params->ext_mem_param[0].rsz_sdr_ptr_e,
			RSZ_EN_0 + RSZ_SDR_PTR_E);
	}

	regw_ip(params->rsz_en[1], RSZ_EN_1);
	if (params->rsz_en[1]) {
		/*testing---- for register write */
		utemp = regr_ip(RSZ_EN_1);

		/*enable RSZ clock */
		regw_ip(1, GCL_SDR);
		/*setting rescale parameters */
		regw_ip(params->rsz_rsc_param[1].rsz_mode, RSZ_EN_1 + RSZ_MODE);
		regw_ip(params->rsz_rsc_param[1].rsz_i_vst,
			RSZ_EN_1 + RSZ_I_VST);
		regw_ip(params->rsz_rsc_param[1].rsz_i_vsz,
			RSZ_EN_1 + RSZ_I_VSZ);
		regw_ip(params->rsz_rsc_param[1].rsz_i_hst,
			RSZ_EN_1 + RSZ_I_HST);
		regw_ip(params->rsz_rsc_param[1].rsz_o_vsz,
			RSZ_EN_1 + RSZ_O_VSZ);
		regw_ip(params->rsz_rsc_param[1].rsz_o_hsz,
			RSZ_EN_1 + RSZ_O_HSZ);
		regw_ip(params->rsz_rsc_param[1].rsz_o_hst,
			RSZ_EN_1 + RSZ_O_HST);
		regw_ip(params->rsz_rsc_param[1].rsz_v_phs,
			RSZ_EN_1 + RSZ_V_PHS);
		regw_ip(params->rsz_rsc_param[1].rsz_v_dif,
			RSZ_EN_1 + RSZ_V_DIF);
		regw_ip(params->rsz_rsc_param[1].rsz_h_phs,
			RSZ_EN_1 + RSZ_H_PHS);
		regw_ip(params->rsz_rsc_param[1].rsz_h_dif,
			RSZ_EN_1 + RSZ_H_DIF);
		regw_ip(params->rsz_rsc_param[1].rsz_h_typ,
			RSZ_EN_1 + RSZ_H_TYP);
		regw_ip(params->rsz_rsc_param[1].rsz_h_lse_sel,
			RSZ_EN_1 + RSZ_H_LSE);
		regw_ip(params->rsz_rsc_param[1].rsz_h_lpf,
			RSZ_EN_1 + RSZ_H_LPF);

		/*seting rgb conversion parameters */
		regw_ip(params->rsz2rgb[1].rsz_rgb_en, RSZ_EN_1 + RSZ_RGB_EN);
		regw_ip(params->rsz2rgb[1].rsz_rgb_en, RSZ_EN_1 + RSZ_RGB_EN);
		utemp =
		    ((params->rsz2rgb[1].rsz_rgb_typ << 0) |
		     (params->rsz2rgb[1].rsz_rgb_msk0 << 1) |
		     (params->rsz2rgb[1].rsz_rgb_msk1) << 2);
		regw_ip(utemp, RSZ_RGB_TYP);
		regw_ip(params->rsz2rgb[1].rsz_rgb_alpha_val,
			RSZ_EN_1 + RSZ_RGB_BLD);

		/*setting external memory parameters */
		regw_ip(params->ext_mem_param[1].rsz_sdr_oft,
			RSZ_EN_1 + RSZ_SDR_OFT);
		regw_ip(params->ext_mem_param[1].rsz_sdr_ptr_s,
			RSZ_EN_1 + RSZ_SDR_PTR_S);
		regw_ip(params->ext_mem_param[1].rsz_sdr_ptr_e,
			RSZ_EN_1 + RSZ_SDR_PTR_E);
	}

	if (!params->rsz_en[0] && !params->rsz_en[1]) {	/*resizer bypass mode */
		rsz_tmm = 0;
		rsz_seq = 0;
		utemp =
		    (params->rsz_seq_seq << 0) |
		    (params->rsz_seq_tmm << 1) |
		    (params->rsz_seq_hrv << 2) |
		    (params->rsz_seq_vrv << 3) | (params->rsz_seq_crv << 4);
		regw_ip(0, RSZ_AAL);
		regw_ip(0, RSZ_EN_0 + RSZ_O_HST);
		regw_ip(0, RSZ_EN_0 + RSZ_V_PHS);
		regw_ip(256, RSZ_EN_0 + RSZ_V_DIF);
		regw_ip(256, RSZ_EN_0 + RSZ_H_DIF);
		regw_ip(0, RSZ_EN_0 + RSZ_H_LSE);
		regw_ip(0, RSZ_EN_0 + RSZ_H_PHS);
		regw_ip(0, RSZ_EN_1);
		/*disable resizer clock, necessary to bypass resizer */
		regw_ip(0, GCL_SDR);
	}

	utemp = regr_ip((RSZ_EN_0 + RSZ_SDR_BAD_H));
	utemp = regr_ip((RSZ_EN_0 + RSZ_SDR_SAD_H));
	return 0;
}
