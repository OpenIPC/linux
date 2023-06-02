/**
 * @file kdrv_ipp_builtin.h
 * @brief type definition of KDRV API.
 */

#ifndef __KDRV_IPP_BUILTIN_H__
#define __KDRV_IPP_BUILTIN_H__

#include "kwrap/type.h"
#include "kwrap/perf.h"
#define IPP_DRIVER_NEW_REG_HANDLE    ENABLE

#define KDRV_IPP_BUILTIN_BUF_LCA_NUM 2
#define KDRV_IPP_BUILTIN_BUF_3DNR_NUM 2
#define KDRV_IPP_BUILTIN_BUF_3DNR_STA_NUM 1
#define KDRV_IPP_BUILTIN_BUF_3DNR_FCVR_NUM 2
#define KDRV_IPP_BUILTIN_BUF_DFG_NUM 2
#define KDRV_IPP_BUILTIN_BUF_WDR_NUM 2
#define KDRV_IPP_BUILTIN_BUF_PM_NUM 3
#define KDRV_IPP_BUILTIN_BUF_VA_NUM 2
#define KDRV_IPP_BUILTIN_BUF_DUMMY_UV_NUM 1


#define KDRV_IPP_BUILTIN_PRI_MASK_NUM (8)
#define KDRV_IPP_BUILTIN_PRI_MASK_REG_BASE03 (0x420)
#define KDRV_IPP_BUILTIN_PRI_MASK_REG_BASE47 (0x4B4)
#define KDRV_IPP_BUILTIN_PRI_MASK_REG_OFS  (0x24)
typedef struct {
	UINT8 enable;
	UINT8 hlw_enable;
	IPOINT coord[4];
	IPOINT coord2[4];
	UINT8 color[3];
	UINT8 weight;
} KDRV_IPP_BUILTIN_PRI_MASK;

typedef union {
	UINT32 val;
	struct {
		UINT32 type : 1;
		UINT32 pxlsize : 2;
		UINT32 : 1;
		UINT32 color_y : 8;
		UINT32 color_u : 8;
		UINT32 color_v : 8;
		UINT32 : 3;
		UINT32 hlw_en : 1;
	} bit;
} KDRV_IPP_BUILTIN_PRI_MASK_REG_CTRL;

typedef union {
	UINT32 val;
	struct {
		UINT32 comp : 2;
		UINT32 : 2;
		UINT32 coefa : 13;
		UINT32 signa : 1;
		UINT32 coefb : 13;
		UINT32 signb : 1;
	} bit_set0;
	struct {
		UINT32 coefc : 26;
		UINT32 signc : 1;
	} bit_set1;
} KDRV_IPP_BUILTIN_PRI_MASK_REG_LINE;

typedef struct {
	UINT8 skip;
	UINT16 src;
	UINT16 dst;
	UINT32 drop_rate;
	UINT32 rate_cnt;
} KDRV_IPP_BUILTIN_RATE_CTL;

typedef struct {
	UINT32 addr;
	UINT32 size;
	UINT32 phy_addr;
} KDRV_IPP_BUILTIN_BLK;


typedef struct {
	UINT32 id;
	UINT32 dbg_func_en;
	BOOL   en;
} KDRV_IPP_BUILTIN_DBG_FUNC_STRC;

typedef struct {
	KDRV_IPP_BUILTIN_BLK lca[KDRV_IPP_BUILTIN_BUF_LCA_NUM];
	KDRV_IPP_BUILTIN_BLK _3dnr_mv[KDRV_IPP_BUILTIN_BUF_3DNR_NUM];
	KDRV_IPP_BUILTIN_BLK _3dnr_ms[KDRV_IPP_BUILTIN_BUF_3DNR_NUM];
	KDRV_IPP_BUILTIN_BLK _3dnr_ms_roi[KDRV_IPP_BUILTIN_BUF_3DNR_NUM];
	KDRV_IPP_BUILTIN_BLK _3dnr_sta[KDRV_IPP_BUILTIN_BUF_3DNR_STA_NUM];
	KDRV_IPP_BUILTIN_BLK _3dnr_fcvg[KDRV_IPP_BUILTIN_BUF_3DNR_FCVR_NUM];
	KDRV_IPP_BUILTIN_BLK defog[KDRV_IPP_BUILTIN_BUF_DFG_NUM];
	KDRV_IPP_BUILTIN_BLK pm[KDRV_IPP_BUILTIN_BUF_PM_NUM];
	KDRV_IPP_BUILTIN_BLK wdr[KDRV_IPP_BUILTIN_BUF_WDR_NUM];
	KDRV_IPP_BUILTIN_BLK va[KDRV_IPP_BUILTIN_BUF_VA_NUM];
	KDRV_IPP_BUILTIN_BLK dummy_uv[KDRV_IPP_BUILTIN_BUF_DUMMY_UV_NUM];
} KDRV_IPP_BUILTIN_PRI_BUF;


typedef enum {
	KDRV_IPP_PARAM_BUILTIN_SET_ALGO_BYPASS_EN = 0,	    /* Set IQ/3A by pass mode enable/disable */
	KDRV_IPP_PARAM_BUILTIN_SET_STAMP_MODE_EN,           /* Set Stamp debug mode enable/disable   */
	KDRV_IPP_PARAM_BUILTIN_SET_PAUSE_IPP_OUTPUT,        /* Set stream pause enable/disable   */
	KDRV_IPP_PARAM_BUILTIN_SET_DBG_FUNC_EN,             /* Set debug function enable/disable */
	KDRV_IPP_PARAM_BUILTIN_SET_ID_MAX,
} KDRV_IPP_BUILTIN_SET_PARAM_ID;

typedef enum {
	KDRV_IPP_PARAM_BUILTIN_GET_DBGINFO = 0,	/* Get debug information*/
	KDRV_IPP_PARAM_BUILTIN_GET_ID_MAX,
} KDRV_IPP_BUILTIN_GET_PARAM_ID;

typedef enum {
	KDRV_IPP_BUILTIN_IFE,
	KDRV_IPP_BUILTIN_DCE,
	KDRV_IPP_BUILTIN_IPE,
	KDRV_IPP_BUILTIN_IME,
	KDRV_IPP_BUILTIN_ENG_MAX,
} KDRV_IPP_BUILTIN_ENG;


/* REFER TO CTL_IPP_FUNC */
typedef enum {
	KDRV_IPP_BUILTIN_FUNC_NONE              =   0x00000000,
	KDRV_IPP_BUILTIN_FUNC_WDR               =   0x00000001,
	KDRV_IPP_BUILTIN_FUNC_SHDR              =   0x00000002,
	KDRV_IPP_BUILTIN_FUNC_DEFOG             =   0x00000004,
	KDRV_IPP_BUILTIN_FUNC_3DNR              =   0x00000008,
	KDRV_IPP_BUILTIN_FUNC_DATASTAMP         =   0x00000010,
	KDRV_IPP_BUILTIN_FUNC_PRIMASK           =   0x00000020,
	KDRV_IPP_BUILTIN_FUNC_PM_PIXELIZTION    =   0x00000040,
	KDRV_IPP_BUILTIN_FUNC_YUV_SUBOUT        =   0x00000080,
	KDRV_IPP_BUILTIN_FUNC_VA_SUBOUT         =   0x00000100,
	KDRV_IPP_BUILTIN_FUNC_3DNR_STA          =   0x00000200,
	KDRV_IPP_BUILTIN_FUNC_GDC               =   0x00000400,
} KDRV_IPP_BUILTIN_FUNC;

/* REFER TO CTL_IPP_DBG_FUNC */
typedef enum {
	KDRV_IPP_BUILTIN_FUNC_ALGO_BY_PASS       =   0x00000001,
	KDRV_IPP_BUILTIN_FUNC_STAMP_MODE         =   0x00000002,
	KDRV_IPP_BUILTIN_FUNC_RESTORE_LAST_FRAME =   0x00000004,
} KDRV_IPP_BUILTIN_DBG_FUNC;

/* callback for push buffer */
#define KDRV_IPP_BUITLIN_MAX_PLANE (4)

typedef enum {
	KDRV_IPP_BUILTIN_PATH_ID_1 = 0,
	KDRV_IPP_BUILTIN_PATH_ID_2 = 1,
	KDRV_IPP_BUILTIN_PATH_ID_3 = 2,
	KDRV_IPP_BUILTIN_PATH_ID_4 = 3,
	KDRV_IPP_BUILTIN_PATH_ID_5 = 4,
	KDRV_IPP_BUILTIN_PATH_ID_6 = 5,
	KDRV_IPP_BUILTIN_PATH_ID_MAX = 6,
} KDRV_IPP_BUILTIN_PATH_ID;

typedef enum {
	KDRV_IPP_BUILTIN_FMT_Y8            = 0x51080400, ///< 1 plane, pixel=Y(w,h)
	KDRV_IPP_BUILTIN_FMT_YUV420        = 0x520c0420, ///< 2 plane, pixel=Y(w,h), UV(w/2,h/2)
	KDRV_IPP_BUILTIN_FMT_YUV420_PLANAR = 0x530c0420, ///< 3 plane, pixel=Y(w,h), U(w/2,h/2), and V(w/2,h/2)
	KDRV_IPP_BUILTIN_FMT_NVX2          = 0x610c2420, ///< novatek-yuv-compress-2 of YUV420 (using by NA51023)
} KDRV_IPP_BUILTIN_FMT;

typedef enum {
	KDRV_IPP_BUILTIN_ISP_EVENT_RESET = 0,
	KDRV_IPP_BUILTIN_ISP_EVENT_TRIG,
	KDRV_IPP_BUILTIN_ISP_EVENT_MAX,
} KDRV_IPP_BUILTIN_ISP_EVENT;

typedef struct {
	BOOL enable;
	ISIZE size;
	KDRV_IPP_BUILTIN_FMT fmt;
	UINT32 loff[KDRV_IPP_BUITLIN_MAX_PLANE];
	UINT32 addr[KDRV_IPP_BUITLIN_MAX_PLANE];
	UINT32 phyaddr[KDRV_IPP_BUITLIN_MAX_PLANE];
	VOS_TICK timestamp;
} KDRV_IPP_BUILTIN_IMG_INFO;

typedef struct {
	const CHAR *name;
	KDRV_IPP_BUILTIN_IMG_INFO out_img[KDRV_IPP_BUILTIN_PATH_ID_MAX];
	UINT32 release_flg;
} KDRV_IPP_BUILTIN_FMD_CB_INFO;

typedef struct {
	UINT32 wdr_addr;
	UINT32 defog_addr;
	UINT32 lca_addr;
	UINT32 _3dnr_mv_addr;
	UINT32 _3dnr_ms_addr;
	UINT32 _3dnr_fcvg_addr;
	UINT32 path_addr_y[KDRV_IPP_BUILTIN_PATH_ID_MAX];
	UINT32 path_addr_u[KDRV_IPP_BUILTIN_PATH_ID_MAX];
	UINT32 path_addr_v[KDRV_IPP_BUILTIN_PATH_ID_MAX];
	UINT32 ime_dram_end_status;
	/* info for lca */
	UINT32 gray_avg_u;
	UINT32 gray_avg_v;
} KDRV_IPP_BUILTIN_PHY_OUT_INFO;

#define KDRV_IPP_BUILTIN_ISP_HIST_NUM		(128)
typedef struct {
	UINT8 enable;
	UINT8 sel;
    UINT16 stcs[KDRV_IPP_BUILTIN_ISP_HIST_NUM];
} KDRV_IPP_BUILTIN_ISP_HIST_RST;

typedef struct {
	UINT32 localmax_max;
	UINT32 coneng_max;
	UINT32 coneng_avg;
} KDRV_IPP_BUILTIN_ISP_EDGE_STCS;

#define KDRV_IPP_BUILTIN_ISP_DEFOG_AIRLIGHT_NUM (3)
typedef struct {
	UINT16 airlight[KDRV_IPP_BUILTIN_ISP_DEFOG_AIRLIGHT_NUM];
} KDRV_IPP_BUILTIN_ISP_DEFOG_STCS;

typedef struct {
	KDRV_IPP_BUILTIN_ISP_HIST_RST hist_rst;
	KDRV_IPP_BUILTIN_ISP_EDGE_STCS edge_stcs;
	KDRV_IPP_BUILTIN_ISP_DEFOG_STCS defog_stcs;
	UINT32 defog_subout_addr;
} KDRV_IPP_BUILTIN_ISP_INFO;

typedef void (*KDRV_IPP_BUILTIN_FMD_CB)(KDRV_IPP_BUILTIN_FMD_CB_INFO *p_info, UINT32 reserved);
typedef void (*KDRV_IPP_BUILTIN_ISP_CB)(UINT32 id, UINT32 status);
INT32 kdrv_ipp_builtin_reg_fmd_cb(KDRV_IPP_BUILTIN_FMD_CB fp);
INT32 kdrv_ipp_builtin_reg_isp_cb(KDRV_IPP_BUILTIN_ISP_CB fp);

KDRV_IPP_BUILTIN_IMG_INFO kdrv_ipp_builtin_get_path_info(const CHAR *name, KDRV_IPP_BUILTIN_PATH_ID pid);
KDRV_IPP_BUILTIN_PHY_OUT_INFO *kdrv_ipp_builtin_get_phy_out_info(CHAR *name);
KDRV_IPP_BUILTIN_ISP_INFO* kdrv_ipp_builtin_get_isp_info(CHAR *name);

/**
    init fastboot flow
*/
#define KDRV_IPP_MAX_HANDLE_NUM (4) //tmp need check
typedef struct {
	/* ime output buffer info */
	const CHAR *name;
    UINT32 src_sie_id_bit;

    /* ipp internal buffer info */
    UINT32 ctrl_blk_addr;
    UINT32 ctrl_blk_size;

    /* shdr ring buffer info */
    UINT32 shdr_ring_buf_addr;
    UINT32 shdr_ring_buf_size;
} KDRV_IPP_BUILTIN_INIT_HDL;

typedef struct {
    UINT8 hdl_num;
	UINT32 valid_src_id_bit;
    KDRV_IPP_BUILTIN_INIT_HDL hdl_info[KDRV_IPP_MAX_HANDLE_NUM];
} KDRV_IPP_BUILTIN_INIT_INFO;

INT32 kdrv_ipp_builtin_init(KDRV_IPP_BUILTIN_INIT_INFO *info);
INT32 kdrv_ipp_builtin_exit(void);
INT32 kdrv_ipp_builtin_unlock_3dnr_ref(void);
void kdrv_ipp_builtin_update_timestamp(void);
BOOL kdrv_ipp_builtin_get_status(void);
BOOL kdrv_ipp_builtin_is_direct(void);
BOOL kdrv_ipp_builtin_is_fastboot(void);
/**
	dump register config for new driver
*/
typedef struct {
	void (*get_hdl_list)(UINT32 *hdl_list, UINT32 list_size);
	void (*get_reg_dtsi)(UINT32 hdl, UINT32 cnt, UINT32 **p_reg);
} KDRV_IPP_BUILTIN_DTSI_CB;

UINT32 kdrv_ipp_builtin_get_reg_num(KDRV_IPP_BUILTIN_ENG eng);
void kdrv_ipp_builtin_reg_dtsi_cb(KDRV_IPP_BUILTIN_DTSI_CB cb);
void kdrv_ipp_builtin_get_hdal_hdl_list(UINT32 *hdl_list, UINT32 list_size);
void kdrv_ipp_builtin_get_hdal_reg_dtsi(UINT32 hdl, UINT32 cnt, UINT32 **p_reg);

/**
	dump frame rate ctrl dtsi
*/
int kdrv_ipp_builtin_frc_dump(char *frc_node, int fd, char *pre_fix);

/**
	builtin ime interface
*/
typedef void (*IME_BUILTIN_ISR_CB)(UINT32 dev_id);

typedef struct {
	void *p_mclk;		/* clk handle for linux */
	void *p_io_addr;	/* virtual register address */
	UINT32 phy_io_base;	/* physical register address */
	UINT32 phy_io_size;	/* physical register size */
	INT32 irq_id;		/* interrupt id */
} NVT_IME_BUILTIN_RESOURCE;

void ime_builtin_reg_isr_cb(UINT32 dev_id, IME_BUILTIN_ISR_CB cb);
void ime_builtin_set_switch_to_hdal(UINT32 dev_id);
NVT_IME_BUILTIN_RESOURCE *ime_builtin_get_resource(UINT32 idx);

/**
    ipp builtin universal set/get interface
*/
INT32 kdrv_ipp_builtin_set(void *p_hdl, UINT32 param_id, void *p_data);
INT32 kdrv_ipp_builtin_get(void *p_hdl, UINT32 param_id, void *p_data);

#endif

