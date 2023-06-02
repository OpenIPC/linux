/*
    Embedded Audio Codec Driver

    This file is the driver for Embedded Audio Codec.
    Default disable ALC. Default enable MIC-bias/NG/DCCAN/ZC.

    @file       eac.c
    @ingroup    mIDrvAud_EAC
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2014.  All rights reserved.
*/
#ifdef __KERNEL__
#include <linux/delay.h>
#include <mach/rcw_macro.h>
#include <linux/clk.h>
//#include "kwrap/type.h"
//#include "kwrap/semaphore.h"
//#include "kwrap/flag.h"

#include "eac.h"
#include "eac_int.h"
#include "eac_reg.h"

static DEFINE_SPINLOCK(my_lock);
#define loc_cpu(myflags)	spin_lock_irqsave(&my_lock, myflags)
#define unl_cpu(myflags)	spin_unlock_irqrestore(&my_lock, myflags)

UINT32 _EAC_REG_BASE_ADDR[1];

#define DBG_WRN(fmt, args...) printk(fmt, ##args)
#define DBG_ERR(fmt, args...) printk(fmt, ##args)
#elif defined(__FREERTOS)
#define __MODULE__ rtos_eac
#define __DBGLVL__ 8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__ "*"

#include "kwrap/error_no.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "kwrap/debug.h"
#include "kwrap/task.h"
#include "kwrap/spinlock.h"
#include "kwrap/type.h"

#include "include/eac.h"
#include "include/eac_reg.h"
#include "eac_int.h"
#include "io_address.h"
#include "pll_protected.h"

static  VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags) vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags) vk_spin_unlock_irqrestore(&my_lock, flags)

unsigned int rtos_eac_debug_level = NVT_DBG_WRN;

#endif

/**
    @addtogroup mIDrvAud_EAC
*/
//@{


static UINT32 g_eac_dc_init = 0x00000000;
static UINT32 dmic2_dc_init = 0x00000000;

/*
    Reset ADC DCCAN offset value

    Reset ADC DCCAN offset value

    @param[in]  b_dc_can_rst
     - @b TRUE:  Reset the DC Cancellation offset value.
     - @b FALSE: Keep the DC Cancellation offset value.

    @return void
*/
static void eac_set_dccan_rst(BOOL b_dc_can_rst)
{
	T_EADC_CTRL0_REG    adc_ctrl_0;

	// Set DC Cancellation's Mode
	adc_ctrl_0.reg = EAC_GETREG(EADC_CTRL0_REG_OFS);
	adc_ctrl_0.bit.ADC_DCCAN_RESET = b_dc_can_rst;
	EAC_SETREG(EADC_CTRL0_REG_OFS, adc_ctrl_0.reg);
}

/*
    Set ADC DCCAN initial value

    Set ADC DCCAN initial value for both left and right channels.

    @param[in] ui_dc_initial Assigned value for DC Cancellation initial value.

    @return void
*/
static void eac_init_dccan_ofs(UINT32 ui_dc_initial, UINT32 ui_dmic2_dcinital)
{
	T_EADC_DCCAN_INIT_REG   adc_dccan_init;
	T_DMIC2_DCCAN_INIT_REG  dmic2_dcinit;
	T_EADC_CTRL0_REG        adc_ctrl_0;

	// Configure the DC Cancellation Initial value
	adc_dccan_init.reg = ui_dc_initial;
	EAC_SETREG(EADC_DCCAN_INIT_REG_OFS, adc_dccan_init.reg);

	// Configure the dmic2 DC Cancellation Initial value
	dmic2_dcinit.reg = ui_dmic2_dcinital;
	EAC_SETREG(DMIC2_DCCAN_INIT_REG_OFS, dmic2_dcinit.reg);

	// Set DCCAN Load Init Value
	adc_ctrl_0.reg = EAC_GETREG(EADC_CTRL0_REG_OFS);
	adc_ctrl_0.bit.ADC_DCCAN_INIT_LOAD = 1;
	EAC_SETREG(EADC_CTRL0_REG_OFS, adc_ctrl_0.reg);

	// Check if the DCCAN Load bit is cleared
	do {

		adc_ctrl_0.reg = EAC_GETREG(EADC_CTRL0_REG_OFS);
	} while (adc_ctrl_0.bit.ADC_DCCAN_INIT_LOAD == 1);

}

/*
  set ADC DCCAN Configuration

  This function configure DCCAN Filter initialization and reset mode.
  The DC cancellation function is used to monitor the audio DC offset and compemsate it.
  Different operation mode has different behavior of DCCAN initialization.

  @param[in] type   DC cancellation selection types. Refer to EAC_DCCAN_TYPE for detail description

  @return void
*/
static void eac_config_dccan(EAC_DCCAN_TYPE type)
{
	// Set the operation mode of DCCAN
	switch (type) {
	case EAC_DCCAN_TYPE_AUTO_RESET: {
			// Set DCCAN Mode as Auto Reset offset value to 0 if eac disabled.
			eac_set_dccan_rst(TRUE);
		}
		break;

	case EAC_DCCAN_TYPE_KEEP: {
			// Set DCCAN Mode as keeping current offset value even if eac disabled.
			eac_set_dccan_rst(FALSE);
		}
		break;

	case EAC_DCCAN_TYPE_RESET_OPTIMAL: {
			eac_set_dccan_rst(TRUE);
			eac_init_dccan_ofs(g_eac_dc_init, dmic2_dc_init);
		}
		break;

	//case EAC_DCCAN_TYPE_KEEP_OPTIMAL:
	default : {
			eac_set_dccan_rst(FALSE);
			eac_init_dccan_ofs(g_eac_dc_init, dmic2_dc_init);
		}
		break;

	}

}


#if 1
/**
    @name Digital to Analog Group APIs
*/

/**
    EAC related CLK API
*/
#ifdef __KERNEL__
void eac_enableclk(BOOL b_en)
{
	struct clk *p_eac_clk;

	p_eac_clk = clk_get(NULL, "f0640000.eac");

	if (IS_ERR(p_eac_clk)) {
		DBG_ERR("failed to get eac clk\n");
	}

	clk_prepare(p_eac_clk);

	if (b_en) {
		clk_enable(p_eac_clk);
	} else {
		clk_disable(p_eac_clk);
	}
	clk_put(p_eac_clk);
}

void eac_enableadcclk(BOOL b_en)
{
	struct clk *p_eac_adc_clk;

	p_eac_adc_clk = clk_get(NULL, "f0640000.eacadc");
	if (IS_ERR(p_eac_adc_clk)) {
		DBG_ERR("failed to get eac-ad clk\n");
	}

	clk_prepare(p_eac_adc_clk);

	if (b_en) {
		clk_enable(p_eac_adc_clk);
	} else {
		clk_disable(p_eac_adc_clk);
	}
	clk_put(p_eac_adc_clk);
}

void eac_enabledacclk(BOOL b_en)
{
	struct clk *p_eac_dac_clk;

	p_eac_dac_clk = clk_get(NULL, "f0640000.eacdac");
	if (IS_ERR(p_eac_dac_clk)) {
		DBG_ERR("failed to get eac-da clk\n");
	}

	clk_prepare(p_eac_dac_clk);

	if (b_en) {
		clk_enable(p_eac_dac_clk);
	} else {
		clk_disable(p_eac_dac_clk);
	}
	clk_put(p_eac_dac_clk);
}

void eac_setdacclkrate(unsigned long clkrate)
{
	struct clk *eacdac_clk;

	eacdac_clk = clk_get(NULL, "f0640000.eacdac");
	if (IS_ERR(eacdac_clk)) {
		DBG_ERR("failed to get eacdac clk\n");
	}
	clk_set_rate(eacdac_clk, clkrate);
	clk_put(eacdac_clk);
}
#else
void eac_enableclk(BOOL b_en)
{
	if(b_en) {
		pll_enable_clock(EAC_D_CLKEN);
	} else {
		pll_disable_clock(EAC_D_CLKEN);
	}

}

void eac_enableadcclk(BOOL b_en)
{
	if(b_en) {
		pll_enable_clock(EAC_A_ADC_CLK);
	} else {
		pll_disable_clock(EAC_A_ADC_CLK);
	}
}

void eac_enabledacclk(BOOL b_en)
{
	if(b_en) {
		pll_enable_clock(EAC_A_DAC_CLK);
	} else {
		pll_disable_clock(EAC_A_DAC_CLK);
	}
}

void eac_setdacclkrate(unsigned long clkrate)
{
//TBD
}
#endif


/**
    Initialize Embedded Audio Codec

    This function is the initialize the default settings of the Embedded Audio Codec module.
    For AD, the ALC is default as disabled. And the DCCAN/NG/Mic_Bias/ZC are default enabled.
    For DA, the ZC is default as enabled.

    @return void
*/

void eac_init_adc(void){
	/*== Analog to Digital Default Values ==*/

	// REG 0x10
	// Default-Enable: MIC_Bias=0 / 12K-Resistor=0 / Boost=0dB / DCCAN=En / Keep_DC_Val / DCCAN-RES=2
	EAC_SETREG(EADC_CTRL0_REG_OFS,      0x10015221);

	// REG 0x14
	// Default DC Init Value
	eac_init_dccan_ofs(g_eac_dc_init, dmic2_dc_init);

	// REG 0x20
	// PGA & D_Gain1 Default Value: 0dB
	EAC_SETREG(EADC_VLMN0_REG_OFS,      0x73730E0E);

	// REG 0x24
	// Default D_Gain2: 0dB
	EAC_SETREG(EADC_VLMN1_REG_OFS,      0x0000C3C3);

	//REG 0x30
	// Default-Enable: NG=En / ZC=En / STEP=5 / ALC-Targer-LR(-12dB) / Bypass-Smooth.
	EAC_SETREG(EALC_CFIG0_REG_OFS,      0x0F0F05C7);

	//RGE 0x34
	// Default-Value for MaxGain(+25.5dB)/ MinGain(-9dB)
	EAC_SETREG(EALC_PGACFIG0_REG_OFS,   0x031C031C);

	//REG 0x38
	// Default-Value for AttTime(0x2)/DecTime(0x3)/HldTime(0x0)/ NgAttTime(0x3)/NgDecTime(0x1)/NgHldTime(0x0)
	EAC_SETREG(EALC_CFIG1_REG_OFS,      0x00340032);

	// REG 0x3C
	// Default ALC Time Resolution: 15ms per unit
	EAC_SETREG(EALC_RESO0_REG_OFS,      0x002FFFFF);

	// REG 0x40
	// Default-Value for NG-Threshold(-69dB) & NG-Factor(0x2)
	EAC_SETREG(EALC_NGCFG0_REG_OFS,     0x010C010C);

	// REG 0x44
	// Default NG Time Resolution: 15ms per unit
	EAC_SETREG(EALC_NGCFG1_REG_OFS,     0x007FFFFF);

	// REG 0x48
	// Default Zero Crossing Timeout Value
	EAC_SETREG(EAC_ZCTC_REG_OFS,        0x00000FFF);

	// PHY Init as Power down
	EAC_SETREG(EAC_PHYCTRL_REG_OFS,     0x000FD400);

	eac_set_load();
}

void eac_init_dac(void){
	/*== Digital to Analog Default Values ==*/

	// REG 0xA4
	// Default Enable DAC ZC and DCCAN-En / DCCAN-Res=3
	EAC_SETREG(EDAC_CTRL0_REG_OFS,      0x03000011);

	// REG 0xA8
	// Default DAC PGA Gain
	//EAC_SETREG(EDAC_PGA_REG_OFS,        0x00000000);

	// REG 0xAC
	// Default DAC Digital Gain: 0dB.
	EAC_SETREG(EDAC_VLMN_REG_OFS,       0x0000C3C3);

	// Set DAC DSM Maximum output swing
	EAC_SETREG(EAC_DACIPCTRL1_REG_OFS,  0x37F006F0);

	/*== Analog PHY Default Values ==*/

	// Set ENB_FCP=0x1. //Reference level to 1.6V
	EAC_SETREG(EAC_DBG0_REG_OFS,        0x00000010);

	// PHY Init as Power down
	EAC_SETREG(EAC_PHYCTRL_REG_OFS,     0x000FD400);

	eac_set_load();
}
void eac_init(void)
{
	/*== Analog to Digital Default Values ==*/

	// REG 0x10
	// Default-Enable: MIC_Bias=0 / 12K-Resistor=0 / Boost=0dB / DCCAN=En / Keep_DC_Val / DCCAN-RES=2
	EAC_SETREG(EADC_CTRL0_REG_OFS,      0x00015000);

	// REG 0x14
	// Default DC Init Value
	eac_init_dccan_ofs(g_eac_dc_init, dmic2_dc_init);

	// REG 0x20
	// PGA & D_Gain1 Default Value: 0dB
	EAC_SETREG(EADC_VLMN0_REG_OFS,      0x73730E0E);

	// REG 0x24
	// Default D_Gain2: 0dB
	EAC_SETREG(EADC_VLMN1_REG_OFS,      0x0000C3C3);

	//REG 0x30
	// Default-Enable: NG=En / ZC=En / STEP=5 / ALC-Targer-LR(-12dB) / Bypass-Smooth.
	EAC_SETREG(EALC_CFIG0_REG_OFS,      0x0B0B05C6);

	//RGE 0x34
	// Default-Value for MaxGain(+25.5dB)/ MinGain(-9dB)
	EAC_SETREG(EALC_PGACFIG0_REG_OFS,   0x081F081F);

	//REG 0x38
	// Default-Value for AttTime(0x2)/DecTime(0x3)/HldTime(0x0)/ NgAttTime(0x3)/NgDecTime(0x1)/NgHldTime(0x0)
	EAC_SETREG(EALC_CFIG1_REG_OFS,      0x00130032);

	// REG 0x3C
	// Default ALC Time Resolution: 15ms per unit
	EAC_SETREG(EALC_RESO0_REG_OFS,      0x002FFFFF);

	// REG 0x40
	// Default-Value for NG-Threshold(-69dB) & NG-Factor(0x2)
	EAC_SETREG(EALC_NGCFG0_REG_OFS,     0x02050205);

	// REG 0x44
	// Default NG Time Resolution: 15ms per unit
	EAC_SETREG(EALC_NGCFG1_REG_OFS,     0x002FFFFF);

	// REG 0x48
	// Default Zero Crossing Timeout Value
	EAC_SETREG(EAC_ZCTC_REG_OFS,        0x00000FFF);



	/*== Digital to Analog Default Values ==*/

	// REG 0xA4
	// Default Enable DAC ZC and DCCAN-En / DCCAN-Res=3
	EAC_SETREG(EDAC_CTRL0_REG_OFS,      0x03000011);

	// REG 0xA8
	// Default DAC PGA Gain
	//EAC_SETREG(EDAC_PGA_REG_OFS,        0x00000000);

	// REG 0xAC
	// Default DAC Digital Gain: 0dB.
	EAC_SETREG(EDAC_VLMN_REG_OFS,       0x0000C3C3);

	// Set DAC DSM Maximum output swing
	EAC_SETREG(EAC_DACIPCTRL1_REG_OFS,  0x37F006F0);

	/*== Analog PHY Default Values ==*/

	// Set ENB_FCP=0x1. //Reference level to 1.6V
	EAC_SETREG(EAC_DBG0_REG_OFS,        0x00000010);

	// PHY Init as Power down
	EAC_SETREG(EAC_PHYCTRL_REG_OFS,     0x0C0FD430);

}

/**
    Set Embedded Audio Codec AD(Record) function Enable/Disable

    This function is used to enable/disable the embedded audio codec recording.

    @param [in] b_en
     - @b TRUE:  Enable AD.
     - @b FALSE: Disable AD.

    @return void
*/
void eac_set_ad_enable(BOOL b_en)
{
	T_EAC_CTRL0_REG eac_ctrl;
	unsigned long flag;

	loc_cpu(flag);

	eac_ctrl.reg = EAC_GETREG(EAC_CTRL0_REG_OFS);
	eac_ctrl.bit.EAC_AD_EN = b_en;
	EAC_SETREG(EAC_CTRL0_REG_OFS, eac_ctrl.reg);

	unl_cpu(flag);
}

/**
    Set Embedded Audio Codec DA(Playback) function Enable/Disable

    This function is used to enable/disable the embedded audio codec playback.

    @param [in] b_en
     - @b TRUE:  Enable DA.
     - @b FALSE: Disable DA.

    @return void
*/
void eac_set_da_enable(BOOL b_en)
{
	T_EAC_CTRL0_REG eac_ctrl;
	unsigned long flag;

	loc_cpu(flag);

	eac_ctrl.reg = EAC_GETREG(EAC_CTRL0_REG_OFS);
	eac_ctrl.bit.EAC_DA_EN = b_en;
	EAC_SETREG(EAC_CTRL0_REG_OFS, eac_ctrl.reg);

	unl_cpu(flag);
}

/**
    Get Embedded Audio Codec AD function Enable/Disable

    This function is used to get the enable/disable of the embedded audio codec AD function.

    @return
     - @b TRUE:  Enabled.
     - @b FALSE: Disabled.
*/
BOOL eac_get_ad_enable(void)
{
	T_EAC_CTRL0_REG reg_ctrl_0;

	reg_ctrl_0.reg = EAC_GETREG(EAC_CTRL0_REG_OFS);
	return reg_ctrl_0.bit.EAC_AD_EN;
}

/**
    Get Embedded Audio Codec DA function Enable/Disable

    This function is used to get the enable/disable of the embedded audio codec DA function.

    @return
     - @b TRUE:  Enabled.
     - @b FALSE: Disabled.
*/
BOOL eac_get_da_enable(void)
{
	T_EAC_CTRL0_REG reg_ctrl_0;

	reg_ctrl_0.reg = EAC_GETREG(EAC_CTRL0_REG_OFS);
	return reg_ctrl_0.bit.EAC_DA_EN;
}

/**
    Load Embedded Audio Codec Dynamic adjustment registers

    Some of the EAC settings have the ability to load new settings dynamically while the EAC is enabled.
    After changing these settings, the user must use this API to activate the new settings.
    These configurations are: EAC_CONFIG_DA_HEADPHONE_GAIN / EAC_CONFIG_DA_LINEOUT_GAIN / EAC_CONFIG_DA_SPEAKER_GAIN / EAC_CONFIG_DA_DGAIN_L /
    EAC_CONFIG_DA_DGAIN_R / EAC_CONFIG_AD_DGAIN1_L / EAC_CONFIG_AD_DGAIN1_R / EAC_CONFIG_AD_DGAIN2_L / EAC_CONFIG_AD_DGAIN2_R / EAC_CONFIG_AD_ALC_TARGET /
    EAC_CONFIG_AD_PGAGAIN_L / EAC_CONFIG_AD_PGAGAIN_R.

    @return void
*/
void eac_set_load(void)
{
	T_EAC_CTRL1_REG     reg_ctrl_1;

	// Set EAC Dynamic Load
	EAC_SETREG(EAC_CTRL1_REG_OFS,  0x1);

	// Polling the Load bit cleared to make sure Load process done
	do {
		reg_ctrl_1.reg = EAC_GETREG(EAC_CTRL1_REG_OFS);
	} while (reg_ctrl_1.bit.LOAD);
}

//@}
#endif

#if 1
/**
    @name Analog to Digital Group APIs
*/
//@{

/**
    Set configuring to EAC AD(Record) Functions

    This api is used to configure the analog to digital(Record) functions of embedded audio codec,
    such as ALC-enable/disable, Recording-target-gain, Noise-Gate, DC-Cancellation, ...etc.

    @param[in] config_id     The configuration selection ID. Please refer to EAC_CONFIG_AD for details.
    @param[in] config_value   The configuration parameter according to the config_id.

    @return void
*/
void eac_set_ad_config(EAC_CONFIG_AD config_id, UINT32 config_value)
{
	T_EADC_CTRL0_REG    adc_ctrl_0;
	T_EADC_VLMN0_REG    adc_volume_0;
	T_EADC_VLMN1_REG    adc_volume_1;
	T_EALC_CFIG0_REG    alc_config_0;
	T_EALC_CFIG1_REG    alc_config_1;
	T_EALC_PGACFIG0_REG pga_clamp;
	T_EALC_NGCFG0_REG   alc_ng_config_0;
	T_EAC_CTRL2_REG     eac_ctrl_2;
	T_EAC_PHYCTRL_REG   phy_ctrl;
	T_DMIC2_VLMN_REG    dmic_vlm;
	unsigned long flag;

	// Lock CPU to prevent race condition
	loc_cpu(flag);

	switch (config_id) {

	/*
	    AD General Control Functions
	*/
	case EAC_CONFIG_AD_DMIC_EN: {
			eac_ctrl_2.reg = EAC_GETREG(EAC_CTRL2_REG_OFS);
			eac_ctrl_2.bit.DMIC_EN  = (config_value > 0);
			EAC_SETREG(EAC_CTRL2_REG_OFS, eac_ctrl_2.reg);
		}
		break;

	case EAC_CONFIG_AD_DMIC_CLK_EN: {
			eac_ctrl_2.reg = EAC_GETREG(EAC_CTRL2_REG_OFS);
			eac_ctrl_2.bit.DMIC_CLK_EN  = (config_value > 0);
			EAC_SETREG(EAC_CTRL2_REG_OFS, eac_ctrl_2.reg);
		}
		break;

	case EAC_CONFIG_AD_DMIC_LRSWAP: {
			eac_ctrl_2.reg = EAC_GETREG(EAC_CTRL2_REG_OFS);
			eac_ctrl_2.bit.DMIC_LR_SWAP  = (config_value > 0);
			EAC_SETREG(EAC_CTRL2_REG_OFS, eac_ctrl_2.reg);
		}
		break;

	case EAC_CONFIG_AD_DMIC2_LRSWAP: {
			eac_ctrl_2.reg = EAC_GETREG(EAC_CTRL2_REG_OFS);
			eac_ctrl_2.bit.DMIC2_LR_SWAP  = (config_value > 0);
			EAC_SETREG(EAC_CTRL2_REG_OFS, eac_ctrl_2.reg);
		}
		break;

	case EAC_CONFIG_AD_DMIC_CHANNEL: {
			eac_ctrl_2.reg = EAC_GETREG(EAC_CTRL2_REG_OFS);
			eac_ctrl_2.bit.ADC_DMIC_CH  = (config_value > 0);
			EAC_SETREG(EAC_CTRL2_REG_OFS, eac_ctrl_2.reg);
		}
		break;

	case EAC_CONFIG_AD_DCCAN_EN: {
			// Set AD Config: DCCAN Function Enable/Disable
			adc_ctrl_0.reg = EAC_GETREG(EADC_CTRL0_REG_OFS);
			adc_ctrl_0.bit.ADC_DCCAN_EN = (config_value > 0);
			EAC_SETREG(EADC_CTRL0_REG_OFS, adc_ctrl_0.reg);
		}
		break;

	case EAC_CONFIG_AD_DCCAN_TYPE: {
			// Unlock the CPU for the following API call.
			unl_cpu(flag);

			// Set AD Config: DCCAN Modes.
			eac_config_dccan(config_value);
		}
		return;

	case EAC_CONFIG_AD_DCCAN_RESO: {
			if (config_value > EAC_DCCAN_RESO_MAX_SAMPLES) {
				config_value = EAC_DCCAN_RESO_MAX_SAMPLES;
			}

			// Set AD Config: DCCAN Resolution as which number samples average.

			adc_ctrl_0.reg = EAC_GETREG(EADC_CTRL0_REG_OFS);
			adc_ctrl_0.bit.ADC_DCCAN_RES = config_value;
			EAC_SETREG(EADC_CTRL0_REG_OFS, adc_ctrl_0.reg);
		}
		break;

	case EAC_CONFIG_AD_DCINIT_L: {
			// Set AD Config: DC Offset Initial Value Left
			g_eac_dc_init &= ~EAC_ADC_DCINIT_MASK;
			g_eac_dc_init |= config_value;
		}
		break;

	case EAC_CONFIG_AD_DCINIT_R: {
			// Set AD Config: DC Offset Initial Value Right
			g_eac_dc_init &= ~(EAC_ADC_DCINIT_MASK << EAC_ADC_DCINIT_R_OFS);
			g_eac_dc_init |= (config_value << EAC_ADC_DCINIT_R_OFS);
		}
		break;

	case EAC_CONFIG_AD_DMIC2_DCINIT_L: {
			// Set AD Config: DC Offset Initial Value Left dmic2
			dmic2_dc_init &= ~EAC_ADC_DCINIT_MASK;
			dmic2_dc_init |= config_value;
		}
		break;

	case EAC_CONFIG_AD_DMIC2_DCINIT_R: {
			// Set AD Config: DC Offset Initial Value Right dmic2
			dmic2_dc_init &= ~(EAC_ADC_DCINIT_MASK << EAC_ADC_DCINIT_R_OFS);
			dmic2_dc_init |= (config_value << EAC_ADC_DCINIT_R_OFS);
		}
		break;

	case EAC_CONFIG_AD_DGAIN1_L: {
			if (config_value > EAC_ADC_DGAIN_MAX) {
				config_value = EAC_ADC_DGAIN_MAX;
			}

			// Set AD Config: Digital Gain 1 Left
			adc_volume_0.reg = EAC_GETREG(EADC_VLMN0_REG_OFS);
			adc_volume_0.bit.ADC_DGAIN_LEFT  = config_value;
			EAC_SETREG(EADC_VLMN0_REG_OFS, adc_volume_0.reg);
		}
		break;

	case EAC_CONFIG_AD_DGAIN1_R: {
			if (config_value > EAC_ADC_DGAIN_MAX) {
				config_value = EAC_ADC_DGAIN_MAX;
			}

			// Set AD Config: Digital Gain 1 Right
			adc_volume_0.reg = EAC_GETREG(EADC_VLMN0_REG_OFS);
			adc_volume_0.bit.ADC_DGAIN_RIGHT = config_value;
			EAC_SETREG(EADC_VLMN0_REG_OFS, adc_volume_0.reg);
		}
		break;

	case EAC_CONFIG_AD_DGAIN2_L: {
			if (config_value > EAC_ADC_DGAIN_MAX) {
				config_value = EAC_ADC_DGAIN_MAX;
			}

			// Set AD Config: Digital Gain 2 Left
			adc_volume_1.reg = EAC_GETREG(EADC_VLMN1_REG_OFS);
			adc_volume_1.bit.ADC_DGAIN2_LEFT  = config_value;
			EAC_SETREG(EADC_VLMN1_REG_OFS, adc_volume_1.reg);
		}
		break;

	case EAC_CONFIG_AD_DGAIN2_R: {
			if (config_value > EAC_ADC_DGAIN_MAX) {
				config_value = EAC_ADC_DGAIN_MAX;
			}

			// Set AD Config: Digital Gain 2 Right
			adc_volume_1.reg = EAC_GETREG(EADC_VLMN1_REG_OFS);
			adc_volume_1.bit.ADC_DGAIN2_RIGHT = config_value;
			EAC_SETREG(EADC_VLMN1_REG_OFS, adc_volume_1.reg);
		}
		break;

	case EAC_CONFIG_AD_DMIC2_DGAIN_L: {
			if (config_value > EAC_ADC_DGAIN_MAX) {
				config_value = EAC_ADC_DGAIN_MAX;
			}

			// Set AD Config: dmic2 digital gain left
			dmic_vlm.reg = EAC_GETREG(DMIC2_VLMN_REG_OFS);
			dmic_vlm.bit.DMIC2_DGAIN_LEFT  = config_value;
			EAC_SETREG(DMIC2_VLMN_REG_OFS, dmic_vlm.reg);
		}
		break;

	case EAC_CONFIG_AD_DMIC2_DGAIN_R: {
			if (config_value > EAC_ADC_DGAIN_MAX) {
				config_value = EAC_ADC_DGAIN_MAX;
			}

			// Set AD Config: dmic2 digital gain right
			dmic_vlm.reg = EAC_GETREG(DMIC2_VLMN_REG_OFS);
			dmic_vlm.bit.DMIC2_DGAIN_RIGHT = config_value;
			EAC_SETREG(DMIC2_VLMN_REG_OFS, dmic_vlm.reg);
		}
		break;


	/*
	    ALC Related Control Functions
	*/
	case EAC_CONFIG_AD_ALC_EN: {
			// Set AD Config: Auto-Level-Control Function Enable/Disable
			alc_config_0.reg = EAC_GETREG(EALC_CFIG0_REG_OFS);
			alc_config_0.bit.ADC_ALC_EN = (config_value > 0);
			EAC_SETREG(EALC_CFIG0_REG_OFS, alc_config_0.reg);
		}
		break;

	case EAC_CONFIG_AD_ALC_TARGET_L: {
			if (config_value > EAC_ALC_TARGET_MAX) {
				config_value = EAC_ALC_TARGET_MAX;
			}

			// Set AD Config: Auto-Level-Control Target Level
			alc_config_0.reg = EAC_GETREG(EALC_CFIG0_REG_OFS);
			alc_config_0.bit.ALC_TARGET_L = config_value;
			EAC_SETREG(EALC_CFIG0_REG_OFS, alc_config_0.reg);
		}
		break;

	case EAC_CONFIG_AD_ALC_TARGET_R: {
			if (config_value > EAC_ALC_TARGET_MAX) {
				config_value = EAC_ALC_TARGET_MAX;
			}

			// Set AD Config: Auto-Level-Control Target Level
			alc_config_0.reg = EAC_GETREG(EALC_CFIG0_REG_OFS);
			alc_config_0.bit.ALC_TARGET_R = config_value;
			EAC_SETREG(EALC_CFIG0_REG_OFS, alc_config_0.reg);
		}
		break;

	case EAC_CONFIG_AD_ALC_MAXGAIN_L: {
			if (config_value > EAC_ALC_MAXGAIN_MAX) {
				config_value = EAC_ALC_MAXGAIN_MAX;
			}

			// Set AD Config: Auto-Level-Control Max Allowed PGA Gain
			pga_clamp.reg = EAC_GETREG(EALC_PGACFIG0_REG_OFS);
			pga_clamp.bit.ALC_MAXGAIN_L = config_value;
			EAC_SETREG(EALC_PGACFIG0_REG_OFS, pga_clamp.reg);
		}
		break;

	case EAC_CONFIG_AD_ALC_MAXGAIN_R: {
			if (config_value > EAC_ALC_MAXGAIN_MAX) {
				config_value = EAC_ALC_MAXGAIN_MAX;
			}

			// Set AD Config: Auto-Level-Control Max Allowed PGA Gain
			pga_clamp.reg = EAC_GETREG(EALC_PGACFIG0_REG_OFS);
			pga_clamp.bit.ALC_MAXGAIN_R = config_value;
			EAC_SETREG(EALC_PGACFIG0_REG_OFS, pga_clamp.reg);
		}
		break;

	case EAC_CONFIG_AD_ALC_MINGAIN_L: {
			if (config_value > EAC_ALC_MINGAIN_MAX) {
				config_value = EAC_ALC_MINGAIN_MAX;
			}

			// Set AD Config: Auto-Level-Control Min Allowed PGA Gain
			pga_clamp.reg = EAC_GETREG(EALC_PGACFIG0_REG_OFS);
			pga_clamp.bit.ALC_MINGAIN_L = config_value;
			EAC_SETREG(EALC_PGACFIG0_REG_OFS, pga_clamp.reg);
		}
		break;

	case EAC_CONFIG_AD_ALC_MINGAIN_R: {
			if (config_value > EAC_ALC_MINGAIN_MAX) {
				config_value = EAC_ALC_MINGAIN_MAX;
			}

			// Set AD Config: Auto-Level-Control Min Allowed PGA Gain
			pga_clamp.reg = EAC_GETREG(EALC_PGACFIG0_REG_OFS);
			pga_clamp.bit.ALC_MINGAIN_R = config_value;
			EAC_SETREG(EALC_PGACFIG0_REG_OFS, pga_clamp.reg);
		}
		break;

	case EAC_CONFIG_AD_ALC_ATTACK_TIME: {
			if (config_value > EAC_ALC_ATTACK_MAX) {
				config_value = EAC_ALC_ATTACK_MAX;
			}

			// Set AD Config: Auto-Level-Control Attack Time
			alc_config_1.reg = EAC_GETREG(EALC_CFIG1_REG_OFS);
			alc_config_1.bit.ALC_ATTACK_TIME = config_value;
			EAC_SETREG(EALC_CFIG1_REG_OFS, alc_config_1.reg);
		}
		break;

	case EAC_CONFIG_AD_ALC_DECAY_TIME: {
			if (config_value > EAC_ALC_DECAY_MAX) {
				config_value = EAC_ALC_DECAY_MAX;
			}

			// Set AD Config: Auto-Level-Control Decay Time
			alc_config_1.reg = EAC_GETREG(EALC_CFIG1_REG_OFS);
			alc_config_1.bit.ALC_DECAY_TIME = config_value;
			EAC_SETREG(EALC_CFIG1_REG_OFS, alc_config_1.reg);
		}
		break;

	case EAC_CONFIG_AD_ALC_HOLD_TIME: {
			if (config_value > EAC_ALC_HOLD_MAX) {
				config_value = EAC_ALC_HOLD_MAX;
			}

			// Set AD Config: Auto-Level-Control Hold Time
			alc_config_1.reg = EAC_GETREG(EALC_CFIG1_REG_OFS);
			alc_config_1.bit.ALC_HOLD_TIME = config_value;
			EAC_SETREG(EALC_CFIG1_REG_OFS, alc_config_1.reg);
		}
		break;

	case EAC_CONFIG_AD_NG_ATTACK_TIME: {
			if (config_value > EAC_ALC_ATTACK_MAX) {
				config_value = EAC_ALC_ATTACK_MAX;
			}

			// Set AD Config: Auto-Level-Control Attack Time
			alc_config_1.reg = EAC_GETREG(EALC_CFIG1_REG_OFS);
			alc_config_1.bit.NG_ATTACK_TIME = config_value;
			EAC_SETREG(EALC_CFIG1_REG_OFS, alc_config_1.reg);
		}
		break;

	case EAC_CONFIG_AD_NG_DECAY_TIME: {
			if (config_value > EAC_ALC_DECAY_MAX) {
				config_value = EAC_ALC_DECAY_MAX;
			}

			// Set AD Config: Auto-Level-Control Decay Time
			alc_config_1.reg = EAC_GETREG(EALC_CFIG1_REG_OFS);
			alc_config_1.bit.NG_DECAY_TIME = config_value;
			EAC_SETREG(EALC_CFIG1_REG_OFS, alc_config_1.reg);
		}
		break;

	case EAC_CONFIG_AD_NG_HOLD_TIME: {
			if (config_value > EAC_ALC_HOLD_MAX) {
				config_value = EAC_ALC_HOLD_MAX;
			}

			// Set AD Config: Auto-Level-Control Hold Time
			alc_config_1.reg = EAC_GETREG(EALC_CFIG1_REG_OFS);
			alc_config_1.bit.NG_HOLD_TIME = config_value;
			EAC_SETREG(EALC_CFIG1_REG_OFS, alc_config_1.reg);
		}
		break;

	case EAC_CONFIG_AD_ALC_STEP: {
			if (config_value > EAC_ALC_STEP_MAX) {
				config_value = EAC_ALC_STEP_MAX;
			}

			// Set AD Config: Config the ALC Max Step Distance.
			alc_config_0.reg = EAC_GETREG(EALC_CFIG0_REG_OFS);
			alc_config_0.bit.ALC_STEP = config_value;
			EAC_SETREG(EALC_CFIG0_REG_OFS, alc_config_0.reg);
		}
		break;

	case EAC_CONFIG_AD_ALC_TRESO: {
			if (config_value > EAC_ALC_TIME_RESO_MAX) {
				config_value = EAC_ALC_TIME_RESO_MAX;
			}

			// Set AD Config: ALC Basic Time Resolution for Attack/Decay Time.
			EAC_SETREG(EALC_RESO0_REG_OFS, config_value);
		}
		break;

	case EAC_CONFIG_AD_ALC_NG_EN: {
			// Set AD Config: ALC Noise Gate Enable/Disable
			alc_config_0.reg = EAC_GETREG(EALC_CFIG0_REG_OFS);
			alc_config_0.bit.ADC_NOISE_GATE_EN = (config_value > 0);
			EAC_SETREG(EALC_CFIG0_REG_OFS, alc_config_0.reg);
		}
		break;

	case EAC_CONFIG_AD_ALC_NG_THD_L: {
			if (config_value > EAC_NG_THRESHOLD_MAX) {
				config_value = EAC_NG_THRESHOLD_MAX;
			}

			// Set AD Config: ALC Noise Gate Threshold Value
			alc_ng_config_0.reg = EAC_GETREG(EALC_NGCFG0_REG_OFS);
			alc_ng_config_0.bit.NG_THRESHOLD_L = config_value;
			EAC_SETREG(EALC_NGCFG0_REG_OFS, alc_ng_config_0.reg);
		}
		break;

	case EAC_CONFIG_AD_ALC_NG_THD_R: {
			if (config_value > EAC_NG_THRESHOLD_MAX) {
				config_value = EAC_NG_THRESHOLD_MAX;
			}

			// Set AD Config: ALC Noise Gate Threshold Value
			alc_ng_config_0.reg = EAC_GETREG(EALC_NGCFG0_REG_OFS);
			alc_ng_config_0.bit.NG_THRESHOLD_R = config_value;
			EAC_SETREG(EALC_NGCFG0_REG_OFS, alc_ng_config_0.reg);
		}
		break;

	case EAC_CONFIG_AD_ALC_NG_TARGET_L: {
			if (config_value > EAC_NG_FACTOR_MAX) {
				config_value = EAC_NG_FACTOR_MAX;
			}

			// Set AD Config: Configure the ALC Noise Gate Decay Speed(Slope).
			alc_ng_config_0.reg = EAC_GETREG(EALC_NGCFG0_REG_OFS);
			alc_ng_config_0.bit.NG_TARGET_L = config_value;
			EAC_SETREG(EALC_NGCFG0_REG_OFS, alc_ng_config_0.reg);
		}
		break;

	case EAC_CONFIG_AD_ALC_NG_TARGET_R: {
			if (config_value > EAC_NG_FACTOR_MAX) {
				config_value = EAC_NG_FACTOR_MAX;
			}

			// Set AD Config: Configure the ALC Noise Gate Decay Speed(Slope).
			alc_ng_config_0.reg = EAC_GETREG(EALC_NGCFG0_REG_OFS);
			alc_ng_config_0.bit.NG_TARGET_R = config_value;
			EAC_SETREG(EALC_NGCFG0_REG_OFS, alc_ng_config_0.reg);
		}
		break;

	case EAC_CONFIG_AD_ALC_NG_TRESO: {
			if (config_value > EAC_ALC_TIME_RESO_MAX) {
				config_value = EAC_ALC_TIME_RESO_MAX;
			}

			// Set AD Config: Configure the Noise Gate Basic Time Resolution for Attack/Decay Time in NoiseGate.
			EAC_SETREG(EALC_NGCFG1_REG_OFS, config_value);
		}
		break;

	case EAC_CONFIG_AD_BOOST_COMPEN_L: {
			// Set AD Config: set the alc boost compensation gain for left channel
			alc_config_0.reg = EAC_GETREG(EALC_CFIG0_REG_OFS);
			alc_config_0.bit.ALC_BOOST_COMPEN_L = config_value;
			EAC_SETREG(EALC_CFIG0_REG_OFS, alc_config_0.reg);
		}
		break;

	case EAC_CONFIG_AD_BOOST_COMPEN_R: {
			// Set AD Config: set the alc boost compensation gain for right channel
			alc_config_0.reg = EAC_GETREG(EALC_CFIG0_REG_OFS);
			alc_config_0.bit.ALC_BOOST_COMPEN_R = config_value;
			EAC_SETREG(EALC_CFIG0_REG_OFS, alc_config_0.reg);
		}
		break;

	case EAC_CONFIG_AD_IIR_ALC_L: {
			// Set AD Config: Config the ALC Left Channel Input sample with IIR or not.
			alc_config_0.reg = EAC_GETREG(EALC_CFIG0_REG_OFS);
			alc_config_0.bit.ALC_IIR_SEL_L = (config_value > 0);
			EAC_SETREG(EALC_CFIG0_REG_OFS, alc_config_0.reg);
		}
		break;

	case EAC_CONFIG_AD_IIR_ALC_R: {
			// Set AD Config: Config the ALC Right Channel Input sample with IIR or not.
			alc_config_0.reg = EAC_GETREG(EALC_CFIG0_REG_OFS);
			alc_config_0.bit.ALC_IIR_SEL_R = (config_value > 0);
			EAC_SETREG(EALC_CFIG0_REG_OFS, alc_config_0.reg);
		}
		break;

	case EAC_CONFIG_AD_IIR_OUT_L: {
			// Set AD Config: IIR Function Enable/Disable for recorded output
			adc_ctrl_0.reg = EAC_GETREG(EADC_CTRL0_REG_OFS);
			adc_ctrl_0.bit.ADCOUT_IIR_SEL_L = (config_value > 0);
			EAC_SETREG(EADC_CTRL0_REG_OFS, adc_ctrl_0.reg);
		}
		break;

	case EAC_CONFIG_AD_IIR_OUT_R: {
			// Set AD Config: IIR Function Enable/Disable for recorded output
			adc_ctrl_0.reg = EAC_GETREG(EADC_CTRL0_REG_OFS);
			adc_ctrl_0.bit.ADCOUT_IIR_SEL_R = (config_value > 0);
			EAC_SETREG(EADC_CTRL0_REG_OFS, adc_ctrl_0.reg);
		}
		break;

	case EAC_CONFIG_AD_IIR2_OUT_L: {
			// Set AD Config: IIR Function Enable/Disable for recorded output
			adc_ctrl_0.reg = EAC_GETREG(EADC_CTRL0_REG_OFS);
			adc_ctrl_0.bit.ADCOUT_IIR2_SEL_L = (config_value > 0);
			EAC_SETREG(EADC_CTRL0_REG_OFS, adc_ctrl_0.reg);
		}
		break;

	case EAC_CONFIG_AD_IIR2_OUT_R: {
			// Set AD Config: IIR Function Enable/Disable for recorded output
			adc_ctrl_0.reg = EAC_GETREG(EADC_CTRL0_REG_OFS);
			adc_ctrl_0.bit.ADCOUT_IIR2_SEL_R = (config_value > 0);
			EAC_SETREG(EADC_CTRL0_REG_OFS, adc_ctrl_0.reg);
		}
		break;


	/*
	    AD Analog Related Control Functions
	*/
	case EAC_CONFIG_AD_PGAGAIN_L: {
			if (config_value > EAC_AD_PGAGAIN_MAX) {
				config_value = EAC_AD_PGAGAIN_MAX;
			}

			// Set AD Config: Left Channel PGA Amplifier Gain
			adc_volume_0.reg = EAC_GETREG(EADC_VLMN0_REG_OFS);
			adc_volume_0.bit.ADC_PGA_GAIN_LEFT  = config_value;
			EAC_SETREG(EADC_VLMN0_REG_OFS, adc_volume_0.reg);
		}
		break;

	case EAC_CONFIG_AD_PGAGAIN_R: {
			if (config_value > EAC_AD_PGAGAIN_MAX) {
				config_value = EAC_AD_PGAGAIN_MAX;
			}

			// Set AD Config: Right Channel PGA Amplifier Gain
			adc_volume_0.reg = EAC_GETREG(EADC_VLMN0_REG_OFS);
			adc_volume_0.bit.ADC_PGA_GAIN_RIGHT = config_value;
			EAC_SETREG(EADC_VLMN0_REG_OFS, adc_volume_0.reg);
		}
		break;

	case EAC_CONFIG_AD_PGABOOST_L: {
			// Set AD Config: Select 10/20/30 dB Boost Gain.
			adc_ctrl_0.reg = EAC_GETREG(EADC_CTRL0_REG_OFS);
			adc_ctrl_0.bit.ADC_ANALOG_BOOST_L = config_value;
			EAC_SETREG(EADC_CTRL0_REG_OFS, adc_ctrl_0.reg);
		}
		break;

	case EAC_CONFIG_AD_PGABOOST_R: {
			// Set AD Config: Select 10/20/30 dB Boost Gain.
			adc_ctrl_0.reg = EAC_GETREG(EADC_CTRL0_REG_OFS);
			adc_ctrl_0.bit.ADC_ANALOG_BOOST_R = config_value;
			EAC_SETREG(EADC_CTRL0_REG_OFS, adc_ctrl_0.reg);
		}
		break;

	case EAC_CONFIG_AD_POWER_EN_L: {
			// Set AD Config: Left Channel Analog Block Power Enable/Disable
			eac_ctrl_2.reg = EAC_GETREG(EAC_CTRL2_REG_OFS);
			eac_ctrl_2.bit.ADC_ANALOG_LEFT_POW  = (config_value > 0);
			EAC_SETREG(EAC_CTRL2_REG_OFS, eac_ctrl_2.reg);
		}
		break;

	case EAC_CONFIG_AD_POWER_EN_R: {
			// Set AD Config: Right Channel Analog Block Power Enable/Disable
			eac_ctrl_2.reg = EAC_GETREG(EAC_CTRL2_REG_OFS);
			eac_ctrl_2.bit.ADC_ANALOG_RIGHT_POW = (config_value > 0);
			EAC_SETREG(EAC_CTRL2_REG_OFS, eac_ctrl_2.reg);
		}
		break;

	case EAC_CONFIG_AD_RESET: {
			// Set AD Config: Analog Block reset enable/disable.
			eac_ctrl_2.reg = EAC_GETREG(EAC_CTRL2_REG_OFS);
			eac_ctrl_2.bit.ADC_ANALOG_RST_L  = (config_value > 0);
			eac_ctrl_2.bit.ADC_ANALOG_RST_R  = (config_value > 0);
#if 0//_FPGA_EMULATION_
			eac_ctrl_2.bit.DAC_ANALOG_RST  = (config_value > 0);
#endif
			EAC_SETREG(EAC_CTRL2_REG_OFS, eac_ctrl_2.reg);
		}
		break;

	case EAC_CONFIG_AD_PDREF_BUF: {
			eac_ctrl_2.reg = EAC_GETREG(EAC_CTRL2_REG_OFS);
			eac_ctrl_2.bit.PDREF_BUF  = (config_value > 0);
			EAC_SETREG(EAC_CTRL2_REG_OFS, eac_ctrl_2.reg);
		}
		break;

	case EAC_CONFIG_AD_PDREF_BIAS: {
			eac_ctrl_2.reg = EAC_GETREG(EAC_CTRL2_REG_OFS);
			eac_ctrl_2.bit.PDREF_BIAS  = (config_value > 0);
			EAC_SETREG(EAC_CTRL2_REG_OFS, eac_ctrl_2.reg);
		}
		break;

	case EAC_CONFIG_AD_MICBIAS_EN: {
			adc_ctrl_0.reg = EAC_GETREG(EADC_CTRL0_REG_OFS);
			adc_ctrl_0.bit.ADC_MIC_BIAS_EN = (config_value > 0);
			EAC_SETREG(EADC_CTRL0_REG_OFS, adc_ctrl_0.reg);
		}
		break;

	case EAC_CONFIG_AD_MICBIAS_LVL: {
			adc_ctrl_0.reg = EAC_GETREG(EADC_CTRL0_REG_OFS);
			adc_ctrl_0.bit.ADC_MIC_BIAS_LVL = (config_value > 0);
			EAC_SETREG(EADC_CTRL0_REG_OFS, adc_ctrl_0.reg);
		}
		break;
	case EAC_CONFIG_AD_PD_VCMBIAS: {
			phy_ctrl.reg = EAC_GETREG(EAC_PHYCTRL_REG_OFS);
			phy_ctrl.bit.PD_BIAS = (config_value > 0);
			EAC_SETREG(EAC_PHYCTRL_REG_OFS, phy_ctrl.reg);
		}
		break;

	default: {
			// Unlock CPU first for the following debug msg
			unl_cpu(flag);

			DBG_WRN("Set Cfg ID Err!\r\n");
		}
		return;

	}

	// Un-Lock CPU
	unl_cpu(flag);

}

/**
    Get configuration of EAC AD(Record) Functions

    This api is used to get the analog to digital(Record) configurations of embedded audio codec,
    such as ALC-enable/disable, Recording-target-gain, Noise-Gate, DC-Cancellation, ...etc.

    @param[in] config_id     The configuration selection ID. Please refer to EAC_CONFIG_AD for details.
     - @b EAC_CONFIG_AD_ALC_EN: Get the ALC Enable/Disable.
     - @b EAC_CONFIG_AD_RESET:  Get the AD Analog Block is set reset or not.
     - @b Others:               Have not implemented. Always return 0x0.

    @return The configuration parameter according to the config_id.
*/
UINT32 eac_get_ad_config(EAC_CONFIG_AD config_id)
{
	UINT32              ret = 0;

	switch (config_id) {

	case EAC_CONFIG_AD_DMIC_EN: {
			T_EAC_CTRL2_REG     eac_ctrl_2;
			eac_ctrl_2.reg = EAC_GETREG(EAC_CTRL2_REG_OFS);
			ret = eac_ctrl_2.bit.DMIC_EN;
		}
		break;

	case EAC_CONFIG_AD_DMIC_CLK_EN: {
			T_EAC_CTRL2_REG     eac_ctrl_2;
			eac_ctrl_2.reg = EAC_GETREG(EAC_CTRL2_REG_OFS);
			ret = eac_ctrl_2.bit.DMIC_CLK_EN;
		}
		break;

	case EAC_CONFIG_AD_DMIC_LRSWAP: {
			T_EAC_CTRL2_REG     eac_ctrl_2;
			eac_ctrl_2.reg = EAC_GETREG(EAC_CTRL2_REG_OFS);
			ret = eac_ctrl_2.bit.DMIC_LR_SWAP;
		}
		break;

	case EAC_CONFIG_AD_DMIC2_LRSWAP: {
			T_EAC_CTRL2_REG     eac_ctrl_2;
			eac_ctrl_2.reg = EAC_GETREG(EAC_CTRL2_REG_OFS);
			ret = eac_ctrl_2.bit.DMIC2_LR_SWAP;
		}
		break;

	case EAC_CONFIG_AD_DMIC_CHANNEL: {
			T_EAC_CTRL2_REG     eac_ctrl_2;
			eac_ctrl_2.reg = EAC_GETREG(EAC_CTRL2_REG_OFS);
			ret = eac_ctrl_2.bit.ADC_DMIC_CH;
		}
		break;

	case EAC_CONFIG_AD_ALC_MODE_DGAIN: {
			T_EAC_CTRL2_REG     eac_ctrl_2;
			eac_ctrl_2.reg = EAC_GETREG(EAC_CTRL2_REG_OFS);
			ret = eac_ctrl_2.bit.ADC_ALC_MODE;
		}
		break;

	case EAC_CONFIG_AD_ALC_EN: {
			T_EALC_CFIG0_REG    alc_config_0;

			alc_config_0.reg = EAC_GETREG(EALC_CFIG0_REG_OFS);
			ret = alc_config_0.bit.ADC_ALC_EN;
		}
		break;

	case EAC_CONFIG_AD_ALC_TARGET_L: {
			T_EALC_CFIG0_REG    alc_config_0;

			alc_config_0.reg = EAC_GETREG(EALC_CFIG0_REG_OFS);
			ret = alc_config_0.bit.ALC_TARGET_L;
		}
		break;

	case EAC_CONFIG_AD_ALC_TARGET_R: {
			T_EALC_CFIG0_REG    alc_config_0;

			alc_config_0.reg = EAC_GETREG(EALC_CFIG0_REG_OFS);
			ret = alc_config_0.bit.ALC_TARGET_R;
		}
		break;

	case EAC_CONFIG_AD_ALC_MAXGAIN_L: {
			T_EALC_PGACFIG0_REG    pga_clamp;

			pga_clamp.reg = EAC_GETREG(EALC_PGACFIG0_REG_OFS);
			ret = pga_clamp.bit.ALC_MAXGAIN_L;
		}
		break;

	case EAC_CONFIG_AD_ALC_MAXGAIN_R: {
			T_EALC_PGACFIG0_REG    pga_clamp;

			pga_clamp.reg = EAC_GETREG(EALC_PGACFIG0_REG_OFS);
			ret = pga_clamp.bit.ALC_MAXGAIN_R;
		}
		break;

	case EAC_CONFIG_AD_ALC_MINGAIN_L: {
			T_EALC_PGACFIG0_REG    pga_clamp;

			pga_clamp.reg = EAC_GETREG(EALC_PGACFIG0_REG_OFS);
			ret = pga_clamp.bit.ALC_MINGAIN_L;
		}
		break;

	case EAC_CONFIG_AD_ALC_MINGAIN_R: {
			T_EALC_PGACFIG0_REG    pga_clamp;

			pga_clamp.reg = EAC_GETREG(EALC_PGACFIG0_REG_OFS);
			ret = pga_clamp.bit.ALC_MINGAIN_R;
		}
		break;

	case EAC_CONFIG_AD_ALC_TRESO: {
			ret = EAC_GETREG(EALC_RESO0_REG_OFS);
		}
		break;

	case EAC_CONFIG_AD_ALC_ATTACK_TIME: {
			T_EALC_CFIG1_REG    alc_config_1;

			alc_config_1.reg = EAC_GETREG(EALC_CFIG1_REG_OFS);
			ret = alc_config_1.bit.ALC_ATTACK_TIME;
		}
		break;

	case EAC_CONFIG_AD_ALC_DECAY_TIME: {
			T_EALC_CFIG1_REG    alc_config_1;

			alc_config_1.reg = EAC_GETREG(EALC_CFIG1_REG_OFS);
			ret = alc_config_1.bit.ALC_DECAY_TIME;
		}
		break;

	case EAC_CONFIG_AD_ALC_HOLD_TIME: {
			T_EALC_CFIG1_REG    alc_config_1;

			alc_config_1.reg = EAC_GETREG(EALC_CFIG1_REG_OFS);
			ret = alc_config_1.bit.ALC_HOLD_TIME;
		}
		break;

	case EAC_CONFIG_AD_NG_ATTACK_TIME: {
			T_EALC_CFIG1_REG    alc_config_1;

			alc_config_1.reg = EAC_GETREG(EALC_CFIG1_REG_OFS);
			ret = alc_config_1.bit.NG_ATTACK_TIME;
		}
		break;

	case EAC_CONFIG_AD_NG_DECAY_TIME: {
			T_EALC_CFIG1_REG    alc_config_1;

			alc_config_1.reg = EAC_GETREG(EALC_CFIG1_REG_OFS);
			ret = alc_config_1.bit.NG_DECAY_TIME;
		}
		break;

	case EAC_CONFIG_AD_NG_HOLD_TIME: {
			T_EALC_CFIG1_REG    alc_config_1;

			alc_config_1.reg = EAC_GETREG(EALC_CFIG1_REG_OFS);
			ret = alc_config_1.bit.NG_HOLD_TIME;
		}
		break;

	case EAC_CONFIG_AD_ALC_STEP: {
			T_EALC_CFIG0_REG    alc_config_0;

			alc_config_0.reg = EAC_GETREG(EALC_CFIG0_REG_OFS);
			ret = alc_config_0.bit.ALC_STEP;
		}
		break;

	case EAC_CONFIG_AD_ALC_NG_EN: {
			T_EALC_CFIG0_REG    alc_config_0;

			alc_config_0.reg = EAC_GETREG(EALC_CFIG0_REG_OFS);
			ret = alc_config_0.bit.ADC_NOISE_GATE_EN;
		}
		break;

	case EAC_CONFIG_AD_ALC_NG_THD_L: {
			T_EALC_NGCFG0_REG   alc_ng_config_0;

			alc_ng_config_0.reg = EAC_GETREG(EALC_NGCFG0_REG_OFS);
			ret = alc_ng_config_0.bit.NG_THRESHOLD_L;
		}
		break;

	case EAC_CONFIG_AD_ALC_NG_THD_R: {
			T_EALC_NGCFG0_REG   alc_ng_config_0;

			alc_ng_config_0.reg = EAC_GETREG(EALC_NGCFG0_REG_OFS);
			ret = alc_ng_config_0.bit.NG_THRESHOLD_R;
		}
		break;

	case EAC_CONFIG_AD_ALC_NG_TARGET_L: {
			T_EALC_NGCFG0_REG   alc_ng_config_0;

			alc_ng_config_0.reg = EAC_GETREG(EALC_NGCFG0_REG_OFS);
			ret = alc_ng_config_0.bit.NG_TARGET_L;
		}
		break;

	case EAC_CONFIG_AD_ALC_NG_TARGET_R: {
			T_EALC_NGCFG0_REG   alc_ng_config_0;

			alc_ng_config_0.reg = EAC_GETREG(EALC_NGCFG0_REG_OFS);
			ret = alc_ng_config_0.bit.NG_TARGET_R;
		}
		break;

	case EAC_CONFIG_AD_ALC_NG_TRESO: {
			ret = EAC_GETREG(EALC_NGCFG1_REG_OFS);
		}
		break;

	case EAC_CONFIG_AD_RESET: {
			T_EAC_CTRL2_REG     eac_ctrl_2;

			eac_ctrl_2.reg = EAC_GETREG(EAC_CTRL2_REG_OFS);
			ret = eac_ctrl_2.bit.ADC_ANALOG_RST_L;
		}
		break;

	case EAC_CONFIG_AD_PGABOOST_L: {
			T_EADC_CTRL0_REG    adc_ctrl_0;

			adc_ctrl_0.reg = EAC_GETREG(EADC_CTRL0_REG_OFS);
			ret = adc_ctrl_0.bit.ADC_ANALOG_BOOST_L;
		}
		break;

	case EAC_CONFIG_AD_PGABOOST_R: {
			T_EADC_CTRL0_REG    adc_ctrl_0;

			adc_ctrl_0.reg = EAC_GETREG(EADC_CTRL0_REG_OFS);
			ret = adc_ctrl_0.bit.ADC_ANALOG_BOOST_L;
		}
		break;

	case EAC_CONFIG_AD_POWER_EN_L: {
			T_EAC_CTRL2_REG     eac_ctrl_2;

			eac_ctrl_2.reg = EAC_GETREG(EAC_CTRL2_REG_OFS);
			ret = eac_ctrl_2.bit.ADC_ANALOG_LEFT_POW;
		}
		break;

	case EAC_CONFIG_AD_POWER_EN_R: {
			T_EAC_CTRL2_REG     eac_ctrl_2;

			eac_ctrl_2.reg = EAC_GETREG(EAC_CTRL2_REG_OFS);
			ret = eac_ctrl_2.bit.ADC_ANALOG_RIGHT_POW;
		}
		break;

	case EAC_CONFIG_AD_IIR_OUT_L: {
			T_EADC_CTRL0_REG    adc_ctrl_0;

			adc_ctrl_0.reg = EAC_GETREG(EADC_CTRL0_REG_OFS);
			ret = adc_ctrl_0.bit.ADCOUT_IIR_SEL_L;
		}
		break;

	case EAC_CONFIG_AD_IIR_OUT_R: {
			T_EADC_CTRL0_REG    adc_ctrl_0;

			adc_ctrl_0.reg = EAC_GETREG(EADC_CTRL0_REG_OFS);
			ret = adc_ctrl_0.bit.ADCOUT_IIR_SEL_R;
		}
		break;

	case EAC_CONFIG_AD_IIR2_OUT_L: {
			T_EADC_CTRL0_REG    adc_ctrl_0;

			adc_ctrl_0.reg = EAC_GETREG(EADC_CTRL0_REG_OFS);
			ret = adc_ctrl_0.bit.ADCOUT_IIR2_SEL_L;
		}
		break;

	case EAC_CONFIG_AD_IIR2_OUT_R: {
			T_EADC_CTRL0_REG    adc_ctrl_0;

			adc_ctrl_0.reg = EAC_GETREG(EADC_CTRL0_REG_OFS);
			ret = adc_ctrl_0.bit.ADCOUT_IIR2_SEL_R;
		}
		break;

	case EAC_CONFIG_AD_PGAGAIN_L: {
			T_EADC_VLMN0_REG    adc_volume_0;

			adc_volume_0.reg = EAC_GETREG(EADC_VLMN0_REG_OFS);
			ret = adc_volume_0.bit.ADC_PGA_GAIN_LEFT;
		}
		break;

	case EAC_CONFIG_AD_PGAGAIN_R: {
			T_EADC_VLMN0_REG    adc_volume_0;

			adc_volume_0.reg = EAC_GETREG(EADC_VLMN0_REG_OFS);
			ret = adc_volume_0.bit.ADC_PGA_GAIN_RIGHT;
		}
		break;


	case EAC_CONFIG_AD_DGAIN1_L: {
			T_EADC_VLMN0_REG    adc_volume_0;

			adc_volume_0.reg = EAC_GETREG(EADC_VLMN0_REG_OFS);
			ret = adc_volume_0.bit.ADC_DGAIN_LEFT;
		}
		break;

	case EAC_CONFIG_AD_DGAIN1_R: {
			T_EADC_VLMN0_REG    adc_volume_0;

			adc_volume_0.reg = EAC_GETREG(EADC_VLMN0_REG_OFS);
			ret = adc_volume_0.bit.ADC_DGAIN_RIGHT;
		}
		break;

	case EAC_CONFIG_AD_DGAIN2_L: {
			T_EADC_VLMN1_REG    adc_volume_1;

			adc_volume_1.reg = EAC_GETREG(EADC_VLMN1_REG_OFS);
			ret = adc_volume_1.bit.ADC_DGAIN2_LEFT;
		}
		break;

	case EAC_CONFIG_AD_DGAIN2_R: {
			T_EADC_VLMN1_REG    adc_volume_1;

			adc_volume_1.reg = EAC_GETREG(EADC_VLMN1_REG_OFS);
			ret = adc_volume_1.bit.ADC_DGAIN2_RIGHT;
		}
		break;

	case EAC_CONFIG_AD_DMIC2_DGAIN_L: {
			T_DMIC2_VLMN_REG    dmic2vlm;

			dmic2vlm.reg = EAC_GETREG(DMIC2_VLMN_REG_OFS);
			ret = dmic2vlm.bit.DMIC2_DGAIN_LEFT;
		}
		break;

	case EAC_CONFIG_AD_DMIC2_DGAIN_R: {
			T_DMIC2_VLMN_REG    dmic2vlm;

			dmic2vlm.reg = EAC_GETREG(DMIC2_VLMN_REG_OFS);
			ret = dmic2vlm.bit.DMIC2_DGAIN_RIGHT;
		}
		break;

	case EAC_CONFIG_AD_BOOST_COMPEN_L: {
			T_EALC_CFIG0_REG    alc_config_0;

			alc_config_0.reg = EAC_GETREG(EALC_CFIG0_REG_OFS);
			ret = alc_config_0.bit.ALC_BOOST_COMPEN_L;
		}
		break;

	case EAC_CONFIG_AD_BOOST_COMPEN_R: {
			T_EALC_CFIG0_REG    alc_config_0;

			alc_config_0.reg = EAC_GETREG(EALC_CFIG0_REG_OFS);
			ret = alc_config_0.bit.ALC_BOOST_COMPEN_R;
		}
		break;

	case EAC_CONFIG_AD_DCCAN_EN: {
			T_EADC_CTRL0_REG    adc_ctrl_0;

			adc_ctrl_0.reg = EAC_GETREG(EADC_CTRL0_REG_OFS);
			ret = adc_ctrl_0.bit.ADC_DCCAN_EN;
		}
		break;

	case EAC_CONFIG_AD_DCCAN_RESO: {
			T_EADC_CTRL0_REG    adc_ctrl_0;

			adc_ctrl_0.reg = EAC_GETREG(EADC_CTRL0_REG_OFS);
			ret = adc_ctrl_0.bit.ADC_DCCAN_RES;
		}
		break;

	case EAC_CONFIG_AD_DCINIT_L: {
			T_EADC_DCCAN_INIT_REG   adc_dccan_init;

			adc_dccan_init.reg = EAC_GETREG(EADC_DCCAN_INIT_REG_OFS);
			ret = adc_dccan_init.bit.DC_INIT_LEFT;
		}
		break;

	case EAC_CONFIG_AD_DCINIT_R: {
			T_EADC_DCCAN_INIT_REG   adc_dccan_init;

			adc_dccan_init.reg = EAC_GETREG(EADC_DCCAN_INIT_REG_OFS);
			ret = adc_dccan_init.bit.DC_INIT_RIGHT;
		}
		break;

	case EAC_CONFIG_AD_DMIC2_DCINIT_L: {
			T_DMIC2_DCCAN_INIT_REG   dmic2_dccan_init;

			dmic2_dccan_init.reg = EAC_GETREG(DMIC2_DCCAN_INIT_REG_OFS);
			ret = dmic2_dccan_init.bit.DMIC2_DC_INIT_LEFT;
		}
		break;

	case EAC_CONFIG_AD_DMIC2_DCINIT_R: {
			T_DMIC2_DCCAN_INIT_REG   dmic2_dccan_init;

			dmic2_dccan_init.reg = EAC_GETREG(DMIC2_DCCAN_INIT_REG_OFS);
			ret = dmic2_dccan_init.bit.DMIC2_DC_INIT_RIGHT;
		}
		break;

	case EAC_CONFIG_AD_MICBIAS_EN: {
			T_EADC_CTRL0_REG    adc_ctrl_0;

			adc_ctrl_0.reg = EAC_GETREG(EADC_CTRL0_REG_OFS);
			ret = adc_ctrl_0.bit.ADC_MIC_BIAS_EN;
		}
		break;

	case EAC_CONFIG_AD_MICBIAS_LVL: {
			T_EADC_CTRL0_REG    adc_ctrl_0;

			adc_ctrl_0.reg = EAC_GETREG(EADC_CTRL0_REG_OFS);
			ret = adc_ctrl_0.bit.ADC_MIC_BIAS_LVL;
		}
		break;

	case EAC_CONFIG_AD_DCOFS_L: {
			T_EADC_DCOFS_VAL_REG    adc_dccan_ofs;

			adc_dccan_ofs.reg = EAC_GETREG(EADC_DCOFS_VAL_REG_OFS);
			ret = adc_dccan_ofs.bit.DC_OFFSET_LEFT;
		}
		break;

	case EAC_CONFIG_AD_DCOFS_R: {
			T_EADC_DCOFS_VAL_REG    adc_dccan_ofs;

			adc_dccan_ofs.reg = EAC_GETREG(EADC_DCOFS_VAL_REG_OFS);
			ret = adc_dccan_ofs.bit.DC_OFFSET_RIGHT;
		}
		break;

	case EAC_CONFIG_AD_DMIC2_DCOFS_L: {
			T_DMIC2_DCOFS_VAL_REG    dmic2_dccan_ofs;

			dmic2_dccan_ofs.reg = EAC_GETREG(DMIC2_DCOFS_VAL_REG_OFS);
			ret = dmic2_dccan_ofs.bit.DMIC2_DC_OFFSET_LEFT;
		}
		break;

	case EAC_CONFIG_AD_DMIC2_DCOFS_R: {
			T_DMIC2_DCOFS_VAL_REG   dmic2_dccan_ofs;

			dmic2_dccan_ofs.reg = EAC_GETREG(DMIC2_DCOFS_VAL_REG_OFS);
			ret = dmic2_dccan_ofs.bit.DMIC2_DC_OFFSET_RIGHT;
		}
		break;

	case EAC_CONFIG_AD_ZC_EN: {
			T_EALC_CFIG0_REG    alc_config_0;

			alc_config_0.reg = EAC_GETREG(EALC_CFIG0_REG_OFS);
			ret = alc_config_0.bit.ADC_ZERO_CROSS_EN;
		}
		break;

	case EAC_CONFIG_AD_ZC_TIMEOUT: {
			ret = EAC_GETREG(EAC_ZCTC_REG_OFS);
		}
		break;

	case EAC_CONFIG_AD_PDREF_BIAS: {

            T_EAC_CTRL2_REG     eac_ctrl_2;

			eac_ctrl_2.reg = EAC_GETREG(EAC_CTRL2_REG_OFS);
			ret = eac_ctrl_2.bit.PDREF_BIAS;

		}
		break;

	case EAC_CONFIG_AD_PD_VCMBIAS: {

            T_EAC_PHYCTRL_REG   phy_ctrl;

			phy_ctrl.reg = EAC_GETREG(EAC_PHYCTRL_REG_OFS);
			ret = phy_ctrl.bit.PD_BIAS;

		}
		break;


	default: {
			DBG_WRN("Get Cfg ID Err!\r\n");
		}
		break;

	}

	return ret;
}

/**
    Configure IIR filter coefficients (7-entries)

    EAC embeds a second order IIR filter. This API is used to configure the filter coefficients.
    The IIR can assign different filter coefficients for left/right channel and the selection is controlled
    by input parameter iir_ch. The input coefficients p_coef have 7 entries. They are:
    p_coef[0]=B0,p_coef[1]=B1,p_coef[2]=B2,p_coef[3]=A0,p_coef[4]=A1,p_coef[5]=A2,p_coef[6]=TotalGain.
    The second order IIR equation is H(z) =  ((B0 + B1*Z1 + B2*Z2) / (A0 - A1*Z1 - A2*Z2)) x (Total-Gain).
    The user can get the floating point parameters from MATLAB simulation and feeds the floating point coefficient
    to EAC_IIRCOEF() to convert to EAC used fixed point INT32 coefficients.
    The floating point coefficient supported range is from -2.0 ~ +1.999969482421875.

    @param[in] iir_ch    IIR channel selection. Please refer to EAC_IIRCH definition.
    @param[in] p_coef    The filter coefficients p_coef have 7 entries. They are:
    p_coef[0]=B0,p_coef[1]=B1,p_coef[2]=B2,p_coef[3]=A0,p_coef[4]=A1,p_coef[5]=A2,p_coef[6]=TotalGain.
    The second order IIR equation is H(z) =  ((B0 + B1*Z1 + B2*Z2) / (A0 - A1*Z1 - A2*Z2)) x (Total-Gain).

    @return void
*/
void eac_set_iir_coef(EAC_IIRCH iir_ch, INT32 *p_coef)
{
	UINT32 i;

	if (iir_ch & EAC_IIRCH_LEFT) {
		for (i = 0; i < 7; i++) {
			EAC_SETREG((EAC_IIRCOF0_REG_OFS + (i << 2)), p_coef[i]);
		}
	}

	if (iir_ch & EAC_IIRCH_RIGHT) {
		for (i = 0; i < 7; i++) {
			EAC_SETREG((EAC_IIRCOF7_REG_OFS + (i << 2)), p_coef[i]);
		}
	}
}

//@}
#endif

#if 1
/**
    @name Digital to Analog Group APIs
*/
//@{

/**
    Set configuring to EAC DA(Playback) Functions

    This api is used to configure the digital to analog(Playback) functions of embedded audio codec,
    such as Digital Gain, Analog Gain, Power-Enable, ...etc.

    @param[in] config_id     The configuration selection ID. Please refer to EAC_CONFIG_DA for details.
    @param[in] config_value   The configuration parameter according to the config_id.

    @return void
*/
void eac_set_da_config(EAC_CONFIG_DA config_id, UINT32 config_value)
{
	T_EDAC_VLMN_REG     dac_volume;
	T_EDAC_CTRL0_REG    dac_control_0;
	T_EAC_CTRL2_REG     eac_ctrl_2;
	T_EDAC_DPOP0_REG    depop_0;
	T_EDAC_DPOP1_REG    depop_1;
	unsigned long flag;


	loc_cpu(flag);
	switch (config_id) {

	/*
	    DA General Control Functions
	*/
	case EAC_CONFIG_DA_DGAIN_L: {
			if (config_value > EAC_DAC_DGAIN_MAX) {
				config_value = EAC_DAC_DGAIN_MAX;
			}

			// Set DA Config: Left Channel Digital Gain
			dac_volume.reg = EAC_GETREG(EDAC_VLMN_REG_OFS);
			dac_volume.bit.DAC_DGAIN_LEFT  = config_value;
			EAC_SETREG(EDAC_VLMN_REG_OFS, dac_volume.reg);
		}
		break;

	case EAC_CONFIG_DA_DGAIN_R: {
			if (config_value > EAC_DAC_DGAIN_MAX) {
				config_value = EAC_DAC_DGAIN_MAX;
			}

			// Set DA Config: Right Channel Digital Gain
			dac_volume.reg = EAC_GETREG(EDAC_VLMN_REG_OFS);
			dac_volume.bit.DAC_DGAIN_RIGHT = config_value;
			EAC_SETREG(EDAC_VLMN_REG_OFS, dac_volume.reg);
		}
		break;

	case EAC_CONFIG_DA_DATAMIXER: {
			// Set DA Config: Select the DA Data Mixer Modes.
			// When in Source Stereo but play mono mode, we suggest to use L/R Average mode.
			// When in Source Stereo and play Stereo mode, we use L/R independent mode.
			dac_control_0.reg = EAC_GETREG(EDAC_CTRL0_REG_OFS);
			dac_control_0.bit.DAC_DATA_MIX = config_value;
			EAC_SETREG(EDAC_CTRL0_REG_OFS, dac_control_0.reg);
		}
		break;

	case EAC_CONFIG_DA_DCCAN_EN: {
			dac_control_0.reg = EAC_GETREG(EDAC_CTRL0_REG_OFS);
			dac_control_0.bit.DAC_DCCAN_EN = (config_value > 0);
			EAC_SETREG(EDAC_CTRL0_REG_OFS, dac_control_0.reg);
		}
		break;

	case EAC_CONFIG_DA_DCCAN_RESO: {
			dac_control_0.reg = EAC_GETREG(EDAC_CTRL0_REG_OFS);
			dac_control_0.bit.DAC_DCCAN_RES = config_value;
			EAC_SETREG(EDAC_CTRL0_REG_OFS, dac_control_0.reg);
		}
		break;


	/*
	    DA Analog Related Control Functions
	*/
	case EAC_CONFIG_DA_LINEOUT_GAIN:
	case EAC_CONFIG_DA_HEADPHONE_GAIN: {
			/*
			if (config_value > EAC_DAC_PGAIN_HP_MAX) {
				config_value = EAC_DAC_PGAIN_HP_MAX;
			}

			// Set DA Config: Select HeadPhone Analog PGA Gain.
			dac_pga.reg = EAC_GETREG(EDAC_PGA_REG_OFS);
			dac_pga.bit.DAC_PGA_LINEOUT = config_value;
			EAC_SETREG(EDAC_PGA_REG_OFS, dac_pga.reg);*/
		}
		break;

	case EAC_CONFIG_DA_SPEAKER_GAIN: {
			/*
			if (config_value > EAC_DAC_PGAIN_SPK_MAX) {
				config_value = EAC_DAC_PGAIN_SPK_MAX;
			}


			// Set DA Config: Select Speaker Analog PGA Gain.
			dac_pga.reg = EAC_GETREG(EDAC_PGA_REG_OFS);
			dac_pga.bit.DAC_PGA_SPK = config_value;
			EAC_SETREG(EDAC_PGA_REG_OFS, dac_pga.reg);*/
		}
		break;

	case EAC_CONFIG_DA_POWER_EN_L: {
			/*
			// Set DA Config: Enable/Disable Analog Left Channel Power
			eac_ctrl_2.reg = EAC_GETREG(EAC_CTRL2_REG_OFS);
			eac_ctrl_2.bit.DAC_ANALOG_LEFT_POW  = (config_value > 0);
			EAC_SETREG(EAC_CTRL2_REG_OFS, eac_ctrl_2.reg);*/
		}
		break;

	case EAC_CONFIG_DA_POWER_EN_R: {
			/*
			// Set DA Config: Enable/Disable Analog Right Channel Power
			eac_ctrl_2.reg = EAC_GETREG(EAC_CTRL2_REG_OFS);
			eac_ctrl_2.bit.DAC_ANALOG_RIGHT_POW = (config_value > 0);
			EAC_SETREG(EAC_CTRL2_REG_OFS, eac_ctrl_2.reg);*/
		}
		break;

	case EAC_CONFIG_DA_RESET: {
			// Set DA Config: Enable/Disable Analog Block Reset
			eac_ctrl_2.reg = EAC_GETREG(EAC_CTRL2_REG_OFS);
			eac_ctrl_2.bit.DAC_ANALOG_RST    = (config_value > 0);
#if 0//_FPGA_EMULATION_
			eac_ctrl_2.bit.ADC_ANALOG_RST_L  = (config_value > 0);
			eac_ctrl_2.bit.ADC_ANALOG_RST_R  = (config_value > 0);
#endif
			EAC_SETREG(EAC_CTRL2_REG_OFS, eac_ctrl_2.reg);
		}
		break;

	case EAC_CONFIG_DA_DEPOP_EN: {
			// Set DA Config: Enable/Disable Depop Function.
			dac_control_0.reg = EAC_GETREG(EDAC_CTRL0_REG_OFS);
			dac_control_0.bit.ANALOG_DEPOP_EN = config_value;
			EAC_SETREG(EDAC_CTRL0_REG_OFS, dac_control_0.reg);
		}
		break;

	case EAC_CONFIG_DA_DEPOP_PERIOD_H: {
			// Set DA Config: Depop High Level Cycle period
			depop_0.reg = EAC_GETREG(EDAC_DPOP0_REG_OFS);
			depop_0.bit.CLK_HIGH_PERIOD = config_value;
			EAC_SETREG(EDAC_DPOP0_REG_OFS, depop_0.reg);
		}
		break;

	case EAC_CONFIG_DA_DEPOP_PERIOD_L: {
			// Set DA Config: Depop Low Level Cycle period
			depop_0.reg = EAC_GETREG(EDAC_DPOP0_REG_OFS);
			depop_0.bit.CLK_LOW_PERIOD = config_value;
			EAC_SETREG(EDAC_DPOP0_REG_OFS, depop_0.reg);
		}
		break;

	case EAC_CONFIG_DA_DEPOP_PERIOD_CYC: {
			// Set DA Config: Depop Low Level Cycle period
			depop_1.reg = EAC_GETREG(EDAC_DPOP1_REG_OFS);
			depop_1.bit.CYCLE_CNT = config_value;
			EAC_SETREG(EDAC_DPOP1_REG_OFS, depop_1.reg);
		}
		break;

	case EAC_CONFIG_DA_TEST_EN: {
			// Set DA Config: Enable/Disable TEST Function.
			dac_control_0.reg = EAC_GETREG(EDAC_CTRL0_REG_OFS);
			dac_control_0.bit.ANALOG_TEST_EN = config_value;
			EAC_SETREG(EDAC_CTRL0_REG_OFS, dac_control_0.reg);
		}
		break;

	case EAC_CONFIG_DA_OSR_SEL: {
			// Set DA Config: set DAC over sampling ratio
			dac_control_0.reg = EAC_GETREG(EDAC_CTRL0_REG_OFS);
			dac_control_0.bit.DAC_OSR_SEL = config_value;
			EAC_SETREG(EDAC_CTRL0_REG_OFS, dac_control_0.reg);
		}
		break;

	default: {
			// Unlock CPU first for the following debug msg
			unl_cpu(flag);

			DBG_WRN("Set Cfg ID Err!\r\n");
		}
		return;

	}

	unl_cpu(flag);
}

/**
    Get configuring of EAC DA(Playback) Functions

    This api is used to get the digital to analog(Playback) configurations of embedded audio codec,
    such as Digital Gain, Analog Gain, Power-Enable, ...etc.

    @param[in] config_id     The configuration selection ID. Please refer to EAC_CONFIG_DA for details.
     - @b EAC_CONFIG_DA_RESET:  Get the DA Analog Block is set reset or not.
     - @b Others:               Have not implemented. Always return 0x0.

    @return The configuration parameter according to the config_id
*/
UINT32 eac_get_da_config(EAC_CONFIG_DA config_id)
{
	UINT32 ret = 0;

	T_EAC_CTRL2_REG     eac_ctrl_2;
	T_EDAC_CTRL0_REG    dac_control_0;
	T_EDAC_OUT_REG      dac_out;
	T_EDAC_VLMN_REG     dac_volume;
	T_EDAC_DPOP0_REG    depop_0;
	T_EDAC_DPOP1_REG    depop_1;

	switch (config_id) {

	case EAC_CONFIG_DA_RESET: {
			eac_ctrl_2.reg = EAC_GETREG(EAC_CTRL2_REG_OFS);
			ret = eac_ctrl_2.bit.DAC_ANALOG_RST;
		}
		break;

	case EAC_CONFIG_DA_SPEAKER_GAIN: {
		/*
			dac_pga.reg = EAC_GETREG(EDAC_PGA_REG_OFS);
			ret = dac_pga.bit.DAC_PGA_SPK;*/
		}
		break;

	case EAC_CONFIG_DA_LINEOUT_GAIN:
	case EAC_CONFIG_DA_HEADPHONE_GAIN: {
		/*
			dac_pga.reg = EAC_GETREG(EDAC_PGA_REG_OFS);
			ret = dac_pga.bit.DAC_PGA_LINEOUT;*/
		}
		break;

	case EAC_CONFIG_DA_POWER_EN_L: {
		/*
			eac_ctrl_2.reg = EAC_GETREG(EAC_CTRL2_REG_OFS);
			ret = eac_ctrl_2.bit.DAC_ANALOG_LEFT_POW;*/
		}
		break;

	case EAC_CONFIG_DA_POWER_EN_R: {
		/*
			eac_ctrl_2.reg = EAC_GETREG(EAC_CTRL2_REG_OFS);
			ret = eac_ctrl_2.bit.DAC_ANALOG_RIGHT_POW;*/
		}
		break;

	case EAC_CONFIG_DA_SPKR_MONO_EN: {
			dac_out.reg = EAC_GETREG(EDAC_OUT_REG_OFS);
			ret = dac_out.bit.DAC_SPK_MONO_EN;
		}
		break;

	case EAC_CONFIG_DA_ZC_EN: {
			dac_control_0.reg = EAC_GETREG(EDAC_CTRL0_REG_OFS);
			ret = dac_control_0.bit.DAC_ZC_EN;
		}
		break;

	case EAC_CONFIG_DA_DATAMIXER: {
			dac_control_0.reg = EAC_GETREG(EDAC_CTRL0_REG_OFS);
			ret = dac_control_0.bit.DAC_DATA_MIX;
		}
		break;

	case EAC_CONFIG_DA_DCCAN_EN: {
			dac_control_0.reg = EAC_GETREG(EDAC_CTRL0_REG_OFS);
			ret = dac_control_0.bit.DAC_DCCAN_EN;
		}
		break;

	case EAC_CONFIG_DA_DCCAN_RESO: {
			dac_control_0.reg = EAC_GETREG(EDAC_CTRL0_REG_OFS);
			ret = dac_control_0.bit.DAC_DCCAN_RES;
		}
		break;

	case EAC_CONFIG_DA_DEPOP_EN: {
			dac_control_0.reg = EAC_GETREG(EDAC_CTRL0_REG_OFS);
			ret = dac_control_0.bit.ANALOG_DEPOP_EN;
		}
		break;

	case EAC_CONFIG_DA_DEPOP_PERIOD_H: {
			depop_0.reg = EAC_GETREG(EDAC_DPOP0_REG_OFS);
			ret = depop_0.bit.CLK_HIGH_PERIOD;
		}
		break;

	case EAC_CONFIG_DA_DEPOP_PERIOD_L: {
			depop_0.reg = EAC_GETREG(EDAC_DPOP0_REG_OFS);
			ret = depop_0.bit.CLK_LOW_PERIOD;
		}
		break;

	case EAC_CONFIG_DA_DEPOP_PERIOD_CYC: {
			depop_1.reg = EAC_GETREG(EDAC_DPOP1_REG_OFS);
			ret = depop_1.bit.CYCLE_CNT;
		}
		break;

	case EAC_CONFIG_DA_TEST_EN: {
			// Set DA Config: Enable/Disable TEST Function.
			dac_control_0.reg = EAC_GETREG(EDAC_CTRL0_REG_OFS);
			ret = dac_control_0.bit.ANALOG_TEST_EN;
		}
		break;

	case EAC_CONFIG_DA_DGAIN_L: {
			dac_volume.reg = EAC_GETREG(EDAC_VLMN_REG_OFS);
			ret = dac_volume.bit.DAC_DGAIN_LEFT;
		}
		break;

	case EAC_CONFIG_DA_DGAIN_R: {
			dac_volume.reg = EAC_GETREG(EDAC_VLMN_REG_OFS);
			ret = dac_volume.bit.DAC_DGAIN_RIGHT;
		}
		break;

	default: {
			DBG_WRN("Get Cfg ID Err!\r\n");
		}
		break;

	}

	return ret;
}

/**
    Check whether the DA path is enabled or not

    This function is used to check whether the DA path is enable or not in embedded audio codec.

    @return
     - @b TRUE:  Digital to Analog is enabled.
     - @b FALSE: Digital to Analog is disabled.
*/
BOOL eac_is_dac_enable(void)
{
	T_EAC_CTRL0_REG     reg_ctrl_0;

	// Check if DA is enabled or not.
	reg_ctrl_0.reg = EAC_GETREG(EAC_CTRL0_REG_OFS);

	return reg_ctrl_0.bit.EAC_DA_EN;
}

/**
    Set Enable/Disable the DA output path for embedded audio codec

    This function is used to select the DA output path  for embedded audio codec.

    @param [in] eac_output       DAC Output path selection
     - @b EAC_OUTPUT_SPK:       Select codec output to Speaker channel. Set TRUE to enable. Set FALSE to disable.
     - @b EAC_OUTPUT_LINE:      Select codec output to Line Out. This would map to EAC_OUTPUT_HP in NT96660. Set TURE/FALSE to enable/disable both the HP left and right channels.
     - @b EAC_OUTPUT_LINE_L:    Select codec output to Line Out  Left Channel. Set TURE/FALSE to enable/disable the Lineout left channel.
     - @b EAC_OUTPUT_LINE_R:    Select codec output to Line Out Right Channel. Set TURE/FALSE to enable/disable the Lineout right channel.
     - @b EAC_OUTPUT_NONE:      Disable all of the codec output path. Set TRUE or FALSE would both disable all of the output path.

    @param [in] b_en             If selected path is EAC_OUTPUT_NONE, this api always disable all of the codec output path.
     - @b TRUE:  Enable the selected output path
     - @b FALSE: Disable the selected output path

    @return void
*/
void eac_set_dac_output(EAC_OUTPUT eac_output, BOOL b_en)
{
	T_EDAC_OUT_REG  dac_out;
	unsigned long flag;

	// This api would enable/disable the specified analog output path.
	// If the speaker output is selected, we would also open the mono enable.
	// This would make the output channel fixed to mono,
	// because the 660 speaker output must be done by the two seperate DAC.

	loc_cpu(flag);

	dac_out.reg = EAC_GETREG(EDAC_OUT_REG_OFS);

	switch (eac_output) {
	case EAC_OUTPUT_SPK: {
			dac_out.bit.DAC_SPK_MONO_EN    = (b_en > 0);
		}
		break;

	case EAC_OUTPUT_CLSD_SPK: {
			DBG_WRN("ClassD No support.\r\n");
		}
		break;

	case EAC_OUTPUT_ALL: {
			dac_out.bit.DAC_SPK_MONO_EN         = 0; // need to make sure lineout can output independant data.
			dac_out.bit.ANALOG_LINEOUT_LEFT_EN  = (b_en > 0);
			dac_out.bit.ANALOG_LINEOUT_RIGHT_EN = (b_en > 0);
		}
		break;

	case EAC_OUTPUT_LINE: {
			dac_out.bit.ANALOG_LINEOUT_LEFT_EN  = (b_en > 0);
			dac_out.bit.ANALOG_LINEOUT_RIGHT_EN = (b_en > 0);

			if (b_en) {
				dac_out.bit.DAC_SPK_MONO_EN     = 0;
			}
		}
		break;

	case EAC_OUTPUT_LINE_L: {
			dac_out.bit.ANALOG_LINEOUT_LEFT_EN  = (b_en > 0);

			if (b_en) {
				dac_out.bit.ANALOG_LINEOUT_RIGHT_EN = 0;
				dac_out.bit.DAC_SPK_MONO_EN         = 0;
			}
		}
		break;

	case EAC_OUTPUT_LINE_R: {
			dac_out.bit.ANALOG_LINEOUT_RIGHT_EN = (b_en > 0);

			if (b_en) {
				dac_out.bit.ANALOG_LINEOUT_LEFT_EN  = 0;
				dac_out.bit.DAC_SPK_MONO_EN         = 0;
			}
		}
		break;

	case EAC_OUTPUT_NONE: {
			dac_out.bit.DAC_SPK_MONO_EN         = 0;
			dac_out.bit.ANALOG_LINEOUT_LEFT_EN  = 0;
			dac_out.bit.ANALOG_LINEOUT_RIGHT_EN = 0;
		}
		break;

	default:
		return;
	}

	EAC_SETREG(EDAC_OUT_REG_OFS, dac_out.reg);

	unl_cpu(flag);

}

/**
    Check the DA output path is Enabled or Disabled

    This function is used to check the DA output path is Enabled or Disabled.

    @param [in] eac_output       DAC Output path selection. Please use EAC_OUTPUT_* as parameter.

    @return If selected path is EAC_OUTPUT_NONE, this api would always return FALSE.
     - @b TRUE:  The selected output path is ENABLE.
     - @b FALSE: The selected output path is DISABLE.
*/
BOOL eac_get_dac_output(EAC_OUTPUT eac_output)
{
	T_EDAC_OUT_REG  dac_out;
	BOOL            ret = 0;


	dac_out.reg = EAC_GETREG(EDAC_OUT_REG_OFS);

	switch (eac_output) {
	case EAC_OUTPUT_SPK: {
			ret = dac_out.bit.DAC_SPK_MONO_EN;
		}
		break;

	case EAC_OUTPUT_CLSD_SPK: {
			ret = 0;
		}
		break;

	case EAC_OUTPUT_LINE: {
			ret = ((dac_out.bit.ANALOG_LINEOUT_LEFT_EN) && (dac_out.bit.ANALOG_LINEOUT_RIGHT_EN));
		}
		break;

	case EAC_OUTPUT_LINE_L: {
			ret = dac_out.bit.ANALOG_LINEOUT_LEFT_EN;
		}
		break;

	case EAC_OUTPUT_LINE_R: {
			ret = dac_out.bit.ANALOG_LINEOUT_RIGHT_EN;
		}
		break;

	default:
		break;
	}

	return ret;
}

/**
    Get Current Record PGA Gain

    Get Current Record PGA Gain
*/
BOOL eac_get_cur_pgagain(UINT16 *p_left, UINT16 *p_right)
{
	T_EAC_CURPGA_REG reg_current_pga;

	reg_current_pga.reg = EAC_GETREG(EAC_CURPGA_REG_OFS);
	EAC_SETREG(EAC_CURPGA_REG_OFS, reg_current_pga.reg & 0x01000100);

	*p_left  = reg_current_pga.bit.LEFT_CH;
	*p_right = reg_current_pga.bit.RIGHT_CH;

	return (reg_current_pga.reg & 0x01000100) > 0;
}

//@}
#endif

//@}

/*
    Debug Mode enable
*/
void eac_debug(BOOL b_en, BOOL b_mode_ad)
{
	T_EAC_DEBUG_REG  reg_dbg;

	reg_dbg.reg = EAC_GETREG(EAC_DEBUG_REG_OFS);

	if (b_en) {
		if (b_mode_ad) {
			reg_dbg.bit.AD_DBG = 1;
			reg_dbg.bit.DA_DBG = 0;
		} else {
			reg_dbg.bit.AD_DBG = 0;
			reg_dbg.bit.DA_DBG = 1;
		}
	} else {
		reg_dbg.reg = 0;
	}

	EAC_SETREG(EAC_DEBUG_REG_OFS, reg_dbg.reg);
}

/*
    DAC type field:
    *0x0: DWA Left
    *0x1: DWA Right
    0x2: SDM Left
    0x3: SDM Right
    0x4: SDM
    0x5: DAI Input
    *0x6: ClassD
    0x7: compensation
*/
void eac_debug_type(UINT32 dac_type)
{
	T_EAC_DEBUG_REG  reg_dbg;

	reg_dbg.reg = EAC_GETREG(EAC_DEBUG_REG_OFS);
	reg_dbg.bit.DBG_TYPE = dac_type;
	EAC_SETREG(EAC_DEBUG_REG_OFS, reg_dbg.reg);
}


#if 1

void eac_set_phypower(BOOL b_en)
{
	T_EAC_PHYCTRL_REG	reg_phy_ctrl;
	unsigned long flag;

	loc_cpu(flag);
	reg_phy_ctrl.reg = EAC_GETREG(EAC_PHYCTRL_REG_OFS);

	if (b_en) {
		reg_phy_ctrl.bit.CS_COM_CTL 		= 2;
		reg_phy_ctrl.bit.CS_COM_DAC 		= 1;
	} else {
		reg_phy_ctrl.bit.CS_COM_CTL 		= 0;
		reg_phy_ctrl.bit.CS_COM_DAC 		= 3;
	}

	EAC_SETREG(EAC_PHYCTRL_REG_OFS, reg_phy_ctrl.reg);
	unl_cpu(flag);
}

void eac_set_phydacpower(BOOL b_en)
{
	T_EAC_PHYCTRL_REG	reg_phy_ctrl;
	unsigned long flag;

	loc_cpu(flag);
	reg_phy_ctrl.reg = EAC_GETREG(EAC_PHYCTRL_REG_OFS);

	if (b_en) {
		reg_phy_ctrl.bit.CS_LN_DAC    = 1;
		reg_phy_ctrl.bit.SW_POP_R     = 0;
        reg_phy_ctrl.bit.SW_POP_L     = 0;
        reg_phy_ctrl.bit.PD_V2I       = 0;
        reg_phy_ctrl.bit.PD_OP_COM    = 0;
	} else {
		reg_phy_ctrl.bit.CS_LN_DAC    = 3;
		reg_phy_ctrl.bit.SW_POP_R     = 1;
        reg_phy_ctrl.bit.SW_POP_L     = 1;
        reg_phy_ctrl.bit.PD_V2I       = 1;
        reg_phy_ctrl.bit.PD_OP_COM    = 1;
	}

	EAC_SETREG(EAC_PHYCTRL_REG_OFS, reg_phy_ctrl.reg);
	unl_cpu(flag);
}


#if 0//def __KERNEL__
EXPORT_SYMBOL(eac_enableclk);
EXPORT_SYMBOL(eac_enableadcclk);
EXPORT_SYMBOL(eac_enabledacclk);
EXPORT_SYMBOL(eac_setdacclkrate);
EXPORT_SYMBOL(eac_init);
EXPORT_SYMBOL(eac_set_ad_enable);
EXPORT_SYMBOL(eac_set_da_enable);
EXPORT_SYMBOL(eac_get_ad_enable);
EXPORT_SYMBOL(eac_get_da_enable);
EXPORT_SYMBOL(eac_set_load);
EXPORT_SYMBOL(eac_set_ad_config);
EXPORT_SYMBOL(eac_get_ad_config);
EXPORT_SYMBOL(eac_set_iir_coef);
EXPORT_SYMBOL(eac_set_da_config);
EXPORT_SYMBOL(eac_get_da_config);
EXPORT_SYMBOL(eac_is_dac_enable);
EXPORT_SYMBOL(eac_set_dac_output);
EXPORT_SYMBOL(eac_get_dac_output);
EXPORT_SYMBOL(eac_get_cur_pgagain);
EXPORT_SYMBOL(eac_set_phypower);
EXPORT_SYMBOL(eac_set_phydacpower);
#endif
#endif

