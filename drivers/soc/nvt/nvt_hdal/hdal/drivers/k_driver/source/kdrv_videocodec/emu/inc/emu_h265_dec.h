#ifndef _EMU_H265_DEC_H_
#define _EMU_H265_DEC_H_

#include "kwrap/type.h"

// for hw trigger //
#include "h26x.h"

// for sw driver //
#include "h26x_def.h"

// for emulation //
#include "emu_h26x_common.h"

//#if (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE)

#define H265_EMU_TEST_WP			0
#define WRITE_REC					0
#define H265_EMU_REC_LINEOFFSET		0   /* could be set enalbe while H26X_EMU_MAX_BUF == Enable */
#define DUMP_ERROR_REC				0


#define H265_MAX_BSCMD_NUM			0x121	// for random bs dma test
#define H265_CMB_BUF_MAX_SIZE		0x80000
#define H265_RND_CMB_BUF_MAX_SIZE	0x90000	// for random bs dma test
//#define H265_MAX_DEC_BS_LEN		0x1400000 // hk 0x30 0000
//#define H265_MAX_DEC_BS_LEN		0x1410000 // hk 0x30 0000
#define H265_MAX_DEC_BS_LEN           0x50000 //hk 0x600000

#define H265_LINEOFFSET_MAX		5120


#define H265D_ENDLESS_RUN	      (0)

#define H265_EMU_PASS             0
#define H265_EMU_FILEERR          1
#define H265_EMU_FAIL             2

typedef struct _H26XDecRegCfg{
	UINT32 uiH26XDec0;		/* 0x20 */
	UINT32 uiH26XFunc1;		/* 0xA4 */
	UINT32 uiH26XTile0;		/* 0xB0 */
	UINT32 uiH26XPic0;		/* 0xB4 */
	UINT32 uiH26XPic1;		/* 0xB8, QP_0 */
	UINT32 uiH26XPic3;		/* 0xC0, ILF_0 */
	UINT32 uiH26XPic5;		/* 0xC8, AEAD_0 */
	UINT32 uiH26X_DIST_FACTOR;	/* 0xCC, PIC_6 DSF_0 */
	UINT32 uiH26XDec1;		/* 0x1BC */
	UINT32 uiH26XRecChkSum;	/* 0x254, CHK_1 */
	UINT32 uiH26XBsLen;		/* 0x268, CHK_6 */
	UINT32 uiH26XBsChkSum;		/* 0x26C, CHK_7 */
	UINT32 uiH26XTile1;		/* 0x354 */
	UINT32 uiH26XTile2;		/* 0x358 */
}__attribute__((packed))H26XDecRegCfg;

typedef struct _H26XDecInfoCfg{
	UINT8 uiYuvName[256];
	UINT32 uiWidth;
	UINT32 uiHeight;
	UINT32 uiFrameNum;
	UINT32 uiCommandLen;
}H26XDecInfoCfg;

typedef struct _H26XDecSeqCfg{
	H26XDecInfoCfg uiH26XInfoCfg;
}H26XDecSeqCfg;

typedef struct _H26XDecPicCfg {
	H26XDecRegCfg uiH26XRegCfg;
	UINT32 uiH26XSrcLineOffset;
	UINT32 uiH26XRecLineOffset;
} H26XDecPicCfg;

#define FRM_DEC_IDX_MAX 17
typedef struct _H26XDecAddr{
	UINT32 uiRefAndRecYAddr[FRM_DEC_IDX_MAX], uiRefAndRecUVAddr[FRM_DEC_IDX_MAX];
	UINT32 uiRef0Idx;
	UINT32 uiRecIdx;
	UINT32 uiIlfRIdx;
	UINT32 uiIlfWIdx;
	UINT32 uiCMDBufAddr;
	UINT32 uiHwBsAddr;

	UINT32 uiColMvsAddr[FRM_DEC_IDX_MAX];
	UINT32 uiColMvRIdx;
	UINT32 uiColMvWIdx;
	UINT32 uiSwRecYAddr;
	UINT32 uiSwRecUVAddr;

	UINT32 uiHwBSCmdNum;
	UINT32 uiHwBsCmdSize[H265_MAX_BSCMD_NUM];

	UINT32 uiTmpRecYAddr;
	UINT32 uiTmpRecUVAddr;
	UINT32 uiTmpColMvsAddr;
	UINT32 uiTmpBsAddr;

	UINT32 uiRecordRecIdx[FRM_DEC_IDX_MAX];
	UINT32 uiKeepRecFlag[FRM_DEC_IDX_MAX];
	UINT32 uiKeepRefIdx;
	UINT32 uiRundReleaseBufCnt;
	UINT32 uiDummyWTAddr;
}H26XDecAddr;

typedef struct _h265_dec_emu_file_t_{
	H26XFile reg_cfg;
	H26XFile info_cfg;
	H26XFile bs;
#ifdef WRITE_REC
	H26XFile hw_rec;
#endif
}h265_dec_emu_file_t;

typedef struct _h265_dec_perf_t_{
	UINT64 cycle_sum;
	UINT64 bslen_sum;
	UINT64 cycle_max;
	UINT64 cycle_max_bslen;
    float cycle_max_avg;
	float cycle_max_bitlen_avg;
    UINT64 cycle_max_frm;
}h265_dec_perf_t;

typedef struct _h265_dec_emu_t_{
	UINT32 uiWidth;
	UINT32 uiHeight;
	UINT32 uiSrcFrmSize;
	UINT32 uiFrameSize;
	UINT32 uiRecLineOffset;
	UINT32 uiIlfLineOffset;
	UINT32 uiTileNum;
    UINT32 tile_width[H26X_MAX_TILE_NUM];
	UINT32 ColMvs_Size;
	UINT32 Y_Size;
	UINT32 UV_Size;
	UINT32 uiBsSize;
	h265_dec_emu_file_t file;
	UINT32 uiVirHwSrcYAddr, uiVirHwSrcUVAddr;
	UINT32     uiAPBAddr;
	H26XRegSet *pH26XRegSet;
	H26XDecSeqCfg sH26XDecSeqCfg;
	UINT8 g_main_argv[0x3000];
	H26XDecPicCfg sH26XDecPicCfg;
	H26XDecAddr   sH26XVirDecAddr;
	UINT8  ucFileDir[256];
	UINT32 uiHwBsMaxLen;
	UINT32 uiPicType;
	UINT32  uiGopSizeEqOneFlag;	/* determine GopSize = 1 */
	UINT32  uiTotalAllocateBuffer;

	UINT32 uiRndSwRst;
	UINT32 uiRndBsBuf;
    UINT32 uiEncFunc1;
    UINT32 wp[5];
	h265_dec_perf_t perf;
}h265_dec_emu_t;

typedef struct _h265_dec_pat_t_{
	UINT32 uiSrcNum;
	UINT32 uiPatNum;
	UINT32 uiPicNum;
}h265_dec_pat_t;

typedef struct _h265_dec_ctx_t_{
	h265_dec_emu_t    emu;
	h265_dec_pat_t    pat;
	UINT32        uiDecId;
	CODEC_TYPE    eCodecType;
}h265_dec_ctx_t;

BOOL emu_h265d_setup(h26x_ctrl_t *p_ctrl);
BOOL emu_h265d_close_one_sequence(h26x_job_t *p_job, h265_dec_ctx_t *ctx, h26x_mem_t *p_mem);
BOOL emu_h265d_prepare_one_sequence(h26x_job_t *p_job, h265_dec_ctx_t *ctx, h26x_mem_t *p_mem);

BOOL emu_h265d_prepare_one_pic(h26x_job_t *p_job, h26x_ver_item_t *p_ver_item);
BOOL emu_h265d_chk_one_pic(h26x_job_t *p_job, UINT32 interrupt);
BOOL emu_h265d_init(h26x_ver_item_t *p_ver_item, h26x_job_t *p_job, UINT8 pucDir[265]);
void emu_h265d_set_nxt_bsbuf(h26x_job_t *p_job);

//#endif /* (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE) */

#endif	/* _EMU_H265_DEC_H_ */
