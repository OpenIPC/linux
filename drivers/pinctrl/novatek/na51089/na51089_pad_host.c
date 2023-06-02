/*
    PAD controller

    Sets the PAD control of each pin.

    @file       pad.c
    @ingroup    mIDrvSys_PAD
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#include <plat/pad.h>
#include <plat/pad_reg.h>
#include "na51089_pinmux.h"

static u32 TOP_PAD_REG_BASE;
#define PAD_GETREG(ofs)         readl((void __iomem *)(TOP_PAD_REG_BASE + ofs))
#define PAD_SETREG(ofs,value)   writel(value, (void __iomem *)(TOP_PAD_REG_BASE + ofs))

static DEFINE_SPINLOCK(pad_lock);
#define loc_cpu(lock, flags) spin_lock_irqsave(lock, flags)
#define unl_cpu(lock, flags) spin_unlock_irqrestore(lock, flags)

#define	PAD_DS_GPIO_BASE_MASK      0xFFFFF
#define	PAD_DS_GROUP_10_MSK        0x0003
#define	PAD_DS_GROUP_16_MSK        0x0030
#define	PAD_DS_GROUP_40_MSK        0xFF00
#define	PAD_DS_GROUP_COMBO_MSK     0xFFFFF

/**
    Set a pin pull up/down/keeper.

    Set a pin pull up/down/keeper.

    @param[in]  pin Pin name. For example PAD_PIN_xxxx. See pad.h.
    @param[in]  pulltype PAD_NONE or PAD_PULLUP or PAD_PULLDOWN or PAD_KEEPER

    @return
        - @b E_OK: Done with no error
        - Others: Error occured.
*/
ER pad_set_pull_updown(PAD_PIN pin, PAD_PULL pulltype)
{
	UINT dw_offset, bit_offset;
	REGVALUE    reg_data;
	unsigned long flags = 0;

	if(pin == PAD_PIN_NOT_EXIST) {
		pr_err("Not Existed Pad Pin\r\n");
		return E_NOEXS;
	}

	bit_offset = pin & 0x1F;
	dw_offset = (pin >> 5);

	loc_cpu(&pad_lock, flags);
	reg_data = PAD_GETREG(PAD_PUPD0_REG_OFS + (dw_offset << 2));
	reg_data &= ~(3 << bit_offset);
	PAD_SETREG(PAD_PUPD0_REG_OFS + (dw_offset << 2), reg_data);
	reg_data |= (pulltype << bit_offset);
	PAD_SETREG(PAD_PUPD0_REG_OFS + (dw_offset << 2), reg_data);
	unl_cpu(&pad_lock, flags);
	return E_OK;
}
EXPORT_SYMBOL(pad_set_pull_updown);

/**
    Get a pin pull up/down/keeper

    Get a pin pull up/down/keeper

    @param[in]      pin         Pin name. For example PAD_PIN_xxxx. See pad.h.
    @param[out]     pulltype    PAD_NONE or PAD_PULLUP or PAD_PULLDOWN or PAD_KEEPER

    @return
        - @b E_OK: Done with no error
        - Others: Error occured.
*/
ER pad_get_pull_updown(PAD_PIN pin, PAD_PULL *pulltype)
{
	UINT dw_offset, bit_offset;
	REGVALUE    reg_data;

	if (pin > PAD_PIN_MAX)
		return E_NOEXS;

	if(pin == PAD_PIN_NOT_EXIST) {
		pr_err("Not Existed Pad Pin\r\n");
		return E_NOEXS;
	}

	bit_offset = pin & 0x1F;
	dw_offset = (pin >> 5);

	reg_data = PAD_GETREG(PAD_PUPD0_REG_OFS + (dw_offset << 2));
	*pulltype = ((reg_data >> bit_offset) & 0x3);

	return E_OK;
}

/**
    Set driving/sink.

    Set driving/sink.

    @param[in] name     For example PAD_DS_xxxx. See pad.h.
    @param[in] driving  PAD_DRIVINGSINK_xxMA. See pad.h.

    @return
        - @b E_OK: Done with no error
        - Others: Error occured.
*/
ER pad_set_drivingsink(PAD_DS name, PAD_DRIVINGSINK driving)
{
	UINT32      dw_offset, bit_offset, bit_mask;
	REGVALUE    reg_data;
	unsigned long flags = 0;

	if ((name & PAD_DS_GPIO_BASE_MASK) > (PAD_DS_MAX & PAD_DS_GPIO_BASE_MASK))
		return E_NOEXS;

	if (name == PAD_PIN_NOT_EXIST) {
		pr_err("Not Existed Pad Pin\r\n");
		return E_NOEXS;
	}

	if (nvt_get_chip_id() == CHIP_NA51055) {
		if(name & PAD_DS_GROUP_40) {
			/* 40MA GROUP */
			dw_offset  = (PAD_DS10_REG_OFS - PAD_DS_REG_OFS);
			dw_offset += (((name & PAD_DS_GPIO_BASE_MASK)>>5)<<2);
			bit_offset = name & 0x1F;
			bit_mask   = 0x07;
			driving &= PAD_DS_GROUP_40_MSK;

			if (driving & PAD_DRIVINGSINK_5MA) {
				driving = 0;
			} else if (driving & PAD_DRIVINGSINK_10MA) {
				driving = 1;
			} else if (driving & PAD_DRIVINGSINK_15MA) {
				driving = 2;
			} else if (driving & PAD_DRIVINGSINK_20MA) {
				driving = 3;
			} else if (driving & PAD_DRIVINGSINK_25MA) {
				driving = 4;
			} else if (driving & PAD_DRIVINGSINK_30MA) {
				driving = 5;
			} else if (driving & PAD_DRIVINGSINK_35MA) {
				driving = 6;
			} else if (driving & PAD_DRIVINGSINK_40MA) {
				driving = 7;
			} else {
				pr_err("No Valid DS value.(0x%X)\r\n", name);
				return E_PAR;
			}
		}
		else if (name & PAD_DS_GROUP_16) {
			/* 16MA GROUP */
			dw_offset  = (((name & PAD_DS_GPIO_BASE_MASK)>>5)<<2);
			bit_offset = name & 0x1F;
			bit_mask   = 0x01;
			driving &= PAD_DS_GROUP_16_MSK;

			if (driving & PAD_DRIVINGSINK_6MA) {
				driving = 0;
			} else if (driving & PAD_DRIVINGSINK_16MA) {
				driving = 1;
			} else {
				pr_err("No Valid DS value.(0x%X)\r\n", name);
				return E_PAR;
			}
		} else {
			/* 10MA GROUP */
			if (name >= PAD_DS_HSIGPIO0) {
				dw_offset  = (((name & PAD_DS_GPIO_BASE_MASK)>>5)<<2) + 0x28;
			} else {
				dw_offset  = (((name & PAD_DS_GPIO_BASE_MASK)>>5)<<2);
			}
			bit_offset = name & 0x1F;
			bit_mask   = 0x01;
			driving &= PAD_DS_GROUP_10_MSK;

			if (driving & PAD_DRIVINGSINK_4MA) {
				driving = 0;
			} else if (driving & PAD_DRIVINGSINK_10MA) {
				driving = 1;
			} else {
				pr_err("No Valid DS value.(0x%X)\r\n",name);
				return E_PAR;
			}
		}

		loc_cpu(&pad_lock, flags);
		reg_data = PAD_GETREG(PAD_DS_REG_OFS + dw_offset);
		reg_data &= ~(bit_mask << bit_offset);
		reg_data |= (driving << bit_offset);
		PAD_SETREG(PAD_DS_REG_OFS + dw_offset, reg_data);
		unl_cpu(&pad_lock, flags);
	} else {
		if(name & PAD_DS_GROUP_40) {
			/* 40MA GROUP */
			dw_offset  = (PAD_DS10_REG_OFS - PAD_DS_REG_OFS);
			dw_offset += (((name & PAD_DS_GPIO_BASE_MASK)>>5)<<2);
			bit_offset = name & 0x1F;
			bit_mask   = 0x07;
			driving &= PAD_DS_GROUP_40_MSK;

			if (driving & PAD_DRIVINGSINK_5MA) {
				driving = 0;
			} else if (driving & PAD_DRIVINGSINK_10MA) {
				driving = 1;
			} else if (driving & PAD_DRIVINGSINK_15MA) {
				driving = 2;
			} else if (driving & PAD_DRIVINGSINK_20MA) {
				driving = 3;
			} else if (driving & PAD_DRIVINGSINK_25MA) {
				driving = 4;
			} else if (driving & PAD_DRIVINGSINK_30MA) {
				driving = 5;
			} else if (driving & PAD_DRIVINGSINK_35MA) {
				driving = 6;
			} else if (driving & PAD_DRIVINGSINK_40MA) {
				driving = 7;
			} else {
				pr_err("No Valid DS value.(0x%X)\r\n", name);
				return E_PAR;
			}
		} else {
			if ((name & ~PAD_DS_GROUP_MASK) >= (PAD_DS_HSIGPIO0 & ~PAD_DS_GROUP_MASK)) {
				dw_offset  = (((name & PAD_DS_GPIO_BASE_MASK)>>5)<<2) + 0x28;
			} else {
				dw_offset  = (((name & PAD_DS_GPIO_BASE_MASK)>>5)<<2);
			}
			bit_offset = name & 0x1F;
			bit_mask   = 0x07;

			if (name & PAD_DS_GROUP_16)
				driving &= PAD_DS_GROUP_COMBO_MSK;

			if ((driving == PAD_DRIVINGSINK_4MA) || (driving == PAD_DRIVINGSINK_6MA)) {
				driving = 0;
			} else if ((driving == PAD_DRIVINGSINK_8MA) || (driving == PAD_DRIVINGSINK_10MA)) {
				driving = 1;
			} else if (driving == PAD_DRIVINGSINK_12MA) {
				driving = 2;
			} else if (driving == PAD_DRIVINGSINK_16MA) {
				driving = 3;
			} else {
				pr_err("No Valid DS value.(0x%X)\r\n",name);
				return E_PAR;
			}
		}

		loc_cpu(&pad_lock, flags);
		reg_data = PAD_GETREG(PAD_DS_REG_OFS + dw_offset);
		reg_data &= ~(bit_mask << bit_offset);
		reg_data |= (driving << bit_offset);
		PAD_SETREG(PAD_DS_REG_OFS + dw_offset, reg_data);
		unl_cpu(&pad_lock, flags);
	}

	return E_OK;
}
EXPORT_SYMBOL(pad_set_drivingsink);

/**
    Get driving/sink.

    Get driving/sink.

    @param[in]  name        For example PAD_DS_xxxx. See pad.h.
    @param[out] driving     PAD_DRIVINGSINK_xxMA. See pad.h.

    @return
        - @b E_OK: Done with no error
        - Others: Error occured.
*/
ER pad_get_drivingsink(PAD_DS name, PAD_DRIVINGSINK *driving)
{
	UINT32      dw_offset, bit_offset, bit_mask;
	REGVALUE    reg_data;

	if ((name & PAD_DS_GPIO_BASE_MASK) > (PAD_DS_MAX & PAD_DS_GPIO_BASE_MASK))
		return E_NOEXS;

	if (name == PAD_PIN_NOT_EXIST) {
		pr_err("Not Existed Pad Pin\r\n");
		return E_NOEXS;
	}

	if (nvt_get_chip_id() == CHIP_NA51055) {
		if (name & PAD_DS_GROUP_40) {
			/* 40MA GROUP */
			dw_offset  = (PAD_DS10_REG_OFS - PAD_DS_REG_OFS);
			dw_offset += (((name & PAD_DS_GPIO_BASE_MASK)>>5)<<2);
			bit_offset = name & 0x1F;
			bit_mask   = 0x07;

			reg_data = PAD_GETREG(PAD_DS_REG_OFS + dw_offset);
			*driving = (reg_data >> bit_offset) & bit_mask;

			if (*driving == 0) {
				*driving = PAD_DRIVINGSINK_5MA;
			} else if (*driving == 1) {
				*driving = PAD_DRIVINGSINK_10MA;
			} else if (*driving == 2) {
				*driving = PAD_DRIVINGSINK_15MA;
			} else if (*driving == 3) {
				*driving = PAD_DRIVINGSINK_20MA;
			} else if (*driving == 4) {
				*driving = PAD_DRIVINGSINK_25MA;
			} else if (*driving == 5) {
				*driving = PAD_DRIVINGSINK_30MA;
			} else if (*driving == 6) {
				*driving = PAD_DRIVINGSINK_35MA;
			} else {
				*driving = PAD_DRIVINGSINK_40MA;
			}
		} else if(name & PAD_DS_GROUP_16) {
			/* 16MA GROUP */
			dw_offset  = (((name & PAD_DS_GPIO_BASE_MASK)>>5)<<2);
			bit_offset = name & 0x1F;
			bit_mask   = 0x01;

			reg_data = PAD_GETREG(PAD_DS_REG_OFS + dw_offset);
			*driving = (reg_data >> bit_offset) & bit_mask;

			if(*driving == 0) {
				*driving = PAD_DRIVINGSINK_6MA;
			} else {
				*driving = PAD_DRIVINGSINK_16MA;
			}
		} else {
			/* 10MA GROUP */

			if (name >= PAD_DS_HSIGPIO0) {
				dw_offset  = (((name & PAD_DS_GPIO_BASE_MASK)>>5)<<2) + 0x28;
			} else {
				dw_offset  = (((name & PAD_DS_GPIO_BASE_MASK)>>5)<<2);
			}
			bit_offset = name & 0x1F;
			bit_mask   = 0x01;

			reg_data = PAD_GETREG(PAD_DS_REG_OFS + dw_offset);
			*driving = (reg_data >> bit_offset) & bit_mask;

			if(*driving == 0) {
				*driving = PAD_DRIVINGSINK_4MA;
			} else {
				*driving = PAD_DRIVINGSINK_10MA;
			}
		}
	} else {
		if (name & PAD_DS_GROUP_40) {
			/* 40MA GROUP */
			dw_offset  = (PAD_DS10_REG_OFS - PAD_DS_REG_OFS);
			dw_offset += (((name & PAD_DS_GPIO_BASE_MASK)>>5)<<2);
			bit_offset = name & 0x1F;
			bit_mask   = 0x07;

			reg_data = PAD_GETREG(PAD_DS_REG_OFS + dw_offset);
			*driving = (reg_data >> bit_offset) & bit_mask;

			if (*driving == 0) {
				*driving = PAD_DRIVINGSINK_5MA;
			} else if (*driving == 1) {
				*driving = PAD_DRIVINGSINK_10MA;
			} else if (*driving == 2) {
				*driving = PAD_DRIVINGSINK_15MA;
			} else if (*driving == 3) {
				*driving = PAD_DRIVINGSINK_20MA;
			} else if (*driving == 4) {
				*driving = PAD_DRIVINGSINK_25MA;
			} else if (*driving == 5) {
				*driving = PAD_DRIVINGSINK_30MA;
			} else if (*driving == 6) {
				*driving = PAD_DRIVINGSINK_35MA;
			} else {
				*driving = PAD_DRIVINGSINK_40MA;
			}
		} else {
			/* 10MA GROUP */
			if ((name >= PAD_DS_HSIGPIO0) && !(name & PAD_DS_GROUP_16)) {
				dw_offset  = (((name & PAD_DS_GPIO_BASE_MASK)>>5)<<2) + 0x28;
			} else {
				dw_offset  = (((name & PAD_DS_GPIO_BASE_MASK)>>5)<<2);
			}
			bit_offset = name & 0x1F;
			bit_mask   = 0x07;

			reg_data = PAD_GETREG(PAD_DS_REG_OFS + dw_offset);
			*driving = (reg_data >> bit_offset) & bit_mask;

			if(*driving == 0) {
				*driving = PAD_DRIVINGSINK_4MA;
			} else if (*driving == 1) {
				*driving = PAD_DRIVINGSINK_8MA;
			} else if (*driving == 2) {
				*driving = PAD_DRIVINGSINK_12MA;
			} else {
				*driving = PAD_DRIVINGSINK_16MA;
			}
		}
	}

	return E_OK;
}
EXPORT_SYMBOL(pad_get_drivingsink);

/**
    Set pad power.

    Set pad power.

    @param[in] pad_power  pointer to pad power struct

    @return
        - @b E_OK: Done with no error
        - Others: Error occured.
*/
ER pad_set_power(PAD_POWER_STRUCT *pad_power)
{
	union PAD_PWR1_REG pad_power1_reg = {0};
	union PAD_PWR2_REG pad_power2_reg = {0};
	union PAD_PWR3_REG pad_power3_reg = {0};
	unsigned long flags = 0;

	if (pad_power == NULL) {
		return E_PAR;
	}
	if (pad_power->pad_power_id == PAD_POWERID_MC1) {
		pr_err("MC1 power id no exist\r\n");
		return E_NOEXS;
	} else if (pad_power->pad_power_id == PAD_POWERID_MC0) {
		pad_power1_reg.bit.MC_POWER0 = pad_power->pad_power;
		pad_power1_reg.bit.MC_SEL = pad_power->bias_current;
		pad_power1_reg.bit.MC_OPSEL = pad_power->opa_gain;
		pad_power1_reg.bit.MC_PULLDOWN = pad_power->pull_down;
		pad_power1_reg.bit.MC_REGULATOR_EN = pad_power->enable;

		loc_cpu(&pad_lock, flags);
		PAD_SETREG(PAD_PWR1_REG_OFS, pad_power1_reg.reg);
		unl_cpu(&pad_lock, flags);

		return E_OK;
	} else if (pad_power->pad_power_id == PAD_POWERID_ADC) {
		if (pad_power->pad_vad & PAD_VAD_2P4V)
			return E_PAR;

		pad_power2_reg.bit.ADC_POWER0 = pad_power->pad_power;
		pad_power2_reg.bit.ADC_SEL = pad_power->bias_current;
		pad_power2_reg.bit.ADC_OPSEL = pad_power->opa_gain;
		pad_power2_reg.bit.ADC_PULLDOWN = pad_power->pull_down;
		pad_power2_reg.bit.ADC_REGULATOR_EN = pad_power->enable;
		pad_power2_reg.bit.ADC_VAD = pad_power->pad_vad;

		loc_cpu(&pad_lock, flags);
		PAD_SETREG(PAD_PWR2_REG_OFS, pad_power2_reg.reg);
		unl_cpu(&pad_lock, flags);

		return E_OK;
	} else if (pad_power->pad_power_id == PAD_POWERID_CSI) {
		if (!(pad_power->pad_vad & PAD_VAD_2P4V))
			return E_PAR;

		pad_power3_reg.bit.CSI_SEL = pad_power->bias_current;
		pad_power3_reg.bit.CSI_OPSEL = pad_power->opa_gain;
		pad_power3_reg.bit.CSI_PULLDOWN = pad_power->pull_down;
		pad_power3_reg.bit.CSI_REGULATOR_EN = pad_power->enable;
		pad_power3_reg.bit.CSI_VAD = pad_power->pad_vad & 0xFF;
		PAD_SETREG(PAD_PWR3_REG_OFS, pad_power3_reg.reg);

		return E_OK;
	} else {
		return E_PAR;
	}
}
EXPORT_SYMBOL(pad_set_power);

/**
    Get pad power.

    get pad power.

    @param[in] pad_power  pointer to pad power struct
*/
void pad_get_power(PAD_POWER_STRUCT *pad_power)
{
	union PAD_PWR1_REG pad_power1_reg = {0};
	union PAD_PWR2_REG pad_power2_reg = {0};
	union PAD_PWR3_REG pad_power3_reg = {0};

	if (pad_power->pad_power_id == PAD_POWERID_MC0) {
		pad_power1_reg.reg = PAD_GETREG(PAD_PWR1_REG_OFS);

		pad_power->pad_power = pad_power1_reg.bit.MC_POWER0;
		pad_power->bias_current = pad_power1_reg.bit.MC_SEL;
		pad_power->opa_gain = pad_power1_reg.bit.MC_OPSEL;
		pad_power->pull_down = pad_power1_reg.bit.MC_PULLDOWN;
		pad_power->enable = pad_power1_reg.bit.MC_REGULATOR_EN;
	} else if (pad_power->pad_power_id == PAD_POWERID_ADC) {
		pad_power2_reg.reg = PAD_GETREG(PAD_PWR2_REG_OFS);

		pad_power->pad_power = pad_power2_reg.bit.ADC_POWER0;
		pad_power->bias_current = pad_power2_reg.bit.ADC_SEL;
		pad_power->opa_gain = pad_power2_reg.bit.ADC_OPSEL;
		pad_power->pull_down = pad_power2_reg.bit.ADC_PULLDOWN;
		pad_power->enable = pad_power2_reg.bit.ADC_REGULATOR_EN;
		pad_power->pad_vad = pad_power2_reg.bit.ADC_VAD;
	} else if (pad_power->pad_power_id == PAD_POWERID_CSI) {
		pad_power3_reg.reg = PAD_GETREG(PAD_PWR3_REG_OFS);

		pad_power->bias_current = pad_power3_reg.bit.CSI_SEL;
		pad_power->opa_gain = pad_power3_reg.bit.CSI_OPSEL;
		pad_power->pull_down = pad_power3_reg.bit.CSI_PULLDOWN;
		pad_power->enable = pad_power3_reg.bit.CSI_REGULATOR_EN;
		pad_power->pad_vad = pad_power3_reg.bit.CSI_VAD;
	}
}
EXPORT_SYMBOL(pad_get_power);

void pad_preset(struct nvt_pinctrl_info *info)
{
	TOP_PAD_REG_BASE = (u32) info->pad_base;
}


ER pad_init(struct nvt_pinctrl_info *info, u32 nr_pad)
{
	int cnt, ret = 0;

	for (cnt = 0; cnt < nr_pad; cnt++) {
		ret = pad_set_pull_updown(info->pad[cnt].pad_gpio_pin, info->pad[cnt].direction);
		if (ret) {
			pr_err("pad pin 0x%x pull failed!\n", info->pad[cnt].pad_gpio_pin);
			return ret;
		}

		ret = pad_set_drivingsink(info->pad[cnt].pad_ds_pin, info->pad[cnt].driving);
		if (ret) {
			pr_err("pad pin 0x%x driving failed!\n", info->pad[cnt].pad_ds_pin);
			return ret;
		}
	}

	return E_OK;
}
