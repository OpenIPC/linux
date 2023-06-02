/**
    @file       sen_int.h

    @brief      Define sensor internal function

    @note      Sensor Driver APIs

    Copyright   Novatek Microelectronics Corp. 2010.  All rights reserved.
*/
#ifndef _SENSOR_INT_H
#define _SENSOR_INT_H

#define KDRV_TOP_READY DISABLE
#define KDRV_CG_READY DISABLE

#if defined(__FREERTOS) || defined(__ECOS) || defined(__UITRON)
#if defined(_NVT_FPGA_)
#define CTL_SEN_FPGA ENABLE
#else
#define CTL_SEN_FPGA DISABLE
#endif
#else // linux
#ifdef CONFIG_NVT_FPGA_EMULATION
#define CTL_SEN_FPGA ENABLE
#else
#define CTL_SEN_FPGA DISABLE
#endif
#endif

#include "plat/top.h"
#include "kflow_videocapture/ctl_sen.h"
#include "kdrv_videocapture/kdrv_ssenif.h"
#include "kdrv_videocapture/kdrv_ssenif_lmt.h"
#include "kdrv_videocapture/kdrv_sie.h"
#include "kdrv_videocapture/kdrv_tge.h"
#include "kwrap/mem.h"
#include "kwrap/error_no.h"
#include "kwrap/flag.h"
#include "kwrap/task.h"
#include <kwrap/spinlock.h>
#include "sen_middle_int.h"
#include "ctl_sen_dbg.h"
#include "comm/sif_api.h"
#include "comm/hwclock.h"


#define _INLINE static inline
#if defined(__FREERTOS) || defined(__ECOS) || defined(__UITRON)
#include <string.h>
#else
#include <linux/clk.h>
#include <linux/clk-provider.h>
#endif

#define CTL_SEN_ID_MAX_SUPPORT 8
#define CTL_SEN_ID_MAX_INPUT KDRV_SIE_MAX_ENG
#define CTL_SEN_MAX_OUTPUT_SIE_IDX KDRV_SIE_MAX_ENG
#define CTL_SEN_TGE_MAX_CH 8
#define SEN_FPS_FMT_CONV2_REAL(sen_fps_fmt) sen_fps_fmt_conv2_real(sen_fps_fmt) ///< x100
#define SEN_DIV_U64(x, y) sen_uint64_dividend(x, y)
#define SEN_240MHZ 240000000
#define SEN_480MHZ 480000000
#define CTL_SEN_PLL5_DFT_FREQ PLL5_MAX_FREQ
#define CTL_SEN_PROC_TIME_CNT_MAX 30 /* time record */
#define CTL_SEN_PINMUX_TBL_MAX 60 /* pinmux cfg */
/* for sen ctrl cmdif */
#define CTL_SEN_VX1_GPIO_NUM 5
/* for sen ctrl if */
#define CTL_SEN_SKIP_SET_KDRV 0xff  /* kdrv not support feature */
/* for kdrv */
#define KDRV_CHIP_SSENIF KDRV_CHIP0
#define KDRV_CHIP_TGE KDRV_CHIP0
#define KDRV_CHIP_SIE KDRV_CHIP0
#define KDRV_SSENIF_GLB_HDL 0

/*
    buffer control
*/
/*
--> buf_addr -----------------------
            |kflow (idx 1)          | (size:context_size)
             -----------------------
            |kflow (idx 2)          | (size:context_size)
             -----------------------
            .
            .
            .
             -----------------------
            |kflow (idx context_num)| (size:context_size)
             -----------------------
            |kdrv                   | (size:kdrv_size)
             -----------------------

*/
typedef struct {
	BOOL b_init;
	UINT32 context_num;     // hdal query device number
	UINT32 context_used;    // used device number
	UINT32 context_size;;   // buffer size per device
	UINT32 buf_addr;        // total buffer start address
	UINT32 buf_size;        // total buffer size
	UINT32 kflow_size;      // base on context_num, kflow total request buffer size
	UINT32 kdrv_size;       // base on context_num, kdrv total request buffer size
	BOOL local_alloc;       // 0: ctl_sen_init buffer from HDAL, 1: kflow allocate buffer for ctl_sen_init
} CTL_SEN_HDL_CONTEXT;

#define CTL_SEN_CONTEXT_SIZE CTL_SEN_ALIGN_ROUNDUP(sizeof(CTL_SEN_INIT_OBJ) + sizeof(CTL_SEN_CTRL_OBJ), 4)
#define CTL_SEN_KDRV_BUF(num) 0 // if kdrv need buffer, add here

/*
    state machine control
*/
typedef enum {
	CTL_SEN_STATE_NONE           = 0x00000000,
	CTL_SEN_STATE_INTCFG         = 0x00000001,
	CTL_SEN_STATE_REGSEN         = 0x00000002,
	CTL_SEN_STATE_OPEN           = 0x00000004,
	CTL_SEN_STATE_PWRON          = 0x00000008,
	CTL_SEN_STATE_PWRSAVE        = 0x00000010,
	CTL_SEN_STATE_SLEEP          = 0x00000020,
	CTL_SEN_STATE_WRITEREG       = 0x00000040,
	CTL_SEN_STATE_READREG        = 0x00000080,
	CTL_SEN_STATE_CHGMODE        = 0x00000100,
	CTL_SEN_STATE_SETCFG         = 0x00000200,
	CTL_SEN_STATE_GETCFG         = 0x00000400,
	CTL_SEN_STATE_WAITINTE       = 0x00000800,

	CTL_SEN_STATE_ALL            = 0x00000FFF,

	ENUM_DUMMY4WORD(CTL_SEN_STATE)
} CTL_SEN_STATE;

/* for tge sync mode (CTL_SEN_CFGID_INIT_IF_TGE_SYNC) */
typedef struct {
	BOOL if_stop_flg;   // set TURE for chk stop if (csi/lvds/slvsec) (if not: stop if), set FALSE when chk all sync_bit is TRUE (for next stop flow)
	BOOL tge_start_flg; // set TURE for ready to start, set FALSE for actual start kdrv_tge
	BOOL tge_stop_flg; // set TURE for chk kdrv_tge stop (if not: stop kdrv_tge), set FALSE when chk all sync_bit is TRUE (for next stop flow)
} CTL_SEN_TGE_SYNC_CTRL_OBJ;

/* need set to unknown in sen_module_open !! */
typedef struct {
	CTL_SEN_STATE state;
	CTL_SEN_CHGMODE_OBJ cur_chgmode;
	CTL_SEN_MODE cur_sen_mode;
	CTL_SEN_TGE_SYNC_CTRL_OBJ tge_sync_obj;
} CTL_SEN_CTRL_OBJ;


/**
    sensor ext object (by project & HW)
*/
typedef struct {
	UINT32 timeout_ms;   ///< time out ms
	UINT32 tge_sync;   ///< tge sync bit
} CTL_SEN_EXT_IF;

typedef struct {
	CTL_SEN_EXT_IF if_info;
} CTL_SEN_INIT_EXT_OBJ;

typedef struct {
	CTL_SEN_PWR_CTRL pwr_ctrl;
	CTL_SEN_INIT_CFG_OBJ init_cfg_obj;
	CTL_SEN_INIT_EXT_OBJ init_ext_obj;
	CTL_SEN_PLUG_IN det_plug_in;
	CTL_SEN_DRV_TAB *drv_tab;
} CTL_SEN_INIT_OBJ, *PCTL_SEN_INIT_OBJ;

typedef struct {
	CHAR name[CTL_SEN_NAME_LEN];
	CTL_SEN_REG_OBJ sendrv_reg_obj;
	CTL_SEN_ID id;  // update when set cfg_obj
	CTL_SEN_INIT_CFG_OBJ init_cfg_obj;
} SEN_MAP_TBL;

/* time record */
typedef enum {
	CTL_SEN_PROC_TIME_ITEM_ENTER,
	CTL_SEN_PROC_TIME_ITEM_EXIT,
	CTL_SEN_PROC_TIME_ITEM_MAX,
	ENUM_DUMMY4WORD(CTL_SEN_PROC_TIME_ITEM),
} CTL_SEN_PROC_TIME_ITEM;

typedef struct {
	CHAR func_name[CTL_SEN_NAME_LEN];
	CTL_SEN_ID id;
	CTL_SEN_PROC_TIME_ITEM item; // enter or exit
	UINT64 time_us_u64; // clock, us
} CTL_SEN_PROC_TIME;

typedef struct {
	CHAR func_name[CTL_SEN_NAME_LEN];
	CTL_SEN_ID id;
	CTL_SEN_PROC_TIME_ITEM item; // enter or exit
	UINT64 time_us_u64; // clock, us
	UINT32 cfg;
	UINT32 tag;
} CTL_SEN_PROC_TIME_ADV;

typedef struct {
	struct vos_mem_cma_info_t cma_info;
	VOS_MEM_CMA_HDL cma_hdl;
} CTL_SEN_VOS_MEM_INFO;

/* sensor ctrl api */
extern INT32 sen_ctrl_init_sendrv(CTL_SEN_ID id);
extern INT32 sen_ctrl_clk_prepare(CTL_SEN_ID id, CTL_SEN_MODE mode);
extern INT32 sen_ctrl_clk_unprepare(CTL_SEN_ID id, CTL_SEN_MODE mode);
extern INT32 sen_ctrl_cfg_pinmux(CTL_SEN_ID id, UINT32 cur_open_id_bit);
extern INT32 sen_ctrl_cfg_pinmux_cmdif(CTL_SEN_ID id, UINT32 cur_open_id_bit, BOOL en);
extern INT32 sen_ctrl_open(CTL_SEN_ID id);
extern INT32 sen_ctrl_close(CTL_SEN_ID id);
extern INT32 sen_ctrl_pwr_ctrl(CTL_SEN_ID id, CTL_SEN_PWR_CTRL_FLAG flag);
extern INT32 sen_ctrl_stop_if(CTL_SEN_ID id);
extern INT32 sen_ctrl_sleep(CTL_SEN_ID id);
extern INT32 sen_ctrl_wakeup(CTL_SEN_ID id);
extern INT32 sen_ctrl_write_reg(CTL_SEN_ID id, CTL_SEN_CMD *cmd);
extern INT32 sen_ctrl_read_reg(CTL_SEN_ID id, CTL_SEN_CMD *cmd);
extern INT32 sen_ctrl_chgmode(CTL_SEN_ID id, CTL_SEN_CHGMODE_OBJ chgmode_obj, CTL_SEN_MODE *chg_sen_mode);
extern INT32 sen_ctrl_set_cfg(CTL_SEN_ID id, CTL_SEN_CFGID cfg_id, void *data);
extern INT32 sen_ctrl_get_cfg(CTL_SEN_ID id, CTL_SEN_CFGID cfg_id, void *data);
extern CTL_SEN_INTE sen_ctrl_wait_interrupt(CTL_SEN_ID id, CTL_SEN_INTE waited_flag);
extern void sen_ctrl_dbg_info(CTL_SEN_ID id, CTL_SEN_DBG_SEL dbg_sel, UINT32 param);


/* sensor ctrl obj */
extern CTL_SEN_HDL_CONTEXT ctl_sen_hdl_ctx;
extern BOOL b_add_ctx[CTL_SEN_ID_MAX_SUPPORT];
extern CTL_SEN_INIT_OBJ *g_ctl_sen_init_obj[CTL_SEN_ID_MAX_SUPPORT];
extern CTL_SEN_CTRL_OBJ *g_ctl_sen_ctrl_obj[CTL_SEN_ID_MAX_SUPPORT];
extern SEN_MAP_TBL sen_map_tbl[CTL_SEN_ID_MAX_SUPPORT];

/* ssenif */
extern KDRV_SSENIF_INTERRUPT sen_ctrl_inte_cov2_ssenif(CTL_SEN_INTE inte);
extern CTL_SEN_INTE sen_ctrl_ssenifinte_cov2_sen(KDRV_SSENIF_INTERRUPT ssenif_inte);
extern UINT32 sen_ctrl_clanesel_conv2_ssenif(CTL_SEN_ID id, CTL_SEN_CLANE_SEL clane_sel);
extern KDRV_SSENIF_CAP sen_get_ssenif_cap_sie(UINT32 sie_id);
extern KDRV_SSENIF_CAP sen_get_ssenif_cap_dl(UINT32 in);
extern KDRV_SSENIF_CAP sen_get_ssenif_cap_ck(UINT32 in);
extern UINT32 sen_get_ssenif_cap(CTL_SEN_ID id);

/* clk info */
extern void sen_uti_set_clken_cb(CTL_SEN_ID id, CTL_SEN_CLK_CB cb);
extern CTL_SEN_CLK_CB sen_uti_get_clken_cb(CTL_SEN_ID id);
extern void sen_uti_set_mclk_src(CTL_SEN_CLK_SEL mclksel, CTL_SEN_CLKSRC_SEL clksel);
extern CTL_SEN_CLKSRC_SEL sen_uti_get_mclk_src(CTL_SEN_CLK_SEL mclksel);
extern void sen_uti_set_mclksel_freq(CTL_SEN_CLK_SEL mclksel, UINT32 freq);
extern UINT32 sen_uti_get_mclksel_freq(CTL_SEN_CLK_SEL mclksel);
extern UINT32 sen_uti_get_mclk_freq(CTL_SEN_ID id, CTL_SEN_MODE mode);
extern CTL_SEN_CLK_SEL sen_uti_get_mclksel(CTL_SEN_ID id, CTL_SEN_MODE mode);
extern CTL_SEN_CLK_SEL sen_conv_mclksel(CTL_SEN_ID id, CTL_SEN_SIEMCLK_SRC mclk_src);


/* tge */
#define TGE_CH_SFT0 0 // VD0/HD0 generator shift & idx
#define TGE_CH_SFT1 1 // VD1/HD1 generator shift & idx
#define TGE_CH_SFT2 2 // VD2/HD2 generator shift & idx
#define TGE_CH_SFT3 3 // VD3/HD3 generator shift & idx
#define TGE_CH_SFT4 4 // VD4/HD4 generator shift & idx
#define TGE_CH_SFT5 5 // VD5/HD5 generator shift & idx
#define TGE_CH_SFT6 6 // VD6/HD6 generator shift & idx
#define TGE_CH_SFT7 7 // VD7/HD7 generator shift & idx
#define TGE_CH_SWAP_MAP 4 // [680 only] CH15 & CH26 & CH37 & CH48
extern KDRV_TGE_CLK_SRC sen_ctrl_clksel_cov2_tge(CTL_SEN_CLK_SEL clc_sel);

/* common covert api */
extern UINT32 sen_fps_fmt_conv2_real(UINT32 sen_fps_fmt);
extern UINT32 sen_uint64_dividend(UINT64 dividend, UINT32 divisor);
extern BOOL sen_ctrl_chk_data_fmt_is_raw(CTL_SEN_DATA_FMT data_fmt);
extern BOOL sen_ctrl_chk_mode_type_is_multiframe(CTL_SEN_MODE_TYPE mode_type);

/* dbg info */
extern void ctl_sen_set_proc_time(CHAR *func_name, CTL_SEN_ID id, CTL_SEN_PROC_TIME_ITEM item);
extern CTL_SEN_PROC_TIME *ctl_sen_get_proc_time(void);
extern UINT32 ctl_sen_get_proc_time_cnt(void);
extern UINT32 ctl_sen_get_cur_state(CTL_SEN_ID id);
extern BOOL sen_ctrl_chk_add_ctx(CTL_SEN_ID id);

/* mem info */
extern INT32 ctl_sen_util_os_malloc(CTL_SEN_VOS_MEM_INFO *vod_mem_info, UINT32 req_size);
extern INT32 ctl_sen_util_os_mfree(CTL_SEN_VOS_MEM_INFO *vod_mem_info);

/* sensor driver operation api */
extern ER sendrv_get(CTL_SEN_ID id, CTL_SENDRV_CFGID sendrv_cfg_id, void *data);
extern void sen_uti_set_sendrv_en_mipi(CTL_SEN_ID id, CTL_SEN_CSI_EN_CB csi_en_cb);
extern BOOL sen_uti_get_sendrv_en_mipi(CTL_SEN_ID id);

/* ctl info */
extern void sen_set_open_id(CTL_SEN_ID id, BOOL open);
extern UINT32 sen_get_open_id(void);

/* dbg level */
typedef enum  {
	CTL_SEN_DBG_LV_NONE = 0,
	CTL_SEN_DBG_LV_ERR,
	CTL_SEN_DBG_LV_WRN,
	CTL_SEN_DBG_LV_IND,
	CTL_SEN_DBG_LV_MAX,
	ENUM_DUMMY4WORD(CTL_SEN_DBG_LV)
} CTL_SEN_DBG_LV;

/* error record op */
typedef enum  {
	CTL_SEN_ER_OP_INIT,
	CTL_SEN_ER_OP_REG,
	CTL_SEN_ER_OP_UNREG,
	CTL_SEN_ER_OP_OPEN,
	CTL_SEN_ER_OP_OPEN_CLK,
	CTL_SEN_ER_OP_OPEN_PINMUX,
	CTL_SEN_ER_OP_CLOSE,
	CTL_SEN_ER_OP_CLOSE_CLK,
	CTL_SEN_ER_OP_PWR,
	CTL_SEN_ER_OP_SLP,
	CTL_SEN_ER_OP_WUP,
	CTL_SEN_ER_OP_WR,
	CTL_SEN_ER_OP_RR,
	CTL_SEN_ER_OP_CHG,
	CTL_SEN_ER_OP_CHGFPS,
	CTL_SEN_ER_OP_SET,
	CTL_SEN_ER_OP_GET,
	CTL_SEN_ER_OP_BUF_QUE, // all sensor id sharing
	CTL_SEN_ER_OP_GLOBAL_INIT, // all sensor id sharing
	CTL_SEN_ER_OP_GLOBAL_UNINIT, // all sensor id sharing
	CTL_SEN_ER_OP_PRE_CLK, // clock prepare
	CTL_SEN_ER_OP_UNP_CLK, // clock unprepare
	CTL_SEN_ER_OP_MAX,
	ENUM_DUMMY4WORD(CTL_SEN_ER_OP)
} CTL_SEN_ER_OP;

/* error record item */
typedef enum  {
	CTL_SEN_ER_ITEM_OUTPUT, // ctl_sen output error code
	CTL_SEN_ER_ITEM_SYS,
	CTL_SEN_ER_ITEM_CMDIF,  // for sen_ctrl_cmdif_get_obj
	CTL_SEN_ER_ITEM_IF,     // for sen_ctrl_if_get_obj
	CTL_SEN_ER_ITEM_SENDRV, // for sen_ctrl_drv_get_obj

	CTL_SEN_ER_ITEM_MAX,
	ENUM_DUMMY4WORD(CTL_SEN_ER_ITEM)
} CTL_SEN_ER_ITEM;

#define CTL_SEN_ALIGN_ROUNDUP(src, align)   ALIGN_CEIL(src, align)
#define CTL_SEN_ALIGN_ROUNDDOWN(src, align) ALIGN_FLOOR(src, align)
#define CTL_SEN_ALIGN_ROUND(src, align)     ALIGN_ROUND(src, align)

extern CTL_SEN_DBG_LV ctl_sen_dbg_level;
#define CTL_SEN_DBG_ERR(fmt, args...)    { if (ctl_sen_dbg_level >= CTL_SEN_DBG_LV_ERR) { DBG_ERR(fmt, ##args);  } }
#define CTL_SEN_DBG_WRN(fmt, args...)    { if (ctl_sen_dbg_level >= CTL_SEN_DBG_LV_WRN) { DBG_WRN(fmt, ##args);  } }
#define CTL_SEN_DBG_IND(fmt, args...)    { if (ctl_sen_dbg_level >= CTL_SEN_DBG_LV_IND) { DBG_IND(fmt, ##args);  } }


#endif

