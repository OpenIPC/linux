#ifndef _AUDLIB_ANR_H
#define _AUDLIB_ANR_H

#include "kdrv_type.h"

#define MAX_STREAM  (16)
#define MAX_CHANNEL (2)

#define NOISE_DETECTING     (0)
#define NOISE_UPDATING      (1)
#define NOISE_UPDATED       (2)

struct ANR_CONFIG {
	// Memory associated
	int memory_needed;      // returned by audlib_anr_pre_init()
	void *p_mem_buffer;      // alloc/set outside ANR library & used in audlib_anr_init()

	// User Configurations
	int blk_size_w;
	int sampling_rate;
	int stereo;
	int nr_db;           // SNR improvement
	int hpf_cutoff_freq;
	int bias_sensitive;

	// Professional Configurations
	int noise_est_hold_time;
	int tone_min_time;
	int spec_bias_low;
	int spec_bias_high;
	int max_bias_limit;
	int m_curve_n1_level;
	int m_curve_n2_level;

	// The following fields are filled by audlib_anr_detect() or set by
	// customer in audlib_anr_init(). If audlib_anr_detect() is called "after"
	// audlib_anr_init(), the detected results will update user configurations.
	unsigned int max_bias;
	unsigned int default_bias;
	unsigned int default_eng;
	unsigned short default_spec[513];
};

extern int  audlib_anr_get_version(void);
extern int  audlib_anr_pre_init(struct ANR_CONFIG *);
extern int  audlib_anr_init(int *, struct ANR_CONFIG *);
extern void audlib_anr_set_snri(int);
extern int  audlib_anr_get_snri(void);
extern void audlib_anr_detect_reset(int);
extern int  audlib_anr_detect(int, short *, struct ANR_CONFIG *);
extern int  audlib_anr_run(int, short *, short *);
extern void audlib_anr_destroy(int *);

int kdrv_audlib_anr_init(void);


#endif
