#ifndef __KDRV_VPE_H__
#define __KDRV_VPE_H__

#include "kwrap/type.h"

#if defined(__LINUX)
#include <linux/list.h>
#endif


#define KDRV_VPE_MAX_IMG                4
#define KDRV_VPE_SUPPORT_DEWARP         1
#define KDRV_VPE_SUPPORT_PARANOMA       1
#define KDRV_VPE_COLUMN_NUMS            8
#define KDRV_VPE_NOISE_CURVE_NUMS       17
#define KDRV_VPE_SCA_FILTER_COEF_NUMS   4
#define KDRV_VPE_GEO_LUT_NUMS           65

#if defined(__LINUX)
typedef struct list_head    KDRV_VPE_LIST_HEAD;
typedef struct kmem_cache   KDRV_VPE_MEM_CACHE;

#else
/* temp for freertos build */
typedef UINT32  KDRV_VPE_LIST_HEAD;
typedef UINT32  KDRV_VPE_MEM_CACHE;
#endif





typedef enum {
	KDRV_VPE_SRC_TYPE_YUV420 = 4,
	ENUM_DUMMY4WORD(KDRV_VPE_SRC_TYPE)
} KDRV_VPE_SRC_TYPE;

typedef enum {
	KDRV_VPE_ROTATION_ROT_BYPASS  = 0,
	ENUM_DUMMY4WORD(KDRV_VPE_VPE_ROTATION)
} KDRV_VPE_ROTATION;

typedef enum {
	KDRV_VPE_DES_TYPE_YUV420  = 0,   ///< YUV420 without compression
	KDRV_VPE_DES_TYPE_YCC_YUV420,    ///< YUV420 with compression
	ENUM_DUMMY4WORD(KDRV_VPE_DES_TYPE)
} KDRV_VPE_DES_TYPE;

typedef enum {
	KDRV_VPE_FUNC_NONE_EN =  0x0,
	KDRV_VPE_FUNC_SHP_EN =   0x1,
	KDRV_VPE_FUNC_DCE_EN =   0x2,
	KDRV_VPE_FUNC_DCTG_EN =  0x4,
	ENUM_DUMMY4WORD(KDRV_VPE_FUNC)
} KDRV_VPE_FUNC;

typedef enum {
	KDRV_VPE_STATUS_NONE =  0x0,
	KDRV_VPE_STATUS_READY,
	KDRV_VPE_STATUS_PARAM_FAIL,
	KDRV_VPE_STATUS_FIRE_FAIL,
	KDRV_VPE_STATUS_DONE,
	ENUM_DUMMY4WORD(KDRV_VPE_JOB_STATUS)
} KDRV_VPE_JOB_STATUS;

#if KDRV_VPE_SUPPORT_DEWARP
typedef enum {
	KDRV_VPE_COL_TYPE_NORMAL =  0x0,
	KDRV_VPE_COL_TYPE_DCTG =  0x1,
#if KDRV_VPE_SUPPORT_PARANOMA
	KDRV_VPE_COL_TYPE_PARANOMA = 0x2,
#endif
	ENUM_DUMMY4WORD(KDRV_VPE_COL_TYPE)
} KDRV_VPE_COL_TYPE;
#endif


/**
    KDRV VPE enum - interrupt enable enum.
*/
typedef enum _KDRV_VPE_INTE {
	KDRV_VPE_INTE_FRM_END           = 0x00000001,  ///< frame-end interrupt status
	KDRV_VPE_INTE_LL_ERR            = 0x00000002,  ///< invalid link-list command interrupt status
	KDRV_VPE_INTE_LL_END            = 0x00000004,  ///< all link-list job done interrupt status
	KDRV_VPE_INTE_RES0_NER_FRM_END  = 0x00000400,  ///< resolution 0 near frame-end interrupt status
	KDRV_VPE_INTE_RES1_NER_FRM_END  = 0x00000800,  ///< resolution 1 near frame-end interrupt status
	KDRV_VPE_INTE_RES2_NER_FRM_END  = 0x00001000,  ///< resolution 2 near frame-end interrupt status
	KDRV_VPE_INTE_RES3_NER_FRM_END  = 0x00002000,  ///< resolution 3 near frame-end interrupt status
	ENUM_DUMMY4WORD(KDRV_VPE_INTE)
} KDRV_VPE_INTE;

/**
    KDRV VPE enum - KDRV_VPE_SET_ISRCB status
*/
typedef enum {
	KDRV_VPE_INTS_FRM_END           = 0x00000001,  ///< frame-end interrupt status
	KDRV_VPE_INTS_LL_ERR            = 0x00000002,  ///< invalid link-list command interrupt status
	KDRV_VPE_INTS_LL_END            = 0x00000004,  ///< all link-list job done interrupt status
	KDRV_VPE_INTS_RES0_NER_FRM_END  = 0x00000400,  ///< resolution 0 near frame-end interrupt status
	KDRV_VPE_INTS_RES1_NER_FRM_END  = 0x00000800,  ///< resolution 1 near frame-end interrupt status
	KDRV_VPE_INTS_RES2_NER_FRM_END  = 0x00001000,  ///< resolution 2 near frame-end interrupt status
	KDRV_VPE_INTS_RES3_NER_FRM_END  = 0x00002000,  ///< resolution 3 near frame-end interrupt status
	ENUM_DUMMY4WORD(KDRV_VPE_ISR_STATUS)
} KDRV_VPE_ISR_STATUS;


typedef struct _KDRV_VPE_GLB_IMG_INFO_ {
	UINT8       src_uv_swap;
	UINT16      src_width;
	UINT16      src_height;
	UINT16      src_in_w;
	UINT16      src_in_h;
	UINT16      src_in_x;
	UINT16      src_in_y;
	UINT32      src_addr;
	UINT8       src_drt;
} KDRV_VPE_GLB_IMG_INFO;

typedef struct _KDRV_VPE_OUT_DUP_INFO_ {
	KDRV_VPE_DES_TYPE       des_type;
	UINT32                  out_addr;
} KDRV_VPE_OUT_DUP_INFO;

typedef struct _KDRV_VPE_RES_DIM_INFO_ {

	UINT16  sca_crop_w;  // image crop width before scaling
	UINT16  sca_crop_h;  // image crop height before scaling
	UINT16  sca_crop_x;  // image crop x start position before scaling
	UINT16  sca_crop_y;  // image crop y start position before scaling

	UINT16  sca_width;   // scale width
	UINT16  sca_height;  // scale height

	UINT16  des_crop_out_w;  // crop output width after scaling
	UINT16  des_crop_out_h;  // crop output height after scaling
	UINT16  des_crop_out_x;  // crop output x start position after scaling
	UINT16  des_crop_out_y;  // crop output y start position after scaling

	UINT16  des_rlt_w;   // result image width
	UINT16  des_rlt_h;   // result image height
	UINT16  des_rlt_x;   // result image x start position
	UINT16  des_rlt_y;   // result image y start position

	UINT16  hole_w;
	UINT16  hole_h;
	UINT16  hole_x;
	UINT16  hole_y;

	UINT16  des_out_w;   // scale output width
	UINT16  des_out_h;   // scale output height
	UINT16  des_out_x;   // scale output x start position
	UINT16  des_out_y;   // scale output y start position

	UINT16  des_width;   // output background width
	UINT16  des_height;  // output background height
} KDRV_VPE_RES_DIM_INFO;

typedef struct _KDRV_VPE_COLUMN_SIZE_INFO_ {
	UINT16                         proc_width;
	UINT16                         proc_x_start;
	UINT16                         col_x_start;
} KDRV_VPE_COLUMN_SIZE_INFO;

typedef struct _KDRV_VPE_RES_SCA_INFO_ {
	UINT8                          sca_iq_en;//0: driver config, 1: IQ config
	UINT8                          sca_y_luma_algo_en;
	UINT8                          sca_x_luma_algo_en;
	UINT8                          sca_y_chroma_algo_en;
	UINT8                          sca_x_chroma_algo_en;
	UINT8                          sca_map_sel;
	UINT16                         sca_ceff_h[KDRV_VPE_SCA_FILTER_COEF_NUMS];
	UINT16                         sca_ceff_v[KDRV_VPE_SCA_FILTER_COEF_NUMS];
} KDRV_VPE_RES_SCA_INFO;


typedef struct _KDRV_VPE_COL_INFO_ {
	UINT16                          col_num; // 0: one column
	KDRV_VPE_COLUMN_SIZE_INFO       col_size_info[KDRV_VPE_COLUMN_NUMS];
} KDRV_VPE_COL_INFO;

typedef struct _KDRV_VPE_OUT_IMG_INFO_ {
	UINT8                   out_dup_num;    ///<  output duplicate numbers
	KDRV_VPE_OUT_DUP_INFO   out_dup_info;   ///<  output duplicate control infomation
	KDRV_VPE_RES_DIM_INFO   res_dim_info;
	KDRV_VPE_RES_SCA_INFO   res_sca_info;
	UINT8                   out_bg_sel;
	UINT8                   res_des_drt;
	UINT8                   res_uv_swap;
} KDRV_VPE_OUT_IMG_INFO;


typedef struct KDRV_VPE_COL_INFO_GET {
	UINT16                      src_width;
	UINT16                      src_height;
	UINT16                      src_in_width; // src in width
	UINT16                      src_in_height;
	UINT16                      src_in_x;
	UINT16                      src_in_y;
	UINT16                      out_width[KDRV_VPE_MAX_IMG];
	KDRV_VPE_DES_TYPE           des_type[KDRV_VPE_MAX_IMG];
	KDRV_VPE_SRC_TYPE           src_type;
	UINT8                       col_num; // 0: one column
	KDRV_VPE_COLUMN_SIZE_INFO   col_size_info[KDRV_VPE_COLUMN_NUMS];

	//#if KDRV_VPE_SUPPORT_DEWARP
	UINT8                       column_type;
	//#endif
} KDRV_VPE_COL_INFO_GET;



typedef struct _KDRV_VPE_SHARPEN_PARAM_ {
	UINT8  edge_weight_src_sel;                     ///< Select source of edge weight calculation, 0~1
	UINT8  edge_weight_th;                          ///< Edge weight coring threshold, 0~255
	UINT8  edge_weight_gain;                        ///< Edge weight gain, 0~255
	UINT8  noise_level;                             ///< Noise Level, 0~255
	UINT8  noise_curve[KDRV_VPE_NOISE_CURVE_NUMS]; ///< 17 control points of noise modulation curve, 0~255
	UINT8  blend_inv_gamma;                         ///< Blending ratio of HPF results, 0~128
	UINT8  edge_sharp_str1;                         ///< Sharpen strength1 of edge region, 0~255
	UINT8  edge_sharp_str2;                         ///< Sharpen strength2 of edge region, 0~255
	UINT8  flat_sharp_str;                          ///< Sharpen strength of flat region,0~255
	UINT8  coring_th;                               ///< Coring threshold, 0~255
	UINT8  bright_halo_clip;                        ///< Bright halo clip ratio, 0~255
	UINT8  dark_halo_clip;                          ///< Dark halo clip ratio, 0~255
	UINT8  sharpen_out_sel;
} KDRV_VPE_SHARPEN_PARAM;



typedef struct _KDRV_VPE_PALETTE_PARAM_ {
	INT32 pal_y;
	INT32 pal_cb;
	INT32 pal_cr;
} KDRV_VPE_PALETTE_PARAM;


typedef struct _KDRV_VPE_DCE_PARAM_ {
	UINT8   dce_mode;
	UINT8   lut2d_sz;
	UINT8   lsb_rand;
	UINT8   fovbound;
	UINT16  boundy;
	UINT16  boundu;
	UINT16  boundv;
	UINT16  cent_x_s;
	UINT16  cent_y_s;
	UINT16  xdist_a1;
	UINT16  ydist_a1;
	UINT16  normfact;
	UINT8   normbit;
	UINT16  fovgain;
	UINT32  hfact;
	UINT32  vfact;
	UINT8   xofs_i;
	UINT32  xofs_f;
	UINT8   yofs_i;
	UINT32  yofs_f;
	UINT16  geo_lut[KDRV_VPE_GEO_LUT_NUMS];
	UINT8   dce_2d_lut_en;
	UINT32  dce_l2d_addr;
	UINT16  dewarp_width;
	UINT16  dewarp_height;

	UINT32  lens_radius;
} KDRV_VPE_DCE_PARAM;

typedef struct _KDRV_VPE_DCTG_PARAM_ {
	KDRV_VPE_COL_TYPE dewarp_type;
	UINT8  mount_type;
	UINT8  ra_en;
	UINT8  lut2d_sz;
	UINT16 lens_r;
	UINT16 lens_x_st;
	UINT16 lens_y_st;
	INT32  theta_st;
	INT32  theta_ed;
	INT32  phi_st;
	INT32  phi_ed;
	INT32  rot_z;
	INT32  rot_y;
	UINT32 dctg_gamma_addr;
} KDRV_VPE_DCTG_PARAM;


typedef struct _KDRV_VPE_CONFIG_ {
	KDRV_VPE_FUNC               func_mode;
	KDRV_VPE_GLB_IMG_INFO       glb_img_info;
	KDRV_VPE_OUT_IMG_INFO       out_img_info[KDRV_VPE_MAX_IMG];
	KDRV_VPE_COL_INFO           col_info;
	KDRV_VPE_SHARPEN_PARAM      sharpen_param;
	KDRV_VPE_PALETTE_PARAM      palette[8];
	KDRV_VPE_DCE_PARAM          dce_param;
	KDRV_VPE_DCTG_PARAM         dctg_param;
	KDRV_VPE_JOB_STATUS         job_status;

	KDRV_VPE_LIST_HEAD          config_list;   ///< for linux
} KDRV_VPE_CONFIG;

typedef struct _KDRV_VPE_JOB_LIST_ {
	KDRV_VPE_LIST_HEAD          job_list;
	void (*callback)(void *param);
	UINT32                      job_num;
	KDRV_VPE_MEM_CACHE          *p_job_cache;
	KDRV_VPE_LIST_HEAD          drv_job_list; //for drv
} KDRV_VPE_JOB_LIST;

//------------------------------------------------------------------------------------------

/**
    KDRV VPE structure - vpe open object
*/
typedef struct _KDRV_VPE_OPENCFG_ {
	UINT32 vpe_clock_sel;               ///< Engine clock selection
} KDRV_VPE_OPENCFG, *pKDRV_VPE_OPENCFG;



/**
    KDRV VPE PARAM ITEM, for kdrv_vpe_set/kdrv_vpe_get
*/
typedef enum {
	KDRV_VPE_PARAM_IPL_OPENCFG = 0,         ///< vpe open engine info,                             data_type: [Set]     KDRV_VPE_OPENCFG
	KDRV_VPE_PARAM_IPL_ISR_CB,              ///< VPE external isr cb

	KDRV_VPE_PARAM_MAX,
	KDRV_VPE_PARAM_REV = 0x80000000,
	ENUM_DUMMY4WORD(KDRV_VPE_PARAM_ID)
} KDRV_VPE_PARAM_ID;
STATIC_ASSERT((KDRV_VPE_PARAM_MAX &KDRV_VPE_PARAM_REV) == 0);

#define KDRV_VPE_IGN_CHK KDRV_VPE_PARAM_REV //only support set/get function


//------------------------------------------------------------------------------------------

extern INT32 kdrv_vpe_open(UINT32 chip, UINT32 engine);
extern INT32 kdrv_vpe_set(UINT32 id, KDRV_VPE_PARAM_ID param_id, void *p_param);
extern INT32 kdrv_vpe_trigger(UINT32 handle, KDRV_VPE_JOB_LIST *p_job_list);
extern INT32 kdrv_vpe_close(UINT32 handle);
extern INT32 kdrv_vpe_get(UINT32 handle, UINT32 parm_id, KDRV_VPE_COL_INFO_GET *p_col_info_get);


#endif

