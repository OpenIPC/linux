#ifndef _EMU_H264_COMMON_H_
#define _EMU_H264_COMMON_H_

#include "kwrap/type.h"

// for hw trigger //
#include "h26x.h"

// for sw driver //
#include "h26x_def.h"
#include "h26xenc_api.h"
#include "h264enc_api.h"

// for emulation //
#include "emu_h26x_common.h"

#define cmp_smart_rec 0 //520 x
#define H264_LINEOFFSET_MAX 5120


//#if (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE)
typedef struct _file_t_{
	H26XFile info;
	H26XFile src;
	H26XFile es;
	H26XFile slice_hdr_len;
	H26XFile slice_hdr_es;
	H26XFile slice_len;
	H26XFile mbqp;
	H26XFile seq;
	H26XFile pic;
	H26XFile chk;
	H26XFile sdebsy;
	H26XFile sdebsuv;
	H26XFile tmnr_mt;
	H26XFile osg_grap[32];
	H26XFile motion_bit;
	H26XFile colmv;
	H26XFile source_out;

	//decode
	H26XFile video_hdr_len;
	H26XFile video_hdr_es; // sps + pps
	//sdec
	H26XFile sde_reg;
}file_t;

typedef struct _info_t_{
	unsigned char yuv_name[256];
	unsigned int dump_src_en;
	unsigned int width;
	unsigned int height;
	unsigned int frame_num;
	unsigned int cmd_len;
}info_t;
typedef enum{
	FF_RDO = 0,
	FF_VAR,
	FF_FRO,
	FF_MASK,
	FF_GDR,
	FF_ROI,
	FF_RRC_SEQ,
	FF_RRC_PIC,
	FF_SRAQ,
	FF_LPM,
	FF_RND,
	FF_SCD,
	FF_TMNR,
	FF_OSG,
	FF_MAQ,
	FF_JND,
    FF_BGR,
    FF_DITHER,
    FF_RMD,
    FF_TNR,
    FF_LAMBDA,
    FF_ESKIP,
    FF_SPN,
	FF_MAX_NUM = 32
}e_FPGA_FUNC; // FF = FPGA_FUNC

typedef struct _seq_t_{
	unsigned int obj_size[FF_MAX_NUM];

	unsigned int width;
	unsigned int height;

	unsigned int dis_loopfilter_idc;
	int dblk_alpha;
	int dblk_beta;

	int chrm_qp_offset;
	int sec_chrm_qp_offset;

	unsigned char min_qp;
	unsigned char max_qp;

	unsigned char fbc_en;
	unsigned char gray_en;
    unsigned char gray_mode_color_en;
	unsigned char fastsearch_en;
	unsigned char entropy_coding;
	unsigned char tran8x8;
	unsigned int  slice_row_num;

	// fpga only (hack parameter) : differ from driver settings  //
	unsigned char icm_use_rec_pixel;
	unsigned char cabac_init_idc;
	unsigned int  log2_max_fno;
	unsigned int  log2_max_poc;
	unsigned char sraq_save_dqp_en;
	unsigned char ime_left_amvp_mode;
	unsigned char sdecmps_en;
	unsigned char sdecmps_rotate;
	unsigned char sdecmps_cbcr_iv;
	unsigned char hw_pad_en;
	unsigned int  tmnr_his_size;
	unsigned char flxsr_en;
	unsigned char force_skip_flag;
} seq_t;

typedef struct _pic_t_{
	unsigned char slice_type;
	unsigned char qp;
	unsigned int  total_slice_hdr_len;

	// fpga only //
	unsigned char mask_en;
	unsigned char roi_en;
	unsigned char rrc_en;
	unsigned char mbqp_en;
	unsigned char rnd_en;
	unsigned char tmnr_en;
	unsigned char osg_en;
	unsigned char skipfrm_en;
	unsigned char maq_en;
	unsigned char dithering_en;
	unsigned char tnr_en;
	unsigned char lambda_en;
	unsigned char spn_en;
} pic_t;

typedef enum _FPGA_REPORT_{
	FPGA_REC_CHKSUM,
	FPGA_EC_REC_CHKSUM,
	FPGA_SRC_Y_CHKSUM,
	FPGA_SRC_C_CHKSUM,
	FPGA_TNR_OUT_Y_CHKSUM,
	FPGA_TNR_OUT_C_CHKSUM,
	FPGA_BS_LEN,
	FPGA_BS_CHKSUM,
	FPGA_RRC_RDOPT_COST_LSB,
	FPGA_RRC_RDOPT_COST_MSB,
	FPGA_RRC_SIZE,
	FPGA_RRC_FRM_COST_LSB,
	FPGA_RRC_FRM_COST_MSB,
	FPGA_RRC_FRM_COMPLEXITY_LSB,
	FPGA_RRC_FRM_COMPLEXITY_MSB,
	FPGA_RRC_COEFF,
	FPGA_RRC_QP_SUM,
	FPGA_SRAQ_ISUM_ACT_LOG,
	FPGA_PSNR_FRM_Y_LSB,
	FPGA_PSNR_FRM_Y_MSB,
	FPGA_PSNR_FRM_U_LSB,
	FPGA_PSNR_FRM_U_MSB,
	FPGA_PSNR_FRM_V_LSB,
	FPGA_PSNR_FRM_V_MSB,
	FPGA_PSNR_ROI_Y_LSB,
	FPGA_PSNR_ROI_Y_MSB,
	FPGA_PSNR_ROI_U_LSB,
	FPGA_PSNR_ROI_U_MSB,
	FPGA_PSNR_ROI_V_LSB,
	FPGA_PSNR_ROI_V_MSB,
    FPGA_PSNR_MOT_Y_LSB,
    FPGA_PSNR_MOT_Y_MSB,
    FPGA_PSNR_MOT_U_LSB,
    FPGA_PSNR_MOT_U_MSB,
    FPGA_PSNR_MOT_V_LSB,
    FPGA_PSNR_MOT_V_MSB,
    FPGA_PSNR_BGR_Y_LSB,
    FPGA_PSNR_BGR_Y_MSB,
    FPGA_PSNR_BGR_U_LSB,
    FPGA_PSNR_BGR_U_MSB,
    FPGA_PSNR_BGR_V_LSB,
    FPGA_PSNR_BGR_V_MSB,
	FPGA_ROI_CNT,
    FPGA_MOT_CNT,
    FPGA_BGR_CNT,
	FPGA_CRC_HIT_Y_CNT,
	FPGA_CRC_HIT_C_CNT,
	FPGA_SCD_REPORT,
	FPGA_TMNR_MT_OUT,
	FPGA_TMNR_HIS,
	FPGA_TMNR_Y_CHKSUM,
	FPGA_TMNR_C_CHKSUM,
	FPGA_MASK_Y_CHKSUM,
	FPGA_MASK_C_CHKSUM,
	FPGA_OSG_Y_CHKSUM,
	FPGA_OSG_C_CHKSUM,
	FPGA_MOTION_CNT,
	FPGA_OSG_0_Y_CHKSUM,
	FPGA_OSG_0_C_CHKSUM,
	FPGA_JND_Y_CHKSUM,
	FPGA_JND_C_CHKSUM,
    FPGA_JND_GRAD,
    FPGA_JND_GRAD_CNT,
    FPGA_DITHER_Y_CHKSUM,
    FPGA_DITHER_C_CHKSUM,
    FPGA_TMNR_REF_Y_CHKSUM,
    FPGA_TMNR_REF_C_CHKSUM,
    FPGA_TMNR_REC_Y_CHKSUM,
    FPGA_TMNR_REC_C_CHKSUM,
	FPGA_REPORT_NUMBER
}FPGA_REPORT;

typedef struct _chk_t_{
	unsigned int result[FPGA_REPORT_NUMBER];
} chk_t;

typedef struct _h264_folder_t_{
	char name[64];

	unsigned int idx;
	unsigned int pat_num;
}h264_folder_t;

typedef struct _h264_perf_t_{
	unsigned int cycle_sum;
	unsigned int bslen_sum;
	unsigned int cycle_max_frm;
	unsigned int cycle_max;
	unsigned int cycle_max_bslen;
}h264_perf_t;

//#endif // (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE)

#endif	// _EMU_H264_COMMON_H_
