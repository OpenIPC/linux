
#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "kwrap/task.h"
#include "kwrap/spinlock.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
#include "kwrap/util.h"
#else
#define vos_util_msec_to_tick(x) msecs_to_jiffies(x)
#endif
#include "comm/hwclock.h"
#include "kflow_common/nvtmpp.h"
#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_core.h"
#include "kflow_videoprocess/isf_vdoprc.h"
#include "kflow_videoprocess/ctl_ipp.h"

#if defined (__FREERTOS)
#define USE_VPE   				DISABLE
#else
#if defined(CONFIG_NVT_SMALL_HDAL)
#define USE_VPE   				DISABLE
#else
#if defined(_BSP_NA51055_)
#define USE_VPE   				ENABLE
#else
#define USE_VPE   				DISABLE
#endif
#endif
#endif
#define USE_ISE   				ENABLE
#if (USE_VPE == ENABLE)
#include "kflow_videoprocess/ctl_vpe.h"
#endif
#if (USE_ISE == ENABLE)
#include "kflow_videoprocess/ctl_ise.h"
#endif

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
#include <malloc.h>
#define module_param_named(a, b, c, d)
#define MODULE_PARM_DESC(a, b)
#define EXPORT_SYMBOL(x)

#define debug_msg 			vk_printk

#include <string.h>
#define SLEEP(x)    		vos_util_delay_ms(1000*(x))
#define MSLEEP(x)    		vos_util_delay_ms(x)
#define USLEEP(x)   		vos_util_delay_us(x)
#define DELAY_M_SEC(x)		vos_util_delay_ms(x)
#define DELAY_U_SEC(x)		vos_util_delay_us(x)
#else
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
#include "comm/nvtmem.h"
#else
#include "frammap/frammap_if.h"
#endif
//#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>

#include <linux/printk.h>
#define debug_msg 			vk_printk

#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
#define SLEEP(x)    		vos_util_delay_ms(1000*(x))
#define MSLEEP(x)    		vos_util_delay_ms(x)
#define USLEEP(x)   		vos_util_delay_us(x)
#define DELAY_M_SEC(x)		vos_util_delay_ms(x)
#define DELAY_U_SEC(x)		vos_util_delay_us(x)
#else
#include <linux/delay.h>
#define SLEEP(x)    		msleep(1000*(x))
#define MSLEEP(x)    		msleep(x)
#define USLEEP(x)   		udelay(x)
#define DELAY_M_SEC(x)      msleep(x) //require delay.h
#define DELAY_U_SEC(x)      udelay(x) //require delay.h
#endif
#endif


#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
#if defined (__FREERTOS)
#define USE_GFX				ENABLE  //external depend
#define USE_VDS				ENABLE  //external depend
#else
#define USE_GFX				ENABLE  //external depend
#define USE_VDS				ENABLE  //external depend
#endif
#else
#if defined (__FREERTOS)
#define USE_GFX				DISABLE  //external depend
#define USE_VDS				DISABLE  //external depend
#else
#define USE_GFX				ENABLE  //external depend
#define USE_VDS				ENABLE  //external depend
#endif
#endif

#define USE_NEW_SHDR			ENABLE
#define USE_IN_FRC				ENABLE
#define USE_OUT_FRC				ENABLE
#define USE_OUT_EXT  			ENABLE
#define USE_PULL	   			ENABLE

#define USER_OUT_BUFFER_QUEUE_TIMEOUT 2000//ms

#define ISF_VDOPRC_VPE_OUT_NUM 	VDOPRC_MAX_VPE_OUT_NUM

#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
#define USE_IN_ONEBUF	   		ENABLE
#define USE_IN_DIRECT	   		ENABLE
#define USE_OUT_ONEBUF	   		ENABLE
#define USE_OUT_LOWLATENCY	   	ENABLE
#define USE_OUT_DIS				ENABLE
#else
#define USE_IN_ONEBUF	   		DISABLE
#define USE_IN_DIRECT	   		DISABLE
#define USE_OUT_ONEBUF	   		DISABLE
#define USE_OUT_LOWLATENCY	   	DISABLE
#define USE_OUT_DIS				DISABLE
#endif

#define ISF_VDOPRC_IN_NUM 		VDOPRC_MAX_IN_NUM
#define ISF_VDOPRC_OUT_NUM 		VDOPRC_MAX_OUT_NUM
#define ISF_VDOPRC_PHY_OUT_NUM 	VDOPRC_MAX_PHY_OUT_NUM
#define ISF_VDOPRC_PATH_NUM 	ISF_VDOPRC_OUT_NUM

#if defined(CONFIG_NVT_SMALL_HDAL)
#define ISF_VDOPRC_OUTQ_MAX 	1 //reduce for small version
#else
#define ISF_VDOPRC_OUTQ_MAX 	15
#endif

#define ISF_NEW_PULL			ENABLE
//#if defined(_BSP_NA51000_) || defined(_BSP_NA51055_)
//#define VDOPRC_OUT_DEPTH_MAX 2
//#endif
//#if defined(_BSP_NA51023_)
#define VDOPRC_OUT_DEPTH_MAX 	(4+ISF_VDOPRC_OUTQ_MAX)
//#endif

#if defined(CONFIG_NVT_SMALL_HDAL)
#define VDOPRC_IN_DEPTH_MAX 	4 //reduce for small version
#else
#define VDOPRC_IN_DEPTH_MAX 	15
#endif
#define VDOPRC_IN_DEPTH_DEF 	2

#define VDOPRC_DBG_TS_MAXNUM	30


#if (USE_OUT_EXT == ENABLE)
typedef struct _ISF_PIPE {
	UINT32 cmd_count;
	UINT32 cmd[64]; //this is call begin(), add_xxxx(), end() to fill action and parameters
} ISF_PIPE;

#define ISF_PIPE_CMD_BEGIN		0xFFFF0001
#define ISF_PIPE_CMD_SCALE		0xFFFF0002
#define ISF_PIPE_CMD_DIR		0xFFFF0003
//#define ISF_PIPE_CMD_PXLFMT	0xFFFF0004
#define ISF_PIPE_CMD_SCROP		0xFFFF0005
#define ISF_PIPE_CMD_DCROP		0xFFFF0006
#define ISF_PIPE_CMD_END		0xFFFF0000

extern void isf_pipe_begin(ISF_PIPE* p_pipe);
extern void isf_pipe_add_scrop(ISF_PIPE* p_pipe, INT32 crop_x, INT32 crop_y, INT32 crop_w, INT32 crop_h);
//extern void isf_pipe_add_dcrop(ISF_PIPE* p_pipe, INT32 crop_x, INT32 crop_y, INT32 crop_w, INT32 crop_h);
extern void isf_pipe_add_scale(ISF_PIPE* p_pipe, INT32 scale_w, INT32 scale_h, UINT32 pxlfmt, INT32 scale_h_align);
extern void isf_pipe_add_dir(ISF_PIPE* p_pipe, UINT32 dir, INT32 h_align);
extern void isf_pipe_end(ISF_PIPE* p_pipe);
extern ISF_RV isf_pipe_exec(ISF_UNIT* p_unit, UINT32 nport, ISF_PIPE* p_pipe, ISF_DATA* p_src_data, ISF_DATA* p_tmp_rotate);

extern BOOL _vdoprc_is_out_rotate(ISF_UNIT *p_thisunit, UINT32 pid);
extern void _vdoprc_config_out_ext(ISF_UNIT *p_thisunit, UINT32 pid, UINT32 en);
extern ISF_RV _isf_vdoprc_oport_do_dispatch_out_ext(struct _ISF_UNIT *p_thisunit, UINT32 oport, ISF_DATA *p_data, INT32 wait_ms, ISF_DATA* p_tmp_rotate);

#endif

typedef void (*IPL_SYNC_CB)(UINT32 path, UINT64 end, UINT64 cycle);
#if (USE_IN_DIRECT == ENABLE)
typedef void (*SIE_UNLOCK_CB)(ISF_UNIT *p_thisunit, UINT32 oport, void *p_header_info);
#endif

typedef struct _VDOPRC_COLOR {
	CTL_IPP_OUT_COLOR_SPACE space;
} VDOPRC_COLOR;

typedef struct _VDOPRC_CTRL {
	UINT32 pipe;
	UINT32 in_cfg_func;
	UINT32 out_cfg_func[ISF_VDOPRC_OUT_NUM];
	UINT32 iq_id;
	UINT32 _3dnr_refpath;
	UINT32 cur_3dnr_refpath;
	UINT32 _lowlatency_trig;
	CTL_IPP_FUNC func_max;
	CTL_IPP_FUNC func;
	CTL_IPP_FUNC cur_func;
#if _TODO
	UINT32 func2_max;
	UINT32 func2;
	UINT32 cur_func2;
#endif
#if (USE_NEW_SHDR == ENABLE)
	UINT32 shdr_cnt;
	UINT32 shdr_j;
	UINT32 shdr_i;
#endif
	CTL_IPP_SCL_METHOD_SEL scale;
	VDOPRC_COLOR color;
} VDOPRC_CTRL;

typedef struct _VDOPRC_PROCESS {
	UINT32 st_osdmask;
} VDOPRC_PROCESS;

typedef struct _VDOPRC_IN_PATH {
	USIZE max_size;
	VDO_PXLFMT max_pxlfmt;
	CTL_IPP_IN_CROP crop;
	CTL_IPP_FLIP_TYPE dir;
#if 0
	UINT32 user_count;
#endif
} VDOPRC_IN_PATH;

typedef struct _VDOPRC_IN_QUEUE {
	ISF_DATA input_pool;
	ISF_DATA* input_data;
	UINT32 input_used[VDOPRC_IN_DEPTH_MAX];
	UINT32 input_max;
	UINT32 input_cnt;
} VDOPRC_IN_QUEUE;

typedef struct _VDOPRC_OUT_QUEUE {
	UINT32 count_new_ok[ISF_VDOPRC_OUT_NUM];
	UINT32 count_new_fail[ISF_VDOPRC_OUT_NUM];
	UINT32 count_push_ok[ISF_VDOPRC_OUT_NUM];
	UINT32 count_push_fail[ISF_VDOPRC_OUT_NUM];
	UINT32 output_en[ISF_VDOPRC_OUT_NUM];
	UINT32* output_cur_en;
	UINT32 output_connecttype[ISF_VDOPRC_OUT_NUM];
#if (USE_OUT_ONEBUF == ENABLE)
	UINT32 force_onebuffer[ISF_VDOPRC_OUT_NUM];
	UINT32 force_onesize[ISF_VDOPRC_OUT_NUM];
	UINT32 force_onej[ISF_VDOPRC_OUT_NUM];
	UINT32 force_one_reset[ISF_VDOPRC_OUT_NUM];
	UINT32 force_refbuffer;
	UINT32 force_refsize;
	UINT32 force_refj;
#endif
#if _TODO
//for Notify
	UINT32 trigdata_id;
	UINT32* cur_trigdata_id;
#endif
#if _TODO
//sync queue
	//UINT32 ISF_VDOPRC_OUT_SEM_ID = {0};
	ISF_DATA sync_data[ISF_VDOPRC_OUT_NUM][VDOPRC_OUT_DEPTH_MAX];
	UINT32 sync_used[ISF_VDOPRC_OUT_NUM][VDOPRC_OUT_DEPTH_MAX];
	UINT32 sync_head[ISF_VDOPRC_OUT_NUM];
	UINT32 sync_tail[ISF_VDOPRC_OUT_NUM];
	UINT32 sync_cnt[ISF_VDOPRC_OUT_NUM];
#endif
//output queue
	ISF_DATA output_data[ISF_VDOPRC_OUT_NUM][VDOPRC_OUT_DEPTH_MAX];
	UINT32 output_used[ISF_VDOPRC_OUT_NUM][VDOPRC_OUT_DEPTH_MAX];
	UINT32 output_type[ISF_VDOPRC_OUT_NUM][VDOPRC_OUT_DEPTH_MAX];
	UINT32 output_cnt[ISF_VDOPRC_OUT_NUM];
	UINT32 output_max[ISF_VDOPRC_OUT_NUM];
#if (USE_OUT_DIS == ENABLE)
	UINT32 dis_mode;
	UINT32 dis_en;
	UINT32 dis_buf_size;
	UINT32 dis_buf_addr;
	//scale ratio
	UINT32 dis_scale_factor[ISF_VDOPRC_PHY_OUT_NUM];
	//tracking queue
	ISF_DATA* track_data[ISF_VDOPRC_PHY_OUT_NUM];
	UINT32 track_framecnt[ISF_VDOPRC_PHY_OUT_NUM];
	UINT32 track_j[ISF_VDOPRC_PHY_OUT_NUM];
#endif
} VDOPRC_OUT_QUEUE;

#if (ISF_NEW_PULL == ENABLE)
typedef struct _ISF_PULL_QUEUE {
	UINT32 		sign; ///< signature, equal to ISF_SIGN_QUEUE
	CHAR *		unit_name; ///< name string
	UINT32 		id; ///< id of this module
	UINT32		en;
	UINT32		head;	///< head pointer
	UINT32		tail;	///< tail pointer
	UINT32		is_full;	///< full flag
	UINT32		cnt; ///< current count
	UINT32		max;		///< max count
	ISF_DATA*	data;
	void*     	p_sem_q;
} ISF_PULL_QUEUE, *PISF_PULL_QUEUE;
#endif

typedef struct _VDOPRC_PULL_QUEUE {
	ISF_DATA 		data[ISF_VDOPRC_OUT_NUM][ISF_VDOPRC_OUTQ_MAX];
#if (ISF_NEW_PULL == ENABLE)
	ISF_PULL_QUEUE	output[ISF_VDOPRC_OUT_NUM];
#else
	ISF_DATA_QUEUE	output[ISF_VDOPRC_OUT_NUM];
#endif
	UINT32			num[ISF_VDOPRC_OUT_NUM];
} VDOPRC_PULL_QUEUE;

typedef struct _VDOPRC_OUT_DBG {
	UINT64 t[ISF_VDOPRC_OUT_NUM][VDOPRC_DBG_TS_MAXNUM];
} VDOPRC_OUT_DBG;

typedef struct _VDOPRC_PRE_ROTATE {
	ISF_DATA rotate_data;
	ISF_DATA* p_data;
} VDOPRC_PRE_ROTATE;

typedef struct _VDOPRC_EXT_CONTEXT {
	BOOL is_proc;
	UINT32 src_path;
	ISF_DATA in_data;
	ISF_DATA* p_data;
	ISF_PIPE pipe;
} VDOPRC_EXT_CONTEXT;

typedef struct _VDOPRC_COMMON_MEM {
	ISF_DATA    memblk;
	MEM_RANGE 	unit_buf;
	MEM_RANGE 	kflow_buf;
#if (USE_VPE == ENABLE)
	MEM_RANGE 	kflow_vpe_buf;
#endif
#if (USE_ISE == ENABLE)
	MEM_RANGE 	kflow_ise_buf;
#endif
	MEM_RANGE 	total_buf;
} VDOPRC_COMMON_MEM;

typedef struct _VDOPRC_CONTEXT {
	SEM_HANDLE ISF_VDOPRC_OUTQ_SEM_ID[ISF_VDOPRC_OUT_NUM];
#if (ISF_NEW_PULL == ENABLE)
#else
	SEM_HANDLE ISF_VDOPRC_OUT_SEM_ID[ISF_VDOPRC_OUT_NUM];
#endif
	UINT32 dev;
	UINT32 dev_handle; //handle of ctl_ipp
	UINT32 dev_trigger_open; //before open
	UINT32 dev_trigger_close; //before close
	UINT32 dev_ready; //enable after open, disable before close
/*
	SEM_HANDLE* inq_sem_id;
	SEM_HANDLE* outq_sem_id;
*/
	/*-------------------------------*/
	IPP_EVENT_FP on_input;
	IPP_EVENT_FP on_process;
	IPP_EVENT_FP on_output;
	IPP_EVENT_FP on_osd;
	IPP_EVENT_FP on_mask;
	IPL_SYNC_CB on_sync;
#if defined(_BSP_NA51023_)
	IPL_MD_CB md_cb;
#endif
#if defined(_BSP_NA51023_)
	IPL_VIEWTRACKING_CB on_viewtrack;
#endif
	/*-------------------------------*/
	MEM_RANGE mem; //for private buf
	NVTMPP_VB_POOL mempool; //for private buf
	ISF_DATA memblk; //for private buf
	NVTMPP_DDR ddr; //for common buf (output)
	/*-------------------------------*/
	UINT32 cur_mode;
	UINT32 new_mode;
	UINT32 func_allow;
	UINT32 ifunc_allow;
	UINT32 cur_in_cfg_func;
	UINT32 new_in_cfg_func;
	UINT32 ofunc_allow[ISF_VDOPRC_OUT_NUM];
	UINT32 cur_out_cfg_func[ISF_VDOPRC_OUT_NUM];
	UINT32 new_out_cfg_func[ISF_VDOPRC_OUT_NUM];
	UINT32 sleep;
	VDOPRC_CTRL ctrl;
	VDOPRC_IN_PATH in[ISF_VDOPRC_IN_NUM];
	CTL_IPP_OUT_PATH out[ISF_VDOPRC_OUT_NUM];
	UINT32 out_crop_mode[ISF_VDOPRC_OUT_NUM];
#if (USE_OUT_ONEBUF == ENABLE)
	CTL_IPP_OUT_PATH_BUFMODE bufmode[ISF_VDOPRC_PHY_OUT_NUM];
#endif
	VDOPRC_PRE_ROTATE out_rot[ISF_VDOPRC_PHY_OUT_NUM];
	VDOPRC_EXT_CONTEXT out_ext[ISF_VDOPRC_OUT_NUM - ISF_VDOPRC_PHY_OUT_NUM];
	ISF_FRC infrc[ISF_VDOPRC_IN_NUM];
	ISF_FRC outfrc[ISF_VDOPRC_OUT_NUM];
	UINT32 nvxcodec; //for yuv-compress
	UINT32 codec[ISF_VDOPRC_OUT_NUM];
	VDOPRC_IN_QUEUE inq;
	VDOPRC_OUT_QUEUE outq;
	VDOPRC_PROCESS proc;
	VDOPRC_PULL_QUEUE pullq;
	VDOPRC_OUT_DBG out_dbg;
	void* p_sem_poll; //for poll_list (shared)
	UINT32 poll_mask; //for poll_list user query
	UINT32 phy_mask; //for poll_list phy-path sync mask
	UINT32 result_mask; //for poll_list result
	UINT32 start_mask; //for poll_list state
#if (USE_IN_DIRECT == ENABLE)
	SIE_UNLOCK_CB sie_unl_cb;
	struct _ISF_UNIT *p_srcunit;
#endif
#if (USE_OUT_ONEBUF == ENABLE)
	UINT32 max_strp_num;
#endif
#if (USE_NEW_SHDR == ENABLE)
	UINT32 shdr_in;
#endif
#if (USE_VPE == ENABLE)
	BOOL vpe_mode;
#endif
#if (USE_ISE == ENABLE)
	BOOL ise_mode;
#endif
#if (USE_VPE == ENABLE || USE_ISE == ENABLE)
	URECT pre_scl_crop[ISF_VDOPRC_VPE_OUT_NUM];
	ISF_VDO_WIN out_win[ISF_VDOPRC_VPE_OUT_NUM];
#endif
	INT32 user_out_blk[ISF_VDOPRC_OUT_NUM];
	UINT32 user_out_blk_size[ISF_VDOPRC_OUT_NUM];
	UINT32 out_h_align[ISF_VDOPRC_OUT_NUM];
	UINT32 err_cnt;
	UINT32 out_order[ISF_VDOPRC_OUT_NUM];
} VDOPRC_CONTEXT;

typedef struct {
	UINT32 ctl_ipp_handle;
	ISIZE img_size;
} _IPP_DS_CB_INPUT_INFO;

typedef struct {
	VDO_FRAME* p_vdoframe;
} _IPP_DS_CB_OUTPUT_INFO;

typedef struct {
	UINT32 ctl_ipp_handle;
	ISIZE img_size;
} _IPP_PM_CB_INPUT_INFO;

typedef struct {
	VDO_FRAME* p_vdoframe;
} _IPP_PM_CB_OUTPUT_INFO;

extern UINT32 g_vdoprc_max_count;
#define DEV_MAX_COUNT	g_vdoprc_max_count
extern ISF_UNIT *g_vdoprc_list[VDOPRC_MAX_NUM]; //list of all videoproc device
#define DEV_UNIT(did)	g_vdoprc_list[(did)]

#define HD_VIDEOPROC_CFG_STRIP_MASK		0x00000007  //vprc stripe rule mask: (default 0)
#define HD_VIDEOPROC_CFG_STRIP_LV1		0x00000000  //vprc "0: cut w>1280, GDC =  on, 2D_LUT off after cut (LL slow)
#define HD_VIDEOPROC_CFG_STRIP_LV2		0x00010000  //vprc "1: cut w>2048, GDC = off, 2D_LUT off after cut (LL fast)
#define HD_VIDEOPROC_CFG_STRIP_LV3		0x00020000  //vprc "2: cut w>2688, GDC = off, 2D_LUT off after cut (LL middle)(2D_LUT best)
#define HD_VIDEOPROC_CFG_STRIP_LV4		0x00030000  //vprc "3: cut w> 720, GDC =  on, 2D_LUT off after cut (LL not allow)(GDC best)
#define HD_VIDEOPROC_CFG_DISABLE_GDC	HD_VIDEOPROC_CFG_STRIP_LV2
#define HD_VIDEOPROC_CFG_LL_FAST		HD_VIDEOPROC_CFG_STRIP_LV2
#define HD_VIDEOPROC_CFG_2DLUT_BEST		HD_VIDEOPROC_CFG_STRIP_LV3
#define HD_VIDEOPROC_CFG_GDC_BEST		HD_VIDEOPROC_CFG_STRIP_LV4

extern UINT32 _isf_vdoprc_get_cfg(void);
extern BOOL _isf_vdoprc_is_init(void);

extern ISF_RV _isf_vdoprc_do_command(UINT32 cmd, UINT32 p0, UINT32 p1, UINT32 p2);

//control proc parameters and state
extern ISF_RV _isf_vdoprc_do_bindinput(ISF_UNIT* p_thisunit, UINT32 iport, ISF_UNIT* p_srcunit, UINT32 oport);
extern ISF_RV _isf_vdoprc_do_offsync(ISF_UNIT *p_thisunit, UINT32 oport);
extern ISF_RV _isf_vdoprc_do_setparam(ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param, UINT32 value);
extern UINT32 _isf_vdoprc_do_getparam(ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param);
extern ISF_RV _isf_vdoprc_do_setparamstruct(ISF_UNIT *p_thisUnit, UINT32 nport, UINT32 param, UINT32* p_struct, UINT32 size);
extern ISF_RV _isf_vdoprc_do_getparamstruct(ISF_UNIT *p_thisUnit, UINT32 nport, UINT32 param, UINT32* p_struct, UINT32 size);
extern ISF_RV _isf_vdoprc_do_updateport(ISF_UNIT *p_thisunit, UINT32 oport, ISF_PORT_CMD cmd);

//input data
extern void _isf_vprc_set_ifunc_allow(ISF_UNIT *p_thisunit);
extern ISF_RV _isf_vprc_check_ifunc(ISF_UNIT *p_thisunit);
extern void _vdoprc_max_in(ISF_UNIT *p_thisunit, USIZE* p_size, VDO_PXLFMT* p_fmt);
extern ISF_RV _vdoprc_config_in_crop(ISF_UNIT *p_thisunit, UINT32 iport);
extern ISF_RV _vdoprc_update_in_crop(ISF_UNIT *p_thisunit, UINT32 iport);
extern ISF_RV _vdoprc_config_in_direct(ISF_UNIT *p_thisunit, UINT32 iport);
extern ISF_RV _vdoprc_update_in_direct(ISF_UNIT *p_thisunit, UINT32 iport);
extern void _vdoprc_iport_setqueuecount(ISF_UNIT *p_thisunit, UINT32 count);
extern ISF_RV _isf_vdoprc_iport_alloc(ISF_UNIT *p_thisunit);
extern ISF_RV _isf_vdoprc_iport_free(ISF_UNIT *p_thisunit);

extern ISF_RV _isf_vdoprc_iport_do_push(ISF_UNIT *p_thisunit, UINT32 iport, ISF_DATA *p_data, INT32 wait_ms);
extern void _isf_vdoprc_iport_do_push_fail(ISF_UNIT *p_thisunit, UINT32 iport, UINT32 buf_handle, UINT32 probe, UINT32 r);
extern void _isf_vdoprc_iport_do_proc_cb(ISF_UNIT *p_thisunit, UINT32 iport, UINT32 buf_handle, UINT32 event, INT32 err);
#if (USE_IN_DIRECT == ENABLE)
extern void _isf_vdoprc_iport_do_cap_cb(ISF_UNIT *p_thisunit, UINT32 iport, UINT32 event, void* evt);
#endif

//output data
extern void _isf_vprc_set_ofunc_allow(ISF_UNIT *p_thisunit);
extern ISF_RV _vdoprc_config_ofunc(ISF_UNIT *p_thisunit, BOOL en);
extern ISF_RV _vdoprc_config_out(ISF_UNIT *p_thisunit, UINT32 pid, UINT32 en);
extern ISF_RV _vdoprc_update_out(ISF_UNIT *p_thisunit, UINT32 pid, UINT32 en);
extern void _vdoprc_oport_initqueue(ISF_UNIT *p_thisunit);
extern void _vdoprc_oport_set_enable(ISF_UNIT *p_thisunit, UINT32 oport, UINT32 en);
extern BOOL _vdoprc_oport_is_enable(ISF_UNIT *p_thisunit);

#if (USE_VPE == ENABLE)
extern ISF_RV _vdoprc_config_out_vpe(ISF_UNIT *p_thisunit, UINT32 pid, UINT32 en, BOOL runtime_update);
extern void _isf_vdoprc_vpe_oport_do_out_cb(ISF_UNIT *p_thisunit, UINT32 event, void *p_in, void *p_out);
extern ISF_RV _vdoprc_config_vpe_in_crop(ISF_UNIT * p_thisunit, UINT32 iport);
extern void _isf_vdoprc_vpe_iport_do_proc_cb(ISF_UNIT *p_thisunit, UINT32 iport, UINT32 buf_handle, UINT32 event, INT32 kr);
#endif
#if (USE_ISE == ENABLE)
extern ISF_RV _vdoprc_config_out_ise(ISF_UNIT *p_thisunit, UINT32 pid, UINT32 en, BOOL runtime_update);
extern void _isf_vdoprc_ise_oport_do_out_cb(ISF_UNIT *p_thisunit, UINT32 event, void *p_in, void *p_out);
extern ISF_RV _vdoprc_config_ise_in_crop(ISF_UNIT * p_thisunit, UINT32 iport);
extern void _isf_vdoprc_ise_iport_do_proc_cb(ISF_UNIT *p_thisunit, UINT32 iport, UINT32 buf_handle, UINT32 event, INT32 kr);
#endif
extern void _isf_vdoprc_push_dummy_queue_for_user_buf(ISF_UNIT *p_thisunit, UINT32 oport, ISF_DATA *p_data);

extern UINT32 _isf_vdoprc_oport_do_new(ISF_UNIT *p_thisunit, UINT32 oport, UINT32 buf_size, UINT32 ddr, UINT32* p_addr);
extern void _isf_vdoprc_oport_do_push(ISF_UNIT *p_thisunit, UINT32 oport, UINT32 buf_handle, VDO_FRAME* p_srcvdoframe);
extern void _isf_vdoprc_oport_do_lock(ISF_UNIT *p_thisunit, UINT32 oport, UINT32 buf_handle);
extern void _isf_vdoprc_oport_do_unlock(ISF_UNIT *p_thisunit, UINT32 oport, UINT32 buf_handle, INT32 err);
extern void _isf_vdoprc_oport_do_proc_begin(ISF_UNIT *p_thisunit, UINT32 oport, UINT32 buf_handle);
extern void _isf_vdoprc_oport_do_proc_end(ISF_UNIT *p_thisunit, UINT32 oport, UINT32 buf_handle);
extern void _isf_vdoprc_oport_do_start(ISF_UNIT *p_thisunit, UINT32 oport);
extern void _isf_vdoprc_oport_do_stop(ISF_UNIT *p_thisunit, UINT32 oport);

//process data
extern void _isf_vprc_set_func_allow(ISF_UNIT *p_thisunit);
extern ISF_RV _isf_vprc_check_func(ISF_UNIT *p_thisunit);

extern void _vdoprc_max_func(ISF_UNIT *p_thisunit, CTL_IPP_FUNC* p_func);
extern ISF_RV _vdoprc_config_func(ISF_UNIT *p_thisunit, BOOL en);
extern ISF_RV _vdoprc_update_func(ISF_UNIT *p_thisunit);

extern void _isf_vdoprc_do_process(ISF_UNIT *p_thisunit, UINT32 event);

extern void _isf_vdoprc_do_input_osd(ISF_UNIT *p_thisunit, UINT32 iport, void *in, void *out);
extern void _isf_vdoprc_finish_input_osd(ISF_UNIT *p_thisunit, UINT32 iport);

extern void _isf_vdoprc_do_input_mask(ISF_UNIT *p_thisunit, UINT32 iport, void *in, void *out);
extern void _isf_vdoprc_finish_input_mask(ISF_UNIT *p_thisunit, UINT32 iport);

extern void _isf_vdoprc_do_output_osd(ISF_UNIT *p_thisunit, UINT32 oport, void *in, void *out);
extern void _isf_vdoprc_do_output_mask(ISF_UNIT *p_thisunit, UINT32 oport, void *in, void *out);

//pull data
extern void _isf_vdoprc_oqueue_do_open(ISF_UNIT *p_thisunit, UINT32 oport);
extern void _isf_vdoprc_oqueue_pre_start(ISF_UNIT *p_thisunit, UINT32 oport);
extern void _isf_vdoprc_oqueue_do_start(ISF_UNIT *p_thisunit, UINT32 oport);
extern void _isf_vdoprc_oqueue_force_stop(ISF_UNIT *p_thisunit, UINT32 oport);
extern void _isf_vdoprc_oqueue_do_stop(ISF_UNIT *p_thisunit, UINT32 oport);
extern void _isf_vdoprc_oqueue_do_close(ISF_UNIT *p_thisunit, UINT32 oport);
extern ISF_RV _isf_vdoprc_oqueue_wait_for_push_in(ISF_UNIT *p_thisunit, UINT32 oport, INT32 wait_ms);
extern ISF_RV _isf_vdoprc_oqueue_do_push(ISF_UNIT *p_thisunit, UINT32 oport, ISF_DATA *p_data);
extern ISF_RV _isf_vdoprc_oqueue_do_pull(ISF_UNIT *p_thisunit, UINT32 oport, ISF_DATA *p_data, INT32 wait_ms);
extern ISF_RV _isf_vdoprc_oqueue_do_push_with_clean(ISF_UNIT *p_thisunit, UINT32 oport, ISF_DATA *p_data, INT32 keep_this);
extern ISF_RV _isf_vdoprc_oqueue_do_push_wait(ISF_UNIT *p_thisunit, UINT32 oport, ISF_DATA *p_data, UINT32 wait_ms);
extern ISF_RV _isf_vdoprc_oqueue_do_poll_list(ISF_UNIT *p_thisunit, VDOPRC_POLL_LIST *p_poll_info);
extern ISF_RV _isf_vdoprc_oqueue_get_poll_mask(ISF_UNIT *p_thisunit, VDOPRC_POLL_LIST *p_poll_info);
extern void _isf_vdoprc_oqueue_cancel_poll(ISF_UNIT *p_thisunit);

//debug
extern void isf_vdoprc_dump_status(int (*dump)(const char *fmt, ...), ISF_UNIT *p_thisunit);

//extend process
typedef void (*_ISF_EXT_PROCESS_CB)(void);

extern ISF_RV _isf_vdoprc_ext_tsk_open(void);
extern ISF_RV _isf_vdoprc_ext_tsk_close(void);
extern ISF_RV _isf_vdoprc_ext_tsk_trigger_proc(ISF_UNIT *p_thisunit, UINT32 ext_pid, UINT32 src_path, ISF_DATA* p_data);


///////////////////////////////////////////////////////////////////////////////
extern void isf_vdoprc_install_id(void) _SECTION(".kercfg_text");
extern void isf_vdoprc_uninstall_id(void) _SECTION(".kercfg_text");

#define ISF_VDOPRC_EXT_TSK_PRI       3
#define ISF_VDOPRC_EXT_TSK_STKSIZE   4096
extern THREAD_HANDLE _SECTION(".kercfg_data") ISF_VDOPRC_EXT_TSK_ID;
extern THREAD_DECLARE(isf_vdoprc_ext_tsk, arglist);

//extern SEM_HANDLE _SECTION(".kercfg_data") ISF_VDOPRC_IN_SEM_ID[];
extern SEM_HANDLE _SECTION(".kercfg_data") ISF_VDOPRC_OUTP_SEM_ID;
extern SEM_HANDLE _SECTION(".kercfg_data") ISF_VDOPRC_OUTQ_SEM_ID[];
#if (ISF_NEW_PULL == ENABLE)
#else
extern SEM_HANDLE _SECTION(".kercfg_data") ISF_VDOPRC_OUT_SEM_ID[];
#endif
extern SEM_HANDLE _SECTION(".kercfg_data") ISF_VDOPRC_PROC_SEM_ID;

extern ID _SECTION(".kercfg_data") FLG_ID_VDOPRC_EXT;


