/*------------------------------------------------------------------------------
 *   Copyright (c) 2008 MStar Semiconductor, Inc.  All rights reserved.
 *------------------------------------------------------------------------------*/
//------------------------------------------------------------------------------
//  Include files
//------------------------------------------------------------------------------
#include <linux/module.h>
#include <linux/moduleparam.h>
//#include <linux/timer.h>
//#include <linux/interrupt.h>
#include <linux/platform_device.h>
//#include <linux/i2c.h>

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>

//#include <sound/driver.h>  remove it in kernel 2.6.29.1
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

//#include <asm/mach-types.h>
//#include <asm/hardware/scoop.h>
//#include <mach/hardware.h>
//#include <mach/platform.h>
//#include <asm/io.h>
//#include <asm/uaccess.h>


#include "ms_platform.h"

#include "infinity_pcm.h"
#include "infinity_codec.h"
#include "infinity_dai.h"
#include "bach_audio_debug.h"
#include "infinity.h"

static int infinity_soc_dai_link_init(struct snd_soc_pcm_runtime *rtd)
{

  AUD_PRINTF(TRACE_LEVEL, "%s: rtd = %s\r\n", __FUNCTION__, rtd->dai_link->name);

  return 0;
}

static int infinity_soc_dai_link_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params)
{

  AUD_PRINTF(TRACE_LEVEL, "%s\r\n", __FUNCTION__);

  return 0;
}

//unsigned int BACH_BASE_ADDR = 0;

static int infinity_soc_card_probe(struct snd_soc_card *card)
{

  AUD_PRINTF(TRACE_LEVEL, "%s\r\n", __FUNCTION__);


  InfinitySetBankBaseAddr(IO_ADDRESS(BACH_RIU_BASE_ADDR));
  AUD_PRINTF(TRACE_LEVEL, "Setup AudioSetBankBaseAddr = 0x%x\n", IO_ADDRESS(BACH_RIU_BASE_ADDR));

  return 0;
}

#if 0
static int infinity_soc_card_late_probe(struct snd_soc_card *card)
{
  snd_soc_dapm_enable_pin(&card->dapm, "DMA RD2");
  snd_soc_dapm_enable_pin(&card->dapm, "DMA RD1");
  snd_soc_dapm_enable_pin(&card->dapm, "DBB I2S TRX");
  snd_soc_dapm_enable_pin(&card->dapm, "BT I2S RX");
  snd_soc_dapm_sync(&card->dapm);

  return 0;
}
#endif

static unsigned int stream_playback_active = 0;
static unsigned int stream_capture_active = 0;
extern void snd_soc_dapm_stream_event(struct snd_soc_pcm_runtime *rtd, int stream, int event);

static int infinity_soc_card_suspend_pre(struct snd_soc_card *card)
{

#if 1
  int i;
  struct snd_soc_dapm_widget *w;

  AUD_PRINTF(TRACE_LEVEL, "%s\r\n", __FUNCTION__);


  for (i = 0; i < card->num_rtd; i++)
  {
    struct snd_soc_dai *codec_dai = card->rtd[i].codec_dai;

    if (card->rtd[i].dai_link->ignore_suspend)
      continue;

    w = codec_dai->playback_widget;
    if (w->active)
    {
      AUD_PRINTF(TRACE_LEVEL, "snd_soc_dapm_stream_event(): stop PLAYBACK %d\n", i);
      snd_soc_dapm_stream_event(&card->rtd[i],
                                SNDRV_PCM_STREAM_PLAYBACK,
                                SND_SOC_DAPM_STREAM_STOP);

      stream_playback_active |= 1 << i;

    }
    w = codec_dai->capture_widget;
    if (w->active)
    {
      AUD_PRINTF(TRACE_LEVEL, "snd_soc_dapm_stream_event(): stop CAPTURE %d\n", i);
      snd_soc_dapm_stream_event(&card->rtd[i],
                                SNDRV_PCM_STREAM_CAPTURE,
                                SND_SOC_DAPM_STREAM_STOP);

      stream_capture_active |= 1 << i;

    }
  }

#endif


#if 1
  snd_soc_dapm_disable_pin(&card->dapm, "DMARD");
  snd_soc_dapm_disable_pin(&card->dapm, "LINEIN");
  snd_soc_dapm_sync(&card->dapm);
#endif

  return 0;
}

static int infinity_soc_card_suspend_post(struct snd_soc_card *card)
{
  AUD_PRINTF(TRACE_LEVEL, "%s\r\n", __FUNCTION__);

  return 0;

}

static int infinity_soc_card_resume_pre(struct snd_soc_card *card)
{
  AUD_PRINTF(TRACE_LEVEL, "%s\r\n", __FUNCTION__);

  return 0;

}

static int infinity_soc_card_resume_post(struct snd_soc_card *card)
{
  int i;

  AUD_PRINTF(TRACE_LEVEL, "%s\n", __FUNCTION__);

#if 1
  snd_soc_dapm_enable_pin(&card->dapm, "DMARD");
  snd_soc_dapm_enable_pin(&card->dapm, "LINEIN");
  snd_soc_dapm_sync(&card->dapm);
#endif


#if 1
  for (i = 0; i < card->num_rtd; i++)
  {
    //struct snd_soc_dai *codec_dai = card->rtd[i].codec_dai;

    if (stream_playback_active & (1 << i))
    {
      AUD_PRINTF(TRACE_LEVEL, "snd_soc_dapm_stream_event(): start PLAYBACK %d\n", i);

      snd_soc_dapm_stream_event(&card->rtd[i],
                                SNDRV_PCM_STREAM_PLAYBACK,
                                SND_SOC_DAPM_STREAM_START);

      stream_playback_active &= ~(1 << i);

    }

    if (stream_capture_active & (1 << i))
    {
      AUD_PRINTF(TRACE_LEVEL, "snd_soc_dapm_stream_event(): start CAPTURE %d\n", i);
      snd_soc_dapm_stream_event(&card->rtd[i],
                                SNDRV_PCM_STREAM_CAPTURE,
                                SND_SOC_DAPM_STREAM_START);

      stream_capture_active &= ~(1 << i);

    }
  }
#endif

  return 0;

}

static struct snd_soc_ops infinity_soc_ops =
{
  .hw_params	= infinity_soc_dai_link_hw_params,
};

static struct snd_soc_dai_link infinity_soc_dais[] =
{
  {
    .name           = "Infinity Soc Dai Link",
    .codec_name     = "infinity-codec",
    .codec_dai_name = "infinity-codec-dai-main",
    .cpu_dai_name   = "infinity-cpu-dai",
    .platform_name  = "infinity-platform",

    .init			= infinity_soc_dai_link_init,

    .ops			= &infinity_soc_ops,
  },
};

//static const struct snd_soc_dapm_widget infinity_card_dapm_widgets[] =
//{
//  SND_SOC_DAPM_LINE("DMA RD1", NULL),
//};

//static const struct snd_soc_dapm_route infinity_card_dapm_routes[] =
//{
//  {"DMARD1", NULL, "DMA RD1"},
//
//};


struct snd_soc_card infinity_soc_card =
{
  .name       = "infinity_snd_machine",
  .owner      = THIS_MODULE,
  .dai_link	  = infinity_soc_dais,
  .num_links  = ARRAY_SIZE(infinity_soc_dais),
  .probe      = infinity_soc_card_probe,

  .suspend_pre = infinity_soc_card_suspend_pre,
  .suspend_post = infinity_soc_card_suspend_post,
  .resume_pre = infinity_soc_card_resume_pre,
  .resume_post = infinity_soc_card_resume_post,
};
EXPORT_SYMBOL_GPL(infinity_soc_card);

#ifndef CONFIG_OF

static struct platform_device *infinity_snd_device = NULL;
static struct platform_device *infinity_cpu_dai_device = NULL;
static struct platform_device *infinity_platform_device = NULL;
static struct platform_device *infinity_codec_device = NULL;

static int __init infinity_snd_init(void)
{
  int ret = 0;

  AUD_PRINTF(TRACE_LEVEL, "%s\r\n", __FUNCTION__);

  //----------------------------------------------------------------
  infinity_codec_device = platform_device_alloc("infinity-codec", -1);
  if (!infinity_codec_device)
  {
    AUD_PRINTF(ERROR_LEVEL, "%s: platform_device_alloc infinity-codec error\r\n", __FUNCTION__);
    return -ENOMEM;
  }
  ret = platform_device_add(infinity_codec_device);
  if (ret)
  {
    AUD_PRINTF(ERROR_LEVEL, "%s: platform_device_add infinity_codec_device error\r\n", __FUNCTION__);
    platform_device_put(infinity_codec_device);
  }

  //----------------------------------------------------------------
  infinity_platform_device = platform_device_alloc("infinity-platform", -1);
  if (!infinity_platform_device)
  {
    AUD_PRINTF(ERROR_LEVEL, "%s: platform_device_alloc infinity-platform error\r\n", __FUNCTION__);
    return -ENOMEM;
  }
  ret = platform_device_add(infinity_platform_device);
  if (ret)
  {
    AUD_PRINTF(ERROR_LEVEL, "%s: platform_device_add infinity_platform_device error\r\n", __FUNCTION__);
    platform_device_put(infinity_platform_device);
  }

  //----------------------------------------------------------------
  infinity_cpu_dai_device = platform_device_alloc("infinity-cpu-dai", -1);
  if (!infinity_cpu_dai_device)
  {
    AUD_PRINTF(ERROR_LEVEL, "%s: platform_device_alloc infinity-cpu-dai error\r\n", __FUNCTION__);
    return -ENOMEM;
  }
  ret = platform_device_add(infinity_cpu_dai_device);
  if (ret)
  {
    AUD_PRINTF(ERROR_LEVEL, "%s: platform_device_add infinity_cpu_dai_device error\r\n", __FUNCTION__);
    platform_device_put(infinity_cpu_dai_device);
  }

  //----------------------------------------------------------------
  infinity_snd_device = platform_device_alloc("soc-audio", -1);
  if (!infinity_snd_device)
  {
    AUD_PRINTF(ERROR_LEVEL, "%s: platform_device_alloc soc-audio error\r\n", __FUNCTION__);
    return -ENOMEM;
  }
  platform_set_drvdata(infinity_snd_device, &infinity_soc_card);
  ret = platform_device_add(infinity_snd_device);
  if (ret)
  {
    AUD_PRINTF(ERROR_LEVEL, "%s: platform_device_add infinity_snd_device error\r\n", __FUNCTION__);
    platform_device_put(infinity_snd_device);
  }

  return ret;
}

static void __exit infinity_snd_exit(void)
{
  AUD_PRINTF(TRACE_LEVEL, "%s\r\n", __FUNCTION__);
  platform_device_unregister(infinity_snd_device);
  platform_device_unregister(infinity_cpu_dai_device);
  platform_device_unregister(infinity_platform_device);
  platform_device_unregister(infinity_codec_device);
}

module_init(infinity_snd_init);
module_exit(infinity_snd_exit);

#else
static int infinity_audio_probe(struct platform_device *pdev)
{

  AUD_PRINTF(TRACE_LEVEL, "%s\r\n", __FUNCTION__);


  infinity_soc_card.dev = &pdev->dev;


  return snd_soc_register_card(&infinity_soc_card);
}

int infinity_audio_remove(struct platform_device *pdev)
{
  AUD_PRINTF(TRACE_LEVEL, "%s\r\n", __FUNCTION__);
  return snd_soc_unregister_card(&infinity_soc_card);
}

static const struct of_device_id infinity_audio_of_match[] = {
	{ .compatible = "mstar,infinity3-audio", },
	{},
};
MODULE_DEVICE_TABLE(of, infinity_audio_of_match);

static struct platform_driver infinity_audio = {
	.driver = {
		.name	= "infinity-audio",
		.owner = THIS_MODULE,
		.pm     = &snd_soc_pm_ops,
		.of_match_table = infinity_audio_of_match,
	},
	.probe		= infinity_audio_probe,
	.remove   = infinity_audio_remove,
};

module_platform_driver(infinity_audio);
#endif



/* Module information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Trevor Wu, trevor.wu@mstarsemi.com");
MODULE_DESCRIPTION("iNfinity3 Bach Audio ASLA SoC Machine");

