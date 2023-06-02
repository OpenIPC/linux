#ifndef __AKPCM_AEC_H__
#define __AKPCM_AEC_H__

#include <mach-anyka/anyka_types.h>

#include "akpcm_defs.h"

/////////////////////////////////////////////////////
// AEC params
#define NN 128 // do not change
#define AEC_FRAME_BYTES (NN << 1)
#define TAIL (NN*4) // aec filter length
#define AEC_DA_THRESH  AK32Q15(0.3) // DA数字限幅，<1
#define AEC_DA_HW_GAIN  5 // 0 ~ HP_GAIN_MAX
#define AEC_AD_HW_GAIN  4 // 0 ~ MAX_MIC_GAIN
#define AEC_AGC_LEVEL   AK32Q15(0.75) // 自动增益控制的目标幅度，<1
#define FAR_FRAME_SIZE  (AEC_FRAME_BYTES << 1)

/* aec frame is per 16 ms, schedule to wait 10ms. unit: jiffies. */
#define AEC_TASK_TIMER  (10*HZ/1000)
#define EQ_ARRAY_NUMBER		54

static const T_S16 aec_volume[6] = { // Q10 format
	AK16Q10(0.178), // 0, -15dB
	AK16Q10(0.251), // 1, -12dB
	AK16Q10(0.355), // 2,  -9dB
	AK16Q10(0.501), // 3,  -6dB
	AK16Q10(0.708), // 4,  -3dB
	AK16Q10(0.999), // 5, 0dB
};

typedef enum {
	AEC_EVENT_NONE = 0,             /* 0 none, general trigger */
	AEC_EVENT_OPEN,                 /* 1 */
	AEC_EVENT_ADC_START,            /* 2 */
	AEC_EVENT_DAC_START,            /* 3 */
	AEC_EVENT_AEC_START,            /* 4 */
	AEC_EVENT_ADC_STOP,             /* 5 */
	AEC_EVENT_DAC_STOP,             /* 6 */
	AEC_EVENT_AEC_STOP,             /* 7 */
}t_aec_event;

#define aec_need_playback(pcm) \
    (((pcm)->aec_status >= AEC_STATUS_SYNC_ADC) \
    && ((pcm)->aec_status <= AEC_STATUS_AEC_WORKING))

/*
 * playback & capture helper routine
 * get how many data to be processed by AEC in playback or capture buffer
 */
unsigned int get_aec_bytes(struct akpcm_runtime *rt);

static inline void update_aec_status(struct akpcm *pcm, e_aec_status status)
{
    pcm->aec_status = status;
}

int aec_init(struct akpcm *pcm);
void aec_exit(struct akpcm *pcm);

int aec_set_param(struct akpcm *pcm, int value);
int aec_event_handler(struct akpcm *pcm, t_aec_event event);

T_VOID aec_set_da_volume(struct akpcm *pcm, T_S16 *data, T_U16 data_len);

void aec_set_nr_max(struct akpcm *pcm, int enable_max);
int aec_get_echo_params(struct akpcm *pcm, T_U8 *aec_param);
int aec_set_echo_params(struct akpcm *pcm, T_U8 *aec_param);

int aec_set_eq_params(struct akpcm *pcm, T_S32 *eq_param);
void aec_enable_eq(struct akpcm *pcm, int eq_enable);

void aec_set_dump_type(struct akpcm *pcm, int dump_type);

#endif
