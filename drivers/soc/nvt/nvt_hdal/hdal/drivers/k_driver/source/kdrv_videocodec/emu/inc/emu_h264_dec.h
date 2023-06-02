#ifndef _EMU_H264_DEC_H_
#define _EMU_H264_DEC_H_

#include "kwrap/type.h"

// for hw trigger //
#include "h26x.h"

// for sw driver //
#include "h26x_def.h"
#include "h264dec_api.h"

// for emulation //
#include "emu_h26x_common.h"
#include "emu_h264_common.h"

#define cmp_smart_rec 0 //520 x


//#if (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE)

#define dec_frm_buf_num (2)
typedef struct _h264d_pat_t_{
	char name[64];
	unsigned int idx;
	unsigned int pic_num;

	file_t file;

	unsigned int seq_obj_size;
	unsigned int pic_obj_size;
	unsigned int chk_obj_size;

	info_t info;
	seq_t  seq;
	pic_t  pic;
	chk_t  chk;

	unsigned int rand_seed;

	h264_perf_t perf;

	unsigned int rec_y_addr[dec_frm_buf_num];
	unsigned int rec_uv_addr[dec_frm_buf_num];

	unsigned int bsdma_buf_addr; // for test dec random bsdma
	unsigned int bsdma_buf_size;// for test dec random bsdma
	unsigned int uiDecResPicBsLen;// for test dec random bsdma
	unsigned int uiHwBsAddr;// for test dec random bsdma

	unsigned int islice_recsum; //for frame skip mode
}h264d_pat_t;

typedef struct _h264d_emu_t_{
	H26XDEC_VAR  var_obj;
	H264DEC_INIT init_obj;
	H264DEC_INFO info_obj;
}h264d_emu_t;

typedef struct _h264d_ctx_t_{
	h264_folder_t folder;
	h264d_pat_t    pat;
	h264d_emu_t   emu;
}h264d_ctx_t;

BOOL emu_h264d_setup(h26x_ctrl_t *p_ctrl);
BOOL  emu_h264d_prepare_one_pic(h26x_job_t *p_job, h26x_ver_item_t *p_ver_item);
BOOL emu_h264d_chk_one_pic(h26x_job_t *p_job, UINT32 interrupt, unsigned int rec_out_en, unsigned int rnd_bs_buf);
void emu_h264d_set_nxt_bsbuf(h26x_job_t *p_job);


//#endif // (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE)

#endif	// _EMU_H264_DEC_H_
