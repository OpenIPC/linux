/*------------------------------------------------------------------------------
 *   Copyright (c) 2008 MStar Semiconductor, Inc.  All rights reserved.
 *  ------------------------------------------------------------------------------*/

//------------------------------------------------------------------------------
//  Include files
//------------------------------------------------------------------------------
#include <linux/module.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/workqueue.h>
#include <linux/moduleparam.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
//#include <sound/driver.h>   remove it for kernel 2.6.29.1

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <asm/div64.h>
#include <sound/tlv.h>
#include <sound/jack.h>

//#include <mach/irqs.h>

#include "infinity_codec.h"
#include "infinity_pcm.h"
#include "bach_audio_debug.h"
#include "infinity.h"

//------------------------------------------------------------------------------
//  Macros
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
//  Variables
//------------------------------------------------------------------------------
static u16 codec_reg_backup[AUD_REG_LEN] =
{
  0x0,    //AUD_PLAYBACK_MUX
  0x1,    //AUD_ADC_MUX
  0x0,    //AUD_ATOP_PWR
  0x3,    //AUD_DPGA_PWR
  0,     //AUD_PLAYBACK_DPGA
  0,   //AUD_CAPTURE_DPGA
  0,   //AUD_MIC_GAIN
  0,   //AUD_MICPRE_GAIN,
  0,   //AUD_LINEIN_GAIN
  0,   //AUD_DIGMIC_PWR
  0,   //AUD_DBG_SINERATE
  0    //AUD_DBG_SINEGAIN
};

static u16 codec_reg[AUD_REG_LEN] =
{
  0x0,    //AUD_PLAYBACK_MUX(0:DMA reader 1:ADC In)
  0x1,    //AUD_ADC_MUX(0:Line-In 1:Mic-in)
  0x0,    //AUD_ATOP_PWR
  0x3,    //AUD_DPGA_PWR
  0,     //AUD_PLAYBACK_DPGA
  0,   //AUD_CAPTURE_DPGA
  0,   //AUD_MIC_GAIN
  0,   //AUD_MICPRE_GAIN,
  0,   //AUD_LINEIN_GAIN
  0,   //AUD_DIGMIC_PWR
  0,   //AUD_DBG_SINERATE
  0    //AUD_DBG_SINEGAIN
};

static struct infinity_pcm_dma_data infinity_pcm_dma_wr[] =
{
  {
    .name		= "DMA writer",
    .channel	= BACH_DMA_WRITER1,
  },
};

static struct infinity_pcm_dma_data infinity_pcm_dma_rd[] =
{
  {
    .name		= "DMA reader",
    .channel	= BACH_DMA_READER1,
  },
};

#ifdef CONFIG_OF
static int nGpio=-1;
static int nOn=0;
#endif

static int snd_soc_codec_update_bits(
	struct snd_soc_codec *codec, unsigned int reg,
	unsigned int mask, unsigned int val)
{
  bool change;
	unsigned int old, new;
	int ret;

  struct snd_soc_component *component = &codec->component;

	ret = component->read(component, reg, &old);
	if (ret < 0)
		return ret;

	new = (old & ~mask) | (val & mask);
	change = old != new;
	if (change)
	{
		ret = component->write(component, reg, new);
  	if (ret < 0)
  		return ret;
	}
	return change;
}



#if DAI_LEVEL_TAG
static int infinity_soc_dai_ops_trigger(struct snd_pcm_substream *substream, int cmd, struct snd_soc_dai *dai)
{
  AUD_PRINTF(DAI_LEVEL, "%s: dai = %s\n", __FUNCTION__, dai->name);
  return 0;
}

static int infinity_soc_dai_ops_prepare(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
  AUD_PRINTF(DAI_LEVEL, "%s: dai = %s\n", __FUNCTION__, dai->name);
  return 0;
}

static int infinity_soc_dai_ops_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
  AUD_PRINTF(DAI_LEVEL, "%s: dai = %s\n", __FUNCTION__, dai->name);
  return 0;
}

static int infinity_soc_dai_ops_hw_free(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
  AUD_PRINTF(DAI_LEVEL, "%s: dai = %s\n", __FUNCTION__, dai->name);
  return 0;
}

static int infinity_soc_dai_ops_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
  AUD_PRINTF(DAI_LEVEL, "%s: dai = %s\n", __FUNCTION__, dai->name);
  return 0;
}

static int infinity_soc_dai_ops_set_sysclk(struct snd_soc_dai *dai, int clk_id,  unsigned int freq, int dir)
{
  AUD_PRINTF(DAI_LEVEL, "%s: dai = %s\n", __FUNCTION__, dai->name);
  return 0;
}

static int infinity_soc_dai_ops_set_pll(struct snd_soc_dai *dai, int pll_id, int source,
                                    unsigned int freq_in, unsigned int freq_out)
{
  AUD_PRINTF(DAI_LEVEL, "%s: dai = %s\n", __FUNCTION__, dai->name);
  return 0;
}

static int infinity_soc_dai_ops_set_clkdiv(struct snd_soc_dai *dai, int div_id, int div)
{
  AUD_PRINTF(DAI_LEVEL, "%s: dai = %s\n", __FUNCTION__, dai->name);
  return 0;
}

static int infinity_soc_dai_ops_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
  AUD_PRINTF(DAI_LEVEL, "%s: dai = %s\n", __FUNCTION__, dai->name);
  return 0;
}

static void infinity_soc_dai_ops_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
  AUD_PRINTF(DAI_LEVEL, "%s: dai = %s\n", __FUNCTION__, dai->name);
}

static int infinity_soc_dai_ops_digital_mute(struct snd_soc_dai *dai, int mute)
{
  AUD_PRINTF(DAI_LEVEL, "%s: dai = %s, mute = %d\n", __FUNCTION__, dai->name, mute);
  return 0;
}
#endif

static struct snd_soc_dai_ops infinity_soc_codec_dai_ops =
{
#if DAI_LEVEL_TAG
  .set_sysclk = infinity_soc_dai_ops_set_sysclk,
  .set_pll    = infinity_soc_dai_ops_set_pll,
  .set_clkdiv = infinity_soc_dai_ops_set_clkdiv,

  .set_fmt    = infinity_soc_dai_ops_set_fmt,

  .startup		= infinity_soc_dai_ops_startup,
  .shutdown		= infinity_soc_dai_ops_shutdown,
  .trigger		= infinity_soc_dai_ops_trigger,
  .prepare    = infinity_soc_dai_ops_prepare,
  .hw_params	= infinity_soc_dai_ops_hw_params,
  .hw_free    = infinity_soc_dai_ops_hw_free,
  .digital_mute = infinity_soc_dai_ops_digital_mute,
#endif
};

static int infinity_soc_dai_probe(struct snd_soc_dai *dai)
{
  AUD_PRINTF(DAI_LEVEL, "%s: dai = %s\n", __FUNCTION__, dai->name);

  dai->playback_dma_data = (void *)&infinity_pcm_dma_rd[dai->id];
  dai->capture_dma_data = (void *)&infinity_pcm_dma_wr[dai->id];

  AUD_PRINTF(DAI_LEVEL, "setup playback_dma_data = %s, channel = %d\n", infinity_pcm_dma_rd[dai->id].name, infinity_pcm_dma_rd[dai->id].channel);
  AUD_PRINTF(DAI_LEVEL, "setup capture_dma_data  = %s, channel = %d\n", infinity_pcm_dma_wr[dai->id].name, infinity_pcm_dma_wr[dai->id].channel);

  return 0;
}

static int infinity_soc_dai_remove(struct snd_soc_dai *dai)
{
  AUD_PRINTF(DAI_LEVEL, "%s: dai = %s\n", __FUNCTION__, dai->name);
  return 0;
}

static int infinity_soc_dai_suspend(struct snd_soc_dai *dai)
{
  AUD_PRINTF(DAI_LEVEL, "%s: dai = %s\n", __FUNCTION__, dai->name);
  return 0;
}

static int infinity_soc_dai_resume(struct snd_soc_dai *dai)
{
  AUD_PRINTF(DAI_LEVEL, "%s: dai = %s\n", __FUNCTION__, dai->name);
  return 0;
}


struct snd_soc_dai_driver infinity_soc_codec_dai_drv[] =
{
  {
    .name				= "infinity-codec-dai-main",
    .probe				= infinity_soc_dai_probe,
    .remove       = infinity_soc_dai_remove,
    .suspend			= infinity_soc_dai_suspend,
    .resume				= infinity_soc_dai_resume,

    .playback			=
    {
      .stream_name	= "Main Playback",
      .channels_min	= 1,
      .channels_max	= 2,
      .rates			= SNDRV_PCM_RATE_8000_48000,
      .formats		= SNDRV_PCM_FMTBIT_S16_LE |
      SNDRV_PCM_FMTBIT_S24_LE |
      SNDRV_PCM_FMTBIT_S32_LE,
    },
    .capture			=
    {
      .stream_name	= "Main Capture",
      .channels_min	= 1,
      .channels_max	= 2,
      .rates			= SNDRV_PCM_RATE_8000_48000,
      .formats		= SNDRV_PCM_FMTBIT_S16_LE,
    },
    .ops				= &infinity_soc_codec_dai_ops,
  },

};

static int infinity_soc_codec_probe(struct snd_soc_codec *codec)
{

   int i;

   AUD_PRINTF(CODEC_LEVEL, "%s: codec = %s\n", __FUNCTION__, dev_name(codec->dev));

  //chip Initialization
  InfinitySysInit();
  AUD_PRINTF(TRACE_LEVEL, "Init system register\n");

  //init codec array
  for (i = 0; i < AUD_REG_LEN; i++)
  {
    snd_soc_write(codec, i, codec_reg_backup[i]);
  }



  return 0;
}

static int infinity_soc_codec_remove(struct snd_soc_codec *codec)
{
  AUD_PRINTF(CODEC_LEVEL, "%s: codec = %s\n", __FUNCTION__, dev_name(codec->dev));
  return 0;
}

static int infinity_soc_codec_resume(struct snd_soc_codec *codec)
{
  int i = 0;
  u16 tmp = 0;

  AUD_PRINTF(CODEC_LEVEL, "%s: codec = %s\n", __FUNCTION__, dev_name(codec->dev));
  InfinitySysInit();
  for (i = 0; i < AUD_REG_LEN; i++)
  {
    tmp = codec_reg[i];
    snd_soc_update_bits(codec, i, 0xff, codec_reg_backup[i]);
    codec_reg_backup[i] = tmp;
  }
#if 0
  snd_soc_dapm_enable_pin(&codec->dapm, "DMAWR1");
  snd_soc_dapm_enable_pin(&codec->dapm, "DMAWR2");
  snd_soc_dapm_enable_pin(&codec->dapm, "DMARD1");
  snd_soc_dapm_enable_pin(&codec->dapm, "DMARD2");
  snd_soc_dapm_enable_pin(&codec->dapm, "BTRX");
  snd_soc_dapm_enable_pin(&codec->dapm, "LINEIN");
  snd_soc_dapm_sync(&codec->dapm);
#endif

  return 0;
}

static int infinity_soc_codec_suspend(struct snd_soc_codec *codec)
{

  int i = 0;
  u16 tmp = 0;

  AUD_PRINTF(CODEC_LEVEL, "%s: codec = %s\n", __FUNCTION__, dev_name(codec->dev));

  for (i = 0; i < AUD_REG_LEN; i++)
  {
    tmp = codec_reg_backup[i];
    codec_reg_backup[i] = codec_reg[i];;
    codec_reg[i] = tmp;
  }

#if 0
  //snd_soc_dapm_disable_pin(&codec->dapm, "DMAWR1");
  //snd_soc_dapm_disable_pin(&codec->dapm, "DMAWR2");
  snd_soc_dapm_disable_pin(&codec->card->dapm, "DMARD1");
  snd_soc_dapm_disable_pin(&codec->card->dapm, "DMARD2");
  snd_soc_dapm_disable_pin(&codec->card->dapm, "BTRX");
  snd_soc_dapm_disable_pin(&codec->card->dapm, "LINEIN1");
  snd_soc_dapm_sync(&codec->card->dapm);
#endif

  return 0;
}

unsigned int infinity_codec_read(struct snd_soc_codec *codec, unsigned int reg)
{

  AUD_PRINTF(CODEC_LEVEL, "%s: codec = %s, reg = 0x%x, val = 0x%x\n",
                          __FUNCTION__, dev_name(codec->dev), reg, codec_reg[reg]);
  return codec_reg[reg];
}



int infinity_codec_write(struct snd_soc_codec *codec, unsigned int reg, unsigned int value)
{
  int ret = 0;

  AUD_PRINTF(CODEC_LEVEL, "%s: codec = %s, reg = 0x%x, val = 0x%x\n",
                          __FUNCTION__, dev_name(codec->dev), reg, value);
  switch(reg)
  {
  case AUD_PLAYBACK_MUX:
    if(codec_reg[reg]==2 && codec_reg[reg]^value)
      InfinitySineGenEnable(FALSE);

    if (value == 0)
    {
        //Switch Mux to DMA Reader
      InfinitySetMux2(BACH_MUX2_MMC1,1);
      InfinityDmaSetRate(BACH_DMA_READER1,InfinityRateFromU32(InfinityDmaGetRate(BACH_DMA_READER1)));
    }
    else if (value == 1)
    {
      //Switch Mux to ADC Input
      InfinitySetMux2(BACH_MUX2_MMC1,0);

      if(InfinityDmaIsWork(BACH_DMA_WRITER1))
      {
        //Set the same sample rate with dma writer
          InfinityDmaSetRate(BACH_DMA_READER1,InfinityRateFromU32(InfinityDmaGetRate(BACH_DMA_WRITER1)));
      }
      else
      {
 #ifdef DIGMIC_EN
          //set 16k sample rate, because 2M and 4M mode both support
          InfinityDigMicSetRate(BACH_RATE_16K);
          InfinityDmaSetRate(BACH_DMA_READER1,BACH_RATE_16K);
          InfinityDmaSetRate(BACH_DMA_WRITER1,BACH_RATE_16K);
 #else
        //Set 48k sample rate
          InfinityDmaSetRate(BACH_DMA_READER1,BACH_RATE_48K);
          InfinityDmaSetRate(BACH_DMA_WRITER1,BACH_RATE_48K);
 #endif
      }
    }
    else if (value == 2)
    {
      InfinityDmaSetRate(BACH_DMA_READER1,BACH_RATE_48K);
      InfinitySineGenEnable(TRUE);
    }
    else
      AUD_PRINTF(ERROR_LEVEL, "%s error parameter, reg = 0x%x, val = 0x%x\n",
                              __FUNCTION__, reg, value);
    break;

  case AUD_ADC_MUX:
    ret = snd_soc_codec_update_bits(codec, AUD_ATOP_PWR, 0x1, 0);

    if (value == 0 || value == 1)
    {
        codec_reg[reg] = value;
    }
    else
    {
      AUD_PRINTF(ERROR_LEVEL, "%s error parameter, reg = 0x%x, val = 0x%x\n",
                              __FUNCTION__, reg, value);
      return 0;
    }

    if (ret > 0)
      snd_soc_codec_update_bits(codec, AUD_ATOP_PWR, 0x1, 0x1);

    break;

  case AUD_ATOP_PWR:
    if ((codec_reg[reg] ^ value) & 0x1)
    {
        if(codec_reg[AUD_ADC_MUX]==0)
            (value & 0x1) ? InfinityOpenAtop(BACH_ATOP_LINEIN) : InfinityCloseAtop(BACH_ATOP_LINEIN);
        else
            (value & 0x1) ? InfinityOpenAtop(BACH_ATOP_MIC) : InfinityCloseAtop(BACH_ATOP_MIC);
    }
    if ((codec_reg[reg] ^ value) & 0x2)
    {
#ifdef CONFIG_OF
		if(nGpio!=-1)
		{
		    if(!(value & 0x2))
		    {
                gpio_direction_output(nGpio,!nOn);
		    }
		}
#endif
        (value & 0x2) ? InfinityOpenAtop(BACH_ATOP_LINEOUT) : InfinityCloseAtop(BACH_ATOP_LINEOUT);
#ifdef CONFIG_OF
		if(nGpio!=-1)
		{

		    if(value & 0x2)
		    {
                gpio_direction_output(nGpio,nOn);
		    }
		}
#endif
        infinity_audio_clk_disable(codec,1);
    }
    break;

  case AUD_DPGA_PWR:
    if ((codec_reg[reg] ^ value) & 0x1)
    {
      InfinitySetPathOnOff(BACH_PATH_PLAYBACK,(value & 0x1) ? TRUE : FALSE);
    }
    if ((codec_reg[reg] ^ value) & 0x2)
    {
      InfinitySetPathOnOff(BACH_PATH_CAPTURE,(value & 0x2) ? TRUE : FALSE);
    }
    break;

  case AUD_PLAYBACK_DPGA:
    AUD_PRINTF(TRACE_LEVEL, "Setup playback volume = %d\n", value);
    InfinitySetPathGain(BACH_PATH_PLAYBACK,(S8)(BACH_DPGA_GAIN_MIN_DB + value));
    break;
  case AUD_CAPTURE_DPGA:
    AUD_PRINTF(TRACE_LEVEL, "Setup capture volume  = %d\n", value);
    InfinitySetPathGain(BACH_PATH_CAPTURE,(S8)(BACH_DPGA_GAIN_MIN_DB + value));
    break;
  case AUD_MIC_GAIN:
    AUD_PRINTF(TRACE_LEVEL, "Setup MIC gain = %d\n", value);
    InfinityAtopAdcGain(value,BACH_ATOP_MIC);
    break;
  case AUD_MICPRE_GAIN:
    AUD_PRINTF(TRACE_LEVEL, "Setup MIC PRE gain = %d\n", value);
    InfinityAtopMicPreGain(value);
    break;
  case AUD_LINEIN_GAIN:
    AUD_PRINTF(TRACE_LEVEL, "Setup LineIn gain = %d\n", value);
    InfinityAtopAdcGain(value,BACH_ATOP_LINEIN);
    break;
  case AUD_DIGMIC_PWR:
    AUD_PRINTF(TRACE_LEVEL,"AUD_DIGMIC_PWR value = %d\n",value);
    if ((codec_reg[reg] ^ value))
    {
      if(!InfinityDigMicEnable(value)){
        AUD_PRINTF(ERROR_LEVEL, "%s DigMic Enable/Disable failed val = 0x%x\n",
                           __FUNCTION__, value);
        return -1;
      }
    }
    break;
  case AUD_DBG_SINERATE:
    AUD_PRINTF(TRACE_LEVEL,"AUD_DBG_SINERATE value = %d\n",value);
    if ((codec_reg[reg] ^ value))
    {
      if(!InfinitySineGenRate(value)){
        AUD_PRINTF(ERROR_LEVEL, "%s SineGen Rate failed val = 0x%x\n",
                           __FUNCTION__, value);
        return -1;
      }
    }
    break;
  case AUD_DBG_SINEGAIN:
    AUD_PRINTF(TRACE_LEVEL,"AUD_DBG_SINERATE value = %d\n",value);
    if ((codec_reg[reg] ^ value))
    {
      if(!InfinitySineGenGain(value)){
        AUD_PRINTF(ERROR_LEVEL, "%s SineGen Gain failed val = 0x%x\n",
                           __FUNCTION__, value);
        return -1;
      }
    }
    break;
  default:
    AUD_PRINTF(ERROR_LEVEL, "%s error parameter, reg = 0x%x, val = 0x%x\n",
                            __FUNCTION__, reg, value);
    break;
  }

  codec_reg[reg] = value;

  return 0;
}

static const unsigned int infinity_dpga_tlv[] =
{
  TLV_DB_RANGE_HEAD(1),
  0, 94, TLV_DB_LINEAR_ITEM(-64, 30),
};

static const char *infinity_chip[]     = {"iNfinity3_0.0"};

static const struct soc_enum infinity_chip_enum =
  SOC_ENUM_SINGLE(SND_SOC_NOPM, 0, 1, infinity_chip);



static const struct snd_kcontrol_new infinity_snd_controls[] =
{
  //TODO: Modify according volume or gain level
  SOC_ENUM(CHIP_VERSION, infinity_chip_enum),

  SOC_SINGLE_TLV(MAIN_PLAYBACK_VOLUME, AUD_PLAYBACK_DPGA, 0, 94, 0, infinity_dpga_tlv),
  SOC_SINGLE_TLV(MAIN_CAPTURE_VOLUME, AUD_CAPTURE_DPGA, 0, 94, 0, infinity_dpga_tlv),
  SOC_SINGLE_TLV(MICIN_GAIN_LEVEL, AUD_MIC_GAIN, 0, 7, 0, NULL),
  SOC_SINGLE_TLV(MICIN_PREGAIN_LEVEL, AUD_MICPRE_GAIN, 0, 3, 0, NULL),
  SOC_SINGLE_TLV(LINEIN_GAIN_LEVEL, AUD_LINEIN_GAIN, 0, 7, 0, NULL),
  SOC_SINGLE_TLV(SINEGEN_GAIN_LEVEL, AUD_DBG_SINEGAIN, 0, 4, 0, NULL),
  SOC_SINGLE_RANGE(SINEGEN_RATE_SELECT, AUD_DBG_SINERATE, 0, 0, 10, 0)

};

static const char *infinity_output_select[]  = {"DMA Reader", "ADC In", "Sine Gen"};
static const char *infinity_adc_select[]     = {"Line-in", "Mic-in"};


static const struct soc_enum infinity_outsel_enum =
  SOC_ENUM_SINGLE(AUD_PLAYBACK_MUX, 0, 3, infinity_output_select);

static const struct soc_enum infinity_adcsel_enum =
  SOC_ENUM_SINGLE(AUD_ADC_MUX, 0, 2, infinity_adc_select);

static const struct snd_kcontrol_new infinity_output_mux_controls =
  SOC_DAPM_ENUM("Playback Select", infinity_outsel_enum);

static const struct snd_kcontrol_new infinity_adc_mux_controls =
  SOC_DAPM_ENUM("ADC Select", infinity_adcsel_enum);


static const struct snd_soc_dapm_widget infinity_dapm_widgets[] =
{
  SND_SOC_DAPM_MUX(MAIN_PLAYBACK_MUX, SND_SOC_NOPM, 0, 0, &infinity_output_mux_controls),

  SND_SOC_DAPM_OUTPUT("LINEOUT"),

  SND_SOC_DAPM_AIF_OUT("DMAWR", "Main Capture",   0, SND_SOC_NOPM, 0, 0),
  SND_SOC_DAPM_AIF_IN("DMARD", "Main Playback",   0, SND_SOC_NOPM, 0, 0),
  SND_SOC_DAPM_AIF_IN("DIGMIC", NULL,   0, AUD_DIGMIC_PWR, 0, 0),
  SND_SOC_DAPM_SIGGEN("SINEGEN"),

  //SND_SOC_DAPM_INPUT("DMARD"),
  SND_SOC_DAPM_INPUT("LINEIN"),
  SND_SOC_DAPM_INPUT("MICIN"),

  SND_SOC_DAPM_DAC("DAC",   NULL, AUD_ATOP_PWR, 1, 0),
  SND_SOC_DAPM_ADC("ADC",   NULL, AUD_ATOP_PWR, 0, 0),

  SND_SOC_DAPM_PGA("Main Playback DPGA", AUD_DPGA_PWR, 0, 0, NULL, 0),
  SND_SOC_DAPM_PGA("Main Capture DPGA",  AUD_DPGA_PWR, 1, 0, NULL, 0),

  SND_SOC_DAPM_MUX(ADC_MUX, SND_SOC_NOPM, 0, 0, &infinity_adc_mux_controls),

};

static const struct snd_soc_dapm_route infinity_codec_routes[] =
{

  {"Main Playback Mux", "DMA Reader", "DMARD"},
#ifdef DIGMIC_EN
  {"Main Playback Mux", "ADC In",     "DIGMIC"},
#else
  {"Main Playback Mux", "ADC In",     "ADC"},
#endif
  {"Main Playback Mux", "Sine Gen",   "SINEGEN"},
  {"Main Playback DPGA", NULL, "Main Playback Mux"},

  {"DAC",     NULL, "Main Playback DPGA"},

  {"LINEOUT", NULL, "DAC"},

  {"DMAWR", NULL, "Main Capture DPGA"},
#ifdef DIGMIC_EN
  {"Main Capture DPGA", NULL, "DIGMIC"},
  {"DIGMIC", NULL, "MICIN"}
#else
  {"Main Capture DPGA", NULL, "ADC"},

  {"ADC",     NULL, "ADC Mux"},

  {"ADC Mux", "Line-in", "LINEIN"},
  {"ADC Mux", "Mic-in",  "MICIN"}
#endif
};

static struct snd_soc_codec_driver infinity_soc_codec_drv =
{
  .probe =    infinity_soc_codec_probe,
  .remove =   infinity_soc_codec_remove,
  .suspend =  infinity_soc_codec_suspend,
  .resume =   infinity_soc_codec_resume,
  .write = infinity_codec_write,
  .read  = infinity_codec_read,

  .dapm_widgets = infinity_dapm_widgets,
  .num_dapm_widgets = ARRAY_SIZE(infinity_dapm_widgets),
  .dapm_routes = infinity_codec_routes,
  .num_dapm_routes = ARRAY_SIZE(infinity_codec_routes),
  .controls =	infinity_snd_controls,
  .num_controls = ARRAY_SIZE(infinity_snd_controls),
};

static int infinity_codec_probe(struct platform_device *pdev)
{

  u32 val;
  int ret;
  u32 array[2];
  struct device_node *node = pdev->dev.of_node; //(struct device_node *)platform_get_drvdata(pdev);

  AUD_PRINTF(TRACE_LEVEL, "%s enter\r\n", __FUNCTION__);


  ret = of_property_read_u32(node, "playback-volume-level", &val);
  if (ret == 0)
  {
    AUD_PRINTF(TRACE_LEVEL, "Get playback-volume-level = %d\n", val);
    codec_reg_backup[AUD_PLAYBACK_DPGA] = val;
  }

  ret = of_property_read_u32(node, "capture-volume-level", &val);
  if (ret == 0)
  {
    AUD_PRINTF(TRACE_LEVEL, "Get capture-volume-level = %d\n", val);
    codec_reg_backup[AUD_CAPTURE_DPGA] = val;
  }

  ret = of_property_read_u32(node, "micin-gain-level", &val);
  if (ret == 0)
  {
    AUD_PRINTF(TRACE_LEVEL, "Get micin-gain-level = %d\n", val);
    codec_reg_backup[AUD_MIC_GAIN] = val;
  }

  ret = of_property_read_u32(node, "micin-pregain-level", &val);
  if (ret == 0)
  {
    AUD_PRINTF(TRACE_LEVEL, "Get micin-pregain-level = %d\n", val);
    codec_reg_backup[AUD_MICPRE_GAIN] = val;
  }

  ret = of_property_read_u32(node, "linein-gain-level", &val);
  if (ret == 0)
  {
    AUD_PRINTF(TRACE_LEVEL, "Get linein-gain-level = %d\n", val);
    codec_reg_backup[AUD_LINEIN_GAIN] = val;
  }

  //  enable for line-out
  ret = of_property_read_u32_array(node, "amp-gpio", array, 2 );
  if (ret == 0)
  {
	ret = gpio_request(array[0],"amp-gpio");
	if(ret == 0)
	{
	  nGpio = array[0];
	  nOn = array[1];
		//gpio_direction_output(array[0],array[1]);
	  gpio_direction_output(nGpio,!nOn);
      AUD_PRINTF(TRACE_LEVEL, "Get amp-gpio = %d %d\n", array[0], array[1]);
	}
	else
	  nGpio = -1;
  }

  //dev_set_name(&pdev->dev, "infinity-codec");

  return snd_soc_register_codec(&pdev->dev, &infinity_soc_codec_drv,
                                infinity_soc_codec_dai_drv, ARRAY_SIZE(infinity_soc_codec_dai_drv));
}

static int infinity_codec_remove(struct platform_device *pdev)
{
  AUD_PRINTF(TRACE_LEVEL, "%s enter\r\n", __FUNCTION__);
  snd_soc_unregister_codec(&pdev->dev);
  return 0;
}

static struct platform_driver infinity_codec_driver =
{
  .probe = infinity_codec_probe,
  .remove = infinity_codec_remove,

  .driver = {
    .name = "infinity-codec",
    .owner = THIS_MODULE,
  },
};

#ifndef CONFIG_OF
module_platform_driver(infinity_codec_driver);
#else
static struct platform_device *infinity_codec_device = NULL;
static int __init infinity_codec_init(void)
{

  int ret = 0;

  struct device_node *np;

  AUD_PRINTF(TRACE_LEVEL, "%s\r\n", __FUNCTION__)

  infinity_codec_device = platform_device_alloc("infinity-codec", -1);
  if (!infinity_codec_device)
  {
    AUD_PRINTF(ERROR_LEVEL, "%s: platform_device_alloc infinity-codec error\r\n", __FUNCTION__);
    return -ENOMEM;
  }

  np = of_find_compatible_node(NULL, NULL, "mstar,infinity3-audio");
  if (np)
  {
    infinity_codec_device->dev.of_node = of_node_get(np);
    of_node_put(np);
  }

  ret = platform_device_add(infinity_codec_device);
  if (ret)
  {
    AUD_PRINTF(ERROR_LEVEL, "%s: platform_device_add infinity_codec_device error\r\n", __FUNCTION__);
    platform_device_put(infinity_codec_device);
  }

  ret = platform_driver_register(&infinity_codec_driver);
  if (ret)
  {
    AUD_PRINTF(ERROR_LEVEL, "%s: platform_driver_register infinity_codec_driver error\r\n", __FUNCTION__);
    platform_device_unregister(infinity_codec_device);
  }

  return ret;
}

static void __exit infinity_codec_exit(void)
{
  AUD_PRINTF(TRACE_LEVEL, "%s\r\n", __FUNCTION__);
  platform_device_unregister(infinity_codec_device);
  platform_driver_unregister(&infinity_codec_driver);
}

module_init(infinity_codec_init);
module_exit(infinity_codec_exit);

#endif
MODULE_LICENSE("GPL");

/* Module information */
MODULE_AUTHOR("Trevor Wu, trevor.wu@mstarsemi.com");
MODULE_DESCRIPTION("Infinity3 Bach Audio ALSA SoC Codec");
