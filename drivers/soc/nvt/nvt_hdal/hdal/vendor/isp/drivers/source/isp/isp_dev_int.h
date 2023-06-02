#ifndef _ISP_DEV_INT_H_
#define _ISP_DEV_INT_H_

#include "isp_dev.h"

//=============================================================================
// struct & definition
//=============================================================================
#define FPS_TIMER_INTERVAL      3       // sec
#define KTHREAD_EVENT_TIME_OUT  600     // ms
#define IOCTL_BUF_SIZE          8192+2  // To match ISPT_MEMORY_INFO
#define SYNC_INFO_BUF           24
#define WAIT_VD_TIMOUT          1000    // ms

extern UINT32 isp_sync_id_sie[ISP_ID_MAX_NUM];
extern UINT32 isp_sync_id_ipp[ISP_ID_MAX_NUM];
extern UINT32 isp_sync_id_enc[ISP_ID_MAX_NUM];

typedef struct ISP_SYNC_INFO {
	ISP_AE_STATUS ae_status;
	UINT32 total_gain;
	UINT32 dgain;
	UINT32 lv;
	UINT32 lv_base;
	UINT32 cgain[3];
	UINT32 ct;
	UINT32 final_cgain[3];
} ISP_SYNC_INFO;

typedef struct _ISP_DEV_INFO {
	// algorithm module interfaces
	ISP_MODULE *isp_ae;
	ISP_MODULE *isp_awb;
	ISP_MODULE *isp_af;
	ISP_MODULE *isp_dr;
	ISP_MODULE *isp_iq;
	ISP_MODULE *isp_ext;

	// statistics buffer
	ISP_CA_RSLT *ca_rslt[ISP_ID_MAX_NUM];
	ISP_LA_RSLT *la_rslt[ISP_ID_MAX_NUM];
	ISP_VA_RSLT *va_rslt[ISP_ID_MAX_NUM];
	ISP_VA_INDEP_RSLT *va_indep_rslt[ISP_ID_MAX_NUM];
	ISP_MD_RSLT *md_rslt[ISP_ID_MAX_NUM];
	ISP_HISTO_RSLT *histo_rslt[ISP_ID_MAX_NUM];
	ISP_EDGE_STCS edge_stcs[ISP_ID_MAX_NUM];

	// ref image buffer
	ISP_IPE_SUBOUT_BUF *ipe_subout_buf[ISP_ID_MAX_NUM];

	// sync item
	// NOTE: Start of frame cnt SyncInfo
	ISP_SYNC_INFO sync_info[ISP_ID_MAX_NUM][SYNC_INFO_BUF];
	// NOTE: end of frame cnt SyncInfo
	UINT32 capture_total_gain[ISP_ID_MAX_NUM];
	UINT32 capture_dgain[ISP_ID_MAX_NUM];
	UINT32 shdr_frame_num[ISP_ID_MAX_NUM];
	UINT32 shdr_ev_ratio[ISP_ID_MAX_NUM][ISP_SEN_MFRAME_MAX_NUM];
	ISP_AE_INIT_INFO ae_init_info[ISP_ID_MAX_NUM];
	ISP_AWB_INIT_INFO awb_init_info[ISP_ID_MAX_NUM];
	ISP_IQ_INIT_INFO iq_init_info[ISP_ID_MAX_NUM];

	CTL_SIE_CA_PARAM sie_ca[ISP_ID_MAX_NUM];

	// motor control table
	CTL_MTR_DRV_TAB *mrt_drv_tab;

	#if defined(__KERNEL__)
	// synchronization mechanism
	wait_queue_head_t wait_vd[ISP_ID_MAX_NUM];

	// frame rate
	struct timer_list fps_timer;
	UINT32 frame_cnt;
	UINT32 prev_tick;
	UINT32 fps;
	UINT32 frame_no;

	// local variable
	UINT32 ioctl_buf[IOCTL_BUF_SIZE];

	// synchronization mechanism
	struct semaphore api_mutex;
	struct semaphore ioc_mutex;
	struct semaphore proc_mutex;
	#endif
} ISP_DEV_INFO;

//=============================================================================
// extern functions
//=============================================================================
extern void isp_dev_reset_item_valid(ISP_ID id, UINT32 frame_cnt);
#if defined(__FREERTOS)
extern ISP_DEV_INFO *isp_get_dev_info(void);
#endif
extern INT32 isp_dev_construct(ISP_DEV_INFO *pdev_info);
extern void isp_dev_deconstruct(ISP_DEV_INFO *pdev_info);

#endif
