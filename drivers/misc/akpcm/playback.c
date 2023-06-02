/*
 *  pcm for anyka chip
 *  Copyright (c) by Anyka, Inc.
 *  Create by panqihe 2014-06-09
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

#define PLAYBACK_WRITE_DEBUG    (0)
#define PLAYBACK_ISR_DEBUG      (0)

/*
 * FILL_PTHRESHOLD:		minimum bytes in buffer
 * WRITE_PTHRESHOLD:	minimum empty bytes space to write
 */
#define FILL_PTHRESHOLD(rt)		(rt->cfg.period_bytes)
#define WRITE_PTHRESHOLD(rt)	(FILL_PTHRESHOLD(rt) + rt->cfg.period_bytes)

static unsigned int dac_app_data_end = 0;
static unsigned int dac_fill_times = 0;
static unsigned int dac_user_data_playing = 0;
static unsigned long long dac_elapse = 0;

/*default playback parameters*/
static struct akpcm_features akpcm_play_hardware = {
	.sample_bits      = AKPCM_SMPL_BIT_U16,
	.rates            = USE_RATE,
	.rate_min         = USE_RATE_MIN,
	.rate_max         = USE_RATE_MAX,
	.channels_min     = 2,
	.channels_max     = 2,
	.period_bytes_min = PLAY_PERIOD_BYTES_MIN,
	.period_bytes_max = PLAY_PERIOD_BYTES_MAX,
	.periods_min      = PLAY_PERIODS_MIN,
	.periods_max      = PLAY_PERIODS_MAX,
	.hp_gain_max      = HEADPHONE_GAIN_MAX,
	.li_gain_max      = LINEIN_GAIN_MAX,
	.mic_gain_max     = MIC_GAIN_MAX,
	.play_dev         = PLAYDEV_MSK, /* HP or(and) LO */
	.cptr_dev         = CPTRDEV_MSK, /* MIC or(and) LI */
};

/*
 * lock_playback_io -
 * lock playback device
 *
 * @pcm:		pointer to pcm device
 */
static inline void lock_playback_io(struct akpcm *pcm)
{
	mutex_lock(&pcm->io_lock);
}

/*
 * unlock_playback_io -
 * unlock playback device
 *
 * @pcm:		pointer to pcm device
 */
static inline void unlock_playback_io(struct akpcm *pcm)
{
	mutex_unlock(&pcm->io_lock);
}

/*
 * clear_playback_ptr -
 * to empty playbakc buffer
 *
 * @rt:		pointer to runtime
 */
static inline void clear_playback_ptr(struct akpcm_runtime *rt)
{
	/* ADC: app->aec->hw */
	/* DAC: aec->hw->app */

	unsigned int app_pos = rt->app_ptr;
	unsigned int aec_pos = rt->aec_ptr;
	unsigned int hw_pos = rt->hw_ptr;
	unsigned int prd_bytes = rt->cfg.period_bytes;
	unsigned int rsv_bytes = prd_bytes;
	struct akpcm *pcm = get_akpcm_ptr();

	if (!in_interrupt()) {
		//printk(KERN_ERR "CPP1\n");
		spin_lock_irq(&rt->ptr_lock);
	}

	if (!has_playback_stream(pcm)) {
		rt->hw_ptr = 0;
		rt->app_ptr = 0;
		rt->aec_ptr = 0;
	} else if (app_pos != aec_pos) {
		app_pos = hw_pos + rsv_bytes;
		if(app_pos >= rt->boundary)
			app_pos -= rt->boundary;

		rt->app_ptr = app_pos;
	}


	if (!in_interrupt()) {
		//printk(KERN_ERR "CPP2\n");
		spin_unlock_irq(&rt->ptr_lock);
	}

	//printk(KERN_ERR "bd:%u,aec:%u,hw:%u,app:%u\n",rt->boundary,rt->aec_ptr,rt->hw_ptr,rt->app_ptr);
}

/*
 * frames_to_bytes -
 * one frames data transfer to bytes size
 *
 * @rt:		pointer to runtime
 * @size:	bytes size of one frame
 */
static inline unsigned int frames_to_bytes(struct akpcm_runtime *rt,
                                unsigned int size)
{
	unsigned int frame_bits = rt->cfg.channels * rt->cfg.sample_bits;
	return (size * frame_bits / 8);
}

/*
 * check_playback_status -
 * check playback current status
 *
 * @pcm:		pointer to pcm device
 */
static inline int check_playback_status(struct akpcm *pcm)
{
    int ret = 0;
    unsigned long flag = (pcm->playback_sm_flag & 3);

    switch (flag) {
	case 0:
		ak_pcm_err("plackback device isn't open!");
		ret = -EPERM;/* Operation not permitted */
		break;
	case 1:
		ak_pcm_err("plackback opened but not prepare!");
		ret = -EPERM;/* Operation not permitted */
		break;
	case 2:
		ak_pcm_err("plackback just prepared!");
		ret = -EPERM;/* Operation not permitted */
		break;
	case 3:
		break;
	default:
		ak_pcm_err("plackback unknow status!");
		ret = -EPERM;/* Operation not permitted */
		break;
	}

	return ret;
}

/*
 * get_user_data -
 * get playback data to here from user space
 *
 * @rt:			pointer to runtime
 * @buf:		pointer to user space buffer
 * @count:		bytes of buffer
 */
static int get_user_data(struct akpcm *pcm, const char __user *buf,
                        size_t count, unsigned int *handle_pos)
{
	struct akpcm_runtime *rt = pcm->play_rt;
	unsigned int buf_bytes = rt->buffer_bytes;
	unsigned int app_pos = *handle_pos;
	unsigned int app_off = (app_pos % buf_bytes);
	unsigned char *vaddr = rt->dma_area;
	int frame_size = FAR_FRAME_SIZE;
	size_t left = count;

    while (left > 0) {
		size_t copy_size = frame_size - pcm->far_frame_off;
		copy_size = min(copy_size, left);

		if (copy_from_user(pcm->far_frame_buf + pcm->far_frame_off,
		        buf, copy_size)){
			ak_pcm_err_ex("akpcm write all error");
			return -EFAULT;
		}

		left -= copy_size;
		buf += copy_size;
		pcm->far_frame_off += copy_size;

		if (frame_size == pcm->far_frame_off) {
			void *playback_buf = (void *)pcm->far_frame_buf;

			/*
			 * NR/AGC far frame
			 * a frame is ready, then send data to noise reduction
			 */
			if (aec_need_playback(pcm) || rt->enable_nr) {
				/* convert to mono format */
				stereo_to_mono(playback_buf, playback_buf, frame_size);

				if (rt->enable_nr) {
					T_AEC_BUF *aec_bufs = &pcm->far_bufs;
					T_S32 ret_size;

					/* do NR and AGC */
					aec_bufs->buf_near = playback_buf;
					/* same addr to avoid copy in aeclib */
					aec_bufs->buf_out = playback_buf;
					aec_bufs->buf_far = AK_NULL;
					ret_size = AECLib_Control(pcm->far_filter, aec_bufs);
					if (ret_size != AEC_FRAME_BYTES)
						ak_pcm_debug("far processing failed: %ld", ret_size);
				}

				/* far's power should not too big to ruin aec */
				if (aec_need_playback(pcm))
					aec_set_da_volume(pcm, (T_S16*)playback_buf, AEC_FRAME_BYTES/2);

				/* covert back to stereo format */
				mono_to_stereo(vaddr+app_off, playback_buf, frame_size);
			} else {
				memcpy(vaddr+app_off, playback_buf, frame_size);
			}

			app_pos += frame_size;
			if(app_pos >= rt->boundary)
			    app_pos -= rt->boundary;

			app_off = (app_pos % buf_bytes);
			pcm->far_frame_off = 0;
		}
	}

	*handle_pos = app_pos;
	return 0;
}

/*
 * check_playback_aec_ptr -
 * check pointers of playback buffer
 */
int check_playback_aec_ptr(void)
{
	struct akpcm *pcm = get_akpcm_ptr();
	struct akpcm_runtime *rt = pcm->play_rt;

	if (!rt)
		return 0;

	if ((rt->app_ptr - rt->aec_ptr) > (rt->buffer_bytes - WRITE_PTHRESHOLD(rt))) {
		//printk(KERN_ERR "%u %u, buffer:%u,prd:%u",rt->app_ptr - rt->aec_ptr, rt->app_ptr - rt->hw_ptr, rt->buffer_bytes, rt->cfg.period_bytes);
		return 1;
	}
	return 0;
}

/*
 * udpate_playback_app_ptr -
 * update playback application ptr when application had write in.
 *
 * @pcm:			pointer to pcm device
 * @app_pos_save:	old app ptr
 * @app_pos:		new app ptr
 */
static inline void update_playback_app_ptr(struct akpcm *pcm,
                    unsigned int app_pos_save, unsigned int app_pos)
{
    struct akpcm_runtime *rt = pcm->play_rt;

	spin_lock_irq(&rt->ptr_lock);
    if (aec_need_playback(pcm)) {
		if (app_pos_save == rt->app_ptr) {
		    rt->app_ptr = app_pos;
		} else {
            /* app_ptr was updated by playback_isr */
			ak_pcm_info("V");
			rt->app_ptr = max(rt->app_ptr, app_pos);
		}
	} else {
		rt->app_ptr = app_pos;
	}

	if (!is_capture_working(pcm)
		 && check_playback_aec_ptr())
		rt->aec_ptr = rt->hw_ptr;// - FAR_FRAME_SIZE;

	spin_unlock_irq(&rt->ptr_lock);
}

/*
 * get_pend_bytes -
 * playback helper routine
 * get pend bytes to play
 * get how many data sent to DAC in playback buffer
 *
 * @rt:			pointer to runtime
 */
static inline unsigned int get_pend_bytes(struct akpcm_runtime *rt)
{
	unsigned int pend_bytes;
	unsigned int hw_pos = rt->hw_ptr;
	unsigned int app_pos = rt->app_ptr;
	unsigned int boundary = rt->boundary;

	/* calculate pend_data & free_space */
	if (app_pos >= hw_pos) {
		pend_bytes = app_pos - hw_pos;
	} else {
		pend_bytes = boundary - hw_pos + app_pos;
	}

	if (pend_bytes > rt->buffer_bytes) {
		ak_pcm_info_ex("pend_bytes:0x%x, buffer_bytes: 0x%x",
		    pend_bytes, rt->buffer_bytes);
		ak_pcm_debug_ex("pos hw=%u, app=%u", hw_pos, app_pos);
	}

	return pend_bytes;
}

/*
 * get_playback_free_bytes -
 * playback helper routine
 * get how many free room in playback buffer
 *
 * @rt:			pointer to runtime
 */
static inline unsigned int get_playback_free_bytes(struct akpcm_runtime *rt)
{
	unsigned int used_bytes;
	unsigned int aec_pos = rt->aec_ptr;
	unsigned int app_pos = rt->app_ptr;
	unsigned int boundary = rt->boundary;//0;

	/* calculate pend_data & free_space */
	if (app_pos >= aec_pos) {
		used_bytes = app_pos - aec_pos;
	} else {
		used_bytes = boundary - aec_pos + app_pos;
	}

#ifdef DEBUG_BYTES
	if (rt->buffer_bytes < used_bytes) {
		ak_pcm_err_ex("buffer_bytes:%u, used_bytes:%u",
			rt->buffer_bytes, used_bytes);
	    ak_pcm_err_ex("aec_pos:%u, app_pos:%u, hw_ptr:%u",
			aec_pos, app_pos, rt->hw_ptr);
	}
#endif

	return (rt->buffer_bytes - used_bytes);
}

/*
 * soft_fadeout -
 * software fadeout
 *
 * @ptr:		buffer
 * @num:		byte of buffer
 */
static int soft_fadeout(unsigned short *ptr, int num)
{
#define MAX_LEVEL (0x7FFF)
	unsigned short data = *(ptr - 1); //TODO: sample is 16 bits
	int step;
	int i;

	if (num <= 0)
		return 0;

	if (data > MAX_LEVEL) {
		step = (0xFFFF - data) / num;
		step = -step;

		if (!step)
			step = -1;
		for (i = 0; i < num; i++) {
			int tmp = *(ptr - 1);
			if (tmp - step <= 0xFFFF) {
				*ptr = tmp - step;
				*(ptr + 1) = tmp - step;
			} else {
				*ptr = 0xFFFF;
				*(ptr + 1) = 0xFFFF;
			}
			ptr += 2;
		}
	} else {
		step = data / num;
		if (!step)
			step = 1;
		for (i = 0; i < num; i++) {
			int tmp = *(ptr - 1);
			if (tmp - step >= 0) {
				*ptr = tmp - step;
				*(ptr + 1) = tmp - step;
			} else {
				*ptr = 0;
				*(ptr + 1) = 0;
			}
			ptr += 2;
		}
	}

	return num;
}

/*
 * check_rubbish_data -
 * check if have rubbish playback data
 *
 * @rt:				pointer to runtime
 */
static int check_rubbish_data(struct akpcm_runtime *rt)
{
    int first_rubbish = 0;

    // TODO: need fix
	// dac interrupt is not expected to raise too soon.
	// when this happen, the first L2 size of data is rubbish
	if (0 == rt->end_time)	{
	    int spend_us = 0;

		rt->end_time  = sched_clock();
		spend_us = ak39_spend_us(rt->start_time, rt->end_time);

		/* less than 10ms is considered abnormal */
		if (spend_us < (10*1000)) {
			ak_pcm_info("dac first int too soon: %dus", spend_us);
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
 * calc_dac_elapse -
 * calc elapse time to play
 *
 * @pcm:		pointer to pcm device
 */
static inline void calc_dac_elapse(struct akpcm *pcm)
{
	struct akpcm_runtime *rt = pcm->play_rt;
	unsigned int prd_bytes = rt->cfg.period_bytes;
	unsigned int actual_rate = pcm->play_hw->actual_rate;

    dac_elapse += (prd_bytes / frames_to_bytes(rt, 1) * 1000000 / actual_rate);
}

/*
 * update_playback_stream -
 * start next dma transfer to play
 *
 * @pcm:		pointer to pcm device
 */
static inline void update_playback_stream(struct akpcm *pcm)
{
    struct akpcm_runtime *rt = pcm->play_rt;
    unsigned char *vaddr = rt->dma_area;
	dma_addr_t phyaddr = rt->dma_addr;
	unsigned int prd_bytes = rt->cfg.period_bytes;
	unsigned int buf_bytes = rt->buffer_bytes;
	u8 id = pcm->L2BufID_DAC;
	unsigned int hw_off = (rt->hw_ptr % buf_bytes);

    unsigned int pend_bytes = get_pend_bytes(rt);
    if (pend_bytes < prd_bytes) {
		unsigned short *ptr = (unsigned short *)(vaddr + hw_off + pend_bytes);
		int num = (prd_bytes - pend_bytes) / 2 / 2;
		soft_fadeout(ptr, num);
	}

	dma_sync_single_for_device(pcm->dev, phyaddr+hw_off,
	    prd_bytes, DMA_TO_DEVICE);
    /* next DMA */
	l2_combuf_dma(phyaddr+hw_off, id, prd_bytes, (tL2DMA_DIR)MEM2BUF, 1);
}

/*
 * stop_playback -
 * stop play
 *
 * @pcm:		pointer to pcm device
 */
static inline void stop_playback(struct akpcm *pcm)
{
	struct akpcm_runtime *rt = pcm->play_rt;

	while(l2_get_status(pcm->L2BufID_DAC));

	ak39_dac_mute(); //by panqihe 2014-07-21
	complete(&(pcm->play_completion));

	//aec_event_handler(pcm, AEC_EVENT_DAC_STOP);

	pcm->far_frame_off = 0;
	clear_playback_ptr(rt);
	clear_bit(IO_BIT_DMA, &(pcm->outflag));
	dac_user_data_playing = 0;
}

/*
 * fill_data_to_da -
 * fill empty data to da buffer.
 * then the playback runnging always.
 *
 * @pcm:		pointer to pcm device
 * @pend_bytes:	pend bytes size to play
 */
static inline unsigned int fill_data_to_da(struct akpcm *pcm,
                                unsigned int pend_bytes)
{
    struct akpcm_runtime *rt = pcm->play_rt;
    unsigned char *vaddr = rt->dma_area;
	unsigned int prd_bytes = rt->cfg.period_bytes;
	unsigned int buf_bytes = rt->buffer_bytes;
	unsigned int hw_off = (rt->hw_ptr % buf_bytes);

    /* aec data should not fill the dac buf */
	ak_pcm_assert(get_aec_bytes(rt) <= (rt->buffer_bytes - FILL_PTHRESHOLD(rt)));

	/* fill a period of user data with 0s */
	memset(vaddr + hw_off + pend_bytes, 0, prd_bytes);//(prd_bytes - pend_bytes));
	pend_bytes = prd_bytes;

	/*
	 * forword app ptr
	 * causion: take care not to conflict with playback_write
	 */
	spin_lock(&rt->ptr_lock); //unnecessary because it is in irq
	rt->app_ptr = rt->hw_ptr + prd_bytes;
	if (rt->app_ptr >= rt->boundary)
		rt->app_ptr -= rt->boundary;

	if (!is_capture_working(pcm)
		 && check_playback_aec_ptr())
		rt->aec_ptr = rt->hw_ptr;// - FAR_FRAME_SIZE;

	spin_unlock(&rt->ptr_lock);

	pcm->aec_has_pend_data = 0;

	return pend_bytes;
}

/*
 * sync_adc -
 * sync adc after dac already startup
 *
 * @pcm:		pointer to pcm device
 */
static inline void sync_adc(struct akpcm *pcm)
{
    struct akpcm_runtime *rt = pcm->play_rt;
	unsigned int prd_bytes = rt->cfg.period_bytes;
    u_int64_t end = 0;
	u_int64_t start = sched_clock();

	// time sequence critical segment
	//   DO NOT add printing msg here until adc dma is actually working

	// This period of data will be fetched into dac approximately
	//  512 bytes (an L2 block) later, plus an unknown conversion latency.
	// Adc will be kicked off much sooner than the conversion latency.
	// I arrange the dac aec_ptr 512 bytes ahead of hw_ptr, so that
	//  adc aec_ptr will be align with dac with a moderate leading tolerance.
	rt->aec_ptr += prd_bytes-512;
	if(rt->aec_ptr >= rt->boundary)
		rt->aec_ptr = 0;

	/* to start ADC */
	ak_pcm_debug_ex("sync");
	capture_start(pcm);
	end = sched_clock();
	/*do not show the message because it is in irq*/
	ak_pcm_debug_ex("dac ~ adc: %dus", ak39_spend_us(start, end));

	update_aec_status(pcm, AEC_STATUS_AEC_WORKING);
	wake_up_interruptible(&(pcm->aec_sync_wq));
}

/*
 * playback_start_dac -
 * start the DAC for playback
 *
 * @pcm:		pointer to pcm device
 */
static void playback_start_dac(struct akpcm *pcm)
{
	int dstsrc = 0;

	ak39_codec_dac_open();

	if (PLAYDEV_HP == pcm->playdev) {
		/* output to HP */
		dstsrc = pcm->mixer_source[MIXER_SRC_HP];
		//ak_pcm_debug("hpsrc=%d", dstsrc);
		set_channel_source(pcm, MIXER_SRC_HP, dstsrc);
	} else {
		/* auto detect an outout device */
		if (pcm->play_dectect & SWITCH_DET_HEADPHONE) {
			dstsrc = pcm->mixer_source[MIXER_SRC_HP];
			set_channel_source(pcm, MIXER_SRC_HP, dstsrc);
		} else {
			set_channel_source(pcm, MIXER_SRC_HP, 0);
		}
	}

	pcm->IsOutputing = true;
}

/*
 * playback_stop_hw
 * stop output process for timer_stop_output
 *
 * @data:			private data
 */
void playback_stop_hw(unsigned long data)
{
	struct akpcm *pcm = (struct akpcm *)data;

	if (!has_playback_stream(pcm)) {
		ak39_codec_dac_close();
		if (PLAYDEV_HP == pcm->playdev) {
			/* output to HP */
			//ak_pcm_debug("close hp channel");
			set_channel_source(pcm, MIXER_SRC_HP, 0);
		} else {
			/* auto detect an outout device */
#if 0
			/* SP */
			if (pcm->play_dectect & SWITCH_DET_SPEAKER) {
				//ak_pcm_debug("close sp channel");
				set_channel_source(pcm, MIXER_SRC_LO, 0);
			}
#endif
			/* HP */
			if (pcm->play_dectect & SWITCH_DET_HEADPHONE) {
				//ak_pcm_debug("close hp channel");
				set_channel_source(pcm, MIXER_SRC_HP, 0);
			}
		}

		/* close other channels if needed */
		//set_channel_source(pcm, MIXER_SRC_ADC2, 0);
		pcm->IsOutputing = false;
	}
}

/////////////////////////////////////////////////////
/*
 * playback_isr -
 * playback L2DMA ISR post-process
 *
 * @data:			private data
 */
void playback_isr(unsigned long data)
{
	struct akpcm *pcm = (struct akpcm *)data;
	struct akpcm_runtime *rt = pcm->play_rt;
	unsigned int prd_bytes = rt->cfg.period_bytes;
	unsigned int buf_bytes = rt->buffer_bytes;
	unsigned int pend_bytes = 0;
	int free_bytes_changed = 0;
	int first_intr_rubbish = check_rubbish_data(rt);

#if PLAYBACK_ISR_DEBUG
    ak_pcm_debug("PB+ aec:%u, hw:%u, app:%u",
        rt->aec_ptr, rt->hw_ptr, rt->app_ptr);
#endif

	update_hw_ptr(rt);

	/* calculate pend_data & elapse */
	pend_bytes = get_pend_bytes(rt);
	calc_dac_elapse(pcm);

    /* user data in da buf is not enough */
	if (pend_bytes <= FILL_PTHRESHOLD(rt)) {
	    pend_bytes = fill_data_to_da(pcm, pend_bytes);
	    ++dac_fill_times;
	    if (dac_app_data_end && (dac_fill_times >= 3)) {
	        dac_user_data_playing = 0;
	    }
	} else {
	    dac_fill_times = 0;
	}

	if (aec_need_playback(pcm)) {
        /* pend data is enough */
		if (pend_bytes >= prd_bytes) {
			pcm->aec_has_pend_data = 1;
		}

		/* skip first intr rubbish */
		if (first_intr_rubbish) {
			rt->aec_ptr += prd_bytes-512;
			if(rt->aec_ptr >= rt->boundary)
				rt->aec_ptr = 0;
		}

		/* to start adc in case of aec */
		if (AEC_STATUS_SYNC_ADC == pcm->aec_status) {
		    sync_adc(pcm);
            free_bytes_changed = 1;
		}
	} else {
        /* AEC is disabled */
	    rt->aec_ptr = rt->hw_ptr;
		free_bytes_changed = 1;
	}

	if(has_playback_stream(pcm) && (pend_bytes) && (pend_bytes <= buf_bytes)){
		/* playback stream is running */
		update_playback_stream(pcm);
	} else if (!has_playback_stream(pcm) && pcm->dac_endwith_zero){
		struct akpcm_runtime *rt = pcm->play_rt;
		unsigned char *vaddr = rt->dma_area;
		unsigned int hw_off = (rt->hw_ptr % buf_bytes);

		/* fill a period of user data with 0s */
		memset(vaddr + hw_off, 0, prd_bytes);

		pcm->dac_endwith_zero = 0;
		update_playback_stream(pcm);
	} else {
	    /* playback stream is not running, stop it */
	    stop_playback(pcm);
		ak_pcm_debug_exx("playback stopped");
	}

	if (free_bytes_changed)
		wake_up_interruptible(&(pcm->play_wq));

#if PLAYBACK_ISR_DEBUG
	ak_pcm_debugx("PB- aec:%u, hw:%u, app:%u",
	    rt->aec_ptr, rt->hw_ptr, rt->app_ptr);
#endif
}

/*
 * playback_start_nr -
 * to start noise reduction for playback
 *
 * @value:			value of nr
 */
static int playback_start_nr(unsigned long value)
{
	struct akpcm *pcm = get_akpcm_ptr();
	struct akpcm_runtime *rt = pcm->play_rt;
	T_AEC_INPUT p_aecin;
	int enable_nr, enable_agc;

	if (rt->enable_nr)
		return 0;

	enable_nr = test_bit(ECHO_BIT_NR, &value);
	enable_agc = test_bit(ECHO_BIT_AGC, &value);

	/* open lib */
	memset(&pcm->far_bufs, 0, sizeof(pcm->far_bufs));
	pcm->far_bufs.len_far = 0;
	pcm->far_bufs.len_near = NN;
	pcm->far_bufs.len_out = NN;
	memset(&p_aecin, 0, sizeof(p_aecin));
	p_aecin.cb_fun.Malloc = akpcm_malloc_cb;
	p_aecin.cb_fun.Free = akpcm_free_cb;
	p_aecin.cb_fun.printf = (AEC_CALLBACK_FUN_PRINTF)printk;
	p_aecin.cb_fun.notify = akpcm_notify_cb;
	p_aecin.m_info.strVersion = ECHO_LIB_VERSION_STRING;
	p_aecin.m_info.chip = ECHO_CHIP_AK39XXEV3;
	p_aecin.m_info.m_Type = AEC_TYPE_1;
	p_aecin.m_info.m_BitsPerSample = 16;
	p_aecin.m_info.m_Channels = 1;
	p_aecin.m_info.m_SampleRate = rt->cfg.rate;
	p_aecin.m_info.m_Private.m_aec.m_framelen = NN;
	p_aecin.m_info.m_Private.m_aec.m_tail = TAIL;
	p_aecin.m_info.m_Private.m_aec.m_aecEna = 0;
	p_aecin.m_info.m_Private.m_aec.m_PreprocessEna = enable_nr;
	p_aecin.m_info.m_Private.m_aec.m_agcEna = enable_agc;
	p_aecin.m_info.m_Private.m_aec.m_agcLevel = AEC_DA_THRESH;
	pcm->far_filter = AECLib_Open(&p_aecin);
	if (AK_NULL == pcm->far_filter)	{
		ak_pcm_info_ex("Error! far filter is not opened!");
		return -EPERM; /* Operation not permitted */
	}

	rt->enable_nr = enable_nr;
	rt->enable_agc = enable_agc;

	return 0;
}

/*
 * playbakc_stop_nr -
 * stop nr of player
 */
static int playback_stop_nr(void)
{
	struct akpcm *pcm = get_akpcm_ptr();
	struct akpcm_runtime *rt = pcm->play_rt;

	rt->enable_nr = 0;
	rt->enable_agc = 0;

	// close lib
	if (pcm->far_filter) {
		AECLib_Close(pcm->far_filter);
		pcm->far_filter = NULL;
	}

	return 0;
}

#if defined CONFIG_SPKHP_SWITCH_AUTO || defined CONFIG_SPKHP_SWITCH_MIXER
/*
 * hpDet_wq_work -
 * delay work process when HPDet is changed
 *
 * @work:		worker for hp detection
 */
static void hpDet_wq_work(struct work_struct *work)
{
	struct akpcm *pcm = container_of(work, struct akpcm, DelayWork.work);
	int hpsrc = pcm->mixer_source[MIXER_SRC_HP];
	int spksrc = pcm->mixer_source[MIXER_SRC_LO];
	int dstsrc = hpsrc | spksrc;

	if (ak_gpio_getpin(pcm->HPDet_gpio.pin) == pcm->HPDet_on_value){
		/* hp is plugged in */
		if(!(EXIST_OUT_DEV_HP & pcm->bfExistPlayDev)){
			/* Use SP, if no HP device exist */
			pcm->play_dectect |= SWITCH_DET_SPEAKER;
			pcm->playdev = PLAYDEV_LO;
		} else {
			pcm->play_dectect &= ~SWITCH_DET_SPEAKER;
			pcm->play_dectect |= SWITCH_DET_HEADPHONE;
			pcm->playdev = PLAYDEV_HP;
		}
	} else {
		/* hp is pulled out */
		if(!(EXIST_OUT_DEV_SP & pcm->bfExistPlayDev)){
			/* Use HP, if no SP device exist */
			pcm->play_dectect |= SWITCH_DET_HEADPHONE;
			pcm->playdev = PLAYDEV_HP;
		} else {
			pcm->play_dectect &= ~SWITCH_DET_HEADPHONE;
			pcm->play_dectect |= SWITCH_DET_SPEAKER;
			pcm->playdev = PLAYDEV_LO;
		}
	}

	if (pcm->IsOutputing) {
		/* HP */
		if (pcm->play_dectect & SWITCH_DET_HEADPHONE) {
			/* if HP plug in, set HP source */
			if(dstsrc == 0) { dstsrc = SOURCE_DAC; }
			set_channel_source(pcm, MIXER_SRC_HP, dstsrc);
		} else {
			/* if HP pull out, clear HP source */
			set_channel_source(pcm, MIXER_SRC_HP, 0);
		}

	}
}

/*
 * HPDet_interrupt -
 * irq handler of hp dectetion
 *
 * @irq:		irq number of hp irq
 * @dev_id:		private data
 */
static irqreturn_t HPDet_interrupt(int irq, void *dev_id)
{
	struct akpcm *pcm = dev_id;

	if(ak_gpio_getpin(pcm->HPDet_gpio.pin) == pcm->HPDet_on_value) {
		/* HP is plugged in */
		irq_set_irq_type(pcm->HPDet_irq, pcm->HPoff_irqType);
		ak_pcm_debug("INT HPDet plug in");
	} else {
		/* HP is pulled out */
		irq_set_irq_type(pcm->HPDet_irq, pcm->HPon_irqType);
		ak_pcm_debug("INT HPDet pull out");
	}

	if (pcm->playdev != PLAYDEV_AUTO) {
		return IRQ_HANDLED;
	}

#ifdef CONFIG_SPKHP_SWITCH_AUTO
	schedule_delayed_work(&pcm->DelayWork, msecs_to_jiffies(100));
#endif
	return IRQ_HANDLED;
}
#endif

/*
 * playback_prepare -
 * prepare to playback
 *
 * @pcm:		pointer to pcm device
 */
static int playback_prepare(struct akpcm *pcm)
{
	struct akpcm_runtime *rt = pcm->play_rt;
	unsigned char *ptr = rt->dma_area;
	unsigned int rt_rate = rt->cfg.rate;
	unsigned int new_size = (rt->cfg.periods * rt->cfg.period_bytes);
	unsigned int i;
	dma_addr_t phyaddr;

	/* allocate memory for loop-buffer */
	if((ptr) && (new_size != rt->buffer_bytes)){
		dma_unmap_single(pcm->dev, rt->dma_addr, rt->buffer_bytes, DMA_TO_DEVICE);
		kfree(ptr);
		rt->dma_area = ptr = NULL;
	}

	if (!ptr) {
		ptr = kmalloc(new_size, GFP_KERNEL|GFP_DMA);
		if(!ptr){
			ak_pcm_debug("playback buffer alloc failed: 0x%x", new_size);
			return -ENOMEM;
		}

		phyaddr = dma_map_single(pcm->dev, ptr, new_size, DMA_TO_DEVICE);
		if (dma_mapping_error(pcm->dev, phyaddr))
			printk(KERN_ERR "error: %s dma_map_single failed.", __FUNCTION__);
		rt->dma_addr = phyaddr;
		rt->dma_area = ptr;
		ak_pcm_debug("playback buffer alloc: 0x%x, 0x%x",
				(int)ptr, new_size);

		/* reset some parameters */
		clear_playback_ptr(rt);
		rt->buffer_bytes = new_size;
		for(i=0; i<64; i++){
			if((new_size>>i) == 0){
				break;
			}
		}
		rt->boundary = new_size << (32-i);
		ak_pcm_debug_ex("playback boundary: 0x%x", rt->boundary);
	}

	/* set sample rate */
	ak_pcm_debug_ex("playback rate: 0x%x, %d", (int)rt_rate, rt_rate);
	pcm->play_hw->actual_rate = ak39_codec_set_dac_samplerate(rt_rate);

	set_bit(STATUS_BIT_PREPARED, &(pcm->playback_sm_flag));
	del_timer(&(pcm->timer_stop_output));

	aec_event_handler(pcm, AEC_EVENT_OPEN);

	return 0;
}

/*
 * playback_start_dma -
 * start to playback
 *
 * @pcm:		pointer to pcm device
 */
void playback_start_dma(struct akpcm *pcm)
{
	struct akpcm_runtime *rt = pcm->play_rt;
	unsigned int prd_bytes = rt->cfg.period_bytes;
	unsigned int buf_bytes = rt->buffer_bytes;
	unsigned int hw_off = (rt->hw_ptr % buf_bytes);
	dma_addr_t phyaddr = rt->dma_addr;
	u8 id = pcm->L2BufID_DAC;

	/* debug only */
	//rt->cfg.period_bytes = 512;
	//prd_bytes = rt->cfg.period_bytes;
    /*no enough data then fill zero*/
	if (get_pend_bytes(rt) < prd_bytes) {
		memset(rt->dma_area, 0, prd_bytes);
		rt->app_ptr += prd_bytes;
	}
    
	del_timer(&(pcm->timer_stop_output));
	init_completion(&(pcm->play_completion));
	set_bit(IO_BIT_STREAM, &(pcm->outflag));
	set_bit(IO_BIT_DMA, &(pcm->outflag));
	dma_sync_single_for_device(pcm->dev, phyaddr+hw_off,
	    prd_bytes, DMA_TO_DEVICE);

	l2_clr_status(id);
	l2_combuf_dma(phyaddr+hw_off, id, prd_bytes, (tL2DMA_DIR)MEM2BUF, 1);
	rt->start_time = sched_clock();
	rt->end_time = 0;
	pcm->aec_has_pend_data = 1;
	pcm->dac_endwith_zero = 1;
	ak_pcm_debug_exx("hw_off=%u, prd_bytes=%d", hw_off, prd_bytes);
}

/*
 * playback_resume -
 * resume playback
 *
 * @pcm:		pointer to pcm device
 */
static void playback_resume(struct akpcm *pcm)
{
	if(is_playback_working(pcm)) {
	    return;
	}

	ak_pcm_func("enter");
	/* sync_dac should be in protect of io lock */
	ak_pcm_assert(pcm->aec_status != AEC_STATUS_SYNC_DAC);

	aec_event_handler(pcm, AEC_EVENT_DAC_START);
	playback_start_dac(pcm);
	playback_start_dma(pcm);
}

/*
 * playback_start_force -
 * force playback start
 *
 * @pcm:		pointer to pcm device
 */
int playback_start_force(struct akpcm *pcm)
{
	struct akpcm_runtime *rt = pcm->play_rt;
	unsigned int pnd_bytes;
	unsigned int prd_bytes;

	if (!is_playback_ready(pcm)) {
		ak_pcm_debug("warn: firing up playback when it is not ready");
		return -EPERM;/* Operation not permitted */
	}

	if (is_playback_working(pcm)) {
		ak_pcm_debug("playback dma is already running");
		return 0;
	}

	ak_pcm_func("enter");

	/* ensure there's at least a period of data in buf, padding 0 if short */
	pnd_bytes = get_pend_bytes(rt);
	prd_bytes = rt->cfg.period_bytes;
	if (pnd_bytes < prd_bytes) {
		unsigned int app_pos = rt->app_ptr;
		unsigned char *vaddr = rt->dma_area;
		unsigned int buf_bytes = rt->buffer_bytes;
		unsigned int app_off = app_pos%buf_bytes;

		memset(vaddr+app_off, 0, prd_bytes);
		app_pos += prd_bytes;
		if(app_pos >= rt->boundary)
			app_pos -= rt->boundary;
		rt->app_ptr = app_pos;
	}

	/* kick start playback */
	playback_start_dac(pcm);

	return 0;
}

/*
 * playback_pause -
 * pause operation for playback
 *
 * @pcm:		pointer to pcm device
 */
int playback_pause(struct akpcm *pcm)
{
	struct akpcm_runtime *rt = pcm->play_rt;

    /* working until start playback DMA */
	if (!is_playback_working(pcm)) {
	    ak_pcm_debug_ex("playback is not working now");
	    return 0;
	}

	ak_pcm_func("enter");

	/* here just clear playback stream running flag. */
	clear_bit(IO_BIT_STREAM, &(pcm->outflag));
	if(is_playback_working(pcm)){
		wait_for_completion(&(pcm->play_completion));
		ak_pcm_info_ex("wait playback completion ok");
	}

	// playback_isr should have pushed AEC_EVENT_DAC_STOP event into fifo,
	//   so here just call handler to handle it
	aec_event_handler(pcm, AEC_EVENT_NONE);

    // clear playback buffer
	clear_playback_ptr(rt);

	return aec_set_param(pcm, pcm->app_flag);
}

/*
 * set_playback_param -
 * set player parameters
 *
 * @pcm:		pointer to pcm device
 * @arg:		arguments of player
 */
static int set_playback_param(struct akpcm *pcm, unsigned long arg)
{
    int ret = 0;
	struct akpcm_runtime *rt = pcm->play_rt;
	struct akpcm_pars *rt_cfg = &rt->cfg;

	if (copy_from_user(rt_cfg, (void __user *)arg, sizeof(struct akpcm_pars))) {
		ret = -EFAULT;
	}

    return ret;
}

/*
 * set_playback_dev -
 * set playback device
 *
 * @pcm:		pointer to pcm device
 * @arg:		arguments of player
 */
static int set_playback_dev(struct akpcm *pcm, unsigned long arg)
{
    int ret = 0;
    unsigned int value = 0;
    int sources = 0;

    ret = get_user(value, (int *)arg);
	if (ret)
	    goto set_pb_dev_end;

	if((pcm->playdev & (~PLAYDEV_MSK)) != 0){
		ret = -EINVAL;
	}
    ak_pcm_debug_ex("IOC_SET_DEV %d", value);

	if(value == pcm->playdev){
		goto set_pb_dev_end;
	}
	if(value == PLAYDEV_AUTO){
		schedule_delayed_work(&pcm->DelayWork, msecs_to_jiffies(100));
		goto set_pb_dev_end;
	}

	/* device auto-dectect */
	/* TODO, but we simply update 'pcm->playdev' in dectect isr */

	/* device not auto-dectect */
	if((value & PLAYDEV_HP) && !(pcm->playdev & PLAYDEV_HP)){
		if(pcm->playdev & PLAYDEV_LO){
			/* we copy  LO src to HP src, and clear LO src */
			sources = pcm->mixer_source[MIXER_SRC_LO];
			set_channel_source(pcm, MIXER_SRC_LO, 0);
			if (sources) {
				set_channel_source(pcm, MIXER_SRC_HP, sources);
			}
		}else{
			sources = pcm->mixer_source[MIXER_SRC_HP];
			set_channel_source(pcm, MIXER_SRC_HP, 0);
		}
	}

	if((value & PLAYDEV_LO) && !(pcm->playdev & PLAYDEV_LO)){
		if(pcm->playdev & PLAYDEV_HP){
			/* we copy  HP src to LO src, and clear HP src */
			sources = pcm->mixer_source[MIXER_SRC_HP];
			set_channel_source(pcm, MIXER_SRC_HP, 0);
			if (sources) {
				set_channel_source(pcm, MIXER_SRC_LO, sources);
			}
		}else{
			sources = pcm->mixer_source[MIXER_SRC_LO];
			set_channel_source(pcm, MIXER_SRC_LO, 0);
		}
	}

	pcm->playdev = value;

set_pb_dev_end:
    return ret;
}

/*
 * get_playback_source -
 * get playback source
 *
 * @pcm:		pointer to pcm device
 * @arg:		arguments of player
 */
static int get_playback_source(struct akpcm *pcm, unsigned long arg)
{
    unsigned int value = 0;
    unsigned int addr;

    switch (pcm->playdev) {
    case PLAYDEV_HP:
        addr = MIXER_SRC_HP;
        break;
    case PLAYDEV_LO:
        addr = MIXER_SRC_LO;
        break;
    default:
        return -EINVAL;
    }

	value = pcm->mixer_source[addr];
	return put_user(value, (int *)arg);
}

/*
 * set_playback_source -
 * set playback source
 *
 * @pcm:		pointer to pcm device
 * @arg:		arguments of player
 */
static int set_playback_source(struct akpcm *pcm, unsigned long arg)
{
    int ret = 0;
    unsigned int value = 0;
    unsigned int addr;

    ret = get_user(value, (int *)arg);
	if (ret)
	    return ret;

	ak_pcm_debug_ex("IOC_SET_SOURCES %d", value);

    switch (pcm->playdev) {
    case PLAYDEV_HP:
        addr = MIXER_SRC_HP;
		if (value < SIGNAL_SRC_MUTE) {
			value = SIGNAL_SRC_MUTE;
		} else if (value > SIGNAL_SRC_MAX){
			value = SIGNAL_SRC_MAX;
		}
#ifndef CONFIG_SPKHP_SWITCH_AUTO
		if (value != SIGNAL_SRC_MUTE){
			pcm->play_dectect |= SWITCH_DET_HEADPHONE;
		} else {
			pcm->play_dectect &= ~SWITCH_DET_HEADPHONE;
		}
#endif

		change_channel_source(pcm, addr, value);
        break;
    case PLAYDEV_LO:
        addr = MIXER_SRC_LO;
		if (value < SIGNAL_SRC_MUTE) {
			value = SIGNAL_SRC_MUTE;
		} else if (value > SIGNAL_SRC_MAX){
			value = SIGNAL_SRC_MAX;
		}
		if (value != SIGNAL_SRC_MUTE) {
			value = SOURCE_LINEIN;
		}

		change_channel_source(pcm, addr, value);
        break;
    default:
        ret = -EINVAL;
        break;
    }

	return ret;
}

/*
 * get_playback_gain -
 * get playback analog gain
 *
 * @pcm:		pointer to pcm device
 * @arg:		arguments of player
 */
static int get_playback_gain(struct akpcm *pcm, unsigned long arg)
{
    int ret = 0;
    unsigned int value = 0;
    unsigned int addr;

    switch (pcm->playdev) {
    case PLAYDEV_HP:
        addr = MIXER_VOL_HP;
		value = pcm->mixer_volume[addr];
		ret = put_user(value, (int *)arg);
        break;
    case PLAYDEV_LO:
        //addr = MIXER_VOL_LO;
		//value = pcm->mixer_volume[addr];
		//ret = put_user(value, (int *)arg);
		ret = -EINVAL;
        break;
    default:
        ret = -EINVAL;
        break;
    }

	return ret;
}

/*
 * set_playback_gain -
 * set playback analog gain
 *
 * @pcm:		pointer to pcm device
 * @arg:		arguments of player
 */
static int set_playback_gain(struct akpcm *pcm, unsigned long arg)
{
    int ret = 0;
    unsigned int addr;
	unsigned int value;

    if(pcm->playdev == PLAYDEV_HP) {
		addr = MIXER_VOL_HP;
	} else if(pcm->playdev == PLAYDEV_LO) {
		ret = -EINVAL;
		goto set_pb_gain_end;
	} else {
		ret = -EINVAL;
		goto set_pb_gain_end;
	}

	ret = get_user(value, (int *)arg);
	if (ret)
	    goto set_pb_gain_end;

	if (value < HEADPHONE_GAIN_MIN) {
		value = HEADPHONE_GAIN_MIN;
	} else if (value > HEADPHONE_GAIN_MAX) {
		value = HEADPHONE_GAIN_MAX;
	}
	ak_pcm_debug_ex("IOC_SET_GAIN, HP gain=%d", value);

	if (pcm->mixer_volume[addr] != value) {
		pcm->mixer_volume[addr] = value;

		/*
		 * normally, volume is map to DAC gain.
		 * in case aec is enabled, volume is changed by software
		 */
		if (aec_need_playback(pcm)) {
			/* do nothing here. volume is applied by aec_set_da_volume() */
		} else {
			akpcm_set_hp_gain(value);
		}
	}

set_pb_gain_end:
	return ret;
}

/*
 * playback_ioctl -
 * device file ops: ioctl
 *
 * @filp:		pointer to device file
 * @cmd:		command
 * @arg:		argument of command
 */
long playback_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
	struct akpcm *pcm = get_akpcm_ptr();
	struct akpcm_runtime *rt;// = pcm->play_rt;
	unsigned long value = 0;
	struct akpcm_features *feats;
	struct akpcm_pars *rt_cfg;
	unsigned long ul;

	lock_playback_io(pcm);

	rt = pcm->play_rt;

	if (!rt) {
		unlock_playback_io(pcm);
		ak_pcm_err_ex("play_rt is NULL");
		return -EPERM;
	}

	/* commands */
	switch(cmd){
	case IOC_PREPARE:
		ak_pcm_debug_ex("IOC_PREPARE");
		ret = playback_prepare(pcm);
		aec_set_param(pcm, pcm->app_flag);
		break;
	case IOC_RESUME:
		ak_pcm_debug_ex("IOC_RESUME");
		playback_resume(pcm);
		break;
	case IOC_PAUSE:
		ak_pcm_debug_ex("IOC_PAUSE");
		playback_pause(pcm);
		break;
	case IOC_GETELAPSE:
	    ak_pcm_debug_ex("IOC_GETELAPSE, dac_elapse=%llu", dac_elapse);
		if (copy_to_user((void __user *)arg, &dac_elapse,
		    sizeof(unsigned long long))) {
			ret = -EFAULT;
		}
		break;
	case IOC_RSTBUF:
		ak_pcm_debug_ex("IOC_RSTBUF");
		clear_playback_ptr(rt);
		break;
	case IOC_RSTALL:
		ak_pcm_debug_ex("IOC_RSTALL");
		clear_playback_ptr(rt);
    	/* reset dac elapse time */
    	dac_elapse = 0;
		break;
	/* features */
	case IOC_GET_FEATS:
	    ak_pcm_debug_ex("IOC_GET_FEATS");
		feats = pcm->play_hw;
		if (copy_to_user((void __user *)arg, feats,
		    sizeof(struct akpcm_features))) {
			ret = -EFAULT;
		}
		break;
	/* configures */
	case IOC_GET_PARS:
	    ak_pcm_debug_ex("IOC_GET_PARS");
		rt_cfg = &rt->cfg;
		if (copy_to_user((void __user *)arg, rt_cfg, sizeof(struct akpcm_pars))) {
			ret = -EFAULT;
		}
		break;
	case IOC_SET_PARS:
        ak_pcm_debug_ex("IOC_SET_PARS");
        ret = set_playback_param(pcm, arg);
		break;
	/* ---------- PLAY DEVICE ------------------------------------ */
	case IOC_GET_DEV:
	    ak_pcm_debug_ex("IOC_GET_DEV");
		value = pcm->playdev;
		ret = put_user(value, (int *)arg);
		break;
	case IOC_SET_DEV:
	    ret = set_playback_dev(pcm, arg);
		break;
		/* ---------- sources ------------------------------------ */
	case IOC_GET_SOURCES:
        ak_pcm_debug_ex("IOC_GET_SOURCES");
        ret = get_playback_source(pcm, arg);
		break;
	case IOC_SET_SOURCES:
	    ak_pcm_debug_ex("IOC_SET_SOURCES");
	    ret = set_playback_source(pcm, arg);
		break;
	/* ---------- GAIN ------------------------------------ */
	case IOC_GET_GAIN:
        ak_pcm_debug_ex("IOC_GET_GAIN");
        ret = get_playback_gain(pcm, arg);
		break;
	case IOC_SET_GAIN:
	    ret = set_playback_gain(pcm, arg);
		break;
	case IOC_SET_NR_AGC:
		if (copy_from_user(&value, (void __user *)arg, sizeof(value))) {
			ret = -EFAULT;
			goto pb_ioc_end;
		}

		ak_pcm_debug_ex("IOC_SET_NR_AGC, value=%lu", value);
		if (test_bit(ECHO_BIT_NR, &value) && test_bit(ECHO_BIT_AGC, &value))
			playback_start_nr(value);
		else
			playback_stop_nr();
		break;
	case IOC_SET_SPEAKER:
		if (copy_from_user(&value, (void __user *)arg, sizeof(value))) {
			ret = -EFAULT;
			goto pb_ioc_end;
		}

		ak_pcm_debug_ex("IOC_SET_SPEAKER, value=%lu", value);
		if (value) {
			ak_gpio_setpin(pcm->SPdown_gpio.pin, 1 - pcm->SPdown_gpio.value);
		} else {
			ak_gpio_setpin(pcm->SPdown_gpio.pin, pcm->SPdown_gpio.value);
		}
		break;
	case IOC_GETTIMER:
		ul = get_timestamp();
		if (copy_to_user((void __user *)arg, &ul, sizeof(unsigned long))) {
			ret = -EFAULT;
		}
		break;
	case IOC_GET_STATUS:
		value = dac_user_data_playing;
		if (aec_need_playback(pcm))
			value &= pcm->aec_has_pend_data;

		ret = put_user(value, (unsigned int *)arg);
		break;
	case IOC_NOTICE_END:
	    ak_pcm_debug_ex("IOC_NOTICE_END");
		dac_app_data_end = 1;
	    break;
	default:
		ret = -EINVAL;
		break;
	}

pb_ioc_end:
	unlock_playback_io(pcm);

	return ret;
}

/*
 * playback_write -
 * device file ops: write
 *
 * @filp:		pointer to device file
 * @cmd:		command
 * @arg:		argument of command
 */
ssize_t playback_write(struct file *filp, const char __user *buf, size_t count,
                    loff_t *f_pos)
{
	struct akpcm *pcm = get_akpcm_ptr();
	struct akpcm_runtime *rt = pcm->play_rt;
	unsigned int pnd_bytes = 0;
	unsigned int app_pos = rt->app_ptr;
	unsigned int app_pos_save = 0;

    if (check_playback_status(pcm)) {
        return -EPERM;
    }

#if PLAYBACK_WRITE_DEBUG
    ak_pcm_func("W+");
	ak_pcm_debug("ptr hw:%u, app:%u, aec:%u",
	    rt->hw_ptr, rt->app_ptr, rt->aec_ptr);
#endif

	/* calculate pend_data & free_space */
	if (wait_event_interruptible(pcm->play_wq,
	    (get_playback_free_bytes(rt) >= (count + WRITE_PTHRESHOLD(rt)))) < 0){
		printk("-");
		ak_pcm_debug("playback write wakeup by signal!");
		return -ERESTARTSYS;
	}

	app_pos_save = rt->app_ptr;
	app_pos = rt->app_ptr;
	/* app_pos should be aligned in frame */
	ak_pcm_assert((app_pos & (FAR_FRAME_SIZE-1)) == 0);

    if (get_user_data(pcm, buf, count, &app_pos)) {
        return -EFAULT;
    }

	update_playback_app_ptr(pcm, app_pos_save, app_pos);
	pnd_bytes = get_pend_bytes(rt);
	if (!dac_user_data_playing && (pnd_bytes >= rt->cfg.threshold)) {
	    dac_user_data_playing = 1;
	}

#if PLAYBACK_WRITE_DEBUG
    ak_pcm_debug("app_pos: %u, pnd_bytes: %u, count: %u",
        app_pos, pnd_bytes, count);
    ak_pcm_func_exx("W-");
#endif

	return count;
}

/*
 * playback_open -
 * device file ops: open file
 *
 * @inode:		device node
 * @filp:		pointer to device file
 */
int playback_open(struct inode *inode, struct file *filp)
{
    int ret = 0;
	struct akpcm *pcm = get_akpcm_ptr();
	struct akpcm_runtime *rt = pcm->play_rt;

	ak_pcm_debug_ex("rt=0x%x", (int)rt);

	lock_playback_io(pcm);

	//if (is_playback_opened(pcm)) {
	if (pcm->dac_opened_count > 0) {
		ak_pcm_err_ex("plackback device is already open");
		//ret = -EPERM; /* Operation not permitted */
		pcm->dac_opened_count++;
		ak_pcm_err_ex("dac_opened_count:%d",pcm->dac_opened_count);
		unlock_playback_io(pcm);
		return ret;
		//goto pb_open_end;
	}

	/* allocate an L2 buffer */
	if (BUF_NULL == pcm->L2BufID_DAC) {
		pcm->L2BufID_DAC = l2_alloc((l2_device_t)ADDR_DAC);
		if (BUF_NULL == pcm->L2BufID_DAC) {
			ak_pcm_err_ex("ERR! no L2 buffer for playback");
			ret = -ENOMEM;
			goto pb_open_end;
		}

		ak_pcm_debug_ex("L2 buffer ID for playback: %d", pcm->L2BufID_DAC);
	}

	/* allocate memory for akpcm_runtime */
	rt = kmalloc(sizeof(struct akpcm_runtime), GFP_KERNEL);
	if (!rt) {
		ak_pcm_err_ex("playback rt allocate failed");
		ret = -ENOMEM;
		goto pb_open_end;
	}

	pcm->play_rt = rt;
	ak_pcm_debug("playback rt allocate: 0x%x", (int)(pcm->play_rt));

	/*
	 * allocate memory for far frame buf
	 * 2 times of aec frame size because it is stereo
	 */
	pcm->far_frame_buf = (T_S8 *)kmalloc(FAR_FRAME_SIZE, GFP_KERNEL);
	if (!pcm->far_frame_buf) {
		ak_pcm_err_ex("playback frame buf allocate failed");
        ret = -ENOMEM;
        goto pb_open_end;
	}
	pcm->far_frame_off = 0;

	/* others */
	rt->dma_area = NULL;
	rt->dma_addr = 0;
	clear_playback_ptr(rt);
	spin_lock_init(&rt->ptr_lock);
	mutex_init(&rt->lock);
	rt->cfg.rate = 8000;
	rt->cfg.channels = 2;
	rt->cfg.sample_bits = 16;
	rt->cfg.period_bytes = 16384;
	rt->cfg.periods = 32;
	rt->cfg.threshold = 16384*30;
	rt->buffer_bytes = (rt->cfg.periods * rt->cfg.period_bytes);
	rt->enable_nr = 0;
	rt->enable_agc = 0;
	pcm->mixer_source[MIXER_SRC_HP] = SOURCE_DAC;
	pcm->mixer_source[MIXER_SRC_LO] = 0;//SOURCE_DAC;
	pcm->playdev = PLAYDEV_HP;
	pcm->IsOutputing = false;
	pcm->play_hw = &akpcm_play_hardware;
	dac_elapse = 0;

	pcm->far_filter = NULL;
	pcm->aec_has_pend_data = 0;

    l2_set_dma_callback(pcm->L2BufID_DAC, playback_isr, (unsigned long)pcm);
	set_bit(STATUS_BIT_OPENED, &(pcm->playback_sm_flag));
	ak_pcm_debug_exx("OK");

pb_open_end:
    if (ret) {
        if (rt) {
    		kfree(rt);
    		pcm->play_rt = NULL;
    	}

        if (BUF_NULL != pcm->L2BufID_DAC) {
            l2_free((l2_device_t)ADDR_DAC);
            pcm->L2BufID_DAC = BUF_NULL;
        }
    } else {
		pcm->dac_opened_count++;
	}
	unlock_playback_io(pcm);

	return ret;
}

/*
 * playback_close -
 * device file ops: close file
 *
 * @inode:		device node
 * @filp:		pointer to device file
 */
int playback_close(struct inode *inode, struct file *filp)
{
	struct akpcm *pcm = get_akpcm_ptr();
	struct akpcm_runtime *rt;// = pcm->play_rt;

	lock_playback_io(pcm);

	//if (!is_playback_opened(pcm)) {
	if (!pcm->dac_opened_count) {
		ak_pcm_err_ex("ERR! plackback device is not open");
		unlock_playback_io(pcm);
		/* Operation not permitted */
		return 0;
	}

	ak_pcm_func_exx("enter");

	rt = pcm->play_rt;

	if (!rt) {
		ak_pcm_err_ex("rt is NULL");
		pcm->dac_opened_count = 0;
		unlock_playback_io(pcm);
		return -EPERM;
	}

	pcm->dac_opened_count--;
	if (pcm->dac_opened_count > 0) {
		ak_pcm_err_ex("playback is used by others, dac_opened_count:%d",pcm->dac_opened_count);
		unlock_playback_io(pcm);
		/* Operation not permitted */
		return 0;
	}

	playback_pause(pcm);

	/* delay to close output channel */
	//pcm->timer_stop_output.expires = jiffies + DELAYS_FOR_CLOSE_DAC;
	//add_timer(&pcm->timer_stop_output);
	playback_stop_hw((unsigned long)pcm);
	/* free loop-buffer memory */
	clear_bit(STATUS_BIT_PREPARED, &(pcm->playback_sm_flag));

	/* may be redundant but shall not miss */
	aec_event_handler(pcm, AEC_EVENT_DAC_STOP);

	if (pcm->far_frame_buf)	{
		kfree(pcm->far_frame_buf);
		pcm->far_frame_buf = NULL;
	}

	if (rt) {
	    if (rt->dma_area) {
    		dma_unmap_single(pcm->dev, rt->dma_addr, rt->buffer_bytes,
    		    DMA_TO_DEVICE);
    		kfree(rt->dma_area);
    		rt->dma_area = NULL;
    	}

		kfree(rt);
		pcm->play_rt = NULL;
	}

	/* free L2 buffers */
	if (BUF_NULL != pcm->L2BufID_DAC) {
		l2_free((l2_device_t)ADDR_DAC);
		pcm->L2BufID_DAC = BUF_NULL;
	}

	clear_bit(STATUS_BIT_OPENED, &(pcm->playback_sm_flag));
	pcm->dac_opened_count = 0;
	unlock_playback_io(pcm);

	ak_pcm_debug_exx("OK");

	return 0;
}
