#include <linux/module.h>
#include <linux/moduleparam.h>

#include <linux/cdev.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>


#include "mdrv_sound_io.h"
//#include "ms_platform.h"

#include "infinity_pcm.h"
//#include "infinity_codec.h"
//#include "infinity_dai.h"
#include "bach_audio_debug.h"

extern struct snd_soc_card infinity_soc_card;

static int infinity_dma_open(struct inode *inode, struct file *filp)
{

    int ret = 0;
    struct snd_pcm_substream *substream = NULL;
    struct snd_pcm_runtime *runtime= NULL;
    struct snd_soc_pcm_runtime *rtd = NULL;

    AUD_PRINTF(TRACE_LEVEL, "%s\n", __FUNCTION__);

    rtd = snd_soc_get_pcm_runtime(&infinity_soc_card, infinity_soc_card.dai_link[iminor(inode)].name);
    if (!rtd)
    {
      AUD_PRINTF(ERROR_LEVEL, "%s: snd_soc_get_pcm_runtime failed = %s\n", __FUNCTION__, infinity_soc_card.dai_link[iminor(inode)].name);
      ret = -ENODEV;
    }
    else
    {
      substream = rtd->pcm->streams[SNDRV_PCM_STREAM_PLAYBACK].substream;

      if (substream)
      {
        snd_pcm_stream_lock_irq(substream);

        filp->private_data = (void *)substream;

        runtime = substream->runtime;
        if (!runtime)
        {
          AUD_PRINTF(ERROR_LEVEL, "%s: runtime NULL failed\n", __FUNCTION__);
          ret = -ENXIO;
        }

        snd_pcm_stream_unlock_irq(substream);

      }
      else
      {
         AUD_PRINTF(ERROR_LEVEL, "%s: substream NULL failed\n", __FUNCTION__);
         ret = -ENODEV;
      }
    }

    return ret;
}

static int infinity_dma_release(struct inode *inode, struct file *filp)
{

    AUD_PRINTF(TRACE_LEVEL, "%s\n", __FUNCTION__);

    filp->private_data = NULL;

    return 0;
}

static ssize_t infinity_dma_read(struct file *filp, char __user *buf,
      size_t count, loff_t *ppos)
{
    u32 offset, frame_size;
    u64 hw_ptr, appl_ptr;
    ssize_t ret;

    snd_pcm_sframes_t avail;

    struct snd_pcm_substream *substream = (struct snd_pcm_substream *)filp->private_data;
    struct snd_pcm_runtime *runtime;

    //AUD_PRINTF(TRACE_LEVEL, "loff = 0x%llx, f_pos = 0x%llx\n", *ppos, filp->f_pos);
    if(!substream)
    {
      AUD_PRINTF(ERROR_LEVEL, "%s: substream NULL failed\n", __FUNCTION__);
      return -ENODEV;
    }

    snd_pcm_stream_lock_irq(substream);

    runtime = substream->runtime;
    if (!runtime)
    {
      ret = -ENXIO;
    }
    else
    {

      if (runtime->status->state == SNDRV_PCM_STATE_RUNNING ||
          runtime->status->state == SNDRV_PCM_STATE_DRAINING)
      {

        frame_size  = runtime->frame_bits/8;
        avail = runtime->buffer_size - snd_pcm_playback_avail(runtime);
        hw_ptr = runtime->hw_ptr_wrap + runtime->status->hw_ptr;
        appl_ptr = hw_ptr + avail;

        //snd_pcm_stream_unlock_irq(substream);
        //AUD_PRINTF(TRACE_LEVEL, "appl_ptr = 0x%llx, hw_ptr = 0x%llx\n", ((hw_ptr + avail) * frame_size), (hw_ptr * frame_size));
        //snd_pcm_stream_lock_irq(substream);

        if ((appl_ptr * frame_size) <=  filp->f_pos)
        {
          //AUD_PRINTF(ERROR_LEVEL, "appl_ptr = 0x%llx, hw_ptr = 0x%llx f_pos = 0x%llx\n", ((hw_ptr + avail) * frame_size), (hw_ptr * frame_size),filp->f_pos);
          ret = 0;
        }
        else if (((appl_ptr * frame_size) - runtime->dma_bytes) > filp->f_pos)
        {
          ret = -EOVERFLOW;
        }
        else
        {
          if (count > ((appl_ptr * frame_size) - filp->f_pos))
            count = (appl_ptr * frame_size) - filp->f_pos;

          ret = count;

          div_u64_rem(filp->f_pos, runtime->dma_bytes, &offset);

          snd_pcm_stream_unlock_irq(substream);

          if (offset + count > runtime->dma_bytes)
          {
            if (copy_to_user((buf + runtime->dma_bytes - offset), runtime->dma_area, ((count + offset) - runtime->dma_bytes)))
              return -EFAULT;

            count = (runtime->dma_bytes - offset);
          }

          if (copy_to_user(buf, (runtime->dma_area + offset), count))
            return -EFAULT;

          snd_pcm_stream_lock_irq(substream);

          *ppos += ret;
        }

      }
      else
        ret = -EPERM;

    }/*!runtime*/

    snd_pcm_stream_unlock_irq(substream);

    return ret;

}

static loff_t infinity_dma_lseek(struct file *file, loff_t offset, int orig)
{
  loff_t ret;
  u32 frame_size;
  u64 hw_ptr;
  snd_pcm_sframes_t avail;
  struct snd_pcm_substream *substream = (struct snd_pcm_substream *)file->private_data;
  struct snd_pcm_runtime *runtime;

  if(!substream)
  {
    AUD_PRINTF(ERROR_LEVEL, "%s: substream NULL failed\n", __FUNCTION__);
    return -ENODEV;
  }


  snd_pcm_stream_lock_irq(substream);


  runtime = substream->runtime;
  if (!runtime)
  {
    ret = -ENXIO;
  }
  else
  {

    frame_size  = runtime->frame_bits/8;
    avail = runtime->buffer_size - snd_pcm_playback_avail(runtime);
    hw_ptr = runtime->hw_ptr_wrap + runtime->status->hw_ptr;

    //mutex_lock(&file_inode(file)->i_mutex);
    switch (orig) {
    case SEEK_CUR:
      offset += file->f_pos;
    case SEEK_SET:
      file->f_pos = offset;
      ret = file->f_pos;
      break;
    case SEEK_END:
      offset += ((hw_ptr + avail) * frame_size);
      file->f_pos = offset;
      ret = file->f_pos;
      break;
/*
    case SEEK_DATA:
      offset += (hw_ptr * frame_size);
      file->f_pos = offset;
      ret = file->f_pos;
      break;
*/
    default:
      ret = -EINVAL;
    }
    //mutex_unlock(&file_inode(file)->i_mutex);
  }
  snd_pcm_stream_unlock_irq(substream);

  return ret;
}

static long infinity_dma_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int err= 0;
    AUD_PRINTF(TRACE_LEVEL, "%s 0x%x!\n",__FUNCTION__,cmd);
    // wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
    if (_IOC_TYPE(cmd) != AUDIO_IOCTL_MAGIC) return -ENOTTY;
    if (_IOC_NR(cmd) > IOCTL_AUDIO_MAXNR) return -ENOTTY;

    if (_IOC_DIR(cmd) & _IOC_READ)
    {
        err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
    }
    else if (_IOC_DIR(cmd) & _IOC_WRITE)
    {
        err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
    }
    if (err)
    {
        return -EFAULT;
    }


    switch(cmd)
    {
        case MDRV_SOUND_STARTTIME_READ:
		{
			AUD_PRINTF(TRACE_LEVEL, "MDRV_SOUND_STARTTIME_READ!\n");
			if(copy_to_user((void __user *)arg, (void*)&g_nPlayStartTime, sizeof(unsigned long long)))
			{
				return -EFAULT;
			}

		}
		break;

        default:
            AUD_PRINTF(ERROR_LEVEL,"Unknown IOCTL Command 0x%08X\n", cmd);
            return -ENOTTY;
    }

    return err;
}

//static struct class * dma_class;

static struct cdev infinity_dma_cdev =
{
  .kobj = {.name= "infinity_dma", },
  .owner = THIS_MODULE,
};

static dev_t dma_dev;

static struct file_operations infinity_dma_fops =
{
    .open = infinity_dma_open,
    .release = infinity_dma_release,
    .read = infinity_dma_read,
    .llseek = infinity_dma_lseek,
    .unlocked_ioctl = infinity_dma_ioctl
};

static ssize_t play_time_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    str += scnprintf(str, end - str, "%llu\n",g_nPlayStartTime);

    return (str - buf);
}

DEVICE_ATTR(play_time, 0444, play_time_show, NULL);

static ssize_t cap_time_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    str += scnprintf(str, end - str, "%llu\n",g_nCapStartTime);

    return (str - buf);
}

DEVICE_ATTR(cap_time, 0444, cap_time_show, NULL);


static int __init infinity_mem_init(void)
{
  int ret = 0;
  struct device *dev;

  AUD_PRINTF(TRACE_LEVEL, "%s\r\n", __FUNCTION__);

  ret = alloc_chrdev_region(&dma_dev, 0, 1, "infinity_dma");
  if (ret) {
      AUD_PRINTF(ERROR_LEVEL,"can't alloc chrdev\n");
      return ret;
  }

  cdev_init(&infinity_dma_cdev, &infinity_dma_fops);
  if (0 != (ret= cdev_add(&infinity_dma_cdev, dma_dev, 1)))
  {
      AUD_PRINTF(ERROR_LEVEL, "%s:Unable add a character device\n", __FUNCTION__);
      unregister_chrdev_region(dma_dev, 1);
      return ret;
  }

/*
  dma_class = class_create(THIS_MODULE, "bach_dma_class");
  if(IS_ERR(dma_class))
  {
      AUD_PRINTF(ERROR_LEVEL, "%s:Failed at class_create().Please exec [mknod] before operate the device\n", __FUNCTION__);
  }
  else
*/
  {
      dev = device_create(sound_class, NULL, dma_dev, NULL, "dmaC0D0p");
  }

  if (IS_ERR(dev))
  {
      ret = PTR_ERR(dev);
      cdev_del(&infinity_dma_cdev);
      unregister_chrdev_region(dma_dev, 1);
  }
  else
  {
      ret = device_create_file(dev, &dev_attr_play_time);
      if(ret)
	      AUD_PRINTF(ERROR_LEVEL, "%s:add attibute failed\n", __FUNCTION__);

      ret = device_create_file(dev, &dev_attr_cap_time);
      if(ret)
          AUD_PRINTF(ERROR_LEVEL, "%s:add attibute failed\n", __FUNCTION__);
  }
  return ret;
}

static void __exit infinity_mem_exit(void)
{
  AUD_PRINTF(TRACE_LEVEL, "%s\r\n", __FUNCTION__);
  device_destroy(sound_class, dma_dev);

  cdev_del(&infinity_dma_cdev);

  unregister_chrdev_region(dma_dev, 1);
}

module_init(infinity_mem_init);
module_exit(infinity_mem_exit);


MODULE_LICENSE("GPL");
