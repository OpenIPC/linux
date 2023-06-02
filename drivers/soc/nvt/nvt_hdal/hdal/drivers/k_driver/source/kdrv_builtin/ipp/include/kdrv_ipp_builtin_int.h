#ifndef __KDRV_IPP_BUILTIN_INT_H__
#define __KDRV_IPP_BUILTIN_INT_H__

#include "kwrap/spinlock.h"
#include "kwrap/list.h"
#include "kwrap/task.h"

#include "kwrap/cpu.h"
#include "kwrap/perf.h"
#include "kdrv_type.h"
#include "kdrv_ipp_builtin.h"
#include "kdrv_ipp_builtin_dbg.h"
#include "kdrv_ipp_builtin_drawstr.h"
#include "kwrap/flag.h"
#include "kdrv_ipp_builtin_dbg_info.h"


/**
	BUILTIN IPP
*/
#define KDRV_IPP_BUILTIN_MSG_ENABLE (1)
#if KDRV_IPP_BUILTIN_MSG_ENABLE
#define KDRV_IPP_BUILTIN_DUMP(fmt, args...) DBG_DUMP(fmt, ##args)
#else
#define KDRV_IPP_BUILTIN_DUMP(fmt, args...)
#endif

#define KDRV_IPP_BUILTIN_ENG_BIT(eng) (1 << eng)


#define KDRV_IPP_BUILTIN_BUF_SIE 2

#define KDRV_IPP_BUILTIN_FMD_CB_NUM	(2)

/* CTL_IPP_FLOW_TYPE */
typedef enum {
	KDRV_IPP_BUILTIN_FLOW_UNKNOWN = 0,
	KDRV_IPP_BUILTIN_FLOW_RAW,
	KDRV_IPP_BUILTIN_FLOW_DIRECT_RAW,
	KDRV_IPP_BUILTIN_FLOW_CCIR,
	KDRV_IPP_BUILTIN_FLOW_DIRECT_CCIR,
	KDRV_IPP_BUILTIN_FLOW_IME_D2D,
	KDRV_IPP_BUILTIN_FLOW_IPE_D2D,
	KDRV_IPP_BUILTIN_FLOW_VR360,
	KDRV_IPP_BUILTIN_FLOW_DCE_D2D,
	KDRV_IPP_BUILTIN_FLOW_CAPTURE_RAW,
	KDRV_IPP_BUILTIN_FLOW_CAPTURE_CCIR,
	KDRV_IPP_BUILTIN_FLOW_MAX
} KDRV_IPP_BUILTIN_FLOW_TYPE;

#define KDRV_IPP_BUILTIN_FRC_BASE (4096)

typedef struct{

	UINT8 set_bit;
	UINT8 expected_bit[2];
	UINT8 w_idx;
	UINT8 r_idx;

}KDRV_IPP_BUILTIN_APPLY_CHK_CTL;

typedef struct {
	const CHAR *name;
	CHAR node[64];
	UINT32 src_sie_id_bit;
	UINT32 flow;
	UINT32 isp_id;
	KDRV_IPP_BUILTIN_BLK vprc_blk[KDRV_IPP_BUILTIN_PATH_ID_MAX][2];
	KDRV_IPP_BUILTIN_BLK vprc_shdr_blk;
	KDRV_IPP_BUILTIN_BLK vprc_ctrl;
	KDRV_IPP_BUILTIN_PRI_BUF pri_buf;
	KDRV_IPP_BUILTIN_FUNC func_en;
	KDRV_IPP_BUILTIN_DBG_FUNC dbg_func_en;
	KDRV_IPP_BUILTIN_PHY_OUT_INFO last_out_info;
	UINT32 _3dnr_ref_path;
	UINT32 fs_cnt;
	VOS_TICK fs_timestamp;
	VOS_TICK pre_fs_timestamp;

	KDRV_IPP_BUILTIN_IMG_INFO path_info[KDRV_IPP_BUILTIN_PATH_ID_MAX];
	void *p_cur_job;
	UINT8 in_buf_release_cnt;
	UINT8 out_buf_release_cnt;
	UINT8 get_phy_out_cnt;

	KDRV_IPP_BUILTIN_RATE_CTL frc[KDRV_IPP_BUILTIN_PATH_ID_MAX];
	KDRV_IPP_BUILTIN_PRI_MASK mask[KDRV_IPP_BUILTIN_PRI_MASK_NUM];

	KDRV_IPP_BUILTIN_DBG_INFO dbg_info;
	UINT8 input_disable_flag;
	KDRV_IPP_BUILTIN_APPLY_CHK_CTL ime_dram_end_ck;
	UINT8 get_isp_info_cnt;
	KDRV_IPP_BUILTIN_ISP_INFO isp_info;

} KDRV_IPP_BUILTIN_HDL;

typedef struct{

	KDRV_IPP_BUILTIN_HDL *p_hdl;
	KDRV_IPP_BUILTIN_DRAW_BUF draw_buf_info;
	BOOL frm_skip_flag;

}KDRV_IPP_BUILTIN_DBG_DRAW_INFO;

typedef struct {
	/* todo: check sie push job struct */
	KDRV_IPP_BUILTIN_HDL *p_owner;
	UINT32 blk[2];		/* blk_addr, used for release buffer */
	UINT32 va[2];		/* va[0]: raw/y, va[1]: raw/uv */
	UINT32 buf_ctrl;	/* refer to SIE_BUILTIN_HEADER_CTL_LOCK/SIE_BUILTIN_HEADER_CTL_PUSH */
	UINT32 timestamp;
	UINT32 count;
	struct vos_list_head pool_list;
	struct vos_list_head proc_list;
} KDRV_IPP_BUILTIN_JOB;
typedef struct {
	UINT32 start_addr;
	UINT32 total_size;
	UINT32 blk_size;
	UINT8 blk_num;
	vk_spinlock_t lock;
	struct vos_list_head free_list_root;
	struct vos_list_head used_list_root;
} KDRV_IPP_BUILTIN_MEM_POOL;
typedef struct {
	void *reg_base[KDRV_IPP_BUILTIN_ENG_MAX];
	KDRV_IPP_BUILTIN_FMD_CB fmd_cb[KDRV_IPP_BUILTIN_FMD_CB_NUM];
	UINT32 dtsi_ver;	/* 0: old version for one direct mode, 1: new version for multi handle */
	UINT32 hdl_num;
	UINT32 _3dnr_lock_addr; //tmp new method need to take care

	KDRV_IPP_BUILTIN_HDL *p_hdl;
	KDRV_IPP_BUILTIN_HDL *p_trig_hdl;	/* hdl that been trig start */
	UINT32 *p_dtsi_buf;					/* buffer to read dtsi array */
	UINT8 is_init;
	UINT8 is_fastboot;

	KDRV_IPP_BUILTIN_MEM_POOL job_pool;
	THREAD_HANDLE proc_tsk_id;
	ID proc_tsk_flg_id;
	vk_spinlock_t job_list_lock;
	struct vos_list_head job_list_root;
	UINT32 valid_src_id_bit;
} KDRV_IPP_BUILTIN_CTL;

INT32 kdrv_ipp_builtin_plat_init_clk(void);
void* kdrv_ipp_builtin_plat_ioremap_nocache(UINT32 addr, UINT32 size);
void kdrv_ipp_builtin_plat_iounmap(void *addr);
INT32 kdrv_ipp_builtin_plat_read_dtsi_array(CHAR *node, CHAR *tag, UINT32 *buf, UINT32 num);
INT32 kdrv_ipp_builtin_plat_read_dtsi_string(CHAR *node, CHAR *tag, const CHAR **str);
void kdrv_ipp_builtin_plat_request_ime_irq(INT32 irq_id, CHAR *name, void *param);
void kdrv_ipp_builtin_plat_release_irq(INT32 irq_id, void *param);
void *kdrv_ipp_builtin_plat_malloc(UINT32 size);
void kdrv_ipp_builtin_plat_free(void *ptr);
void kdrv_ipp_builtin_ife_shared_sram_clksel(void);
void kdrv_ipp_builtin_cnn_dce_shared_sram_clksel(void);
#define KDRV_IPP_BUILTIN_TSK_PRIORITY (3)
#define KDRV_IPP_BUILTIN_QUE_DEPTH (8)

#define KDRV_IPP_BUILTIN_TSK_PAUSE			FLGPTN_BIT(0)
#define KDRV_IPP_BUILTIN_TSK_PAUSE_END		FLGPTN_BIT(1)
#define KDRV_IPP_BUILTIN_TSK_RESUME			FLGPTN_BIT(2)
#define KDRV_IPP_BUILTIN_TSK_RESUME_END		FLGPTN_BIT(3)
#define KDRV_IPP_BUILTIN_TSK_TRIGGER		FLGPTN_BIT(4)
#define KDRV_IPP_BUILTIN_TSK_EXIT			FLGPTN_BIT(5)
#define KDRV_IPP_BUILTIN_TSK_EXIT_END		FLGPTN_BIT(6)
#define KDRV_IPP_BUILTIN_TSK_CHK			FLGPTN_BIT(7)
#define KDRV_IPP_BUILTIN_TSK_JOBDONE		FLGPTN_BIT(8)
#define KDRV_IPP_BUILTIN_TSK_FASTBOOT_DONE	FLGPTN_BIT(9)
/**
	BUILTIN IME
*/

#define NVT_IME_BUILTIN_CHIP_COUNT		(1)
#define NVT_IME_BUILTIN_MINOR_COUNT		(1)
#define NVT_IME_BUILTIN_TOTAL_CH_COUNT	(NVT_IME_BUILTIN_CHIP_COUNT * NVT_IME_BUILTIN_MINOR_COUNT)
#define NVT_IME_BUILTIN_NAME            "builtin_ime"

/* chip_id, eng_id to idx mapping */
#define IME_BUILTIN_CHIP_IDX(chip)				(chip - KDRV_CHIP0)
#define IME_BUILTIN_ENG_IDX(eng)				(eng - KDRV_VIDEOPROCS_IME_ENGINE0)
#define IME_BUILTIN_CONV2_HDL_IDX(chip, eng)	((IME_BUILTIN_CHIP_IDX(chip) * NVT_IME_BUILTIN_MINOR_COUNT) + IME_BUILTIN_ENG_IDX(eng))
#define IME_BUILTIN_IDX_CHIP(idx)				((idx / NVT_IME_BUILTIN_MINOR_COUNT) + KDRV_CHIP0)
#define IME_BUILTIN_IDX_ENG(idx)				((idx % NVT_IME_BUILTIN_MINOR_COUNT) + KDRV_VIDEOPROCS_IME_ENGINE0)

/* interrupt status */
#define IME_BUILTIN_INTS_FB_FRM_END          0x10000000  ///< fastboot last frame-end
#define IME_BUILTIN_INTS_FRM_START           0x20000000
#define IME_BUILTIN_INTS_FRM_END             0x80000000  ///< frame-end status

typedef struct {
	UINT32 dev_id;
	CHAR name[16];

	NVT_IME_BUILTIN_RESOURCE resource;
	IME_BUILTIN_ISR_CB fastboot_cb;
	IME_BUILTIN_ISR_CB hdal_cb;
	UINT8 switch_to_hdal;
} IME_BUILTIN_HANDLE;

typedef struct {
	UINT32 chip_num;
	UINT32 eng_num;
	UINT32 total_ch;
	IME_BUILTIN_HANDLE *p_hdl;
} IME_BUILTIN_CTL;

void ime_builtin_init(UINT32 chip_num, UINT32 eng_num);
void ime_builtin_init_resource(NVT_IME_BUILTIN_RESOURCE *p_resource, UINT32 idx);
void ime_builtin_exit(void);
void ime_builtin_reg_fastboot_cb(UINT32 dev_id, IME_BUILTIN_ISR_CB cb);
void ime_builtin_unreg_fastboot_cb(UINT32 dev_id);

void ime_builtin_get_path_info(KDRV_IPP_BUILTIN_PATH_ID pid, KDRV_IPP_BUILTIN_IMG_INFO *info);

#endif
