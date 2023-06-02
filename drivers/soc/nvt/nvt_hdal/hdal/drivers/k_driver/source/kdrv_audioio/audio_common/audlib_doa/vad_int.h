
#ifndef _VAD_INT_H
#define _VAD_INT_H

#ifndef __KERNEL__
#include <stdint.h>
#include <stddef.h>
#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "kdrv_audioio/audlib_src.h"
#else
#include <linux/delay.h>
#include <mach/rcw_macro.h>
#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "kdrv_audioio/audlib_src.h"

#include "audlib_doa_dbg.h"

#endif

#define ASRC_READY	ENABLE
#define VAD_48KHZ_EN	DISABLE

// If enabled, the vad input size is fixed as 10ms
#define VAD_REDUCE_STACK_SZ	ENABLE

typedef struct WebRtcVadInst VadInst;

typedef struct {
  int32_t S_48_24[8];
  int32_t S_24_24[16];
  int32_t S_24_16[8];
  int32_t S_16_8[8];
} WebRtcSpl_State48khzTo8khz;


enum { kNumChannels = 6 };  // Number of frequency bands (named channels).
enum { kNumGaussians = 2 };  // Number of Gaussians per channel in the GMM.
enum { kTableSize = kNumChannels * kNumGaussians };
enum { kMinEnergy = 10 };  // Minimum energy required to trigger audio signal.

typedef struct VadInstT_
{
    int vad;
    int32_t downsampling_filter_states[4];
    WebRtcSpl_State48khzTo8khz state_48_to_8;
    int16_t noise_means[kTableSize];
    int16_t speech_means[kTableSize];
    int16_t noise_stds[kTableSize];
    int16_t speech_stds[kTableSize];
    // TODO(bjornv): Change to |frame_count|.
    int32_t frame_counter;
    int16_t over_hang; // Over Hang
    int16_t num_of_speech;
    // TODO(bjornv): Change to |age_vector|.
    int16_t index_vector[16 * kNumChannels];
    int16_t low_value_vector[16 * kNumChannels];
    // TODO(bjornv): Change to |median|.
    int16_t mean_value[kNumChannels];
    int16_t upper_state[5];
    int16_t lower_state[5];
    int16_t hp_filter_state[4];
    int16_t over_hang_max_1[3];
    int16_t over_hang_max_2[3];
    int16_t individual[3];
    int16_t total[3];

    int init_flag;

} VadInstT;

#define WEBRTC_SPL_WORD16_MAX       32767


#define WEBRTC_SPL_MUL_16_16(a, b) \
    ((int32_t) (((int16_t)(a)) * ((int16_t)(b))))
#define WEBRTC_SPL_MUL_16_16_RSFT(a, b, c) \
    (WEBRTC_SPL_MUL_16_16(a, b) >> (c))
#define WEBRTC_SPL_MUL(a, b) \
    ((int32_t) ((int32_t)(a) * (int32_t)(b)))





int WebRtcVad_Process(VadInst* handle, int fs, const int16_t* audio_frame, int frame_length);
int WebRtcVad_InitCore(VadInstT* self);


#endif
