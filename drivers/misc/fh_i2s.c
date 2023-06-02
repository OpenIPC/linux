/**@file
 * @Copyright (c) 2016 Shanghai Fullhan Microelectronics Co., Ltd.
 * @brief
 *
 * @author      fullhan
 * @date        2016-7-15
 * @version     V1.0
 * @version     V1.1  modify code style
 * @note: misc i2s driver for fh8830 embedded i2s codec.
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

#include "fh_i2s.h"


#define NR_DESCS_PER_CHANNEL 64

#define FIX_SAMPLE_BIT       32

#define I2S_HW_NUM_RX  0
#define I2S_HW_NUM_TX  1
#define I2S_DMA_CAP_CHANNEL 2
#define I2S_DMA_PAY_CHANNEL 3

#define I2S_CTRL 						0x0
#define I2S_TXFIFO_CTRL 				0x4
#define I2S_RXFIFO_CTRL 				0x8
#define I2S_STATUS             			0x0c
#define I2S_DAT_CTL				  		0x10
#define I2S_DBG_CTL					  	0x14
#define I2S_STATUS1					  	0x18
#define I2S_STATUS2						0x1c

#define I2S_DACL_FIFO 					0xf0a00100
#define I2S_DACR_FIFO 					0xf0a00300
#define I2S_ADC_FIFO					0xf0a00200

#define I2S_DMA_PREALLOC_SIZE 128*1024

#define I2S_INTR_RX_UNDERFLOW   0x10000
#define I2S_INTR_RX_OVERFLOW    0x20000
#define I2S_INTR_TX_UNDERFLOW   0x40000
#define I2S_INTR_TX_OVERFLOW    0x80000

#define I2S_EXT_EN             1<<12
#define I2S_EN				   1<<0
#define I2S_DACL_CHEN_EN       1<<30
#define I2S_DACR_CHEN_EN       1<<31
#define I2S_ADC_CHEN_EN        1<<29
#define I2S_SHIFT_BIT          6
#define I2S_DAC_FIFO_CLEAR     1<<4
#define I2S_ADC_FIFO_CLEAR     1<<4
#define I2S_ADC_FIFO_EN		   1<<0
#define I2S_DAC_FIFO_EN        1<<0
#define FH_i2s_DEBUG
#ifdef FH_i2s_DEBUG
#define PRINT_i2s_DBG(fmt, args...)     \
    do                              \
    {                               \
        printk("FH_i2s_DEBUG: ");   \
        printk(fmt, ## args);       \
    }                               \
    while(0)
#else
#define PRINT_i2s_DBG(fmt, args...)  do { } while (0)
#endif

enum i2s_type
{
    capture = 0,
    playback,
};


enum i2s_state
{
    STATE_NORMAL = 0,
    STATE_XRUN,
    STATE_STOP,
    STATE_RUN,
    STATE_PAUSE
};

struct i2s_infor_record_t
{
	int record_pid;
	int play_pid;
}i2s_infor_record;


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

struct i2s_config {
	int rate;
	int volume;
    int frame_bit;
    int channels;
    int buffer_size;
    int period_size;
    int buffer_bytes;
    int period_bytes;
    int start_threshold;
    int stop_threshold;
};

struct i2s_ptr_t
{
	struct i2s_config cfg;
    enum i2s_state state;
    long size;
    int hw_ptr;
    int appl_ptr;
    spinlock_t lock;
    struct device dev;
    u8 *area; /*virtual pointer*/
    dma_addr_t addr; /*physical address*/
    u8 * mmap_addr;
};

struct fh_i2s_cfg
{
    struct i2s_ptr_t capture;
    struct i2s_ptr_t playback;
    wait_queue_head_t readqueue;
    wait_queue_head_t writequeue;
	struct semaphore sem_capture;
	struct semaphore sem_playback;
};

struct fh_i2s_dma_chan
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
    struct dma_async_tx_descriptor   *cdesc;
    unsigned int        descs_allocated;
};

struct fh_I2S_dma_transfer
{
    struct dma_chan *chan;
    struct fh_dma_slave cfg;
    struct scatterlist sgl;
    struct fh_cyclic_desc *desc;
};

struct channel_assign
{
	int capture_channel;
	int playback_channel;
};

struct i2s_dev
{
	struct channel_assign channel_assign;
    struct fh_i2s_cfg i2s_config;
    struct miscdevice fh_i2s_miscdev;
};

static const struct file_operations I2S_fops;

static struct i2s_dev fh_i2s_dev =
{
    .channel_assign = {
        .capture_channel = I2S_DMA_CAP_CHANNEL,
        .playback_channel = I2S_DMA_PAY_CHANNEL,
    },
    .fh_i2s_miscdev = {
        .fops       = &I2S_fops,
        .name       = "fh_fh8830_i2s",
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
    int         irq;
} fh_i2s_module;
static struct fh_dma_chan *dma_rx_transfer = NULL;
static struct fh_dma_chan *dma_tx_transfer = NULL;
static struct work_struct playback_wq;

static struct i2s_param_store
{
    int input_volume;
} i2s_param_store;


static void fh_I2S_tx_dma_done(void *arg);
static void fh_I2S_rx_dma_done(void *arg);
static bool  fh_I2S_dma_chan_filter(struct dma_chan *chan, void *filter_param);

void fh_I2S_stop_playback(struct fh_i2s_cfg *i2s_config)
{

    if(i2s_config->playback.state == STATE_STOP)
    {
        return;
    }
    i2s_config->playback.state = STATE_STOP;
    writel(0, fh_i2s_module.regs + I2S_TXFIFO_CTRL);//tx fifo disable
    fh_dma_cyclic_stop(dma_tx_transfer->chan);
    fh_dma_cyclic_free(dma_tx_transfer->chan);
    up(&i2s_config->sem_playback);
}

void fh_I2S_stop_capture(struct fh_i2s_cfg *i2s_config)
{

    u32 rx_status;
    if(i2s_config->capture.state == STATE_STOP)
    {
        return;
    }
    rx_status = readl( fh_i2s_module.regs + I2S_RXFIFO_CTRL);//clear rx fifo
    rx_status =  rx_status|I2S_ADC_FIFO_CLEAR;
    writel(rx_status,fh_i2s_module.regs + I2S_RXFIFO_CTRL);

    i2s_config->capture.state = STATE_STOP;

    writel(0, fh_i2s_module.regs + I2S_RXFIFO_CTRL);//rx fifo disable

    fh_dma_cyclic_stop(dma_rx_transfer->chan);
    fh_dma_cyclic_free(dma_rx_transfer->chan);
    up(&i2s_config->sem_capture);
}


int fh_i2s_get_factor_from_table(int rate)
{
	return 0;
}

void fh_switch_input_volume(int volume)
{

}

void init_i2s(enum i2s_type type,struct fh_i2s_cfg  *i2s_config)
{

}

static inline long bytes_to_frames(int frame_bit, int bytes)
{
    return bytes * 8 /frame_bit;
}

static inline long  fh_i2s_frames_to_bytes(int frame_bit, int frames)
{
    return frames * frame_bit / 8;
}

int i2s_avail_data_len(enum i2s_type type,struct fh_i2s_cfg *stream)
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

static int fh_i2s_close(struct inode *ip, struct file *fp)
{
	struct miscdevice *miscdev = fp->private_data;
	struct i2s_dev	*dev = container_of(miscdev, struct i2s_dev, fh_i2s_miscdev);
	struct fh_i2s_cfg *i2s_config = &dev->i2s_config;
	int pid;

	pid= current->tgid;

	if( i2s_infor_record.play_pid == pid)
	{
		fh_I2S_stop_playback(i2s_config);

	}
	if (i2s_infor_record.record_pid==pid)
	{
		fh_I2S_stop_capture(i2s_config);
	}
	return 0;
}

int fh_i2s_register_tx_dma(struct fh_i2s_cfg  *i2s_config)
{
    int ret;
    unsigned int reg;
    struct fh_dma_slave *tx_slave;
    tx_slave =  kzalloc(sizeof(struct fh_dma_slave), GFP_KERNEL);
    if (!tx_slave)
    {
        return -ENOMEM;
    }
    tx_slave->cfg_hi = FHC_CFGH_DST_PER(I2S_HW_NUM_TX);
    tx_slave->dst_msize = FH_DMA_MSIZE_8;
    tx_slave->src_msize = FH_DMA_MSIZE_8;
    tx_slave->reg_width = FH_DMA_SLAVE_WIDTH_32BIT;
    tx_slave->fc = FH_DMA_FC_D_M2P;
    tx_slave->tx_reg = I2S_DACL_FIFO;
    dma_tx_transfer->chan->private =  tx_slave;

    if ((i2s_config->playback.cfg.buffer_bytes < i2s_config->playback.cfg.period_bytes) ||
        (i2s_config->playback.cfg.buffer_bytes <= 0) || (i2s_config->playback.cfg.period_bytes <= 0) ||
        (i2s_config->playback.cfg.buffer_bytes/i2s_config->playback.cfg.period_bytes > NR_DESCS_PER_CHANNEL))
    {
        printk(KERN_ERR "buffer_size and period_size are invalid\n");
        ret = -EINVAL;
        goto fail;
    }

    dma_tx_transfer->cdesc =
    	    fh_dma_cyclic_prep(dma_tx_transfer->chan,i2s_config->playback.addr,
    	    		i2s_config->playback.cfg.buffer_bytes,i2s_config->playback.cfg.period_bytes, DMA_MEM_TO_DEV);
    if(dma_tx_transfer->cdesc <= 0)
    {
        printk(KERN_ERR "cyclic desc err\n");
        ret = -ENOMEM;
        goto fail;
    }
    dma_tx_transfer->cdesc->period_callback = fh_I2S_tx_dma_done;
    dma_tx_transfer->cdesc->period_callback_param = i2s_config;
    fh_dma_cyclic_start(dma_tx_transfer->chan);
    if (tx_slave)
    {
        kfree(tx_slave);
    }

    reg = readl(fh_i2s_module.regs + I2S_CTRL);
    reg = reg <<I2S_SHIFT_BIT;
    reg |= I2S_DACL_CHEN_EN;
	writel(reg,fh_i2s_module.regs + I2S_CTRL);// enable left tx fifo

	writel(0x1<<4,fh_i2s_module.regs +  I2S_TXFIFO_CTRL);// clear tx fifo
	writel(0x20027,fh_i2s_module.regs + I2S_TXFIFO_CTRL);// enbale tx fifo

    /*must set NULL to tell DMA driver that we free the DMA slave*/
    dma_tx_transfer->chan->private = NULL;

    return 0;
fail:
    return ret;
}

int fh_i2s_register_rx_dma( struct fh_i2s_cfg  *i2s_config)
{
    int ret;
    unsigned int reg;
    struct fh_dma_slave *rx_slave;
    rx_slave =  kzalloc(sizeof(struct fh_dma_slave), GFP_KERNEL);
    if (!rx_slave)
    {
        return -ENOMEM;
    }

    rx_slave->cfg_hi = FHC_CFGH_SRC_PER(I2S_HW_NUM_RX);
    rx_slave->dst_msize = FH_DMA_MSIZE_8;
    rx_slave->src_msize = FH_DMA_MSIZE_8;
    rx_slave->reg_width = FH_DMA_SLAVE_WIDTH_32BIT;
    rx_slave->fc = FH_DMA_FC_D_P2M;
    rx_slave->rx_reg = I2S_ADC_FIFO;
	dma_rx_transfer->chan->private =  rx_slave;

    if ((i2s_config->capture.cfg.buffer_bytes < i2s_config->capture.cfg.period_bytes) ||
        (i2s_config->capture.cfg.buffer_bytes <= 0) ||(i2s_config->capture.cfg.period_bytes <= 0) ||
        (i2s_config->capture.cfg.buffer_bytes/i2s_config->capture.cfg.period_bytes > NR_DESCS_PER_CHANNEL))
    {
        printk(KERN_ERR "buffer_size and period_size are invalid\n");
        ret = -EINVAL;
        goto fail;
    }

    dma_rx_transfer->cdesc=
    fh_dma_cyclic_prep(dma_rx_transfer->chan,i2s_config->capture.addr,
    		i2s_config->capture.cfg.buffer_bytes,i2s_config->capture.cfg.period_bytes, DMA_DEV_TO_MEM);
    if(dma_rx_transfer->cdesc <= 0)
    {
        printk(KERN_ERR" cyclic desc err\n");
        ret = -ENOMEM;
        goto fail;
    }

    dma_rx_transfer->cdesc->period_callback = fh_I2S_rx_dma_done;
    dma_rx_transfer->cdesc->period_callback_param = i2s_config;
    fh_dma_cyclic_start(dma_rx_transfer->chan);

    reg = readl(fh_i2s_module.regs + I2S_CTRL);
    reg = reg<<I2S_SHIFT_BIT;
    reg |= I2S_ADC_CHEN_EN ;
  	writel(reg,fh_i2s_module.regs + I2S_CTRL);//enbale dac fifo
	writel(0x1<<4,fh_i2s_module.regs + I2S_RXFIFO_CTRL);// clear rx fifo
	writel(0x20027,fh_i2s_module.regs + I2S_RXFIFO_CTRL);// enable rx fifo


    if (rx_slave)
    {
        kfree(rx_slave);
    }
    /*must set NULL to tell DMA driver that we free the DMA slave*/
    dma_rx_transfer->chan->private = NULL;
    return 0;
fail:
    kfree(rx_slave);
    return ret;
}


void fh_i2s_playback_start_wq_handler(struct work_struct *work)
{
    int avail;
    while(1)
    {
        if (STATE_STOP == fh_i2s_dev.i2s_config.playback.state)
        {
            return;
        }
        avail = i2s_avail_data_len(playback, &fh_i2s_dev.i2s_config);
        if (avail > fh_i2s_dev.i2s_config.playback.cfg.period_bytes)
        {
            msleep(0);
        }
        else
        {
            break;
        }

    }
}

int fh_I2S_start_playback(struct fh_i2s_cfg *i2s_config)
{
    int ret;

    if(i2s_config->playback.state == STATE_RUN)
    {
        return 0;
    }
	if (i2s_config->playback.cfg.buffer_bytes >= I2S_DMA_PREALLOC_SIZE)
    {
        printk("DMA prealloc buffer is smaller than  i2s_config->buffer_bytes\n");
        return -ENOMEM;
    }
    memset(i2s_config->playback.area, 0x0, i2s_config->playback.cfg.buffer_bytes);
    i2s_config->playback.size = i2s_config->playback.cfg.buffer_bytes;
    i2s_config->playback.state = STATE_RUN;
    ret = fh_i2s_register_tx_dma(i2s_config);
    if (ret < 0)
    {
        return ret;
    }
    INIT_WORK(&playback_wq, fh_i2s_playback_start_wq_handler);
    schedule_work(&playback_wq);

    return 0;
}

int fh_I2S_start_capture(struct fh_i2s_cfg *i2s_config)
{
    if(i2s_config->capture.state == STATE_RUN)
    {
        return 0;
    }

    if (i2s_config->capture.cfg.buffer_bytes >= I2S_DMA_PREALLOC_SIZE)
    {
        printk("DMA prealloc buffer is smaller than  i2s_config->buffer_bytes\n");
        return -ENOMEM;
    }
    memset(i2s_config->capture.area, 0, i2s_config->capture.cfg.buffer_bytes);
    i2s_config->capture.size = i2s_config->capture.cfg.buffer_bytes;

    i2s_config->capture.state = STATE_RUN;
    return fh_i2s_register_rx_dma(i2s_config);
}


static void fh_I2S_rx_dma_done(void *arg)
{
    struct fh_i2s_cfg *i2s_config;
    i2s_config = ( struct fh_i2s_cfg *)arg;
    spin_lock(&i2s_config->capture.lock);
    i2s_config->capture.hw_ptr += i2s_config->capture.cfg.period_bytes;
    if (i2s_config->capture.hw_ptr > i2s_config->capture.size )
    {
        i2s_config->capture.hw_ptr = i2s_config->capture.hw_ptr - i2s_config->capture.size;
    }
    spin_unlock(&i2s_config->capture.lock);
    if (waitqueue_active(&i2s_config->readqueue))
    {
        int avail = i2s_avail_data_len(capture,i2s_config);
        if (avail > i2s_config->capture.cfg.period_bytes)
        {
            wake_up_interruptible(&i2s_config->readqueue);
        }
    }

}


static void fh_I2S_tx_dma_done(void *arg)
{

	struct fh_i2s_cfg *i2s_config;
    i2s_config = ( struct fh_i2s_cfg *)arg;
	spin_lock(&i2s_config->playback.lock);
    i2s_config->playback.hw_ptr +=  i2s_config->playback.cfg.period_bytes;
    if (i2s_config->playback.hw_ptr > i2s_config->playback.size )
    {
        i2s_config->playback.hw_ptr = i2s_config->playback.hw_ptr - i2s_config->playback.size;
    }
    spin_unlock(&i2s_config->playback.lock);
    if (waitqueue_active(&i2s_config->writequeue))
    {
        int avail = i2s_avail_data_len(playback,i2s_config);
        if (avail > i2s_config->playback.cfg.period_bytes)
        {
            wake_up_interruptible(&i2s_config->writequeue);
        }
    }
}

bool  fh_I2S_dma_chan_filter(struct dma_chan *chan, void *filter_param)
{
    int dma_channel = *(int *)filter_param;
    bool ret = false;

    if (chan->chan_id == dma_channel)
    {
        ret = true;
    }
    return ret;
}

int fh_i2s_arg_config_support(struct fh_i2s_cfg_arg * cfg)
{
    return 0;
}

void fh_i2s_reset_dma_buff(enum i2s_type type, struct fh_i2s_cfg *i2s_config)
{
    if (capture == type)
    {
        i2s_config->capture.appl_ptr = 0;
        i2s_config->capture.hw_ptr = 0;
    }
    else
    {
        i2s_config->playback.appl_ptr = 0;
        i2s_config->playback.hw_ptr = 0;
    }
}

static long fh_i2s_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct fh_i2s_cfg_arg cfg;
    struct miscdevice *miscdev = filp->private_data;
    struct i2s_dev  *dev = container_of(miscdev, struct i2s_dev, fh_i2s_miscdev);
    struct fh_i2s_cfg  *i2s_config = &dev->i2s_config;
    int pid;
    int rx_status;
    int tx_status;

    pid = current->tgid;
    switch (cmd)
    {
        case I2S_INIT_CAPTURE_MEM:

            if (copy_from_user((void *)&cfg, (void __user *)arg, sizeof(struct fh_i2s_cfg_arg)))
            {
                printk(KERN_ERR "copy err\n");
                return -EIO;
            }
            if (0 == fh_i2s_arg_config_support(&cfg))
            {
            	if (down_trylock(&i2s_config->sem_capture))
	            {
	            	printk(KERN_ERR "another thread is running capture.\n");
	                return -EBUSY;
	            }
               	i2s_infor_record.record_pid =  pid;
                i2s_config->capture.cfg.volume = cfg.volume;
                i2s_config->capture.cfg.rate = cfg.rate;
                i2s_config->capture.cfg.channels = cfg.channels;
                i2s_config->capture.cfg.buffer_size = cfg.buffer_size;
                i2s_config->capture.cfg.frame_bit = FIX_SAMPLE_BIT;
                i2s_config->capture.cfg.period_size = cfg.period_size;
                i2s_config->capture.cfg.buffer_bytes = fh_i2s_frames_to_bytes(i2s_config->capture.cfg.frame_bit,i2s_config->capture.cfg.buffer_size);
                i2s_config->capture.cfg.period_bytes = fh_i2s_frames_to_bytes(i2s_config->capture.cfg.frame_bit,i2s_config->capture.cfg.period_size);
                i2s_config->capture.cfg.start_threshold =i2s_config->capture.cfg.buffer_bytes;
                i2s_config->capture.cfg.stop_threshold = i2s_config->capture.cfg.buffer_bytes;
                fh_i2s_reset_dma_buff(capture, i2s_config);
                init_waitqueue_head(&i2s_config->readqueue);
                spin_lock_init(&i2s_config->capture.lock);
                init_i2s(capture, i2s_config);
                i2s_param_store.input_volume = i2s_config->capture.cfg.volume;
                /* *  config sample  *  */

            }
            else
            {
                return -EINVAL;
            }

            break;
        case I2S_INIT_PLAYBACK_MEM:
            if (copy_from_user((void *)&cfg, (void __user *)arg, sizeof(struct fh_i2s_cfg_arg)))
            {
                printk(KERN_ERR "copy err\n");
                return -EIO;
            }
            
            if (0 == fh_i2s_arg_config_support(&cfg))
            {
            	if (down_trylock(&i2s_config->sem_playback))
	            {
	            	printk(KERN_ERR "another thread is running playback.\n");
	                return -EBUSY;
	            }
            	i2s_infor_record.play_pid = pid;
                i2s_config->playback.cfg.volume = cfg.volume;
                i2s_config->playback.cfg.rate = cfg.rate;
                i2s_config->playback.cfg.channels = cfg.channels;
                i2s_config->playback.cfg.buffer_size = cfg.buffer_size;
                i2s_config->playback.cfg.frame_bit = FIX_SAMPLE_BIT;
                i2s_config->playback.cfg.period_size = cfg.period_size;
                i2s_config->playback.cfg.buffer_bytes = fh_i2s_frames_to_bytes(i2s_config->playback.cfg.frame_bit,i2s_config->playback.cfg.buffer_size);
                i2s_config->playback.cfg.period_bytes = fh_i2s_frames_to_bytes(i2s_config->playback.cfg.frame_bit,i2s_config->playback.cfg.period_size);
                i2s_config->playback.cfg.start_threshold =i2s_config->playback.cfg.buffer_bytes;
                i2s_config->playback.cfg.stop_threshold = i2s_config->playback.cfg.buffer_bytes;
                fh_i2s_reset_dma_buff(playback, i2s_config);
                init_waitqueue_head(&i2s_config->writequeue);
                spin_lock_init(&i2s_config->playback.lock);
                init_i2s(playback, i2s_config);

            }
            else
            {
                return -EINVAL;
            }
            break;
        case I2S_AI_EN:
            if (i2s_infor_record.record_pid != pid){
            	return -EBUSY;
            }
            return fh_I2S_start_capture(i2s_config);
        case I2S_AO_EN:
			if (i2s_infor_record.play_pid != pid) {
				return -EBUSY;
			}
			return fh_I2S_start_playback(i2s_config);

        case I2S_AI_DISABLE:
            printk("[ac_driver]AC_AI_DISABLE\n");
			if (i2s_infor_record.record_pid != pid) {
				return -EBUSY;
			}
			fh_I2S_stop_capture(i2s_config);
            printk(" AC_AI_DISABLE\n");
            break;
        case I2S_AO_DISABLE:
            printk("[ac_driver]AC_AO_DISABLE\n");
			if (i2s_infor_record.play_pid != pid) {
				return -EBUSY;
			}
            fh_I2S_stop_playback(i2s_config);
            printk(" AC_AO_DISABLE\n");
            break;
        case I2S_AI_PAUSE:
			if (i2s_infor_record.record_pid != pid) {
				return -EBUSY;
			}
            printk(KERN_INFO "capture pause\n");
            i2s_config->capture.state = STATE_PAUSE;
            rx_status = readl(fh_i2s_module.regs + I2S_RXFIFO_CTRL);/*rx fifo disable*/
            rx_status =  rx_status&(~I2S_ADC_FIFO_EN);
            writel(rx_status, fh_i2s_module.regs + I2S_RXFIFO_CTRL);/*rx fifo disable*/
            break;
        case I2S_AI_RESUME:
			if (i2s_infor_record.record_pid != pid) {
				return -EBUSY;
			}
            printk(KERN_INFO "capture resume\n");
            i2s_config->capture.state = STATE_RUN;
            rx_status = readl( fh_i2s_module.regs + I2S_RXFIFO_CTRL);//clear rx fifo
            rx_status =  rx_status|I2S_ADC_FIFO_CLEAR;
            writel(rx_status,fh_i2s_module.regs + I2S_RXFIFO_CTRL);/*enable rx fifo*/
            rx_status =  rx_status&(~I2S_ADC_FIFO_CLEAR);
            rx_status =  rx_status|I2S_ADC_FIFO_EN;
            writel(rx_status,fh_i2s_module.regs + I2S_RXFIFO_CTRL);/*enable rx fifo*/
            break;
        case I2S_AO_PAUSE:
			if (i2s_infor_record.play_pid != pid) {
				return -EBUSY;
			}
            i2s_config->playback.state = STATE_PAUSE;
            printk(KERN_INFO "playback pause\n");
            tx_status = readl(fh_i2s_module.regs + I2S_TXFIFO_CTRL);/*rx fifo disable*/
            tx_status =  tx_status&(~I2S_DAC_FIFO_EN);
            writel(tx_status, fh_i2s_module.regs + I2S_TXFIFO_CTRL);/*tx fifo disable*/
            break;
        case I2S_AO_RESUME:
			if (i2s_infor_record.play_pid != pid) {
				return -EBUSY;
			}
            printk(KERN_INFO "playback resume\n");
            i2s_config->playback.state = STATE_RUN;
            tx_status = readl( fh_i2s_module.regs + I2S_TXFIFO_CTRL);//clear rx fifo
            tx_status =  tx_status|I2S_DAC_FIFO_EN;
            writel(tx_status,fh_i2s_module.regs + I2S_TXFIFO_CTRL); //enable tx fifo read enable
            break;
        default:
            return -ENOTTY;
    }
    return 0;
}

static int fh_i2s_open(struct inode *ip, struct file *fp)
{

    fp->private_data = &fh_i2s_dev.fh_i2s_miscdev;

    return 0;
}

static u32 fh_i2s_poll(struct file *filp, poll_table *wait)
{
    struct miscdevice *miscdev = filp->private_data;
    struct i2s_dev  *dev = container_of(miscdev, struct i2s_dev, fh_i2s_miscdev);
    struct fh_i2s_cfg  *i2s_config = &dev->i2s_config;
    u32 mask = 0;
    long avail;
    if (STATE_RUN == i2s_config->capture.state)
    {
        poll_wait(filp,&i2s_config->readqueue,wait);
        avail = i2s_avail_data_len(capture, i2s_config);
        if (avail >  i2s_config->capture.cfg.period_bytes)
        {
            mask |=  POLLIN | POLLRDNORM;
        }
    }
    if (STATE_RUN == i2s_config->playback.state)
    {
        poll_wait(filp,&i2s_config->writequeue,wait);
        avail = i2s_avail_data_len(playback, i2s_config);
        if (avail >  i2s_config->playback.cfg.period_bytes)
        {
            mask |=  POLLOUT | POLLWRNORM;
        }
    }
    return mask;
}

static int fh_i2s_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{

    int ret;
    struct miscdevice *miscdev = filp->private_data;
    struct i2s_dev  *dev = container_of(miscdev, struct i2s_dev, fh_i2s_miscdev);
    struct fh_i2s_cfg  *i2s_config = &dev->i2s_config;
    int after,left;
    int pid,avail;
    pid = current->tgid;
    if (i2s_infor_record.record_pid != pid){
    	return -EBUSY;
    }

    avail = i2s_avail_data_len(capture, i2s_config);
    if (avail > len)
    {
        avail = len;
    }
    after = avail + i2s_config->capture.appl_ptr;
    if(after  > i2s_config->capture.size)
    {
        left = avail - (i2s_config->capture.size - i2s_config->capture.appl_ptr);
        ret = copy_to_user(buf, i2s_config->capture.area+i2s_config->capture.appl_ptr, i2s_config->capture.size-i2s_config->capture.appl_ptr);
        ret = copy_to_user(buf+i2s_config->capture.size-i2s_config->capture.appl_ptr,i2s_config->capture.area,left);
        spin_lock(&i2s_config->capture.lock);
        i2s_config->capture.appl_ptr = left;
        spin_unlock(&i2s_config->capture.lock);
    }
    else
    {
        ret = copy_to_user(buf,i2s_config->capture.area+i2s_config->capture.appl_ptr,avail);
        spin_lock(&i2s_config->capture.lock);
        i2s_config->capture.appl_ptr += avail;
        spin_unlock(&i2s_config->capture.lock);
    }

    return avail;

}

static int fh_i2s_write(struct file *filp, const char __user *buf,
                            size_t len, loff_t *off)
{

    struct miscdevice *miscdev = filp->private_data;
    struct i2s_dev  *dev = container_of(miscdev, struct i2s_dev, fh_i2s_miscdev);
    struct fh_i2s_cfg  *i2s_config = &dev->i2s_config;
    int  ret;
    int after,left;
    int pid,avail;
   pid = current->tgid;
   if (i2s_infor_record.play_pid != pid){
	return -EBUSY;
   }
    avail = i2s_avail_data_len(playback,i2s_config);
    if (0 == avail)
    {
        return 0;
    }
    if (avail > len)
    {
        avail = len;
    }
    after = avail+i2s_config->playback.appl_ptr;
    if(after  > i2s_config->playback.size)
    {
        left = avail - (i2s_config->playback.size-i2s_config->playback.appl_ptr);
        ret = copy_from_user(i2s_config->playback.area+i2s_config->playback.appl_ptr,buf,i2s_config->playback.size-i2s_config->playback.appl_ptr);
        ret = copy_from_user(i2s_config->playback.area,buf+i2s_config->playback.size-i2s_config->playback.appl_ptr,left);
        spin_lock(&i2s_config->playback.lock);
        i2s_config->playback.appl_ptr = left;
        spin_unlock(&i2s_config->playback.lock);
    }
    else
    {
        ret = copy_from_user(i2s_config->playback.area+i2s_config->playback.appl_ptr,buf,avail);
        spin_lock(&i2s_config->playback.lock);
        i2s_config->playback.appl_ptr += avail;
        spin_unlock(&i2s_config->playback.lock);
    }

     return avail;
}

static irqreturn_t fh_i2s_interrupt(int irq, void *dev_id)
{
#if 0
#ifndef CONFIG_MACH_FH8830_FPGA
	u32 interrupts, rx_status;
    struct fh_i2s_cfg  *i2s_config = &fh_i2s_dev.i2s_config;

    interrupts = readl(fh_i2s_module.regs + I2S_CTRL);
    //interrupts &= ~(0x3ff) << 16;
    writel(interrupts, fh_i2s_module.regs + I2S_CTRL);

    if(interrupts & I2S_INTR_RX_UNDERFLOW)
    {
        fh_I2S_stop_capture(i2s_config);
        fh_I2S_start_capture(i2s_config);
        PRINT_i2s_DBG("I2S_INTR_RX_UNDERFLOW\n");
    }

    if(interrupts & I2S_INTR_RX_OVERFLOW)
    {
        if (i2s_config->capture.state == STATE_RUN) {
            fh_I2S_stop_capture(i2s_config);
            fh_I2S_start_capture(i2s_config);
        } else {
            rx_status = readl( fh_i2s_module.regs + I2S_RXFIFO_CTRL);//clear rx fifo
            rx_status =  rx_status|(1<<4);
            writel(rx_status,fh_i2s_module.regs + I2S_RXFIFO_CTRL);
        }
        PRINT_i2s_DBG("I2S_INTR_RX_OVERFLOW\n");
    }

    if(interrupts & I2S_INTR_TX_UNDERFLOW)
    {
        fh_I2S_stop_playback(i2s_config);
        fh_I2S_start_playback(i2s_config);
        PRINT_i2s_DBG("I2S_INTR_TX_UNDERFLOW\n");
    }

    if(interrupts & I2S_INTR_TX_OVERFLOW)
    {
        fh_I2S_stop_playback(i2s_config);
        fh_I2S_start_playback(i2s_config);
        PRINT_i2s_DBG("I2S_INTR_TX_OVERFLOW\n");
    }

    PRINT_i2s_DBG("interrupts: 0x%x\n", interrupts);
#endif
#endif
    return IRQ_HANDLED;
}

static const struct file_operations I2S_fops =
{
    .owner      = THIS_MODULE,
    .llseek     = no_llseek,
    .unlocked_ioctl = fh_i2s_ioctl,
    .release = fh_i2s_close,
    .open = fh_i2s_open,
    .poll = fh_i2s_poll,
    .read = fh_i2s_read,
    .write = fh_i2s_write,

};

static int __devinit fh_i2s_drv_probe(struct platform_device *pdev)
{
    int ret;
    struct resource *irq_res, *mem;

    mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!mem)
        return -EINVAL;
    printk("I2S probe\n");
    if (!devm_request_mem_region(&pdev->dev, mem->start, resource_size(mem),
                                 "fh_fh8830_i2s_module"))
        return -ENOMEM;
    printk("I2S :%d\n",__LINE__);
    PRINT_i2s_DBG("%d\n",__LINE__);
    fh_i2s_module.regs = devm_ioremap(&pdev->dev, mem->start, resource_size(mem));
    PRINT_i2s_DBG("probe: regs %p\n",fh_i2s_module.regs);
    if (!fh_i2s_module.regs){
    	ret = -ENOMEM;
    	goto remap_fail;
        }

    fh_i2s_module.clk = clk_get(NULL, "ac_clk");
	if (!fh_i2s_module.clk) {
		ret = -EINVAL;
		goto clk_fail;
		}
    clk_enable(fh_i2s_module.clk);
    PRINT_i2s_DBG("%d\n",__LINE__);
    spin_lock_init(&fh_i2s_module.lock);

    ret = misc_register(&fh_i2s_dev.fh_i2s_miscdev);

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
    fh_i2s_module.irq = irq_res->start;
    ret = request_irq(fh_i2s_module.irq, fh_i2s_interrupt, IRQF_SHARED, "i2s", &fh_i2s_module);
    ret = readl(fh_i2s_module.regs + I2S_DBG_CTL);
    ret = ret | I2S_EXT_EN| I2S_EN;
    writel(ret,fh_i2s_module.regs + I2S_DBG_CTL);
    return 0;

out_disable_clk:
    clk_disable(fh_i2s_module.clk);
	fh_i2s_module.clk = NULL;
clk_fail:
	devm_iounmap(&pdev->dev, fh_i2s_module.regs);
	fh_i2s_module.regs = NULL;
remap_fail:
	devm_release_mem_region(&pdev->dev, mem->start, resource_size(mem));
    return ret;
}

static int __devexit fh_I2S_drv_remove(struct platform_device *pdev)
{
	struct resource *mem;
    misc_deregister(&fh_i2s_dev.fh_i2s_miscdev);

    free_irq(fh_i2s_module.irq, &fh_i2s_module);

	if (fh_i2s_module.clk) {
		clk_disable(fh_i2s_module.clk);
    	clk_put(fh_i2s_module.clk);
	}
	if (fh_i2s_module.regs) {
		devm_iounmap(&pdev->dev, fh_i2s_module.regs);
	}
	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (mem) {
		devm_release_mem_region(&pdev->dev, mem->start, resource_size(mem));
    }
    printk("I2S remove ok\n");
    return 0;
}

static struct platform_driver fh_i2s_driver =
{
    .probe      = fh_i2s_drv_probe,
    .remove     = __devexit_p(fh_I2S_drv_remove),
    .driver     = {
        .name   = "fh_fh8830_i2s",
        .owner  = THIS_MODULE,
    }
};

void i2s_prealloc_dma_buffer(struct fh_i2s_cfg  *i2s_config)
{
    int pg;
    gfp_t gfp_flags;
    pg = get_order(I2S_DMA_PREALLOC_SIZE);
    gfp_flags = GFP_KERNEL | __GFP_COMP | __GFP_NORETRY | __GFP_NOWARN;
    i2s_config->capture.dev.coherent_dma_mask = DMA_BIT_MASK(32);
    i2s_config->capture.area  = dma_alloc_coherent(&i2s_config->capture.dev, PAGE_SIZE << pg, &i2s_config->capture.addr, gfp_flags );
    if (!i2s_config->capture.area)
    {
        printk(KERN_ERR"no enough mem for capture  buffer alloc\n");
        return ;
    }
    i2s_config->playback.dev.coherent_dma_mask = DMA_BIT_MASK(32);
    i2s_config->playback.area  = dma_alloc_coherent(&i2s_config->playback.dev, PAGE_SIZE << pg, &i2s_config->playback.addr, gfp_flags );
    if (!i2s_config->playback.area)
    {
        printk(KERN_ERR"no enough mem for  playback buffer alloc\n");
        return ;
    }
}

void i2s_free_prealloc_dma_buffer(struct fh_i2s_cfg  *i2s_config)
{
    int pg;
    pg = get_order(I2S_DMA_PREALLOC_SIZE);
    dma_free_coherent(&i2s_config->capture.dev, PAGE_SIZE<<pg, i2s_config->capture.area, i2s_config->capture.addr);
    dma_free_coherent(&i2s_config->playback.dev, PAGE_SIZE<<pg, i2s_config->playback.area, i2s_config->playback.addr);
}

static void init_i2s_mutex(struct fh_i2s_cfg  *i2s_config)
{
    sema_init(&i2s_config->sem_capture, 1);
    sema_init(&i2s_config->sem_playback, 1);
}

int i2s_request_dma_channel(void)
{
    dma_cap_mask_t mask;
    /*request i2s rx dma channel*/
    dma_rx_transfer = kzalloc(sizeof(struct fh_dma_chan), GFP_KERNEL);
    if (!dma_rx_transfer)
    {
        printk(KERN_ERR"alloc  dma_rx_transfer failed\n");
        goto mem_fail;
    }
    memset(dma_rx_transfer, 0, sizeof(struct fh_dma_chan));

    dma_cap_zero(mask);
    dma_cap_set(DMA_SLAVE, mask);
    dma_rx_transfer->chan = dma_request_channel(mask, fh_I2S_dma_chan_filter, &fh_i2s_dev.channel_assign.capture_channel);
    if (!dma_rx_transfer->chan)
    {
        printk(KERN_ERR"request i2s rx dma channel failed \n");
        goto channel_fail;
    }

    /*request i2s tx dma channel*/
    dma_tx_transfer = kzalloc(sizeof(struct fh_dma_chan), GFP_KERNEL);
    if (!dma_tx_transfer)
    {
        printk(KERN_ERR"alloc  dma_tx_transfer failed\n");
        goto mem_fail;
    }
    memset(dma_tx_transfer, 0, sizeof(struct fh_dma_chan));

    dma_cap_zero(mask);
    dma_cap_set(DMA_SLAVE, mask);
    dma_tx_transfer->chan = dma_request_channel(mask, fh_I2S_dma_chan_filter, &fh_i2s_dev.channel_assign.playback_channel);
    if (!dma_tx_transfer->chan)
    {
        printk(KERN_ERR"request dma channel failed \n");
        return -EFAULT;
    }

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

    return -EFAULT;
}

void i2s_release_dma_channel(void)
{
    /*release i2s tx dma channel*/
    if (dma_tx_transfer != NULL)
    {
    	if (dma_tx_transfer->chan) {
			dma_release_channel(dma_tx_transfer->chan);
			dma_tx_transfer->chan = NULL;
    	}
        kfree(dma_tx_transfer);
        dma_tx_transfer = NULL;
    }

    /*release i2s rx dma channel*/
    if (dma_rx_transfer != NULL)
    {
    	if (dma_rx_transfer->chan) {
			dma_release_channel(dma_rx_transfer->chan);
			dma_rx_transfer->chan = NULL;
    	}
        
        kfree(dma_rx_transfer);
        dma_rx_transfer = NULL;
    }


}
static void create_fh8830_i2s_proc(void);
static void remove_fh8830_i2s_proc(void);
static int __init fh_i2s_init(void)
{
    int status;
	init_i2s_mutex(&fh_i2s_dev.i2s_config);

    i2s_prealloc_dma_buffer(&fh_i2s_dev.i2s_config);

    status = i2s_request_dma_channel();
    if(status)
    	printk("fh i2s init fail status=0x%x\n",status);
    create_fh8830_i2s_proc();
    return platform_driver_register(&fh_i2s_driver);
}
module_init(fh_i2s_init);

static void __exit fh_i2s_exit(void)
{

    remove_fh8830_i2s_proc();
    i2s_release_dma_channel();
    i2s_free_prealloc_dma_buffer(&fh_i2s_dev.i2s_config);
    platform_driver_unregister(&fh_i2s_driver);
}
module_exit(fh_i2s_exit);

MODULE_AUTHOR("FH_i2s");
MODULE_DESCRIPTION("FH_i2s");
MODULE_LICENSE("GPL");

/****************************debug proc*****************************/
#include <linux/proc_fs.h>
#include <asm/unistd.h>
struct proc_dir_entry *proc_ac_entry;
#define proc_name "fh_fh8830_i2s"
#define I2S_TEST_LOOP 1
#define I2S_TEST_OUT  0
ssize_t proc_ac_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    ssize_t len = 0;
	int i;
	unsigned int reg;
	unsigned int *data_addr;
	unsigned int file_len;
	struct file *fp;
	loff_t pos;
	mm_segment_t old_fs;
	for (i = 0;i <= 0x1C;i += 4) {
		printk(KERN_INFO"0x%x reg = 0x%x\n",i, readl(fh_i2s_module.regs + i));
	}
	for (i = 80;i <= 0xd0;i += 4) {
			printk(KERN_INFO"0x%x reg = 0x%x\n",i, readl(fh_i2s_module.regs + i));
		}
	printk("loop test start __________\n");
	writel(0xe0000000,fh_i2s_module.regs + I2S_CTRL);// ctrl
	writel(0x1,fh_i2s_module.regs + I2S_TXFIFO_CTRL);// tx
	writel(0x1,fh_i2s_module.regs + I2S_RXFIFO_CTRL);// rx
	writel(0x1 <<12 |0x3,fh_i2s_module.regs + I2S_DBG_CTL);// loop

	writel(0x1000000,0xfe0901ac);
	i = 0;
	data_addr = kzalloc(4096*4, GFP_KERNEL);
	    if (!data_addr)
	    {
	        printk("alloc  dma_rx_transfer failed\n");
	    }
	    memset(data_addr, 0, 4096*4);
	    mdelay(1000);
	while (1) {
#if I2S_TEST_LOOP
		reg =0xff & readl(fh_i2s_module.regs + 0x0c);

		if(reg >0){
			reg = readl(fh_i2s_module.regs +0x200);
			writel(reg,fh_i2s_module.regs + 0x100);
		}

#endif


#if I2S_TEST_OUT
		reg =0xff00 & readl(fh_i2s_module.regs + 0x0c);
		reg = reg>>8;
	//	printk("write dac date reg = %x \n ",reg);
		if (reg < 0x40) {
			writel(i, fh_i2s_module.regs + 0x100);
			writel(i, fh_i2s_module.regs + 0x300);


		}
		i = i+0x200;
		if(i >= 0xffffff)
			i = 0;
	//	printk("water level 0x%x\n",readl(fh_i2s_module.regs + 0x0c));

#endif
	}
//TEST I2S_INPUT TO FILE
	pos =0;

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	fp =  filp_open("/home/test",O_RDWR|O_CREAT,0644);
	if(IS_ERR(fp)){
		printk("file is error\n");
		return 1;

	}
	 file_len = fp->f_op->write(fp,(void*)data_addr,4096,&pos);
	 printk("write len is %d\n",len);
	 set_fs(old_fs);
	 filp_close(fp,NULL);
	printk("loop test stop ___________\n");
	for (i = 0;i <= 0x1C;i += 4) {
		printk(KERN_INFO"0x%x reg = 0x%x\n",i, readl(fh_i2s_module.regs + i));
	}
	for (i = 80;i <= 0xd0;i += 4) {
			printk(KERN_INFO"0x%x reg = 0x%x\n",i, readl(fh_i2s_module.regs + i));
		}
    return len;
}

static void create_fh8830_i2s_proc(void)
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

static void remove_fh8830_i2s_proc(void)
{
    remove_proc_entry(proc_name, NULL);
}
