/*
 * ak_adc.c - ak ADC1 operation API
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <mach/adc.h>
#include <mach/gpio.h>

#define PK(fmt...) 		  		//printk(fmt)  // debug  

static spinlock_t adc1_lock;
void adc_close_channel(int channel_type);
void adc_open_channel(int channel_type);
int adc_open_init(int channel_type);
/**
 * @BRIEF  config saradc clk
* @AUTHOR luoyongchuang
* @DATE   2016-06 -13
 * @PARAM  div     SARADC CLK = 12Mhz/(div+1); 
 * @PARAM  div value always 2
 * @RETURN 
 * @NOTE: 
 */
static void adc1_clk_cfg(unsigned long div)
{
	unsigned long saradc_chief_driven;
	unsigned long saradc_module_driven;

	//reserve the driven value	
	saradc_chief_driven = ((REG32(SAR_IF_CFG_REG)) & (0x1<<0));
	saradc_module_driven = ((REG32(SAR_IF_CFG_REG)) & (0x7<<5));

	//disable module chief driven by sar adc clk 
	REG32(SAR_IF_CFG_REG) &= (~(1<<0));

		//disable Ain0_sampling   Ain1_sampling   Bat_sampling
	REG32(SAR_IF_CFG_REG) &= (~(0x7<<5));

	//close sar adc clk
	REG32(AD_DA_CLK1_REG) &= (~(0x1<<3));

	//cofig div 
	REG32(AD_DA_CLK1_REG) &= (~0x7);
	REG32(AD_DA_CLK1_REG) |= (div & 0x7);

	//open sar adc clk
	REG32(AD_DA_CLK1_REG) |= (0x1<<3);

	//get back the driven cfg
	REG32(SAR_IF_CFG_REG) |= (saradc_chief_driven | saradc_module_driven);
}

static void power_on_adc1(void)
{
	REG32(SAR_ADC_ACR_REG2) &= (~(1 << 0));
}

static void power_off_adc1(void)
{
	REG32(SAR_ADC_ACR_REG2) |= (1 << 0);
}
#if 0
static void enable_adc1_channel(int channel)
{
	REG32(SAR_IF_CFG_REG) &= ~(1 << 0);
	REG32(SAR_IF_CFG_REG) |= (1 << (channel + 5));
	REG32(SAR_IF_CFG_REG) |= (1 << 0);
}

static void disable_adc1_channel(int channel)
{
	REG32(SAR_IF_CFG_REG) &= ~(1 << 0);
	REG32(SAR_IF_CFG_REG) &= ~(1 << (channel + 5));
	REG32(SAR_IF_CFG_REG) |= (1 << 0);
}
#endif
/**
* @BRIEF  get sar adc sample value  不屏蔽中断
* @AUTHOR luoyongchuang
* @DATE   2016-06 -13
* @PARAM   p_spl_val 
* @PARAM  val_index  指示是第几个通道
* @RETURN   sample value  返回值为    读到的CHANNEL 个数
* @NOTE:  传入三个整数的数组，按顺序排列是ain0, ain1, bat
*/
unsigned long adc_get_sample_val(int channel_type)
{
	unsigned long val = 0;

	//disable  sample 
	REG32(SAR_IF_CFG_REG) &= (~(1<<0));
	
	//clear sample done interrupt
	REG32(SAR_IF_INT_STATUS_REG) &= (~0x1);
	
	//eable  sample 
	REG32(SAR_IF_CFG_REG) |= (1<<0);

	//wait sample done
	while( (REG32(SAR_IF_INT_STATUS_REG) & 1) == 0)
	{
		mdelay(1);
		PK("sample NOT done.\n");
	}

	//disable  sample 
	REG32(SAR_IF_CFG_REG) &= (~(1<<0));

	
	//check whether enable sample open
	if( (channel_type == 0) || (channel_type == 1) )
	{
		val = ((REG32(SAR_IF_SMP_DAT_REG)>>(channel_type*12)) & 0xfff);
	}
	else if(channel_type == 2)
	{
		val = ((REG32(SAR_IF_INT_STATUS_REG)>>20) & 0xfff);
	}

	return val;
}


/**
 * @brief:	Read AD0/AD1/BAT voltage
* @AUTHOR luoyongchuang
* @DATE   2016-06 -13
 * 
 * @Warning: 	Please don`t use this function in IRQ handle routine!
 */
unsigned long adc1_read_channel(int channel)
{
	unsigned long val = 0;
	unsigned long flags;

//	power_on_adc1();
//	adc_open_channel(AK_ADC1_BAT);
//	mdelay(1);
	adc_open_init(channel);
  	spin_lock_irqsave(&adc1_lock, flags);		
	val = adc_get_sample_val(channel);

	PK("value is %ld mv\n",val);
	if (channel == AK_ADC1_BAT) 
	{
		val = val*(2)*(3000)/4096; //324/100是HPVDD verf参考电压。
		PK(" battery val is %ld mv\n",val);			
	}	
	else 
		val = val*(3000)/4096; 

#if 0
	enable_adc1_channel(channel);
	mdelay(1);

	val = ((REG32(SAR_IF_SMP_DAT_REG) >> (channel * 10)) & 0x3ff); 
	if (channel == AK_ADC1_BAT) {
		for(count = 0; count < 4; count++) {
			mdelay(1);
			val += ((REG32(SAR_IF_SMP_DAT_REG) >> (channel * 10)) & 0x3ff); 
		}
		val /= 5;
	}
/**
    由于不用电池，并且默认	CONFIG_AK_GETAIN 是 定义的，因此此项基本处于屏蔽状态。
**/	
#if defined(CONFIG_AK_GETAIN)	
#else
	val = (val * AK_AVCC) >> 10;
#endif

	disable_adc1_channel(channel);
#endif 

	adc_close_channel(channel);
	power_off_adc1();
	spin_unlock_irqrestore(&adc1_lock, flags);

	return val;
}


/**
* @BRIEF   close channel
* @AUTHOR luoyongchuang
* @DATE   2016-06 -13
* @PARAM  T_SARADC_CHANNEL_TYPE: SARADC_AIN0, SARADC_AIN1, SARADC_BAT
* @PARAM  
* @RETURN 
* @NOTE: 
*/
void adc_close_channel(int channel_type)
{
	unsigned long state;

	//close channel
	if( (REG32(SAR_IF_CFG_REG) & (1<<(channel_type+5))) == 0)
	{
		PK("this channel already close.\n");
		//return;
	}

	//reserve the sample switch state
	state = (REG32(SAR_IF_CFG_REG) & (1<<0));
	
	//close sample
	REG32(SAR_IF_CFG_REG) &= (~(1<<0));

	//clear channel interrupt state and sample interrupt state
	REG32(SAR_IF_INT_STATUS_REG) &= (~((1<<0) | (1<<(channel_type+1))));

	//close channel
	REG32(SAR_IF_CFG_REG) &= (~(1<<(channel_type+5)));

	if( channel_type == AK_ADC1_BAT)
	{
		//disable ratio
		REG32(SAR_ADC_ACR_REG2) &= (~(1<<7));   
	}
	
	//resume the sample switch state
	REG32(SAR_IF_CFG_REG) |= state;
	
}

/**
* @BRIEF   config channel
* @AUTHOR Zou Tianxiang
* @DATE   2012-11 -10
* @PARAM  T_SARADC_CHANNEL_CFG
* @PARAM  T_SARADC_CHANNEL_TYPE : SARADC_AIN0, SARADC_AIN1, SARADC_BAT
* @RETURN  AK_TRUE:   config channel success 
*                  AK_FALSE:  config channel false 
* @NOTE: 
*/
unsigned long adc_config_channel( unsigned long clkdiv,int channel_type)
{
	unsigned long samplerate;
	unsigned long adc1_clk;
	unsigned long spl_cycle, spl_hold, spl_wait;

	//make sure the saradc is power on
	if( (REG32(SAR_ADC_ACR_REG2) &  (1<<0))  ==  (1<<0))
	{
		PK("saradc is power down, please initial first.\n");
	}
	
	//reserve the sample state
	//ul_state= (REG32(SAR_IF_CFG_REG) & (1<<0));
	
	//disable sample 
	REG32(SAR_IF_CFG_REG) &= (~(1<<0));

	//clear all interrupt state
	REG32(SAR_IF_INT_STATUS_REG) = 0x0;
	
      //select channel
    if((channel_type == 0) || (channel_type == 1))
	  	REG32(SAR_ADC_ACR_REG2) |= ( 1<<(5+channel_type) );
	else if(channel_type == 2)
	    REG32(SAR_ADC_ACR_REG2) |= ( 1<<4 );

	 //enable channel
	REG32(SAR_IF_CFG_REG) |= ( 1<<(5+channel_type) );
	 

	/* one channel one time, default samplerate is 5000 */
	adc1_clk = ADC1_MAIN_CLK / (clkdiv + 1);
	samplerate = DEFAULT_SAMPLE;
	spl_cycle = adc1_clk / samplerate;
	spl_wait = 1;
	spl_hold = spl_wait + 16 + 1;

	REG32(SAR_IF_CFG_REG) &= (~(0xff << 14));
	REG32(SAR_IF_CFG_REG) |= (spl_wait << 14);

	REG32(SAR_TIMING_CFG_REG) = 0;
	REG32(SAR_TIMING_CFG_REG) = spl_cycle | (spl_hold << 16) ;	

	//config sample cnt
	REG32(SAR_IF_CFG_REG) &= (~(0x7<<8));
	REG32(SAR_IF_CFG_REG) |= (1<<8); //现在设置成1，可以设置1-7的值。应该是采集多少次后的平均值。

	//config threshold
	//if( (p_channel_cfg->thresh_val != 0) && (p_channel_cfg->thresh_val < 0x400))
	{
		//config threshold val
		REG32(SAR_THRESHOLD_REG) &= (~(0x3ff<<(channel_type*10)));
		REG32(SAR_THRESHOLD_REG) |= (100<<(channel_type*10));
		
		//config threshold polarity
		REG32(SAR_IF_CFG_REG) &= (~(0x1<<(channel_type+11)));	
		REG32(SAR_IF_CFG_REG) |= (1<<(channel_type+11));
	
		//config threshold interrupt 
		REG32(SAR_IF_CFG_REG) &= (~(0x1<<(channel_type+2)));
		REG32(SAR_IF_CFG_REG) |= (1<<(channel_type+2));
	} 
	if( channel_type == AK_ADC1_BAT)
	{
		//use 1:2 radio
		REG32(SAR_ADC_ACR_REG2) |= (1<<7);   
	}
	
	//config sample done interrupt
	REG32(SAR_IF_CFG_REG) &= (~(0x1<<1));		
	REG32(SAR_IF_CFG_REG) |= (0<<1);
	return 0;
}

/**
* @BRIEF   open channel
* @AUTHOR luoyongchuang
* @DATE   2016-06 -13
* @PARAM  T_SARADC_CHANNEL_TYPE: SARADC_AIN0, SARADC_AIN1, SARADC_BAT
* @PARAM  
* @RETURN 
* @NOTE: 
*/
void adc_open_channel(int channel_type)
{
	unsigned long state;

	//make sure the saradc is power on
	if( (REG32(SAR_ADC_ACR_REG2) &  (1<<0))  ==  (1<<0))
	{
		PK("saradc is power down, please initial first.\n");
		//while(1);
	}
	
	//check whether the channel already open
	if( (REG32(SAR_IF_CFG_REG) & (1<<(5+channel_type)) ) == (1<<(5+channel_type)) )
	{
		PK("this channel already open.\n");
		return ;
	}
	
	//reserve the sample switch state
	state = (REG32(SAR_IF_CFG_REG) & (1<<0));
	
	//close sample
	REG32(SAR_IF_CFG_REG) &= (~(1<<0));

	//clear channel interrupt state and sample interrupt state
	REG32(SAR_IF_INT_STATUS_REG) &= (~((1<<0) | (1<<(channel_type+1))));

    //enable channel
	REG32(SAR_IF_CFG_REG) |= ( 1<<(5+channel_type) );
	

	//resume the sample switch state
	REG32(SAR_IF_CFG_REG) |= state;

}
/**
 * @brief:	ADC1 initialization
 * @author:	Zhongjunchao
 * @data:	2011-7-20
 * @modify:	2013-4-27
 *
 * @note:	This function will init ADC1 clock and sample rate, and then enable
 * 		ADC1 but not power on. Any device use ADC1 please use the read value API.
 * 		We use some default value, if ADC1 can`t work make be will change it.
 * 		Please use this function in machine init.
 */
int adc1_init(void)
{
	
	return 0;
}
/**
 * @brief:	ADC1 initialization
 * @author:	Zhongjunchao
 * @data:	2011-7-20
 * @modify:	2013-4-27
 *
 * @note:	This function will init ADC1 clock and sample rate, and then enable
 * 		ADC1 but not power on. Any device use ADC1 please use the read value API.
 * 		We use some default value, if ADC1 can`t work make be will change it.
 * 		Please use this function in machine init.
 */

int adc_open_init(int channel_type)
{
	
	unsigned long  clkdiv;

	spin_lock_init(&adc1_lock);
	/* reset adc1 */
	REG32(RESET_CTRL_REG) &= (~(1<<30));

	/* config adc1 clk, default 1.5MHz(BAT use only in 1.5MHz) */
	clkdiv = ADC1_MAIN_CLK/ADC1_DEFAULT_CLK - 1;
	clkdiv &= 0x7;
	adc1_clk_cfg(clkdiv); //svt上直接赋值7

	/* release reset */
	REG32(RESET_CTRL_REG) |= (1<<30);

	/* disable all sample */
	REG32(SAR_IF_CFG_REG) &= ( ~( (1<<0) | (0x7<<5) ) );

	/* clear all adc1 interrupt state */
	REG32(SAR_IF_INT_STATUS_REG) = 0x0;

	/* mask all adc1 interrupt */
	REG32(SAR_IF_CFG_REG) &= (~ (0xf<<1));	

	/* power on adc1 */
	power_on_adc1();

	//select vref  = 3V
	REG32(SAR_ADC_ACR_REG2) &= (~(1<<3));

	//disable  sample 
	REG32(SAR_IF_CFG_REG) &= (~(1<<0));
	
	adc_close_channel(channel_type);

	adc_config_channel(clkdiv , channel_type);
	
	adc_open_channel(channel_type);

	//power_off_adc1();

	return 0;
}
