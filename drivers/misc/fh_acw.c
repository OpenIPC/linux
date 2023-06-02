/**@file
 * @Copyright (c) 2016 Shanghai Fullhan Microelectronics Co., Ltd.
 * @brief
 *
 * @author      fullhan
 * @date        2016-7-15
 * @version     V1.0
 * @version     V1.1  modify code style
 * @note: misc audio driver for fh8830 embedded audio codec.
 * @note History:
 * @note     <author>   <time>    <version >   <desc>
 * @note
 * @warning: the codec is fixed to 24 bit, so remember to move the 24 bit data to 16 bit in
 *   application layer, the next version CPU will sovle this bug.
 */


#include <linux/irqreturn.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/file.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/poll.h>
#include <linux/ioctl.h>
#include <linux/i2c.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/fh_dmac.h>
#include <mach/pmu.h>
#include <mach/fh_i2s_plat.h>
#ifdef CONFIG_USE_OF
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_mdio.h>
#include <linux/of_net.h>
#include <linux/of_platform.h>
#else
#include <linux/irqdomain.h>
#include <mach/pmu.h>
#endif

#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include <linux/fh_acw.h>

#define DEVICE_NAME				"fh_audio"

#define NR_DESCS_PER_CHANNEL 64

#define FIX_SAMPLE_BIT       32

#define ACW_CTRL						0x0
#define ACW_TXFIFO_CTRL					0x4
#define ACW_RXFIFO_CTRL					0x8
#define ACW_DAT_CTRL					0x0c
#define ACW_I2S_CTRL					0x10
#define ACW_TRACK_CTRL					0x14
#define ACW_DAT_CTRL0					0x18
#define ACW_DAT_CTRL1					0x1c

#define ACW_REG_DACL_FIFO_OFFSET		0x100
#define ACW_REG_DACR_FIFO_OFFSET		0x100
#define ACW_REG_ADC_FIFO_OFFSET			0x200

#define AUDIO_DMA_PREALLOC_SIZE (32*1024)

#define ACW_INTR_RX_UNDERFLOW   0x10000
#define ACW_INTR_RX_OVERFLOW    0x20000
#define ACW_INTR_TX_UNDERFLOW   0x40000
#define ACW_INTR_TX_OVERFLOW    0x80000

// #define FPGA_TEST
// #define FH_AUDIO_DEBUG
#ifdef FH_AUDIO_DEBUG
#define PRINT_AUDIO_DBG(fmt, args...)     \
	do { \
		pr_info( "FH_AUDIO_DEBUG: ");   \
		pr_info(fmt, ## args);       \
	} while(0)
#else
#define PRINT_AUDIO_DBG(fmt, args...)  do { } while (0)
#endif

static const struct file_operations acw_fops;

static struct audio_dev fh_audio_dev = {
	.channel_assign = {
		.capture_channel = -1,
		.playback_channel = -1,
	},
	.dma_master = 0,
	.fh_audio_miscdev = {
		.fops = &acw_fops,
		.name = DEVICE_NAME,
		.minor = MISC_DYNAMIC_MINOR,
	}

};

static struct {
	spinlock_t      lock;
	void __iomem    *paddr;
	void __iomem    *vaddr;
	struct clk      *clk;
	struct clk      *acodec_pclk;
	struct clk      *acodec_mclk;
	int         irq;
} fh_audio_module;

static struct infor_record_t infor_record;

//#define STERO
static struct fh_audio_dma *dma_rx_transfer = NULL;
static struct fh_audio_dma *dma_tx_transfer = NULL;
#ifdef	STERO
static struct fh_audio_dma *dma_tx_right_transfer = NULL;
#endif
static struct work_struct playback_wq;


#ifdef	STERO
static void fh_acw_tx_right_dma_done(void *arg);
#define TX_RIGHT_CHANNEL_DMA_CHANNEL 0
#endif
static void fh_acw_tx_dma_done(void *arg);
static void fh_acw_rx_dma_done(void *arg);
static bool  fh_acw_dma_chan_filter(struct dma_chan *chan, void *filter_param);

static void fh_acw_stop_playback(struct fh_audio_cfg *audio_config)
{
	unsigned int status;

	if(audio_config->playback.state == STATE_STOP) {
		return;
	}
	audio_config->playback.state = STATE_STOP;
	status = readl(fh_audio_module.vaddr + ACW_CTRL);
	status &=(~(1<<29));
	writel(status, fh_audio_module.vaddr + ACW_CTRL);//tx fifo disable
	fh_dma_cyclic_stop(dma_tx_transfer->chan);
	fh_dma_cyclic_free(dma_tx_transfer->chan);
#ifdef	STERO
	fh_dma_cyclic_stop(dma_tx_right_transfer->chan);
	fh_dma_cyclic_free(dma_tx_right_transfer->chan);
#endif
	up(&audio_config->sem_playback);
}

static void fh_acw_stop_capture(struct fh_audio_cfg *audio_config)
{
	u32 rx_status, status;

	if(audio_config->capture.state == STATE_STOP) {
		return;
	}
	rx_status = readl(fh_audio_module.vaddr + ACW_RXFIFO_CTRL);//clear rx fifo
	rx_status =  rx_status|(1<<4);
	writel(rx_status,fh_audio_module.vaddr + ACW_RXFIFO_CTRL);

	audio_config->capture.state = STATE_STOP;

	status = readl(fh_audio_module.vaddr + ACW_CTRL);
	status &=(~(1<<28));
	writel(status, fh_audio_module.vaddr + ACW_CTRL);//tx fifo disable

	fh_dma_cyclic_stop(dma_rx_transfer->chan);
	fh_dma_cyclic_free(dma_rx_transfer->chan);
	up(&audio_config->sem_capture);
}

static void init_audio(enum audio_type type,struct fh_audio_cfg  *audio_config)
{
#ifndef CONFIG_MACH_FH8830_FPGA
	// int reg;
//    reg = readl(fh_audio_module.vaddr + ACW_CTRL);
//    if ((reg & 0x80000000) == 0)
//    {
//        writel(0x80000000, fh_audio_module.vaddr + ACW_CTRL);/*enable audio*/
//    }
//    reg = readl(fh_audio_module.vaddr + ACW_MISC_CTRL);
//    if (0x40400 != reg)
//    {
//        writel(0x40400,fh_audio_module.vaddr + ACW_MISC_CTRL);/*misc ctl*/
//    }
//    if (capture == type)
//    {
//        writel(0x61141b06,fh_audio_module.vaddr + ACW_ADC_PATH_CTRL);/*adc cfg*/
//        writel(0x167f2307, fh_audio_module.vaddr + ACW_ADC_ALC_CTRL);/*adc alc*/
//        writel(0, fh_audio_module.vaddr + ACW_RXFIFO_CTRL);/*rx fifo disable*/
//        switch_input_volume(audio_config->capture.cfg.volume);
//        switch_rate(capture, audio_config->capture.cfg.rate);
//        switch_io_type(capture, audio_config->capture.cfg.io_type);
//    }
//    else
//    {
//        writel(0x3b403f09, fh_audio_module.vaddr + ACW_DAC_PATH_CTRL);/*dac cfg*/
//        writel(0, fh_audio_module.vaddr + ACW_TXFIFO_CTRL);/*tx fifo disable*/
//        switch_rate(playback, audio_config->playback.cfg.rate);
//        switch_io_type(playback, audio_config->playback.cfg.io_type);
//    }
#endif

}

static inline long bytes_to_frames(int frame_bit, int bytes)
{
	return bytes * 8 /frame_bit;
}

static inline long  frames_to_bytes(int frame_bit, int frames)
{
	return frames * frame_bit / 8;
}

static int avail_data_len(enum audio_type type,struct fh_audio_cfg *stream)
{
	int delta;
	if (capture == type)
	{
		spin_lock(&stream->capture.lock);
		delta = stream->capture.hw_ptr - stream->capture.appl_ptr;
		spin_unlock(&stream->capture.lock);
		if (delta < 0)
		{
			delta += stream->capture.size;
		}
		return delta;
	}
	else
	{
		spin_lock(&stream->playback.lock);
		delta = stream->playback.appl_ptr - stream->playback.hw_ptr;
		spin_unlock(&stream->playback.lock);
		if (delta < 0)
		{
			delta += stream->playback.size;
		}
		return stream->playback.size - delta;
	}
}

static int fh_audio_close(struct inode *ip, struct file *fp)
{
	struct miscdevice *miscdev = fp->private_data;
	struct audio_dev	*dev = container_of(miscdev, struct audio_dev, fh_audio_miscdev);
	struct fh_audio_cfg *audio_config = &dev->audio_config;
	int pid;

#ifndef CONFIG_MACH_FH8830_FPGA
	//disable interrupts
	u32 reg;
	reg = readl(fh_audio_module.vaddr + ACW_CTRL);
	reg &= ~(0x3ff);
	writel(reg, fh_audio_module.vaddr + ACW_CTRL);
#endif

	pid= current->tgid;
	if(infor_record.play_pid == pid)
	{
		fh_acw_stop_playback(audio_config);

	}
	if (infor_record.record_pid==pid)
	{
		fh_acw_stop_capture(audio_config);
	}
	return 0;
}

static int register_tx_dma(struct fh_audio_cfg  *audio_config)
{
	int ret;
	struct fh_dma_slave *tx_config;
	int dma_hs_num = fh_audio_dev.dma_tx_hs_num;

	tx_config =  kzalloc(sizeof(struct fh_dma_slave), GFP_KERNEL);
	if (!tx_config)
	{
		return -ENOMEM;
	}
#ifdef STERO
	//right channel
	struct dma_slave_config *tx_config_right;
	tx_config_right =  kzalloc(sizeof(struct dma_slave_config), GFP_KERNEL);
	if (!tx_config_right)
	{
		return -ENOMEM;
	}
	tx_config_right->slave_id = ACI2S_TX_HW_HANDSHAKE;
	tx_config_right->src_maxburst = FH_DMA_MSIZE_8;
	tx_config_right->dst_maxburst = FH_DMA_MSIZE_8;
	tx_config_right->dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	tx_config_right->device_fc = TRUE;
	tx_config_right->dst_addr = ACW_REG_DACR_FIFO_OFFSET;

	dma_tx_right_transfer->cdesc =fh_dma_cyclic_prep(dma_tx_right_transfer->chan,audio_config->playback.addr,
								   audio_config->playback.cfg.buffer_bytes,audio_config->playback.cfg.period_bytes, DMA_TO_DEVICE);
		if(dma_tx_transfer->cdesc <= 0)
		{
			pr_err("cyclic desc err\n");
			ret = -ENOMEM;
			goto fail;
		}
		dma_tx_transfer->cdesc->period_callback = fh_acw_tx_right_dma_done;
		dma_tx_transfer->cdesc->period_callback_param = audio_config;
		fh_dma_cyclic_start(dma_tx_right_transfer->chan);
		if (tx_slave_right)
		{
			kfree(tx_slave_right);
		}
		/*must set NULL to tell DMA driver that we free the DMA slave*/
		dma_tx_right_transfer->chan->private = NULL;
#endif

#ifdef CONFIG_FH_DMAC
	tx_config->cfg_hi = FHC_CFGH_DST_PER(dma_hs_num);
#else
	tx_config->cfg_hi = dma_hs_num;
#endif

	tx_config->dst_msize = FH_DMA_MSIZE_8;
	tx_config->src_msize = FH_DMA_MSIZE_8;
	tx_config->reg_width = FH_DMA_SLAVE_WIDTH_32BIT;
	tx_config->fc = FH_DMA_FC_D_M2P;
	tx_config->tx_reg = (u32)fh_audio_module.paddr + ACW_REG_DACL_FIFO_OFFSET;
	tx_config->src_master = 0;
	tx_config->dst_master = fh_audio_dev.dma_master;

	dma_tx_transfer->chan->private =  tx_config;
	if ((audio_config->playback.cfg.buffer_bytes < audio_config->playback.cfg.period_bytes) ||
		(audio_config->playback.cfg.buffer_bytes <= 0) || (audio_config->playback.cfg.period_bytes <= 0) ||
		(audio_config->playback.cfg.buffer_bytes/audio_config->playback.cfg.period_bytes > NR_DESCS_PER_CHANNEL))
	{
		pr_err( "buffer_size and period_size are invalid\n");
		ret = -EINVAL;
		goto fail;
	}

	dma_tx_transfer->cdesc =fh_dma_cyclic_prep(dma_tx_transfer->chan,audio_config->playback.addr,
							   audio_config->playback.cfg.buffer_bytes,audio_config->playback.cfg.period_bytes, DMA_MEM_TO_DEV);
	if(dma_tx_transfer->cdesc <= 0)
	{
		pr_err( "cyclic desc err\n");
		ret = -ENOMEM;
		goto fail;
	}
	dma_tx_transfer->cdesc->period_callback = fh_acw_tx_dma_done;
	dma_tx_transfer->cdesc->period_callback_param = audio_config;
	fh_dma_cyclic_start(dma_tx_transfer->chan);
	if (tx_config)
	{
		kfree(tx_config);
	}
	/*must set NULL to tell DMA driver that we free the DMA slave*/
	dma_tx_transfer->chan->private = NULL;
	return 0;
fail:
	return ret;
}

static int register_rx_dma(struct fh_audio_cfg  *audio_config)
{
	int ret,status;

	struct fh_dma_slave *rx_config;
	int dma_hs_num = fh_audio_dev.dma_rx_hs_num;
	rx_config =  kzalloc(sizeof(struct fh_dma_slave), GFP_KERNEL);
	if (!rx_config)
	{
		return -ENOMEM;
	}
#ifdef CONFIG_FH_DMAC
	rx_config->cfg_hi = FHC_CFGH_SRC_PER(dma_hs_num);
#else
	rx_config->cfg_hi = dma_hs_num;
#endif
	rx_config->dst_msize = FH_DMA_MSIZE_8;
	rx_config->src_msize = FH_DMA_MSIZE_8;
	rx_config->reg_width = FH_DMA_SLAVE_WIDTH_32BIT;
	rx_config->fc = FH_DMA_FC_D_P2M;
	rx_config->rx_reg = (u32)fh_audio_module.paddr + ACW_REG_ADC_FIFO_OFFSET;
	rx_config->src_master = fh_audio_dev.dma_master;
	rx_config->dst_master = 0;

	dma_rx_transfer->chan->private =  rx_config;
	if ((audio_config->capture.cfg.buffer_bytes < audio_config->capture.cfg.period_bytes) ||
		(audio_config->capture.cfg.buffer_bytes <= 0) ||(audio_config->capture.cfg.period_bytes <= 0) ||
		(audio_config->capture.cfg.buffer_bytes/audio_config->capture.cfg.period_bytes > NR_DESCS_PER_CHANNEL))
	{
		pr_err( "buffer_size and period_size are invalid\n");
		ret = -EINVAL;
		goto fail;
	}
	dma_rx_transfer->cdesc = fh_dma_cyclic_prep(dma_rx_transfer->chan,audio_config->capture.addr,
							   audio_config->capture.cfg.buffer_bytes, audio_config->capture.cfg.period_bytes,DMA_DEV_TO_MEM);
	if(dma_rx_transfer->cdesc <= 0)
	{
		pr_err(" cyclic desc err\n");
		ret = -ENOMEM;
		goto fail;
	}
	dma_rx_transfer->cdesc->period_callback = fh_acw_rx_dma_done;
	dma_rx_transfer->cdesc->period_callback_param = audio_config;
	fh_dma_cyclic_start(dma_rx_transfer->chan);

	writel(0x10024,fh_audio_module.vaddr + ACW_RXFIFO_CTRL);/*enable rx fifo*/
	status = readl(fh_audio_module.vaddr + ACW_CTRL);
	status |=(1<<28);
	writel(status, fh_audio_module.vaddr + ACW_CTRL);//tx fifo disable
	if (rx_config)
	{
		kfree(rx_config);
	}
	/*must set NULL to tell DMA driver that we free the DMA slave*/
	dma_rx_transfer->chan->private = NULL;
	return 0;
fail:
	kfree(rx_config);
	return ret;
}


static void playback_start_wq_handler(struct work_struct *work)
{
	int avail;
	unsigned int status;

	while (1) {
		if (STATE_STOP == fh_audio_dev.audio_config.playback.state)	{
			return;
		}
		avail = avail_data_len(playback, &fh_audio_dev.audio_config);
		if (avail > fh_audio_dev.audio_config.playback.cfg.period_bytes)
			msleep(0);
		else {
			writel(0x110024, fh_audio_module.vaddr + ACW_TXFIFO_CTRL); //enable tx fifo read enable
			status = readl(fh_audio_module.vaddr + ACW_CTRL);
			status |=(1<<29);
			writel(status, fh_audio_module.vaddr + ACW_CTRL);//tx fifo disable

			break;
		}
	}
}

static int fh_acw_start_playback(struct fh_audio_cfg *audio_config)
{
	int ret;

	if (audio_config->playback.state == STATE_RUN) {
		return 0;
	}

	if (audio_config->playback.cfg.buffer_bytes > AUDIO_DMA_PREALLOC_SIZE)	{
		pr_err("DMA prealloc buffer is smaller than  audio_config->buffer_bytes\n");
		return -ENOMEM;
	}
	memset(audio_config->playback.area, 0, audio_config->playback.cfg.buffer_bytes);
	audio_config->playback.size = audio_config->playback.cfg.buffer_bytes;
	audio_config->playback.state = STATE_RUN;
	ret = register_tx_dma(audio_config);
	if (ret < 0) {
		return ret;
	}
	INIT_WORK(&playback_wq, playback_start_wq_handler);
	schedule_work(&playback_wq);
	return 0;
}

static int fh_acw_start_capture(struct fh_audio_cfg *audio_config)
{
	if(audio_config->capture.state == STATE_RUN)
		return 0;

	if (audio_config->capture.cfg.buffer_bytes > AUDIO_DMA_PREALLOC_SIZE) {
		pr_err("DMA prealloc buffer is smaller than  audio_config->buffer_bytes\n");
		return -ENOMEM;
	}
	memset(audio_config->capture.area, 0, audio_config->capture.cfg.buffer_bytes);
	audio_config->capture.size = audio_config->capture.cfg.buffer_bytes;

	audio_config->capture.state = STATE_RUN;
	return register_rx_dma(audio_config);
}


static void fh_acw_rx_dma_done(void *arg)
{
	struct fh_audio_cfg *audio_config = (struct fh_audio_cfg *) arg;
	struct audio_ptr_t *cap = &audio_config->capture;
	int hw_pos = 0;

	spin_lock(&cap->lock);

	hw_pos = fh_dma_get_dst_addr(dma_rx_transfer->chan) - cap->addr;

	/* align to period_bytes*/
	hw_pos -= hw_pos % cap->cfg.period_bytes;

	cap->hw_ptr = hw_pos;

	spin_unlock(&cap->lock);
	if (waitqueue_active(&audio_config->readqueue)) {
		int avail = avail_data_len(capture, audio_config);

		if (avail >= cap->cfg.period_bytes)
			wake_up_interruptible(&audio_config->readqueue);
	}
}
#ifdef	STERO
static void fh_acw_tx_right_dma_done(void *arg)
{
	pr_info("tx_right_dam_done\n");

}
#endif

static void fh_acw_tx_dma_done(void *arg)
{
	struct fh_audio_cfg *audio_config = (struct fh_audio_cfg *) arg;
	struct audio_ptr_t *play = &audio_config->playback;
	int hw_pos = 0;
	int delta = 0;

	spin_lock(&play->lock);

	hw_pos = fh_dma_get_src_addr(dma_tx_transfer->chan) - play->addr;

	/* align to period_bytes*/
	hw_pos -= hw_pos % play->cfg.period_bytes;
	delta = hw_pos - play->hw_ptr;

	if (delta < 0) {
		memset(play->area + play->hw_ptr, 0, play->size - play->hw_ptr);
		memset(play->area, 0, hw_pos);
	} else
		memset(play->area + play->hw_ptr, 0, delta);

	play->hw_ptr = hw_pos;

	spin_unlock(&play->lock);
	if (waitqueue_active(&audio_config->writequeue)) {
		int avail = avail_data_len(playback, audio_config);

		if (avail >= play->cfg.period_bytes)
			wake_up_interruptible(&audio_config->writequeue);
	}
}

bool  fh_acw_dma_chan_filter(struct dma_chan *chan, void *filter_param)
{
	int dma_channel = *(int *)filter_param;
	bool ret = false;

	if (chan->chan_id == dma_channel)
	{
		ret = true;
	}
	return ret;
}

static int arg_config_support(struct fh_audio_cfg_arg * cfg)
{
	return 0;
}

static void reset_dma_buff(enum audio_type type, struct fh_audio_cfg *audio_config)
{
	if (capture == type)
	{
		audio_config->capture.appl_ptr = 0;
		audio_config->capture.hw_ptr = 0;
	}
	else
	{
		audio_config->playback.appl_ptr = 0;
		audio_config->playback.hw_ptr = 0;
	}
}

unsigned int config_flag = 0;
static long fh_audio_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct fh_audio_cfg_arg cfg;
	struct miscdevice *miscdev = filp->private_data;
	struct audio_dev  *dev = container_of(miscdev, struct audio_dev, fh_audio_miscdev);
	struct fh_audio_cfg  *audio_config = &dev->audio_config;
	int pid;
	int rx_status, tx_status;

	pid = current->tgid;
	switch (cmd)
	{
		case AC_INIT_CAPTURE_MEM:

			if (copy_from_user((void *)&cfg, (void __user *)arg, sizeof(struct fh_audio_cfg_arg)))
			{
				pr_err( "copy err\n");
				return -EIO;
			}
			if (0 == arg_config_support(&cfg))
			{
				if (down_trylock(&audio_config->sem_capture))
				{
					pr_err( "another thread is running capture.\n");
					return -EBUSY;
				}
				infor_record.record_pid =  pid;
				audio_config->capture.cfg.io_type = cfg.io_type;
				audio_config->capture.cfg.volume = cfg.volume;
				audio_config->capture.cfg.rate = cfg.rate;
				audio_config->capture.cfg.channels = cfg.channels;
				audio_config->capture.cfg.buffer_size = cfg.buffer_size;
				audio_config->capture.cfg.frame_bit = FIX_SAMPLE_BIT;
				audio_config->capture.cfg.period_size = cfg.period_size;
				audio_config->capture.cfg.buffer_bytes = frames_to_bytes(audio_config->capture.cfg.frame_bit,audio_config->capture.cfg.buffer_size);
				audio_config->capture.cfg.period_bytes = frames_to_bytes(audio_config->capture.cfg.frame_bit,audio_config->capture.cfg.period_size);
				audio_config->capture.cfg.start_threshold =audio_config->capture.cfg.buffer_bytes;
				audio_config->capture.cfg.stop_threshold = audio_config->capture.cfg.buffer_bytes;
				reset_dma_buff(capture, audio_config);
				init_waitqueue_head(&audio_config->readqueue);
				spin_lock_init(&audio_config->capture.lock);
				init_audio(capture, audio_config);

				/*  * config wrapper work format  *   */
				writel(0x55, fh_audio_module.vaddr + ACW_DAT_CTRL);//set work format
				writel(0x111, fh_audio_module.vaddr + ACW_I2S_CTRL);//set work format
				writel(0x00, fh_audio_module.vaddr + ACW_TRACK_CTRL);
				config_flag = 1;
			}
			else
			{
				return -EINVAL;
			}

			break;
		case AC_INIT_PLAYBACK_MEM:
			if (copy_from_user((void *)&cfg, (void __user *)arg, sizeof(struct fh_audio_cfg_arg)))
			{
				pr_err( "copy err\n");
				return -EIO;
			}

			if (0 == arg_config_support(&cfg))
			{
				if (down_trylock(&audio_config->sem_playback))
				{
					pr_err( "another thread is running playback.\n");
					return -EBUSY;
				}
				infor_record.play_pid = pid;
				audio_config->playback.cfg.io_type = cfg.io_type;
				audio_config->playback.cfg.volume = cfg.volume;
				audio_config->playback.cfg.rate = cfg.rate;
				audio_config->playback.cfg.channels = cfg.channels;
				audio_config->playback.cfg.buffer_size = cfg.buffer_size;
				audio_config->playback.cfg.frame_bit = FIX_SAMPLE_BIT;
				audio_config->playback.cfg.period_size = cfg.period_size;
				audio_config->playback.cfg.buffer_bytes = frames_to_bytes(audio_config->playback.cfg.frame_bit,audio_config->playback.cfg.buffer_size);
				audio_config->playback.cfg.period_bytes = frames_to_bytes(audio_config->playback.cfg.frame_bit,audio_config->playback.cfg.period_size);
				audio_config->playback.cfg.start_threshold =audio_config->playback.cfg.buffer_bytes;
				audio_config->playback.cfg.stop_threshold = audio_config->playback.cfg.buffer_bytes;
				reset_dma_buff(playback, audio_config);
				if(!config_flag){
					/*  * config wrapper work format  *   */
					writel(0x55, fh_audio_module.vaddr + ACW_DAT_CTRL);//set work format
					writel(0x111, fh_audio_module.vaddr + ACW_I2S_CTRL);//set work format

					writel(0x00, fh_audio_module.vaddr + ACW_TRACK_CTRL); //select left channel
					/** dont config again **/
					config_flag = 1;

				}
				init_waitqueue_head(&audio_config->writequeue);
				spin_lock_init(&audio_config->playback.lock);
				init_audio(playback, audio_config);
			}
			else
			{
				return -EINVAL;
			}
			break;
		case AC_AI_EN:
			if (infor_record.record_pid != pid){
				return -EBUSY;
			}
			return fh_acw_start_capture(audio_config);
		case AC_AO_EN:
			if (infor_record.play_pid != pid) {
				return -EBUSY;
			}
			return fh_acw_start_playback(audio_config);
		case AC_SET_VOL:
		case AC_SET_INPUT_MODE:
		case AC_SET_OUTPUT_MODE:
			return 0;
		case AC_AI_DISABLE:
			pr_info("[ac_driver]AC_AI_DISABLE\n");
			if (infor_record.record_pid != pid) {
				return -EBUSY;
			}
			fh_acw_stop_capture(audio_config);
			pr_info(" AC_AI_DISABLE\n");
			break;
		case AC_AO_DISABLE:
			pr_info("[ac_driver]AC_AO_DISABLE\n");
			if (infor_record.play_pid != pid) {
				return -EBUSY;
			}
			fh_acw_stop_playback(audio_config);
			pr_info(" AC_AO_DISABLE\n");
			break;
		case AC_AI_PAUSE:
			if (infor_record.record_pid != pid) {
				return -EBUSY;
			}
			pr_info( "capture pause\n");
			audio_config->capture.state = STATE_PAUSE;
			rx_status = readl(fh_audio_module.vaddr + ACW_RXFIFO_CTRL);/*rx fifo disable*/
			rx_status =  rx_status&(~(1<<0));
			writel(rx_status, fh_audio_module.vaddr + ACW_RXFIFO_CTRL);/*rx fifo disable*/
			break;
		case AC_AI_RESUME:
			if (infor_record.record_pid != pid) {
				return -EBUSY;
			}
			pr_info( "capture resume\n");
			audio_config->capture.state = STATE_RUN;
#ifndef CONFIG_MACH_FH8830_FPGA
			rx_status = readl(fh_audio_module.vaddr + ACW_RXFIFO_CTRL);//clear rx fifo
			rx_status =  rx_status|(1<<4);
			writel(rx_status,fh_audio_module.vaddr + ACW_RXFIFO_CTRL);/*enable rx fifo*/
			rx_status =  rx_status&(~(1<<4));
			rx_status =  rx_status|(1<<0);
			writel(rx_status,fh_audio_module.vaddr + ACW_RXFIFO_CTRL);/*enable rx fifo*/
#endif
			break;
		case AC_AO_PAUSE:
			if (infor_record.play_pid != pid) {
				return -EBUSY;
			}
			audio_config->playback.state = STATE_PAUSE;
			pr_info( "playback pause\n");
			tx_status = readl(fh_audio_module.vaddr + ACW_TXFIFO_CTRL);/*rx fifo disable*/
			tx_status =  tx_status&(~(1<<0));
			writel(tx_status, fh_audio_module.vaddr + ACW_TXFIFO_CTRL);/*tx fifo disable*/
			break;
		case AC_AO_RESUME:
			if (infor_record.play_pid != pid) {
				return -EBUSY;
			}
			pr_info( "playback resume\n");
			audio_config->playback.state = STATE_RUN;
			tx_status = readl(fh_audio_module.vaddr + ACW_TXFIFO_CTRL);//clear rx fifo
			tx_status =  tx_status|(1<<0);
			writel(tx_status,fh_audio_module.vaddr + ACW_TXFIFO_CTRL); //enable tx fifo read enable
			break;
		default:
			return -ENOTTY;
	}
	return 0;
}

static int fh_audio_open(struct inode *ip, struct file *fp)
{
	u32 reg;

	fp->private_data = &fh_audio_dev.fh_audio_miscdev;
	//enable interrupts
	reg = readl(fh_audio_module.vaddr + ACW_CTRL);
	reg |= 0xa; //tx & rx, under & overflow
	writel(reg, fh_audio_module.vaddr + ACW_CTRL);
	return 0;
}

static u32 fh_audio_poll(struct file *filp, poll_table *wait)
{
	struct miscdevice *miscdev = filp->private_data;
	struct audio_dev  *dev = container_of(miscdev, struct audio_dev, fh_audio_miscdev);
	struct fh_audio_cfg  *audio_config = &dev->audio_config;
	u32 mask = 0;
	long avail;
	if (STATE_RUN == audio_config->capture.state)
	{
		poll_wait(filp,&audio_config->readqueue,wait);
		avail = avail_data_len(capture, audio_config);
		if (avail >  audio_config->capture.cfg.period_bytes)
		{
			mask |=  POLLIN | POLLRDNORM;
		}
	}
	if (STATE_RUN == audio_config->playback.state)
	{
		poll_wait(filp,&audio_config->writequeue,wait);
		avail = avail_data_len(playback, audio_config);
		if (avail >  audio_config->playback.cfg.period_bytes)
		{
			mask |=  POLLOUT | POLLWRNORM;
		}
	}
	return mask;
}

static int fh_audio_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{

	int ret;
	struct miscdevice *miscdev = filp->private_data;
	struct audio_dev  *dev = container_of(miscdev, struct audio_dev, fh_audio_miscdev);
	struct fh_audio_cfg  *audio_config = &dev->audio_config;
	int after,left;
	int pid,avail;
	pid = current->tgid;
	if (infor_record.record_pid != pid){
		return -EBUSY;
	}

	avail = avail_data_len(capture, audio_config);
	if (avail > len)
	{
		avail = len;
	}
	after = avail + audio_config->capture.appl_ptr;
	if(after  > audio_config->capture.size)
	{
		left = avail - (audio_config->capture.size - audio_config->capture.appl_ptr);
		ret = copy_to_user(buf, audio_config->capture.area+audio_config->capture.appl_ptr, audio_config->capture.size-audio_config->capture.appl_ptr);
		ret = copy_to_user(buf+audio_config->capture.size-audio_config->capture.appl_ptr,audio_config->capture.area,left);
		spin_lock(&audio_config->capture.lock);
		audio_config->capture.appl_ptr = left;
		spin_unlock(&audio_config->capture.lock);
	}
	else
	{
		ret = copy_to_user(buf,audio_config->capture.area+audio_config->capture.appl_ptr,avail);
		spin_lock(&audio_config->capture.lock);
		audio_config->capture.appl_ptr += avail;
		spin_unlock(&audio_config->capture.lock);
	}

	return avail;

}

static int fh_audio_write(struct file *filp, const char __user *buf,
							size_t len, loff_t *off)
{

	struct miscdevice *miscdev = filp->private_data;
	struct audio_dev  *dev = container_of(miscdev, struct audio_dev, fh_audio_miscdev);
	struct fh_audio_cfg  *audio_config = &dev->audio_config;
	int  ret;
	int after,left;
	int pid,avail;
   pid = current->tgid;
   if (infor_record.play_pid != pid){
	return -EBUSY;
   }
	avail = avail_data_len(playback,audio_config);
	if (0 == avail)
	{
		return 0;
	}
	if (avail > len)
	{
		avail = len;
	}
	after = avail+audio_config->playback.appl_ptr;
	if(after  > audio_config->playback.size)
	{
		left = avail - (audio_config->playback.size-audio_config->playback.appl_ptr);
		ret = copy_from_user(audio_config->playback.area+audio_config->playback.appl_ptr,buf,audio_config->playback.size-audio_config->playback.appl_ptr);
		ret = copy_from_user(audio_config->playback.area,buf+audio_config->playback.size-audio_config->playback.appl_ptr,left);
		spin_lock(&audio_config->playback.lock);
		audio_config->playback.appl_ptr = left;
		spin_unlock(&audio_config->playback.lock);
	}
	else
	{
		ret = copy_from_user(audio_config->playback.area+audio_config->playback.appl_ptr,buf,avail);
		spin_lock(&audio_config->playback.lock);
		audio_config->playback.appl_ptr += avail;
		spin_unlock(&audio_config->playback.lock);
	}

	 return avail;
}

static irqreturn_t fh_audio_interrupt(int irq, void *dev_id)
{
	u32 interrupts, rx_status;
	struct fh_audio_cfg  *audio_config = &fh_audio_dev.audio_config;

	interrupts = readl(fh_audio_module.vaddr + ACW_CTRL);
	//interrupts &= ~(0x3ff) << 16;
	writel(interrupts, fh_audio_module.vaddr + ACW_CTRL);

	if(interrupts & ACW_INTR_RX_UNDERFLOW)
	{
		fh_acw_stop_capture(audio_config);
		fh_acw_start_capture(audio_config);
		PRINT_AUDIO_DBG("ACW_INTR_RX_UNDERFLOW\n");
	}

	if(interrupts & ACW_INTR_RX_OVERFLOW)
	{
		if (audio_config->capture.state == STATE_RUN) {
			fh_acw_stop_capture(audio_config);
			fh_acw_start_capture(audio_config);
		} else {
			rx_status = readl(fh_audio_module.vaddr + ACW_RXFIFO_CTRL);//clear rx fifo
			rx_status =  rx_status|(1<<4);
			writel(rx_status,fh_audio_module.vaddr + ACW_RXFIFO_CTRL);
		}
		PRINT_AUDIO_DBG("ACW_INTR_RX_OVERFLOW\n");
	}

	if(interrupts & ACW_INTR_TX_UNDERFLOW)
	{
		fh_acw_stop_playback(audio_config);
		fh_acw_start_playback(audio_config);
		PRINT_AUDIO_DBG("ACW_INTR_TX_UNDERFLOW\n");
	}

	if(interrupts & ACW_INTR_TX_OVERFLOW)
	{
		fh_acw_stop_playback(audio_config);
		fh_acw_start_playback(audio_config);
		PRINT_AUDIO_DBG("ACW_INTR_TX_OVERFLOW\n");
	}

	PRINT_AUDIO_DBG("interrupts: 0x%x\n", interrupts);

	return IRQ_HANDLED;
}

static const struct file_operations acw_fops =
{
	.owner      = THIS_MODULE,
	.llseek     = no_llseek,
	.unlocked_ioctl = fh_audio_ioctl,
	.release = fh_audio_close,
	.open = fh_audio_open,
	.poll = fh_audio_poll,
	.read = fh_audio_read,
	.write = fh_audio_write,

};

static int fh_audio_drv_probe(struct platform_device *pdev)
{
	int ret = 0;
	int irq;
	int rx_dma_channel, tx_dma_channel, dma_master;
	int dma_rx_hs_num, dma_tx_hs_num;
	struct resource *res_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
#ifdef CONFIG_USE_OF
	struct device_node *np = pdev->dev.of_node;

	fh_audio_module.vaddr = of_iomap(np, 0);

	ret = of_property_read_s32(np, "rx_dma_channel", &rx_dma_channel);
	ret |= of_property_read_s32(np, "tx_dma_channel", &tx_dma_channel);
	ret |= of_property_read_s32(np, "dma_master", &dma_master);
	ret |= of_property_read_s32(np, "dma_rx_hs_num", &dma_rx_hs_num);
	ret |= of_property_read_s32(np, "dma_tx_hs_num", &dma_tx_hs_num);

	if (ret) {
		ret = -EINVAL;
		dev_err(&pdev->dev, "error when parse dmacfg %s", np->full_name);
		goto out_return;
	}

	irq = irq_of_parse_and_map(np, 0);

	fh_audio_module.clk = of_clk_get_by_name(np, "ac_clk");

	fh_audio_module.acodec_pclk = of_clk_get_by_name(np, "acodec_pclk");
	fh_audio_module.acodec_mclk = of_clk_get_by_name(np, "acodec_mclk");

#else
	struct fh_i2s_platform_data *plat_data =
			(struct fh_i2s_platform_data *)dev_get_platdata(&pdev->dev);

	rx_dma_channel = plat_data->dma_capture_channel;
	tx_dma_channel = plat_data->dma_playback_channel;
	dma_master = plat_data->dma_master;
	dma_rx_hs_num = plat_data->dma_rx_hs_num;
	dma_tx_hs_num = plat_data->dma_tx_hs_num;

	fh_audio_module.vaddr = devm_ioremap(&pdev->dev, res_mem->start,
			resource_size(res_mem));

	irq = irq_create_mapping(NULL, ACW_IRQ);

	fh_audio_module.clk = clk_get(NULL, plat_data->clk);
	if (plat_data->acodec_pclk)
		fh_audio_module.acodec_pclk = clk_get(NULL, plat_data->acodec_pclk);
	if (plat_data->acodec_mclk)
		fh_audio_module.acodec_mclk = clk_get(NULL, plat_data->acodec_mclk);
#endif

	if (res_mem)
		fh_audio_module.paddr = (void *)res_mem->start;


	if (!fh_audio_module.vaddr || !fh_audio_module.paddr) {
		ret = -ENOMEM;
		goto out_return;
	}


	if (rx_dma_channel < 0 || tx_dma_channel < 0 || dma_master < 0) {
		ret = -EINVAL;
		dev_err(&pdev->dev, "invalid dma_channel");
		goto out_return;
	} else if (dma_rx_hs_num < 0 || dma_tx_hs_num < 0) {
		ret = -EINVAL;
		dev_err(&pdev->dev, "invalid dma_hs_num");
		goto out_return;
	} else {
		fh_audio_dev.channel_assign.capture_channel = rx_dma_channel;
		fh_audio_dev.channel_assign.playback_channel = tx_dma_channel;
		fh_audio_dev.dma_master = dma_master;
		fh_audio_dev.dma_rx_hs_num = dma_rx_hs_num;
		fh_audio_dev.dma_tx_hs_num = dma_tx_hs_num;
		dev_info(&pdev->dev, "rx_dma_channel %d, tx_dma_channel %d, \
		dma_master %d", rx_dma_channel, tx_dma_channel, dma_master);
	}

	if (IS_ERR(fh_audio_module.clk)) {
		fh_audio_module.clk = NULL;
		dev_err(&pdev->dev, "failed to get acw clk\n");
	} else {
		clk_prepare_enable(fh_audio_module.clk);
	}

	if (IS_ERR(fh_audio_module.acodec_pclk))
		fh_audio_module.acodec_pclk = NULL;
	else
		clk_prepare_enable(fh_audio_module.acodec_pclk);

	if (IS_ERR(fh_audio_module.acodec_mclk))
		fh_audio_module.acodec_mclk = NULL;
	else
		clk_prepare_enable(fh_audio_module.acodec_mclk);

	spin_lock_init(&fh_audio_module.lock);

	ret = misc_register(&fh_audio_dev.fh_audio_miscdev);

	if (ret)
		goto out_return;

	if (irq <= 0) {
		dev_info(&pdev->dev, "%s: ERROR: cannot get irq\n", __func__);
		ret = -ENXIO;
		goto out_return;
	}

	fh_audio_module.irq = irq;

	ret = devm_request_irq(&pdev->dev, irq, fh_audio_interrupt, IRQF_SHARED,
			KBUILD_MODNAME, &fh_audio_module);

	if (ret)
		goto out_return;

	dev_info(&pdev->dev, "FH ACW Driver\n");
	return 0;

out_return:
	return ret;
}

static int fh_acw_drv_remove(struct platform_device *pdev)
{
	misc_deregister(&fh_audio_dev.fh_audio_miscdev);

	devm_free_irq(&pdev->dev, fh_audio_module.irq, &fh_audio_module);

	if (fh_audio_module.clk) {
		clk_disable_unprepare(fh_audio_module.clk);
		clk_put(fh_audio_module.clk);
	}
	dev_info(&pdev->dev, "FH ACW Driver Removed\n");
	return 0;
}

static const struct of_device_id fh_dma_of_id_table[] = {
	{ .compatible = "fh,fh-acw" },
	{}
};

static struct platform_driver fh_audio_driver = {
	.probe      = fh_audio_drv_probe,
	.remove     = fh_acw_drv_remove,
	.driver     = {
		.name   = DEVICE_NAME,
		.owner  = THIS_MODULE,
		.of_match_table = of_match_ptr(fh_dma_of_id_table),
	}
};

static int audio_prealloc_dma_buffer(struct fh_audio_cfg  *audio_config)
{
	int pg;
	gfp_t gfp_flags;
	pg = get_order(AUDIO_DMA_PREALLOC_SIZE);
	gfp_flags = GFP_KERNEL | __GFP_COMP | __GFP_NORETRY | __GFP_NOWARN;
	audio_config->capture.dev.coherent_dma_mask = DMA_BIT_MASK(32);
	audio_config->capture.area  = dma_alloc_coherent(&audio_config->capture.dev, PAGE_SIZE << pg, &audio_config->capture.addr, gfp_flags );
	if (!audio_config->capture.area)
	{
		pr_err("no enough mem for capture  buffer alloc\n");
		return -1;
	}
	audio_config->playback.dev.coherent_dma_mask = DMA_BIT_MASK(32);
	audio_config->playback.area  = dma_alloc_coherent(&audio_config->playback.dev, PAGE_SIZE << pg, &audio_config->playback.addr, gfp_flags );
	if (!audio_config->playback.area)
	{
		pr_err("no enough mem for  playback buffer alloc\n");
		return -1;
	}

	return 0;
}

static void audio_free_prealloc_dma_buffer(struct fh_audio_cfg  *audio_config)
{
	int pg;
	pg = get_order(AUDIO_DMA_PREALLOC_SIZE);
	dma_free_coherent(&audio_config->capture.dev, PAGE_SIZE<<pg, audio_config->capture.area, audio_config->capture.addr);
	dma_free_coherent(&audio_config->playback.dev, PAGE_SIZE<<pg, audio_config->playback.area, audio_config->playback.addr);
}

static void init_audio_mutex(struct fh_audio_cfg  *audio_config)
{
	sema_init(&audio_config->sem_capture, 1);
	sema_init(&audio_config->sem_playback, 1);
}

static int audio_request_dma_channel(void)
{
	dma_cap_mask_t mask;
	/*request audio rx dma channel*/
	dma_rx_transfer = kzalloc(sizeof(struct fh_audio_dma), GFP_KERNEL);
	if (!dma_rx_transfer)
	{
		pr_err("alloc  dma_rx_transfer failed\n");
		goto mem_fail;
	}
	memset(dma_rx_transfer, 0, sizeof(struct fh_audio_dma));

	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);
	dma_rx_transfer->chan = dma_request_channel(mask, fh_acw_dma_chan_filter, &fh_audio_dev.channel_assign.capture_channel);
	if (!dma_rx_transfer->chan)
	{
		pr_err("request audio rx dma channel failed \n");
		goto channel_fail;
	}

	/*request audio tx dma channel*/
	dma_tx_transfer = kzalloc(sizeof(struct fh_audio_dma), GFP_KERNEL);
	if (!dma_tx_transfer)
	{
		pr_err("alloc  dma_tx_transfer failed\n");
		goto mem_fail;
	}
	memset(dma_tx_transfer, 0, sizeof(struct fh_audio_dma));

	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);
	dma_tx_transfer->chan = dma_request_channel(mask, fh_acw_dma_chan_filter, &fh_audio_dev.channel_assign.playback_channel);
	if (!dma_tx_transfer->chan)
	{
		pr_err("request dma channel failed \n");
		return -EFAULT;
	}
#ifdef STERO
	/*request audio tx dma channel*/
	dma_tx_right_transfer = kzalloc(sizeof(struct fh_audio_dma), GFP_KERNEL);
	if (!dma_tx_right_transfer)
	{
		pr_err("alloc  dma_tx_right_transfer failed\n");
		goto mem_fail;
	}
	memset(dma_tx_right_transfer, 0, sizeof(struct fh_audio_dma));

	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);
	dma_tx_right_transfer->chan = dma_request_channel(mask, TX_RIGHT_CHANNEL_DMA_CHANNEL, &fh_audio_dev.channel_assign.playback_channel);
	if (!dma_tx_right_transfer->chan)
	{
		pr_err("request dma channel dma_tx_right_transfer failed \n");
		return -EFAULT;
	}
#endif
	return 0;
channel_fail:
	if (dma_rx_transfer->chan)
	{
		dma_release_channel(dma_rx_transfer->chan);
		dma_rx_transfer->chan = NULL;
	}
	if (dma_tx_transfer->chan)
	{
		dma_release_channel(dma_tx_transfer->chan);
		dma_tx_transfer->chan = NULL;
	}
#ifdef STERO
	if (dma_tx_right_transfer->chan)
	{
		dma_release_channel(dma_tx_right_transfer->chan);
		dma_tx_right_transfer->chan = NULL;
	}
#endif
mem_fail:
	if (dma_rx_transfer != NULL)
	{
		kfree(dma_rx_transfer);
		dma_rx_transfer = NULL;
	}
	if (dma_tx_transfer != NULL)
	{
		kfree(dma_tx_transfer);
		dma_tx_transfer = NULL;
	}
#ifdef	STERO
	if (dma_tx_right_transfer != NULL)
	{
		kfree(dma_tx_right_transfer);
		dma_tx_right_transfer = NULL;
	}
#endif
	return -EFAULT;
}

static void audio_release_dma_channel(void)
{
	/*release audio tx dma channel*/
	if (dma_tx_transfer != NULL)
	{
		if (dma_tx_transfer->chan) {
			dma_release_channel(dma_tx_transfer->chan);
			dma_tx_transfer->chan = NULL;
		}
		kfree(dma_tx_transfer);
		dma_tx_transfer = NULL;
	}

	/*release audio rx dma channel*/
	if (dma_rx_transfer != NULL)
	{
		if (dma_rx_transfer->chan) {
			dma_release_channel(dma_rx_transfer->chan);
			dma_rx_transfer->chan = NULL;
		}

		kfree(dma_rx_transfer);
		dma_rx_transfer = NULL;
	}
#ifdef STERO
	/*release audio tx dma channel*/
	if (dma_tx_right_transfer != NULL)
	{
		if (dma_tx_right_transfer->chan) {
			dma_release_channel(dma_tx_right_transfer->chan);
			dma_tx_right_transfer->chan = NULL;
		}
		kfree(dma_tx_right_transfer);
		dma_tx_right_transfer = NULL;
	}

#endif

}

static int __init fh_audio_init(void)
{
	int ret = 0;
#ifdef CONFIG_FH_ACW_PROC
	create_proc();
#endif
	init_audio_mutex(&fh_audio_dev.audio_config);
	ret = platform_driver_register(&fh_audio_driver);
	if (ret)
		goto failed;
	ret = audio_prealloc_dma_buffer(&fh_audio_dev.audio_config);
	if (ret)
		goto unregister_driver;
	ret = audio_request_dma_channel();
	if (ret)
		goto free_dma_buffer;

	return 0;

free_dma_buffer:
	audio_free_prealloc_dma_buffer(&fh_audio_dev.audio_config);
unregister_driver:
	platform_driver_unregister(&fh_audio_driver);
failed:
#ifdef CONFIG_FH_ACW_PROC
	remove_proc();
#endif
	return ret;
}
module_init(fh_audio_init);

static void __exit fh_audio_exit(void)
{

#ifdef CONFIG_FH_ACW_PROC
	remove_proc();
#endif
	audio_release_dma_channel();
	audio_free_prealloc_dma_buffer(&fh_audio_dev.audio_config);
	platform_driver_unregister(&fh_audio_driver);
}
module_exit(fh_audio_exit);

MODULE_AUTHOR("Fullhan");
MODULE_DESCRIPTION("Fullhan ACW device driver");
MODULE_LICENSE("GPL");

/****************************debug proc*****************************/
#ifdef CONFIG_FH_ACW_PROC
#include <linux/proc_fs.h>

#define PROC_NAME "driver/"DEVICE_NAME
static struct proc_dir_entry *proc_file;

static void *v_seq_start(struct seq_file *s, loff_t *pos)
{
	static unsigned long counter = 0;

	if (*pos == 0)
		return &counter;
	else
	{
		*pos = 0;
		return NULL;
	}
}

static void *v_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	(*pos)++;
	return NULL;
}

static void v_seq_stop(struct seq_file *s, void *v)
{

}

static int v_seq_show(struct seq_file *sfile, void *v)
{
	int i;
	u32 data;

	if (fh_audio_module.vaddr == NULL)
		seq_printf(sfile, "reg is null\n");
	else {
		for (i = 0; i <= 0x20; i += 4) {
			data = readl(fh_audio_module.vaddr + i);
			seq_printf(sfile, "0x%02x reg = 0x%x\n", i, data);
		}
	}

	return 0;
}

static const struct seq_operations fh_acw_seq_ops = {
	.start = v_seq_start,
	.next = v_seq_next,
	.stop = v_seq_stop,
	.show = v_seq_show
};

static int proc_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &fh_acw_seq_ops);
}

static struct file_operations fh_acw_proc_ops =
{
	.owner = THIS_MODULE,
	.open = proc_open,
	.read = seq_read,
};

static void create_proc(void)
{
	proc_file = proc_create(PROC_NAME, 0644, NULL, &fh_acw_proc_ops);

	if (proc_file == NULL)
		pr_err("%s: ERROR: %s proc file create failed",
			   __func__, DEVICE_NAME);

}

static void remove_proc(void)
{
	remove_proc_entry(PROC_NAME, NULL);
}
#endif
