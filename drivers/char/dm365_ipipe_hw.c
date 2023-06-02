/*
 * Copyright (C) 2008-2009 Texas Instruments Inc
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
#include <media/davinci/dm365_ipipe.h>
#include <media/davinci/dm3xx_ipipe.h>
#include "dm365_ipipe_hw.h"

//#define CONFIG_IMP_DEBUG
static void ipipe_clock_enable(void)
{
	/* enable IPIPE MMR for register write access */
	regw_ip(IPIPE_GCK_MMR_DEFAULT, IPIPE_GCK_MMR);
	/* enable the clock wb,cfa,dfc,d2f,pre modules */
	regw_ip(IPIPE_GCK_PIX_DEFAULT, IPIPE_GCK_PIX);
	/* enable RSZ MMR for register write access */
}


#ifdef CONFIG_IMP_DEBUG
void ipipe_hw_dump_config(void)
{
	u32 utemp;

	ipipeif_dump_register();
	utemp = regr_ip(IPIPE_SRC_EN);
	printk(KERN_NOTICE "IPIPE_SRC_EN = 0x%x\n", utemp);
	utemp = regr_ip(IPIPE_SRC_MODE);
	printk(KERN_NOTICE "IPIPE_SRC_MODE = 0x%x\n", utemp);
	utemp = regr_ip(IPIPE_SRC_FMT);
	printk(KERN_NOTICE "IPIPE_SRC_FMT = 0x%x\n", utemp);
	utemp = regr_ip(IPIPE_SRC_COL);
	printk(KERN_NOTICE "IPIPE_SRC_COL = 0x%x\n", utemp);
	utemp = regr_ip(IPIPE_SRC_VPS);
	printk(KERN_NOTICE "IPIPE_SRC_VPS = 0x%x\n", utemp);
	utemp = regr_ip(IPIPE_SRC_VSZ);
	printk(KERN_NOTICE "IPIPE_SRC_VSZ = 0x%x\n", utemp);
	utemp = regr_ip(IPIPE_SRC_HPS);
	printk(KERN_NOTICE "IPIPE_SRC_HPS = 0x%x\n", utemp);
	utemp = regr_ip(IPIPE_SRC_HSZ);
	printk(KERN_NOTICE "IPIPE_SRC_HSZ = 0x%x\n", utemp);
	utemp = regr_ip(IPIPE_SEL_SBU);
	printk(KERN_NOTICE "IPIPE_SEL_SBU = 0x%x\n", utemp);
	utemp = regr_ip(IPIPE_DMA_STA);
	printk(KERN_NOTICE "IPIPE_DMA_STA = 0x%x\n", utemp);
	utemp = regr_ip(IPIPE_GCK_MMR);
	printk(KERN_NOTICE "IPIPE_GCK_MMR = 0x%x\n", utemp);
	utemp = regr_ip(IPIPE_GCK_PIX);
	printk(KERN_NOTICE "IPIPE_GCK_PIX = 0x%x\n", utemp);
	printk(KERN_NOTICE "Resizer Registers\n");
	printk(KERN_NOTICE
	       "**************************************************\n");
	utemp = regr_rsz(RSZ_SRC_EN);
	printk(KERN_NOTICE "RSZ_SRC_EN = 0x%x\n", utemp);
	utemp = regr_rsz(RSZ_SRC_MODE);
	printk(KERN_NOTICE "RSZ_SRC_MODE = 0x%x\n", utemp);
	utemp = regr_rsz(RSZ_SRC_FMT0);
	printk(KERN_NOTICE "RSZ_SRC_FMT0 = 0x%x\n", utemp);
	utemp = regr_rsz(RSZ_SRC_FMT1);
	printk(KERN_NOTICE "RSZ_SRC_FMT1 = 0x%x\n", utemp);
	utemp = regr_rsz(RSZ_SRC_VPS);
	printk(KERN_NOTICE "RSZ_SRC_VPS = 0x%x\n", utemp);
	utemp = regr_rsz(RSZ_SRC_VSZ);
	printk(KERN_NOTICE "RSZ_SRC_VSZ = 0x%x\n", utemp);
	utemp = regr_rsz(RSZ_SRC_HPS);
	printk(KERN_NOTICE "RSZ_SRC_HPS = 0x%x\n", utemp);
	utemp = regr_rsz(RSZ_SRC_HSZ);
	printk(KERN_NOTICE "RSZ_SRC_HSZ = 0x%x\n", utemp);
	utemp = regr_rsz(RSZ_DMA_RZA);
	printk(KERN_NOTICE "RSZ_DMA_RZA = 0x%x\n", utemp);
	utemp = regr_rsz(RSZ_DMA_RZB);
	printk(KERN_NOTICE "RSZ_DMA_RZB = 0x%x\n", utemp);
	utemp = regr_rsz(RSZ_DMA_STA);
	printk(KERN_NOTICE "RSZ_DMA_STA = 0x%x\n", utemp);
	utemp = regr_rsz(RSZ_GCK_MMR);
	printk(KERN_NOTICE "RSZ_GCK_MMR = 0x%x\n", utemp);
	utemp = regr_rsz(RSZ_GCK_SDR);
	printk(KERN_NOTICE "RSZ_GCK_SDR = 0x%x\n", utemp);
	utemp = regr_rsz(RSZ_IRQ_RZA);
	printk(KERN_NOTICE "RSZ_IRQ_RZA = 0x%x\n", utemp);
	utemp = regr_rsz(RSZ_IRQ_RZB);
	printk(KERN_NOTICE "RSZ_IRQ_RZB = 0x%x\n", utemp);
	utemp = regr_rsz(RSZ_YUV_Y_MIN);
	printk(KERN_NOTICE "RSZ_YUV_Y_MIN = 0x%x\n", utemp);
	utemp = regr_rsz(RSZ_YUV_Y_MAX);
	printk(KERN_NOTICE "RSZ_YUV_Y_MAX = 0x%x\n", utemp);
	utemp = regr_rsz(RSZ_YUV_C_MIN);
	printk(KERN_NOTICE "RSZ_YUV_C_MIN = 0x%x\n", utemp);
	utemp = regr_rsz(RSZ_YUV_C_MAX);
	printk(KERN_NOTICE "RSZ_YUV_C_MAX = 0x%x\n", utemp);
	utemp = regr_rsz(RSZ_YUV_PHS);
	printk(KERN_NOTICE "RSZ_YUV_PHS = 0x%x\n", utemp);
	utemp = regr_rsz(RSZ_SEQ);
	printk(KERN_NOTICE "RSZ_SEQ = 0x%x\n", utemp);

	/* Resize - A registers */
	utemp = regr_rsz((RSZ_EN_A));
	printk(KERN_NOTICE "RSZ A EN = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_A + RSZ_MODE));
	printk(KERN_NOTICE "RSZ A MODE = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_A + RSZ_420));
	printk(KERN_NOTICE "RSZ A 420 = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_A + RSZ_I_VPS));
	printk(KERN_NOTICE "RSZ A I_VPS = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_A + RSZ_I_HPS));
	printk(KERN_NOTICE "RSZ A I_HPS = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_A + RSZ_O_VSZ));
	printk(KERN_NOTICE "RSZ A O_VSZ = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_A + RSZ_O_HSZ));
	printk(KERN_NOTICE "RSZ A O_HSZ = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_A + RSZ_V_PHS_Y));
	printk(KERN_NOTICE "RSZ A V_PHS_Y = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_A + RSZ_V_PHS_C));
	printk(KERN_NOTICE "RSZ A V_PHS_C = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_A + RSZ_V_DIF));
	printk(KERN_NOTICE "RSZ A V_DIF = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_A + RSZ_V_TYP));
	printk(KERN_NOTICE "RSZ A V_TYP = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_A + RSZ_V_LPF));
	printk(KERN_NOTICE "RSZ A V_LPF = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_A + RSZ_H_PHS));
	printk(KERN_NOTICE "RSZ A H_PHS = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_A + RSZ_H_PHS_ADJ));
	printk(KERN_NOTICE "RSZ A H_PHS_ADJ = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_A + RSZ_H_DIF));
	printk(KERN_NOTICE "RSZ A H_DIF = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_A + RSZ_H_TYP));
	printk(KERN_NOTICE "RSZ A H_TYP = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_A + RSZ_H_LPF));
	printk(KERN_NOTICE "RSZ A H_LPF = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_A + RSZ_DWN_EN));
	printk(KERN_NOTICE "RSZ A DWN_EN = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_A + RSZ_DWN_AV));
	printk(KERN_NOTICE "RSZ A DWN_AV = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_A + RSZ_RGB_EN));
	printk(KERN_NOTICE "RSZ A RGB_EN = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_A + RSZ_RGB_TYP));
	printk(KERN_NOTICE "RSZ A RGB_TYP = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_A + RSZ_RGB_BLD));
	printk(KERN_NOTICE "RSZ A RGB_BLD = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_A + RSZ_SDR_Y_BAD_H));
	printk(KERN_NOTICE "RSZ A SDR_Y_BAD_H = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_A + RSZ_SDR_Y_BAD_L));
	printk(KERN_NOTICE "RSZ A SDR_Y_BAD_L = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_A + RSZ_SDR_Y_SAD_H));
	printk(KERN_NOTICE "RSZ A SDR_Y_SAD_H = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_A + RSZ_SDR_Y_SAD_L));
	printk(KERN_NOTICE "RSZ A SDR_Y_SAD_L = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_A + RSZ_SDR_Y_OFT));
	printk(KERN_NOTICE "RSZ A SDR_Y_OFT = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_A + RSZ_SDR_Y_PTR_S));
	printk(KERN_NOTICE "RSZ A SDR_Y_Y_PTR_S = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_A + RSZ_SDR_Y_PTR_E));
	printk(KERN_NOTICE "RSZ A SDR_Y_PTR_E = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_A + RSZ_SDR_C_BAD_H));
	printk(KERN_NOTICE "RSZ A SDR_C_BAD_H = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_A + RSZ_SDR_C_BAD_L));
	printk(KERN_NOTICE "RSZ A SDR_C_BAD_L = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_A + RSZ_SDR_C_SAD_H));
	printk(KERN_NOTICE "RSZ A SDR_C_SAD_H = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_A + RSZ_SDR_C_SAD_L));
	printk(KERN_NOTICE "RSZ A SDR_C_SAD_L = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_A + RSZ_SDR_C_OFT));
	printk(KERN_NOTICE "RSZ A SDR_C_OFT = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_A + RSZ_SDR_C_PTR_S));
	printk(KERN_NOTICE "RSZ A SDR_C_PTR_S = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_A + RSZ_SDR_C_PTR_E));
	printk(KERN_NOTICE "RSZ A SDR_C_PTR_E = 0x%x\n", utemp);

	/* Resize - B registers */
	utemp = regr_rsz((RSZ_EN_B));
	printk(KERN_NOTICE "RSZ B EN = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_B + RSZ_MODE));
	printk(KERN_NOTICE "RSZ B MODE = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_B + RSZ_420));
	printk(KERN_NOTICE "RSZ B 420 = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_B + RSZ_I_VPS));
	printk(KERN_NOTICE "RSZ B I_VPS = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_B + RSZ_I_HPS));
	printk(KERN_NOTICE "RSZ B I_HPS = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_B + RSZ_O_VSZ));
	printk(KERN_NOTICE "RSZ B O_VSZ = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_B + RSZ_O_HSZ));
	printk(KERN_NOTICE "RSZ B O_HSZ = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_B + RSZ_V_PHS_Y));
	printk(KERN_NOTICE "RSZ B V_PHS_Y = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_B + RSZ_V_PHS_C));
	printk(KERN_NOTICE "RSZ B V_PHS_C = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_B + RSZ_V_DIF));
	printk(KERN_NOTICE "RSZ B V_DIF = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_B + RSZ_V_TYP));
	printk(KERN_NOTICE "RSZ B V_TYP = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_B + RSZ_V_LPF));
	printk(KERN_NOTICE "RSZ B V_LPF = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_B + RSZ_H_PHS));
	printk(KERN_NOTICE "RSZ B H_PHS = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_B + RSZ_H_PHS_ADJ));
	printk(KERN_NOTICE "RSZ B H_PHS_ADJ = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_B + RSZ_H_DIF));
	printk(KERN_NOTICE "RSZ B H_DIF = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_B + RSZ_H_TYP));
	printk(KERN_NOTICE "RSZ B H_TYP = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_B + RSZ_H_LPF));
	printk(KERN_NOTICE "RSZ B H_LPF = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_B + RSZ_DWN_EN));
	printk(KERN_NOTICE "RSZ B DWN_EN = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_B + RSZ_DWN_AV));
	printk(KERN_NOTICE "RSZ B DWN_AV = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_B + RSZ_RGB_EN));
	printk(KERN_NOTICE "RSZ B RGB_EN = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_B + RSZ_RGB_TYP));
	printk(KERN_NOTICE "RSZ B RGB_TYP = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_B + RSZ_RGB_BLD));
	printk(KERN_NOTICE "RSZ B RGB_BLD = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_B + RSZ_SDR_Y_BAD_H));
	printk(KERN_NOTICE "RSZ B SDR_Y_BAD_H = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_B + RSZ_SDR_Y_BAD_L));
	printk(KERN_NOTICE "RSZ B SDR_Y_BAD_L = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_B + RSZ_SDR_Y_SAD_H));
	printk(KERN_NOTICE "RSZ B SDR_Y_SAD_H = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_B + RSZ_SDR_Y_SAD_L));
	printk(KERN_NOTICE "RSZ B SDR_Y_SAD_L = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_B + RSZ_SDR_Y_OFT));
	printk(KERN_NOTICE "RSZ B SDR_Y_OFT = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_B + RSZ_SDR_Y_PTR_S));
	printk(KERN_NOTICE "RSZ B SDR_Y_Y_PTR_S = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_B + RSZ_SDR_Y_PTR_E));
	printk(KERN_NOTICE "RSZ B SDR_Y_PTR_E = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_B + RSZ_SDR_C_BAD_H));
	printk(KERN_NOTICE "RSZ b SDR_C_BAD_H = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_B + RSZ_SDR_C_BAD_L));
	printk(KERN_NOTICE "RSZ B SDR_C_BAD_L = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_B + RSZ_SDR_C_SAD_H));
	printk(KERN_NOTICE "RSZ B SDR_C_SAD_H = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_B + RSZ_SDR_C_SAD_L));
	printk(KERN_NOTICE "RSZ B SDR_C_SAD_L = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_B + RSZ_SDR_C_OFT));
	printk(KERN_NOTICE "RSZ B SDR_C_OFT = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_B + RSZ_SDR_C_PTR_S));
	printk(KERN_NOTICE "RSZ B SDR_C_PTR_S = 0x%x\n", utemp);
	utemp = regr_rsz((RSZ_EN_B + RSZ_SDR_C_PTR_E));
	printk(KERN_NOTICE "RSZ B SDR_C_PTR_E = 0x%x\n", utemp);
}
#else
void ipipe_hw_dump_config(void)
{
}
#endif

/* Set input channel format to either 420 Y or C format */
int rsz_set_in_pix_format(unsigned char y_c)
{
	u32 utemp;
	utemp = regr_rsz(RSZ_SRC_FMT1);
	utemp |= (y_c & 1);
	regw_rsz(utemp, RSZ_SRC_FMT1);
	return 0;
}

static int rsz_set_common_params(struct ipipe_params *params)
{
	u32 utemp;
	struct rsz_common_params *rsz_common = &params->rsz_common;

	/* Set mode */
	regw_rsz(params->ipipe_mode, RSZ_SRC_MODE);

	/* data source selection  and bypass */
	utemp = ((rsz_common->passthrough << RSZ_BYPASS_SHIFT) |
		(rsz_common->source));

	regw_rsz(utemp, RSZ_SRC_FMT0);
	utemp = regr_rsz(RSZ_SRC_MODE);
	/* src image selection */
	utemp = (rsz_common->raw_flip & 1) |
		(rsz_common->src_img_fmt << RSZ_SRC_IMG_FMT_SHIFT) |
		((rsz_common->y_c & 1) << RSZ_SRC_Y_C_SEL_SHIFT);

	regw_rsz(utemp, RSZ_SRC_FMT1);
	regw_rsz(rsz_common->vps & IPIPE_RSZ_VPS_MASK, RSZ_SRC_VPS);
	regw_rsz(rsz_common->hps & IPIPE_RSZ_HPS_MASK, RSZ_SRC_HPS);
	regw_rsz(rsz_common->vsz & IPIPE_RSZ_VSZ_MASK, RSZ_SRC_VSZ);
	regw_rsz(rsz_common->hsz & IPIPE_RSZ_HSZ_MASK, RSZ_SRC_HSZ);
	regw_rsz(rsz_common->yuv_y_min, RSZ_YUV_Y_MIN);
	regw_rsz(rsz_common->yuv_y_max, RSZ_YUV_Y_MAX);
	regw_rsz(rsz_common->yuv_c_min, RSZ_YUV_C_MIN);
	regw_rsz(rsz_common->yuv_c_max, RSZ_YUV_C_MAX);
	/* chromatic position */
	regw_rsz(rsz_common->out_chr_pos, RSZ_YUV_PHS);
	utemp = regr_rsz(RSZ_SRC_MODE);
	return 0;
}

static void rsz_set_rsz_regs(unsigned int rsz_id, struct ipipe_params *params)
{
	struct ipipe_rsz_rescale_param *rsc_params;
	struct ipipe_rsz_resize2rgb *rgb;
	struct ipipe_ext_mem_param *ext_mem;
	u32 utemp, reg_base;

	utemp = regr_rsz(RSZ_SEQ);
	if (rsz_id == RSZ_A) {
		rsc_params = &params->rsz_rsc_param[RSZ_A];
		rgb = &params->rsz2rgb[RSZ_A];
		ext_mem = &params->ext_mem_param[RSZ_A];
		utemp = rsc_params->h_flip << RSZA_H_FLIP_SHIFT;
		utemp |= rsc_params->v_flip << RSZA_V_FLIP_SHIFT;
		reg_base = RSZ_EN_A;
	} else {
		rsc_params = &params->rsz_rsc_param[RSZ_B];
		rgb = &params->rsz2rgb[RSZ_B];
		ext_mem = &params->ext_mem_param[RSZ_B];
		utemp = rsc_params->h_flip << RSZB_H_FLIP_SHIFT;
		utemp |= rsc_params->v_flip << RSZB_V_FLIP_SHIFT;
		reg_base = RSZ_EN_B;
	}
	/* update flip settings */
	regw_rsz(utemp, RSZ_SEQ);

	regw_rsz(rsc_params->mode, reg_base + RSZ_MODE);
	utemp = (rsc_params->cen << RSZ_CEN_SHIFT) | rsc_params->yen;
	regw_rsz(utemp, reg_base + RSZ_420);
	regw_rsz(rsc_params->i_vps & RSZ_VPS_MASK, reg_base + RSZ_I_VPS);
	regw_rsz(rsc_params->i_hps & RSZ_HPS_MASK, reg_base + RSZ_I_HPS);
	regw_rsz(rsc_params->o_vsz & RSZ_O_VSZ_MASK, reg_base + RSZ_O_VSZ);
	regw_rsz(rsc_params->o_hsz & RSZ_O_HSZ_MASK, reg_base + RSZ_O_HSZ);
	regw_rsz(rsc_params->v_phs_y & RSZ_V_PHS_MASK, reg_base + RSZ_V_PHS_Y);
	regw_rsz(rsc_params->v_phs_c & RSZ_V_PHS_MASK, reg_base + RSZ_V_PHS_C);
	/* keep this additional adjustment to zero for now */
	regw_rsz(rsc_params->v_dif & RSZ_V_DIF_MASK, reg_base + RSZ_V_DIF);

	utemp = (rsc_params->v_typ_y & 1)
		| ((rsc_params->v_typ_c & 1) << RSZ_TYP_C_SHIFT);
	regw_rsz(utemp, reg_base + RSZ_V_TYP);

	utemp = (rsc_params->v_lpf_int_y & RSZ_LPF_INT_MASK) |
		 ((rsc_params->v_lpf_int_c & RSZ_LPF_INT_MASK) <<
		 RSZ_LPF_INT_C_SHIFT);
	regw_rsz(utemp, reg_base + RSZ_V_LPF);

	regw_rsz(rsc_params->h_phs & RSZ_H_PHS_MASK, reg_base + RSZ_H_PHS);
	regw_rsz(0, reg_base + RSZ_H_PHS_ADJ);
	regw_rsz(rsc_params->h_dif & RSZ_H_DIF_MASK, reg_base + RSZ_H_DIF);
	utemp = (rsc_params->h_typ_y & 1) |
		((rsc_params->h_typ_c & 1) << RSZ_TYP_C_SHIFT);
	regw_rsz(utemp, reg_base + RSZ_H_TYP);
	utemp = (rsc_params->h_lpf_int_y & RSZ_LPF_INT_MASK) |
		 ((rsc_params->h_lpf_int_c & RSZ_LPF_INT_MASK) <<
		 RSZ_LPF_INT_C_SHIFT);
	regw_rsz(utemp, reg_base + RSZ_H_LPF);

	regw_rsz(rsc_params->dscale_en & 1, reg_base + RSZ_DWN_EN);
	utemp = rsc_params->h_dscale_ave_sz & RSZ_DWN_SCALE_AV_SZ_MASK;
	utemp |= ((rsc_params->v_dscale_ave_sz & RSZ_DWN_SCALE_AV_SZ_MASK) <<
		  RSZ_DWN_SCALE_AV_SZ_V_SHIFT);
	regw_rsz(utemp, reg_base + RSZ_DWN_AV);

	/* setting rgb conversion parameters */
	regw_rsz(rgb->rgb_en, reg_base + RSZ_RGB_EN);
	utemp = ((rgb->rgb_typ << RSZ_RGB_TYP_SHIFT) |
		 (rgb->rgb_msk0 << RSZ_RGB_MSK0_SHIFT) |
		 (rgb->rgb_msk1) << RSZ_RGB_MSK1_SHIFT);
	regw_rsz(utemp, reg_base + RSZ_RGB_TYP);
	regw_rsz(rgb->rgb_alpha_val & RSZ_RGB_ALPHA_MASK,
		reg_base + RSZ_RGB_BLD);

	/* setting external memory parameters */
	regw_rsz(ext_mem->rsz_sdr_oft_y, reg_base + RSZ_SDR_Y_OFT);
	regw_rsz(ext_mem->rsz_sdr_ptr_s_y, reg_base + RSZ_SDR_Y_PTR_S);
	regw_rsz(ext_mem->rsz_sdr_ptr_e_y, reg_base + RSZ_SDR_Y_PTR_E);
	regw_rsz(ext_mem->rsz_sdr_oft_c, reg_base + RSZ_SDR_C_OFT);
	regw_rsz(ext_mem->rsz_sdr_ptr_s_c, reg_base + RSZ_SDR_C_PTR_S);
#ifdef CONFIG_VIDEO_YCBCR
	regw_rsz(ext_mem->rsz_sdr_ptr_e_c, reg_base + RSZ_SDR_C_PTR_E);
#else
	regw_rsz((ext_mem->rsz_sdr_ptr_e_c >> 1), reg_base + RSZ_SDR_C_PTR_E);
#endif
}

static int ipipe_setup_resizer(struct ipipe_params *params)
{				/*set the registers of either RSZ0 or RSZ1 */
	u32 utemp;

	/* enable MMR gate to write to Resizer */
	regw_rsz(1, RSZ_GCK_MMR);

	/* Enable resizer if it is not in bypass mode */
	if (params->rsz_common.passthrough)
		regw_rsz(0, RSZ_GCK_SDR);
	else
		regw_rsz(1, RSZ_GCK_SDR);

	rsz_set_common_params(params);

	regw_rsz(params->rsz_en[0], RSZ_EN_A);
	if (params->rsz_en[0]) {
		printk(KERN_DEBUG
		       "ipipe_set_resizer, resizer - A enabled\n");
		/*setting rescale parameters */
		rsz_set_rsz_regs(RSZ_A, params);
	}

	regw_rsz(params->rsz_en[1], RSZ_EN_B);
	if (params->rsz_en[1]) {
		printk(KERN_DEBUG
		       "ipipe_set_resizer, resizer - B enabled\n");
		rsz_set_rsz_regs(RSZ_B, params);
	}
	utemp = regr_rsz(RSZ_SRC_MODE);
	return 0;
}

/* ipipe_hw_setup:It is used for Hardware Setup */
int ipipe_hw_setup(struct ipipe_params *config)
{
	u32 utemp = 0;
	u32 data_format;

	if (!config) {
		printk(KERN_ERR "NULL config block received\n");
		return -EINVAL;
	}

	if (ipipeif_hw_setup(&config->ipipeif_param) < 0) {
		printk(KERN_ERR "Unable to configure IPIPEIF");
		return -EINVAL;
	}

	/* enable clock to IPIPE */
	vpss_enable_clock(VPSS_IPIPE_CLOCK, 1);
	/* enable clock to MMR and modules before writting
	 * to ipipe registers
	 */
	ipipe_clock_enable();

	if (config->rsz_common.source == IPIPEIF_DATA) {
		/* we need to skip configuring IPIPE */
		regw_ip(0, IPIPE_SRC_EN);
	} else {
		/* enable ipipe mode to either one shot or continuous */
		utemp = config->ipipe_mode;
		regw_ip((utemp), IPIPE_SRC_MODE);
#ifdef CONFIG_VIDEO_YCBCR
		if(config->ipipe_dpaths_fmt != IPIPE_YUV2YUV)
#endif
		regw_ip(1, IPIPE_SRC_EN);
		data_format = config->ipipe_dpaths_fmt;
		regw_ip(data_format, IPIPE_SRC_FMT);
		/* set size */
		regw_ip(config->ipipe_vps & IPIPE_RSZ_VPS_MASK, IPIPE_SRC_VPS);
		regw_ip(config->ipipe_hps & IPIPE_RSZ_HPS_MASK, IPIPE_SRC_HPS);
		regw_ip(config->ipipe_vsz & IPIPE_RSZ_VSZ_MASK, IPIPE_SRC_VSZ);
		regw_ip(config->ipipe_hsz & IPIPE_RSZ_HSZ_MASK, IPIPE_SRC_HSZ);

		if ((data_format == IPIPE_RAW2YUV) ||
		    (data_format == IPIPE_RAW2RAW)) {
			/* Combine all the fields to make COLPAT register
			 * of IPIPE
			 */
			utemp = (config->ipipe_colpat_elep <<
				COLPAT_EE_SHIFT);
			utemp |= (config->ipipe_colpat_elop <<
				COLPAT_EO_SHIFT);
			utemp |= (config->ipipe_colpat_olep <<
				COLPAT_OE_SHIFT);
			utemp |= (config->ipipe_colpat_olop <<
				COLPAT_OO_SHIFT);
			regw_ip(utemp, IPIPE_SRC_COL);
		}
	}
	return ipipe_setup_resizer(config);
}
#ifdef CONFIG_VIDEO_YCBCR
int ipipe_hw_set_ipipeif_addr(struct ipipe_params *config, unsigned int address)
{
	if (!config) {
		printk(KERN_ERR "NULL config block received\n");
		return -EINVAL;
	}

	if (ipipeif_set_address(&config->ipipeif_param,address) < 0) {
		printk(KERN_ERR "Unable to set IPIPEIF sdram addr");
		return -EINVAL;
	}

	return 0;
}
#endif
static void rsz_set_y_address(unsigned int address, unsigned int offset)
{
	u32 utemp;
	utemp = (address & SET_LOW_ADD);
	regw_rsz(utemp, offset + RSZ_SDR_Y_BAD_L);
	regw_rsz(utemp, offset + RSZ_SDR_Y_SAD_L);
	utemp = (address & SET_HIGH_ADD) >> 16;
	regw_rsz(utemp, offset + RSZ_SDR_Y_BAD_H);
	regw_rsz(utemp, offset + RSZ_SDR_Y_SAD_H);
}

static void rsz_set_c_address(unsigned int address, unsigned int offset)
{
	u32 utemp;
	utemp = (address & SET_LOW_ADD);

	regw_rsz(utemp, offset + RSZ_SDR_C_BAD_L);
	regw_rsz(utemp, offset + RSZ_SDR_C_SAD_L);
	utemp = (address & SET_HIGH_ADD) >> 16;
	regw_rsz(utemp, offset + RSZ_SDR_C_BAD_H);
	regw_rsz(utemp, offset + RSZ_SDR_C_SAD_H);
}

/* Assume we get a valid params ptr and resize_no set to RSZ_A
 * or RSZ_B. This could be called in the interrupt context and
 * must be efficient
 */
int rsz_set_output_address(struct ipipe_params *params,
			   int resize_no, unsigned int address)
{
	unsigned int utemp;
	unsigned int rsz_start_add;
	struct ipipe_ext_mem_param *mem_param =
		&params->ext_mem_param[resize_no];
	struct rsz_common_params *rsz_common =
		&params->rsz_common;
	struct ipipe_rsz_rescale_param *rsc_param =
		&params->rsz_rsc_param[resize_no];

#ifdef CONFIG_V2R_DEBUG
	printk(KERN_DEBUG "rsz_set_output_address %d\n", resize_no);
#endif
	if (resize_no == RSZ_A)
		rsz_start_add = RSZ_EN_A;
	else
		rsz_start_add = RSZ_EN_B;
	/* y_c = 0 for y, = 1 for c */
	if (rsz_common->src_img_fmt == RSZ_IMG_420) {
		if (rsz_common->y_c) {
			/* C channel */
			utemp = (address + mem_param->flip_ofst_c);
			rsz_set_c_address(utemp, rsz_start_add);
		} else {
			utemp = (address + mem_param->flip_ofst_y);
			rsz_set_y_address(utemp, rsz_start_add);
		}
	} else {
		if (rsc_param->cen && rsc_param->yen) {
			/* 420 */
		utemp = (address + mem_param->c_offset);
			utemp = (address + mem_param->c_offset
				+ mem_param->flip_ofst_c);
			utemp += mem_param->user_y_ofst + mem_param->user_c_ofst;
			if (resize_no == RSZ_B)
				utemp += params->ext_mem_param[RSZ_A].user_y_ofst +
				params->ext_mem_param[RSZ_A].user_c_ofst;
			/* set C address */
			rsz_set_c_address(utemp, rsz_start_add);
		}
		utemp = (address + mem_param->flip_ofst_y);
		utemp += mem_param->user_y_ofst;
		if (resize_no == RSZ_B)
			utemp += params->ext_mem_param[RSZ_A].user_y_ofst +
				params->ext_mem_param[RSZ_A].user_c_ofst;
		/* set Y address */
		rsz_set_y_address(utemp, rsz_start_add);
	}
	/* resizer must be enabled */
	regw_rsz(params->rsz_en[resize_no], rsz_start_add);
	return 0;
}

int ipipe_set_lutdpc_regs(struct prev_lutdpc *dpc)
{
	u32 utemp, count, lut_start_addr = DPC_TB0_START_ADDR,
		max_tbl_size = (LUT_DPC_MAX_SIZE >> 1);

	ipipe_clock_enable();
	regw_ip(dpc->en, DPC_LUT_EN);
	if (1 == dpc->en) {
		utemp = LUTDPC_TBL_256_EN;
		utemp |= (dpc->repl_white & 1);
		regw_ip(utemp, DPC_LUT_SEL);

		regw_ip(LUT_DPC_START_ADDR, DPC_LUT_ADR);
		regw_ip(dpc->dpc_size, DPC_LUT_SIZ & LUT_DPC_SIZE_MASK);
		if (dpc->table != NULL) {
			count = 0;
			while (count < dpc->dpc_size) {
				if (count >= max_tbl_size)
					lut_start_addr = DPC_TB1_START_ADDR;
				utemp =
				    (dpc->table[count].horz_pos
					& LUT_DPC_H_POS_MASK);
				utemp |= ((dpc->table[count].vert_pos
					 & LUT_DPC_V_POS_MASK)
					 << LUT_DPC_V_POS_SHIFT);
				utemp |= (dpc->table[count].method
					 << LUT_DPC_CORR_METH_SHIFT);
				w_ip_table(utemp, (lut_start_addr +
					((count % max_tbl_size) << 2)));
				count++;
			}
		}

	}
	return 0;
}

static void set_dpc_thresholds(struct prev_otfdpc_2_0 *dpc_thr)
{
	regw_ip((dpc_thr->corr_thr.r & OTFDPC_DPC2_THR_MASK),
		DPC_OTF_2C_THR_R);
	regw_ip((dpc_thr->corr_thr.gr & OTFDPC_DPC2_THR_MASK),
		DPC_OTF_2C_THR_GR);
	regw_ip((dpc_thr->corr_thr.gb & OTFDPC_DPC2_THR_MASK),
		DPC_OTF_2C_THR_GB);
	regw_ip((dpc_thr->corr_thr.b & OTFDPC_DPC2_THR_MASK),
		DPC_OTF_2C_THR_B);
	regw_ip((dpc_thr->det_thr.r & OTFDPC_DPC2_THR_MASK),
		DPC_OTF_2D_THR_R);
	regw_ip((dpc_thr->det_thr.gr & OTFDPC_DPC2_THR_MASK),
		DPC_OTF_2D_THR_GR);
	regw_ip((dpc_thr->det_thr.gb & OTFDPC_DPC2_THR_MASK),
		DPC_OTF_2D_THR_GB);
	regw_ip((dpc_thr->det_thr.b & OTFDPC_DPC2_THR_MASK),
		DPC_OTF_2D_THR_B);
}

int ipipe_set_otfdpc_regs(struct prev_otfdpc *otfdpc)
{
	u32 utemp;
	struct prev_otfdpc_2_0 *dpc_2_0 = &otfdpc->alg_cfg.dpc_2_0;
	struct prev_otfdpc_3_0 *dpc_3_0 = &otfdpc->alg_cfg.dpc_3_0;

	ipipe_clock_enable();

	regw_ip((otfdpc->en & 1), DPC_OTF_EN);
	if (1 == otfdpc->en) {
		utemp = (otfdpc->det_method << OTF_DET_METHOD_SHIFT);
		utemp |= otfdpc->alg;
		regw_ip(utemp, DPC_OTF_TYP);
		if (otfdpc->det_method == IPIPE_DPC_OTF_MIN_MAX) {
			/* ALG= 0, TYP = 0, DPC_OTF_2D_THR_[x]=0
			 * DPC_OTF_2C_THR_[x] = Maximum thresohld
			 * MinMax method
			 */
			dpc_2_0->det_thr.r = dpc_2_0->det_thr.gb =
			dpc_2_0->det_thr.gr = dpc_2_0->det_thr.b = 0;
			set_dpc_thresholds(dpc_2_0);
		} else {
			/* MinMax2 */
			if (otfdpc->alg == IPIPE_OTFDPC_2_0)
				set_dpc_thresholds(dpc_2_0);
			else {
				regw_ip((dpc_3_0->act_adj_shf
					& OTF_DPC3_0_SHF_MASK), DPC_OTF_3_SHF);
				/* Detection thresholds */
				regw_ip(((dpc_3_0->det_thr
					& OTF_DPC3_0_THR_MASK) <<
					OTF_DPC3_0_THR_SHIFT), DPC_OTF_3D_THR);
				regw_ip((dpc_3_0->det_slp
					& OTF_DPC3_0_SLP_MASK),
					DPC_OTF_3D_SLP);
				regw_ip((dpc_3_0->det_thr_min
					& OTF_DPC3_0_DET_MASK),
					DPC_OTF_3D_MIN);
				regw_ip((dpc_3_0->det_thr_max
					& OTF_DPC3_0_DET_MASK),
					DPC_OTF_3D_MAX);
				/* Correction thresholds */
				regw_ip(((dpc_3_0->corr_thr
					& OTF_DPC3_0_THR_MASK) <<
					OTF_DPC3_0_THR_SHIFT), DPC_OTF_3C_THR);
				regw_ip((dpc_3_0->corr_slp
					& OTF_DPC3_0_SLP_MASK),
					DPC_OTF_3C_SLP);
				regw_ip((dpc_3_0->corr_thr_min
					& OTF_DPC3_0_CORR_MASK),
					DPC_OTF_3C_MIN);
				regw_ip((dpc_3_0->corr_thr_max
					& OTF_DPC3_0_CORR_MASK),
					DPC_OTF_3C_MAX);
			}
		}
	}
	return 0;
}

/* 2D Noise filter */
int ipipe_set_d2f_regs(unsigned int id, struct prev_nf *noise_filter)
{
	u32 utemp;
	int count = 0;
	u32 offset = D2F_1ST;

	/* id = 0 , NF1 & id = 1, NF 2 */
	if (id)
		offset = D2F_2ND;
	ipipe_clock_enable();
	regw_ip(noise_filter->en & 1, offset + D2F_EN);
	if (1 == noise_filter->en) {
		/* Combine all the fields to make D2F_CFG register of IPIPE */
		utemp = ((noise_filter->spread_val & D2F_SPR_VAL_MASK) <<
			 D2F_SPR_VAL_SHIFT) |
			 ((noise_filter->shft_val & D2F_SHFT_VAL_MASK) <<
			 D2F_SHFT_VAL_SHIFT) |
			 (noise_filter->gr_sample_meth <<
			 D2F_SAMPLE_METH_SHIFT) |
			 ((noise_filter->apply_lsc_gain & 1) <<
			 D2F_APPLY_LSC_GAIN_SHIFT) | D2F_USE_SPR_REG_VAL;

		regw_ip(utemp, offset + D2F_TYP);
		/* edge detection minimum */
		regw_ip(noise_filter->edge_det_min_thr & D2F_EDGE_DET_THR_MASK,
			offset + D2F_EDG_MIN);
		/* edge detection maximum */
		regw_ip(noise_filter->edge_det_max_thr & D2F_EDGE_DET_THR_MASK,
			offset + D2F_EDG_MAX);
		count = 0;
		while (count < IPIPE_NF_STR_TABLE_SIZE) {
			regw_ip((noise_filter->str[count] & D2F_STR_VAL_MASK),
				offset + D2F_STR + count * 4);
			count++;
		}
		count = 0;
		while (count < IPIPE_NF_THR_TABLE_SIZE) {
			regw_ip(noise_filter->thr[count] & D2F_THR_VAL_MASK,
				offset + D2F_THR + count * 4);
			count++;
		}
	}
	return 0;
}

#define IPIPE_U8Q5(decimal, integer) \
	(((decimal & 0x1f) | ((integer & 0x7) << 5)))

/* Green Imbalance Correction */
int ipipe_set_gic_regs(struct prev_gic *gic)
{
	u32 utemp;
	ipipe_clock_enable();
	regw_ip(gic->en & 1, GIC_EN);
	if (gic->en) {
		utemp = gic->wt_fn_type << GIC_TYP_SHIFT;
		utemp |= (gic->thr_sel << GIC_THR_SEL_SHIFT);
		utemp |= ((gic->apply_lsc_gain & 1) <<
				GIC_APPLY_LSC_GAIN_SHIFT);
		regw_ip(utemp, GIC_TYP);
		regw_ip(gic->gain & GIC_GAIN_MASK, GIC_GAN);
		if (gic->gic_alg == IPIPE_GIC_ALG_ADAPT_GAIN) {
			if (gic->thr_sel == IPIPE_GIC_THR_REG) {
				regw_ip(gic->thr & GIC_THR_MASK, GIC_THR);
				regw_ip(gic->slope & GIC_SLOPE_MASK, GIC_SLP);
			} else {
				/* Use NF thresholds */
				utemp = IPIPE_U8Q5(gic->nf2_thr_gain.decimal, \
						gic->nf2_thr_gain.integer);
				regw_ip(utemp, GIC_NFGAN);
			}
		} else
			/* Constant Gain. Set threshold to maximum */
			regw_ip(GIC_THR_MASK, GIC_THR);
	}
	return 0;
}

#define IPIPE_U13Q9(decimal, integer) \
	(((decimal & 0x1ff) | ((integer & 0xf) << 9)))
/* White balance */
int ipipe_set_wb_regs(struct prev_wb *wb)
{
	u32 utemp;

	ipipe_clock_enable();
	/* Ofsets. S12 */
	regw_ip(wb->ofst_r & WB_OFFSET_MASK, WB2_OFT_R);
	regw_ip(wb->ofst_gr & WB_OFFSET_MASK, WB2_OFT_GR);
	regw_ip(wb->ofst_gb & WB_OFFSET_MASK, WB2_OFT_GB);
	regw_ip(wb->ofst_b & WB_OFFSET_MASK, WB2_OFT_B);

	/* Gains. U13Q9 */
	utemp = IPIPE_U13Q9((wb->gain_r.decimal), (wb->gain_r.integer));
	regw_ip(utemp, WB2_WGN_R);
	utemp = IPIPE_U13Q9((wb->gain_gr.decimal), (wb->gain_gr.integer));
	regw_ip(utemp, WB2_WGN_GR);
	utemp = IPIPE_U13Q9((wb->gain_gb.decimal), (wb->gain_gb.integer));
	regw_ip(utemp, WB2_WGN_GB);
	utemp = IPIPE_U13Q9((wb->gain_b.decimal), (wb->gain_b.integer));
	regw_ip(utemp, WB2_WGN_B);
	return 0;
}

/* CFA */
int ipipe_set_cfa_regs(struct prev_cfa *cfa)
{
	ipipe_clock_enable();
	regw_ip(cfa->alg, CFA_MODE);
	regw_ip(cfa->hpf_thr_2dir & CFA_HPF_THR_2DIR_MASK, CFA_2DIR_HPF_THR);
	regw_ip(cfa->hpf_slp_2dir & CFA_HPF_SLOPE_2DIR_MASK, CFA_2DIR_HPF_SLP);
	regw_ip(cfa->hp_mix_thr_2dir & CFA_HPF_MIX_THR_2DIR_MASK,
			CFA_2DIR_MIX_THR);
	regw_ip(cfa->hp_mix_slope_2dir & CFA_HPF_MIX_SLP_2DIR_MASK,
			CFA_2DIR_MIX_SLP);
	regw_ip(cfa->dir_thr_2dir & CFA_DIR_THR_2DIR_MASK, CFA_2DIR_DIR_THR);
	regw_ip(cfa->dir_slope_2dir & CFA_DIR_SLP_2DIR_MASK, CFA_2DIR_DIR_SLP);
	regw_ip(cfa->nd_wt_2dir & CFA_ND_WT_2DIR_MASK, CFA_2DIR_NDWT);
	regw_ip(cfa->hue_fract_daa & CFA_DAA_HUE_FRA_MASK, CFA_MONO_HUE_FRA);
	regw_ip(cfa->edge_thr_daa & CFA_DAA_EDG_THR_MASK, CFA_MONO_EDG_THR);
	regw_ip(cfa->thr_min_daa & CFA_DAA_THR_MIN_MASK, CFA_MONO_THR_MIN);
	regw_ip(cfa->thr_slope_daa & CFA_DAA_THR_SLP_MASK, CFA_MONO_THR_SLP);
	regw_ip(cfa->slope_min_daa & CFA_DAA_SLP_MIN_MASK, CFA_MONO_SLP_MIN);
	regw_ip(cfa->slope_slope_daa & CFA_DAA_SLP_SLP_MASK, CFA_MONO_SLP_SLP);
	regw_ip(cfa->lp_wt_daa & CFA_DAA_LP_WT_MASK, CFA_MONO_LPWT);
	return 0;
}

int ipipe_set_rgb2rgb_regs(unsigned int id, struct prev_rgb2rgb *rgb)
{
	u32 utemp, offset = RGB1_MUL_BASE, offset_mask = RGB2RGB_1_OFST_MASK,
		integ_mask = 0xf;

	ipipe_clock_enable();

	if (id) {
		/* For second RGB module, gain integer is 3 bits instead
		of 4, offset has 11 bits insread of 13 */
		offset = RGB2_MUL_BASE;
		integ_mask = 0x7;
		offset_mask = RGB2RGB_2_OFST_MASK;
	}
	/* Gains */
	utemp = ((rgb->coef_rr.decimal & 0xff) |
		(((rgb->coef_rr.integer) & integ_mask) << 8));
	regw_ip(utemp, offset + RGB_MUL_RR);
	utemp = ((rgb->coef_gr.decimal & 0xff) |
		(((rgb->coef_gr.integer) & integ_mask) << 8));
	regw_ip(utemp, offset + RGB_MUL_GR);
	utemp = ((rgb->coef_br.decimal & 0xff) |
		(((rgb->coef_br.integer) & integ_mask) << 8));
	regw_ip(utemp, offset + RGB_MUL_BR);
	utemp = ((rgb->coef_rg.decimal & 0xff) |
		(((rgb->coef_rg.integer) & integ_mask) << 8));
	regw_ip(utemp, offset + RGB_MUL_RG);
	utemp = ((rgb->coef_gg.decimal & 0xff) |
		(((rgb->coef_gg.integer) & integ_mask) << 8));
	regw_ip(utemp, offset + RGB_MUL_GG);
	utemp = ((rgb->coef_bg.decimal & 0xff) |
		(((rgb->coef_bg.integer) & integ_mask) << 8));
	regw_ip(utemp, offset + RGB_MUL_BG);
	utemp = ((rgb->coef_rb.decimal & 0xff) |
		(((rgb->coef_rb.integer) & integ_mask) << 8));
	regw_ip(utemp, offset + RGB_MUL_RB);
	utemp = ((rgb->coef_gb.decimal & 0xff) |
		(((rgb->coef_gb.integer) & integ_mask) << 8));
	regw_ip(utemp, offset + RGB_MUL_GB);
	utemp = ((rgb->coef_bb.decimal & 0xff) |
		(((rgb->coef_bb.integer) & integ_mask) << 8));
	regw_ip(utemp, offset + RGB_MUL_BB);

	/* Offsets */
	regw_ip(rgb->out_ofst_r & offset_mask, offset + RGB_OFT_OR);
	regw_ip(rgb->out_ofst_g & offset_mask, offset + RGB_OFT_OG);
	regw_ip(rgb->out_ofst_b & offset_mask, offset + RGB_OFT_OB);
	return 0;
}

static void ipipe_update_gamma_tbl(struct ipipe_gamma_entry *table,
				   int size, u32 addr)
{
	int count;
	u32 utemp;

	for (count = 0; count < size; count++) {
		utemp = table[count].slope & GAMMA_MASK;
		utemp |= ((table[count].offset & GAMMA_MASK) << GAMMA_SHIFT);
		w_ip_table(utemp, (addr + (count * 4)));
	}
}

/* Gamma correction */
int ipipe_set_gamma_regs(struct prev_gamma *gamma)
{
	u32 utemp;
	int table_size = 0;

	ipipe_clock_enable();
	utemp = ((gamma->bypass_r << GAMMA_BYPR_SHIFT)
		| (gamma->bypass_b << GAMMA_BYPG_SHIFT)
		| (gamma->bypass_g << GAMMA_BYPB_SHIFT)
		| (gamma->tbl_sel << GAMMA_TBL_SEL_SHIFT)
		| (gamma->tbl_size << GAMMA_TBL_SIZE_SHIFT));

	regw_ip(utemp, GMM_CFG);
	if (gamma->tbl_sel == IPIPE_GAMMA_TBL_RAM) {
		if (gamma->tbl_size == IPIPE_GAMMA_TBL_SZ_64)
			table_size = 64;
		else if (gamma->tbl_size == IPIPE_GAMMA_TBL_SZ_128)
			table_size = 128;
		else if (gamma->tbl_size == IPIPE_GAMMA_TBL_SZ_256)
			table_size = 256;
		else if (gamma->tbl_size == IPIPE_GAMMA_TBL_SZ_512)
			table_size = 512;
		if (!(gamma->bypass_r)) {
			if (gamma->table_r != NULL)
				ipipe_update_gamma_tbl(gamma->table_r,
						       table_size,
						       GAMMA_R_START_ADDR);
		}
		if (!(gamma->bypass_b)) {
			if (gamma->table_b != NULL)
				ipipe_update_gamma_tbl(gamma->table_b,
						       table_size,
						       GAMMA_B_START_ADDR);
		}
		if (!(gamma->bypass_g)) {
			if (gamma->table_g != NULL)
				ipipe_update_gamma_tbl(gamma->table_g,
						       table_size,
						       GAMMA_G_START_ADDR);
		}

	}
	return 0;
}

/* 3D LUT */
int ipipe_set_3d_lut_regs(struct prev_3d_lut *lut_3d)
{
	u32 utemp, i, bnk_index, tbl_index;
	struct ipipe_3d_lut_entry *tbl;

	ipipe_clock_enable();
	regw_ip(lut_3d->en, D3LUT_EN);
	if (lut_3d->en) {
		if (lut_3d->table) {
			tbl = lut_3d->table;
			for (i = 0 ; i < MAX_SIZE_3D_LUT; i++) {
				/* Each entry has 0-9 (B), 10-19 (G) and
				20-29 R values */
				utemp = (tbl[i].b & D3_LUT_ENTRY_MASK);
				utemp |= ((tbl[i].g & D3_LUT_ENTRY_MASK) <<
					 D3_LUT_ENTRY_G_SHIFT);
				utemp |= ((tbl[i].r & D3_LUT_ENTRY_MASK) <<
					 D3_LUT_ENTRY_R_SHIFT);
				bnk_index = (i % 4);
				tbl_index = (i >> 2);
				tbl_index <<= 2;
				if (bnk_index == 0)
					w_ip_table(utemp,
						tbl_index + D3L_TB0_START_ADDR);
				else if (bnk_index == 1)
					w_ip_table(utemp,
						tbl_index + D3L_TB1_START_ADDR);
				else if (bnk_index == 2)
					w_ip_table(utemp,
						tbl_index + D3L_TB2_START_ADDR);
				else
					w_ip_table(utemp,
						tbl_index + D3L_TB3_START_ADDR);
			}
		}
	}
	return 0;
}

/* Lumina adjustments */
int ipipe_set_lum_adj_regs(struct prev_lum_adj *lum_adj)
{
	u32 utemp;

	ipipe_clock_enable();
	/* combine fields of YUV_ADJ to set brightness and contrast */
	utemp = ((lum_adj->contrast << LUM_ADJ_CONTR_SHIFT)
		|(lum_adj->brightness << LUM_ADJ_BRIGHT_SHIFT));
	regw_ip(utemp, YUV_ADJ);
	return 0;
}

#define IPIPE_S12Q8(decimal, integer) \
	(((decimal & 0xff) | ((integer & 0xf) << 8)))
/* RGB2YUV */
int ipipe_set_rgb2ycbcr_regs(struct prev_rgb2yuv *yuv)
{
	u32 utemp;

	/* S10Q8 */
	ipipe_clock_enable();
	utemp = IPIPE_S12Q8((yuv->coef_ry.decimal), (yuv->coef_ry.integer));
	regw_ip(utemp, YUV_MUL_RY);
	utemp = IPIPE_S12Q8((yuv->coef_gy.decimal), (yuv->coef_gy.integer));
	regw_ip(utemp, YUV_MUL_GY);
	utemp = IPIPE_S12Q8((yuv->coef_by.decimal), (yuv->coef_by.integer));
	regw_ip(utemp, YUV_MUL_BY);
	utemp = IPIPE_S12Q8((yuv->coef_rcb.decimal), (yuv->coef_rcb.integer));
	regw_ip(utemp, YUV_MUL_RCB);
	utemp = IPIPE_S12Q8((yuv->coef_gcb.decimal), (yuv->coef_gcb.integer));
	regw_ip(utemp, YUV_MUL_GCB);
	utemp = IPIPE_S12Q8((yuv->coef_bcb.decimal), (yuv->coef_bcb.integer));
	regw_ip(utemp, YUV_MUL_BCB);
	utemp = IPIPE_S12Q8((yuv->coef_rcr.decimal), (yuv->coef_rcr.integer));
	regw_ip(utemp, YUV_MUL_RCR);
	utemp = IPIPE_S12Q8((yuv->coef_gcr.decimal), (yuv->coef_gcr.integer));
	regw_ip(utemp, YUV_MUL_GCR);
	utemp = IPIPE_S12Q8((yuv->coef_bcr.decimal), (yuv->coef_bcr.integer));
	regw_ip(utemp, YUV_MUL_BCR);
	regw_ip(yuv->out_ofst_y & RGB2YCBCR_OFST_MASK, YUV_OFT_Y);
	regw_ip(yuv->out_ofst_cb & RGB2YCBCR_OFST_MASK, YUV_OFT_CB);
	regw_ip(yuv->out_ofst_cr & RGB2YCBCR_OFST_MASK, YUV_OFT_CR);
	return 0;
}

/* YUV 422 conversion */
int ipipe_set_yuv422_conv_regs(struct prev_yuv422_conv *conv)
{
	u32 utemp;

	ipipe_clock_enable();
	/* Combine all the fields to make YUV_PHS register of IPIPE */
	utemp = ((conv->chrom_pos << 0) | (conv->en_chrom_lpf << 1));
	regw_ip(utemp, YUV_PHS);
	return 0;
}

/* GBCE */
int ipipe_set_gbce_regs(struct prev_gbce *gbce)
{
	unsigned int count, tbl_index;
	u32 utemp = 0, mask = GBCE_Y_VAL_MASK;

	if (gbce->type == IPIPE_GBCE_GAIN_TBL)
		mask = GBCE_GAIN_VAL_MASK;

	ipipe_clock_enable();
	regw_ip(gbce->en & 1, GBCE_EN);
	if (gbce->en) {
		regw_ip(gbce->type, GBCE_TYP);
		if (gbce->table) {
			for (count = 0; count < MAX_SIZE_GBCE_LUT; count++) {
				tbl_index = count >> 1;
				tbl_index <<= 2;
				/* Each table has 2 LUT entries, first in LS
				 * and second in MS positions
				 */
				if (count % 2) {
					utemp |=
						((gbce->table[count] & mask) <<
						GBCE_ENTRY_SHIFT);
					w_ip_table(utemp,
						tbl_index + GBCE_TB_START_ADDR);
				} else
					utemp = gbce->table[count] & mask;
			}
		}
	}
	return 0;
}
/* Edge Enhancement */
int ipipe_set_ee_regs(struct prev_yee *ee)
{
	unsigned int count, tbl_index;
	u32 utemp;

	ipipe_clock_enable();
	regw_ip(ee->en, YEE_EN);
	if (1 == ee->en) {
		utemp = ee->en_halo_red & 1;
		utemp |= (ee->merge_meth << YEE_HALO_RED_EN_SHIFT);
		regw_ip(utemp, YEE_TYP);
		regw_ip(ee->hpf_shft, YEE_SHF);
		regw_ip(ee->hpf_coef_00 & YEE_COEF_MASK, YEE_MUL_00);
		regw_ip(ee->hpf_coef_01 & YEE_COEF_MASK, YEE_MUL_01);
		regw_ip(ee->hpf_coef_02 & YEE_COEF_MASK, YEE_MUL_02);
		regw_ip(ee->hpf_coef_10 & YEE_COEF_MASK, YEE_MUL_10);
		regw_ip(ee->hpf_coef_11 & YEE_COEF_MASK, YEE_MUL_11);
		regw_ip(ee->hpf_coef_12 & YEE_COEF_MASK, YEE_MUL_12);
		regw_ip(ee->hpf_coef_20 & YEE_COEF_MASK, YEE_MUL_20);
		regw_ip(ee->hpf_coef_21 & YEE_COEF_MASK, YEE_MUL_21);
		regw_ip(ee->hpf_coef_22 & YEE_COEF_MASK, YEE_MUL_22);
		regw_ip(ee->yee_thr & YEE_THR_MASK, YEE_THR);
		regw_ip(ee->es_gain & YEE_ES_GAIN_MASK, YEE_E_GAN);
		regw_ip(ee->es_thr1 & YEE_ES_THR1_MASK, YEE_E_THR1);
		regw_ip(ee->es_thr2 & YEE_THR_MASK, YEE_E_THR2);
		regw_ip(ee->es_gain_grad & YEE_THR_MASK, YEE_G_GAN);
		regw_ip(ee->es_ofst_grad & YEE_THR_MASK, YEE_G_OFT);

		if (ee->table != NULL) {
			for (count = 0; count < MAX_SIZE_YEE_LUT; count++) {
				tbl_index = count >> 1;
				tbl_index <<= 2;
				/* Each table has 2 LUT entries, first in LS
				 * and second in MS positions
				 */
				if (count % 2) {
					utemp |= ((ee->table[count] &
						YEE_ENTRY_MASK) <<
						YEE_ENTRY_SHIFT);
					w_ip_table(utemp,
						tbl_index + YEE_TB_START_ADDR);
				} else
					utemp = ee->table[count] &
						YEE_ENTRY_MASK;
			}
		}
	}
	return 0;
}

/* Chromatic Artifact Correction. CAR */
static void ipipe_set_mf(void)
{
	/* typ to dynamic switch */
	regw_ip(IPIPE_CAR_DYN_SWITCH, CAR_TYP);
	/* Set SW0 to maximum */
	regw_ip(CAR_MF_THR, CAR_SW);
}

static void ipipe_set_gain_ctrl(struct prev_car *car)
{
	regw_ip(IPIPE_CAR_CHR_GAIN_CTRL, CAR_TYP);
	regw_ip(car->hpf, CAR_HPF_TYP);
	regw_ip(car->hpf_shft & CAR_HPF_SHIFT_MASK, CAR_HPF_SHF);
	regw_ip(car->hpf_thr, CAR_HPF_THR);
	regw_ip(car->gain1.gain, CAR_GN1_GAN);
	regw_ip(car->gain1.shft & CAR_GAIN1_SHFT_MASK, CAR_GN1_SHF);
	regw_ip(car->gain1.gain_min & CAR_GAIN_MIN_MASK, CAR_GN1_MIN);
	regw_ip(car->gain2.gain, CAR_GN2_GAN);
	regw_ip(car->gain2.shft & CAR_GAIN2_SHFT_MASK, CAR_GN2_SHF);
	regw_ip(car->gain2.gain_min & CAR_GAIN_MIN_MASK, CAR_GN2_MIN);
}

int ipipe_set_car_regs(struct prev_car *car)
{
	u32 utemp;
	ipipe_clock_enable();
	regw_ip(car->en, CAR_EN);
	if (car->en) {
		switch (car->meth) {
		case IPIPE_CAR_MED_FLTR:
			{
				ipipe_set_mf();
				break;
			}
		case IPIPE_CAR_CHR_GAIN_CTRL:
			{
				ipipe_set_gain_ctrl(car);
				break;
			}
		default:
			{
				/* Dynamic switch between MF and Gain Ctrl. */
				ipipe_set_mf();
				ipipe_set_gain_ctrl(car);
				/* Set the threshold for switching between
				 * the two Here we overwrite the MF SW0 value
				 */
				regw_ip(IPIPE_CAR_DYN_SWITCH, CAR_TYP);
				utemp = car->sw1;
				utemp <<= CAR_SW1_SHIFT;
				utemp |= car->sw0;
				regw_ip(utemp, CAR_SW);
			}
		}
	}
	return 0;
}

/* Chromatic Gain Suppression */
int ipipe_set_cgs_regs(struct prev_cgs *cgs)
{
	ipipe_clock_enable();
	regw_ip(cgs->en, CGS_EN);
	if (cgs->en) {
		/* Set the bright side parameters */
		regw_ip(cgs->h_thr, CGS_GN1_H_THR);
		regw_ip(cgs->h_slope, CGS_GN1_H_GAN);
		regw_ip(cgs->h_shft & CAR_SHIFT_MASK, CGS_GN1_H_SHF);
		regw_ip(cgs->h_min, CGS_GN1_H_MIN);
	}
	return 0;
}

void rsz_src_enable(int enable)
{
	regw_rsz(enable, RSZ_SRC_EN);
}

int rsz_enable(int rsz_id, int enable)
{
	if (rsz_id == RSZ_A) {
		regw_rsz(enable, RSZ_EN_A);
		/* We always enable RSZ_A. RSZ_B is enable upon request from
		 * application. So enable RSZ_SRC_EN along with RSZ_A
		 */
		regw_rsz(enable, RSZ_SRC_EN);
	} else if (rsz_id == RSZ_B)
		regw_rsz(enable, RSZ_EN_B);
	else
		return -EINVAL;
	return 0;
}
