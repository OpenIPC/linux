#ifndef _AKPCM_AEC_DUMP_H_
#define _AKPCM_AEC_DUMP_H_

enum aec_dump_id {
	AEC_DUMP_NEAR,
	AEC_DUMP_FAR,
	AEC_DUMP_RES,
	AEC_DUMP_AFTER_EQ,
	AEC_DUMP_MAX
};

enum aec_dump_type {
	AEC_DUMP_NULL = 0,
	AEC_DUMP_FILE,
	AEC_DUMP_DEBUG
};

struct aec_dump_info {
	unsigned char id;
	unsigned char *data;
	unsigned long size;
    int sample_rate;
};

void print_isr(int dump_type);

void putstr_isr(int dump_type, char *str);

void aec_dump_complete(int dump_type);

void aec_dump(int dump_type, struct aec_dump_info *info);

void aec_dump_sync(struct akpcm *pcm);

#endif
