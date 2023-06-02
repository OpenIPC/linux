#ifndef __AKPCM_DEFS_H__
#define __AKPCM_DEFS_H__

#include <plat/l2.h>
#include <mach-anyka/echo_interface.h>
#include <mach-anyka/sdfilter.h>
#include <mach/gpio.h>
#include <linux/cdev.h>

#include "akpcm.h"

#define DEBUG_BYTES

#if 1
#define ak_pcm_debug(fmt, arg...) \
	printk(KERN_DEBUG "@@@ "fmt" @@@\n", ##arg)
#define ak_pcm_debugx(fmt, arg...) \
	printk(KERN_DEBUG "@@@ "fmt" @@@\n\n", ##arg)
#define ak_pcm_debug_ex(fmt, arg...) \
	printk(KERN_DEBUG "@@@ [%s:%d]: "fmt" @@@\n", __func__, __LINE__, ##arg)
#define ak_pcm_debug_exx(fmt, arg...) \
	printk(KERN_DEBUG "@@@ [%s:%d]: "fmt" @@@\n\n", __func__, __LINE__, ##arg)
#else
#define ak_pcm_debug(fmt, arg...)
#define ak_pcm_debug_ex(fmt, arg...)
#define ak_pcm_debug_exx(fmt, arg...)
#endif

#define ak_pcm_func(fmt, arg...) \
	printk(KERN_INFO "@@@ [%s]: "fmt" @@@\n", __func__, ##arg)
#define ak_pcm_func_exx(fmt, arg...) \
        printk(KERN_INFO "@@@ [%s]: "fmt" @@@\n\n", __func__, ##arg)

#define ak_pcm_info(fmt, arg...) \
	printk(KERN_ERR "@@@ "fmt" @@@\n", ##arg)
#define ak_pcm_info_ex(fmt, arg...) \
	printk(KERN_ERR "@@@ [%s:%d]: "fmt" @@@\n", __func__, __LINE__, ##arg)
#define ak_pcm_err(fmt, arg...) \
	printk(KERN_ERR "@@@ error! "fmt" @@@\n", ##arg)
#define ak_pcm_err_ex(fmt, arg...) \
	printk(KERN_ERR "@@@ [%s:%d] error! "fmt" @@@\n", __func__, __LINE__, ##arg)
#define ak_pcm_warn(fmt, arg...) \
	printk(KERN_ERR "@@@ warn! "fmt" @@@\n", ##arg)
#define ak_pcm_warn_ex(fmt, arg...) \
	printk(KERN_ERR "@@@ [%s:%d] warn! "fmt" @@@\n\n", __func__, __LINE__, ##arg)
#define ak_pcm_assert(exp) \
	((exp)? 1 : ak_pcm_info_ex("assert"))

#define USE_FORMATS             (AKPCM_FMTBIT_U8 | AKPCM_SMPL_BIT_U16)
#define USE_RATE                (AKPCM_RATE_CONTINUOUS | AKPCM_RATE_ALL)
#define USE_RATE_MIN            8000
#define USE_RATE_MAX            48000
#define PLAY_PERIOD_BYTES_MIN   512
#define PLAY_PERIOD_BYTES_MAX   65536
#define PLAY_PERIODS_MIN        4
#define PLAY_PERIODS_MAX        4096
#define DELAYS_FOR_CLOSE_DAC    (HZ*30)
#define CAPT_PERIOD_BYTES_MIN   512
#define CAPT_PERIOD_BYTES_MAX   32768
#define CAPT_PERIODS_MIN        4
#define CAPT_PERIODS_MAX        2048
#define MAX_TIMESTAMP_CNT       80

/* GAIN chennels(ANALOG) */
#define MIXER_VOL_HP            0
#define MIXER_VOL_LI            1
#define MIXER_VOL_MIC           2
#define MIXER_VOL_END           2
/* GAIN chennels's default value */
#define DEFAULT_HPVOL           1
#define DEFAULT_LINEINVOL       0xF
#define DEFAULT_MICVOL          7
/* sources(OUTPUTS) */
#define MIXER_SRC_HP            0
#define MIXER_SRC_LO            1
#define MIXER_SRC_ADC2          2
#define MIXER_SRC_END           2
/* devices dectect switch(OUTPUTS) */
#define MIXER_SWITCH_HPDET      0
#define MIXER_SWITCH_END        0
/* devices for playback */
#define PLAYDEV_HP              (AKPCM_PLAYDEV_HP)
#define PLAYDEV_LO              (AKPCM_PLAYDEV_LO)
#define PLAYDEV_AUTO            (AKPCM_PLAYDEV_AUTO)
#define PLAYDEV_MSK             (AKPCM_PLAYDEV_HP)
/* devices for capture */
#define CPTRDEV_MIC             (AKPCM_CPTRDEV_MIC)
#define CPTRDEV_LI              (AKPCM_CPTRDEV_LI)
#define CPTRDEV_AUTO            (AKPCM_CPTRDEV_AUTO)
#define CPTRDEV_MSK             (AKPCM_CPTRDEV_MIC | AKPCM_CPTRDEV_LI)
/* supported devices for playback */
#define EXIST_OUT_DEV_SP        (1<<1)
#define EXIST_OUT_DEV_HP        (1<<0)
#define EXIST_OUT_DEV_ALL       (EXIST_OUT_DEV_SP|EXIST_OUT_DEV_HP)

enum {
	SWITCH_DET_NULL = 0,
	SWITCH_DET_SPEAKER = 1,
	SWITCH_DET_HEADPHONE = 2
};

enum {
	CAPTURE_DET_NULL = 0,
	CAPTURE_DET_LINEIN = 1,
};

/* AEC utils interface */
typedef enum {
	AEC_STATUS_IDLE            = 0,  /* not started */
	AEC_STATUS_AEC_OPENED      = 1,  /* aec opened */
	AEC_STATUS_WAIT_ADC        = 2,  /* dac is working, wait for adc */
	AEC_STATUS_SYNC_ADC        = 3,  /* to start adc with sync condition */
	AEC_STATUS_SYNC_DAC        = 4,  /* to start dac with sync condition */
	AEC_STATUS_AEC_WORKING     = 5,  /* aec is working */
	AEC_STATUS_NR_OPENED       = 6,  /* only nr is opened */
	AEC_STATUS_NR_WORKING      = 7,  /* only nr is working */
}e_aec_status;

/* bit[0]-opened; bit[1]-prepared */
enum status_bit {
    STATUS_BIT_OPENED = 0x00,
    STATUS_BIT_PREPARED,
};

/* bit[0]-strm; bit[1]-DMA; bit[2] suspend */
enum pcm_io_bit {
    IO_BIT_STREAM = 0x00,
    IO_BIT_DMA,
    IO_BIT_SUSPEND
};

typedef l2_dma_transfer_direction_t tL2DMA_DIR;

struct akpcm_runtime {
	struct akpcm_pars cfg;
	unsigned int hw_ptr;  /* hardware ptr (volatile ???) */
	unsigned int app_ptr; /* application ptr (volatile ???) */
	unsigned int aec_ptr; /* AEC ptr */
	spinlock_t ptr_lock;  /* lock to protect ptr */
	struct mutex lock;

	/* -- SW parameters -- */
	unsigned int buffer_bytes;
	unsigned int boundary;

	/* -- DMA -- */
	unsigned char *dma_area; /* DMA area(vaddr) */
	dma_addr_t dma_addr; /* physical bus address */

	/* -- AEC/NR/AGC -- */
	int enable_nr;  // noise reduction
	int enable_agc; // automatic gain control
	unsigned long ts;

	/* start data check time */
	u_int64_t start_time;
	u_int64_t end_time;
};

struct akpcm {
	struct cdev cdevice[2];
	struct device *dev;
	struct akpcm_features *play_hw;
	struct akpcm_features *capt_hw;
	struct akpcm_runtime *play_rt;
	struct akpcm_runtime *cptr_rt;
	struct mutex io_lock;

	/* AEC */
	int enable_aec; // accoustic echo cancellation
	int enable_nr;
	int enable_agc;
	unsigned long app_flag;
	int app_enable_aec;
	int app_enable_nr;
	int app_enable_agc;
	int aec_dump_type;
	int eq_enable;
	e_aec_status aec_status;
	struct mutex aec_status_lock;
	wait_queue_head_t aec_sync_wq;
	struct workqueue_struct *aec_event_wq;
	struct work_struct aec_event_work;
	T_VOID *aec_filter;
	T_VOID *filter_handle;
	T_AEC_BUF aec_bufs;
	T_S16 *aec_out_buf; // temp buf for aec result
	struct task_struct *aec_thread;
	wait_queue_head_t aec_wq;
	bool aec_thread_run;
	bool aec_data_triger;
	struct task_struct *aec_prio_supervisor_thread;
	wait_queue_head_t aec_prio_wq;
	bool aec_supervisor_thread_run;
	/* -1: to lower down, 0: normal, 1: to raise high, 2: raised */
	int aec_high_prio;
	/* far NR */
	T_VOID *far_filter;
	T_AEC_BUF far_bufs;
	T_S8 *far_frame_buf;    // temp buf for dac frame process
	int far_frame_off;      // write offset of far_frame_buf

	struct completion play_completion; /* for L2DMA */
	struct completion capt_completion; /* for L2DMA */
	wait_queue_head_t play_wq;
	wait_queue_head_t capt_wq;
	int playdev;
	int cptrdev;
	int dev_manual;

	void __iomem *Analog_Regs;
	void __iomem *I2S_Regs;
	void __iomem *ADC2_Regs;

	u8 L2BufID_DAC;
	u8 L2BufID_ADC2;

	unsigned long playback_sm_flag; //bit[0]-opened; bit[1]-prepared
	unsigned long capture_sm_flag;  //bit[0]-opened; bit[1]-prepared
	unsigned long outflag; //bit[0]-strm; bit[1]-DMA; bit[2] suspend
	unsigned long inflag;  //bit[0]-strm; bit[1]-DMA; bit[2] suspend

	int mixer_volume[MIXER_VOL_END+1];
	int mixer_source[MIXER_SRC_END+1];

	/* EXIST_OUT_DEV_SP(b[1]), EXIST_OUT_DEV_HP(b[0]) */
	int bfExistPlayDev;

	int play_dectect;
	struct gpio_info HPDet_gpio; // headphone detect GPIO
	int HPDet_irq;
	int HPDet_on_value;
	int HPon_irqType;
	int HPoff_irqType;

	struct gpio_info SPdown_gpio;// speaker shutdown GPIO

	struct gpio_info hpmute_gpio;
	int hpmute_EnValue;  // value for hpmute enable
	int hpmute_DisValue; // value for hpmute disable

	int bIsHPmuteUsed; // whether to use hardware de-pipa or not

	struct delayed_work DelayWork;
	struct timer_list timer_stop_output;

	bool IsOutputing;

	struct gpio_info linindet_gpio;// lineint detecting GPIO
	int lininon_irqType;
	int lininoff_irqType;
	int capture_dectect;
	struct delayed_work LininWork;
	int linindet_irq;
	unsigned long timestamp[MAX_TIMESTAMP_CNT];

	u8 aec_has_pend_data;
	u8 dac_endwith_zero;

	int dac_opened_count;
};

//extern u_int64_t ak39_gethrtick(void);

/*
 * convert a stereo buffer to mono format
 * mono samples will be put into first half of buf
 */
static inline void stereo_to_mono(void *dest, void *src, int buf_size)
{
	int i;

	for (i=0; i<buf_size/4; i++) {
		((T_S16*)dest)[i] = ((T_S16*)src)[2*i];
	}
}

/*
 * covert a mono buffer to stereo format
 * mono samples shall be located at first half of buf
 */
static inline void mono_to_stereo(void *dest, void *src, int buf_size)
{
	int i;

	for (i=buf_size/4; i>0; i--) {
		((T_S16*)dest)[2*i-1] = ((T_S16*)src)[i-1];
		((T_S16*)dest)[2*i-2] = ((T_S16*)src)[i-1];
	}
}

static inline unsigned long get_timestamp(void)
{
	unsigned long ul;

	ul = jiffies;
	if (ul >= INITIAL_JIFFIES)
		ul -= INITIAL_JIFFIES;
	else
		ul = (~(unsigned long)0) - INITIAL_JIFFIES + ul;
	ul = jiffies_to_msecs(ul);

	return ul;
}

void akpcm_set_mic_gain(unsigned long gain);
void akpcm_set_hp_gain(unsigned long gain);

struct akpcm* get_akpcm_ptr(void);

void akpcm_notify_cb(T_U32 code);
void* akpcm_malloc_cb(unsigned long size);
void akpcm_free_cb(void * mem);

/**
 * set source for OUTPUT(HP or ADC2)
 * author: panqihe 2014-06-25
 * @addr: MIXER_SRC_HP or MIXER_SRC_ADC2
 * @src: source
 */
void set_channel_source(struct akpcm *pcm, int addr, int src);

/**
 * change source for OUTPUT(HP or ADC2)
 * author panqihe 2014-06-25
 * @addr: MIXER_SRC_HP or MIXER_SRC_ADC2
 * @src: source
 * return int
 */
int change_channel_source(struct akpcm *pcm, int addr, int src);

static inline int ak39_spend_us(u_int64_t start, u_int64_t end)
{
	int spend = (int)(end - start) / 1000; // 12M ticks/s
    ak_pcm_assert(end >= start);

    return spend;
}

#endif
