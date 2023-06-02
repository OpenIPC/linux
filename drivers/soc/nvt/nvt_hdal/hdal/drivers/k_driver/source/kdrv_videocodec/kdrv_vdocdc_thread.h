#ifndef _KDRV_VDOCDC_THREAD_H_
#define _KDRV_VDOCDC_THREAD_H_

#include "kwrap/type.h"
#include "kwrap/task.h"
#include "kdrv_type.h"

#include "kdrv_videoenc/kdrv_videoenc.h"
#include "kdrv_videodec/kdrv_videodec.h"

#define VDOCDC_JOB_MAX	(4)

typedef enum _KDRV_VDOCDC_MODE_{
	VDOCDC_ENC_MODE = 0,
	VDOCDC_DEC_MODE,
	VDOCDC_NOT_SUPPORT
} KDRV_VDOCDC_MODE;

typedef struct _KDRV_VDOCDC_LLC_{
	UINT32 va[VDOCDC_JOB_MAX];
	UINT32 pa[VDOCDC_JOB_MAX];
	UINT32 size;
	UINT8  job_st_idx;
	UINT8  run_st_idx;
	UINT8  cur_idx;
	UINT32 id[VDOCDC_JOB_MAX];
	KDRV_VDOCDC_MODE codec_mode[VDOCDC_JOB_MAX];
	KDRV_CALLBACK_FUNC *callback[VDOCDC_JOB_MAX];
	void *user_data[VDOCDC_JOB_MAX];
	void *p_var[VDOCDC_JOB_MAX];
	void *p_param[VDOCDC_JOB_MAX];
}KDRV_VDOCDC_LLC;

int kdrv_vdocdc_create_thread(void);
int kdrv_vdocdc_remove_thread(void);
int kdrv_vdocdc_add_job(KDRV_VDOCDC_MODE codec_mode, UINT32 id, UINT32 uiVaApbAddr, KDRV_CALLBACK_FUNC *p_cb_func, void *user_data, void *p_var, void *p_param);
void kdrv_vdocdc_get_interrupt(UINT32 interrupt);

void kdrv_vdocdc_init_llc(void);
int kdrv_vdocdc_set_llc_mem(UINT32 addr, UINT32 size);
UINT32 kdrv_vdocdc_get_llc_mem(void);

#endif // _KDRV_VDOCDC_THREAD_H_