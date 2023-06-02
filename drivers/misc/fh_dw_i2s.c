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
 * @warning: the codec is fixed to 24 bit,
 * so remember to move the 24 bit data to 16 bit in
 *   application layer, the next version CPU will sovle this bug.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

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
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/sched.h>
#include <mach/fh_dmac.h>
#include <mach/pmu.h>
#include <mach/fh_predefined.h>
#include <mach/chip.h>
#include <mach/fh_i2s.h>

#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "ac.h"

#define DEVICE_NAME				"fh_audio"

#define NR_DESCS_PER_CHANNEL	64
#define FIX_SAMPLE_BIT			32

#define AUDIO_DMA_PREALLOC_SIZE  (128*1024)

#define I2S_INTR_RX_AVAILABLE       0x1
#define I2S_INTR_RX_OVERRUN         0x2
#define I2S_INTR_TX_AVAILABLE       0x10
#define I2S_INTR_TX_OVERRUN         0x20

#define I2S_REG_IER_OFFSET          0x00  /*i2s enable reg*/
#define I2S_REG_IRER_OFFSET         0x04  /*i2s receiver block enable*/
#define I2S_REG_ITER_OFFSET         0x08  /*i2s transmitter block*/
#define I2S_REG_CER_OFFSET          0x0c /*clk en*/
#define I2S_REG_CCR_OFFSET          0x10 /*clk cfg reg*/
#define I2S_REG_RXFFR_OFFSET        0x14 /*reset rx fifo reg*/
#define I2S_REG_TXFFR_OFFSET        0x18 /*reset tx fifo reg*/
#define I2S_REG_LRBR0_OFFSET        0x20 /*left rx/tx buf reg*/
#define I2S_REG_RRBR0_OFFSET        0x24 /*right rx/tx buf reg*/
#define I2S_REG_RER0_OFFSET         0x28  /*rx en register*/
#define I2S_REG_TER0_OFFSET         0x2c  /*tx en*/
#define I2S_REG_RCR0_OFFSET         0x30  /*rx config*/
#define I2S_REG_TCR0_OFFSET         0x34 /*tx config*/
#define I2S_REG_ISR0_OFFSET         0x38 /*intt status reg*/
#define I2S_REG_IMR0_OFFSET         0x3c /*intt mask reg*/
#define I2S_REG_ROR0_OFFSET         0x40  /*rx overrun reg*/
#define I2S_REG_TOR0_OFFSET         0x44  /*tx overrun reg*/
#define I2S_REG_RFCR0_OFFSET        0x48  /*rx fifo config reg*/
#define I2S_REG_TFCR0_OFFSET        0x4c  /*tx fifo config reg*/
#define I2S_REG_RFF0_OFFSET         0x50  /*rx fifo flush reg*/
#define I2S_REG_TFF0_OFFSET         0x54  /*tx fifo flush reg*/

#define I2S_REG_RXDMA_OFFSET	0x1c0 /*Receiver Block DMA Register*/
#define I2S_REG_RRXDMA_OFFSET	0x1c4 /*Reset Receiver Block DMA Register*/
#define I2S_REG_TXDMA_OFFSET	0x1c8 /*Transmitter Block DMA Register*/
#define I2S_REG_RTXDMA_OFFSET	0x1cc /*Reset Transmitter Block DMA Register*/

#define I2S_REG_DMACR_OFFSET	0x180 /* DMA Control Register */
#define I2S_REG_DMATDLR_OFFSET	0x184
#define I2S_REG_DMARDLR_OFFSET	0x188

#define I2S_DMA_RXEN_BIT		0
#define I2S_DMA_TXEN_BIT		1

#define I2S_DMA_CAP_CHANNEL		2
#define I2S_DMA_PLAY_CHANNEL	3

/* #define FH_AUDIO_DEBUG*/
#ifdef FH_AUDIO_DEBUG
#define PRINT_AUDIO_DBG(fmt, args...)           \
	do {                                        \
		printk(KERN_INFO "FH_AUDIO_DEBUG: ");   \
		printk(fmt, ## args);                   \
	} while (0);
#else
#define PRINT_AUDIO_DBG(fmt, args...)  do { } while (0)
#endif

enum audio_type {
	capture = 0, playback,
};

enum audio_state {
	STATE_NORMAL = 0, STATE_INIT, STATE_STOP, STATE_RUN, STATE_PAUSE
};

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

struct audio_ptr_t {
	struct audio_config cfg;
	enum audio_state state;
	long size;
	int hw_ptr;
	int appl_ptr;
	spinlock_t lock;
	struct device dev;
	u8 *area; /*virtual pointer*/
	dma_addr_t addr; /*physical address*/
	u8 *mmap_addr;
};

struct fh_audio_cfg {
	struct audio_ptr_t capture;
	struct audio_ptr_t playback;
	wait_queue_head_t readqueue;
	wait_queue_head_t writequeue;
	struct semaphore sem_capture;
	struct semaphore sem_playback;
};

struct fh_dma_chan {
	struct dma_chan *chan;
	void __iomem *ch_regs;
	u8 mask;
	u8 priority;
	bool paused;
	bool initialized;
	spinlock_t lock;
	/* these other elements are all protected by lock */
	unsigned long flags;
	dma_cookie_t completed;
	struct list_head active_list;
	struct list_head queue;
	struct list_head free_list;
	struct fh_cyclic_desc *cdesc;
	unsigned int descs_allocated;
};

struct channel_assign {
	int capture_channel;
	int playback_channel;
};

struct audio_dev {
	struct channel_assign channel_assign;
	int dma_master;
	struct fh_audio_cfg audio_config;
	struct miscdevice fh_audio_miscdev;
	struct fh_i2s_platform_data *plat_data;
};

static const struct file_operations fh_i2s_fops;

static struct audio_dev fh_audio_dev = {
	.channel_assign = {
		.capture_channel = I2S_DMA_CAP_CHANNEL,
		.playback_channel = I2S_DMA_PLAY_CHANNEL,
	},
	.dma_master = 1,
	.fh_audio_miscdev = {
		.fops = &fh_i2s_fops,
		.name = DEVICE_NAME,
		.minor = MISC_DYNAMIC_MINOR,
	}
};

static struct {
	spinlock_t lock;
	unsigned long paddr;
	void __iomem *vaddr;
	unsigned long acw_vaddr;
	struct clk *clk;
	struct clk *acodec_clk;
	unsigned long in_use;
	unsigned long next_heartbeat;
	struct timer_list timer;
	int expect_close;
	int irq;
} fh_i2s_module;

static struct infor_record_t
{
	int record_pid;
	int play_pid;
} infor_record;

static struct fh_dma_chan *dma_rx_transfer;
static struct fh_dma_chan *dma_tx_transfer;

static struct work_struct playback_wq;

static struct audio_param_store {
	int input_volume;
	enum io_select input_io_type;
} audio_param_store;

static void fh_i2s_tx_dma_done(void *arg);
static void fh_i2s_rx_dma_done(void *arg);
static bool fh_i2s_dma_chan_filter(struct dma_chan *chan, void *filter_param);
static void create_proc(void);
static void remove_proc(void);
static int config_i2s_clk(u32 rate, u32 bit);

static void fh_i2s_stop_playback(struct fh_audio_cfg *audio_config)
{
	if (audio_config->playback.state == STATE_INIT)
		goto free;
	if (audio_config->playback.state == STATE_STOP)
		return;
	audio_config->playback.state = STATE_STOP;
	clear_bit(I2S_DMA_TXEN_BIT, fh_i2s_module.vaddr + I2S_REG_DMACR_OFFSET);
	writel(0, fh_i2s_module.vaddr + I2S_REG_ITER_OFFSET);
	fh_dma_cyclic_stop(dma_tx_transfer->chan);
	fh_dma_cyclic_free(dma_tx_transfer->chan);

free:
	up(&audio_config->sem_playback);
}

static void fh_i2s_stop_capture(struct fh_audio_cfg *audio_config)
{
	if (audio_config->capture.state == STATE_INIT)
		goto free;
	if (audio_config->capture.state == STATE_STOP)
		return;
	audio_config->capture.state = STATE_STOP;

	clear_bit(I2S_DMA_RXEN_BIT, fh_i2s_module.vaddr + I2S_REG_DMACR_OFFSET);
	writel(0, fh_i2s_module.vaddr + I2S_REG_IRER_OFFSET);

	fh_dma_cyclic_stop(dma_rx_transfer->chan);
	fh_dma_cyclic_free(dma_rx_transfer->chan);

free:
	up(&audio_config->sem_capture);
}

static void switch_io_type(enum audio_type type, enum io_select io_type)
{
}

static int get_param_from_volume(int volume)
{
	int param, max_param, min_param, max_volume;

	max_volume = 100;
	if (volume < 0 || volume > max_volume) {
		printk(KERN_ERR "unsupported input volume\n");
		return -EINVAL;
	}
	max_param = 63;
	min_param = 0;
	param = max_param - (max_volume - volume);
	if (param <= min_param)
		param = min_param;
	return param;
}

static inline long bytes_to_frames(int frame_bit, int bytes)
{
	return bytes * 8 / frame_bit;
}

static inline long frames_to_bytes(int frame_bit, int frames)
{
	return frames * frame_bit / 8;
}

static void switch_input_volume(int volume)
{
}

static void reset_dma_buff(struct audio_ptr_t *config)
{
	config->appl_ptr = 0;
	config->hw_ptr = 0;
}

static int config_i2s_frame_bit(int bit)
{
	int wss = 0;
	int wlen = 0;

	switch (bit) {
	case 16:
		wss = 0;
		wlen = 0b010;
		break;
	case 24:
		wss = 1;
		wlen = 0b100;
		break;
	case 32:
		wss = 2;
		wlen = 0b101;
		break;
	default:
		return -EINVAL;
		break;
	}

	/* config sclk cycles (ws_out) */
	writel(wss << 3, fh_i2s_module.vaddr + I2S_REG_CCR_OFFSET);

	writel(wlen, fh_i2s_module.vaddr + I2S_REG_RCR0_OFFSET);
	writel(wlen, fh_i2s_module.vaddr + I2S_REG_TCR0_OFFSET);
	return 0;
}

static int init_audio(enum audio_type type, struct fh_audio_cfg *audio_config,
		struct fh_audio_cfg_arg *cfg)
{
	struct audio_ptr_t *config = NULL;
	int ret = 0;
	int pid = current->tgid;

	if (type == capture) {
		config = &audio_config->capture;
		init_waitqueue_head(&audio_config->readqueue);
	} else if (type == playback) {
		config = &audio_config->playback;
		init_waitqueue_head(&audio_config->writequeue);
	}

	config->cfg.io_type = cfg->io_type;
	config->cfg.volume = cfg->volume;
	config->cfg.rate = cfg->rate;
	config->cfg.channels = cfg->channels;
	config->cfg.buffer_size = cfg->buffer_size;
	config->cfg.frame_bit = cfg->frame_bit;
	config->cfg.period_size = cfg->period_size;
	config->cfg.buffer_bytes =
		frames_to_bytes(config->cfg.frame_bit, config->cfg.buffer_size);
	config->cfg.period_bytes =
		frames_to_bytes(config->cfg.frame_bit, config->cfg.period_size);
	config->cfg.start_threshold = config->cfg.buffer_bytes;
	config->cfg.stop_threshold = config->cfg.buffer_bytes;
	reset_dma_buff(config);
	spin_lock_init(&config->lock);
	audio_param_store.input_io_type = config->cfg.io_type;
	audio_param_store.input_volume = config->cfg.volume;

	ret = config_i2s_clk(cfg->rate, cfg->frame_bit);
	if (ret) {
		pr_err("config_i2s_clk error %d\n", ret);
		return ret;
	}

	/*  * config wrapper work format  *   */
	writel(1, fh_i2s_module.vaddr + I2S_REG_RER0_OFFSET);/*rx en*/
	writel(1, fh_i2s_module.vaddr + I2S_REG_TER0_OFFSET);/*tx en*/

	/*set dma fifo size*/
	writel(0x1f, fh_i2s_module.vaddr + I2S_REG_DMARDLR_OFFSET);
	/*set dma fifo size*/
	writel(0x1f, fh_i2s_module.vaddr + I2S_REG_DMATDLR_OFFSET);
	writel(0x1, fh_i2s_module.vaddr + I2S_REG_CER_OFFSET);/*en clk*/

	if (type == capture) {
		infor_record.record_pid = pid;
		audio_config->capture.state = STATE_INIT;
	} else if (type == playback) {
		infor_record.play_pid = pid;
		audio_config->playback.state = STATE_INIT;
	}

	writel(0x1, fh_i2s_module.vaddr + I2S_REG_IER_OFFSET);

	return 0;
}

static int avail_data_len(enum audio_type type, struct fh_audio_cfg *stream)
{
	int delta;

	if (capture == type) {
		spin_lock(&stream->capture.lock);
		delta = stream->capture.hw_ptr - stream->capture.appl_ptr;
		spin_unlock(&stream->capture.lock);
		if (delta < 0)
			delta += stream->capture.size;
		return delta;
	} else {
		spin_lock(&stream->playback.lock);
		delta = stream->playback.appl_ptr - stream->playback.hw_ptr;
		spin_unlock(&stream->playback.lock);
		if (delta <= 0)
			delta += stream->playback.size;
		return stream->playback.size - delta;
	}
}

static int fh_audio_close(struct inode *ip, struct file *fp)
{
	struct miscdevice *miscdev = fp->private_data;
	struct audio_dev
	*dev = container_of(miscdev, struct audio_dev, fh_audio_miscdev);
	struct fh_audio_cfg *audio_config = &dev->audio_config;
	int pid;

	pid = current->tgid;
	if (infor_record.play_pid == pid)
		fh_i2s_stop_playback(audio_config);

	if (infor_record.record_pid == pid)
		fh_i2s_stop_capture(audio_config);
	return 0;
}

static int register_tx_dma(struct fh_audio_cfg *audio_config)
{
	int ret;
	struct fh_dma_slave *tx_config;

	tx_config = kzalloc(sizeof(struct fh_dma_slave), GFP_KERNEL);
	if (!tx_config)
		return -ENOMEM;

	tx_config->cfg_hi = FHC_CFGH_DST_PER(11);
	tx_config->dst_msize = FH_DMA_MSIZE_8;
	tx_config->src_msize = FH_DMA_MSIZE_8;
	if (audio_config->playback.cfg.frame_bit == 16)
		tx_config->reg_width = FH_DMA_SLAVE_WIDTH_16BIT;
	else
		tx_config->reg_width = FH_DMA_SLAVE_WIDTH_32BIT;
	tx_config->fc = FH_DMA_FC_D_M2P;
	tx_config->tx_reg = (u32)fh_i2s_module.paddr + I2S_REG_TXDMA_OFFSET;
	tx_config->src_master = 0;
	tx_config->dst_master = fh_audio_dev.dma_master;

	dma_tx_transfer->chan->private = tx_config;
	if ((audio_config->playback.cfg.buffer_bytes
			< audio_config->playback.cfg.period_bytes)
			|| (audio_config->playback.cfg.buffer_bytes <= 0)
			|| (audio_config->playback.cfg.period_bytes <= 0)
			|| (audio_config->playback.cfg.buffer_bytes
			/ audio_config->playback.cfg.period_bytes
			> NR_DESCS_PER_CHANNEL)) {
		printk(KERN_ERR "buffer_size and period_size are invalid\n");
		ret = -EINVAL;
		goto fail;
	}

	dma_tx_transfer->cdesc = fh_dma_cyclic_prep(dma_tx_transfer->chan,
		audio_config->playback.addr,
		audio_config->playback.cfg.buffer_bytes,
		audio_config->playback.cfg.period_bytes,
		DMA_MEM_TO_DEV);
	if (IS_ERR(dma_tx_transfer->cdesc)) {
		printk(KERN_ERR "cyclic desc err %ld\n",
				PTR_ERR(dma_tx_transfer->cdesc));
		ret = -ENOMEM;
		goto fail;
	}
	dma_tx_transfer->cdesc->period_callback = fh_i2s_tx_dma_done;
	dma_tx_transfer->cdesc->period_callback_param = audio_config;
	fh_dma_cyclic_start(dma_tx_transfer->chan);
	kfree(tx_config);
	/*must set NULL to tell DMA driver that we free the DMA slave*/
	dma_tx_transfer->chan->private = NULL;
	return 0;
fail:
	return ret;
}

static int register_rx_dma(struct fh_audio_cfg *audio_config)
{
	int ret;
	struct fh_dma_slave *rx_config;

	rx_config = kzalloc(sizeof(struct fh_dma_slave), GFP_KERNEL);
	if (!rx_config)
		return -ENOMEM;
	rx_config->cfg_hi = FHC_CFGH_SRC_PER(10);
	rx_config->dst_msize = FH_DMA_MSIZE_8;
	rx_config->src_msize = FH_DMA_MSIZE_8;
	if (audio_config->capture.cfg.frame_bit == 16)
		rx_config->reg_width = FH_DMA_SLAVE_WIDTH_16BIT;
	else
		rx_config->reg_width = FH_DMA_SLAVE_WIDTH_32BIT;
	rx_config->fc = FH_DMA_FC_D_P2M;
	rx_config->rx_reg = (u32)fh_i2s_module.paddr + I2S_REG_RXDMA_OFFSET;
	rx_config->src_master = fh_audio_dev.dma_master;
	rx_config->dst_master = 0;
	dma_rx_transfer->chan->private = rx_config;
	if ((audio_config->capture.cfg.buffer_bytes
			< audio_config->capture.cfg.period_bytes)
			|| (audio_config->capture.cfg.buffer_bytes <= 0)
			|| (audio_config->capture.cfg.period_bytes <= 0)
			|| (audio_config->capture.cfg.buffer_bytes
			/ audio_config->capture.cfg.period_bytes
			> NR_DESCS_PER_CHANNEL)) {
		printk(KERN_ERR "buffer_size and period_size are invalid\n");
		ret = -EINVAL;
		goto fail;
	}
	dma_rx_transfer->cdesc = fh_dma_cyclic_prep(dma_rx_transfer->chan,
		audio_config->capture.addr,
		audio_config->capture.cfg.buffer_bytes,
		audio_config->capture.cfg.period_bytes,
		DMA_DEV_TO_MEM);
	if (IS_ERR(dma_rx_transfer->cdesc)) {
		printk(KERN_ERR "cyclic desc err %ld\n",
				PTR_ERR(dma_rx_transfer->cdesc));
		ret = -ENOMEM;
		goto fail;
	}
	dma_rx_transfer->cdesc->period_callback = fh_i2s_rx_dma_done;
	dma_rx_transfer->cdesc->period_callback_param = audio_config;
	fh_dma_cyclic_start(dma_rx_transfer->chan);

	kfree(rx_config);
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

	while (1) {
		if (STATE_STOP == fh_audio_dev.audio_config.playback.state)
			return;
		avail = avail_data_len(playback, &fh_audio_dev.audio_config);
		if (avail
			> fh_audio_dev.audio_config.playback.cfg.period_bytes)
			msleep(20);
		else {
			/*enable rx fifo*/
			writel(0x1, fh_i2s_module.vaddr + I2S_REG_ITER_OFFSET);
			/*reset dma*/
			writel(0x1, fh_i2s_module.vaddr
					+ I2S_REG_RTXDMA_OFFSET);
			/*reset dma*/
			writel(0x1, fh_i2s_module.vaddr + I2S_REG_TXFFR_OFFSET);
			/*reset dma*/
			writel(0x1, fh_i2s_module.vaddr + I2S_REG_TER0_OFFSET);
			break;
		}
	}
}

static int fh_i2s_start_playback(struct fh_audio_cfg *audio_config)
{
	int ret;

	if (audio_config->playback.state == STATE_RUN)
		return 0;

	if (audio_config->playback.cfg.buffer_bytes
			>= AUDIO_DMA_PREALLOC_SIZE) {
		pr_info("DMA prealloc buffer is smaller than"
			"audio_config->buffer_bytes\n");
		return -ENOMEM;
	}
	memset(audio_config->playback.area, 0,
		audio_config->playback.cfg.buffer_bytes);
	audio_config->playback.size = audio_config->playback.cfg.buffer_bytes;
	audio_config->playback.state = STATE_RUN;
	ret = register_tx_dma(audio_config);
	if (ret)
		return ret;
	INIT_WORK(&playback_wq, playback_start_wq_handler);
	schedule_work(&playback_wq);

	/*reset tx dma*/
	writel(0x1, fh_i2s_module.vaddr + I2S_REG_RTXDMA_OFFSET);
	/*reset tx fifo*/
	writel(0x1, fh_i2s_module.vaddr + I2S_REG_TXFFR_OFFSET);

	writel(0x1, fh_i2s_module.vaddr + I2S_REG_ITER_OFFSET);/*tx en*/
	/*dma en tx*/
	set_bit(I2S_DMA_TXEN_BIT, fh_i2s_module.vaddr + I2S_REG_DMACR_OFFSET);

	return 0;
}

static int fh_i2s_start_capture(struct fh_audio_cfg *audio_config)
{
	int ret;

	if (audio_config->capture.state == STATE_RUN)
		return 0;

	if (audio_config->capture.cfg.buffer_bytes >= AUDIO_DMA_PREALLOC_SIZE) {
		printk(
		"DMA prealloc buffer is smaller than  audio_config->buffer_bytes\n");
		return -ENOMEM;
	}
	memset(audio_config->capture.area, 0,
		audio_config->capture.cfg.buffer_bytes);
	audio_config->capture.size = audio_config->capture.cfg.buffer_bytes;

	audio_config->capture.state = STATE_RUN;

	ret = register_rx_dma(audio_config);
	if (ret)
		return ret;

	/*reset rx dma*/
	writel(0x1, fh_i2s_module.vaddr + I2S_REG_RRXDMA_OFFSET);
	/*reset rx fifo*/
	writel(0x1, fh_i2s_module.vaddr + I2S_REG_RXFFR_OFFSET);

	writel(0x1, fh_i2s_module.vaddr + I2S_REG_IRER_OFFSET);/*rx en*/

	/*dma en rx*/
	set_bit(I2S_DMA_RXEN_BIT, fh_i2s_module.vaddr + I2S_REG_DMACR_OFFSET);

	return 0;
}

static void fh_i2s_rx_dma_done(void *arg)
{
	struct fh_audio_cfg *audio_config;

	audio_config = (struct fh_audio_cfg *) arg;
	spin_lock(&audio_config->capture.lock);
	audio_config->capture.hw_ptr += audio_config->capture.cfg.period_bytes;
	if (audio_config->capture.hw_ptr > audio_config->capture.size)
		audio_config->capture.hw_ptr = audio_config->capture.hw_ptr
		- audio_config->capture.size;
	spin_unlock(&audio_config->capture.lock);
	if (waitqueue_active(&audio_config->readqueue)) {
		int avail = avail_data_len(capture, audio_config);
		if (avail > audio_config->capture.cfg.period_bytes)
			wake_up_interruptible(&audio_config->readqueue);
	}

}

static void fh_i2s_tx_dma_done(void *arg)
{
	struct fh_audio_cfg *audio_config;

	audio_config = (struct fh_audio_cfg *) arg;
	spin_lock(&audio_config->playback.lock);
	audio_config->playback.hw_ptr +=
		audio_config->playback.cfg.period_bytes;
	if (audio_config->playback.hw_ptr > audio_config->playback.size) {
		audio_config->playback.hw_ptr = audio_config->playback.hw_ptr
		- audio_config->playback.size;
	}
	memset(audio_config->playback.area + audio_config->playback.hw_ptr
			- audio_config->playback.cfg.period_bytes, 0,
			audio_config->playback.cfg.period_bytes);
	spin_unlock(&audio_config->playback.lock);
	if (waitqueue_active(&audio_config->writequeue)) {
		int avail = avail_data_len(playback, audio_config);

		if (avail > audio_config->playback.cfg.period_bytes)
			wake_up_interruptible(&audio_config->writequeue);
	}
}

static bool fh_i2s_dma_chan_filter(struct dma_chan *chan,
		void *filter_param)
{
	int dma_channel = *(int *) filter_param;
	bool ret = false;

	if (chan->chan_id == dma_channel)
		ret = true;
	return ret;
}

static int arg_config_support(struct fh_audio_cfg_arg *cfg)
{
	int ret;

	ret = get_param_from_volume(cfg->volume);
	if (ret < 0)
		return -EINVAL;
	return 0;
}

static long fh_audio_ioctl(struct file *filp, unsigned int cmd,
	unsigned long arg)
{
	struct fh_audio_cfg_arg cfg;
	struct miscdevice *miscdev = filp->private_data;
	struct audio_dev *dev =
		container_of(miscdev, struct audio_dev, fh_audio_miscdev);
	struct fh_audio_cfg *audio_config = &dev->audio_config;
	int ret;
	int value, pid;
	int __user *p = (int __user *)arg;

	pid = current->tgid;
	switch (cmd) {
	case AC_INIT_CAPTURE_MEM:

		if (copy_from_user((void *)&cfg, (void __user *)arg,
				sizeof(struct fh_audio_cfg_arg))) {
			printk(KERN_ERR "copy err\n");
			return -EIO;
		}
		if (0 == arg_config_support(&cfg)) {
			if (down_trylock(&audio_config->sem_capture)) {
				pr_info("another thread is running capture.\n");
				return -EBUSY;
			}
			ret = init_audio(capture, audio_config, &cfg);
			if (ret) {
				up(&audio_config->sem_capture);
				return ret;
			}
		} else {
			return -EINVAL;
		}

		break;
	case AC_INIT_PLAYBACK_MEM:
		if (copy_from_user((void *)&cfg, (void __user *)arg,
			sizeof(struct fh_audio_cfg_arg))) {
			printk(KERN_ERR "copy err\n");
			return -EIO;
		}

		if (0 == arg_config_support(&cfg)) {
			if (down_trylock(&audio_config->sem_playback)) {
				printk(KERN_ERR "another thread is running playback.\n");
				return -EBUSY;
			}
			ret = init_audio(playback, audio_config, &cfg);
			if (ret) {
				up(&audio_config->sem_playback);
				return ret;
			}
		} else
			return -EINVAL;
		break;
	case AC_AI_EN:
		if (infor_record.record_pid != pid)
			return -EBUSY;
		return fh_i2s_start_capture(audio_config);
	case AC_AO_EN:
		if (infor_record.play_pid != pid)
			return -EBUSY;
		return fh_i2s_start_playback(audio_config);
	case AC_SET_VOL:
		if (infor_record.record_pid != pid)
			return -EBUSY;
		if (get_user(value, p))
			return -EFAULT;
		ret = get_param_from_volume(value);
		if (ret < 0)
			return -EINVAL;
		audio_param_store.input_volume = value;
		switch_input_volume(audio_param_store.input_volume);
		break;
	case AC_SET_INPUT_MODE:
		if (infor_record.record_pid != pid)
			return -EBUSY;
		if (get_user(value, p))
			return -EFAULT;
		if (value != mic_in && value != line_in)
			return -EINVAL;
		audio_param_store.input_io_type = value;
		switch_io_type(capture,
			audio_param_store.input_io_type);
		break;
	case AC_SET_OUTPUT_MODE:
		if (infor_record.play_pid != pid)
			return -EBUSY;
		if (get_user(value, p))
			return -EFAULT;
		if (value != speaker_out && value != line_out)
			return -EINVAL;
		switch_io_type(playback, value);
		break;
	case AC_AI_DISABLE:
		pr_info("[ac_driver]AC_AI_DISABLE\n");
		if (infor_record.record_pid != pid)
			return -EBUSY;
		fh_i2s_stop_capture(audio_config);
		pr_info(" AC_AI_DISABLE\n");
		break;
	case AC_AO_DISABLE:
		pr_info("[ac_driver]AC_AO_DISABLE\n");
		if (infor_record.play_pid != pid)
			return -EBUSY;
		fh_i2s_stop_playback(audio_config);
		pr_info(" AC_AO_DISABLE\n");
		break;
	case AC_AI_PAUSE:
		if (infor_record.record_pid != pid)
			return -EBUSY;
		printk(KERN_INFO "capture pause\n");
		audio_config->capture.state = STATE_PAUSE;
		/*enable rx*/
		writel(0, fh_i2s_module.vaddr + I2S_REG_IRER_OFFSET);
		/*rx en*/
		writel(0, fh_i2s_module.vaddr + I2S_REG_RER0_OFFSET);
		break;
	case AC_AI_RESUME:
		if (infor_record.record_pid != pid)
			return -EBUSY;
		printk(KERN_INFO "capture resume\n");
		audio_config->capture.state = STATE_RUN;
		/*enable rx*/
		writel(1, fh_i2s_module.vaddr + I2S_REG_IRER_OFFSET);
		/*reset rx dma*/
		writel(1, fh_i2s_module.vaddr + I2S_REG_RRXDMA_OFFSET);
		/*reset rx fifo*/
		writel(1, fh_i2s_module.vaddr + I2S_REG_RXFFR_OFFSET);
		/*rx en*/
		writel(1, fh_i2s_module.vaddr + I2S_REG_RER0_OFFSET);
		break;
	case AC_AO_PAUSE:
		if (infor_record.play_pid != pid)
			return -EBUSY;
		audio_config->playback.state = STATE_PAUSE;
		writel(0, fh_i2s_module.vaddr + I2S_REG_ITER_OFFSET);
		writel(0, fh_i2s_module.vaddr + I2S_REG_TER0_OFFSET);
		printk(KERN_INFO "playback pause\n");
		break;
	case AC_AO_RESUME:
		if (infor_record.play_pid != pid)
			return -EBUSY;
		printk(KERN_INFO "playback resume\n");
		audio_config->playback.state = STATE_RUN;
		/*enable tx*/
		writel(1, fh_i2s_module.vaddr + I2S_REG_ITER_OFFSET);
		/*reset tx dma*/
		writel(1, fh_i2s_module.vaddr + I2S_REG_RTXDMA_OFFSET);
		/*reset tx fifo*/
		writel(1, fh_i2s_module.vaddr + I2S_REG_TXFFR_OFFSET);
		/*tx en*/
		writel(1, fh_i2s_module.vaddr + I2S_REG_TER0_OFFSET);
		break;
	case AC_AI_SET_VOL:
	case AC_AO_SET_VOL:
	case AC_AI_MICIN_SET_VOL:
		return 0;

	default:
		return -ENOTTY;
	}
	return 0;
}

static int fh_audio_open(struct inode *ip, struct file *fp)
{

	fp->private_data = &fh_audio_dev.fh_audio_miscdev;
	return 0;
}

static u32 fh_audio_poll(struct file *filp, poll_table *wait)
{
	struct miscdevice *miscdev = filp->private_data;
	struct audio_dev
	*dev = container_of(miscdev, struct audio_dev,
		fh_audio_miscdev);
	struct fh_audio_cfg *audio_config = &dev->audio_config;
	u32 mask = 0;
	long avail;
	if (STATE_RUN == audio_config->capture.state) {
		poll_wait(filp, &audio_config->readqueue, wait);
		avail = avail_data_len(capture, audio_config);
		if (avail > audio_config->capture.cfg.period_bytes)
			mask |= POLLIN | POLLRDNORM;
	}
	if (STATE_RUN == audio_config->playback.state) {
		poll_wait(filp, &audio_config->writequeue, wait);
		avail = avail_data_len(playback, audio_config);
		if (avail > audio_config->playback.cfg.period_bytes)
			mask |= POLLOUT | POLLWRNORM;
	}
	return mask;
}

static int fh_audio_read(struct file *filp, char __user *buf,
		size_t len, loff_t *off)
{
	int ret;
	struct miscdevice *miscdev = filp->private_data;
	struct audio_dev *dev = container_of(miscdev, struct audio_dev,
			fh_audio_miscdev);
	struct fh_audio_cfg *audio_config = &dev->audio_config;
	int after, left;
	int pid, avail;

	pid = current->tgid;
	if (infor_record.record_pid != pid)
		return -EBUSY;

	/* wait for enough data*/
	ret = wait_event_interruptible_timeout(audio_config->readqueue,
		avail_data_len(capture, audio_config) >
		audio_config->capture.cfg.period_bytes,
		msecs_to_jiffies(5000));

	len -= len % (audio_config->capture.cfg.frame_bit / 8 * 2);

	avail = avail_data_len(capture, audio_config);
	if (avail > len)
		avail = len;
	after = avail + audio_config->capture.appl_ptr;
	if (after > audio_config->capture.size) {
		left = avail - (audio_config->capture.size
			- audio_config->capture.appl_ptr);
		ret = copy_to_user(buf, audio_config->capture.area
			+ audio_config->capture.appl_ptr,
			audio_config->capture.size
			- audio_config->capture.appl_ptr);
		ret = copy_to_user(buf+audio_config->capture.size
			-audio_config->capture.appl_ptr,
			audio_config->capture.area, left);
		spin_lock(&audio_config->capture.lock);
		audio_config->capture.appl_ptr = left;
		spin_unlock(&audio_config->capture.lock);
	} else {
		ret = copy_to_user(buf,
		audio_config->capture.area+audio_config->capture.appl_ptr,
			avail);
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
	struct audio_dev *dev = container_of(miscdev, struct audio_dev,
		fh_audio_miscdev);
	struct fh_audio_cfg *audio_config = &dev->audio_config;
	int ret;
	int after, left;
	int pid, avail;

	pid = current->tgid;
	if (infor_record.play_pid != pid)
		return -EBUSY;

	/* wait for enough data*/
	ret = wait_event_interruptible_timeout(audio_config->writequeue,
		avail_data_len(playback, audio_config) >
		audio_config->playback.cfg.period_bytes,
		msecs_to_jiffies(5000));

	len -= len % (audio_config->playback.cfg.frame_bit / 8 * 2);

	avail = avail_data_len(playback, audio_config);
	if (0 == avail)
		return 0;
	if (avail > len)
		avail = len;
	after = avail+audio_config->playback.appl_ptr;
	if (after > audio_config->playback.size) {
		left = avail - (audio_config->playback.size-
				audio_config->playback.appl_ptr);
		ret = copy_from_user(audio_config->playback.area
			+ audio_config->playback.appl_ptr,
			buf, audio_config->playback.size
			- audio_config->playback.appl_ptr);
		ret = copy_from_user(audio_config->playback.area,
			buf+audio_config->playback.size
			- audio_config->playback.appl_ptr, left);
		spin_lock(&audio_config->playback.lock);
		audio_config->playback.appl_ptr = left;
		spin_unlock(&audio_config->playback.lock);
	} else {
		ret = copy_from_user(audio_config->playback.area
			+ audio_config->playback.appl_ptr, buf, avail);
		spin_lock(&audio_config->playback.lock);
		audio_config->playback.appl_ptr += avail;
		spin_unlock(&audio_config->playback.lock);
	}

	return avail;
}

static irqreturn_t fh_audio_interrupt(int irq, void *dev_id)
{
	u32 interrupts;
	struct fh_audio_cfg *audio_config = &fh_audio_dev.audio_config;

	interrupts = readl(fh_i2s_module.vaddr + I2S_REG_ISR0_OFFSET);

	PRINT_AUDIO_DBG("interrupts: 0x%x\n", interrupts);

	if (interrupts & I2S_INTR_RX_AVAILABLE) {
		fh_i2s_stop_capture(audio_config);
		fh_i2s_start_capture(audio_config);
		PRINT_AUDIO_DBG("I2S_INTR_RX_AVAILABLE\n");
	}

	if (interrupts & I2S_INTR_RX_OVERRUN) {
		if (audio_config->capture.state == STATE_RUN) {
			fh_i2s_stop_capture(audio_config);
			fh_i2s_start_capture(audio_config);
		} else {
			/* reset rx fifo*/
		}
		PRINT_AUDIO_DBG("I2S_INTR_RX_OVERRUN\n");
	}

	if (interrupts & I2S_INTR_TX_AVAILABLE) {
		fh_i2s_stop_playback(audio_config);
		fh_i2s_start_playback(audio_config);
		PRINT_AUDIO_DBG("I2S_INTR_TX_AVAILABLE\n");
	}

	if (interrupts & I2S_INTR_TX_OVERRUN) {
		fh_i2s_stop_playback(audio_config);
		fh_i2s_start_playback(audio_config);
		PRINT_AUDIO_DBG("I2S_INTR_TX_OVERRUN\n");
	}

	return IRQ_HANDLED;
}

static const struct file_operations fh_i2s_fops = {
		.owner = THIS_MODULE,
		.llseek = no_llseek,
		.unlocked_ioctl = fh_audio_ioctl,
		.release = fh_audio_close,
		.open = fh_audio_open,
		.poll = fh_audio_poll,
		.read = fh_audio_read,
		.write = fh_audio_write,

};

#define FREQ_12_288_MHz		12288000
#define FREQ_11_2896_MHz	11289600

static int config_pllvco_freq(u32 freq)
{
	u32 addr = fh_i2s_module.acw_vaddr;
	if (addr == 0) {
		pr_err("acw_vaddr is null\n");
		return -EFAULT;
	}
	if (freq == FREQ_12_288_MHz) {
		/* config PLLVCO to 12.288M */
		writel(0x03, addr + 0xac);
		msleep(20);
		writel(0x07, addr + 0xb0);
		writel(0x0f, addr + 0xb4);
		writel(0x07, addr + 0xb8);
		writel(0x31, addr + 0xbc);
		writel(0x26, addr + 0xc0);
		writel(0xe9, addr + 0xc4);
	} else if (freq == FREQ_11_2896_MHz) {
		/* config PLLVCO to 11.2896M */
		writel(0x03, addr + 0xac);
		msleep(20);
		writel(0x06, addr + 0xb0);
		writel(0x1f, addr + 0xb4);
		writel(0x0f, addr + 0xb8);
		writel(0x86, addr + 0xbc);
		writel(0xc2, addr + 0xc0);
		writel(0x26, addr + 0xc4);
	} else {
		pr_err("unsupport freq %d\n", freq);
		return -EINVAL;
	}

	writel(0x00, addr + 0x9c);
	writel(0x21, addr + 0xc8);
	writel(0x07, addr + 0xcc);
	writel(0x05, addr + 0xd0);
	writel(0x02, addr + 0x8c);
	writel(0x10, addr + 0xa0);

	return 0;
}

static int config_i2s_clk(u32 rate, u32 bit)
{
	u32 ret, freq, div;
	struct fh_i2s_platform_data *pd = fh_audio_dev.plat_data;

	if (rate % 8000 == 0) {
		freq = FREQ_12_288_MHz;
	} else if (rate % 22050 == 0) {
		freq = FREQ_11_2896_MHz;
	} else {
		pr_err("unsupport rate %d\n", rate);
		return -EINVAL;
	}

	ret = config_pllvco_freq(freq);
	if (ret) {
		pr_err("config_pllvco_freq err %d\n", ret);
		return ret;
	}

	div = freq / rate / bit / 2;

	if (div % 4 != 0 || freq % (rate*bit*2) != 0) {
		pr_err("unsupport rate %d and bit %d\n", rate, bit);
		return -EINVAL;
	}

	pr_debug("freq %d, rate %d, bit %d, div %d\n", freq, rate, bit, div);

	if (pd->clk_config)
		ret = pd->clk_config((div - 1) << 8 | 0x03);
	else {
		pr_err("pd->clk_config is invalid");
		ret = -EINVAL;
	}

	ret = config_i2s_frame_bit(bit);
	if (ret) {
		pr_err("config_i2s_frame_bit err %d\n", ret);
		return ret;
	}

	return 0;
}

static int fh_i2s_drv_probe(struct platform_device *pdev)
{
	int ret = 0;
	int irq;
	u32 acw_virt_addr = 0;

	struct resource *res_mem =
			platform_get_resource(pdev, IORESOURCE_MEM, 0);
	struct resource *res_irq =
			platform_get_resource(pdev, IORESOURCE_IRQ, 0);

	fh_audio_dev.plat_data =
			(struct fh_i2s_platform_data *)pdev->dev.platform_data;

	if (res_mem) {
		fh_i2s_module.paddr = res_mem->start;
		fh_i2s_module.vaddr = devm_ioremap(&pdev->dev, res_mem->start,
			resource_size(res_mem));
	}

	if (!fh_audio_dev.plat_data) {
		dev_err(&pdev->dev, "plat_data not configured\n");
		ret = -EINVAL;
		goto out_return;
	}

	fh_audio_dev.channel_assign.capture_channel =
			fh_audio_dev.plat_data->dma_capture_channel;
	fh_audio_dev.channel_assign.playback_channel =
			fh_audio_dev.plat_data->dma_playback_channel;
	fh_audio_dev.dma_master = fh_audio_dev.plat_data->dma_master;

	if (!fh_i2s_module.vaddr || !fh_i2s_module.paddr) {
		dev_err(&pdev->dev, "get mem resource error\n");
		ret = -ENOMEM;
		goto out_return;
	}

	if (!res_irq) {
		pr_err("%s: getting resource failed"
				"cannot get IORESOURCE_IRQ\n", __func__);
		ret = -ENXIO;
		goto out_return;
	}

	spin_lock_init(&fh_i2s_module.lock);

	ret = misc_register(&fh_audio_dev.fh_audio_miscdev);

	if (ret)
		goto out_return;

	irq = res_irq->start;
	if (!irq) {
		dev_err(&pdev->dev, "%s: cannot get irq\n", __func__);
		ret = -ENXIO;
		goto out_return;
	}

	fh_i2s_module.irq = irq;

	ret = devm_request_irq(&pdev->dev, irq, fh_audio_interrupt, IRQF_SHARED,
			DEVICE_NAME, &fh_i2s_module);

	if (ret)
		goto out_return;

	acw_virt_addr = (unsigned int)devm_ioremap(&pdev->dev,
				ACW_REG_BASE, SZ_4K);
	if (!acw_virt_addr) {
		dev_err(&pdev->dev, "cannot get acw_virt_addr\n");
		ret = -ENOMEM;
		goto out_free_irq;
	}
	fh_i2s_module.acw_vaddr = acw_virt_addr;

	fh_i2s_module.clk = clk_get(NULL, "i2s_clk");
	if (IS_ERR(fh_i2s_module.clk)) {
		ret = PTR_ERR(fh_i2s_module.clk);
		goto out_free_irq;
	} else {
		clk_enable(fh_i2s_module.clk);
	}

	if (fh_audio_dev.plat_data->acodec_clk_name) {
		char *clk_name = fh_audio_dev.plat_data->acodec_clk_name;

		fh_i2s_module.acodec_clk = clk_get(NULL, clk_name);
		if (IS_ERR(fh_i2s_module.acodec_clk))
			dev_err(&pdev->dev, "cannot find clk: %s\n", clk_name);
		else
			clk_enable(fh_i2s_module.acodec_clk);
	}

	dev_info(&pdev->dev, "FH DW I2S Driver\n");
	return 0;

out_free_irq:
	devm_free_irq(&pdev->dev, irq, &fh_i2s_module);
out_return:
	dev_err(&pdev->dev, "%s failed\n", __func__);
	return ret;
}

static int fh_i2s_drv_remove(struct platform_device *pdev)
{
	misc_deregister(&fh_audio_dev.fh_audio_miscdev);

	devm_free_irq(&pdev->dev, fh_i2s_module.irq, &fh_i2s_module);

	if (fh_i2s_module.clk) {
		clk_disable(fh_i2s_module.clk);
		clk_put(fh_i2s_module.clk);
	}
	if (fh_i2s_module.acodec_clk) {
		clk_disable(fh_i2s_module.acodec_clk);
		clk_put(fh_i2s_module.acodec_clk);
	}
	dev_info(&pdev->dev, "FH DW I2S Driver Removed\n");
	return 0;
}

static struct platform_driver fh_audio_driver = {
	.probe = fh_i2s_drv_probe,
	.remove = fh_i2s_drv_remove,
	.driver = {
			.name = DEVICE_NAME,
			.owner = THIS_MODULE,
	}
};

static int audio_prealloc_dma_buffer(struct fh_audio_cfg *audio_config)
{
	int pg;
	gfp_t gfp_flags;
	pg = get_order(AUDIO_DMA_PREALLOC_SIZE);
	gfp_flags = GFP_KERNEL | __GFP_COMP | __GFP_NORETRY | __GFP_NOWARN;
	audio_config->capture.dev.coherent_dma_mask = DMA_BIT_MASK(32);
	audio_config->capture.area = dma_alloc_coherent(
			&audio_config->capture.dev, PAGE_SIZE << pg,
			&audio_config->capture.addr, gfp_flags);
	if (!audio_config->capture.area) {
		printk(KERN_ERR"no enough mem for capture  buffer alloc\n");
		return -1;
	}
	audio_config->playback.dev.coherent_dma_mask = DMA_BIT_MASK(32);
	audio_config->playback.area = dma_alloc_coherent(
			&audio_config->playback.dev, PAGE_SIZE << pg,
			&audio_config->playback.addr, gfp_flags);
	if (!audio_config->playback.area) {
		printk(KERN_ERR"no enough mem for  playback buffer alloc\n");
		return -1;
	}
	return 0;
}

static void audio_free_prealloc_dma_buffer(struct fh_audio_cfg *audio_config)
{
	int pg;
	pg = get_order(AUDIO_DMA_PREALLOC_SIZE);
	dma_free_coherent(&audio_config->capture.dev, PAGE_SIZE << pg,
		audio_config->capture.area, audio_config->capture.addr);
	dma_free_coherent(&audio_config->playback.dev, PAGE_SIZE << pg,
		audio_config->playback.area,
		audio_config->playback.addr);
}

static void init_audio_mutex(struct fh_audio_cfg *audio_config)
{
	sema_init(&audio_config->sem_capture, 1);
	sema_init(&audio_config->sem_playback, 1);
}

static int audio_request_dma_channel(void)
{
	dma_cap_mask_t mask;

	dma_rx_transfer = kzalloc(sizeof(struct fh_dma_chan), GFP_KERNEL);
	if (!dma_rx_transfer) {
		printk(KERN_ERR"alloc  dma_rx_transfer failed\n");
		goto mem_fail;
	}

	dma_tx_transfer = kzalloc(sizeof(struct fh_dma_chan), GFP_KERNEL);
	if (!dma_tx_transfer) {
		printk(KERN_ERR"alloc  dma_tx_transfer failed\n");
		goto mem_fail;
	}

	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);
	dma_rx_transfer->chan = dma_request_channel(mask,
			fh_i2s_dma_chan_filter,
			&fh_audio_dev.channel_assign.capture_channel);
	if (!dma_rx_transfer->chan) {
		printk(KERN_ERR"request audio rx dma channel failedx\n");
		goto channel_fail;
	}

	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);
	dma_tx_transfer->chan = dma_request_channel(mask,
			fh_i2s_dma_chan_filter,
			&fh_audio_dev.channel_assign.playback_channel);
	if (!dma_tx_transfer->chan) {
		printk(KERN_ERR"request tx dma channel failed\n");
		goto channel_fail;
	}

	return 0;
channel_fail:
	if (dma_rx_transfer->chan) {
		dma_release_channel(dma_rx_transfer->chan);
		dma_rx_transfer->chan = NULL;
	}
	if (dma_tx_transfer->chan) {
		dma_release_channel(dma_tx_transfer->chan);
		dma_tx_transfer->chan = NULL;
	}

mem_fail:
	if (dma_rx_transfer != NULL) {
		kfree(dma_rx_transfer);
		dma_rx_transfer = NULL;
	}
	if (dma_tx_transfer != NULL) {
		kfree(dma_tx_transfer);
		dma_tx_transfer = NULL;
	}

	return -EFAULT;
}

static void audio_release_dma_channel(void)
{
	/*release audio tx dma channel*/
	if (dma_tx_transfer != NULL) {
		if (dma_tx_transfer->chan) {
			dma_release_channel(dma_tx_transfer->chan);
			dma_tx_transfer->chan = NULL;
		}
		kfree(dma_tx_transfer);
		dma_tx_transfer = NULL;
	}

	/*release audio rx dma channel*/
	if (dma_rx_transfer != NULL) {
		if (dma_rx_transfer->chan) {
			dma_release_channel(dma_rx_transfer->chan);
			dma_rx_transfer->chan = NULL;
		}

		kfree(dma_rx_transfer);
		dma_rx_transfer = NULL;
	}
}

static int __init fh_audio_init(void)
{
	int ret = 0;

	create_proc();
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
	remove_proc();
	pr_err("%s failed\n", __func__);
	return ret;
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

MODULE_AUTHOR("Fullhan");
MODULE_DESCRIPTION("FH_DW_I2S");
MODULE_LICENSE("GPL");

/****************************debug proc*****************************/
#include <linux/proc_fs.h>

#define PROC_NAME "driver/"DEVICE_NAME
static struct proc_dir_entry *proc_file;

static void *v_seq_start(struct seq_file *s, loff_t *pos)
{
	static unsigned long counter;
	if (*pos == 0)
		return &counter;
	else {
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

	if (fh_i2s_module.vaddr == NULL)
		seq_printf(sfile, "reg is null\n");
	else {
		for (i = 0; i <= 0x20; i += 4) {
			data = readl(fh_i2s_module.vaddr + i);
			seq_printf(sfile, "0x%02x reg = 0x%x\n", i, data);
		}
	}

	return 0;
}

static const struct seq_operations fh_dwi2s_seq_ops = {
	.start = v_seq_start,
	.next = v_seq_next,
	.stop = v_seq_stop,
	.show = v_seq_show
};

static int proc_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &fh_dwi2s_seq_ops);
}

static const struct file_operations fh_dwi2s_proc_ops = {
	.owner = THIS_MODULE,
	.open = proc_open,
	.read = seq_read,
};

static void create_proc(void)
{
	proc_file = proc_create(PROC_NAME, 0644, NULL, &fh_dwi2s_proc_ops);

	if (proc_file == NULL)
		pr_err("%s: ERROR: %s proc file create failed",
			   __func__, DEVICE_NAME);

}

static void remove_proc(void)
{
	remove_proc_entry(PROC_NAME, NULL);
}
