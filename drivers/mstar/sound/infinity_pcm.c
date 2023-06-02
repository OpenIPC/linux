/*------------------------------------------------------------------------------
 *   Copyright (c) 2008 MStar Semiconductor, Inc.  All rights reserved.
 *  ------------------------------------------------------------------------------*/
//------------------------------------------------------------------------------
//  Include Files
//------------------------------------------------------------------------------
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>

#include <linux/delay.h>
#include <linux/spinlock.h>

//#include <sound/driver.h>   remove it for kernel 2.6.29.1
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>


#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include <asm/dma.h>
#include <asm/io.h>
#include <ms_platform.h>
#include "ms_msys.h"

#include "infinity_codec.h"
#include "infinity_pcm.h"
#include "bach_audio_debug.h"
#include "infinity.h"
#include "mdrv_msys_io.h"


#ifdef CONFIG_OF
static struct platform_device *infinity_dma_device = NULL;
static BOOL fClk=FALSE;
static BOOL isDmaWork[2]={FALSE,FALSE};
#endif


//------------------------------------------------------------------------------
//  Macros
//------------------------------------------------------------------------------

#define params_period_bytes(p) \
	(hw_param_interval_c((p), SNDRV_PCM_HW_PARAM_PERIOD_BYTES)->min)

//------------------------------------------------------------------------------
//  Variables
//------------------------------------------------------------------------------
struct infinity_pcm_runtime_data
{
  spinlock_t            lock;
  U32                   state;
  size_t                dma_level_count;
  size_t                int_level_count;
  struct infinity_pcm_dma_data *dma_data;
  void *private_data;
  size_t                remain_count;
};

static struct snd_pcm_hardware infinity_pcm_playback_hardware =
{
  .info				= SNDRV_PCM_INFO_INTERLEAVED,
  .formats			= SNDRV_PCM_FMTBIT_S16_LE,
  .rates			= SNDRV_PCM_RATE_8000_48000,
  .rate_min			= 8000,
  .rate_max			= 48000,
  .channels_min		= 1,
  .channels_max		= 2,
  .buffer_bytes_max	= 96 * 1024,
  .period_bytes_min	= 2 * 1024,
  .period_bytes_max	= 24 * 1024,
  .periods_min		= 4,
  .periods_max		= 8,
  .fifo_size		= 32,
};

static struct snd_pcm_hardware infinity_pcm_capture_hardware =
{
  .info				= SNDRV_PCM_INFO_INTERLEAVED,
  .formats			= SNDRV_PCM_FMTBIT_S16_LE,
  .rates			= SNDRV_PCM_RATE_8000_48000,
  .rate_min			= 8000,
  .rate_max			= 48000,
  .channels_min		= 1,
  .channels_max		= 2,
  .buffer_bytes_max	= 80 * 1024,
  .period_bytes_min	= 1 * 1024,
  .period_bytes_max	= 10 * 1024,
  .periods_min		= 4,
  .periods_max		= 16,
  .fifo_size		= 32,
};


unsigned int INFINITY_IRQ_ID = 0; //INT_IRQ_AU_SYSTEM;
//------------------------------------------------------------------------------
//  Function definition
//------------------------------------------------------------------------------

unsigned long long g_nPlayStartTime = 0;
unsigned long long g_nCapStartTime = 0;
EXPORT_SYMBOL_GPL(g_nPlayStartTime);
EXPORT_SYMBOL_GPL(g_nCapStartTime);

//------------------------------------------------------------------------------
//  Function
//------------------------------------------------------------------------------

static irqreturn_t  infinity_pcm_dma_irq(int irq, void *dev_id)
{
  struct snd_pcm_substream *substream = (struct snd_pcm_substream *)dev_id;
  struct snd_pcm_runtime *runtime = substream->runtime;
  struct infinity_pcm_runtime_data *prtd = runtime->private_data;
  struct infinity_pcm_dma_data *dma_data = prtd->dma_data;

  unsigned long flags,fgs;
  unsigned int state = 0;
  //unsigned long time;
  //snd_pcm_uframes_t offset;
 // AUD_PRINTF(PLAYBACK_IRQ_LEVEL, "in %s,status 0x%x\n",  __func__,InfinityReadReg2Byte(0x1009bc));
  spin_lock_irqsave(&prtd->lock, flags);
  if ((SNDRV_PCM_STREAM_PLAYBACK == substream->stream) && snd_pcm_running(substream))
  {
    if (prtd->state != DMA_EMPTY)
    {
      //time = jiffies_to_msecs(jiffies);

      if (InfinityDmaIsEmpty((BachDmaChannel_e)dma_data->channel))
      {
        prtd->state = DMA_EMPTY;

        spin_unlock_irqrestore(&prtd->lock, flags);

		snd_pcm_stream_lock_irqsave(substream, fgs);
		if (snd_pcm_running(substream))
			snd_pcm_stop(substream, SNDRV_PCM_STATE_XRUN);
        snd_pcm_stream_unlock_irqrestore(substream, fgs);

        AUD_PRINTF(PLAYBACK_IRQ_LEVEL, "EMPTY: chanId = %d, T = %d\n",  dma_data->channel, (unsigned int)jiffies_to_msecs(jiffies));
        spin_lock_irqsave(&prtd->lock, flags);

      }
      else if ((prtd->state != DMA_UNDERRUN) && InfinityDmaIsUnderrun((BachDmaChannel_e)dma_data->channel))
      {

        //BachDmaMaskInt(dma_data->channel, BACH_DMA_INT_OVERRUN, FALSE);
        InfinityDmaMaskInt((BachDmaChannel_e)dma_data->channel, BACH_DMA_INT_UNDERRUN, TRUE);

        prtd->state = DMA_UNDERRUN;

        spin_unlock_irqrestore(&prtd->lock, flags);
        snd_pcm_period_elapsed(substream);
       // AUD_PRINTF(PLAYBACK_IRQ_LEVEL, "UNDER: chanId = %d, frameCnt = 0x%x, T = %d\n",
       //            dma_data->channel, (unsigned int)bytes_to_frames(runtime, InfinityDmaGetLevelCnt(dma_data->channel)), (unsigned int)jiffies_to_msecs(jiffies));
        spin_lock_irqsave(&prtd->lock, flags);
      }
    }
  }
  else if ((SNDRV_PCM_STREAM_CAPTURE == substream->stream) && snd_pcm_running(substream))  // CAPTURE device
  {
    state = prtd->state;

    if ((prtd->state != DMA_UNDERRUN) && InfinityDmaIsUnderrun((BachDmaChannel_e)dma_data->channel))
    {

      InfinityDmaMaskInt((BachDmaChannel_e)dma_data->channel, BACH_DMA_INT_UNDERRUN, TRUE);
      InfinityDmaMaskInt((BachDmaChannel_e)dma_data->channel, BACH_DMA_INT_OVERRUN, FALSE);

      prtd->state = DMA_UNDERRUN;
      spin_unlock_irqrestore(&prtd->lock, flags);
      AUD_PRINTF(PCM_DEBUG_LEVEL, "UNDER: chanId = %d, previous state = %d\n", dma_data->channel, state);
      spin_lock_irqsave(&prtd->lock, flags);

    }
    else if (prtd->state != DMA_FULL)
    {
      if (InfinityDmaIsFull((BachDmaChannel_e)dma_data->channel))
      {
        InfinityDmaMaskInt((BachDmaChannel_e)dma_data->channel, BACH_DMA_INT_OVERRUN, TRUE);
        InfinityDmaMaskInt((BachDmaChannel_e)dma_data->channel, BACH_DMA_INT_UNDERRUN, FALSE);

        prtd->state = DMA_FULL;

        spin_unlock_irqrestore(&prtd->lock, flags);

		snd_pcm_stream_lock_irqsave(substream, fgs);
	    if (snd_pcm_running(substream))
		    snd_pcm_stop(substream, SNDRV_PCM_STATE_XRUN);
        snd_pcm_stream_unlock_irqrestore(substream, fgs);

        AUD_PRINTF(CAPTURE_IRQ_LEVEL, "FULL: chanId = %d, previous state = %d\n", dma_data->channel, state);
        spin_lock_irqsave(&prtd->lock, flags);
      }
      else if ((prtd->state != DMA_OVERRUN) && InfinityDmaIsOverrun((BachDmaChannel_e)dma_data->channel))
      {
        InfinityDmaMaskInt((BachDmaChannel_e)dma_data->channel, BACH_DMA_INT_OVERRUN, TRUE);
        InfinityDmaMaskInt((BachDmaChannel_e)dma_data->channel, BACH_DMA_INT_UNDERRUN, FALSE);

        prtd->state = DMA_OVERRUN;

        spin_unlock_irqrestore(&prtd->lock, flags);
        snd_pcm_period_elapsed(substream);

        AUD_PRINTF(PCM_DEBUG_LEVEL, "OVER: chanId = %d, previous state = %d, remainder = 0x%x\n",
                   dma_data->channel, state, (unsigned int)bytes_to_frames(runtime, InfinityDmaGetLevelCnt((BachDmaChannel_e)dma_data->channel)));
        spin_lock_irqsave(&prtd->lock, flags);
      }
    }
  }
  spin_unlock_irqrestore(&prtd->lock, flags);

 // AUD_PRINTF(TRACE_LEVEL, "!!BACH_DMA1_CTRL_0 = 0x%x,  BACH_DMA1_CTRL_8 = 0x%x, level count = %d\n",
 // 		InfinityReadReg(BACH_REG_BANK1, BACH_DMA1_CTRL_0), InfinityReadReg(BACH_REG_BANK1, BACH_DMA1_CTRL_8), InfinityDmaGetLevelCnt(prtd->dma_data->channel));

 // if(prtd->state == DMA_EMPTY)
 //   AUD_PRINTF(PLAYBACK_IRQ_LEVEL, "Out of infinity_pcm_dma_irq\n");

  return IRQ_HANDLED;
}

static int infinity_pcm_open(struct snd_pcm_substream *substream)
{
  struct snd_pcm_runtime *runtime = substream->runtime;
  struct snd_soc_pcm_runtime *rtd = substream->private_data;
  struct snd_soc_dai_link *dai_link = rtd->dai_link;
  struct snd_pcm *pcm = rtd->pcm;

  struct infinity_pcm_runtime_data *prtd;
  int ret = 0;

  AUD_PRINTF(PCM_LEVEL, "%s: stream = %s, pcmC0D%d (substream = %s), dai_link = %s\n",
                        __FUNCTION__, (substream->stream == SNDRV_PCM_STREAM_PLAYBACK ? "PLAYBACK" : "CAPTURE"),
                        pcm->device, substream->name, dai_link->name);

  if (SNDRV_PCM_STREAM_PLAYBACK == substream->stream)
  {
    snd_soc_set_runtime_hwparams(substream, &infinity_pcm_playback_hardware);
  }
  else if (SNDRV_PCM_STREAM_CAPTURE == substream->stream)
  {
    snd_soc_set_runtime_hwparams(substream, &infinity_pcm_capture_hardware);
  }
  else
  {
    ret = -EINVAL;
    goto out;
  }
  /* Ensure that buffer size is a multiple of period size */
#if 0
  ret = snd_pcm_hw_constraint_integer(runtime,
                                      SNDRV_PCM_HW_PARAM_PERIODS);
  if (ret < 0)
    goto out;
#endif

  prtd = kzalloc(sizeof(*prtd), GFP_KERNEL);
  if (prtd == NULL)
  {
    ret = -ENOMEM;
    goto out;
  }

  spin_lock_init(&prtd->lock);
  runtime->private_data = prtd;

out:
  return ret;
}

static int infinity_pcm_close(struct snd_pcm_substream *substream)
{
  struct snd_pcm_runtime *runtime = substream->runtime;

  AUD_PRINTF(PCM_LEVEL, "%s: stream = %s\n", __FUNCTION__,
             (substream->stream == SNDRV_PCM_STREAM_PLAYBACK ? "PLAYBACK" : "CAPTURE"));

  kfree(runtime->private_data); //free infinity_pcm_runtime_data
  runtime->private_data = NULL;

  return 0;
}

extern int infinity_audio_clk_enable(void)
{
#ifdef CONFIG_OF
  int num_parents, i;
  struct clk **snd_clks;

  if(!fClk){
    fClk = TRUE;
    AUD_PRINTF(PCM_LEVEL, "enable clk!!!!\n");
    num_parents = of_clk_get_parent_count(infinity_dma_device->dev.of_node);
    if(num_parents>0){
      snd_clks = kzalloc((sizeof(struct clk *) * num_parents), GFP_KERNEL);
      if(!snd_clks)
      {
        AUD_PRINTF(ERROR_LEVEL, "[%s] Fail to kzalloc!\n", __FUNCTION__ );
        return 0;
      }
      for(i = 0; i < num_parents; i++){
        snd_clks[i] = of_clk_get(infinity_dma_device->dev.of_node, i);
        if (IS_ERR(snd_clks[i])){
          AUD_PRINTF(ERROR_LEVEL, "[%s] Fail to get clk!\n", __FUNCTION__ );
          kfree(snd_clks);
          return -EINVAL;
        }else{
          clk_prepare_enable(snd_clks[i]);
        }
      }
      kfree(snd_clks);
    }
  }
#endif
  return 0;
}

extern int infinity_audio_clk_disable(struct snd_soc_codec *codec, int mode)
{
#ifdef CONFIG_OF
  int num_parents, i;
  struct clk **snd_clks;
  u16 nValue;

  if(!(InfinityDmaIsWork(BACH_DMA_READER1)||InfinityDmaIsWork(BACH_DMA_WRITER1)
    || isDmaWork[SNDRV_PCM_STREAM_PLAYBACK] || isDmaWork[SNDRV_PCM_STREAM_CAPTURE])
    && fClk){
    if(mode==0)
    {
        nValue = snd_soc_read(codec,AUD_ATOP_PWR);
        if(nValue & 0x2)
            return 0;
    }
    fClk = FALSE;
    AUD_PRINTF(PCM_LEVEL, "disable clk!!!!\n");
    num_parents = of_clk_get_parent_count(infinity_dma_device->dev.of_node);
    if(num_parents>0){
      snd_clks = kzalloc((sizeof(struct clk *) * num_parents), GFP_KERNEL);
      if(!snd_clks)
      {
        AUD_PRINTF(ERROR_LEVEL, "[%s] Fail to kzalloc!\n", __FUNCTION__ );
        return 0;
      }
      for(i = 0; i < num_parents; i++){
        snd_clks[i] = of_clk_get(infinity_dma_device->dev.of_node, i);
        if (IS_ERR(snd_clks[i])){
          AUD_PRINTF(ERROR_LEVEL, "[%s] Fail to get clk!\n", __FUNCTION__ );
          kfree(snd_clks);
          return -EINVAL;
        }else{
          clk_disable_unprepare(snd_clks[i]);
        }
      }
      kfree(snd_clks);
    }
  }
#endif
  return 0;
}

EXPORT_SYMBOL_GPL(infinity_audio_clk_disable);


/* this may get called several times by oss emulation */
static int infinity_pcm_hw_params(struct snd_pcm_substream *substream,
                              struct snd_pcm_hw_params *params)
{
  struct snd_pcm_runtime *runtime = substream->runtime;
  struct snd_soc_pcm_runtime *rtd = substream->private_data;
  struct snd_soc_codec *codec = rtd->codec;
  struct infinity_pcm_runtime_data *prtd = runtime->private_data;
  struct infinity_pcm_dma_data *dma_data;
  int err = 0;


  AUD_PRINTF(PCM_LEVEL, "%s: stream = %s\n", __FUNCTION__,
             (substream->stream == SNDRV_PCM_STREAM_PLAYBACK ? "PLAYBACK" : "CAPTURE"));

  AUD_PRINTF(PCM_LEVEL, "params_channels     = %d\n", params_channels(params));
  AUD_PRINTF(PCM_LEVEL, "params_rate         = %d\n", params_rate(params));
  AUD_PRINTF(PCM_LEVEL, "params_period_size  = %d\n", params_period_size(params));
  AUD_PRINTF(PCM_LEVEL, "params_periods      = %d\n", params_periods(params));
  AUD_PRINTF(PCM_LEVEL, "params_buffer_size  = %d\n", params_buffer_size(params));
  AUD_PRINTF(PCM_LEVEL, "params_buffer_bytes = %d\n", params_buffer_bytes(params));
  AUD_PRINTF(PCM_LEVEL, "params_sample_width = %d\n", snd_pcm_format_physical_width(params_format(params)));

  AUD_PRINTF(PCM_LEVEL, "params_access = %d, params_format = %d, params_subformat = %d\n",
             params_access(params), params_format(params), params_subformat(params));

  err = infinity_audio_clk_enable();
  if(err)
    return err;

  dma_data = snd_soc_dai_get_dma_data(rtd->codec_dai, substream);

  if (!dma_data)
    return 0;

#ifdef DIGMIC_EN
  if(dma_data->channel==BACH_DMA_WRITER1)
  {
    if(!InfinityDigMicSetRate(InfinityRateFromU32(params_rate(params))))
    {
      AUD_PRINTF(ERROR_LEVEL, "%s: rate = %d digital mic not supported\n",
                            __FUNCTION__, params_rate(params));
      return -EINVAL;
    }
  }
#endif

  if(!InfinityDmaSetRate((BachDmaChannel_e)dma_data->channel, InfinityRateFromU32(params_rate(params))))
  {
    AUD_PRINTF(ERROR_LEVEL, "%s: dma = %d, rate = %d not supported\n",
                            __FUNCTION__, dma_data->channel, params_rate(params));
    return -EINVAL;
  }

  if(snd_soc_read(codec, AUD_PLAYBACK_MUX)==1 && dma_data->channel==BACH_DMA_WRITER1)
      InfinityDmaSetRate(BACH_DMA_READER1,InfinityRateFromU32(params_rate(params)));

  snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);
  runtime->dma_bytes = params_buffer_bytes(params);

  if (prtd->dma_data)
    return 0;

  prtd->dma_data = dma_data;

  err = request_irq(INFINITY_IRQ_ID, //INT_MS_AUDIO_1,
                    infinity_pcm_dma_irq,
                    IRQF_SHARED,
                    (const char*)dma_data->name,
                    (void *)substream);
  if (!err)
  {
    /*
     * Link channel with itself so DMA doesn't need any
     * reprogramming while looping the buffer
     */
    AUD_PRINTF(PCM_LEVEL, "dma name = %s, channel id = %u\n", dma_data->name, dma_data->channel);
    AUD_PRINTF(PCM_LEVEL, "dma buf phy addr = 0x%08x\n", (runtime->dma_addr - MIU0_OFFSET));
    AUD_PRINTF(PCM_LEVEL, "dma buf vir addr = 0x%08x\n", (unsigned int)runtime->dma_area);
    AUD_PRINTF(PCM_LEVEL, "dma buf size     = 0x%08x\n", runtime->dma_bytes);

    // Re-set up the underrun and overrun
    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
    {
      InfinityDmaInitChannel(dma_data->channel,
                   (runtime->dma_addr - MIU0_OFFSET),
                   runtime->dma_bytes,
                   params_channels(params),
                   snd_pcm_format_physical_width(params_format(params)),
                   params_rate(params),
                   (runtime->dma_bytes + 1024),
                   (runtime->dma_bytes - params_period_bytes(params)));

      AUD_PRINTF(PCM_LEVEL, "dma overrun      = 0x%08x\n", (runtime->dma_bytes + 1024));
      AUD_PRINTF(PCM_LEVEL, "dma underrun     = 0x%08x\n", (runtime->dma_bytes - params_period_bytes(params)));
    }
    else
    {
      InfinityDmaInitChannel(dma_data->channel,
                   (runtime->dma_addr - MIU0_OFFSET),
                   runtime->dma_bytes,
                   params_channels(params),
                   snd_pcm_format_physical_width(params_format(params)),
                   params_rate(params),
                   params_period_bytes(params),
                   256);

      AUD_PRINTF(PCM_LEVEL, "dma overrun      = 0x%08x\n", params_period_bytes(params));
      AUD_PRINTF(PCM_LEVEL, "dma underrun     = 0x%08x\n", 256);
    }

    isDmaWork[substream->stream] = TRUE;

    memset(runtime->dma_area, 0, runtime->dma_bytes);
  }

  return err;
}


static int infinity_pcm_hw_free(struct snd_pcm_substream *substream)
{
  struct snd_pcm_runtime *runtime = substream->runtime;
  struct infinity_pcm_runtime_data *prtd = runtime->private_data;
  struct snd_soc_pcm_runtime *rtd = substream->private_data;
  struct snd_soc_codec *codec = rtd->codec;
  int err;

  AUD_PRINTF(PCM_LEVEL, "%s: stream = %s\n", __FUNCTION__,
             (substream->stream == SNDRV_PCM_STREAM_PLAYBACK ? "PLAYBACK" : "CAPTURE"));

  if (prtd->dma_data == NULL)
    return 0;
  AUD_PRINTF(PCM_LEVEL, "%s: Dma mask 0x%x, status 0x%x\n", __FUNCTION__,InfinityReadReg2Byte(0x1009ac),InfinityReadReg2Byte(0x1009bc));
  free_irq(INFINITY_IRQ_ID, (void *)substream);
  isDmaWork[substream->stream] = FALSE;

  err = infinity_audio_clk_disable(codec,0);
  if(err)
    return err;
  //omap_dma_unlink_lch(prtd->dma_ch, prtd->dma_ch);
  //omap_free_dma(prtd->dma_ch);
  prtd->dma_data = NULL;
  snd_pcm_set_runtime_buffer(substream, NULL);

  return 0;
}


static int infinity_pcm_prepare(struct snd_pcm_substream *substream)
{
  struct snd_pcm_runtime *runtime = substream->runtime;
  struct infinity_pcm_runtime_data *prtd = runtime->private_data;
  struct infinity_pcm_dma_data *dma_data = prtd->dma_data;
  int ret = 0;
  AUD_PRINTF(PCM_LEVEL, "%s: stream = %s, channel = %s\n", __FUNCTION__,
             (substream->stream == SNDRV_PCM_STREAM_PLAYBACK ? "PLAYBACK" : "CAPTURE"), dma_data->name);

  return ret;
}

static int infinity_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
  struct snd_pcm_runtime *runtime = substream->runtime;
  struct infinity_pcm_runtime_data *prtd = runtime->private_data;
  struct infinity_pcm_dma_data *dma_data = prtd->dma_data;
  unsigned long flags;
  int ret = 0;
  struct timespec   tv;


  AUD_PRINTF(PCM_LEVEL, "%s: stream = %s, channel = %s, cmd = %d\n", __FUNCTION__,
             (substream->stream == SNDRV_PCM_STREAM_PLAYBACK ? "PLAYBACK" : "CAPTURE"), dma_data->name, cmd);

 // AUD_PRINTF(TRACE_LEVEL, "!!BACH_DMA1_CTRL_0 = 0x%x,  BACH_DMA1_CTRL_8 = 0x%x, level count = %d\n",
 // 		InfinityReadReg(BACH_REG_BANK1, BACH_DMA1_CTRL_0), InfinityReadReg(BACH_REG_BANK1, BACH_DMA1_CTRL_8), InfinityDmaGetLevelCnt(prtd->dma_data->channel));

  spin_lock_irqsave(&prtd->lock, flags);
  switch (cmd)
  {
  case SNDRV_PCM_TRIGGER_START:
  case SNDRV_PCM_TRIGGER_RESUME:
  case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
    InfinityDmaStartChannel((BachDmaChannel_e)dma_data->channel);

    if(cmd==SNDRV_PCM_TRIGGER_START)
    {
      if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
      {
        //AUD_PRINTF(ERROR_LEVEL, "Play Trig Time = %llu\n",Chip_Get_US_Ticks());
        //g_nPlayStartTime = Chip_Get_US_Ticks();
        do_posix_clock_monotonic_gettime(&tv);
        g_nPlayStartTime = (unsigned long long)tv.tv_sec * 1000000LL + (unsigned long long) (tv.tv_nsec/1000LL);
        //AUD_PRINTF(ERROR_LEVEL, "Play Trig Time = %llu, old %llu\n",g_nPlaySt
      }
      else
      {
        //AUD_PRINTF(ERROR_LEVEL, "Cap Trig Time = %llu\n",Chip_Get_US_Ticks())
        //g_nCapStartTime = Chip_Get_US_Ticks();
        do_posix_clock_monotonic_gettime(&tv);
        g_nCapStartTime = (unsigned long long)tv.tv_sec * 1000000LL + (unsigned long long) (tv.tv_nsec/1000LL);
      }
    }
    break;

  case SNDRV_PCM_TRIGGER_STOP:
  case SNDRV_PCM_TRIGGER_SUSPEND:
  case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
    memset(runtime->dma_area, 0, runtime->dma_bytes);
    prtd->dma_level_count = 0;
    prtd->remain_count = 0;
    prtd->state = 0;
    if(cmd==SNDRV_PCM_TRIGGER_STOP)
    {
	  if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
	  {
      g_nPlayStartTime = 0;
	  }
	  else
	  {
	    g_nCapStartTime = 0;
	  }
    }

    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
      InfinityDmaMaskInt((BachDmaChannel_e)dma_data->channel, BACH_DMA_INT_OVERRUN, TRUE);
    else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
      InfinityDmaMaskInt((BachDmaChannel_e)dma_data->channel, BACH_DMA_INT_UNDERRUN, TRUE);
    else
    {
      ret = -EINVAL;
      break;
    }

   // if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
   //   InfinityDmaMaskInt(dma_data->channel, BACH_DMA_INT_EMPTY, TRUE);

    InfinityDmaStopChannel((BachDmaChannel_e)dma_data->channel);

    break;
  default:
    ret = -EINVAL;
  }
  spin_unlock_irqrestore(&prtd->lock, flags);

  // AUD_PRINTF(TRACE_LEVEL, "!!Out BACH_DMA1_CTRL_0 = 0x%x,  BACH_DMA1_CTRL_8 = 0x%x, level count = %d\n",
  //		InfinityReadReg(BACH_REG_BANK1, BACH_DMA1_CTRL_0), InfinityReadReg(BACH_REG_BANK1, BACH_DMA1_CTRL_8), InfinityDmaGetLevelCnt(prtd->dma_data->channel));

  return ret;
}

static snd_pcm_uframes_t infinity_pcm_pointer(struct snd_pcm_substream *substream)
{
  struct snd_pcm_runtime *runtime = substream->runtime;
  struct infinity_pcm_runtime_data *prtd = runtime->private_data;
  struct infinity_pcm_dma_data *dma_data = prtd->dma_data;

  snd_pcm_uframes_t offset = 0;
  unsigned long flags;
  size_t last_dma_count_level = 0;

  if (SNDRV_PCM_STREAM_PLAYBACK == substream->stream)
  {
    spin_lock_irqsave(&prtd->lock, flags);

    last_dma_count_level = InfinityDmaGetLevelCnt((BachDmaChannel_e)dma_data->channel);
    //udelay(500);
    if (prtd->dma_level_count > runtime->dma_bytes * 2)
      prtd->dma_level_count -= runtime->dma_bytes;
    offset = bytes_to_frames(runtime, ((prtd->dma_level_count - last_dma_count_level - prtd->remain_count) % runtime->dma_bytes));

    spin_unlock_irqrestore(&prtd->lock, flags);
  }
  else if (SNDRV_PCM_STREAM_CAPTURE == substream->stream)
  {
    spin_lock_irqsave(&prtd->lock, flags);

    last_dma_count_level = InfinityDmaGetLevelCnt((BachDmaChannel_e)dma_data->channel);
    //udelay(5000);

/*
    if (last_dma_count_level <= 64)
      last_dma_count_level = 0;
    else
      last_dma_count_level = last_dma_count_level - 64;
*/

    if (prtd->dma_level_count > runtime->dma_bytes * 2)
      prtd->dma_level_count -= runtime->dma_bytes;
    offset = bytes_to_frames(runtime, ((prtd->dma_level_count + last_dma_count_level - prtd->remain_count) % runtime->dma_bytes));
    spin_unlock_irqrestore(&prtd->lock, flags);
  }

  AUD_PRINTF(PCM_DEBUG_LEVEL, "%s: stream id = %d, channel id = %d, frame offset = 0x%x\n",
             __FUNCTION__, substream->stream, dma_data->channel, (unsigned int)offset);

  //AUD_PRINTF(TRACE_LEVEL, "last_dma_count_level = 0x%x, offset(bytes) = 0x%x, dma size byte = 0x%x, buffer_size = 0x%x\n", last_dma_count_level, offset*4, runtime->dma_bytes, runtime->buffer_size);
  return offset;
}

static int infinity_pcm_copy(struct snd_pcm_substream *substream,
                         int channel, snd_pcm_uframes_t pos,
                         void __user *dst, snd_pcm_uframes_t count)
{
  struct snd_pcm_runtime *runtime = substream->runtime;
  struct infinity_pcm_runtime_data *prtd = runtime->private_data;
  struct infinity_pcm_dma_data *dma_data = prtd->dma_data;
  unsigned long flags;
  unsigned char *hwbuf = runtime->dma_area + frames_to_bytes(runtime, pos);

  unsigned long nCnt, nLevelCnt;

 // AUD_PRINTF(TRACE_LEVEL, "BACH_DMA1_CTRL_0 = 0x%x,  BACH_DMA1_CTRL_8 = 0x%x, level count = %d, need bytes = %d\n",
 // 		InfinityReadReg(BACH_REG_BANK1, BACH_DMA1_CTRL_0), InfinityReadReg(BACH_REG_BANK1, BACH_DMA1_CTRL_8), InfinityDmaGetLevelCnt(prtd->dma_data->channel),frames_to_bytes(runtime, count));

  if (SNDRV_PCM_STREAM_PLAYBACK == substream->stream)
  {
    if (copy_from_user(hwbuf, dst, frames_to_bytes(runtime, count)))
      return -EFAULT;

    spin_lock_irqsave(&prtd->lock, flags);
    Chip_Flush_MIU_Pipe();
    nLevelCnt = InfinityDmaGetLevelCnt((BachDmaChannel_e)dma_data->channel);
    nCnt = InfinityDmaTrigLevelCnt((BachDmaChannel_e)dma_data->channel, (frames_to_bytes(runtime, count) + prtd->remain_count));
    if (prtd->state == DMA_EMPTY)
    {
      InfinityDmaClearInt((BachDmaChannel_e)dma_data->channel);
      prtd->state = DMA_NORMAL;
    }


    if (((nLevelCnt + nCnt)  >= (runtime->dma_bytes - frames_to_bytes(runtime, runtime->period_size))) && snd_pcm_running(substream))
    {
        InfinityDmaMaskInt((BachDmaChannel_e)dma_data->channel, BACH_DMA_INT_UNDERRUN, FALSE);
        prtd->state = DMA_NORMAL;
    }

    prtd->dma_level_count += frames_to_bytes(runtime, count);
    prtd->remain_count = (frames_to_bytes(runtime, count) + prtd->remain_count) - nCnt;

    if (prtd->remain_count)
      AUD_PRINTF(PCM_LEVEL, "%s: stream id = %d, channel id = %d, cnt = 0x%x, frame_bytes = 0x%x\n",
                 __FUNCTION__, substream->stream, dma_data->channel, (unsigned int)nCnt, (unsigned int)frames_to_bytes(runtime, count));
#if 0
    if (DmaIsEmpty(dma_data->channel))
    {
      DmaMaskInt(dma_data->channel, BACH_DMA_INT_UNDERRUN, FALSE);
      DmaMaskInt(dma_data->channel, BACH_DMA_INT_EMPTY, FALSE);
    }
#endif
    spin_unlock_irqrestore(&prtd->lock, flags);
  }
  else if (SNDRV_PCM_STREAM_CAPTURE == substream->stream)
  {
    if (copy_to_user(dst, hwbuf, frames_to_bytes(runtime, count)))
      return -EFAULT;

    spin_lock_irqsave(&prtd->lock, flags);
    nCnt = InfinityDmaTrigLevelCnt((BachDmaChannel_e)dma_data->channel, frames_to_bytes(runtime, count) + prtd->remain_count);
    if (prtd->state == DMA_FULL)
    {
      InfinityDmaClearInt((BachDmaChannel_e)dma_data->channel);
      prtd->state = DMA_NORMAL;
      AUD_PRINTF(PCM_LEVEL, "%s: FULL Clear\n", __FUNCTION__);
    }

/*
    if ((prtd->dma_level_count%runtime->dma_bytes) != frames_to_bytes(runtime, pos))
      AUD_PRINTF(PCM_LEVEL, "%s: dma_level_count = %d, pos_bytes = %d\n",
                 __FUNCTION__, prtd->dma_level_count, frames_to_bytes(runtime, pos));
*/

    prtd->dma_level_count += frames_to_bytes(runtime, count);
    prtd->remain_count = (frames_to_bytes(runtime, count) + prtd->remain_count)- nCnt;

    if (prtd->remain_count)
      AUD_PRINTF(PCM_LEVEL, "%s: stream id = %d, channel id = %d, cnt = 0x%x, frame_bytes = 0x%x\n",
                 __FUNCTION__, substream->stream, dma_data->channel, (unsigned int)nCnt, (unsigned int)frames_to_bytes(runtime, count));

    spin_unlock_irqrestore(&prtd->lock, flags);
  }

  AUD_PRINTF(PCM_DEBUG_LEVEL, "frame_pos = 0x%x, frame_count = 0x%x, framLevelCnt = 0x%x, T = %d\n",
             (unsigned int)pos, (unsigned int)count, InfinityDmaGetLevelCnt((BachDmaChannel_e)dma_data->channel)/4, jiffies_to_msecs(jiffies));

 // AUD_PRINTF(TRACE_LEVEL, "Out BACH_DMA1_CTRL_0 = 0x%x,  BACH_DMA1_CTRL_8 = 0x%x, level count = %d\n",
 //		InfinityReadReg(BACH_REG_BANK1, BACH_DMA1_CTRL_0), InfinityReadReg(BACH_REG_BANK1, BACH_DMA1_CTRL_8), InfinityDmaGetLevelCnt(prtd->dma_data->channel));

  return 0;
}


#if 0
static S32 msb2501_pcm_mmap(struct snd_pcm_substream *substream, struct vm_area_struct *vma)
{
  struct snd_pcm_runtime *runtime = substream->runtime;

  AUD_PRINTF(TRACE_LEVEL, "%s\r\n", __FUNCTION__);

  return dma_mmap_writecombine(substream->pcm->card->dev,
                               vma, runtime->dma_area,
                               runtime->dma_addr, runtime->dma_bytes);
}
#endif

static struct snd_pcm_ops infinity_pcm_ops =
{
  .open		    = infinity_pcm_open,
  .close		  = infinity_pcm_close,
  .ioctl		  = snd_pcm_lib_ioctl,
  .hw_params	= infinity_pcm_hw_params,
  .hw_free	  = infinity_pcm_hw_free,
  .prepare	  = infinity_pcm_prepare,
  .trigger	  = infinity_pcm_trigger,
  .pointer	  = infinity_pcm_pointer,
  .copy		    = infinity_pcm_copy,
};

static u64 infinity_pcm_dmamask = DMA_BIT_MASK(64);

static void* alloc_dmem(const char* name, unsigned int size, dma_addr_t *addr)
{
  MSYS_DMEM_INFO dmem;
  memcpy(dmem.name,name,strlen(name)+1);
  dmem.length=size;
  if(0!=msys_request_dmem(&dmem))
  {
    return NULL;
  }
  *addr=dmem.phys;
  return (void *)((uintptr_t)dmem.kvirt);
}

static void free_dmem(const char* name, unsigned int size, void *virt, dma_addr_t addr)
{
  MSYS_DMEM_INFO dmem;
  memcpy(dmem.name,name,strlen(name)+1);
  dmem.length=size;
  dmem.kvirt=(unsigned long long)((uintptr_t)virt);
  dmem.phys=(unsigned long long)((uintptr_t)addr);
  msys_release_dmem(&dmem);
}


static int infinity_pcm_preallocate_dma_buffer(struct snd_pcm *pcm,
    int stream)
{
  char name[16];

  struct snd_pcm_substream *substream = pcm->streams[stream].substream;
  struct snd_dma_buffer *buf = &substream->dma_buffer;
  size_t size = 0;

  AUD_PRINTF(PLATFORM_LEVEL, "%s: stream = %s\r\n", __FUNCTION__,
             (substream->stream == SNDRV_PCM_STREAM_PLAYBACK ? "PLAYBACK" : "CAPTURE"));

  if (SNDRV_PCM_STREAM_PLAYBACK == stream)
  {
    size = infinity_pcm_playback_hardware.buffer_bytes_max;
    snprintf(name, 16, "pcmC%dD%dp", pcm->card->number, pcm->device);
  }
  else if (SNDRV_PCM_STREAM_CAPTURE == stream)  // CAPTURE device
  {
    size = infinity_pcm_capture_hardware.buffer_bytes_max;
    snprintf(name, 16, "pcmC%dD%dc", pcm->card->number, pcm->device);
  }
  else
    return -EINVAL;

  buf->dev.type = SNDRV_DMA_TYPE_DEV;
  buf->dev.dev = pcm->card->dev;
  buf->private_data = NULL;
  buf->area = alloc_dmem(name, PAGE_ALIGN(size),&buf->addr);

  if (!buf->area)
    return -ENOMEM;
  buf->bytes = PAGE_ALIGN(size);

  AUD_PRINTF(PLATFORM_LEVEL, "dma buffer size 0x%x\n", buf->bytes);
  AUD_PRINTF(PLATFORM_LEVEL, "physical dma address 0x%08x\n", buf->addr);
  AUD_PRINTF(PLATFORM_LEVEL, "virtual dma address 0x%08x\n", (unsigned int)buf->area);

  return 0;
}

static void infinity_pcm_free_dma_buffers(struct snd_pcm *pcm)
{
  struct snd_pcm_substream *substream;
  struct snd_dma_buffer *buf;
  int stream;
  char name[16];

  AUD_PRINTF(PLATFORM_LEVEL, "%s\n", __FUNCTION__);

  for (stream = 0; stream < 2; stream++)
  {
    substream = pcm->streams[stream].substream;
    if (!substream)
      continue;


    buf = &substream->dma_buffer;
    if (!buf->area)
      continue;

    if (SNDRV_PCM_STREAM_PLAYBACK == stream)
 	{
      snprintf(name, 16, "pcmC%dD%dp", pcm->card->number, pcm->device);
    }
    else if (SNDRV_PCM_STREAM_CAPTURE == stream)  // CAPTURE device
    {
      snprintf(name, 16, "pcmC%dD%dc", pcm->card->number, pcm->device);
    }

    free_dmem(name, buf->bytes, buf->area, buf->addr);
    buf->area = NULL;
  }
}

static int infinity_pcm_new(struct snd_soc_pcm_runtime *rtd)
{
  struct snd_card *card = rtd->card->snd_card;
  struct snd_pcm *pcm = rtd->pcm;
  int ret = 0;

  AUD_PRINTF(PLATFORM_LEVEL, "%s: snd_pcm device id = %d\r\n", __FUNCTION__, pcm->device);

  if (!card->dev->dma_mask)
    card->dev->dma_mask = &infinity_pcm_dmamask;
  if (!card->dev->coherent_dma_mask)
    card->dev->coherent_dma_mask = DMA_BIT_MASK(64);

  if (pcm->streams[SNDRV_PCM_STREAM_PLAYBACK].substream)
  {
    ret = infinity_pcm_preallocate_dma_buffer(pcm, SNDRV_PCM_STREAM_PLAYBACK);
    if (ret)
      goto out;
  }

  if (pcm->streams[SNDRV_PCM_STREAM_CAPTURE].substream)
  {
    ret = infinity_pcm_preallocate_dma_buffer(pcm, SNDRV_PCM_STREAM_CAPTURE);
    if (ret)
      goto out;
  }

out:
  /* free preallocated buffers in case of error */
  if (ret)
    infinity_pcm_free_dma_buffers(pcm);

  return ret;
}

/*
static struct class * audio_class;
static char * audio_classname = "mstar_audio_class";
static struct device *audio_dev;
*/
static int infinity_pcm_probe(struct snd_soc_platform *platform)
{
  AUD_PRINTF(PLATFORM_LEVEL, "%s: platform = %s\n", __FUNCTION__, dev_name(platform->dev));

  InfinityDmaReset();
  AUD_PRINTF(PLATFORM_LEVEL, "Dma reset\n");
  return 0;
}

static int infinity_pcm_remove(struct snd_soc_platform *platform)
{
  AUD_PRINTF(PLATFORM_LEVEL, "%s: platform = %s\n", __FUNCTION__, dev_name(platform->dev));
  return 0;
}

static int infinity_pcm_suspend(struct snd_soc_dai *dai)
{
  AUD_PRINTF(PLATFORM_LEVEL, "%s: dai = %s\n", __FUNCTION__, dai->name);

  return 0;
}

static int infinity_pcm_resume(struct snd_soc_dai *dai)
{
  struct snd_pcm_substream *substream;
  struct snd_pcm_runtime *runtime;
  struct infinity_pcm_runtime_data *prtd;
  struct infinity_pcm_dma_data *dma_data;
  //struct snd_soc_pcm_runtime *rtd;
  //struct snd_soc_codec *codec;

  int stream, i = 0;

  AUD_PRINTF(PLATFORM_LEVEL, "%s: dai = %s\n", __FUNCTION__, dai->name);
  InfinityDmaReset();
  AUD_PRINTF(PLATFORM_LEVEL, "Dma reset\n");

	for (i = 0; i < dai->card->num_rtd; i++) {
		if (dai->card->rtd[i].dai_link->ignore_suspend)
			continue;

    for (stream = 0; stream < 2; stream++) {
  		for (substream = dai->card->rtd[i].pcm->streams[stream].substream;
  		     substream; substream = substream->next) {
  			/* FIXME: the open/close code should lock this as well */
  			if (substream->runtime == NULL)
  				continue;

        runtime = substream->runtime;
        prtd = runtime->private_data;
        //rtd = substream->private_data;
        //codec = rtd->codec;

        if (prtd->dma_data)
        {
          dma_data = prtd->dma_data;
          /*
           * Link channel with itself so DMA doesn't need any
           * reprogramming while looping the buffer
           */

          InfinityDmaSetRate((BachDmaChannel_e)dma_data->channel, InfinityRateFromU32(runtime->rate));

          InfinityDmaInitChannel(dma_data->channel,
                             (runtime->dma_addr - MIU0_OFFSET),
                             runtime->dma_bytes,
                             runtime->channels,
                             snd_pcm_format_physical_width(runtime->format),
                             runtime->rate,
                             runtime->dma_bytes * 3 / 4,
                             runtime->dma_bytes * 1 / 4);

          AUD_PRINTF(PCM_LEVEL, "dma name = %s, channel id = %u\n", dma_data->name, dma_data->channel);
          AUD_PRINTF(PCM_LEVEL, "dma buf phy addr = 0x%08x\n", (runtime->dma_addr - MIU0_OFFSET));
          AUD_PRINTF(PCM_LEVEL, "dma buf vir addr = 0x%08x\n", (unsigned int)runtime->dma_area);
          AUD_PRINTF(PCM_LEVEL, "dma buf size     = 0x%08x\n", runtime->dma_bytes);

          // Re-set up the underrun and overrun
          if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
          {
            InfinityDmaSetThreshold((BachDmaChannel_e)dma_data->channel, (runtime->dma_bytes + 1024), (runtime->dma_bytes - frames_to_bytes(runtime, runtime->period_size)));

            AUD_PRINTF(PCM_LEVEL, "dma overrun      = 0x%08x\n", (runtime->dma_bytes + 1024));
            AUD_PRINTF(PCM_LEVEL, "dma underrun     = 0x%08x\n", (runtime->dma_bytes - frames_to_bytes(runtime, runtime->period_size)));

          }
          else
          {
            InfinityDmaSetThreshold((BachDmaChannel_e)dma_data->channel, frames_to_bytes(runtime, runtime->period_size), 256);

            AUD_PRINTF(PCM_LEVEL, "dma overrun      = 0x%08x\n", frames_to_bytes(runtime, runtime->period_size));
            AUD_PRINTF(PCM_LEVEL, "dma underrun     = 0x%08x\n", 256);
          }

          memset(runtime->dma_area, 0, runtime->dma_bytes);
          prtd->dma_level_count = 0;
          prtd->remain_count = 0;
          prtd->state = 0; //EMPTY
        }

  		}
  	}

	}

  return 0;
}

static struct snd_soc_platform_driver infinity_soc_platform =
{
  .probe = infinity_pcm_probe,
  .remove = infinity_pcm_remove,
  .suspend = infinity_pcm_suspend,
  .resume = infinity_pcm_resume,

  .ops		= &infinity_pcm_ops,
  .pcm_new	= infinity_pcm_new,
  .pcm_free	= infinity_pcm_free_dma_buffers,
};

static int infinity_platform_probe(struct platform_device *pdev)
{
  struct resource *res_irq;
  u32 val;
  s32 ret;


  AUD_PRINTF(TRACE_LEVEL, "%s\n", __FUNCTION__);

  res_irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
  if (res_irq)
  {
		INFINITY_IRQ_ID = res_irq->start;
		AUD_PRINTF(TRACE_LEVEL, "Platform get resource IORESOURCE_IRQ = 0x%x\n", INFINITY_IRQ_ID);
  }
  else
  {

    INFINITY_IRQ_ID = of_irq_to_resource(pdev->dev.of_node, 0, NULL);
    AUD_PRINTF(TRACE_LEVEL, "Platform get resource IORESOURCE_IRQ = 0x%x\n", INFINITY_IRQ_ID);
    if (!INFINITY_IRQ_ID)
		  return -EINVAL;

  }

  ret = of_property_read_u32(pdev->dev.of_node, "playback-dma-buffer", &val);
  if (ret == 0)
  {
    AUD_PRINTF(TRACE_LEVEL, "Get playback-dma-buffer = %d\n", val);
    infinity_pcm_playback_hardware.buffer_bytes_max = val;
  }

  ret = of_property_read_u32(pdev->dev.of_node, "capture-dma-buffer", &val);
  if (ret == 0)
  {
    AUD_PRINTF(TRACE_LEVEL, "Get capture-dma-buffer = %d\n", val);
    infinity_pcm_capture_hardware.buffer_bytes_max = val;
  }
  //dev_set_name(&pdev->dev, "infinity-platform");

  return snd_soc_register_platform(&pdev->dev, &infinity_soc_platform);
}

static int infinity_platform_remove(struct platform_device *pdev)
{
  AUD_PRINTF(TRACE_LEVEL, "%s\n", __FUNCTION__);
  snd_soc_unregister_platform(&pdev->dev);
  return 0;
}

static struct platform_driver infinity_dma_driver =
{
  .driver = {
    .name = "infinity-platform",
    .owner = THIS_MODULE,
  },

  .probe = infinity_platform_probe,
  .remove = infinity_platform_remove,
};

#ifndef CONFIG_OF
module_platform_driver(infinity_dma_driver);
#else
//static struct platform_device *infinity_dma_device = NULL;
static int __init infinity_dma_init(void)
{

  int ret = 0;

  struct device_node *np;

  AUD_PRINTF(TRACE_LEVEL, "%s\r\n", __FUNCTION__)

  infinity_dma_device = platform_device_alloc("infinity-platform", -1);
  if (!infinity_dma_device)
  {
    AUD_PRINTF(ERROR_LEVEL, "%s: platform_device_alloc infinity_dma_device error\r\n", __FUNCTION__);
    return -ENOMEM;
  }

  np = of_find_compatible_node(NULL, NULL, "mstar,infinity3-audio");
  if(np)
  {
    infinity_dma_device->dev.of_node = of_node_get(np);
    of_node_put(np);
  }

  ret = platform_device_add(infinity_dma_device);
  if (ret)
  {
    AUD_PRINTF(ERROR_LEVEL, "%s: platform_device_add infinity_dma_device error\r\n", __FUNCTION__);
    platform_device_put(infinity_dma_device);
  }

  ret = platform_driver_register(&infinity_dma_driver);
  if (ret)
  {
    AUD_PRINTF(ERROR_LEVEL, "%s: platform_driver_register infinity_dma_driver error\r\n", __FUNCTION__);
    platform_device_unregister(infinity_dma_device);
  }

  return ret;
}

static void __exit infinity_dma_exit(void)
{
  AUD_PRINTF(TRACE_LEVEL, "%s\r\n", __FUNCTION__);
  platform_device_unregister(infinity_dma_device);
  platform_driver_unregister(&infinity_dma_driver);
}

module_init(infinity_dma_init);
module_exit(infinity_dma_exit);

#endif
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Trevor Wu, trevor.wu@mstarsemi.com");
MODULE_DESCRIPTION("iNfinity3 Bach PCM DMA module");

