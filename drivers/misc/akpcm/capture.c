/*
 *  pcm capture for anyka chip
 *  Copyright (c) by Anyka, Inc.
 *  Create by huang_haitao 2018-01-31
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#include <linux/platform_device.h>
#include <linux/fs.h>
#include <mach/akpcmL0.h>
#include <linux/dma-mapping.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/hardirq.h>

#include "aec.h"
#include "playback.h"
#include "capture.h"

#define CAPTURE_READ_DEBUG    (0)
#define CAPTURE_ISR_DEBUG     (0)

/*
 * FULL_THRESTHOLD -
 * is full state for the buffer
 */
#define FULL_THRESTHOLD(rt)		(rt->cfg.period_bytes)

/*default arguments*/
static struct akpcm_features akpcm_capture_hardware = {
	.sample_bits      = AKPCM_SMPL_BIT_U8 | AKPCM_SMPL_BIT_U16,
	.rates            = USE_RATE,
	.rate_min         = USE_RATE_MIN,
	.rate_max         = USE_RATE_MAX,
	.channels_min     = 1,
	.channels_max     = 1,
	.period_bytes_min = CAPT_PERIOD_BYTES_MIN,
	.period_bytes_max = CAPT_PERIOD_BYTES_MAX,
	.periods_min      = CAPT_PERIODS_MIN,
	.periods_max      = CAPT_PERIODS_MAX,
	.hp_gain_max      = HEADPHONE_GAIN_MAX,
	.li_gain_max      = LINEIN_GAIN_MAX,
	.mic_gain_max     = MIC_GAIN_MAX,
	.play_dev         = PLAYDEV_MSK, /* HP or(and) LO */
	.cptr_dev         = CPTRDEV_MSK, /* MIC or(and) LI */
};

/*
 * lock_capture_io -
 * lock capture device
 *
 * @pcm:		pointer to pcm device
 */
static inline void lock_capture_io(struct akpcm *pcm)
{
	mutex_lock(&pcm->io_lock);
}

/*
 * unlock_capture_io -
 * unlock capture device
 *
 * @pcm:		pointer to pcm device
 */
static inline void unlock_capture_io(struct akpcm *pcm)
{
	mutex_unlock(&pcm->io_lock);
}

/*
 * clear_capture_ptr -
 * clear all cache for capturer
 *
 * @rt:			pointer to runtime
 */
static inline void clear_capture_ptr(struct akpcm_runtime *rt)
{
	if (!in_interrupt())
		spin_lock_irq(&rt->ptr_lock);
	/*all ptr is NULL means cache for capture is empty*/
	rt->hw_ptr = 0;
	rt->app_ptr = 0;
	rt->aec_ptr = 0;
	if (!in_interrupt())
		spin_unlock_irq(&rt->ptr_lock);
}

/*
 * check_capture_status -
 * check capture current status
 *
 * @pcm:		pointer to pcm device
 */
static inline int check_capture_status(struct akpcm *pcm)
{
    int ret = 0;
    unsigned long flag = (pcm->capture_sm_flag & 3);

    switch (flag) {
	case 0:
		ak_pcm_info_ex("ERR! capture device isn't open!");
		ret = -EPERM;/* Operation not permitted */
		break;
	case 1:
		ak_pcm_info_ex("ERR! capture close is operating!");
		ret = -EPERM;/* Operation not permitted */
		break;
	case 2:
		ak_pcm_info_ex("ERR! in capture device read!");
		ret = -EPERM;/* Operation not permitted */
		break;
	case 3:
		break;
	default:
		ak_pcm_info_ex("ERR! in capture device read!");
		ret = -EPERM;/* Operation not permitted */
		break;
	}

	return ret;
}

/*
 * put_user_data -
 * put capture data from here to user space
 *
 * @rt:			pointer to runtime
 * @buf:		pointer to user space buffer
 * @count:		bytes of buffer
 */
static inline int put_user_data(struct akpcm_runtime *rt,
                    char __user *buf, size_t count)
{
    unsigned int app_pos = rt->app_ptr;
    unsigned int buf_bytes = rt->buffer_bytes;
	unsigned int app_off = (app_pos % buf_bytes);
    unsigned char  *vaddr = rt->dma_area;

    if (buf_bytes >= (app_off + count)) {
		if (copy_to_user(buf, vaddr+app_off, count)) {
			return -EFAULT;
		}
	} else {
		unsigned int fist_bytes = buf_bytes - app_off;

		if (copy_to_user(buf, vaddr+app_off, fist_bytes)) {
			return -EFAULT;
		}
		if (copy_to_user(buf+fist_bytes, vaddr, count-fist_bytes)) {
			return -EFAULT;
		}
	}

	return 0;
}

/*
 * udpate_capture_app_ptr -
 * update capture application ptr when application had read out.
 *
 * @rt:			pointer to runtime
 * @pre_app_pos:	old app ptr
 * @cur_app_pos:	new app ptr
 */
static inline void update_capture_app_ptr(struct akpcm_runtime *rt,
                    unsigned int pre_app_pos, unsigned int cur_app_pos)
{
	spin_lock_irq(&rt->ptr_lock);
#if 0
	if (pre_app_pos != rt->app_ptr) {
		/* app_ptr was updated by capture_isr */
		printk("U");
		rt->app_ptr = max(rt->app_ptr, cur_app_pos);
	} else {
		rt->app_ptr = cur_app_pos;
	}
#else
	if (cur_app_pos <= rt->aec_ptr)
		rt->app_ptr = cur_app_pos;
	else
		rt->app_ptr = rt->aec_ptr;
#endif

	if(rt->app_ptr >= rt->boundary)
		rt->app_ptr -= rt->boundary;

	spin_unlock_irq(&rt->ptr_lock);
}

/*
 * get_valid_bytes -
 * capture helper routine
 * get how many data ready to upper layer in capture buffer
 *
 * @rt:			pointer to runtime
 */
static inline unsigned int get_valid_bytes(struct akpcm_runtime *rt)
{
	unsigned int valid_bytes;
	unsigned int aec_pos = rt->aec_ptr;
	unsigned int app_pos = rt->app_ptr;
	unsigned int boundary = rt->boundary;

	/* calculate pend_data & free_space */
	if (aec_pos >= app_pos) {
		valid_bytes = aec_pos - app_pos;
	} else {
		valid_bytes = boundary - app_pos + aec_pos;
	}

#ifdef DEBUG_BYTES
	if (valid_bytes > rt->buffer_bytes) {
		ak_pcm_err("valid_bytes:%u; aec_pos:%u, app_pos:%u, hw_ptr:%u",
			valid_bytes, aec_pos, app_pos, rt->hw_ptr);
	}
#endif

	return valid_bytes;
}

/*
 * get_capture_free_bytes -
 * capture helper routine
 * get how many free room in capture buffer
 *
 * @rt:			pointer to runtime
 */
static inline unsigned int get_capture_free_bytes(struct akpcm_runtime *rt)
{
	unsigned int used_bytes;
	unsigned int hw_pos = rt->hw_ptr;
	unsigned int app_pos = rt->app_ptr;
	unsigned int boundary = rt->boundary;

	/* calculate pend_data & free_space */
	if (hw_pos >= app_pos) {
		used_bytes = hw_pos - app_pos;
	} else {
		used_bytes = boundary - app_pos + hw_pos;
	}

#ifdef DEBUG_BYTES
	if (rt->buffer_bytes < used_bytes) {
		ak_pcm_err_ex("buffer_bytes:%u, used_bytes:%u",
		    rt->buffer_bytes, used_bytes);
	}
#endif

	return (rt->buffer_bytes - used_bytes);
}

/*
 * clean_abnormal_aec_ptr -
 * correct wrong aec pointer
 * called in adc interrupt
 */
static inline void clean_abnormal_aec_ptr(void)
{
	/* ADC: app->aec->hw */
	/* DAC: aec->hw->app */

	struct akpcm *pcm = get_akpcm_ptr();
	struct akpcm_runtime *cptr_rt = pcm->cptr_rt;
	struct akpcm_runtime *play_rt = pcm->play_rt;
	unsigned int cptr_free_bytes = get_capture_free_bytes(cptr_rt);
	unsigned int cptr_prd_bytes = cptr_rt->cfg.period_bytes;

	int dac_working = has_playback_stream(pcm);
	int dac_aec_abn = dac_working ? check_playback_aec_ptr() : 0;
	int adc_aec_abn = cptr_rt->app_ptr > cptr_rt->aec_ptr;
	int adc_notfree_space = 
		cptr_free_bytes <= (FULL_THRESTHOLD(cptr_rt) + cptr_prd_bytes);
	int adc_aec_slow = (adc_notfree_space && !adc_aec_abn)
			? (cptr_rt->aec_ptr - cptr_rt->app_ptr <= cptr_prd_bytes) : 0;

	if ((!pcm->enable_aec)) {
			if (adc_aec_abn || adc_aec_slow) {
				cptr_rt->aec_ptr = cptr_rt->hw_ptr;
				pr_debug("cptr aec_ptr abn");
			}
	} else if (dac_aec_abn || adc_aec_abn || adc_aec_slow) {
		if (cptr_rt->hw_ptr < cptr_rt->aec_ptr
				|| play_rt->hw_ptr < play_rt->aec_ptr) {
			cptr_rt->aec_ptr = cptr_rt->hw_ptr;// - AEC_FRAME_BYTES;
			play_rt->aec_ptr = play_rt->hw_ptr;// - FAR_FRAME_SIZE;

			pr_debug("dac:%d, cpapp:%d, cpaecslow:%d, cphw:%d,"\
				   " cpae:%d, pyhw:%d, pyae:%d\n",
					dac_aec_abn, adc_aec_abn, adc_aec_slow,
					cptr_rt->hw_ptr, cptr_rt->aec_ptr, play_rt->hw_ptr, play_rt->aec_ptr);
		} else {
			unsigned int cptr_aec_bytes = cptr_rt->hw_ptr - cptr_rt->aec_ptr;
			unsigned int play_aec_bytes = play_rt->hw_ptr - play_rt->aec_ptr;
			unsigned int off;

			if (cptr_aec_bytes >= (play_aec_bytes >> 1)) {
				off = play_aec_bytes >> 1;
			} else {
				off = cptr_aec_bytes;
			}

			if (off > 0) {
				cptr_rt->aec_ptr += off;
				play_rt->aec_ptr += off << 1;
			} else {
				cptr_rt->aec_ptr = cptr_rt->hw_ptr;// - AEC_FRAME_BYTES;
				play_rt->aec_ptr = play_rt->hw_ptr;// - FAR_FRAME_SIZE;
			}

#if 0
			printk(KERN_ERR "dac:%d, cpapp:%d, cpaecslow:%d, off:%d\n",
					dac_aec_abn, adc_aec_abn, adc_aec_slow, off);
#endif
		}
	}
}

/*
 * LininDet_wq_work -
 * worker for linein in/out detection
 *
 * @work:		the wait work
 */
void LininDet_wq_work(struct work_struct *work)
{
	struct akpcm *pcm = container_of(work, struct akpcm, LininWork.work);

	if (ak_gpio_getpin(pcm->linindet_gpio.pin) ==
			pcm->linindet_gpio.value) {
		/* linein is plugged in */
		pcm->capture_dectect |= CAPTURE_DET_LINEIN;
		if (pcm->dev_manual)
			return;
		pcm->cptrdev = CPTRDEV_LI;
		set_channel_source(pcm, MIXER_SRC_ADC2, SOURCE_ADC_LINEIN);
		pr_info("akpcm %s: linin on.\n", __func__);
	} else {
		pcm->capture_dectect &= ~CAPTURE_DET_LINEIN;
		if (pcm->dev_manual)
			return;
		pcm->cptrdev = CPTRDEV_MIC;
		set_channel_source(pcm, MIXER_SRC_ADC2, SOURCE_ADC_MIC);
		pr_info("akpcm %s: linin off.\n", __func__);
	}
}

/*
 * linindet_interrupt -
 * linein in/out irq handler
 *
 * @irq:			irq number of handler
 * @dev_id:			private data
 */
irqreturn_t linindet_interrupt(int irq, void *dev_id)
{
	struct akpcm *pcm = dev_id;

	if (ak_gpio_getpin(pcm->linindet_gpio.pin) ==
			pcm->linindet_gpio.value) {
		/* linein is plugged in */
		irq_set_irq_type(pcm->linindet_irq, pcm->lininoff_irqType);
		pr_info("akpcm %s: linin on.\n", __func__);
	} else {
		irq_set_irq_type(pcm->linindet_irq, pcm->lininon_irqType);
		pr_info("akpcm %s: linin off.\n", __func__);
	}

	schedule_delayed_work(&pcm->LininWork, msecs_to_jiffies(100));

	return IRQ_HANDLED;
}

/*
 * start_adc -
 * start the ADC for capture
 *
 * @pcm:		pointer to pcm device
 */
static void start_adc(struct akpcm *pcm)
{
	struct akpcm_runtime *rt = pcm->cptr_rt;
	unsigned int rt_channels = rt->cfg.channels;
	/* prerare for HW */
	int ADC2src = pcm->mixer_source[MIXER_SRC_ADC2];

	ak39_set_adc2_channels(rt_channels);
	ak39_codec_adc2_open();

	switch (ADC2src) {
	case SOURCE_ADC_MIC:
	    ak39_set_mic_power(1);
		ak39_set_adc2_in(SOURCE_ADC_MIC);
	    break;
	case SOURCE_ADC_LINEIN:
	    ak39_set_linein_power(1);
		ak39_set_adc2_in(SOURCE_ADC_LINEIN);
	    break;
	default:
	    break;
	}
}

/*
 * set_capture_timestamp -
 * set timestamp for capture data
 *
 * @timestamp:			to store the timestamp
 * @prd_bytes:			bytes of one transfer dma
 * @hw_off:				hardware pointer to the cache
 */
static void set_capture_timestamp(unsigned long *timestamp,
                unsigned int prd_bytes, unsigned int hw_off)
{
	int index = hw_off / prd_bytes;
	*(timestamp + index) = get_timestamp();
}

/*
 * get_hw_off -
 * get current hardware pointer
 *
 * @rt:				pointer to runtime
 */
static inline unsigned int get_hw_off(struct akpcm_runtime *rt)
{
    return (rt->hw_ptr % rt->buffer_bytes);
}

/*
 * check_rubbish_data -
 * check if have rubbish capture data
 *
 * @rt:				pointer to runtime
 */
static inline int check_rubbish_data(struct akpcm_runtime *rt)
{
    int first_rubbish = 0;

	// TODO: need fix
	// adc interrupt is not expected to raise too soon.
	// when this happen, the first L2 size of data is rubbish
	if (0 == rt->end_time)	{
	    int spend_us = 0;

		rt->end_time = sched_clock();
		spend_us = ak39_spend_us(rt->start_time, rt->end_time);

		/* less than 15ms is considered abnormal */
		if (spend_us < (15 * 1000)) {
			ak_pcm_info("adc first int too soon: %dus", spend_us);
			first_rubbish = 1;
		}
	}

	return first_rubbish;
}

/*
 * update_hw_ptr -
 * update hardware pointer
 *
 * @rt:				pointer to runtime
 */
static inline void update_hw_ptr(struct akpcm_runtime *rt)
{
    unsigned int prd_bytes = rt->cfg.period_bytes;

    rt->hw_ptr += prd_bytes;
	if(rt->hw_ptr >= rt->boundary)
		rt->hw_ptr -= rt->boundary;
}

/*
 * sync_dac -
 * sync dac startup.
 * if adc startup before dac, it is called
 *
 * @pcm:				pointer to pcm devic3
 */
static inline void sync_dac(struct akpcm *pcm)
{
    struct akpcm_runtime *rt = pcm->cptr_rt;
    u_int64_t end = 0;
	u_int64_t start = sched_clock();

	/* to start dac */
	ak_pcm_debug_ex("sync");
	playback_start_dma(pcm);
	end = sched_clock();
	/*do not show the message because it is in irq*/
	ak_pcm_debug_ex("adc ~ dac: %d(us)", ak39_spend_us(start, end));

	update_aec_status(pcm, AEC_STATUS_AEC_WORKING);
	wake_up_interruptible(&(pcm->aec_sync_wq));

	/* aec data starts from next period */
	rt->aec_ptr = rt->hw_ptr;
}

/*
 * capture_to_aec -
 * capture data is ready to process aec
 *
 * @pcm:				pointer to pcm devic3
 * @first_intr_rubbish:	rubbish data
 */
static void capture_to_aec(struct akpcm *pcm, int first_intr_rubbish)
{
    struct akpcm_runtime *rt = pcm->cptr_rt;
    unsigned int prd_bytes = rt->cfg.period_bytes;

    /* skip first intr rubbish */
	if (first_intr_rubbish)	{
		rt->aec_ptr += prd_bytes;
		if(rt->aec_ptr >= rt->boundary)
			rt->aec_ptr -= rt->boundary;
	}

#if 0
	/* check if has deposited too many aec data */
	if ((get_aec_bytes(rt) > pcm->cptr_rt->cfg.threshold)
		&& (0 == pcm->aec_high_prio)) {
		/* if so, call on tasklet to handle */
		pcm->aec_high_prio = 1;
		wake_up_interruptible(&(pcm->aec_prio_wq));
	}
#endif

	pcm->aec_data_triger = 1;
	wake_up_interruptible(&(pcm->aec_wq));
}

/*
 * handle_adc_buffer_full -
 * process the capture buffer is full
 *
 * @pcm:				pointer to pcm devic3
 * @free_bytes:			current free capture bytes
 */
static void handle_adc_buffer_full(struct akpcm *pcm, unsigned int free_bytes)
{
    struct akpcm_runtime *rt = pcm->cptr_rt;
    unsigned int prd_bytes = rt->cfg.period_bytes;

    /* aec data should not fill the adc buf */
	ak_pcm_assert(get_aec_bytes(rt) <= (rt->buffer_bytes - FULL_THRESTHOLD(rt)));//prd_bytes));
	if (pcm->enable_aec && free_bytes < FULL_THRESTHOLD(rt)) {
	    ak_pcm_info("warn: adc buffer full, ruin aec potentially");
		ak_pcm_info_ex("free_bytes=%u, prd_bytes=%u, valid_bytes=%u",
			free_bytes, prd_bytes, get_valid_bytes(rt));
	}

	/* causion: take care not to conflict with capture_read on app_ptr */
	spin_lock(&rt->ptr_lock); // unnecessary because it is in irq
	rt->app_ptr = rt->hw_ptr - (rt->buffer_bytes - FULL_THRESTHOLD(rt) - prd_bytes);
	if(rt->app_ptr >= rt->boundary)
		rt->app_ptr -= rt->boundary;

	spin_unlock(&rt->ptr_lock);
}

/*
 * capture_isr -
 * DMA transfer for capture
 *
 * @data:		private data
 */
void capture_isr(unsigned long data)
{
	struct akpcm *pcm = (struct akpcm *)data;
	struct akpcm_runtime *rt = pcm->cptr_rt;
	unsigned int prd_bytes = rt->cfg.period_bytes;
	unsigned int hw_off = 0;
	u8 id = pcm->L2BufID_ADC2;
	dma_addr_t phyaddr = rt->dma_addr;
	int valid_bytes_changed = 0;
	int first_intr_rubbish = 0;
	unsigned int free_bytes = 0;

#if CAPTURE_ISR_DEBUG
    ak_pcm_debug("CP+ ptr aec:%u, hw:%u, app:%u",
        rt->aec_ptr, rt->hw_ptr, rt->app_ptr);
#endif

	hw_off = get_hw_off(rt);
	set_capture_timestamp(pcm->timestamp, prd_bytes, hw_off);
	dma_sync_single_for_cpu(pcm->dev, phyaddr+hw_off, prd_bytes, DMA_FROM_DEVICE);

    update_hw_ptr(rt);
	hw_off = get_hw_off(rt);

    first_intr_rubbish = check_rubbish_data(rt);

	/* to start dac in case of aec */
	if (AEC_STATUS_SYNC_DAC == pcm->aec_status) {
	    sync_dac(pcm);
		valid_bytes_changed = 1;
	}

	if ((AEC_STATUS_AEC_WORKING == pcm->aec_status)
		|| (AEC_STATUS_NR_WORKING == pcm->aec_status)) {
		/* when AEC is working, aec pointer is managed by handle_aec_task. */
		capture_to_aec(pcm, first_intr_rubbish);
	} else {
		/* AEC is not working */
		rt->aec_ptr = rt->hw_ptr;
		valid_bytes_changed = 1;
	}

	if (has_capture_stream(pcm)) {
		/* capture stream is running */
		free_bytes = get_capture_free_bytes(rt);
		/* adc buf is full */
		if (free_bytes <= FULL_THRESTHOLD(rt)) {
		    handle_adc_buffer_full(pcm, free_bytes);
			valid_bytes_changed = 1;
		}

        /* next DMA */
		l2_combuf_dma(phyaddr+hw_off, id, prd_bytes, (tL2DMA_DIR)BUF2MEM, 1);
	} else {
		/* capture stream is not running, stop it */
		ak_pcm_debug_ex("pcm capture stream stopped");
		/* stop DMA */
		clear_bit(IO_BIT_DMA, &(pcm->inflag));
		complete(&(pcm->capt_completion));
		ak_pcm_debug_ex("completion capture");
	}

	clean_abnormal_aec_ptr();

	if (valid_bytes_changed)
		wake_up_interruptible(&(pcm->capt_wq));

#if CAPTURE_ISR_DEBUG
	ak_pcm_debugx("CP- ptr aec:%u, hw:%u, app:%u",
	    rt->aec_ptr, rt->hw_ptr, rt->app_ptr);
#endif
}

/*
 * capture_prepare -
 * prepare for capture(open ADC2, power on MIC).
 *
 * @pcm:		pointer to pcm device
 */
static int capture_prepare(struct akpcm *pcm)
{
	struct akpcm_runtime *rt = pcm->cptr_rt;
	unsigned char *ptr = rt->dma_area;
	unsigned int new_size = (rt->cfg.periods * rt->cfg.period_bytes);
	int i;

	ak_pcm_func("enter");

	/* allocate memory for loop-buffer */
	if((ptr) && (new_size != rt->buffer_bytes)){
		capture_pause(pcm);
		dma_unmap_single(pcm->dev, rt->dma_addr, rt->buffer_bytes, DMA_FROM_DEVICE);
		kfree(ptr);
		rt->dma_area = ptr = NULL;
	}

	if (!ptr) {
		ptr = kmalloc(new_size, GFP_KERNEL);
		if(!ptr){
			ak_pcm_debug("capture buffer alloc failed: 0x%x", new_size);
			return -ENOMEM;
		} else {
			dma_addr_t phyaddr;
			phyaddr = dma_map_single(pcm->dev, ptr, new_size, DMA_FROM_DEVICE);
			if (dma_mapping_error(pcm->dev, phyaddr))
				ak_pcm_err_ex("dma_map_single failed.");
			rt->dma_addr = phyaddr;
			rt->dma_area = ptr;
			ak_pcm_debug("capture buffer alloc: ptr=0x%p, new_size=0x%x",
			    ptr, new_size);
		}

		/* reset some parameters */
		clear_capture_ptr(rt);
		rt->buffer_bytes = new_size;
		for(i=0; i<64; i++){
			if((new_size>>i) == 0){
				break;
			}
		}
		rt->boundary = new_size << (32-i);
		ak_pcm_debug_ex("capture boundary: 0x%x", rt->boundary);
	}

	set_bit(STATUS_BIT_PREPARED, &(pcm->capture_sm_flag));
	aec_event_handler(pcm, AEC_EVENT_OPEN);

	ak_pcm_func_exx("OK");

	return 0;
}

/*
 * capture_start -
 * start operation for capture
 *
 * @pcm:		pointer to pcm device
 */
void capture_start(struct akpcm *pcm)
{
	struct akpcm_runtime *rt = pcm->cptr_rt;
	unsigned int prd_bytes = rt->cfg.period_bytes;
	dma_addr_t phyaddr = rt->dma_addr;
	u8 id = pcm->L2BufID_ADC2;

	ak_pcm_func("enter");

	clear_capture_ptr(rt);
	l2_clr_status(id);
	/* start tranferring from offset 0 */
	l2_combuf_dma(phyaddr, id, prd_bytes, (tL2DMA_DIR)BUF2MEM, 1);

	start_adc(pcm);
	rt->start_time = sched_clock();
	rt->end_time = 0;

	set_bit(IO_BIT_STREAM, &(pcm->inflag)); //set stream running flag
	set_bit(IO_BIT_DMA, &(pcm->inflag));    //set DMA running flag
}

/*
 * capture_start -
 * resume operation for capture
 *
 * @pcm:		pointer to pcm device
 */
static void capture_resume(struct akpcm *pcm)
{
	if(is_capture_working(pcm))
		return;

	ak_pcm_func("enter, aec_status=%d", pcm->aec_status);

	if (pcm->aec_status == AEC_STATUS_WAIT_ADC) {
	    /* capture_start is called in playback isr */
		aec_event_handler(pcm, AEC_EVENT_ADC_START);
	} else {
	    /* sync_adc should be in protect of io lock */
		ak_pcm_assert(pcm->aec_status != AEC_STATUS_SYNC_ADC);

		capture_start(pcm);
		aec_event_handler(pcm, AEC_EVENT_ADC_START);
	}
}

/*
 * capture_start -
 * pause operation for capture
 *
 * @pcm:		pointer to pcm device
 */
void capture_pause(struct akpcm *pcm)
{
    /* working until start capture DMA */
	if (!is_capture_working(pcm)) {
        ak_pcm_debug_ex("capture is not working now");
	    return;
	}

	ak_pcm_func("enter");

	/* clear capture stream running flag. STOP when current DMA finish */
	clear_bit(IO_BIT_STREAM, &(pcm->inflag));
	if (is_capture_working(pcm)) {
		wait_for_completion(&(pcm->capt_completion));
		ak_pcm_info_ex("wait capture completion ok");
	}

	ak39_codec_adc2_close();

	/* nowadays, aec is disabled when adc is stopped */
	aec_event_handler(pcm, AEC_EVENT_AEC_STOP);
	aec_set_param(pcm, pcm->app_flag);
	aec_event_handler(pcm, AEC_EVENT_ADC_STOP);
}

/*
 * set_capture_dev -
 * set device
 *
 * @pcm:		pointer to pcm device
 * @arg:		argument for device
 */
static int set_capture_dev(struct akpcm *pcm, unsigned long arg)
{
    unsigned int value;
    int ret = get_user(value, (int *)arg);
	if (ret) {
		ret = -ENOMEM;
		goto set_dev_end;
	}
	if((pcm->cptrdev & (~CPTRDEV_MSK)) != 0){
		ret = -EINVAL;
		goto set_dev_end;
	}


	switch (value) {
	case CPTRDEV_AUTO:
	    ak_pcm_debug_ex("IOC_SET_DEV, value=%d, auto", value);
		pcm->dev_manual = 0;
		if (pcm->capture_dectect & CAPTURE_DET_LINEIN) {
			pcm->cptrdev = CPTRDEV_LI;
			change_channel_source(pcm, MIXER_SRC_ADC2, SOURCE_ADC_LINEIN);
		} else {
			pcm->cptrdev = CPTRDEV_MIC;
			change_channel_source(pcm, MIXER_SRC_ADC2, SOURCE_ADC_MIC);
		}
		break;
	case CPTRDEV_MIC:
	case CPTRDEV_LI:
		pcm->dev_manual = 1;
		if (value == pcm->cptrdev){
			break;
		}
		pcm->cptrdev = value;
		if (value == CPTRDEV_LI) {
		    ak_pcm_debug_ex("IOC_SET_DEV, value=%d, linein", value);
			change_channel_source(pcm, MIXER_SRC_ADC2, SOURCE_ADC_LINEIN);
		} else {
		    ak_pcm_debug_ex("IOC_SET_DEV, value=%d, mic", value);
			change_channel_source(pcm, MIXER_SRC_ADC2, SOURCE_ADC_MIC);
		}
		break;
	default:
		ak_pcm_debug("akpcm set capture device(s) no defined, value=%d", value);
		ret = -EINVAL;
		break;
	}

set_dev_end:
    return ret;
}

/*
 * set_capture_gain -
 * set analog gain
 *
 * @pcm:		pointer to pcm device
 * @arg:		argument for gain
 */
static int set_capture_gain(struct akpcm *pcm, unsigned long arg)
{
    unsigned int addr;
	unsigned int value;
	int ret = 0;

    if (pcm->cptrdev == CPTRDEV_MIC) {
		addr = MIXER_VOL_MIC;
		ret = get_user(value, (int *)arg);
		if (ret)
		    goto set_gain_end;

		if (value < MIC_GAIN_MIN) {
			value = MIC_GAIN_MIN;
		} else if (value > MIC_GAIN_MAX) {
			value = MIC_GAIN_MAX;
		}
		ak_pcm_debug_ex("IOC_SET_GAIN, set MIC gain: %d", value);

		if (pcm->mixer_volume[addr] != value) {
			akpcm_set_mic_gain(value);
		}
		pcm->mixer_volume[addr] = value;
	}else{
		addr = MIXER_VOL_LI;
		ret = get_user(value, (int *)arg);
		if (ret)
		    goto set_gain_end;

		if (value < LINEIN_GAIN_MIN) {
			value = LINEIN_GAIN_MIN;
		} else if (value > LINEIN_GAIN_MAX) {
			value = LINEIN_GAIN_MAX;
		}
		ak_pcm_debug_ex("IOC_SET_GAIN, set LI gain: %d", value);

		if (pcm->mixer_volume[addr] != value) {
			ak39_set_linein_gain(value);
		}
		pcm->mixer_volume[addr] = value;
	}

set_gain_end:
    return ret;
}

/*
 * set_capture_param -
 * set parameters
 *
 * @pcm:		pointer to pcm device
 * @arg:		argument for parameters
 */
static int set_capture_param(struct akpcm *pcm, unsigned long arg)
{
    int ret = 0;
    struct akpcm_runtime *rt = pcm->cptr_rt;
    struct akpcm_pars *rt_cfg = &rt->cfg;

	if (copy_from_user(rt_cfg, (void __user *)arg, sizeof(struct akpcm_pars))) {
		ret = -EFAULT;
		goto set_param_end;
	}
	if (rt_cfg->periods > MAX_TIMESTAMP_CNT) {
		ak_pcm_info("periods too large");
		ret = -EINVAL;
		goto set_param_end;
	}

	if (rt_cfg->threshold <= rt_cfg->period_bytes)
		rt_cfg->threshold = rt_cfg->period_bytes * 3;

	pcm->capt_hw->actual_rate = ak39_codec_set_adc2_samplerate(rt->cfg.rate);

set_param_end:
    return ret;
}

/*
 * set_capture_source -
 * set source
 *
 * @pcm:		pointer to pcm device
 * @arg:		argument for source
 */
static int set_capture_source(struct akpcm *pcm, unsigned long arg)
{
	unsigned int value = 0;
    unsigned int addr = MIXER_SRC_ADC2;
	int ret = get_user(value, (int *)arg);
	if (ret)
	    return ret;

	ak_pcm_debug_ex("IOC_SET_SOURCES value=%d", value);
	if (value < SIGNAL_ADC_SRC_MUTE) {
		value = SIGNAL_ADC_SRC_MUTE;
	} else if (value > SIGNAL_ADC_SRC_MAX){
		value = SIGNAL_ADC_SRC_MAX;
	}
	change_channel_source(pcm, addr, value);

	return 0;
}

/*
 * set_nr_max -
 * set max nr
 *
 * @pcm:		pointer to pcm device
 * @arg:		argument for max nr
 */
static int set_nr_max(struct akpcm *pcm, unsigned long arg)
{
	unsigned int enable = 0;
	int ret = get_user(enable, (int *)arg);

	if (!ret) {
	    ak_pcm_debug("IOC_SET_NR_MAX enable=%d", enable);
	    if (pcm->aec_filter && pcm->cptr_rt->enable_agc) {
            aec_set_nr_max(pcm, enable);
    	}
	}

	return ret;
}

/*
 * set_ad_eq_attr -
 * set eq of adc
 *
 * @pcm:		pointer to pcm device
 * @arg:		argument for eq
 */
static int set_ad_eq_attr(struct akpcm *pcm, unsigned long arg)
{
	T_S32 temp[EQ_ARRAY_NUMBER];
	if (copy_from_user(temp, (void __user *)arg, sizeof(T_S32) * EQ_ARRAY_NUMBER)) {
		ak_pcm_info_ex("copy from user failed");
		return -EFAULT;
	}

	return aec_set_eq_params(pcm, temp);
}

/*
 * get_echo_attr -
 * get echo attr
 *
 * @pcm:		pointer to pcm device
 * @aec_param:	the buffer for read to
 */
static int get_echo_attr(struct akpcm *pcm, T_AEC_PARAMS *aec_param)
{
	return aec_get_echo_params(pcm, (T_U8 *)aec_param);
}

/*
 * set_echo_attr -
 * set echo attr
 *
 * @pcm:		pointer to pcm device
 * @aec_param:	the buffer to set
 */
static int set_echo_attr(struct akpcm *pcm, T_AEC_PARAMS *aec_param)
{
	return aec_set_echo_params(pcm, (T_U8 *)aec_param);
}

/*
 * capture_ioctl -
 * capture device file ops: ioctl
 *
 * @flip:			capture device file
 * @cmd:			command
 * @arg;			argument for the command
 */
long capture_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct akpcm *pcm = get_akpcm_ptr();
	struct akpcm_runtime *rt = pcm->cptr_rt;
	struct akpcm_features *feats;
	struct akpcm_pars *rt_cfg;
	unsigned int addr;
	unsigned int value;
	int ret = 0;
	unsigned long ul;
	T_AEC_PARAMS aec_param;

	lock_capture_io(pcm);

	/* commands */
	switch (cmd) {
	case IOC_PREPARE:
		ak_pcm_debug_ex("IOC_PREPARE");
		ret = capture_prepare(pcm);
		break;
	case IOC_SET_AEC:
		if (copy_from_user(&value, (void __user *)arg, sizeof(value))) {
			ret = -EFAULT;
			goto cap_ioc_end;
		}

        ak_pcm_debug_ex("IOC_SET_AEC value=%d", value);
        if (value) {
            set_bit(ECHO_BIT_AEC, &(pcm->app_flag));
        } else {
            clear_bit(ECHO_BIT_AEC, &(pcm->app_flag));
        }

        if (is_playback_working(pcm)) {
            ret = playback_pause(pcm);
        } else {
            ret = aec_set_param(pcm, pcm->app_flag);
        }
		break;
	case IOC_RESUME:
		ak_pcm_debug_ex("IOC_RESUME");
		if (copy_from_user(&value, (void __user *)arg, sizeof(value))) {
			ret = -EFAULT;
			goto cap_ioc_end;
		}

		ret = aec_set_param(pcm, value);
		capture_resume(pcm);
		break;
	case IOC_PAUSE:
		ak_pcm_debug_ex("IOC_PAUSE");
		capture_pause(pcm);
		break;
	case IOC_RSTBUF:
		ak_pcm_debug_ex("IOC_RSTBUF, reset buf");
		/* reset buffer */
	    clear_capture_ptr(rt);
		break;
		/* features */
	case IOC_GET_FEATS:
	    ak_pcm_debug_ex("IOC_GET_FEATS");
		feats = pcm->capt_hw;
		if (copy_to_user((void __user *)arg, feats,
		        sizeof(struct akpcm_features))) {
			ret = -EFAULT;
		}
		break;
		/* configures */
	case IOC_GET_PARS:
	    ak_pcm_debug_ex("IOC_GET_PARS");
		rt_cfg = &rt->cfg;
		if (copy_to_user((void __user *)arg, rt_cfg,
		        sizeof(struct akpcm_pars))) {
			ret = -EFAULT;
		}
		break;
	case IOC_SET_PARS:
		ak_pcm_debug_ex("IOC_SET_PARS");
        set_capture_param(pcm, arg);
		break;
		/* ---------- capture devices ------------------------------------ */
	case IOC_GET_DEV:
		value = pcm->cptrdev;
		ret = put_user(value, (int *)arg);
		break;
	case IOC_SET_DEV:
        ret = set_capture_dev(pcm, arg);
		break;
		/* ---------- sources ------------------------------------ */
	case IOC_GET_SOURCES:
		addr = MIXER_SRC_ADC2;
		value = pcm->mixer_source[addr];
		ret = put_user(value, (int *)arg);
		break;
	case IOC_SET_SOURCES:
        set_capture_source(pcm, arg);
		break;
		/* ---------- GAIN ------------------------------------ */
	case IOC_GET_GAIN:
		if (pcm->cptrdev == CPTRDEV_MIC) {
			addr = MIXER_VOL_MIC;
		}else{
			addr = MIXER_VOL_LI;
		}
		value = pcm->mixer_volume[addr];
		ret = put_user(value, (int *)arg);
		break;
	case IOC_SET_GAIN:
		set_capture_gain(pcm, arg);
		break;
	case IOC_GETTIMER:
		ul = rt->ts;
		if (copy_to_user((void __user *)arg, &ul, sizeof(unsigned long))) {
			ret = -EFAULT;
		}
		break;
	case IOC_GET_STATUS:
		value = is_capture_working(pcm);
		ret = put_user(value, (unsigned int *)arg);
		break;
	case IOC_GET_DATA_LENGTH:
		value = get_valid_bytes(rt);
		if (copy_to_user((void __user *)arg, &value, sizeof(unsigned int))) {
			ret = -EFAULT;
		}
		break;
	case IOC_SET_NR_MAX:
        set_nr_max(pcm, arg);
	    break;
	case IOC_SET_AEC_DUMP:
		get_user(value, (int *)arg);
		aec_set_dump_type(pcm, value);
		break;
	case IOC_SET_AD_EQ:
        ak_pcm_debug_ex("IOC_SET_AD_EQ");
		get_user(value, (int *)arg);
		aec_enable_eq(pcm, value);
		break;
	case IOC_SET_AD_EQ_ATTR:
        ak_pcm_debug_ex("IOC_SET_AD_EQ_ATTR");
        ret = set_ad_eq_attr(pcm, arg);
		break;
	case IOC_GET_ECHO_PARAM:
		ret = get_echo_attr(pcm, &aec_param);
		if (copy_to_user((void __user *)arg, &aec_param, sizeof(T_AEC_PARAMS))) {
			ret = -EFAULT;
		}
		break;
	case IOC_SET_ECHO_PARAM:
		if (copy_from_user(&aec_param, (void __user *)arg, sizeof(T_AEC_PARAMS))) {
			ret = -EFAULT;
			goto cap_ioc_end;
		}
		ret = set_echo_attr(pcm, &aec_param);
		break;
	default:
		ret = -ENOTTY;
		break;
	}

cap_ioc_end:
	unlock_capture_io(pcm);

	return ret;
}

/*
 * capture_read -
 * capture device file ops: read
 *
 * @flip:			capture device file
 * @buf:			buffer to store to
 * @count:			bytes of read
 * @f_pos:			current position of read
 */
ssize_t capture_read(struct file *filp, char __user *buf, size_t count,
                    loff_t *f_pos)
{
	struct akpcm *pcm = get_akpcm_ptr();
	struct akpcm_runtime *rt = pcm->cptr_rt;
	unsigned int buf_bytes = rt->buffer_bytes;
	unsigned int prd_bytes = rt->cfg.period_bytes;
	unsigned int app_pos = rt->app_ptr;
	unsigned int pre_app_pos = rt->app_ptr;
	unsigned int app_off = (app_pos % buf_bytes);
	unsigned long ts;

    if (check_capture_status(pcm)) {
        return -EPERM;
    }

#if CAPTURE_READ_DEBUG
    ak_pcm_func("R+");
    ak_pcm_debug("ptr hw:%u, app:%u, aec: %u",
        rt->hw_ptr, rt->app_ptr, rt->aec_ptr);
#endif

	/* calculate valid_data & free_space */
	if (wait_event_interruptible(pcm->capt_wq,
	    (get_valid_bytes(rt) >= count)) < 0) {
		return -ERESTARTSYS;
	}

	/* modify under protection */
	app_pos = rt->app_ptr;
	pre_app_pos = rt->app_ptr;
	app_off = (app_pos % buf_bytes);

    if (put_user_data(rt, buf, count)) {
        return -EFAULT;
    }

	app_pos += count;
	if(app_pos >= rt->boundary)
	    app_pos -= rt->boundary;

	update_capture_app_ptr(rt, pre_app_pos, app_pos);

	ts = pcm->timestamp[app_off / prd_bytes];
	ts += 1000 * (app_off % prd_bytes) / (pcm->capt_hw->actual_rate * 2);
	pcm->cptr_rt->ts = ts;

#if CAPTURE_READ_DEBUG
    ak_pcm_debug("app_pos: %u, valid_bytes: %u, count: %u",
        app_pos, get_valid_bytes(rt), count);
    ak_pcm_func_exx("R-");
#endif

	return count;
}

/*
 * capture_open -
 * open capture
 *
 * @inode:			pointer to capturer device node
 * @filp:			capturer device file
 */
int capture_open(struct inode *inode, struct file *filp)
{
	struct akpcm *pcm = get_akpcm_ptr();
	struct akpcm_runtime *rt = pcm->cptr_rt;

	ak_pcm_debug_ex("rt=0x%x", (int)rt);
	if (is_capture_opened(pcm)) {
		ak_pcm_debug_ex("ERR! capture device is already open!");
		return -EPERM; /* Operation not permitted */
	}

	/* allocate an L2 buffer */
	if (BUF_NULL == pcm->L2BufID_ADC2) {
		pcm->L2BufID_ADC2 = l2_alloc((l2_device_t)ADDR_ADC);
		if (BUF_NULL == pcm->L2BufID_ADC2){
			ak_pcm_debug_ex("Error! alloc L2 buffer for capture!");
			return -ENOMEM;
		}

		ak_pcm_debug_ex("L2 buffer ID for capture: %d", pcm->L2BufID_ADC2);
	}

	/* allocate memory for akpcm_runtime */
	rt = kmalloc(sizeof(struct akpcm_runtime), GFP_KERNEL);
	if (!rt) {
		l2_free((l2_device_t)ADDR_ADC);
		pcm->L2BufID_ADC2 = BUF_NULL;
		return -ENOMEM;
	}

	pcm->cptr_rt = rt;
	ak_pcm_debug("capture rt allocate: 0x%x", (int)(pcm->cptr_rt));

	/* others */
	rt->dma_area = NULL;
	rt->dma_addr = 0;
	clear_capture_ptr(rt);
	spin_lock_init(&rt->ptr_lock);
	mutex_init(&rt->lock);
	rt->cfg.rate = 16000;
	rt->cfg.channels = 2;
	rt->cfg.sample_bits = 16;
	rt->cfg.period_bytes = 16*1024;
	rt->cfg.periods = 16;
	rt->buffer_bytes = (rt->cfg.periods * rt->cfg.period_bytes);
	rt->enable_nr = 0;
	rt->enable_agc = 0;
	/*
	pcm->mixer_source[MIXER_SRC_ADC2] = SOURCE_MIC;
	pcm->cptrdev = CPTRDEV_MIC;
	*/

	pcm->capt_hw = &akpcm_capture_hardware;
	init_completion(&(pcm->capt_completion));

    l2_set_dma_callback(pcm->L2BufID_ADC2, capture_isr, (unsigned long)pcm);
	set_bit(STATUS_BIT_OPENED, &(pcm->capture_sm_flag));
	ak_pcm_debug_exx("OK");

	return 0;
}

/*
 * capture_close -
 * close capture
 *
 * @inode:			pointer to capturer device node
 * @filp:			capturer device file
 */
int capture_close(struct inode *inode, struct file *filp)
{
	struct akpcm *pcm = get_akpcm_ptr();
	struct akpcm_runtime *rt = pcm->cptr_rt;
	int hpsrc=0, losrc=0, ADC2src=0;

	ak_pcm_func("enter");

	if (!is_capture_opened(pcm)) {
		ak_pcm_debug_ex("ERR! capture device is not open!");
		return -EPERM; /* Operation not permitted */
	}

	lock_capture_io(pcm);

	capture_pause(pcm);

	hpsrc = pcm->mixer_source[MIXER_SRC_HP];
	losrc = pcm->mixer_source[MIXER_SRC_LO];
	ADC2src = pcm->mixer_source[MIXER_SRC_ADC2];
	if (SOURCE_ADC_MIC & ADC2src) {
		/* stop process when MIC is use as a capture device  */
		/* ??? only MIC will be use as a capture device ??? */
		/* ??? here ALL devices is checked. NEED TO BE FIXED ??? */
		/* ??? only make sure WORKING devices is not using it ??? */
		/* ??? don't check NOT-WORKING devices ??? */
		if ((0==(hpsrc&SOURCE_MIC)) && (0==(losrc&SOURCE_MIC))) {
			/* power off MIC, if no other device use it */
			ak39_set_mic_power(0);
		}
		ak39_set_adc2_in(0);
		/* ??? here ALL devices is checked. NEED TO BE FIXED ??? */
		/* ??? only make sure WORKING devices is not using it ??? */
		/* ??? don't check NOT-WORKING devices ??? */
		if ((0==hpsrc) && (!((SOURCE_DAC|SOURCE_MIC) & losrc))) {
			/* power off VCM_REF, if no other device use it */
			/* ??? onlye make sure WORKING devices is not using it ??? */
			/* ??? don't check NOT-WORKING devices ??? */
			//ak39_set_vcm_ref_power(0);
		}
	}

	/* The following codes is added by panqihe 2014-06-12 */
	if (SOURCE_ADC_LINEIN & ADC2src) {
		/* stop process when LINE_IN is use as a capture device  */
		// TODO ...
	}
	/* free loop-buffer memory */
	clear_bit(STATUS_BIT_PREPARED, &(pcm->capture_sm_flag));

	if (rt && rt->dma_area) {
		dma_unmap_single(pcm->dev, rt->dma_addr, rt->buffer_bytes, DMA_FROM_DEVICE);
		kfree(rt->dma_area);
		rt->dma_area = NULL;
	}

	/* free L2 buffers */
	if (BUF_NULL != pcm->L2BufID_ADC2) {
		l2_free((l2_device_t)ADDR_ADC);
		pcm->L2BufID_ADC2 = BUF_NULL;
	}

	unlock_capture_io(pcm);

	if (rt) {
		kfree(rt);
		pcm->cptr_rt = NULL;
	}
	clear_bit(STATUS_BIT_OPENED, &(pcm->capture_sm_flag));
	pcm->app_flag = 0;

    ak_pcm_debug_exx("OK");

	return 0;
}
