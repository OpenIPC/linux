#ifndef _AI_LIB_H_
#define _AI_LIB_H_

#ifdef __KERNEL__

//#include "mach/fmem.h"
//#include <mach/rcw_macro.h>
#include <linux/string.h>

#include <linux/clk.h>
#include <linux/hrtimer.h>

#else


#endif

#include <kwrap/semaphore.h>
//#include "kwrap/flag.h"
#include "kdrv_ai.h"
#include "cnn_lib.h"
#include "nue_lib.h"
#include "nue2_lib.h"

#define KDRV_AI_HANDLE_MAX_NUM (1)

#define KDRV_AI_LOCK            FLGPTN_BIT(0)  //(ofs bit << id)
//#define KDRV_AI_FMD             FLGPTN_BIT(8)
#define KDRV_AI_CNN_FMD         FLGPTN_BIT(9)
#define KDRV_AI_NUE_FMD         FLGPTN_BIT(10)
#define KDRV_AI_TIMEOUT         FLGPTN_BIT(12)
#define KDRV_AI_HDL_UNLOCK      FLGPTN_BIT(13)
#define KDRV_AI_RESET         	FLGPTN_BIT(14)
#define KDRV_AI_NUE2_FMD        FLGPTN_BIT(15)
#define KDRV_AI_CNN2_FMD        FLGPTN_BIT(16)
#define KDRV_AI_LOCK_BITS       0x000000ff
#define KDRV_AI_INIT_BITS       (KDRV_AI_LOCK_BITS | KDRV_AI_HDL_UNLOCK)

#define KDRV_AI_FLG_IMPROVE     1

#if defined(__FREERTOS)
extern ID     	  FLG_ID_KDRV_AI[AI_ENG_TOTAL];
#else
extern FLGPTN     FLG_ID_KDRV_AI[AI_ENG_TOTAL];
#endif

extern SEM_HANDLE SEMID_KDRV_AI[AI_ENG_TOTAL];

#if KDRV_AI_FLG_IMPROVE
extern SEM_HANDLE ai_sem_id[AI_ENG_TOTAL][KDRV_AI_HANDLE_MAX_NUM];
extern SEM_HANDLE ai_sem_param_id[AI_ENG_TOTAL];
extern SEM_HANDLE ai_sem_ch_id[KDRV_AI_HANDLE_MAX_NUM];
extern SEM_HANDLE ai_handle_sem_id[AI_ENG_TOTAL];
extern SEM_HANDLE ai_ioctl_sem_id;
#endif

typedef ER(*KDRV_AI_SET_FP)(UINT32, void *);
typedef ER(*KDRV_AI_GET_FP)(UINT32, void *);

extern void kdrv_ai_install_id(void);
extern void kdrv_ai_init(void);
extern void kdrv_ai_uninstall_id(void);

extern int kdrv_ai_do_vmalloc_for_netflowsample(VOID);

extern UINT32 g_kdrv_ai_ll_base_addr[AI_ENG_TOTAL];

extern BOOL g_ai_isr_trig;
/**
    AI KDRV handle structure
*/
#define KDRV_AI_HANDLE_LOCK    0x00000001

typedef struct _KDRV_AI_HANDLE {
	UINT32 entry_id;
	UINT32 flag_id;
	UINT32 lock_bit;
	UINT32 sts;
	KDRV_AI_ISRCB isrcb_fp;
} KDRV_AI_HANDLE;

/**
    AI KDRV structure
*/
typedef struct {
	KDRV_AI_TRIG_MODE mode;
	UINT32 app_parm_cnt;
	KDRV_AI_APP_HEAD app_parm;
	UINT32 ll_parm_cnt;
	KDRV_AI_LL_HEAD ll_parm;
} KDRV_AI_PRAM, *pKDRV_AI_PRAM;

/**
    AI time structure
*/
typedef struct {
#ifdef __KERNEL__
	struct hrtimer htimer;
	ktime_t kt_periode;
#elif defined(__FREERTOS)
#else
   	BOOL is_open_timer;
	SWTIMER_ID   timer_id;
	SWTIMER_CB   timer_cb;
#endif
} KDRV_AI_TIME, *pKDRV_AI_TIME;


ER kdrv_ai_cnn_trigger(BOOL cnn_id, CNN_OPMODE mode, CNN_PARM *p_parm);
ER kdrv_ai_cnn_done(BOOL cnn_id, KDRV_AI_HANDLE *p_handle);
ER kdrv_ai_nue_trigger(NUE_OPMODE mode, NUE_PARM *p_parm);
ER kdrv_ai_nue_done(KDRV_AI_HANDLE *p_handle);
ER kdrv_ai_nue2_trigger(NUE2_OPMODE mode, NUE2_PARM *p_parm);
ER kdrv_ai_nue2_done(KDRV_AI_HANDLE *p_handle);
ER kdrv_ai_softmax_trig(KDRV_AI_SOFTMAX_PARM *p_parm);
ER kdrv_ai_softmax_done(KDRV_AI_HANDLE *p_handle);
ER kdrv_ai_anchor_trig(KDRV_AI_ANCHOR_PARM *p_parm);
ER kdrv_ai_anchor_done(KDRV_AI_HANDLE *p_handle);
ER kdrv_ai_preproc_trig(KDRV_AI_PREPROC_PARM *p_parm);
ER kdrv_ai_preproc_done(KDRV_AI_HANDLE *p_handle);
ER kdrv_ai_nue2_ll_trig(UINT32 pa);
ER kdrv_ai_nue2_ll_done(KDRV_AI_HANDLE *p_handle);

ER kdrv_ai_neural_trig(BOOL cnn_id, KDRV_AI_NEURAL_PARM *p_parm);
ER kdrv_ai_neural_done(BOOL cnn_id, KDRV_AI_NEURAL_PARM *p_parm, KDRV_AI_HANDLE *p_handle);
ER kdrv_ai_roipool_trig(KDRV_AI_ROIPOOL_PARM *p_parm);
ER kdrv_ai_roipool_done(KDRV_AI_HANDLE *p_handle);
ER kdrv_ai_svm_trig(KDRV_AI_SVM_PARM *p_parm);
ER kdrv_ai_svm_done(KDRV_AI_SVM_PARM *p_parm, KDRV_AI_HANDLE *p_handle);
ER kdrv_ai_fc_trig(KDRV_AI_FC_PARM *p_parm);
ER kdrv_ai_fc_done(KDRV_AI_FC_PARM *p_parm, KDRV_AI_HANDLE *p_handle);
ER kdrv_ai_permute_trig(KDRV_AI_PERMUTE_PARM *p_parm);
ER kdrv_ai_permute_done(KDRV_AI_HANDLE *p_handle);
ER kdrv_ai_reorg_trig(KDRV_AI_REORG_PARM *p_parm);
ER kdrv_ai_reorg_done(KDRV_AI_HANDLE *p_handle);
ER kdrv_ai_cnn_ll_trig(BOOL cnn_id, UINT32 pa);
ER kdrv_ai_cnn_ll_done(BOOL cnn_id, KDRV_AI_HANDLE *p_handle);
ER kdrv_ai_nue_ll_trig(UINT32 pa);
ER kdrv_ai_nue_ll_done(KDRV_AI_HANDLE *p_handle);

#if LL_SUPPORT_ROI
ER kdrv_ai_link_ll_update_addr(AI_USR_LAYER_INFO* usr_layer_info, UINT32 net_id);
ER kdrv_ai_link_uninit(UINT32 net_id);
UINT32 kdrv_ai_get_drv_base_va(UINT32 net_id);
UINT32 kdrv_ai_get_drv_base_pa(UINT32 net_id);
UINT32 kdrv_ai_get_drv_io_buf_size(UINT32 net_id);
#endif

UINT32 kdrv_ai_drv_get_net_supported_num(VOID);
#if defined(__FREERTOS)
UINT32 kdrv_ai_drv_get_clock_freq(UINT8 clk_idx);
#endif

#endif //_KDRV_AI_ID_H
