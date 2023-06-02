#ifndef _PD_SHM_H_
#define _PD_SHM_H_
#include "hd_type.h"

#define PD_SHMSZ   0x1000
#define PD_SHM_KEY 8001
#define PD_MAX_NUM 32

typedef struct _PD_SHM_RESULT
{
	int     category;
	float 	score;
	int 	x1;
	int 	y1;
	int 	x2;
	int 	y2;
}PD_SHM_RESULT;

typedef struct _PD_SHM_INFO_ {
	unsigned int  init_tag;
	HD_DIM        pd_path;
	HD_DIM        enc_path[3];
	unsigned int  pd_num;
	PD_SHM_RESULT pd_results[PD_MAX_NUM];
	int           exit;
} PD_SHM_INFO;


STATIC_ASSERT(sizeof(PD_SHM_INFO) <= PD_SHMSZ);

#endif
