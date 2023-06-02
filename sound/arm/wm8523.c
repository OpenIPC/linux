#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/workqueue.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <asm/bitops.h>
#include <sound/tlv.h>
#include <sound/control.h>
#include <sound/soc.h>
#include <sound/ak_pcm.h>
#include <sound/wm8523.h>

#include <mach/gpio.h>
#include <mach/clock.h>
#include <mach/map.h>
#include <mach/ak_codec.h>

#define AK_MCLK		(12000000)
#define DAC_INIT	0
#define WM8523_INIT	1
//ADC inner module
typedef enum _ADC_INNER_MODULE_TYPE
{
	ADC_CONTROLLER = 0,
	ADC_FILTER_WITH_CLK,
	ADC_FILTER_WITH_HSCLK,
	ADC_ANALOG,
	ADC_ALL_MODULE,
}T_ADC_INNER_MODULE;

//DAC inner module
typedef enum _DAC_INNER_MODULE_TYPE
{
	DAC_CONTROLLER = 0,
	DAC_FILTER_WITH_CLK,
	DAC_FILTER_WITH_HSCLK,
	DAC_ANALOG,
	DAC_ALL_MODULE,
}T_DAC_INNER_MODULE;

struct ak39_codec {
	struct ak_codec_dai dai;
	void __iomem  *analog_ctrl_base;
	void __iomem  *adda_cfg_base;
	struct delayed_work d_work;



	struct gpio_info hpdet_gpio;
	struct gpio_info spkrshdn_gpio;
	struct gpio_info hpmute_gpio;

	int hp_det_irq;
	int irq_hp_on_type;
	int hp_on_value;
	int playmode;
	int hpmute_en_val;

	unsigned 	used_hp_mute:1; // whether to use hardware de-pipa or not
	unsigned 	outputing:1;
	unsigned 	dac_state:1;
	unsigned 	adc2_state:1;
};

struct wm8523_data {
	unsigned short i2s_mclk;
	unsigned short aif2;
	unsigned int sample_rate;
	unsigned int channels;

	unsigned long status;
	struct work_struct work;
	struct ak_codec_dai dai;
};

static unsigned int g_i2s_mclk = 11289600;  //default clock for 44.1khz
static struct i2c_client *wm8523_client;
static struct wm8523_data *wm8523_data_p;

static inline struct wm8523_data *to_ak39_codec(struct ak_codec_dai *dai)
{
	return dai ? container_of(dai, struct wm8523_data, dai): NULL;
}

static struct {
	int div;
	int bclk_div;
} bclk_divs[] = {
	{ 32,  2  },
	{ 64,  3  },
	{ 128,  4  },
};

static unsigned short wm8523_read(u8 reg)
{
	return swab16(i2c_smbus_read_word_data(wm8523_client, reg));
}

static int wm8523_write(u8 reg, u16 value)
{
	return i2c_smbus_write_word_data(wm8523_client, reg, swab16(value));
}

/**
 * @brief  open ADC2
 * @author 
 * @date   
 * @param[in]  void
 * @return  void
 */
void ak39_codec_adc2_open(struct ak_codec_dai *dai)
{
	struct ak39_codec *codec = to_ak39_codec(dai);

	//unsigned long i;
	if(codec->adc2_state)
		return;

	codec->adc2_state = 1;

	REG32(codec->analog_ctrl_base + SOFT_RST_CTRL_REG) &= ~(1<<3);
	REG32(codec->analog_ctrl_base + CLOCK_GATE_CTRL_REG) &= ~(1<<3);

	// soft reset ADC2 controller
	REG32(codec->analog_ctrl_base + SOFT_RST_CTRL_REG) |= (ADC2_SOFT_RST);
	REG32(codec->analog_ctrl_base + SOFT_RST_CTRL_REG) &= ~(ADC2_SOFT_RST);

//	set_cur_pmos(codec, 0x0);  //must be set to 0 when ADC2 is working

	REG32(codec->analog_ctrl_base + MULTIPLE_FUN_CTRL_REG1) |= IN_DAAD_EN; //enable internal

	//disable vcm2 discharge
	REG32(codec->analog_ctrl_base + ANALOG_CTRL_REG1) &= ~(0x1f << 4);

	//SelVcm3
	REG32(codec->analog_ctrl_base + ANALOG_CTRL_REG1) |= VCM3_SEL;

	//PowerOn Vcm2/3
	REG32(codec->analog_ctrl_base + ANALOG_CTRL_REG1) &= ~PD_VCM3;
	REG32(codec->analog_ctrl_base + ANALOG_CTRL_REG1) &= ~PD_VCM2;

	//SetVcmNormal
	REG32(codec->analog_ctrl_base + ANALOG_CTRL_REG1) &= ~PL_VCM2;
	REG32(codec->analog_ctrl_base + ANALOG_CTRL_REG2) &= ~PL_VCM3;

	//EnableAdc2Limit
	REG32(codec->analog_ctrl_base + ANALOG_CTRL_REG2) |= ADC_LIM;

	REG32(codec->adda_cfg_base + ADC2_CONFIG_REG) &= ~HOST_RD_INT_EN;
	REG32(codec->adda_cfg_base + ADC2_CONFIG_REG) |= CH_POLARITY_SEL;//Receive the left channel data when the lrclk is high
	REG32(codec->adda_cfg_base + ADC2_CONFIG_REG) &= ~I2S_EN;        //Internal ADC MODE
	REG32(codec->adda_cfg_base + ADC2_CONFIG_REG) &= ~WORD_LENGTH_MASK;
	REG32(codec->adda_cfg_base + ADC2_CONFIG_REG) |= (0xF << 8);     //WORD LENGTH IS 16 BIT

	REG32(codec->analog_ctrl_base + SOFT_RST_CTRL_REG) |= (ADC2_SOFT_RST);

	//Enable adc controller
	REG32(codec->adda_cfg_base + ADC2_CONFIG_REG) |= ADC2_CTRL_EN;

	//Enable l2
	REG32(codec->adda_cfg_base + ADC2_CONFIG_REG) |= ADC2MODE_L2_EN;

	//Power on adc2 conversion
	REG32(codec->analog_ctrl_base + ANALOG_CTRL_REG2) &= ~PD_S2D;

	REG32(codec->analog_ctrl_base + ANALOG_CTRL_REG2) &= ~PD_ADC2;

	mdelay(1);
	pd_ref_enable(codec);
}

/**
 * @brief      close ADC2
 * @author 
 * @date   
 * @param[in]    void 
 * @return  void
 */
void ak39_codec_adc2_close(struct ak_codec_dai *dai)
{
	unsigned long reg_val;
	struct ak39_codec *codec = to_ak39_codec(dai);

	if(!codec->adc2_state)
		return;

	codec->adc2_state = 0;

	//disable l2
	REG32(codec->adda_cfg_base + ADC2_CONFIG_REG) &= ~ADC2MODE_L2_EN;

	//disable adc2 clk
	REG32(codec->analog_ctrl_base + HIGHSPEED_CLOCK_CTRL_REG) &= ~ADC2_CLK_EN;

	//disable ADC2 interface
	REG32(codec->adda_cfg_base + ADC2_CONFIG_REG) &= ~ADC2_CTRL_EN;

	//Power off adc2
	REG32(codec->analog_ctrl_base + ANALOG_CTRL_REG2) |= PD_ADC2;

	if((2 << 12) != (REG32(codec->analog_ctrl_base + ANALOG_CTRL_REG1) & (2 << 12)))
	{
		if(!(REG32(codec->analog_ctrl_base + CLOCK_CTRL_REG) & DAC_CLK_EN))
		{
			reg_val = REG32(codec->analog_ctrl_base + ANALOG_CTRL_REG1);
			reg_val |= PD_VCM3;
			reg_val |= PD_VCM2;
			reg_val |= PL_VCM2;
			//reg_val |= PD_REF;
			REG32(codec->analog_ctrl_base + ANALOG_CTRL_REG1) = reg_val;
			pd_ref_disable(codec);

		}
	}
	REG32(codec->analog_ctrl_base + CLOCK_CTRL_REG) |= (DAC_DIV_VLD);     //inhabit adc2 clock

}

/**
 * @brief      set ADC2 sample rate
 * @author 
 * @date   
 * @param[in]    samplerate:  desired rate to be set
 * @return  void
 */
unsigned long ak39_codec_set_adc2_samplerate(struct ak_codec_dai *dai,
	   		unsigned int  samplerate)
{
	unsigned char mode_sel = 0;
    unsigned char save_div = 0;
	unsigned long out_sr = 0;
	unsigned long i = 0;
	struct ak39_codec *codec = to_ak39_codec(dai);

	//disable HCLK, and disable ADC interface
	REG32(codec->adda_cfg_base + ADC2_CONFIG_REG) &= ~(ADC2_CTRL_EN);
	REG32(codec->analog_ctrl_base + HIGHSPEED_CLOCK_CTRL_REG) &= ~(ADC2_HCLK_EN);
	REG32(codec->analog_ctrl_base + HIGHSPEED_CLOCK_CTRL_REG) &= ~(ADC2_CLK_EN);

	set_adc_highspeed(codec, samplerate);		
	
	out_sr = get_adc2_osr_div(codec, &mode_sel, &save_div, samplerate);

	REG32(codec->analog_ctrl_base + SOFT_RST_CTRL_REG) &= ~(1<<27); //reset adc from adc clk
	i = 0;
    while(REG32(codec->analog_ctrl_base + HIGHSPEED_CLOCK_CTRL_REG) & (ADC2_DIV_VLD))
    {
        ++i;
        if(i > 100000)
        {
            printk("adc set sr fail\n");
            return 0;
        }
    } 
    REG32(codec->analog_ctrl_base + HIGHSPEED_CLOCK_CTRL_REG) &= ~(MASK_ADC2_DIV);
    REG32(codec->analog_ctrl_base + HIGHSPEED_CLOCK_CTRL_REG) |= ADC2_DIV(save_div);

    REG32(codec->analog_ctrl_base + FADEOUT_CTRL_REG) &= ~(1<<ADC2_OSR_BIT);
    REG32(codec->analog_ctrl_base + FADEOUT_CTRL_REG) |= (mode_sel << ADC2_OSR_BIT);

	REG32(codec->analog_ctrl_base + HIGHSPEED_CLOCK_CTRL_REG) |= (ADC2_DIV_VLD);
	i = 0;
    while(REG32(codec->analog_ctrl_base + HIGHSPEED_CLOCK_CTRL_REG) & (ADC2_DIV_VLD))
    {
        ++i;
        if(i > 100000)
        {
            printk("adc set clk reg fail\n");
            return 0;
        }
    } 
	REG32(codec->analog_ctrl_base + SOFT_RST_CTRL_REG) |= (1<<27); //release the reset adc from adc clk

	
	REG32(codec->analog_ctrl_base + HIGHSPEED_CLOCK_CTRL_REG) |= (ADC2_CLK_EN);//enable ADC2 Clock 
	REG32(codec->analog_ctrl_base + HIGHSPEED_CLOCK_CTRL_REG) |= (ADC2_HCLK_EN);//enable ADC2 Clock 
	REG32(codec->adda_cfg_base + ADC2_CONFIG_REG) |= (ADC2_CTRL_EN);
	return out_sr;
}


/**
 * @brief      set ADC23 channel
 * @author 
 * @date   
 * @param[in]    chnl: 1-mono; 2-stereo
 * @return  void
 */
void ak39_codec_set_adc2_channels(struct ak_codec_dai *dai, unsigned int chnl)
{

}


/**
 * @brief   cfg shutdown speaker GPIO
 * @author 
 * @date   
 * @param[in]  bOn: 1-power on; 0-power off
 * @return  void
 */
void ak39_codec_speak_on(struct ak39_codec *codec, bool bOn)
{
	unsigned int pin = codec->spkrshdn_gpio.pin; 
	ak_setpin_as_gpio(pin);
	ak_gpio_cfgpin(pin, AK_GPIO_DIR_OUTPUT);
	ak_gpio_setpin(pin, bOn);
}

static unsigned int clk_get_core_pll_freq(void)
{
	unsigned int m, n, od;
	unsigned int pll_cfg_val;
	unsigned int  core_pll_freq;	
	
	pll_cfg_val = REG32(AK_VA_SYSCTRL + 0x08) ;
	m = (pll_cfg_val & 0xfe);
	n = ((pll_cfg_val & 0xf00)>>8);
	od = ((pll_cfg_val & 0x3000)>>12); 

	core_pll_freq = (m * 12 * 1000000) / (n *  (1<<od));
	printk("====m=%d,n=%d,od=%d,core_pll =%d========================\n",m,n,od,core_pll_freq);
	return core_pll_freq;
}

void dac_clk_gate_close(T_DAC_INNER_MODULE module_type)
{
	switch(module_type)
	{
		case DAC_CONTROLLER:								//close dac controller gate
			REG32(AK_VA_SYSCTRL + 0x1C) |= (1<<4);
			break;
			
		case  DAC_FILTER_WITH_CLK:							//close dac fillter clk gate
			REG32(AK_VA_SYSCTRL + 0x0C) &= (~(1<<28));
			break;

		case  DAC_FILTER_WITH_HSCLK:						//close dac fillter hsclk gate
			REG32(AK_VA_SYSCTRL + 0x10) &= (~(1<<18));
			break;

		case  DAC_ANALOG:									//close dac analog clk
			REG32(AK_VA_SYSCTRL + 0x9C) |= (1<<9);
			break;
			
		case  DAC_ALL_MODULE:
			//close dac controller gate
			REG32(AK_VA_SYSCTRL + 0x1C) |= (1<<4);

			//close dac fillter clk gate
			REG32(AK_VA_SYSCTRL + 0x0C) &= (~(1<<28));

			//close dac fillter hsclk gate
			REG32(AK_VA_SYSCTRL + 0x10) &= (~(1<<18));

			//close dac analog clk
			REG32(AK_VA_SYSCTRL + 0x9C) |= (1<<9);
			break;
			
		defalut:
			printk("The module type not exist in dac_clk_gate_close function!\n");
			break;
	}

}

void dac_config_dac_clk_div(u32 div, u32 div_frac)
{
	//close the dac_filter_en_cfg
	REG32(AK_VA_SYSCTRL + 0x70) &= (~(1<<3));

	//reset dac from dac clk, dac_rst and dachs_rst must be reset or clear at the same time
	REG32(AK_VA_SYSCTRL + 0x20) &= (~((1<<26)|(1<<28)));

	//wait the sddac_div_vld_cfg  clear
	while( (REG32(AK_VA_SYSCTRL + 0x0C) & (1<<29)) !=0 )
	{;}
	
	//disable the sd_dac_clk
	dac_clk_gate_close(DAC_FILTER_WITH_CLK);

	//change the div val
	REG32(AK_VA_SYSCTRL + 0x0C) &= (~(0xfffff<<4));
	REG32(AK_VA_SYSCTRL + 0x0C) |= (div<<4);
	REG32(AK_VA_SYSCTRL + 0x0C) |= (div_frac<<12);

	// set sddac_div_vld_cfg
	REG32(AK_VA_SYSCTRL + 0x0C) |= (1<<29);	

	//wait the sddac_div_vld_cfg  clear
	while( (REG32(AK_VA_SYSCTRL + 0x0C) & (1<<29)) !=0 )
	{;}

	//release the reset dac from dac clk
	REG32(AK_VA_SYSCTRL + 0x20) |= (1<<26)|(1<<28);
	

}
void dac_clk_gate_open(T_DAC_INNER_MODULE module_type)
{
	switch(module_type)
	{
		case DAC_CONTROLLER:								//open dac controller gate
			REG32(AK_VA_SYSCTRL + 0x1C) &= (~(1<<4));
			break;
			
		case  DAC_FILTER_WITH_CLK:							//open dac fillter clk gate 
			REG32(AK_VA_SYSCTRL + 0x0C) |= (1<<28);
			break;

		case  DAC_FILTER_WITH_HSCLK:						//open dac fillter hsclk gate
			REG32(AK_VA_SYSCTRL + 0x10) |= (1<<18);
			break;

		case  DAC_ANALOG:									//open dac analog clk
			REG32(AK_VA_SYSCTRL + 0x9C) &= (~(1<<9));
			break;
			
		case  DAC_ALL_MODULE:
			//open dac controller gate
			REG32(AK_VA_SYSCTRL + 0x1C) &= (~(1<<4));

			//open dac fillter clk gate 
			REG32(AK_VA_SYSCTRL + 0x0C) |= (1<<28);

			//open dac fillter hsclk gate
			REG32(AK_VA_SYSCTRL + 0x10) |= (1<<18);

			//open dac analog clk
			REG32(AK_VA_SYSCTRL + 0x9C) &= (~(1<<9));
			break;
			
		defalut:
			printk("The module type not exist in dac_clk_gate_open function!\n");
			break;
	}

}

void I2SSetTransMclk(u32 I2SMclk)
{
	unsigned int dac_clk_div;
	unsigned int core_pll_freq;
		
	core_pll_freq = clk_get_core_pll_freq();
	
	dac_clk_div = core_pll_freq / I2SMclk - 1;

	//printk(KERN_ERR "I2SSetTransMclk,pll=%d,dac_clk_div=%d\n",core_pll_freq,dac_clk_div);
	//close the dac clk and disable dac filter 
	//config dac_clk_div
	dac_config_dac_clk_div(dac_clk_div, 0);

	//open dac clk gate
	dac_clk_gate_open(DAC_FILTER_WITH_CLK);

}


static void ak39_dac_init(struct ak_codec_dai *dai)
{
	int RegValue;

	//struct ak39_codec *codec = to_ak39_codec(dai);
printk("==============ak39_dac_init==============================\r\n");
	if (!test_and_set_bit(DAC_INIT, &wm8523_data_p->status)) {

		#if 0
 		wm8523_write(WM8523_DEVICE_ID, 0x0000);  //reset all register of wm8523 to default value
		wm8523_write(WM8523_PSCTRL1, 0x0003);  //power up UN-mute		
		wm8523_write(WM8523_AIF_CTRL1, 0x00AA); //master mode, 16 bits, i2s format, BCLK inverted 
		wm8523_write(WM8523_AIF_CTRL2, wm8523_data_p->aif2); //stereo, bclk_div, lrclk_ratio, set by aif2
		wm8523_write(WM8523_DAC_CTRL3, 0); //
  		#endif
   
		REG32(AK_VA_SYSCTRL + 0x58) &= ~(0x7 << 25);   //mask I2SSR and I2SST & disable inter DAC
		REG32(AK_VA_SYSCTRL + 0x58) |= (1<<26);	   //to extern DAC via I2S slave mode

		/* disable pulldown */
		REG32(AK_VA_SYSCTRL + 0x80) |= (1 << 8);
		REG32(AK_VA_SYSCTRL + 0x80) |= (1 << 9);
		REG32(AK_VA_SYSCTRL + 0x80) |= (1 << 10);
	
		REG32(AK_VA_SYSCTRL + 0x80) |= (1 << 7);
		REG32(AK_VA_SYSCTRL + 0x80) |= (1 << 12);


		/* set share pin as i2s */

		REG32(AK_VA_SYSCTRL + 0x74) |= (1 << 8);
		REG32(AK_VA_SYSCTRL + 0x74) |= (1 << 9);
		REG32(AK_VA_SYSCTRL + 0x74) |= (1 << 10);
	
		REG32(AK_VA_SYSCTRL + 0x74) |= (1 << 11);
		REG32(AK_VA_SYSCTRL + 0x74) |= (1 << 13);

	
		//must enable DAC controller clock first , or DAC_CTRL_REG1 and DAC_CTRL_REG2 can't be congigured
		REG32(AK_VA_SYSCTRL + 0x1C) &= ~(1<<4);	
		REG32(AK_VA_SYSCTRL + 0x20) &= ~(1<<4);
	
		unsigned int m_DAC_CTRL_I2S_CFG_REG = ioremap(0x20110000, 0x10);
		//config the dac word length ,
		REG32(m_DAC_CTRL_I2S_CFG_REG +0x04) &= (~0x1f<<0);
		REG32(m_DAC_CTRL_I2S_CFG_REG +0x04) |= 24;//16; 	
	

		//REG32(m_DAC_CTRL_I2S_CFG_REG) &= (~(1<<4)); //normal mode, for 24 bits I2S data
		REG32(m_DAC_CTRL_I2S_CFG_REG) |= (1<<4);	//memory saving mode, for 16 bits I2S data
	
		REG32(m_DAC_CTRL_I2S_CFG_REG) &= (~((1<<2) | (0x1<<3)));	//disable MUTE
	
		//enalbe dac controller, enable l2 mode, Normat format 
		REG32(m_DAC_CTRL_I2S_CFG_REG) |= ((0x1<<0) | (1<<1));
	
		//set MCLK clock
		//I2SSetTransMclk(wm8523_data_p->i2s_mclk);
		
		//printk(KERN_ERR "wm8523_data_p->i2s_mclk= %d\n",g_i2s_mclk);
		I2SSetTransMclk(g_i2s_mclk);

		REG32(AK_VA_SYSCTRL + 0x58) &= (~(3<<30));
		REG32(AK_VA_SYSCTRL + 0x58) |= (1<<31);	//use I2S_MCLK = DAC_CLK	

		#if 0	
		printk(KERN_ERR "REG32(I2S_CFG_REG) = 0x%x\n", REG32(AK_VA_SYSCTRL + 0x58));
		printk(KERN_ERR "REG32(SHARE_PIN_CFG1_REG) = 0x%x\n", REG32(AK_VA_SYSCTRL + 0x74) );	
		printk(KERN_ERR "REG32(DAC_CTRL_I2S_CFG_REG) = 0x%x\n", REG32(m_DAC_CTRL_I2S_CFG_REG+0x04));
		printk(KERN_ERR "REG32(DAC_CTRL_CFG_REG) = 0x%x\n", REG32(m_DAC_CTRL_I2S_CFG_REG));
		#endif
	}
}

static void ak39_dac_exit(void)
{
	int RegValue;

	if (test_and_clear_bit(DAC_INIT, &wm8523_data_p->status)) {
		printk(KERN_ERR "ak39_dac_exit\n");
	}
#if 0
	if (test_and_clear_bit(DAC_INIT, &wm8523_data_p->status)) {
		REG32(RegAddr.pAddress2002E + DAC_CONFIG_REG) &= (~L2_EN);

    		RegValue = REG32(RegAddr.pAddress0800 + ANALOG_CTRL_REG1_READ);   
    		RegValue |= (PD_OP);
    		RegValue |= (PD_CK);
    		REG32(RegAddr.pAddress0800 + ANALOG_CTRL_REG1_WRITE) = RegValue;

		/* too long */
		mdelay(10);

    		REG32(RegAddr.pAddress0800 + CLK_DIV_REG2) &= (~DAC_CLK_EN);
    		REG32(RegAddr.pAddress0800 + CLK_DIV_REG2) |= DAC_GATE;
    		REG32(RegAddr.pAddress0800 + ANALOG_CTRL_REG4) &= (~DAC_EN);
	}
	#endif
}

static void wm8523_playback_start(void)
{
	//if (!test_and_set_bit(WM8523_INIT, &wm8523_data_p->status)) {
		printk(KERN_ERR "wm8523_playback_start\n");
		#if 1
 		wm8523_write(WM8523_DEVICE_ID, 0x0000);  //reset all register of wm8523 to default value
		wm8523_write(WM8523_PSCTRL1, 0x0003);  //power up UN-mute		
		wm8523_write(WM8523_AIF_CTRL1, 0x00AA); //master mode, 16 bits, i2s format, BCLK inverted 
		wm8523_write(WM8523_AIF_CTRL2, wm8523_data_p->aif2); //stereo, bclk_div, lrclk_ratio, set by aif2
		wm8523_write(WM8523_DAC_CTRL3, 0); //
  		#endif

		//do not need to set WM8523_DAC_GAINL, WM8523_DAC_GAINR,WM8523_ZERO_DETECT, use default value
#if 0
	printk(KERN_EMERG "WM8523_DEVICE_ID=%04x\n", wm8523_read(WM8523_DEVICE_ID));
	printk(KERN_EMERG "WM8523_REVISION=%04x\n", wm8523_read(WM8523_REVISION));
	printk(KERN_EMERG "WM8523_PSCTRL1=%04x\n", wm8523_read(WM8523_PSCTRL1));
	printk(KERN_EMERG "WM8523_AIF_CTRL1=%04x\n", wm8523_read(WM8523_AIF_CTRL1));
	printk(KERN_EMERG "WM8523_AIF_CTRL2=%04x\n", wm8523_read(WM8523_AIF_CTRL2));
	printk(KERN_EMERG "WM8523_DAC_CTRL3=%04x\n", wm8523_read(WM8523_DAC_CTRL3));
	printk(KERN_EMERG "WM8523_DAC_GAINL=%04x\n", wm8523_read(WM8523_DAC_GAINL));
	printk(KERN_EMERG "WM8523_DAC_GAINR=%04x\n", wm8523_read(WM8523_DAC_GAINR));
	printk(KERN_EMERG "WM8523_ZERO_DETECT=%04x\n", wm8523_read(WM8523_ZERO_DETECT));
#endif
//	}
}

static void wm8523_playback_end(void)
{
	if (test_and_clear_bit(WM8523_INIT, &wm8523_data_p->status)) {
		//set wm8523 register here

	}
}

static void wm8523_set_samplerate(unsigned int rate)
{
	int i;
	unsigned int sys_clk;
	unsigned int bclk;
	unsigned int aif2_sr =0;
	unsigned int aif2_bclkdiv =3;
	//unsigned int sys_fs_rate[] = {64, 128, 192, 256, 384, 512, 768, 1024, 1408, 1536};
	unsigned int sys_fs_rate[] = {128, 192, 256, 384, 512, 768, 1152};

	//save i2s mclk

	if (rate == wm8523_data_p->sample_rate)
	{
		printk(KERN_ERR "wm8523_set_samplerate, rate not changed,rate=%d\n",rate);
		return;
	}
	
	if (!(rate % 4000))
	{		
		sys_clk = 12288000;
	}
	else if (!(rate % 11025))
	{
		sys_clk = 11289600;
	}
	else
	{
		printk(KERN_ERR "wm8523_set_samplerate,invalid rate=%d\n",rate);			
		return;
	}

	//save i2s mclk
	g_i2s_mclk = sys_clk;

	for (i = 0; i < 7; i++) {
		if ((sys_clk / sys_fs_rate[i]) == rate)
			break;
	}

	if (i == 7)
	{
		printk(KERN_ERR "wm8523_set_samplerate, sys_fs_rate not valid\n");
		return;
	}
	else
	{
		aif2_sr = i+1;
	}

	switch (rate) {
		case 8000:
		case 11025:
		case 12000:
		case 16000:
		case 22050:
		case 24000:
		case 32000:
		case 44100:
		case 48000:
		case 64000:
		case 88200:
		case 96000:
			break;
		default:
			printk("==>Did not support this samplerate %u\n", rate);
			return;
	}

	//bclk = (wm8523_data_p->channels * 16) * rate;   //for 24 bits scenario
	bclk = (wm8523_data_p->channels * 32) * rate;   //for 24 bits scenario, set to 32 for I2S DOUT width

	if(sys_clk/4 == bclk)
	{
		aif2_bclkdiv = 0;	
	}
	else if(sys_clk/8 == bclk)
	{
		aif2_bclkdiv = 1;
	}
	else if(32*rate == bclk)
	{
		aif2_bclkdiv = 2;
	}
	else if(64*rate == bclk)
	{
		aif2_bclkdiv = 3;
	}
	else if(128*rate == bclk)
	{
		aif2_bclkdiv = 4;
	}
	else
	{		
		printk("wm8523_set_samplerate, error\n");
		return;
	}
	
	wm8523_data_p->sample_rate = rate;
	wm8523_data_p->aif2 = (aif2_bclkdiv<<WM8523_BCLKDIV_SHIFT)+aif2_sr;
}

static void wm8523_set_channels(unsigned int channels)
{
	wm8523_data_p->channels = channels;
}

static void wm8523_start_to_play(struct ak_codec_dai *dai, 
		unsigned int channels, unsigned int samplerate)
{
	

	wm8523_set_channels(channels);
	wm8523_set_samplerate(samplerate);
	ak39_dac_init(dai);
	wm8523_playback_start();
}


static struct ak_codec_ops wm8523_ops = {
	.dac_init		= ak39_dac_init,
	.dac_exit		= ak39_dac_exit,
	.adc_init		= ak39_codec_adc2_open,
	.adc_exit		= ak39_codec_adc2_close,
	.set_dac_samplerate	= wm8523_set_samplerate,
	.set_adc_samplerate	= ak39_codec_set_adc2_samplerate,
	.set_dac_channels	= wm8523_set_channels,
	.set_adc_channels	= ak39_codec_set_adc2_channels,
	.playback_start		= wm8523_playback_start,
	.playback_end		= wm8523_playback_end,
	.capture_start		= ak39_codec_capture_start,
	.capture_end		= ak39_codec_capture_stop,
	.start_to_play		= wm8523_start_to_play,
};

static struct snd_kcontrol_new wm8523_controls[] = {};

static int wm8523_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;
	u16 value = 0;
	unsigned long clk168m;
	struct wm8523_data *wm8523;
	struct wm8523_platform_data *pdata = client->dev.platform_data;

	printk("===>%s: wm8523 init\n", __func__);


	//if (!pdata)
	//	return -ENODEV;

	wm8523 = kzalloc(sizeof(struct wm8523_data), GFP_KERNEL);
	if (!wm8523) {
		printk("alloc wm8523 memory failed\n");
		return -ENOMEM;
	}
	
	wm8523_client = client;
	wm8523_data_p = wm8523;
	i2c_set_clientdata(client, wm8523);
	//printk("=========read ID=================\r\n");
	//mdelay(5000);
	/* read chip id */
	#if 1
	value = wm8523_read(WM8523_DEVICE_ID);
	printk("=========read ID,0x%x=================\r\n", value);
	if (value != 0x8523) {
		printk("wm8523 did not work, ID is %x\n", value);
		//return -ENODEV;
	}
	#endif
	#if 0	
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "akpcm_AnalogCtrlRegs"); 
		if(!res)
		{
			printk(KERN_ERR "no memory resource for analog_ctrl_res\n");
			err = -ENXIO;
			goto out_free_codec;
		}
	
		wm8523->analog_ctrl_base = ioremap(res->start, res->end - res->start + 1);
		if (!wm8523->analog_ctrl_base) {
			printk(KERN_ERR "could not remap analog_ctrl_res memory");
			err = -ENXIO;
			goto out_free_codec;
		}
	
	#endif
	

	wm8523->dai.ops = &wm8523_ops;
	wm8523->dai.num_kcontrols = ARRAY_SIZE(wm8523_controls);
	wm8523->dai.kcontrols = wm8523_controls;
	ret = ak_codec_register(&wm8523->dai);
	if (ret) {
		printk("register codec ops failed\n");
		goto err_out;
	}
	
	#if 0
	/* we need a 12MHz MCLK */
	clk168m = ak_get_asic_pll_clk();
	wm8523->mclk_div = clk168m / AK_MCLK - 1;
	if ((wm8523->mclk_div + 1) * AK_MCLK != clk168m)
		printk("Warning: clk168m can not div to %uHz\n", AK_MCLK);
	#endif
	//printk("===>%s: wm8523 end\n", __func__);
	return 0;

err_out:
	kfree(wm8523);

	return ret;
}

static int wm8523_remove(struct i2c_client *client)
{
	struct wm8523_data *wm8523 = i2c_get_clientdata(client);

	kfree(wm8523);
	return 0;
}

static const struct i2c_device_id wm8523_id[] = {
	{ "wm8523", 0 },
	{ }
};

static struct i2c_driver wm8523_driver = {
	.probe 		= wm8523_probe,
	.remove 	= wm8523_remove,
	.id_table	= wm8523_id,
	.driver 	= {
		.owner	= THIS_MODULE,
		.name = "wm8523",
	},
};

static int __init wm8523_init(void)
{
	printk("==============wm8523====================\r\n");
	return i2c_add_driver(&wm8523_driver);
}

static void __exit wm8523_exit(void)
{
	i2c_del_driver(&wm8523_driver);
}

module_init(wm8523_init);
module_exit(wm8523_exit);

MODULE_AUTHOR("zhongjunchao@anyka.oa");
MODULE_DESCRIPTION("WM8523 DAC Driver");
MODULE_LICENSE("GPL");



