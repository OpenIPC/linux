/* *
 * Copyright (C) 2009 Texas Instruments Inc
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
#include <media/davinci/dm365_af.h>
#include <media/davinci/dm365_aew.h>
#include <media/davinci/dm365_a3_hw.h>
#include <linux/export.h>
#include <linux/module.h>
/* 3A hardware module configuration */
struct a3_config {
	/* lock for write protection */
	struct mutex lock;
	/* base address */
	void __iomem *base;
};

/* H3A module configuration */
static struct a3_config a3_cfg;

/* register access routines */
static inline u32 regr(unsigned int offset)
{
	return __raw_readl(a3_cfg.base + offset);
}

static inline u32 regw(u32 val, unsigned int offset)
{
	__raw_writel(val, (a3_cfg.base + offset));
	return val;
}

/* Function to set register */
int af_register_setup(struct device *dev, struct af_device *af_dev)
{
	unsigned int utemp = 0;
	unsigned int address = 0;
	int index;

	/* Lock resource since this register is written by
	 * both the AF and AEW drivers
	 */
	mutex_lock(&a3_cfg.lock);
	/* Configure Hardware Registers */
	/* Set PCR Register */
	utemp = regr(AFPCR);	/* Read PCR Register */

	/*Set Accumulator Mode */
	utemp &= ~FVMODE;
	if (af_dev->config->mode == ACCUMULATOR_PEAK)
		utemp |= FVMODE;
	/* Set A-law */
	utemp &= ~AF_ALAW_EN;
	if (af_dev->config->alaw_enable == H3A_AF_ENABLE)
		utemp |= AF_ALAW_EN;

	/* Set HFV only or HFV and VFV */
	utemp &= ~AF_VF_EN;
	if (af_dev->config->fv_sel == AF_HFV_AND_VFV)
		utemp |= AF_VF_EN;
	/* Set RGB Position */
	utemp &= ~RGBPOS;
	utemp |= (af_dev->config->rgb_pos) << AF_RGBPOS_SHIFT;

	utemp &= ~AF_MED_EN;
	/*HMF Configurations */
	if (af_dev->config->hmf_config.enable == H3A_AF_ENABLE) {
		/* Enable HMF */
		utemp |= AF_MED_EN;

		/* Set Median Threshold */
		utemp &= ~MED_TH;
		utemp |=
		    (((af_dev->config->hmf_config.threshold)
			<< AF_MED_TH_SHIFT) & MED_TH);
	}
	/* Set PCR Register */
	regw(utemp , AFPCR);
	mutex_unlock(&a3_cfg.lock);

	/* Configure AFPAX1 */
	/*Paxel parameter configuration */
	/*Set Width in AFPAX1 Register */
	utemp = (SET_VAL(af_dev->config->paxel_config.width)) <<
		AF_PAXW_SHIFT;

	/* Set height in AFPAX1 */
	utemp &= ~PAXH;
	utemp |= (SET_VAL(af_dev->config->paxel_config.height));

	regw(utemp , AFPAX1);

	/* Configure AFPAX2 Register */
	/* Set Column Increment in AFPAX2 Register */
	utemp = 0;
	utemp &= ~AFINCH;
	utemp |=
	    (SET_VAL(af_dev->config->paxel_config.column_incr)) <<
	    AF_COLUMN_INCR_SHIFT;

	/* Set Line Increment in AFPAX2 Register */
	utemp &= ~AFINCV;
	utemp |=
	    (SET_VAL(af_dev->config->paxel_config.line_incr)) <<
	    AF_LINE_INCR_SHIFT;

	/* Set Vertical Count */
	utemp &= ~PAXVC;
	utemp |= (af_dev->config->paxel_config.vt_cnt - 1) << AF_VT_COUNT_SHIFT;
	/* Set Horizontal Count */
	utemp &= ~PAXHC;
	utemp |= (af_dev->config->paxel_config.hz_cnt - 1);
	regw(utemp, AFPAX2);

	/* Configure PAXSTART Register */
	/*Configure Horizontal Start */
	utemp = 0;
	utemp &= ~PAXSH;
	utemp |=
	    (af_dev->config->paxel_config.hz_start) << AF_HZ_START_SHIFT;
	/* Configure Vertical Start */
	utemp &= ~PAXSV;
	utemp |= af_dev->config->paxel_config.vt_start;
	regw(utemp , AFPAXSTART);

	/*SetIIRSH Register */
	regw(af_dev->config->iir_config.hz_start_pos, AFIIRSH);

	/* Set IIR Filter0 Coefficients */
	address = AFCOEF010;
	for (index = 0; index < AF_NUMBER_OF_HFV_COEF; index += 2) {
		utemp = af_dev->config->iir_config.coeff_set0[index] &
			COEF_MASK0;
		if (index < AF_NUMBER_OF_HFV_COEF-1) {
			utemp |=
			(((af_dev->config->iir_config.coeff_set0[index + 1]) <<
			AF_COEF_SHIFT) & COEF_MASK1);
		}
		regw(utemp, address);
		dev_dbg(dev, "\n COEF0 %x", regr(address));
		address = address + AF_OFFSET;
	}

	/* Set IIR Filter1 Coefficients */
	address = AFCOEF110;
	for (index = 0; index < AF_NUMBER_OF_HFV_COEF; index += 2) {
		utemp = af_dev->config->iir_config.coeff_set1[index] &
			COEF_MASK0;
		if (index < AF_NUMBER_OF_HFV_COEF-1) {
			utemp |=
			(((af_dev->config->iir_config.coeff_set1[index + 1]) <<
			AF_COEF_SHIFT) & COEF_MASK1);
		}
		regw(utemp, address);
		dev_dbg(dev, "\n COEF0 %x", regr(address));
		address = address + AF_OFFSET;
	}

	/* HFV thresholds for FIR 1 & 2 */
	utemp = af_dev->config->fir_config.hfv_thr1 & HFV_THR0_MASK;
	utemp |= ((af_dev->config->fir_config.hfv_thr2 << HFV_THR2_SHIFT) &
			HFV_THR2_MASK);
	regw(utemp, AF_HFV_THR);

	/* VFV coefficients and thresholds */
	utemp = af_dev->config->fir_config.coeff_1[0] & VFV_COEF_MASK0;
	utemp |= ((af_dev->config->fir_config.coeff_1[1] << 8) &
			VFV_COEF_MASK1);
	utemp |= ((af_dev->config->fir_config.coeff_1[2] << 16) &
			VFV_COEF_MASK2);
	utemp |= ((af_dev->config->fir_config.coeff_1[3] << 24) &
			VFV_COEF_MASK3);
	regw(utemp, AF_VFV_CFG1);

	utemp = af_dev->config->fir_config.coeff_1[4] & VFV_COEF_MASK0;
	utemp |= ((af_dev->config->fir_config.vfv_thr1 << VFV_THR_SHIFT) &
			VFV_THR_MASK);
	regw(utemp, AF_VFV_CFG2);

	/* VFV coefficients and thresholds */
	utemp = af_dev->config->fir_config.coeff_2[0] & VFV_COEF_MASK0;
	utemp |= ((af_dev->config->fir_config.coeff_2[1] << 8) &
			VFV_COEF_MASK1);
	utemp |= ((af_dev->config->fir_config.coeff_2[2] << 16) &
			VFV_COEF_MASK2);
	utemp |= ((af_dev->config->fir_config.coeff_2[3] << 24) &
			VFV_COEF_MASK3);
	regw(utemp, AF_VFV_CFG3);

	utemp = af_dev->config->fir_config.coeff_2[4] & VFV_COEF_MASK0;
	utemp |= ((af_dev->config->fir_config.vfv_thr2 << VFV_THR_SHIFT) &
			VFV_THR_MASK);
	regw(utemp, AF_VFV_CFG4);
	/* Set AFBUFST to Current buffer Physical Address */
	regw((unsigned int)(virt_to_phys(af_dev->buff_curr)), AFBUFST);

	return 0;
}
EXPORT_SYMBOL(af_register_setup);

inline u32 af_get_hw_state(void)
{
	return (regr(AFPCR) & AF_BUSYAF) >> AF_BUSYAF_SHIFT;
}
EXPORT_SYMBOL(af_get_hw_state);

inline u32 aew_get_hw_state(void)
{
	return (regr(AEWPCR) & AEW_BUSYAEWB) >> AEW_BUSYAEW_SHIFT;
}
EXPORT_SYMBOL(aew_get_hw_state);

inline u32 af_get_enable(void)
{
	return regr(AFPCR) & AF_EN;
}
EXPORT_SYMBOL(af_get_enable);

inline u32 aew_get_enable(void)
{
	return (regr(AEWPCR) & AEW_EN) >> AEW_EN_SHIFT;
}
EXPORT_SYMBOL(aew_get_enable);

/* Function to Enable/Disable AF Engine */
inline void af_engine_setup(struct device *dev, int enable)
{
	unsigned int pcr;

	mutex_lock(&a3_cfg.lock);
	pcr = regr(AFPCR);
	dev_dbg(dev, "\nEngine Setup value before PCR : %x", pcr);

	/* Set AF_EN bit in PCR Register */
	if (enable)
		pcr |= AF_EN;
	else
		pcr &= ~AF_EN;

	regw(pcr, AFPCR);
	mutex_unlock(&a3_cfg.lock);

	dev_dbg(dev, "\n Engine Setup value after PCR : %x", pcr);
}
EXPORT_SYMBOL(af_engine_setup);

/* Function to set address */
inline void af_set_address(struct device *dev, unsigned long address)
{
	regw((address & ~0x3F), AFBUFST);
}
EXPORT_SYMBOL(af_set_address);

/* Function to set hardware configuration registers */
int aew_register_setup(struct device *dev, struct aew_device *aew_dev)
{
	unsigned utemp;

	mutex_lock(&a3_cfg.lock);
	/* Set up the registers */
	utemp = regr(AEWPCR);

	/* Enable A Law */
	if (aew_dev->config->alaw_enable == H3A_AEW_ENABLE)
		utemp |= AEW_ALAW_EN;
	else
		utemp &= ~AEW_ALAW_EN;

	utemp &= ~AEW_MED_EN;
	/*HMF Configurations */
	if (aew_dev->config->hmf_config.enable == H3A_AEW_ENABLE) {
		/* Enable HMF */
		utemp |= AEW_MED_EN;
		/* Set Median Threshold */
		utemp &= ~MED_TH;
		utemp |=
		    (((aew_dev->config->hmf_config.threshold) <<
			AF_MED_TH_SHIFT) & MED_TH);
	}

	/*Configure Saturation limit */
	utemp &= ~AVE2LMT;
	utemp |= aew_dev->config->saturation_limit << AEW_AVE2LMT_SHIFT;
	regw(utemp, AEWPCR);
	mutex_unlock(&a3_cfg.lock);

	/*Window parameter configuration */
	/* Configure Window Width in AEWWIN1 register */
	utemp =
	    ((SET_VAL(aew_dev->config->window_config.height)) <<
	     AEW_WINH_SHIFT);

	/* Configure Window height  in AEWWIN1 register */
	utemp |=
	    ((SET_VAL(aew_dev->config->window_config.width)) <<
	     AEW_WINW_SHIFT);

	/* Configure Window vertical count  in AEWWIN2 register */
	utemp |=
	    ((aew_dev->config->window_config).vt_cnt - 1) << AEW_VT_COUNT_SHIFT;

	/* Configure Window horizontal count  in AEWWIN1 register */
	utemp |= ((aew_dev->config->window_config).hz_cnt - 1);

	/* Configure Window vertical start  in AEWWIN1 register */
	regw(utemp, AEWWIN1);

	/*Window Start parameter configuration */

	utemp =
	    (aew_dev->config->window_config).vt_start << AEW_VT_START_SHIFT;

	/* Configure Window horizontal start  in AEWWIN2 register */
	utemp &= ~WINSH;
	utemp |= (aew_dev->config->window_config).hz_start;
	regw(utemp, AEWINSTART);

	/*Window Line Increment configuration */
	/*Configure vertical line increment in AEWSUBWIN */
	utemp =
	    (SET_VAL(aew_dev->config->window_config.
			 vt_line_incr) << AEW_LINE_INCR_SHIFT);

	/* Configuring Horizontal Line increment in AEWSUBWIN */
	utemp &= ~AEWINCH;
	utemp |= (SET_VAL(aew_dev->config->window_config.hz_line_incr));

	regw(utemp, AEWSUBWIN);

	/* Black Window Configuration */
	/* Configure vertical start and height in AEWWINBLK */
	utemp =
	    (aew_dev->config->blackwindow_config).
	    vt_start << AEW_BLKWIN_VT_START_SHIFT;

	/* Configure height in Black window */
	utemp &= ~BLKWINH;
	utemp |= (SET_VAL(aew_dev->config->blackwindow_config.height));
	regw(utemp, AEWINBLK);

	/* AE/AWB engine configuration */
	utemp = aew_dev->config->sum_shift & AEW_SUMSHFT_MASK;
	utemp |= (((aew_dev->config->out_format) << AEFMT_SHFT) & AEFMT_MASK);
	regw(utemp, AEW_CFG);

	/* Set AFBUFST to Current buffer Physical Address */
	regw((unsigned int)(virt_to_phys(aew_dev->buff_curr)), AEWBUFST);

	return 0;
}
EXPORT_SYMBOL(aew_register_setup);

/* Function to enable/ disable AEW Engine */
inline void aew_engine_setup(struct device *dev, int value)
{
	unsigned int pcr;

	dev_dbg(dev, "\nAEW_REG(PCR) Before Setting %x", regr(AEWPCR));

	mutex_lock(&a3_cfg.lock);
	/* Read Pcr Register */
	pcr = regr(AEWPCR);
	pcr &= ~AEW_EN;
	pcr |= (value << AEW_EN_SHIFT);

	/*Set AF_EN bit in PCR Register */
	regw(pcr, AEWPCR);
	mutex_unlock(&a3_cfg.lock);

	dev_dbg(dev, "\nAfter Setting %d : PCR VALUE %x", value,
		regr(AEWPCR));

}
EXPORT_SYMBOL(aew_engine_setup);

/* Function used to set adddress */
inline void aew_set_address(struct device *dev, unsigned long address)
{
	regw((address & ~0x3F), AEWBUFST);
}
EXPORT_SYMBOL(aew_set_address);

static int  dm365_afew_hw_init(void)
{
	printk(KERN_NOTICE "dm365_afew_hw_init\n");
	mutex_init(&a3_cfg.lock);
	a3_cfg.base = ioremap(DM365_A3_HW_ADDR, DM365_A3_HW_ADDR_SIZE);
	if (!a3_cfg.base) {
		printk(KERN_ERR "Unable to ioremap 3A registers\n");
		return -EINVAL;
	}
	regw(0, LINE_START);
	return 0;
}

static void dm365_afew_hw_exit(void)
{
	printk(KERN_NOTICE "dm365_afew_hw_exit\n");
	iounmap(a3_cfg.base);
}
subsys_initcall(dm365_afew_hw_init);
module_exit(dm365_afew_hw_exit);
MODULE_LICENSE("GPL");
