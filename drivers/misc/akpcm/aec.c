#include <linux/kfifo.h>
#include <linux/kthread.h>
#include <linux/hardirq.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include "aec.h"
#include "aec_dump.h"
#include "capture.h"
#include "playback.h"

#define AEC_TASK_DEBUG    (0)



static u_int32_t aec_cost; // us
static int aec_frame_count;
static T_U32 far_threshold = AEC_DA_THRESH; // far threshold for aec
static T_U32 agc_level = AEC_AGC_LEVEL;         // agc's target level, 0: use default. use AK32Q15
static T_U16 max_gain = 4;          // agc's max_gain, Q0
static T_U16 min_gain = AK16Q10(0.1);          // agc's min_gain, use AK16Q10
static T_S16 far_digi_gain = AK16Q10(1); // 100%  // digtal gain for far signal, Q10, 0: use default. use AK16Q10
static T_S16 near_digi_gain = AK16Q10(0.5); // 50%     // digtal gain for near signal, Q10, 0: use default. use AK16Q10
static T_S16 noise_suppress_db = -40;  // attenuation of noise in dB (negative number), 0: use default
static T_S16 near_sensitivity = 20;

static T_S32 peqpara[EQ_ARRAY_NUMBER] =
{
    0x1,  //cIsEnable
    0x0,  //preGain
    0x400,  //preVol
    0x2,  //bands
    0x0acae8a8, 0xea6a2eaf, 0x0acae8a8, 0x13de4aa7, 0xf8b2a804,  //pab[0]
    0x0a32aa0a, 0x060fd5d9, 0x096b2e33, 0xf9f02a27, 0xfc6227c3,  //pab[1]
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  //pab[2]
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  //pab[3]
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  //pab[4]
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  //pab[5]
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  //pab[6]
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  //pab[7]
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  //pab[8]
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  //pab[9]
};

static DEFINE_KFIFO(aec_event_buf, t_aec_event, 8);

static T_S32 DA_factor = AK32Q15(0.75);

static int open_filter_lib(struct akpcm *pcm)
{	
	int ret = 0;
	T_AUDIO_FILTER_INPUT s_ininfo;
	memset(&s_ininfo, 0, sizeof(T_AUDIO_FILTER_INPUT));
	
	// open eq handle	
	s_ininfo.cb_fun.Malloc = (MEDIALIB_CALLBACK_FUN_MALLOC)akpcm_malloc_cb;
	s_ininfo.cb_fun.Free = (MEDIALIB_CALLBACK_FUN_FREE)akpcm_free_cb;
	s_ininfo.cb_fun.printf = (MEDIALIB_CALLBACK_FUN_PRINTF)printk; 
	s_ininfo.cb_fun.delay = AK_NULL;
	s_ininfo.m_info.m_BitsPerSample = 16;
	s_ininfo.m_info.m_Channels = 1;
	s_ininfo.m_info.m_SampleRate = 44100; 
	s_ininfo.m_info.m_Type = _SD_FILTER_EQ;
	s_ininfo.chip = AUDIOLIB_CHIP_AK39XXEV3;	 //�ĳ���Ӧ��оƬ�ͺ�
	s_ininfo.strVersion = AUDIO_FILTER_VERSION_STRING;
	s_ininfo.m_info.m_Private.m_eq.eqmode =  _SD_EQ_USER_DEFINE;	
	s_ininfo.m_info.m_Private.m_eq.preGain = (T_S16)(-1*(1<<10));	
	s_ininfo.m_info.m_Private.m_eq.bands = 2;	
	s_ininfo.m_info.m_Private.m_eq.bandfreqs[0] = 80;	
	s_ininfo.m_info.m_Private.m_eq.bandTypes[0] = FILTER_TYPE_HPF;	
	s_ininfo.m_info.m_Private.m_eq.bandQ[0] = (T_U16)(0.707*(1<<10));
	s_ininfo.m_info.m_Private.m_eq.bandfreqs[1] = 120;
	s_ininfo.m_info.m_Private.m_eq.bandTypes[1] = FILTER_TYPE_PF1;	
	s_ininfo.m_info.m_Private.m_eq.bandgains[1] = (T_U16)(3*(1<<10));	
	s_ininfo.m_info.m_Private.m_eq.bandQ[1] = (T_U16)(0.707*(1<<10));
	
	pcm->filter_handle = _SD_Filter_Open(&s_ininfo); 
	if (!pcm->filter_handle) {
		ret = -EPERM; /* Operation not permitted */
	}

	return ret;
}

static int set_filter_lib_param(struct akpcm *pcm, T_S32 *peqpara)
{

	ak_pcm_info_ex("--- ***************pcm->filter_handle=%p\n", pcm->filter_handle);

	if (AK_TRUE != _SD_Filter_SetEqTimePara(pcm->filter_handle, peqpara)) {
		ak_pcm_err_ex("Error! set_filter_lib_param error!");
		return -EFAULT;
	}

	return 0;

}

static int filter_lib_control(struct akpcm *pcm,  unsigned char *inbuf,
						unsigned int inlen, unsigned char *outbuf, unsigned int outlen)
{
	T_AUDIO_FILTER_BUF_STRC fbuf_strc={0};	
	int processlen = 0;	
	fbuf_strc.buf_in = inbuf;	
	fbuf_strc.buf_out = outbuf;	
	fbuf_strc.len_in = inlen;	//������Ч���ݳ��� 	
	fbuf_strc.len_out = outlen; //���buffer��С	
	if (!pcm->filter_handle) {
		ak_pcm_err_ex("filter lib handle is NULL");
		return -EINVAL;
	}
	processlen = _SD_Filter_Control(pcm->filter_handle, &fbuf_strc);
	return processlen;
}

static int close_filter_lib(struct akpcm *pcm)
{
    int ret = 0;
    if (!pcm->eq_enable)
        return 0;

	if (NULL == pcm->filter_handle) {
		ak_pcm_err_ex("filter lib handle is NULL");
		return -EFAULT;
	}

	if (_SD_Filter_Close(pcm->filter_handle)) {
        ret = 0;
    } else {
        ret = -EFAULT;
    }

	pcm->filter_handle = NULL;
	return ret;
}

void print_event_string(t_aec_event event)
{
	switch (event) {
	case AEC_EVENT_NONE:
	    ak_pcm_info_ex("event-%d: none, general trigger", event);
	    break;
	case AEC_EVENT_OPEN:
	    ak_pcm_info_ex("event-%d: open", event);
	    break;
    case AEC_EVENT_ADC_START:
        ak_pcm_info_ex("event-%d: adc start", event);
        break;
    case AEC_EVENT_DAC_START:
        ak_pcm_info_ex("event-%d: dac start", event);
        break;
    case AEC_EVENT_AEC_START:
        ak_pcm_info_ex("event-%d: aec start", event);
        break;
    case AEC_EVENT_ADC_STOP:
        ak_pcm_info_ex("event-%d: adc stop", event);
        break;
    case AEC_EVENT_DAC_STOP:
        ak_pcm_info_ex("event-%d: dac stop", event);
        break;
    case AEC_EVENT_AEC_STOP:
        ak_pcm_info_ex("event-%d: aec stop", event);
        break;
	default:
	    ak_pcm_info_ex("event-%d: unknow", event);
	    break;
	}
}

/*
 * playback & capture helper routine
 * get how many data to be processed by AEC in playback or capture buffer
 */
inline unsigned int get_aec_bytes(struct akpcm_runtime *rt)
{
	unsigned int aec_bytes;
	unsigned int aec_pos = rt->aec_ptr;
	unsigned int hw_pos = rt->hw_ptr;
	unsigned int boundary = rt->boundary;

	/* calculate pend_data & free_space */
	if (hw_pos >= aec_pos) {
		aec_bytes = hw_pos - aec_pos;
	} else {
		aec_bytes = boundary - aec_pos + hw_pos;
	}

#ifdef DEBUG_BYTES
	if (aec_bytes > rt->buffer_bytes) {
	    struct akpcm *pcm = get_akpcm_ptr();
		ak_pcm_err_ex("%s err aec_bytes:%u",
		    (pcm->play_rt == rt) ? "PB":"CP", aec_bytes);
		ak_pcm_err_ex("ptr app:%u, aec:%u, hw:%u",
		    rt->app_ptr, rt->aec_ptr, rt->hw_ptr);
	}
#endif

	return aec_bytes;
}

static inline void release_aec_resource(struct akpcm *pcm)
{
    /* close lib */
	if (pcm->aec_filter) {
		AECLib_Close(pcm->aec_filter);
		pcm->aec_filter = NULL;
	}
	/* release temp bufs */
	if (pcm->aec_out_buf) {
		kfree(pcm->aec_out_buf);
		pcm->aec_out_buf = NULL;
	}
}

static inline int update_capt_aec_ptr(struct akpcm_runtime *cptr_rt)
{
    unsigned int aec_ptr = cptr_rt->aec_ptr;

	if (aec_ptr >= cptr_rt->hw_ptr) {
		printk(KERN_ERR "%s aec>=hw", __func__);
		return 0;
	}

	aec_ptr += AEC_FRAME_BYTES;
	if(aec_ptr >= cptr_rt->boundary)
		aec_ptr -= cptr_rt->boundary;

	cptr_rt->aec_ptr = aec_ptr;

	return 1;
}

static inline int update_play_aec_ptr(struct akpcm_runtime *play_rt)
{
	unsigned int aec_ptr = play_rt->aec_ptr;

	if (aec_ptr >= play_rt->hw_ptr) {
		printk(KERN_ERR "%s aec>=hw", __func__);
		return 0;
	}

	aec_ptr += FAR_FRAME_SIZE;
	if(aec_ptr >= play_rt->boundary) {
	    aec_ptr -= play_rt->boundary;
	}

	play_rt->aec_ptr = aec_ptr;

	return 1;
}

static void handle_aec_task(struct akpcm *pcm)
{
	struct akpcm_runtime *play_rt = pcm->play_rt;
	struct akpcm_runtime *cptr_rt = pcm->cptr_rt;
	struct aec_dump_info info;
	T_AEC_BUF *aec_bufs = &pcm->aec_bufs;
	T_S16 *near_frame = AK_NULL;
	T_S16 *far_frame = AK_NULL;
	u_int64_t start_time = 0;
	u_int64_t end_time = 0;
	int spend = 0;
	int spend_valid = 0;
	int cptr_update_aec;
	int play_done = 0;

    if (cptr_rt) {
        info.sample_rate = cptr_rt->cfg.rate;
    }
    
	print_isr(pcm->aec_dump_type);

#if AEC_TASK_DEBUG
    ak_pcm_func("T+");
#endif

	start_time  = sched_clock();
	while (1) {
		spin_lock_irq(&cptr_rt->ptr_lock);
		if (!(get_aec_bytes(cptr_rt) >= AEC_FRAME_BYTES)) {
			spin_unlock_irq(&cptr_rt->ptr_lock);
			break;
		}
		spin_unlock_irq(&cptr_rt->ptr_lock);

		/* get far end frame */
		if (pcm->enable_aec) {
		    /* playback may stop in isr */
		    if (!is_playback_working(pcm)) {
		        break;
		    }

			spin_lock_irq(&play_rt->ptr_lock);
			/* if there is not enouph far data, skip this round */
			if (get_aec_bytes(play_rt) < FAR_FRAME_SIZE) {
				spin_unlock_irq(&play_rt->ptr_lock);
				break;
			}

			far_frame  = (T_S16*)(play_rt->dma_area +
			    (play_rt->aec_ptr % play_rt->buffer_bytes));

			spin_unlock_irq(&play_rt->ptr_lock);

			/* extrace single track from stereo */
			stereo_to_mono(far_frame, far_frame, FAR_FRAME_SIZE);
			
			info.id = AEC_DUMP_FAR;
			info.data = (unsigned char *)far_frame;
			info.size = AEC_FRAME_BYTES;
			aec_dump(pcm->aec_dump_type, &info);
		}

		/* when collected sufficient data, do AEC task */
		near_frame = (T_S16*)(cptr_rt->dma_area +
		    (cptr_rt->aec_ptr % cptr_rt->buffer_bytes));

		if (pcm->aec_dump_type == AEC_DUMP_FILE || pcm->aec_dump_type == AEC_DUMP_DEBUG) {
			info.id = AEC_DUMP_NEAR;
			info.data = (unsigned char *)near_frame;
			info.size = AEC_FRAME_BYTES;
		    aec_dump(pcm->aec_dump_type, &info);
		}
		
		if (pcm->eq_enable) {
			filter_lib_control(pcm, (unsigned char *)near_frame, 
							AEC_FRAME_BYTES, (unsigned char *)near_frame, AEC_FRAME_BYTES);
	
			if (pcm->aec_dump_type == AEC_DUMP_FILE || pcm->aec_dump_type == AEC_DUMP_DEBUG) {
				/* dump after eq audio data */
				info.id = AEC_DUMP_AFTER_EQ;
				info.data = (unsigned char *)near_frame;
				info.size = AEC_FRAME_BYTES;
			    aec_dump(pcm->aec_dump_type, &info);
			}
		}

		/* put data to AEC process */
		aec_bufs->buf_near = near_frame;
		aec_bufs->buf_far = far_frame;/*far_frame is NULL if dac closed, it is allowed*/
		/* near and out buf should not be the same */
		aec_bufs->buf_out = pcm->aec_out_buf;

        if (pcm->aec_filter) {
            AECLib_Control(pcm->aec_filter, aec_bufs);
        }

		/* copy to near buffer */
		memcpy(near_frame, aec_bufs->buf_out, AEC_FRAME_BYTES);

		if (pcm->aec_dump_type == AEC_DUMP_FILE || pcm->aec_dump_type == AEC_DUMP_DEBUG) {
			info.id = AEC_DUMP_RES;
			info.data = (unsigned char *)near_frame;
			info.size = AEC_FRAME_BYTES;
			
    		aec_dump(pcm->aec_dump_type, &info);
    		aec_dump_sync(pcm);
    	}

		aec_frame_count++;
		spend_valid++;

		spin_lock_irq(&cptr_rt->ptr_lock);

		/* update capture aec pointer */
		cptr_update_aec = update_capt_aec_ptr(cptr_rt);

		if (pcm->enable_aec && cptr_update_aec) {
		    /* update playback aec pointer */
		    update_play_aec_ptr(play_rt);
			play_done = 1;
		}
		spin_unlock_irq(&cptr_rt->ptr_lock);

		/* wakeup capture read */
		wake_up_interruptible(&(pcm->capt_wq));

		/* wakeup playback write */
		if (play_done)
			wake_up_interruptible(&(pcm->play_wq));

		if ((pcm->aec_high_prio == 2)
		    && ((get_aec_bytes(cptr_rt) < cptr_rt->cfg.threshold))) {
			pcm->aec_high_prio = -1;
			wake_up_interruptible(&pcm->aec_prio_wq);
		}
	}

	if (spend_valid) {
		end_time = sched_clock();
		spend = ak39_spend_us(start_time, end_time);
		aec_cost += spend;
	}

#if AEC_TASK_DEBUG
    if (play_rt) {
        ak_pcm_debug("[PB]: aec_bytes:%u, hw_pos: %u, aec_ptr:%u",
            get_aec_bytes(play_rt), play_rt->hw_ptr, play_rt->aec_ptr);
    }
    if (cptr_rt) {
        ak_pcm_debug("[CP]: aec_bytes:%u, hw_pos: %u, aec_ptr:%u",
            get_aec_bytes(cptr_rt), cptr_rt->hw_ptr, cptr_rt->aec_ptr);
    }
    ak_pcm_debug("aec_cost:%u, spend:%d, af_cnt: %d",
        aec_cost, spend, aec_frame_count);
    ak_pcm_func_exx("T-");
#endif
}

static int aec_thread(void *data)
{
	struct akpcm *pcm = (struct akpcm *)data;

	ak_pcm_func("enter");

	while (pcm->aec_thread_run) {
		if (wait_event_interruptible(pcm->aec_wq,
			(pcm->aec_data_triger || !(pcm->aec_thread_run))) < 0){
			ak_pcm_debug("aec thread wakeup by signal!");
			return -ERESTARTSYS;
		}

		if (!pcm->aec_thread_run)
			break;

        mutex_lock(&(pcm->aec_status_lock));
        if (pcm->aec_filter) {
		    handle_aec_task(pcm);
		}
		mutex_unlock(&(pcm->aec_status_lock));
		pcm->aec_data_triger = 0;
	}

	ak_pcm_func("stop working");

	/* Wait until we are told to stop */
	for (;;) {
		set_current_state(TASK_INTERRUPTIBLE);
		if (kthread_should_stop())
			break;
		schedule();
	}
	__set_current_state(TASK_RUNNING);

	ak_pcm_func("leave");

	return 0;
}

static int supervise_aec_priority(void *data)
{
	struct akpcm *pcm = (struct akpcm *)data;
	struct akpcm_runtime *cptr_rt = pcm->cptr_rt;
	struct sched_param param;

	ak_pcm_func("enter");

	while (pcm->aec_supervisor_thread_run) {
		if (wait_event_interruptible(pcm->aec_prio_wq,
				((pcm->aec_high_prio == -1)
				 || (pcm->aec_high_prio == 1)
				 || !(pcm->aec_supervisor_thread_run)))){
			ak_pcm_debug("aec priority supervisor thread wakeup by signal!");
			return -ERESTARTSYS;
		}

		switch (pcm->aec_high_prio) {
		case 1: //to raise
			param.sched_priority = MAX_RT_PRIO-2;

			ak_pcm_info("raise aec task's priority, aec_bytes=%d",
					get_aec_bytes(cptr_rt));
			sched_setscheduler_nocheck(pcm->aec_thread, SCHED_FIFO, &param);
			pcm->aec_high_prio = 2;
			break;
		case -1: //to restore
			param.sched_priority = MAX_RT_PRIO-1;

			ak_pcm_info("restore aec task's priority, aec_bytes=%d",
					get_aec_bytes(cptr_rt));
			sched_setscheduler_nocheck(pcm->aec_thread, SCHED_FIFO, &param);
			pcm->aec_high_prio = 0;
			break;
		default:
			break;
		}
	}

	ak_pcm_func("stop working");

	/* Wait until we are told to stop */
	for (;;) {
		set_current_state(TASK_INTERRUPTIBLE);
		if (kthread_should_stop())
			break;
		schedule();
	}
	__set_current_state(TASK_RUNNING);

	ak_pcm_func("leave");

	return 0;
}

static inline void exit_aec_thread(struct akpcm *pcm)
{
    /* let two aec thread exit */
	pcm->aec_thread_run = 0;
	pcm->aec_supervisor_thread_run = 0;
	if (pcm->aec_prio_supervisor_thread) {
	    wake_up_interruptible(&(pcm->aec_prio_wq));
	    kthread_stop(pcm->aec_prio_supervisor_thread);
	    pcm->aec_prio_supervisor_thread = NULL;
	}

	if (pcm->aec_thread) {
	    wake_up_interruptible(&(pcm->aec_wq));
	    kthread_stop(pcm->aec_thread);
	    pcm->aec_thread = NULL;
	}
}

static int create_aec_thread(struct akpcm *pcm)
{
    int ret = 0;
    struct sched_param param = {0};

    init_waitqueue_head(&(pcm->aec_wq));

	/* create aec thread */
	pcm->aec_thread = kthread_create(aec_thread, pcm, "aec_task");
	if (IS_ERR(pcm->aec_thread)) {
	    ak_pcm_info_ex("ERROR! unable to create aec thread: %ld",
				PTR_ERR(pcm->aec_thread));
		ret = PTR_ERR(pcm->aec_thread);
		goto create_thread_end;
	}

    param.sched_priority = (MAX_RT_PRIO - 1);
	/* similar to workqueues */
	sched_setscheduler_nocheck(pcm->aec_thread, SCHED_FIFO, &param);

	/* create aec priority supervisor thread */
	pcm->aec_high_prio = 0;
	init_waitqueue_head(&(pcm->aec_prio_wq));
	pcm->aec_prio_supervisor_thread = kthread_create(supervise_aec_priority,
	    pcm, "aec_priority");
	if (IS_ERR(pcm->aec_prio_supervisor_thread)) {
	    ak_pcm_info_ex("ERROR! unable to create aec priority supervisor "
				"thread: %ld", PTR_ERR(pcm->aec_prio_supervisor_thread));
		ret = PTR_ERR(pcm->aec_prio_supervisor_thread);
		goto create_thread_end;
	}

	/* give it a very high priority */
	param.sched_priority = (MAX_RT_PRIO - 3);
	/* similar to workqueues */
	sched_setscheduler_nocheck(pcm->aec_prio_supervisor_thread,
	    SCHED_FIFO, &param);

create_thread_end:
    if (ret) {
        exit_aec_thread(pcm);
    }

    return ret;
}

static int open_aec_lib(struct akpcm *pcm)
{
    int ret = 0;
    T_AEC_INPUT p_aecin;

    memset(&pcm->aec_bufs, 0, sizeof(pcm->aec_bufs));
	pcm->aec_bufs.len_far = NN;
	pcm->aec_bufs.len_near = NN;
	pcm->aec_bufs.len_out = NN;

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
    
    if (pcm->cptr_rt)
	    p_aecin.m_info.m_SampleRate = pcm->cptr_rt->cfg.rate;
    else
        p_aecin.m_info.m_SampleRate = 8000;
    
	p_aecin.m_info.m_Private.m_aec.m_framelen = NN;
	p_aecin.m_info.m_Private.m_aec.m_tail = TAIL;
	p_aecin.m_info.m_Private.m_aec.m_aecEna = pcm->app_enable_aec;
	p_aecin.m_info.m_Private.m_aec.m_PreprocessEna = pcm->app_enable_nr;
	p_aecin.m_info.m_Private.m_aec.m_agcEna = pcm->app_enable_agc;
	p_aecin.m_info.m_Private.m_aec.m_agcLevel = agc_level;
	p_aecin.m_info.m_Private.m_aec.m_maxGain = max_gain;
	p_aecin.m_info.m_Private.m_aec.m_farThreshold = far_threshold;
	p_aecin.m_info.m_Private.m_aec.m_farDigiGain = far_digi_gain;
	p_aecin.m_info.m_Private.m_aec.m_nearDigiGain = near_digi_gain;

	p_aecin.m_info.m_Private.m_aec.m_minGain = min_gain;
	p_aecin.m_info.m_Private.m_aec.m_noiseSuppressDb = noise_suppress_db;
	p_aecin.m_info.m_Private.m_aec.m_nearSensitivity = near_sensitivity;

	pcm->aec_filter = AECLib_Open(&p_aecin);
	if (!pcm->aec_filter) {
		ak_pcm_err_ex("AEC open failed!");
		ret = -EPERM; /* Operation not permitted */
	} else {
		ak_pcm_info_ex("AEC open success, version:%s", ECHO_LIB_VERSION_STRING);
	}

	return ret;
}

static int open_aec(void)
{
    int ret = 0;
	struct akpcm *pcm = get_akpcm_ptr();	
	T_S8 * version = NULL;
	T_AUDIO_FILTER_CB_FUNS cb_filter={0};
	
	int enable_aec = pcm->app_enable_aec;
	int enable_nr  = pcm->app_enable_nr;
	int enable_agc = pcm->app_enable_agc;

    if (pcm->aec_filter) {
		ak_pcm_info_ex("skip, because aec is already opened.");
		return 0;
	}

	if (pcm->eq_enable && !pcm->filter_handle) {
		if (open_filter_lib(pcm)) {
			ak_pcm_err_ex("Error! open_filter_libis NULL!");
			return -EPERM;
		} else {

			cb_filter.printf = (MEDIALIB_CALLBACK_FUN_PRINTF)printk;
			version = _SD_GetAudioFilterVersions(&cb_filter);
			ak_pcm_info_ex("filter lib open success, version:%s", version);

			if (set_filter_lib_param(pcm, peqpara)) {
				ak_pcm_err_ex("Error! set_filter_lib_param error!");
				return -EPERM;
			}
		}
	}	

	ak_pcm_info_ex("aec=%d, nr=%d, agc=%d", enable_aec, enable_nr, enable_agc);
	if (!enable_aec && !enable_nr)
		return 0;

	ak_pcm_assert(pcm->aec_status == AEC_STATUS_IDLE);
    if (open_aec_lib(pcm)) {
        return -EPERM;
    }

	/* prepare aec result buf */
	pcm->aec_out_buf = (T_S16 *)kmalloc(AEC_FRAME_BYTES, GFP_KERNEL);
	if (!pcm->aec_out_buf) {
		ak_pcm_err_ex("alloc AEC out buf failed!");
		ret = -ENOMEM;
		goto open_aec_end;
	}

	pcm->enable_aec = enable_aec;
	pcm->enable_nr  = enable_nr;
	pcm->enable_agc = enable_agc;

	/* aec thread working */
	pcm->aec_thread_run = 1;
	pcm->aec_data_triger = 0;
	aec_cost=0;
	aec_frame_count=0;
	wake_up_process(pcm->aec_thread);

	/* aec priority supervisor thread working */
	pcm->aec_supervisor_thread_run = 1;
	wake_up_process(pcm->aec_prio_supervisor_thread);

open_aec_end:
    if (ret) {
        release_aec_resource(pcm);
    }

	return ret;
}

static int close_aec(struct akpcm *pcm)
{
	if (!pcm->aec_filter) {
		ak_pcm_info_ex("skip, because aec is not opened.");
		return 0;
	}

	ak_pcm_func("+");

	update_aec_status(pcm, AEC_STATUS_IDLE);
	print_isr(pcm->aec_dump_type);
	aec_dump_complete(pcm->aec_dump_type);

	/* restore hp and mic gain */
	akpcm_set_hp_gain(pcm->mixer_volume[MIXER_VOL_HP]);
	akpcm_set_mic_gain(pcm->mixer_volume[MIXER_VOL_MIC]);

	pcm->enable_aec = 0;
	pcm->enable_nr = 0;
	pcm->enable_agc = 0;
	release_aec_resource(pcm);

	ak_pcm_info_ex("AEC: %d frames, total: %d(ms), %d(us) per frame",
	    aec_frame_count, (aec_cost / 1000),
	    aec_frame_count ? (aec_cost / aec_frame_count) : 0);

	ak_pcm_func("-");

	return 0;
}

/* check aec prerequisite, and try to open aec or just nr */
static int try_open_aec(struct akpcm *pcm)
{
	int adc_ready = is_capture_ready(pcm);
	int dac_ready = is_playback_ready(pcm);
	int dac_working = is_playback_working(pcm);
	int adc_working = is_capture_working(pcm);
	int ret = -1;

	ak_pcm_func("+ app_enable_aec=%d, app_enable_nr=%d",
	    pcm->app_enable_aec, pcm->app_enable_nr);
	ak_pcm_debug("adc_ready=%d, dac_ready=%d, adc_working=%d, dac_working=%d",
	    adc_ready, dac_ready, adc_working, dac_working);

	ak_pcm_assert(pcm->aec_status == AEC_STATUS_IDLE);

    if (adc_ready) {
        /* user want to enable AEC */
    	if (pcm->app_enable_aec) {
    		/* adc and dac must all be ready. */
    		if (dac_ready) {
    		    /* adc and dac must be the same sample rate. */
                if((pcm->play_rt->cfg.rate == pcm->cptr_rt->cfg.rate)) {
                    //&& !(adc_working && dac_working)) {
        			ret = open_aec();
        		}
    		} else if (pcm->app_enable_nr) { /* user also enable nr */
		        /* to open aec with only nr enabled */
    			ak_pcm_info_ex("dac not ready, open with nr only");
    			pcm->app_enable_aec = 0;
    			ret = open_aec();
    			pcm->app_enable_aec = 1;
    		}
    	} else if (pcm->app_enable_nr) { /* user just want to enable NR */
    		ret = open_aec();
    	}
    }

	ak_pcm_func("- %s", ret ? "not opened" : "opened");

	return ret;
}

static e_aec_status switch_aec_status(struct akpcm *pcm, t_aec_event event,
                        e_aec_status old_status)
{
    e_aec_status new_status = old_status;

    switch (pcm->aec_status) {
	case AEC_STATUS_IDLE:
		if ((event == AEC_EVENT_OPEN)
		    && !pcm->aec_filter     /* make sure aec not opened */
		    && (try_open_aec(pcm) >= 0)){
			if (pcm->enable_aec) {
				int dac_working = is_playback_working(pcm);
				int adc_working = is_capture_working(pcm);
				ak_pcm_assert(!(dac_working&&adc_working));

				new_status = AEC_STATUS_AEC_OPENED;
				if (dac_working && !adc_working) {
					new_status = AEC_STATUS_WAIT_ADC;
				} else if (!dac_working && adc_working) {
					new_status = AEC_STATUS_SYNC_DAC;
				}
			} else if (pcm->enable_nr) {
				/* only enable NR/AGC */
				new_status = AEC_STATUS_NR_OPENED;
				update_aec_status(pcm, new_status);

				new_status = AEC_STATUS_NR_WORKING;
			}
		}
		break;
	case AEC_STATUS_AEC_OPENED:
		if (event == AEC_EVENT_DAC_START) {
			new_status = AEC_STATUS_WAIT_ADC;
		} else if (event == AEC_EVENT_ADC_START) {
			ak_pcm_assert(!is_playback_working(pcm));

			new_status = AEC_STATUS_SYNC_DAC;
		}
		break;
	case AEC_STATUS_WAIT_ADC:
		if (event == AEC_EVENT_ADC_START) {
			ak_pcm_assert(!is_capture_working(pcm));

			new_status = AEC_STATUS_SYNC_ADC;
		}
		break;
	case AEC_STATUS_SYNC_ADC:
		if (event == AEC_EVENT_ADC_START) {
			/* shall be in isr context, so do least thing */
			new_status = AEC_STATUS_AEC_WORKING;
		}
		break;
	case AEC_STATUS_SYNC_DAC:
		if (event == AEC_EVENT_DAC_START) {
			/* shall be in isr context, so do least thing */
			new_status = AEC_STATUS_AEC_WORKING;
		}
		break;
	default:
		break;
	}

	return new_status;
}

static e_aec_status start_procedure(struct akpcm *pcm, e_aec_status new_status)
{
    switch (new_status) {
    case AEC_STATUS_SYNC_ADC:
        /* wait until adc dma started */
		ak_pcm_debug("adc sync wait");
		update_aec_status(pcm, new_status);
		if (wait_event_interruptible(pcm->aec_sync_wq,
					(is_capture_working(pcm))) < 0) {
			return -ERESTARTSYS;
		}

		ak_pcm_debug("adc sync started");
		/* aec_status have been altered in isr */
		new_status = pcm->aec_status;
        break;
    case AEC_STATUS_SYNC_DAC:
        playback_start_force(pcm);

		/* wait until dac dma started */
		ak_pcm_debug("dac sync wait");
		update_aec_status(pcm, new_status);
		if (wait_event_interruptible(pcm->aec_sync_wq,
					(is_playback_working(pcm))) < 0) {
			return -ERESTARTSYS;
		}

		ak_pcm_debug("dac sync started");
		/* aec_status have been altered in isr */
		new_status = pcm->aec_status;
        break;
    default:
        break;
    }

    return new_status;
}

static t_aec_event exit_procedure(struct akpcm *pcm, t_aec_event event,
                        e_aec_status old_status, e_aec_status new_status)
{
    t_aec_event next_event = AEC_EVENT_NONE;

    if (pcm->enable_aec) {
		switch (event) {
		case AEC_EVENT_DAC_STOP:
		case AEC_EVENT_ADC_STOP:
			next_event = AEC_EVENT_OPEN; // reopen
			/* intentionally fall down */
		case AEC_EVENT_AEC_STOP:
			close_filter_lib(pcm);

			close_aec(pcm);
			new_status = AEC_STATUS_IDLE;
			break;
		default:
			break;
		}
	} else if (pcm->enable_nr) {
	    /* only nr is working, stopping of adc can cause nr to stop */
		switch (event) {
		case AEC_EVENT_ADC_STOP:
			next_event = AEC_EVENT_OPEN; // reopen
			/* intentionally fall down */
		case AEC_EVENT_AEC_STOP:
			close_filter_lib(pcm);
			close_aec(pcm);
			new_status = AEC_STATUS_IDLE;
			break;
		default:
			break;
		}
	}

    if (new_status != old_status) {
        ak_pcm_debug_ex("event:%d, status: %d->%d",
            event, old_status, new_status);
        update_aec_status(pcm, new_status);
    }

	return next_event;
}

static int handle_aec_event(struct akpcm *pcm, t_aec_event event)
{
	e_aec_status old_status = pcm->aec_status;
	e_aec_status new_status = old_status;
	t_aec_event next_event = AEC_EVENT_NONE;

	ak_pcm_func("+ aec_status=%d, event=%d", pcm->aec_status, event);

    new_status = switch_aec_status(pcm, event, old_status);

	/* start procedure */
	if (new_status != old_status) {
	    ak_pcm_debug_ex("aec status switch: %d->%d", old_status, new_status);
        new_status = start_procedure(pcm, new_status);
	}

	/*
	 * exit procedure
	 * aec is working, stopping of adc or dac can cause aec to stop.
	 */
    next_event = exit_procedure(pcm, event, old_status, new_status);

	ak_pcm_func_exx("-");

	return (next_event == AEC_EVENT_NONE)? 0 : handle_aec_event(pcm, next_event);
}

static void handle_event_wq(unsigned long data)
{
	struct akpcm *pcm = get_akpcm_ptr();
	t_aec_event buf_event;
	int ret = 0;

	ak_pcm_func("enter");

	mutex_lock(&(pcm->aec_status_lock));
	while (!kfifo_is_empty(&aec_event_buf)) {
		ret = kfifo_get(&aec_event_buf, &buf_event);
		ak_pcm_debug("process buffered aec event %d", buf_event);
		handle_aec_event(pcm, buf_event);
	}
	mutex_unlock(&(pcm->aec_status_lock));
}

int aec_set_param(struct akpcm *pcm, int value)
{
	ak_pcm_func("+ value=%d", value);

	pcm->app_flag = value;
	pcm->app_enable_aec = test_bit(ECHO_BIT_AEC, &(pcm->app_flag));
	pcm->app_enable_nr  = test_bit(ECHO_BIT_NR, &(pcm->app_flag));
	pcm->app_enable_agc = test_bit(ECHO_BIT_AGC, &(pcm->app_flag));

	ak_pcm_debug_ex("aec:%d->%d, nr:%d->%d, agc=%d->%d",
			pcm->enable_aec, pcm->app_enable_aec,
			pcm->enable_nr,  pcm->app_enable_nr,
			pcm->enable_agc, pcm->app_enable_agc);

    /* params not changed */
	if ((pcm->enable_aec == pcm->app_enable_aec)
		&& (pcm->enable_nr == pcm->app_enable_nr)
		&& (pcm->enable_agc == pcm->app_enable_agc)) {
		return 0;
	}

	/* params has been changed, then open / re-open / close aec */
	if (pcm->enable_aec || pcm->enable_nr) {
	    /* aec is already opened, stop aec */
		aec_event_handler(pcm, AEC_EVENT_AEC_STOP);
	}

	if (pcm->app_enable_aec || pcm->app_enable_nr) {
	    /* user needs aec */
		aec_event_handler(pcm, AEC_EVENT_OPEN);
	}

	ak_pcm_func_exx("-");

	return 0;
}

int aec_event_handler(struct akpcm *pcm, t_aec_event event)
{
    print_event_string(event);

	if (in_interrupt()) {
		ak_pcm_debug("put aec event %d into buffer", event);
		kfifo_put(&aec_event_buf, &event);
		queue_work(pcm->aec_event_wq, &(pcm->aec_event_work));
		return 0;
	}

	/*handle event from isr*/
	handle_event_wq(0);

	mutex_lock(&(pcm->aec_status_lock));
	if (event > AEC_EVENT_NONE)
		handle_aec_event(pcm, event);

	mutex_unlock(&(pcm->aec_status_lock));

	return 0;
}

int aec_init(struct akpcm *pcm)
{
	pcm->enable_aec = 0;
	pcm->enable_nr = 0;
	pcm->enable_agc = 0;
	pcm->app_flag = 0;
	pcm->app_enable_aec = 0;
	pcm->app_enable_nr = 0;
	pcm->app_enable_agc = 0;
	pcm->aec_dump_type = 0;
	pcm->eq_enable = 0;
	pcm->aec_status = AEC_STATUS_IDLE;
	mutex_init(&(pcm->aec_status_lock));
	init_waitqueue_head(&(pcm->aec_sync_wq));
	pcm->aec_filter = NULL;
	pcm->aec_event_wq = create_singlethread_workqueue("aec_event");
	INIT_WORK(&(pcm->aec_event_work), (work_func_t)handle_event_wq);

	pcm->aec_thread_run = 0;
	pcm->aec_data_triger = 0;

	return create_aec_thread(pcm);
}

void aec_exit(struct akpcm *pcm)
{
    exit_aec_thread(pcm);
}

T_VOID aec_set_da_volume(struct akpcm *pcm, T_S16 *data, T_U16 data_len)
{
	T_S16 da_thresh = far_threshold;
	T_S16 max_sample = 0;
	T_S32 factor;
	T_U32 i;
	T_S32 tmp;

	for (i = 0; i < data_len; i++) {
		tmp = data[i];

		data[i] = (T_S16)tmp;

		tmp = (data[i] >= 0)? data[i] : -data[i];
		if (tmp > max_sample)
			max_sample = (T_S16)tmp;
	}

	if (max_sample > da_thresh) {
		factor = (((T_S32)da_thresh) << 15) / max_sample;
		if (factor < DA_factor)
			DA_factor = factor;
		else
			DA_factor = (DA_factor * AK32Q15(0.99) + factor * AK32Q15(0.01)) >> 15;
	} else {
		DA_factor = (DA_factor * AK32Q15(.505)) >> 14; // *1.01
		if (DA_factor > 32767)
			DA_factor = 32767;
	}

	for(i = 0; i < data_len; i++) {
		tmp = data[i];
		tmp = (tmp * DA_factor) >> 15;
		data[i] = (T_S16)tmp;
	}
}

void aec_set_nr_max(struct akpcm *pcm, int enable_max)
{
#if 0
    T_AEC_REQUEST aec_req = {0};
	T_U32 gain = 0;

    if (enable_max) {
        gain = AK16Q10(0);
    } else {
        gain = AK16Q10(0.1);
    }
	//aec_req.m_req_type = AEC_REQ_SET_AGC_MINGAIN2;
	aec_req.addrLen = 4;
	aec_req.addr = (T_U8*)(&gain);

	AECLib_Request(pcm->aec_filter, &aec_req);
#endif
    ak_pcm_info_ex("can not ste aec_set_nr_max");
}

int aec_get_echo_params(struct akpcm *pcm, T_U8 *aec_param)
{
    T_AEC_PARAMS *aec_param_info;
    aec_param_info = (T_AEC_PARAMS *)aec_param;
	if (!pcm->aec_filter) {
		ak_pcm_info_ex("pcm->aec_filter=%p", pcm->aec_filter);
    	aec_param_info->m_farThreshold = far_threshold;
	    aec_param_info->m_agcLevel = agc_level;
	    aec_param_info->m_maxGain = max_gain;
	    aec_param_info->m_minGain = min_gain;
	    aec_param_info->m_farDigiGain = far_digi_gain;
	    aec_param_info->m_nearDigiGain = near_digi_gain;
	    aec_param_info->m_noiseSuppressDb = noise_suppress_db;
	    aec_param_info->m_nearSensitivity = near_sensitivity;
        aec_param_info->m_aecEna = 0;
        aec_param_info->m_agcEna = 0;
        aec_param_info->m_PreprocessEna = 0;
        return -EFAULT;
	} else {     
        T_AEC_REQUEST aec_request = {0};
    	ak_pcm_info_ex("aec_get_echo_params");

        aec_request.m_req_type = AEC_REQ_GET_PARAMS;
        aec_request.addr = aec_param;
        aec_request.addrLen = sizeof(T_AEC_PARAMS);
        AECLib_Request(pcm->aec_filter, &aec_request);
   }
   return 0;
}

int aec_set_echo_params(struct akpcm *pcm, T_U8 *aec_param)
{
	T_AEC_REQUEST aec_request = {0};
	T_AEC_PARAMS *aec_param_info;
    int ret = 0;
	if (!pcm->aec_filter) {
		ak_pcm_info_ex("pcm->aec_filter=%p", pcm->aec_filter);
		ret = -EFAULT;
	} else {
        ak_pcm_info_ex("aec_set_echo_params");
        
        aec_request.m_req_type = AEC_REQ_SET_PARAMS;
        aec_request.addr = (T_U8 *)aec_param;
        aec_request.addrLen = sizeof(T_AEC_PARAMS);
        AECLib_Request(pcm->aec_filter, &aec_request);
    } 

	/* notice the playback the far threshold */
	aec_param_info = (T_AEC_PARAMS *)aec_param;
	far_threshold = aec_param_info->m_farThreshold;
	agc_level = aec_param_info->m_agcLevel;
	max_gain = aec_param_info->m_maxGain;
	min_gain = aec_param_info->m_minGain;
	far_digi_gain = aec_param_info->m_farDigiGain;
	near_digi_gain = aec_param_info->m_nearDigiGain;
	noise_suppress_db = aec_param_info->m_noiseSuppressDb;
	near_sensitivity = aec_param_info->m_nearSensitivity;

    if (aec_param_info->m_PreprocessEna) {
        set_bit(ECHO_BIT_NR, &(pcm->app_flag));
    } else {
        clear_bit(ECHO_BIT_NR, &(pcm->app_flag));
    }

    if (aec_param_info->m_agcEna) {
        set_bit(ECHO_BIT_AGC, &(pcm->app_flag));
    } else {
        clear_bit(ECHO_BIT_AGC, &(pcm->app_flag));
    }
    return ret;
}

int aec_set_eq_params(struct akpcm *pcm, T_S32 *eq_param)
{
    T_AUDIO_FILTER_CB_FUNS cb_filter={0};
	T_S8 * version = NULL;

	ak_pcm_info_ex("aec_set_eq_params, eq_param=%p, pcm =%p", eq_param, pcm);

	if (!(pcm->filter_handle)) {
		if (open_filter_lib(pcm)) {
			ak_pcm_err_ex("Error! open_filter_libis NULL!");
			return -EFAULT;
		} else {
			cb_filter.printf = (MEDIALIB_CALLBACK_FUN_PRINTF)printk;
			version = _SD_GetAudioFilterVersions(&cb_filter);
			ak_pcm_info_ex("filter lib open success, version:%s", version);
		}
	}
		
	if (set_filter_lib_param(pcm, eq_param)) {
		ak_pcm_err_ex("Error! set_filter_lib_param error!");
		return -EFAULT;
	}

	memcpy(peqpara, eq_param, sizeof(T_S32) * EQ_ARRAY_NUMBER);

	if (!pcm->eq_enable)
		pcm->eq_enable = 1;

    return 0;
}

void aec_enable_eq(struct akpcm *pcm, int eq_enable)
{
	T_AUDIO_FILTER_CB_FUNS cb_filter={0};
	T_S8 * version = NULL;

	if (pcm->eq_enable == eq_enable)
		return;
	
	if (eq_enable) {
		if (!pcm->filter_handle) {
			if (open_filter_lib(pcm)) {
				ak_pcm_err_ex("Error! open_filter_libis NULL!");
				return;
			} else {
				cb_filter.printf = (MEDIALIB_CALLBACK_FUN_PRINTF)printk;
				version = _SD_GetAudioFilterVersions(&cb_filter);
				ak_pcm_info_ex("filter lib open success, version:%s", version);

				if (set_filter_lib_param(pcm, peqpara)) {
					ak_pcm_err_ex("Error! set_filter_lib_param error!");
					return;
				}
			}
		} 
		pcm->eq_enable = 1;
	} else {
		pcm->eq_enable = 0;
		close_filter_lib(pcm);
	}
	
	ak_pcm_info_ex("eq enable = %d", pcm->eq_enable);
}

void aec_set_dump_type(struct akpcm *pcm, int dump_type){
	if (dump_type < 0 || dump_type > AEC_DUMP_DEBUG)
		return;
	pcm->aec_dump_type = dump_type;
	ak_pcm_debug("dump type= %d", pcm->aec_dump_type);
}

