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
#include <mach/fh_dmac.h>
#include <mach/fh_predefined.h>

#include "ac.h"
struct i2c_adapter *codec_i2c_adapter;
#define NR_DESCS_PER_CHANNEL 64
#define DW_I2S_FH8833 0xf0900000
#define FIX_SAMPLE_BIT       32

#define ACW_HW_NUM_RX  0
#define ACW_HW_NUM_TX  1
#define ACW_DMA_CAP_CHANNEL 3
#define ACW_DMA_PAY_CHANNEL 2

#define ACW_CTRL 						0x0
#define ACW_TXFIFO_CTRL 				0x4
#define ACW_RXFIFO_CTRL 				0x8
#define ACW_STATUS             			0x0c
#define ACW_DAT_CTL				  		0x10
#define ACW_DBG_CTL					  	0x14
#define ACW_STATUS1					  	0x18
#define ACW_STATUS2						0x1c

#define ACW_DACL_FIFO 					0xf0a00100
#define ACW_DACR_FIFO 					0xf0a00100
#define ACW_ADC_FIFO					0xf0a00200

#define AUDIO_DMA_PREALLOC_SIZE 128*1024

#define ACW_INTR_RX_UNDERFLOW   0x10000
#define ACW_INTR_RX_OVERFLOW    0x20000
#define ACW_INTR_TX_UNDERFLOW   0x40000
#define ACW_INTR_TX_OVERFLOW    0x80000

 //#define FH_AUDIO_DEBUG
#ifdef FH_AUDIO_DEBUG
#define PRINT_AUDIO_DBG(fmt, args...)     \
    do                              \
    {                               \
        printk("FH_AUDIO_DEBUG: ");   \
        printk(fmt, ## args);       \
    }                               \
    while(0)
#else
#define PRINT_AUDIO_DBG(fmt, args...)  do { } while (0)
#endif

enum audio_type
{
    capture = 0,
    playback,
};

int i2c_write_codec(u8 addr, u8 data) {
		int rval;
		struct i2c_msg msgs[1];
		u8 send[2];
		msgs[0].len = 2;
		msgs[0].addr = 0x1b;
		msgs[0].flags = 0;
		msgs[0].buf = send;
		send[0] = addr;
		send[1] = data;
		rval = i2c_transfer(codec_i2c_adapter, msgs, 1);
		return rval;
}

enum audio_state
{
    STATE_NORMAL = 0,
    STATE_XRUN,
    STATE_STOP,
    STATE_RUN,
    STATE_PAUSE
};

struct infor_record_t
{
	int record_pid;
	int play_pid;
}infor_record;

struct audio_config {
	int rate;
	int volume;
	enum io_select io_type;
    int frame_bit;
    int channels;
    int buffer_size;
    int period_size;
    int buffer_bytes;
    int period_bytes;
    int start_threshold;
    int stop_threshold;
};

struct audio_ptr_t
{
	struct audio_config cfg;
    enum audio_state state;
    long size;
    int hw_ptr;
    int appl_ptr;
    spinlock_t lock;
    struct device dev;
    u8 *area; /*virtual pointer*/
    dma_addr_t addr; /*physical address*/
    u8 * mmap_addr;
};

struct fh_audio_cfg
{
    struct audio_ptr_t capture;
    struct audio_ptr_t playback;
    wait_queue_head_t readqueue;
    wait_queue_head_t writequeue;
	struct semaphore sem_capture;
	struct semaphore sem_playback;
};

struct fh_dma_chan
{
    struct dma_chan     *chan;
    void __iomem        *ch_regs;
    u8          mask;
    u8          priority;
    bool            paused;
    bool                initialized;
    spinlock_t      lock;
    /* these other elements are all protected by lock */
    unsigned long       flags;
    dma_cookie_t        completed;
    struct list_head    active_list;
    struct list_head    queue;
    struct list_head    free_list;
    struct fh_cyclic_desc   *cdesc;
    unsigned int        descs_allocated;
};

struct fh_acw_dma_transfer
{
    struct dma_chan *chan;
    struct fh_dma_slave cfg;
    struct scatterlist sgl;
    struct dma_async_tx_descriptor *desc;
};

struct channel_assign
{
	int capture_channel;
	int playback_channel;
};

struct audio_dev
{
	struct channel_assign channel_assign;
    struct fh_audio_cfg audio_config;
    struct miscdevice fh_audio_miscdev;
};

static const struct file_operations acw_fops;

static struct audio_dev fh_audio_dev =
{
    .channel_assign = {
        .capture_channel = ACW_DMA_CAP_CHANNEL,
        .playback_channel = ACW_DMA_PAY_CHANNEL,
    },
    .fh_audio_miscdev = {
        .fops       = &acw_fops,
        .name       = "fh_audio",
        .minor      = MISC_DYNAMIC_MINOR,
    }

};

static struct
{
    spinlock_t      lock;
    void __iomem        *regs;
    struct clk      *clk;
    unsigned long       in_use;
    unsigned long       next_heartbeat;
    struct timer_list   timer;
    int         expect_close;
    int         irq;
} fh_audio_module;
//#define STERO
static struct fh_dma_chan *dma_rx_transfer = NULL;
static struct fh_dma_chan *dma_tx_transfer = NULL;
#ifdef 	STERO
static struct fh_dma_chan *dma_tx_right_transfer = NULL;
#endif
static struct work_struct playback_wq;

static struct audio_param_store
{
    int input_volume;
    enum io_select input_io_type;
} audio_param_store;

#ifdef 	STERO
static void fh_acw_tx_right_dma_done(void *arg);
#define TX_RIGHT_CHANNEL_DMA_CHANNEL 0
#endif
static void fh_acw_tx_dma_done(void *arg);
static void fh_acw_rx_dma_done(void *arg);
static bool  fh_acw_dma_chan_filter(struct dma_chan *chan, void *filter_param);
static void create_proc(void);
static void remove_proc(void);

void fh_acw_stop_playback(struct fh_audio_cfg *audio_config)
{
	unsigned int status;
    if(audio_config->playback.state == STATE_STOP)
    {
        return;
    }
    audio_config->playback.state = STATE_STOP;
    status = readl( fh_audio_module.regs + 0x8);
    status &=(~(1<<0));
    writel(status, fh_audio_module.regs + 0x8);//tx fifo disable
    fh_dma_cyclic_stop(dma_tx_transfer->chan);
    fh_dma_cyclic_free(dma_tx_transfer->chan);
#ifdef 	STERO
    fh_dma_cyclic_stop(dma_tx_right_transfer->chan);
    fh_dma_cyclic_free(dma_tx_right_transfer->chan);
#endif
    up(&audio_config->sem_playback);
}

void fh_acw_stop_capture(struct fh_audio_cfg *audio_config)
{

    u32 rx_status,status;

    if(audio_config->capture.state == STATE_STOP)
    {
        return;
    }
//    rx_status = readl( fh_audio_module.regs + ACW_RXFIFO_CTRL);//clear rx fifo
//    rx_status =  rx_status|(1<<4);
//    writel(rx_status,fh_audio_module.regs + ACW_RXFIFO_CTRL);

    audio_config->capture.state = STATE_STOP;

    status = readl( fh_audio_module.regs + 0x4);
    status &=(~(1<<0));
    writel(status, fh_audio_module.regs + 0x4);//tx fifo disable

    fh_dma_cyclic_stop(dma_rx_transfer->chan);
    fh_dma_cyclic_free(dma_rx_transfer->chan);
    up(&audio_config->sem_capture);
}

void switch_io_type(enum audio_type type, enum io_select io_type)
{
#if 0
#ifndef CONFIG_MACH_FH8830_FPGA
	int reg;
    if (capture == type)
    {
        reg = readl(fh_audio_module.regs + ACW_ADC_PATH_CTRL);
        if (mic_in == io_type)
        {
            printk(KERN_INFO"audio input changed to mic_in\n");
            writel( reg & (~(1<<1)),fh_audio_module.regs + ACW_ADC_PATH_CTRL);
        }
        else if (line_in == io_type)
        {
            printk(KERN_INFO"audio input changed to line_in\n");
            writel(reg | (1<<1), fh_audio_module.regs + ACW_ADC_PATH_CTRL);
        }
    }
    else
    {
        reg = readl(fh_audio_module.regs + ACW_DAC_PATH_CTRL);
        if (speaker_out == io_type)
        {
            printk(KERN_INFO"audio output changed to speaker_out\n");
            reg = reg & (~(3<<21));
            writel(reg, fh_audio_module.regs + ACW_DAC_PATH_CTRL);
            reg = reg | (1<<21);
            writel(reg,fh_audio_module.regs + ACW_DAC_PATH_CTRL);
            reg = reg | (1<<18);
            writel(reg, fh_audio_module.regs + ACW_DAC_PATH_CTRL);/*unmute speaker*/
            reg = reg | (3<<30);
            writel(reg,fh_audio_module.regs + ACW_DAC_PATH_CTRL);/*mute line out*/
        }
        else if (line_out == io_type)
        {
            printk(KERN_INFO"audio output changed to line_out\n");
            reg = reg & (~(3<<21));
            writel(reg, fh_audio_module.regs + ACW_DAC_PATH_CTRL);/*mute speaker*/
            reg = reg & (~(3<<30));
            writel(reg, fh_audio_module.regs + ACW_DAC_PATH_CTRL);/*unmute line out*/
        }
    }
#endif
#endif
}

int get_factor_from_table(int rate)
{
    int factor;
    switch(rate)
    {
        case 8000:
            factor = 4;
            break;
        case 16000:
            factor = 1;
            break;
        case 32000:
            factor = 0;
            break;
        case 44100:
            factor = 13;
            break;
        case 48000:
            factor = 6;
            break;
        default:
            factor = -EFAULT;
            break;
    }
    return factor;
}

void switch_rate(enum audio_type type, int rate)
{
#if 0 
#ifndef CONFIG_MACH_FH8830_FPGA
	int reg, factor;
    factor = get_factor_from_table(rate);
    if (factor < 0)
    {
        printk(KERN_ERR "unsupported sample rate\n");
        return;
    }
    reg = readl(fh_audio_module.regs + ACW_DIG_IF_CTRL);
    if (capture == type)
    {
        printk(KERN_INFO"capture rate set to %d\n", rate);
        reg = reg & (~(0xf<<0));
        writel(reg, fh_audio_module.regs + ACW_DIG_IF_CTRL);/*adc and dac sample rate*/
        reg = reg | (factor<<0);
        writel(reg,fh_audio_module.regs + ACW_DIG_IF_CTRL);
    }
    else
    {
        printk(KERN_INFO"playback rate set to %d\n", rate);
        reg = reg & (~(0xf<<8));
        writel(reg, fh_audio_module.regs + ACW_DIG_IF_CTRL);/*adc and dac sample rate*/
        reg = reg | (factor<<8);
        writel(reg, fh_audio_module.regs + ACW_DIG_IF_CTRL);
    }
#endif
#endif
}

int get_param_from_volume(int volume)
{
    int param, max_param, min_param, max_volume;
	max_volume = 100;
	if (volume < 0 || volume > max_volume)
    {
        printk(KERN_ERR "unsupported input volume\n");
        return -EINVAL;
    }
	max_param = 63;
    min_param = 0;
	param = max_param - (max_volume - volume);
	if (param <= min_param)
    {
        param = min_param;
    }
    return param;
}

void switch_input_volume(int volume)
{
#if 0
#ifndef CONFIG_MACH_FH8830_FPGA
    int reg, param;
    param = get_param_from_volume(volume);
    if (param < 0)
    {
        return;
    }
    printk(KERN_INFO"capture volume set to %d\n", volume);
	reg = readl(fh_audio_module.regs + ACW_ADC_PATH_CTRL);
    reg = reg & (~(0x3f<<8));
    writel(reg, fh_audio_module.regs + ACW_ADC_PATH_CTRL);
    reg = reg | (param<<8);
    writel(reg,fh_audio_module.regs + ACW_ADC_PATH_CTRL);
#endif
#endif
}

void init_audio(enum audio_type type,struct fh_audio_cfg  *audio_config)
{
	writel(0x1,fh_audio_module.regs +0x0);
#ifndef CONFIG_MACH_FH8830_FPGA
    int reg;
//    reg = readl(fh_audio_module.regs + ACW_CTRL);
//    if ((reg & 0x80000000) == 0)
//    {
//        writel(0x80000000, fh_audio_module.regs + ACW_CTRL);/*enable audio*/
//    }
//    reg = readl(fh_audio_module.regs + ACW_MISC_CTRL);
//    if (0x40400 != reg)
//    {
//        writel(0x40400,fh_audio_module.regs + ACW_MISC_CTRL);/*misc ctl*/
//    }
//    if (capture == type)
//    {
//        writel(0x61141b06,fh_audio_module.regs + ACW_ADC_PATH_CTRL);/*adc cfg*/
//        writel(0x167f2307, fh_audio_module.regs + ACW_ADC_ALC_CTRL);/*adc alc*/
//        writel(0, fh_audio_module.regs + ACW_RXFIFO_CTRL);/*rx fifo disable*/
//        switch_input_volume(audio_config->capture.cfg.volume);
//        switch_rate(capture, audio_config->capture.cfg.rate);
//        switch_io_type(capture, audio_config->capture.cfg.io_type);
//    }
//    else
//    {
//        writel(0x3b403f09, fh_audio_module.regs + ACW_DAC_PATH_CTRL);/*dac cfg*/
//        writel(0, fh_audio_module.regs + ACW_TXFIFO_CTRL);/*tx fifo disable*/
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

int avail_data_len(enum audio_type type,struct fh_audio_cfg *stream)
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

	pid= current->tgid;
//#ifndef CONFIG_MACH_FH8830_FPGA
//	//disable interrupts
//	u32 reg;
//    reg = readl(fh_audio_module.regs + ACW_CTRL);
//    reg &= ~(0x3ff);
//    writel(reg, fh_audio_module.regs + ACW_CTRL);
//#endif
	if( infor_record.play_pid == pid)
	{
		fh_acw_stop_playback(audio_config);

	}
	if (infor_record.record_pid==pid)
	{
		fh_acw_stop_capture(audio_config);
	}
	return 0;
}

int register_tx_dma(struct fh_audio_cfg  *audio_config)
{
	int ret;
	struct fh_dma_slave *tx_config;

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
		kfree(tx_config);
		return -ENOMEM;
	}
	tx_config_right->slave_id = ACW_HW_NUM_TX;
	tx_config_right->src_maxburst = FH_DMA_MSIZE_8;
	tx_config_right->dst_maxburst = FH_DMA_MSIZE_8;
	tx_config_right->dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	tx_config_right->device_fc = TRUE;
	tx_config_right->dst_addr = ACW_DACR_FIFO;

	dma_tx_right_transfer->cdesc =fh_dma_cyclic_prep(dma_tx_right_transfer->chan,audio_config->playback.addr,
				audio_config->playback.cfg.buffer_bytes,audio_config->playback.cfg.period_bytes, DMA_TO_DEVICE);

	if(dma_tx_transfer->cdesc <= 0)
	{
	    printk(KERN_ERR"cyclic desc err\n");
	    ret = -ENOMEM;
	    kfree(tx_config_right);
	    goto fail;
	}
	dma_tx_transfer->cdesc->period_callback = fh_acw_tx_right_dma_done;
	dma_tx_transfer->cdesc->period_callback_param = audio_config;
	fh_dma_cyclic_start(dma_tx_right_transfer->chan);

	kfree(tx_config_right);

	/*must set NULL to tell DMA driver that we free the DMA slave*/
	dma_tx_right_transfer->chan->private = NULL;
#endif


	tx_config->cfg_hi = FHC_CFGH_DST_PER(11);
	tx_config->dst_msize = FH_DMA_MSIZE_8;
	tx_config->src_msize = FH_DMA_MSIZE_8;
	tx_config->reg_width = FH_DMA_SLAVE_WIDTH_32BIT;
	tx_config->fc = FH_DMA_FC_D_M2P;
	tx_config->tx_reg = 0xf09001c8;

	dma_tx_transfer->chan->private =  tx_config;
	if ((audio_config->playback.cfg.buffer_bytes < audio_config->playback.cfg.period_bytes) ||
	(audio_config->playback.cfg.buffer_bytes <= 0) || (audio_config->playback.cfg.period_bytes <= 0) ||
	(audio_config->playback.cfg.buffer_bytes/audio_config->playback.cfg.period_bytes > NR_DESCS_PER_CHANNEL))
	{
		printk(KERN_ERR "buffer_size and period_size are invalid\n");
		ret = -EINVAL;
		goto fail;
	}

	dma_tx_transfer->cdesc =fh_dma_cyclic_prep(dma_tx_transfer->chan,audio_config->playback.addr,
			       audio_config->playback.cfg.buffer_bytes,audio_config->playback.cfg.period_bytes, DMA_MEM_TO_DEV);
	if(dma_tx_transfer->cdesc <= 0)
	{
		printk(KERN_ERR "cyclic desc err\n");
		ret = -ENOMEM;
		goto fail;
	}
	dma_tx_transfer->cdesc->period_callback = fh_acw_tx_dma_done;
	dma_tx_transfer->cdesc->period_callback_param = audio_config;
	fh_dma_cyclic_start(dma_tx_transfer->chan);

	kfree(tx_config);

	/*must set NULL to tell DMA driver that we free the DMA slave*/
	dma_tx_transfer->chan->private = NULL;
	return 0;
fail:
	kfree(tx_config);
	return ret;
}

int register_rx_dma( struct fh_audio_cfg  *audio_config)
{
    int ret,status;

    struct fh_dma_slave *rx_config;
    rx_config =  kzalloc(sizeof(struct fh_dma_slave), GFP_KERNEL);
    if (!rx_config)
    {
        return -ENOMEM;
    }

    rx_config->cfg_hi = FHC_CFGH_SRC_PER(10);
    rx_config->dst_msize = FH_DMA_MSIZE_8;
    rx_config->src_msize = FH_DMA_MSIZE_8;
    rx_config->reg_width = FH_DMA_SLAVE_WIDTH_32BIT;
    rx_config->fc = FH_DMA_FC_D_P2M;
    rx_config->rx_reg = 0xf09001c0;
    dma_rx_transfer->chan->private =  rx_config;
    if ((audio_config->capture.cfg.buffer_bytes < audio_config->capture.cfg.period_bytes) ||
        (audio_config->capture.cfg.buffer_bytes <= 0) ||(audio_config->capture.cfg.period_bytes <= 0) ||
        (audio_config->capture.cfg.buffer_bytes/audio_config->capture.cfg.period_bytes > NR_DESCS_PER_CHANNEL))
    {
        printk(KERN_ERR "buffer_size and period_size are invalid\n");
        ret = -EINVAL;
        goto fail;
    }
    dma_rx_transfer->cdesc = fh_dma_cyclic_prep(dma_rx_transfer->chan,audio_config->capture.addr,
                               audio_config->capture.cfg.buffer_bytes, audio_config->capture.cfg.period_bytes,DMA_DEV_TO_MEM);
    if(dma_rx_transfer->cdesc <= 0)
    {
        printk(KERN_ERR" cyclic desc err\n");
        ret = -ENOMEM;
        goto fail;
    }
    dma_rx_transfer->cdesc->period_callback = fh_acw_rx_dma_done;
    dma_rx_transfer->cdesc->period_callback_param = audio_config;
    fh_dma_cyclic_start(dma_rx_transfer->chan);

    writel(0x1,fh_audio_module.regs + 0x4);/*enable rx fifo*/
    writel(0x1,fh_audio_module.regs + 0x1c4);/*reset dma*/
    writel(0x1,fh_audio_module.regs + 0x14);/*reset dma*/
    writel(0x1,fh_audio_module.regs + 0x28);/*reset dma*/

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


void playback_start_wq_handler(struct work_struct *work)
{
    int avail;
    unsigned int status;
    while(1)
    {
        if (STATE_STOP == fh_audio_dev.audio_config.playback.state)
        {
            return;
        }
        avail = avail_data_len(playback, &fh_audio_dev.audio_config);
        if (avail > fh_audio_dev.audio_config.playback.cfg.period_bytes)
        {
            msleep(0);
        }
        else
        {
        	   writel(0x1,fh_audio_module.regs + 0x8);/*enable rx fifo*/
        	    writel(0x1,fh_audio_module.regs + 0x1cc);/*reset dma*/
        	    writel(0x1,fh_audio_module.regs + 0x18);/*reset dma*/
        	    writel(0x1,fh_audio_module.regs + 0x2c);/*reset dma*/

            break;
        }
    }
}

int fh_acw_start_playback(struct fh_audio_cfg *audio_config)
{
    int ret;

    if(audio_config->playback.state == STATE_RUN)
    {
        return 0;
    }

    if (audio_config->playback.cfg.buffer_bytes >= AUDIO_DMA_PREALLOC_SIZE)
    {
        printk("DMA prealloc buffer is smaller than  audio_config->buffer_bytes\n");
        return -ENOMEM;
    }
    memset(audio_config->playback.area, 0, audio_config->playback.cfg.buffer_bytes);
    audio_config->playback.size = audio_config->playback.cfg.buffer_bytes;
    audio_config->playback.state = STATE_RUN;
    ret = register_tx_dma(audio_config);
    if (ret < 0)
    {
        return ret;
    }
    INIT_WORK(&playback_wq, playback_start_wq_handler);
    schedule_work(&playback_wq);
    return 0;
}

int fh_acw_start_capture(struct fh_audio_cfg *audio_config)
{
    if(audio_config->capture.state == STATE_RUN)
    {
        return 0;
    }

    if (audio_config->capture.cfg.buffer_bytes >= AUDIO_DMA_PREALLOC_SIZE)
    {
        printk("DMA prealloc buffer is smaller than  audio_config->buffer_bytes\n");
        return -ENOMEM;
    }
    memset(audio_config->capture.area, 0, audio_config->capture.cfg.buffer_bytes);
    audio_config->capture.size = audio_config->capture.cfg.buffer_bytes;

    audio_config->capture.state = STATE_RUN;
    return register_rx_dma(audio_config);
}


static void fh_acw_rx_dma_done(void *arg)
{
    struct fh_audio_cfg *audio_config;
    audio_config = ( struct fh_audio_cfg *)arg;
printk("%s",__func__);
    spin_lock(&audio_config->capture.lock);
    audio_config->capture.hw_ptr += audio_config->capture.cfg.period_bytes;
    if (audio_config->capture.hw_ptr > audio_config->capture.size )
    {
        audio_config->capture.hw_ptr = audio_config->capture.hw_ptr - audio_config->capture.size;
    }
    spin_unlock(&audio_config->capture.lock);
    if (waitqueue_active(&audio_config->readqueue))
    {
        int avail = avail_data_len(capture,audio_config);
        if (avail > audio_config->capture.cfg.period_bytes)
        {
            wake_up_interruptible(&audio_config->readqueue);
        }
    }

}
#ifdef 	STERO
static void fh_acw_tx_right_dma_done(void *arg)
{
	printk("tx_right_dam_done\n");

}
#endif

static void fh_acw_tx_dma_done(void *arg)
{
	printk("%s",__func__);
    struct fh_audio_cfg *audio_config;
    audio_config = ( struct fh_audio_cfg *)arg;
    spin_lock(&audio_config->playback.lock);
    audio_config->playback.hw_ptr +=  audio_config->playback.cfg.period_bytes;
    if (audio_config->playback.hw_ptr > audio_config->playback.size )
    {
        audio_config->playback.hw_ptr = audio_config->playback.hw_ptr - audio_config->playback.size;
    }
    spin_unlock(&audio_config->playback.lock);
    if (waitqueue_active(&audio_config->writequeue))
    {
        int avail = avail_data_len(playback,audio_config);
        if (avail > audio_config->playback.cfg.period_bytes)
        {
            wake_up_interruptible(&audio_config->writequeue);
        }
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

int arg_config_support(struct fh_audio_cfg_arg * cfg)
{
	int ret;

	ret = get_param_from_volume(cfg->volume);
	if (ret < 0) {
		return -EINVAL;
	}
	ret = get_factor_from_table(cfg->rate);
	if (ret < 0) {
		return -EINVAL;
	}
    return 0;
}

void reset_dma_buff(enum audio_type type, struct fh_audio_cfg *audio_config)
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
    int ret;
    int value,pid;
    int __user *p = (int __user *)arg;
    int rx_status;

#ifndef CONFIG_MACH_FH8830_FPGA
    int tx_status;
#endif
    pid = current->tgid;
    switch (cmd)
    {
        case AC_INIT_CAPTURE_MEM:

            if (copy_from_user((void *)&cfg, (void __user *)arg, sizeof(struct fh_audio_cfg_arg)))
            {
                printk(KERN_ERR "copy err\n");
                return -EIO;
            }
            if (0 == arg_config_support(&cfg))
            {
            	if (down_trylock(&audio_config->sem_capture))
	            {
	            	printk(KERN_ERR "another thread is running capture.\n");
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
                audio_param_store.input_io_type = audio_config->capture.cfg.io_type;
                audio_param_store.input_volume = audio_config->capture.cfg.volume;
                /* *  config sample  *  */
                codec_i2c_adapter = i2c_get_adapter(0);
                i2c_write_codec(0x20, 0x00);//config pll to 8k rate
            	i2c_write_codec(0x22, 0x14);
            	i2c_write_codec(0x23, 0x55);
                i2c_write_codec(0x24, 0x0);
                msleep(1000);
                i2c_write_codec(0x20, 0x01);

                i2c_write_codec(0x10, 0x11);
                i2c_write_codec(0x17, 0xf0);
                i2c_write_codec(0x0, 0x01);
                i2c_write_codec(0x1, 0x5);
                i2c_write_codec(0x1f, 0x1);
                i2c_write_codec(0x2, 0x10);
                /*  * config wrapper work format  *   */
        	    writel(0x08,fh_audio_module.regs + 0x10);/*reset dma*/
        	    writel(0x3,fh_audio_module.regs + 0x180);/*reset dma*/
        	    writel(0x1f,fh_audio_module.regs + 0x188);/*reset dma*/
        	    writel(0x1f,fh_audio_module.regs + 0x184);/*reset dma*/
        	    writel(0x1,fh_audio_module.regs + 0x8);/*reset dma*/
        	    writel(0x1,fh_audio_module.regs + 0xc);/*reset dma*/
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
                printk(KERN_ERR "copy err\n");
                return -EIO;
            }
            
            if (0 == arg_config_support(&cfg))
            {
            	if (down_trylock(&audio_config->sem_playback))
	            {
	            	printk(KERN_ERR "another thread is running playback.\n");
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
                codec_i2c_adapter = i2c_get_adapter(0);
                if(!config_flag){
                	/* * config sample  *  */
                	i2c_write_codec(0x20, 0x0);//config pll
                  	i2c_write_codec(0x22, 0x14);
                    i2c_write_codec(0x23, 0x55);
                    i2c_write_codec(0x24, 0x0);
					msleep(1000);
					i2c_write_codec(0x20, 0x1);

					i2c_write_codec(0x10, 0x11);
					i2c_write_codec(0x17, 0xf0);
					i2c_write_codec(0x0, 0x01);
					i2c_write_codec(0x1, 0x5);
					i2c_write_codec(0x1f, 0x1);

					i2c_write_codec(0x2, 0x10);
					/*  * config wrapper work format  *   */

	        	    writel(0x08,fh_audio_module.regs + 0x10);/*reset dma*/
	         	    writel(0x3,fh_audio_module.regs + 0x180);/*reset dma*/
	        	    writel(0x1f,fh_audio_module.regs + 0x188);/*reset dma*/
	        	    writel(0x1f,fh_audio_module.regs + 0x184);/*reset dma*/
	        	    writel(0x1,fh_audio_module.regs + 0x8);/*reset dma*/
	        	    writel(0x1,fh_audio_module.regs + 0xc);/*reset dma*/
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
			if (infor_record.record_pid != pid) {
				return -EBUSY;
			}
            if (get_user(value, p))
            {
                return -EFAULT;
            }
			ret = get_param_from_volume(value);
			if (ret < 0) {
				return -EINVAL;
			}
            audio_param_store.input_volume = value;
            switch_input_volume(audio_param_store.input_volume);
            break;
        case AC_SET_INPUT_MODE:
			if (infor_record.record_pid != pid) {
				return -EBUSY;
			}
            if (get_user(value, p))
            {
                return -EFAULT;
            }
			if (value != mic_in && value != line_in) {
				return -EINVAL;
			}
            audio_param_store.input_io_type = value;
            switch_io_type(capture, audio_param_store.input_io_type);
            break;
        case AC_SET_OUTPUT_MODE:
			if (infor_record.play_pid != pid) {
				return -EBUSY;
			}
            if (get_user(value, p))
            {
                return -EFAULT;
            }
			if (value != speaker_out && value != line_out) {
				return -EINVAL;
			}
            switch_io_type(playback, value);
            break;
        case AC_AI_DISABLE:
            printk("[ac_driver]AC_AI_DISABLE\n");
			if (infor_record.record_pid != pid) {
				return -EBUSY;
			}
			fh_acw_stop_capture(audio_config);
            printk(" AC_AI_DISABLE\n");
            break;
        case AC_AO_DISABLE:
            printk("[ac_driver]AC_AO_DISABLE\n");
			if (infor_record.play_pid != pid) {
				return -EBUSY;
			}
            fh_acw_stop_playback(audio_config);
            printk(" AC_AO_DISABLE\n");
            break;
        case AC_AI_PAUSE:
			if (infor_record.record_pid != pid) {
				return -EBUSY;
			}
            printk(KERN_INFO "capture pause\n");
            audio_config->capture.state = STATE_PAUSE;
            rx_status = readl(fh_audio_module.regs + ACW_RXFIFO_CTRL);/*rx fifo disable*/
            rx_status =  rx_status&(~(1<<0));
            writel(rx_status, fh_audio_module.regs + ACW_RXFIFO_CTRL);/*rx fifo disable*/
            break;
        case AC_AI_RESUME:
			if (infor_record.record_pid != pid) {
				return -EBUSY;
			}
            printk(KERN_INFO "capture resume\n");
            audio_config->capture.state = STATE_RUN;
#ifndef CONFIG_MACH_FH8830_FPGA
            rx_status = readl( fh_audio_module.regs + ACW_RXFIFO_CTRL);//clear rx fifo
            rx_status =  rx_status|(1<<4);
            writel(rx_status,fh_audio_module.regs + ACW_RXFIFO_CTRL);/*enable rx fifo*/
            rx_status =  rx_status&(~(1<<4));
            rx_status =  rx_status|(1<<0);
            writel(rx_status,fh_audio_module.regs + ACW_RXFIFO_CTRL);/*enable rx fifo*/
#endif
            break;
        case AC_AO_PAUSE:
			if (infor_record.play_pid != pid) {
				return -EBUSY;
			}
            audio_config->playback.state = STATE_PAUSE;
            printk(KERN_INFO "playback pause\n");
//#ifndef CONFIG_MACH_FH8830_FPGA
//            tx_status = readl(fh_audio_module.regs + ACW_TXFIFO_CTRL);/*rx fifo disable*/
//            tx_status =  tx_status&(~(1<<0));
//            writel(tx_status, fh_audio_module.regs + ACW_TXFIFO_CTRL);/*tx fifo disable*/
//#endif
            break;
        case AC_AO_RESUME:
			if (infor_record.play_pid != pid) {
				return -EBUSY;
			}
            printk(KERN_INFO "playback resume\n");
            audio_config->playback.state = STATE_RUN;
//#ifndef CONFIG_MACH_FH8830_FPGA
//            tx_status = readl( fh_audio_module.regs + ACW_TXFIFO_CTRL);//clear rx fifo
//            tx_status =  tx_status|(1<<0);
//            writel(tx_status,fh_audio_module.regs + ACW_TXFIFO_CTRL); //enable tx fifo read enable
//#endif
            break;
        default:
            return -ENOTTY;
    }
    return 0;
}

static int fh_audio_open(struct inode *ip, struct file *fp)
{

    fp->private_data = &fh_audio_dev.fh_audio_miscdev;
//    //enable interrupts

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
#ifndef CONFIG_MACH_FH8830_FPGA
	u32 interrupts, rx_status;
    struct fh_audio_cfg  *audio_config = &fh_audio_dev.audio_config;

    interrupts = readl(fh_audio_module.regs + ACW_CTRL);
    //interrupts &= ~(0x3ff) << 16;
    writel(interrupts, fh_audio_module.regs + ACW_CTRL);

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
            rx_status = readl( fh_audio_module.regs + ACW_RXFIFO_CTRL);//clear rx fifo
            rx_status =  rx_status|(1<<4);
            writel(rx_status,fh_audio_module.regs + ACW_RXFIFO_CTRL);
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
#endif
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

static int __devinit fh_audio_drv_probe(struct platform_device *pdev)
{
    int ret;
    struct resource *irq_res, *mem;

    mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!mem)
        return -EINVAL;
    printk("acw probe\n");
    if (!devm_request_mem_region(&pdev->dev, mem->start, resource_size(mem),
                                 "fh_audio_module"))
        return -ENOMEM;

    fh_audio_module.regs = devm_ioremap(&pdev->dev, DW_I2S_FH8833, resource_size(mem));

    if (!fh_audio_module.regs){
    	ret = -ENOMEM;
    	goto remap_fail;
        }

    fh_audio_module.clk = clk_get(NULL, "ac_clk");
	if (!fh_audio_module.clk) {
		ret = -EINVAL;
		goto clk_fail;
		}
    clk_enable(fh_audio_module.clk);

    spin_lock_init(&fh_audio_module.lock);

    ret = misc_register(&fh_audio_dev.fh_audio_miscdev);

    if (ret)
        goto out_disable_clk;

    irq_res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
    if (!irq_res)
    {
        pr_err("%s: ERROR: getting resource failed"
               "cannot get IORESOURCE_IRQ\n", __func__);
        ret = -ENXIO;
        goto out_disable_clk;
    }

    fh_audio_module.irq = irq_res->start;

    ret = request_irq(fh_audio_module.irq, fh_audio_interrupt, IRQF_SHARED, "audio", &fh_audio_module);

    return 0;

out_disable_clk:
    clk_disable(fh_audio_module.clk);
	fh_audio_module.clk = NULL;
clk_fail:
	devm_iounmap(&pdev->dev, fh_audio_module.regs);
	fh_audio_module.regs = NULL;
remap_fail:
	devm_release_mem_region(&pdev->dev, mem->start, resource_size(mem));
    return ret;
}

static int __devexit fh_acw_drv_remove(struct platform_device *pdev)
{
	struct resource *mem;
    misc_deregister(&fh_audio_dev.fh_audio_miscdev);

    free_irq(fh_audio_module.irq, &fh_audio_module);

	if (fh_audio_module.clk) {
		clk_disable(fh_audio_module.clk);
    	clk_put(fh_audio_module.clk);
	}
	if (fh_audio_module.regs) {
		devm_iounmap(&pdev->dev, fh_audio_module.regs);
	}
	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (mem) {
		devm_release_mem_region(&pdev->dev, mem->start, resource_size(mem));
    }
    printk("acw remove ok\n");
    return 0;
}

static struct platform_driver fh_audio_driver =
{
    .probe      = fh_audio_drv_probe,
    .remove     = __devexit_p(fh_acw_drv_remove),
    .driver     = {
        .name   = "fh_acw",
        .owner  = THIS_MODULE,
    }
};

void audio_prealloc_dma_buffer(struct fh_audio_cfg  *audio_config)
{
    int pg;
    gfp_t gfp_flags;
    pg = get_order(AUDIO_DMA_PREALLOC_SIZE);
    gfp_flags = GFP_KERNEL | __GFP_COMP | __GFP_NORETRY | __GFP_NOWARN;
    audio_config->capture.dev.coherent_dma_mask = DMA_BIT_MASK(32);
    audio_config->capture.area  = dma_alloc_coherent(&audio_config->capture.dev, PAGE_SIZE << pg, &audio_config->capture.addr, gfp_flags );
    if (!audio_config->capture.area)
    {
        printk(KERN_ERR"no enough mem for capture  buffer alloc\n");
        return ;
    }
    audio_config->playback.dev.coherent_dma_mask = DMA_BIT_MASK(32);
    audio_config->playback.area  = dma_alloc_coherent(&audio_config->playback.dev, PAGE_SIZE << pg, &audio_config->playback.addr, gfp_flags );
    if (!audio_config->playback.area)
    {
        printk(KERN_ERR"no enough mem for  playback buffer alloc\n");
        return ;
    }
}

void audio_free_prealloc_dma_buffer(struct fh_audio_cfg  *audio_config)
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

int audio_request_dma_channel(void)
{
    dma_cap_mask_t mask;
    /*request audio rx dma channel*/
    dma_rx_transfer = kzalloc(sizeof(struct fh_dma_chan), GFP_KERNEL);
    if (!dma_rx_transfer)
    {
        printk(KERN_ERR"alloc  dma_rx_transfer failed\n");
        goto mem_fail;
    }
    memset(dma_rx_transfer, 0, sizeof(struct fh_dma_chan));

    dma_cap_zero(mask);
    dma_cap_set(DMA_SLAVE, mask);
    dma_rx_transfer->chan = dma_request_channel(mask, fh_acw_dma_chan_filter, &fh_audio_dev.channel_assign.capture_channel);
    if (!dma_rx_transfer->chan)
    {
        printk(KERN_ERR"request audio rx dma channel failed \n");
        goto channel_fail;
    }

    /*request audio tx dma channel*/
    dma_tx_transfer = kzalloc(sizeof(struct fh_dma_chan), GFP_KERNEL);
    if (!dma_tx_transfer)
    {
        printk(KERN_ERR"alloc  dma_tx_transfer failed\n");
        goto mem_fail;
    }
    memset(dma_tx_transfer, 0, sizeof(struct fh_dma_chan));

    dma_cap_zero(mask);
    dma_cap_set(DMA_SLAVE, mask);
    dma_tx_transfer->chan = dma_request_channel(mask, fh_acw_dma_chan_filter, &fh_audio_dev.channel_assign.playback_channel);
    if (!dma_tx_transfer->chan)
    {
        printk(KERN_ERR"request dma channel failed \n");
        return -EFAULT;
    }
#ifdef STERO
    /*request audio tx dma channel*/
    dma_tx_right_transfer = kzalloc(sizeof(struct fh_dma_chan), GFP_KERNEL);
    if (!dma_tx_right_transfer)
    {
        printk(KERN_ERR"alloc  dma_tx_right_transfer failed\n");
        goto mem_fail;
    }
    memset(dma_tx_right_transfer, 0, sizeof(struct fh_dma_chan));

    dma_cap_zero(mask);
    dma_cap_set(DMA_SLAVE, mask);
    dma_tx_right_transfer->chan = dma_request_channel(mask, TX_RIGHT_CHANNEL_DMA_CHANNEL, &fh_audio_dev.channel_assign.playback_channel);
    if (!dma_tx_right_transfer->chan)
    {
        printk(KERN_ERR"request dma channel dma_tx_right_transfer failed \n");
        return -EFAULT;
    }
#endif
    return 0;
channel_fail:
    if (!dma_rx_transfer->chan)
    {
        dma_release_channel(dma_rx_transfer->chan);
		dma_rx_transfer->chan = NULL;
    }
    if (!dma_tx_transfer->chan)
    {
        dma_release_channel(dma_tx_transfer->chan);
		dma_tx_transfer->chan = NULL;
    }
#ifdef STERO
    if (!dma_tx_right_transfer->chan)
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
#ifdef 	STERO
    if (dma_tx_right_transfer != NULL)
    {
        kfree(dma_tx_right_transfer);
        dma_tx_right_transfer = NULL;
    }
#endif
    return -EFAULT;
}

void audio_release_dma_channel(void)
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
    create_proc();
    init_audio_mutex(&fh_audio_dev.audio_config);
    audio_prealloc_dma_buffer(&fh_audio_dev.audio_config);
    audio_request_dma_channel();
    return platform_driver_register(&fh_audio_driver);
}
module_init(fh_audio_init);

static void __exit fh_audio_exit(void)
{

    remove_proc();
    audio_release_dma_channel();
    audio_free_prealloc_dma_buffer(&fh_audio_dev.audio_config);
    platform_driver_unregister(&fh_audio_driver);
}
module_exit(fh_audio_exit);

MODULE_AUTHOR("FH_AUDIO");
MODULE_DESCRIPTION("FH_AUDIO");
MODULE_LICENSE("GPL");

/****************************debug proc*****************************/
#include <linux/proc_fs.h>

struct proc_dir_entry *proc_ac_entry;
#define proc_name "fh_audio"

ssize_t proc_ac_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    ssize_t len = 0;
	int i;

	for (i = 0;i <= 0x20;i += 4) {
		printk(KERN_INFO"0x%x reg = 0x%x\n",i, readl(fh_audio_module.regs + i));
	}
    return len;
}

static void create_proc(void)
{
    proc_ac_entry = create_proc_entry(proc_name, S_IRUGO, NULL);
    if (!proc_ac_entry)
    {
        printk(KERN_ERR"create proc failed\n");
    }
    else
    {
        proc_ac_entry->read_proc = proc_ac_read;
    }
}

static void remove_proc(void)
{
    remove_proc_entry(proc_name, NULL);
}

