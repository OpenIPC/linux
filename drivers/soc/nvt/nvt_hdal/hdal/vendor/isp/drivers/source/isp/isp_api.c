#if defined(__FREERTOS)
#include "string.h"
#include <stdio.h>
#include <kwrap/nvt_type.h>
#else
#include "kwrap/type.h"
#include "isp_builtin.h"
#endif
#include "kwrap/cpu.h"
#include <plat/top.h>

#include "isp_api.h"
#include "kdrv_videoenc/kdrv_videoenc.h"
#include "kflow_videocapture/ctl_sen.h"
#include "kflow_videocapture/ctl_sie_isp.h"
#include "kflow_videocapture/ctl_sie_utility.h"
#include "kflow_videoprocess/ctl_ipp_isp.h"
#include "kflow_videoenc/isf_vdoenc_int.h"
#include "isp_uti.h"
#include "isp_dbg.h"
#if defined(__FREERTOS)
#include "isp_dev_int.h"
#else
#include <kdrv_builtin/kdrv_builtin.h>
#include "isp_main.h"
#endif

//=============================================================================
// define
//=============================================================================
#define ISP_IF_REG_NAME             "NVT_ISP_IF"
#define ISP_IF_SIE_REG_EVENT        ISP_EVENT_SIE_VD | ISP_EVENT_PARAM_RST
#define ISP_IF_IPP_REG_EVENT        ISP_EVENT_IPP_CFGEND | ISP_EVENT_IPP_CFGEND_IMM | ISP_EVENT_PARAM_RST
#define ISP_IF_ENC_REG_EVENT        ISP_EVENT_ENC_TNR | ISP_EVENT_ENC_RATIO | ISP_EVENT_ENC_SHARPEN | ISP_EVENT_PARAM_RST
#define ISP_START_3A_FRAME_CNT_NUM  3

//=============================================================================
// global
//=============================================================================
static BOOL isp_id_valid[ISP_ID_MAX_NUM] = {0};
UINT32 isp_sync_id_sie[ISP_ID_MAX_NUM];
UINT32 isp_sync_id_ipp[ISP_ID_MAX_NUM];
UINT32 isp_sync_id_enc[ISP_ID_MAX_NUM];
static BOOL yout_lock[ISP_ID_MAX_NUM] = {FALSE};
static BOOL raw_lock[ISP_ID_MAX_NUM] = {FALSE};
static UINT32 isp_src_id_mask[CTL_SIE_ID_MAX_NUM] = {0};
CTL_IPP_ISP_YUV_OUT ctl_ipp_isp_yuv_out;
CTL_SIE_ISP_HEADER_INFO ctl_isp_sie_header_info;
CTL_IPP_ISP_3DNR_STA ctl_ipp_isp_3dnr_sta;
static ISP_ENC_ISP_RATIO isp_enc_ratio[ISP_ID_MAX_NUM];
static ISP_SENSOR_INIT_INFO sensor_init_info;
static BOOL emu_enable = FALSE;
static BOOL is_fastboot[ISP_ID_MAX_NUM];
static BOOL sen_id_mapping_enable;
static UINT32 sen_id_mapping_table[ISP_ID_MAX_NUM];
static UINT32 sie_id_mapping_table[ISP_ID_MAX_NUM];
static UINT32 ipp_id_mapping_table[ISP_ID_MAX_NUM];

//=============================================================================
// function declaration
//=============================================================================
static INT32 isp_api_cb_sie(ISP_ID id, ISP_EVENT evt, UINT32 frame_cnt, void *param);
static INT32 isp_api_cb_ipp(ISP_ID id, ISP_EVENT evt, UINT32 frame_cnt, void *param);
static INT32 isp_api_cb_enc(ISP_ID id, ISP_EVENT evt, UINT32 frame_cnt, void *param);
BOOL isp_get_id_valid(UINT32 id);
static ER isp_api_get_sensor_data_fmt(ISP_ID id, UINT32 *data_fmt);
static ER isp_api_get_sensor_mode_type(ISP_ID id, UINT32 *mode_type);
static void isp_api_get_fastboot_sync_info(UINT32 id);

//=============================================================================
// extern functions
//=============================================================================
void isp_api_reg_if(UINT32 id_list)
{
	ISP_DEV_INFO *pdev_info = isp_get_dev_info();
	UINT32 i;
	INT32 reg_sie_rt = 0;
	ER reg_ipp_rt = E_OK;

	reg_sie_rt = ctl_sie_isp_evt_fp_reg(ISP_IF_REG_NAME, &isp_api_cb_sie, ISP_IF_SIE_REG_EVENT, CTL_SIE_ISP_CB_MSG_NONE);
	if (reg_sie_rt != 0) {
		DBG_ERR("Reg sie ERROR! \r\n");
	}

	reg_ipp_rt = ctl_ipp_isp_evt_fp_reg(ISP_IF_REG_NAME, &isp_api_cb_ipp, ISP_IF_IPP_REG_EVENT, CTL_IPP_ISP_CB_MSG_NONE);
	if (reg_ipp_rt != E_OK) {
		DBG_ERR("Reg ipp ERROR! \r\n");
	}

	// Check size
	if ((sizeof(ISP_CA_RSLT) != sizeof(CTL_SIE_ISP_CA_RSLT)) ||
		(sizeof(ISP_LA_RSLT) != sizeof(CTL_SIE_ISP_LA_RSLT)) ||
		(sizeof(ISP_VA_RSLT) != sizeof(CTL_IPP_ISP_VA_RST)) ||
		(sizeof(ISP_VA_INDEP_RSLT) != sizeof(CTL_IPP_ISP_VA_INDEP_RST)) ||
		(sizeof(ISP_HISTO_RSLT) != sizeof(CTL_IPP_ISP_DCE_HIST_RST)) ||
		(sizeof(ISP_EDGE_STCS) != sizeof(CTL_IPP_ISP_EDGE_STCS)) ||
		(sizeof(ISP_DEFOG_STCS) != sizeof(CTL_IPP_ISP_DEFOG_STCS))) {
		DBG_ERR("Size of statistics structure ERROR! \r\n");
	}

	for (i = 0; i < ISP_ID_MAX_NUM; i++) {
		if ((id_list >> i) & 0x1) {
			isp_id_valid[i] = TRUE;
		} else {
			continue;
		}

		if (pdev_info != NULL) {
			pdev_info->ca_rslt[i] = isp_uti_mem_alloc(sizeof(ISP_CA_RSLT));
			pdev_info->la_rslt[i] = isp_uti_mem_alloc(sizeof(ISP_LA_RSLT));
			pdev_info->va_rslt[i] = isp_uti_mem_alloc(sizeof(ISP_VA_RSLT));
			pdev_info->va_indep_rslt[i] = isp_uti_mem_alloc(sizeof(ISP_VA_INDEP_RSLT));
			pdev_info->md_rslt[i] = isp_uti_mem_alloc(sizeof(ISP_MD_RSLT));
			pdev_info->histo_rslt[i] = isp_uti_mem_alloc(sizeof(ISP_HISTO_RSLT));
			pdev_info->ipe_subout_buf[i] = isp_uti_mem_alloc(sizeof(ISP_IPE_SUBOUT_BUF));
		}

		is_fastboot[i] = isp_api_get_fastboot_valid();
		if (is_fastboot[i]) {
			isp_api_get_fastboot_sync_info(i);
		}
	}
}

void isp_api_unreg_if(void)
{
	ISP_DEV_INFO *pdev_info = isp_get_dev_info();
	UINT32 i;
	INT32 reg_sie_rt = 0;
	ER reg_ipp_rt = E_OK;

	reg_sie_rt = ctl_sie_isp_evt_fp_unreg(ISP_IF_REG_NAME);
	if (reg_sie_rt != 0) {
		DBG_ERR("Unreg sie ERROR! \r\n");
	}

	reg_ipp_rt = ctl_ipp_isp_evt_fp_unreg(ISP_IF_REG_NAME);
	if (reg_ipp_rt != E_OK) {
		DBG_ERR("Unreg ipp ERROR! \r\n");
	}

	for (i = 0; i < ISP_ID_MAX_NUM; i++) {
		if (isp_id_valid[i] && (pdev_info != NULL)) {
			isp_uti_mem_free(pdev_info->ca_rslt[i]);
			isp_uti_mem_free(pdev_info->la_rslt[i]);
			isp_uti_mem_free(pdev_info->va_rslt[i]);
			isp_uti_mem_free(pdev_info->va_indep_rslt[i]);
			isp_uti_mem_free(pdev_info->histo_rslt[i]);
			isp_uti_mem_free(pdev_info->ipe_subout_buf[i]);
			pdev_info->ca_rslt[i] = NULL;
			pdev_info->la_rslt[i] = NULL;
			pdev_info->va_rslt[i] = NULL;
			pdev_info->va_indep_rslt[i] = NULL;
			pdev_info->histo_rslt[i] = NULL;
			pdev_info->ipe_subout_buf[i] = NULL;
		}
	}
}

void isp_api_reg_enc_if(void)
{
	UINT32 i;
	ER reg_enc_rt = E_OK;

	reg_enc_rt = kflow_videoenc_evt_fp_reg(ISP_IF_REG_NAME, &isp_api_cb_enc, ISP_IF_ENC_REG_EVENT, KFLOW_VIDEOENC_WAIT_FLG_NONE);
	if (reg_enc_rt != E_OK) {
		DBG_ERR("Reg enc ERROR! \r\n");
	}

	// Initial global value
	for (i = 0; i < ISP_ID_MAX_NUM; i++) {
		isp_enc_ratio[i].enc_edge_ratio = 256;
		isp_enc_ratio[i].enc_2dnr_ratio = 256;
		isp_enc_ratio[i].enc_3dnr_ratio = 256;
	}
}

void isp_api_unreg_enc_if(void)
{
	ER reg_enc_rt = E_OK;

	reg_enc_rt = kflow_videoenc_evt_fp_unreg(ISP_IF_REG_NAME);
	if (reg_enc_rt != E_OK) {
		DBG_ERR("Unreg enc ERROR! \r\n");
	}
}

static ER isp_api_check_id_mask(UINT32 mask, UINT32 frame_num)
{
	ER rt = E_OK;
	UINT32 i, cnt = 0;

	for (i = 0; i < ISP_ID_MAX_NUM; i++) {
		if (mask & (0x1 << i)) {
			cnt++;
		}
	}
	if (cnt != frame_num) {
		rt = E_SYS;
	}

	return rt;
}

static INT32 isp_api_cb_sie(ISP_ID id, ISP_EVENT evt, UINT32 frame_cnt, void *param)
{
	BOOL mapping_enable = (id >> 31) & 0x1; // 1 bit
	UINT32 sen_id = (id >> 24) & 0x7F;      // 7 bit
	UINT32 sie_id = id & 0xFFFFFF;          // 24 bit
	ISP_DEV_INFO *pdev_info = isp_get_dev_info();
	ISP_AE_TRIG_OBJ ae_trig_obj;
	ISP_AWB_TRIG_OBJ awb_trig_obj;
	ISP_IQ_TRIG_OBJ iq_trig_obj;
	ISP_FUNC_EN fun_en;
	CTL_SIE_ID src_id;
	UINT32 frame_num = 1, i;
	static UINT32 ae_src_id_cnt[CTL_SIE_ID_MAX_NUM] = {0};
	static UINT32 ae_src_id_mask[CTL_SIE_ID_MAX_NUM] = {0};
	static UINT32 awb_src_id_cnt[CTL_SIE_ID_MAX_NUM] = {0};
	static UINT32 awb_src_id_mask[CTL_SIE_ID_MAX_NUM] = {0};
	static UINT32 iq_src_id_mask[CTL_SIE_ID_MAX_NUM] = {0};
	UINT32 start_3a_frame_cnt_num = ISP_START_3A_FRAME_CNT_NUM;
	static UINT32 start_3a_frame_cnt[CTL_SIE_ID_MAX_NUM] = {0};
	UINT32 data_fmt = ISP_SEN_DATA_FMT_RGB, mode_type = ISP_SEN_MODE_LINEAR;
	UINT32 dbg_mode = isp_dbg_get_dbg_mode(sie_id);
	CTL_SIE_ISP_MD_RSLT ctl_sie_md_relt = {0};

	if (!isp_id_valid[sie_id]) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sie_id) & ISP_DBG_WRN_MSG, "id list not valid (%d) \r\n", sie_id);
		return 0;
	}

	if (pdev_info == NULL) {
		return 0;
	}
	if (sie_id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(dbg_mode & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", id);
		return 0;
	}

	sen_id_mapping_enable = mapping_enable;
	sen_id_mapping_table[sie_id] = sen_id;
	sie_id_mapping_table[sie_id] = id;

	ctl_sie_isp_get(id, CTL_SIE_ISP_ITEM_FUNC_EN, &fun_en);
	ctl_sie_isp_get(id, CTL_SIE_ISP_ITEM_DUPL_SRC_ID, &src_id);

	if (fun_en & ISP_FUNC_EN_SHDR) {
		isp_api_get_frame_num(src_id, &frame_num);
		if (frame_num == 1) {
			PRINT_ISP_WRN(dbg_mode & ISP_DBG_WRN_MSG, "SHDR frame number must > 1 (%d) \r\n", frame_num);
		}

		start_3a_frame_cnt_num = frame_num*ISP_START_3A_FRAME_CNT_NUM-1;
	}

	isp_sync_id_sie[sie_id] = frame_cnt;

	switch (evt) {
	case ISP_EVENT_PARAM_RST:
	case ISP_EVENT_SIE_VD:
		#if defined(__KERNEL__)
		wake_up_interruptible_all(&pdev_info->wait_vd[sie_id]);
		#endif

		isp_dev_reset_item_valid(sie_id, frame_cnt);

		if (fun_en & ISP_FUNC_EN_AE) {
			ctl_sie_isp_set(id, CTL_SIE_ISP_ITEM_LA_BUF, pdev_info->la_rslt[sie_id]);
			ctl_sie_md_relt.md_th_rslt = pdev_info->md_rslt[sie_id]->th;
			ctl_sie_isp_get(id, CTL_SIE_ISP_ITEM_MD_RSLT, &ctl_sie_md_relt);
			pdev_info->md_rslt[sie_id]->blk_dif_cnt = ctl_sie_md_relt.blk_dif_cnt;
			pdev_info->md_rslt[sie_id]->total_blk_diff = ctl_sie_md_relt.total_blk_diff;
		}

		if (fun_en & ISP_FUNC_EN_AWB) {
			ctl_sie_isp_set(id, CTL_SIE_ISP_ITEM_CA_BUF, pdev_info->ca_rslt[sie_id]);
		}

		if (start_3a_frame_cnt[sie_id] < start_3a_frame_cnt_num) {
			start_3a_frame_cnt[sie_id]++;
		}

		if (evt == ISP_EVENT_PARAM_RST) {
			if (is_fastboot[sie_id]) {
				isp_api_get_fastboot_sync_info(sie_id);
				is_fastboot[sie_id] = FALSE;
			}

			isp_src_id_mask[src_id] = 0;
			start_3a_frame_cnt[sie_id] = 0;
			for (i = 0; i < SYNC_INFO_BUF; i++) {
				isp_dev_reset_item_valid(sie_id, i);
			}
			ae_trig_obj.reset = TRUE;
			awb_trig_obj.reset = TRUE;
			iq_trig_obj.msg = ISP_TRIG_IQ_SIE_IMM;
			iq_trig_obj.sie.reset = TRUE;
			PRINT_ISP(dbg_mode & ISP_DBG_SIE_CB, "sie rst,   id = %8X, frm = %d, 3A start %d/%d \r\n", id, frame_cnt, start_3a_frame_cnt[sie_id], start_3a_frame_cnt_num);
		} else {
			ae_trig_obj.reset = FALSE;
			awb_trig_obj.reset = FALSE;
			iq_trig_obj.msg = ISP_TRIG_IQ_SIE;
			iq_trig_obj.sie.reset = FALSE;
			PRINT_ISP(dbg_mode & ISP_DBG_SIE_CB, "sie cb,    id = %8X, frm = %d, 3A start %d/%d \r\n", id, frame_cnt, start_3a_frame_cnt[sie_id], start_3a_frame_cnt_num);
		}

		isp_src_id_mask[src_id] |= 1 << sie_id;
		isp_api_get_sensor_data_fmt(src_id, &data_fmt);
		isp_api_get_sensor_mode_type(src_id, &mode_type);

		if ((pdev_info->isp_ae != NULL) && (fun_en & ISP_FUNC_EN_AE) && ((start_3a_frame_cnt[sie_id] >= start_3a_frame_cnt_num) || (evt == ISP_EVENT_PARAM_RST))) {
			if (fun_en & ISP_FUNC_EN_SHDR) {
				ae_src_id_cnt[src_id]++;
				ae_src_id_mask[src_id] |= 1 << sie_id;
				if (ae_src_id_cnt[src_id] == frame_num) {
					PRINT_ISP(dbg_mode & ISP_DBG_SIE_CB, "sie vd,    id = %8X, frm = %d, src id = %d, cnt = %d, mask = %d \r\n", id, frame_cnt, src_id, ae_src_id_cnt[src_id], ae_src_id_mask[src_id]);
					ae_trig_obj.func_en = fun_en;
					ae_trig_obj.src_id_mask = ae_src_id_mask[src_id];
					if (isp_api_check_id_mask(ae_trig_obj.src_id_mask, frame_num) == E_OK) {
						pdev_info->isp_ae->fn_trig(src_id, &ae_trig_obj);
					}
					ae_src_id_cnt[src_id] = 0;
					ae_src_id_mask[src_id] = 0;
				}
			} else {
				PRINT_ISP(dbg_mode & ISP_DBG_SIE_CB, "sie vd,    id = %8X, frm = %d \r\n", id, frame_cnt);
				ae_trig_obj.func_en = fun_en;
				ae_trig_obj.src_id_mask = 1 << sie_id;
				pdev_info->isp_ae->fn_trig(sie_id, &ae_trig_obj);
			}
		}
		if ((pdev_info->isp_awb != NULL) && (fun_en & ISP_FUNC_EN_AWB) && ((start_3a_frame_cnt[sie_id] >= start_3a_frame_cnt_num) || (evt == ISP_EVENT_PARAM_RST))) {
			awb_trig_obj.data_fmt = data_fmt;
			if (fun_en & ISP_FUNC_EN_SHDR) {
				awb_src_id_cnt[src_id]++;
				awb_src_id_mask[src_id] |= 1 << sie_id;
				if (awb_src_id_cnt[src_id] == frame_num) {
					awb_trig_obj.func_en = fun_en;
					awb_trig_obj.src_id_mask = awb_src_id_mask[src_id];
					if (isp_api_check_id_mask(awb_trig_obj.src_id_mask, frame_num) == E_OK) {
						pdev_info->isp_awb->fn_trig(src_id, &awb_trig_obj);
					}
					awb_src_id_cnt[src_id] = 0;
					awb_src_id_mask[src_id] = 0;
				}
			} else {
				awb_trig_obj.func_en = fun_en;
				awb_trig_obj.src_id_mask =  1 << sie_id;
				pdev_info->isp_awb->fn_trig(sie_id, &awb_trig_obj);
			}
		}

		if (pdev_info->isp_iq != NULL) {
			iq_trig_obj.sie.data_fmt = data_fmt;
			iq_trig_obj.sie.mode_type = mode_type;
			iq_trig_obj.sie.func_en = fun_en;
			iq_trig_obj.sie.dupl_src_id = src_id;
			if (fun_en & ISP_FUNC_EN_SHDR) {
				iq_src_id_mask[src_id] |= 1 << sie_id;
				iq_trig_obj.sie.src_id_mask = iq_src_id_mask[src_id];
			} else {
				iq_trig_obj.sie.src_id_mask = 1 << sie_id;
			}
			pdev_info->isp_iq->fn_trig(sie_id, &iq_trig_obj);
		}
		break;

	default:
		PRINT_ISP_WRN(dbg_mode & ISP_DBG_WRN_MSG, "ISP_EVENT out of range (%d) \r\n", evt);
		break;
	}
	return 0;
}

static INT32 isp_api_cb_ipp(ISP_ID id, ISP_EVENT evt, UINT32 frame_cnt, void *param)
{
	BOOL mapping_enable = (id >> 31) & 0x1; // 1 bit
	UINT32 sen_id = (id >> 24) & 0x7F;      // 7 bit
	UINT32 ipp_info = (id >> 16) & 0xFF;    // 8 bit
	UINT32 ipp_id = id & 0xFFFF;            // 16 bit
	ISP_DEV_INFO *pdev_info = isp_get_dev_info();
	ISP_AF_TRIG_OBJ af_trig_obj;
	ISP_IQ_TRIG_OBJ iq_trig_obj;
	ISP_FUNC_EN fun_en = 0;
	CTL_IPP_ISP_STATUS_INFO ipp_isp_status_info = {0};
	CTL_IPP_ISP_DEFOG_SUBOUT ipp_isp_defog_subout = {0};
	CTL_IPP_ISP_STRP_INFO ipp_isp_stripe_info = {0};
	UINT32 data_fmt = ISP_SEN_DATA_FMT_RGB, mode_type = ISP_SEN_MODE_LINEAR;
	UINT32 start_3a_frame_cnt_num = ISP_START_3A_FRAME_CNT_NUM;
	static UINT32 start_3a_frame_cnt[CTL_SIE_ID_MAX_NUM] = {0};
	UINT32 dbg_mode = isp_dbg_get_dbg_mode(ipp_id);
	ER rt = E_OK;
	UINT32 i;
	INT32 j;

	if (!isp_id_valid[ipp_id]) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(ipp_id) & ISP_DBG_WRN_MSG, "id list not valid (%d) \r\n", ipp_id);
		return 0;
	}

	if (pdev_info == NULL) {
		return 0;
	}
	if (ipp_id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(dbg_mode & ISP_DBG_WRN_MSG, "isp id out of range (0x%x) \r\n", id);
		return 0;
	}

	sen_id_mapping_enable = mapping_enable;
	sen_id_mapping_table[ipp_id] = sen_id;
	ipp_id_mapping_table[ipp_id] = id;

	ctl_ipp_isp_get(id, CTL_IPP_ISP_ITEM_STATUS_INFO, &ipp_isp_status_info);
	if (((ipp_isp_status_info.flow == CTL_IPP_ISP_FLOW_CAPTURE_RAW) || (ipp_isp_status_info.flow == CTL_IPP_ISP_FLOW_CAPTURE_CCIR)) && (evt != ISP_EVENT_IPP_CFGEND_IMM)) {
		// capture mode with ISP_EVENT_PARAM_RST/ISP_EVENT_IPP_CFGEND
		return 0;
	}
	if (((ipp_isp_status_info.flow != CTL_IPP_ISP_FLOW_CAPTURE_RAW) && (ipp_isp_status_info.flow != CTL_IPP_ISP_FLOW_CAPTURE_CCIR)) && (evt == ISP_EVENT_IPP_CFGEND_IMM)) {
		// preview mode with ISP_EVENT_IPP_CFGEND_IMM
		return 0;
	}
	rt |= ctl_ipp_isp_get(id, CTL_IPP_ISP_ITEM_FUNC_EN, &fun_en);
	rt |= ctl_ipp_isp_get(id, CTL_IPP_ISP_ITEM_EDGE_STCS, &pdev_info->edge_stcs[ipp_id]);
	rt |= ctl_ipp_isp_get(id, CTL_IPP_ISP_ITEM_STRIPE_INFO, &ipp_isp_stripe_info);
	if ((evt != ISP_EVENT_PARAM_RST) && (rt != E_OK)) {
		PRINT_ISP_WRN(dbg_mode & ISP_DBG_WRN_MSG, "ID(%d) evt(0x%x) frm(%d) trigger information get fail, do not trigger!! \r\n", id, evt, frame_cnt);
		return 0;
	}

	isp_sync_id_ipp[ipp_id] = frame_cnt;
	if ((fun_en & ISP_FUNC_EN_SHDR) && (isp_src_id_mask[ipp_id] != 0)) {
		for (j = ISP_ID_MAX_NUM - 1; j >= 0; j--) {
			if ((0x1 << j) & isp_src_id_mask[ipp_id]) {
				break;
			}
		}

		if (j != (INT32)ipp_id) {
			isp_sync_id_ipp[j] = frame_cnt;
		}
	}

	switch (evt) {
	case ISP_EVENT_PARAM_RST:
	case ISP_EVENT_IPP_CFGEND:
	case ISP_EVENT_IPP_CFGEND_IMM:
		if (evt == ISP_EVENT_PARAM_RST) {
			if (is_fastboot[ipp_id]) {
				isp_api_get_fastboot_sync_info(ipp_id);
				is_fastboot[ipp_id] = FALSE;
			}

			start_3a_frame_cnt[ipp_id] = 0;
			for (i = 0; i < SYNC_INFO_BUF; i++) {
				isp_dev_reset_item_valid(ipp_id, i);
			}

			if ((fun_en & ISP_FUNC_EN_SHDR) && (isp_src_id_mask[ipp_id] != 0)) {
				for (j = ISP_ID_MAX_NUM - 1; j >= 0; j--) {
					if ((0x1 << j) & isp_src_id_mask[ipp_id]) {
						break;
					}
				}
				if (j != (INT32)ipp_id) {
					for (i = 0; i < SYNC_INFO_BUF; i++) {
						isp_dev_reset_item_valid(j, i);
					}
				}
			}

			af_trig_obj.reset = TRUE;

			iq_trig_obj.msg = ISP_TRIG_IQ_IPP_IMM;
			iq_trig_obj.ipp.reset = TRUE;
			iq_trig_obj.ipp.ipp_capture = FALSE;
			PRINT_ISP(dbg_mode & ISP_DBG_IPP_CB, "ipp rst,   id = %8X, frm = %d, func = 0x%x, 3A start %d/%d \r\n", id, frame_cnt, fun_en, start_3a_frame_cnt[ipp_id], start_3a_frame_cnt_num);
		} else if (evt == ISP_EVENT_IPP_CFGEND) {

			if (start_3a_frame_cnt[ipp_id] < start_3a_frame_cnt_num) {
				start_3a_frame_cnt[ipp_id]++;
			}

			af_trig_obj.reset = FALSE;

			iq_trig_obj.msg = ISP_TRIG_IQ_IPP;
			iq_trig_obj.ipp.reset = FALSE;
			iq_trig_obj.ipp.ipp_capture = FALSE;
			if (ipp_info == 0) {
				PRINT_ISP(dbg_mode & ISP_DBG_IPP_CB, "ipp cb_M,  id = %8X, frm = %d, func = 0x%x, 3A start %d/%d \r\n", id, frame_cnt, fun_en, start_3a_frame_cnt[ipp_id], start_3a_frame_cnt_num);
			} else {
				PRINT_ISP(dbg_mode & ISP_DBG_IPP_CB, "ipp cb_S,  id = %8X, frm = %d, func = 0x%x, 3A start %d/%d \r\n", id, frame_cnt, fun_en, start_3a_frame_cnt[ipp_id], start_3a_frame_cnt_num);
			}
		} else if (evt == ISP_EVENT_IPP_CFGEND_IMM) {
			af_trig_obj.reset = FALSE;

			iq_trig_obj.msg = ISP_TRIG_IQ_IPP_IMM;
			iq_trig_obj.ipp.reset = FALSE;
			iq_trig_obj.ipp.ipp_capture = TRUE;
			PRINT_ISP(dbg_mode & ISP_DBG_IPP_CB, "ipp cb_C,  id = %8X, frm = %d, func = 0x%x, 3A start %d/%d \r\n", id, frame_cnt, fun_en, start_3a_frame_cnt[ipp_id], start_3a_frame_cnt_num);
		}

		if (fun_en & ISP_FUNC_EN_AF) {
			ctl_ipp_isp_get(id, CTL_IPP_ISP_ITEM_VA_RST, pdev_info->va_rslt[ipp_id]);
			ctl_ipp_isp_get(id, CTL_IPP_ISP_ITEM_VA_INDEP_RST, pdev_info->va_indep_rslt[ipp_id]);
		}

		if (fun_en & ISP_FUNC_EN_WDR) {
			ctl_ipp_isp_get(id, CTL_IPP_ISP_ITEM_DCE_HIST_RST, pdev_info->histo_rslt[ipp_id]);
		}

		if (fun_en & ISP_FUNC_EN_DEFOG) {
			ctl_ipp_isp_get(id, CTL_IPP_ISP_ITEM_DEFOG_SUBOUT, &ipp_isp_defog_subout);
			if (ipp_isp_defog_subout.addr != 0) {
				for (i = 0; i < ISP_SUBOUT_MAX_SIZE / 2; i++) {
					pdev_info->ipe_subout_buf[ipp_id]->avg[2 * i] = *((UINT32 *)ipp_isp_defog_subout.addr + i) >> 16 & 0x3FF;
					pdev_info->ipe_subout_buf[ipp_id]->avg[2 * i + 1] = *((UINT32 *)ipp_isp_defog_subout.addr + i) & 0x3FF;
				}
			}
		}

		if ((pdev_info->isp_af != NULL) && (fun_en & ISP_FUNC_EN_AF) && ((start_3a_frame_cnt[ipp_id] >= start_3a_frame_cnt_num) || (evt == ISP_EVENT_PARAM_RST))) {
			af_trig_obj.func_en = fun_en;
			pdev_info->isp_af->fn_trig(ipp_id, &af_trig_obj);
		}

		if (pdev_info->isp_iq != NULL) {
			if (ipp_isp_status_info.flow == CTL_IPP_ISP_FLOW_DIRECT_RAW) {
				iq_trig_obj.ipp.ipp_direct = TRUE;
			} else {
				iq_trig_obj.ipp.ipp_direct = FALSE;
			}
			iq_trig_obj.ipp.stripe_num = ipp_isp_stripe_info.num;
			isp_api_get_sensor_data_fmt(ipp_id, &data_fmt);
			isp_api_get_sensor_mode_type(ipp_id, &mode_type);
			iq_trig_obj.ipp.data_fmt = data_fmt;
			iq_trig_obj.ipp.mode_type = mode_type;
			if ((ipp_isp_status_info.flow == CTL_IPP_ISP_FLOW_CCIR) || (ipp_isp_status_info.flow == CTL_IPP_ISP_FLOW_CAPTURE_CCIR)) {
				iq_trig_obj.ipp.flow_type = ISP_FLOW_CCIR;
			} else {
				iq_trig_obj.ipp.flow_type = ISP_FLOW_RAW;
			}
			iq_trig_obj.ipp.func_en = fun_en;
			iq_trig_obj.ipp.ipp_info = ipp_info;
			pdev_info->isp_iq->fn_trig(ipp_id, &iq_trig_obj);
		}
		break;

	default:
		PRINT_ISP_WRN(dbg_mode & ISP_DBG_WRN_MSG, "ISP_EVENT out of range (%d) \r\n", evt);
		break;
	}
	return 0;
}

static INT32 isp_api_cb_enc(ISP_ID id, ISP_EVENT evt, UINT32 frame_cnt, void *param)
{
	ISP_DEV_INFO *pdev_info = isp_get_dev_info();
	ISP_IQ_TRIG_OBJ iq_trig_obj;
	KDRV_VDOENC_ISP_RATIO enc_isp_ratio = {0};
	UINT32 dbg_mode = isp_dbg_get_dbg_mode(id);
	ER rt = E_OK;

	if (pdev_info == NULL) {
		return 0;
	}
	if (id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", id);
		return 0;
	}

	isp_sync_id_enc[id] = frame_cnt;

	switch (evt) {
	case ISP_EVENT_ENC_TNR:
	case ISP_EVENT_ENC_SHARPEN:
		PRINT_ISP(dbg_mode & ISP_DBG_ENC_CB, "enc  cb,   id = %5X, frm = %d \r\n", id, frame_cnt);
		if (pdev_info->isp_iq != NULL) {
			iq_trig_obj.msg = ISP_TRIG_IQ_ENC;
			iq_trig_obj.enc.reset = FALSE;
			pdev_info->isp_iq->fn_trig(id, &iq_trig_obj);
		}
		break;

	case ISP_EVENT_ENC_RATIO:
		rt |= kflow_videoenc_get(id, KFLOW_VIDEOENC_ISP_ITEM_RATIO, &enc_isp_ratio);
		PRINT_ISP(dbg_mode & ISP_DBG_ENC_CB, "enc  cb,   id = %5X, isp_ratio(%d, (%d, %d, %d)) \r\n"
			, id, enc_isp_ratio.ratio_base, enc_isp_ratio.edge_ratio, enc_isp_ratio.dn_2d_ratio, enc_isp_ratio.dn_3d_ratio);
		if ((rt != E_OK) || (enc_isp_ratio.ratio_base == 0)) {
			isp_enc_ratio[id].enc_edge_ratio = 256;
			isp_enc_ratio[id].enc_2dnr_ratio = 256;
			isp_enc_ratio[id].enc_3dnr_ratio = 256;
		} else {
			isp_enc_ratio[id].enc_edge_ratio = (enc_isp_ratio.edge_ratio << 8) / enc_isp_ratio.ratio_base;
			isp_enc_ratio[id].enc_2dnr_ratio = (enc_isp_ratio.dn_2d_ratio << 8) / enc_isp_ratio.ratio_base;
			isp_enc_ratio[id].enc_3dnr_ratio = (enc_isp_ratio.dn_3d_ratio << 8) / enc_isp_ratio.ratio_base;
		}
		break;

	default:
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(id) & ISP_DBG_WRN_MSG, "ENC CB out of ISP_EVENT (%d) \r\n", evt);
		break;
	}
	return 0;
}

BOOL isp_get_id_valid(UINT32 id)
{
	return isp_id_valid[id];
}

ER isp_api_get_expt(ISP_ID id, ISP_SENSOR_CTRL *sensor_ctrl)
{
	PCTL_SEN_OBJ p_sen_ctl_obj[CTL_SEN_ID_MAX] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	UINT32 sen_id;
	ER rt;

	if (sen_id_mapping_enable) {
		sen_id = sen_id_mapping_table[id];
	} else {
		sen_id = id;
	}

	p_sen_ctl_obj[sen_id] = ctl_sen_get_object(sen_id);
	if (p_sen_ctl_obj[sen_id] == NULL) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object NULL, id: %d \r\n", id);
		return E_NOEXS;
	}
	if (p_sen_ctl_obj[sen_id]->get_cfg == NULL) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object->get_cfg NULL, id: %d \r\n", id);
		return E_NOEXS;
	}
	if (p_sen_ctl_obj[sen_id]->is_opened() == FALSE) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object->is_opened() FALSE, id: %d \r\n", id);
		return E_NOEXS;
	}

	rt = p_sen_ctl_obj[sen_id]->get_cfg(CTL_SEN_CFGID_GET_EXPT, sensor_ctrl);

	return rt;
}

ER isp_api_get_gain(ISP_ID id, ISP_SENSOR_CTRL *sensor_ctrl)
{
	PCTL_SEN_OBJ p_sen_ctl_obj[CTL_SEN_ID_MAX] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	UINT32 sen_id;
	ER rt;

	if (sen_id_mapping_enable) {
		sen_id = sen_id_mapping_table[id];
	} else {
		sen_id = id;
	}

	p_sen_ctl_obj[sen_id] = ctl_sen_get_object(sen_id);
	if (p_sen_ctl_obj[sen_id] == NULL) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object NULL, id: %d \r\n", id);
		return E_NOEXS;
	}
	if (p_sen_ctl_obj[sen_id]->get_cfg == NULL) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object->get_cfg NULL, id: %d \r\n", id);
		return E_NOEXS;
	}
	if (p_sen_ctl_obj[sen_id]->is_opened() == FALSE) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object->is_opened() FALSE, id: %d \r\n", id);
		return E_NOEXS;
	}

	rt = p_sen_ctl_obj[sen_id]->get_cfg(CTL_SEN_CFGID_GET_GAIN, sensor_ctrl);

	return rt;
}

ER isp_api_get_direction(ISP_ID id, ISP_SENSOR_DIRECTION *sensor_direction)
{
	CTL_SEN_FLIP sen_flip = CTL_SEN_FLIP_NONE;
	PCTL_SEN_OBJ p_sen_ctl_obj[CTL_SEN_ID_MAX] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	UINT32 sen_id;
	ER rt;

	if (sen_id_mapping_enable) {
		sen_id = sen_id_mapping_table[id];
	} else {
		sen_id = id;
	}

	p_sen_ctl_obj[sen_id] = ctl_sen_get_object(sen_id);
	if (p_sen_ctl_obj[sen_id] == NULL) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object NULL, id: %d \r\n", sen_id);
		return E_NOEXS;
	}
	if (p_sen_ctl_obj[sen_id]->get_cfg == NULL) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object->get_cfg NULL, id: %d \r\n", sen_id);
		return E_NOEXS;
	}
	if (p_sen_ctl_obj[sen_id]->is_opened() == FALSE) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object->is_opened() FALSE, id: %d \r\n", sen_id);
		return E_NOEXS;
	}

	rt = p_sen_ctl_obj[sen_id]->get_cfg(CTL_SEN_CFGID_FLIP_TYPE, &sen_flip);
	if (rt != 0) {
		return rt;
	}

	if (sen_flip & CTL_SEN_FLIP_H) {
		sensor_direction->mirror = TRUE;
	}
	if (sen_flip & CTL_SEN_FLIP_V) {
		sensor_direction->flip = TRUE;
	}

	return rt;
}

ER isp_api_get_sync_time(ISP_ID id, UINT32 *sync_time)
{
	PCTL_SEN_OBJ p_sen_ctl_obj[CTL_SEN_ID_MAX] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	CTL_SEN_GET_ATTR_PARAM attr_param;
	UINT32 sen_id;
	ER rt;

	if (sen_id_mapping_enable) {
		sen_id = sen_id_mapping_table[id];
	} else {
		sen_id = id;
	}

	p_sen_ctl_obj[sen_id] = ctl_sen_get_object(sen_id);
	if (p_sen_ctl_obj[sen_id] == NULL) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object NULL, id: %d \r\n", sen_id);
		return E_NOEXS;
	}
	if (p_sen_ctl_obj[sen_id]->get_cfg == NULL) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object->get_cfg NULL, id: %d \r\n", sen_id);
		return E_NOEXS;
	}
	if (p_sen_ctl_obj[sen_id]->is_opened() == FALSE) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object->is_opened() FALSE, id: %d \r\n", sen_id);
		return E_NOEXS;
	}

	rt = p_sen_ctl_obj[sen_id]->get_cfg(CTL_SEN_CFGID_GET_ATTR, &attr_param);
	if (rt != 0) {
		return rt;
	}

	*sync_time = attr_param.sync_timing;

	return rt;
}

ER isp_api_get_frame_num(ISP_ID id, UINT32 *frame_num)
{
	PCTL_SEN_OBJ p_sen_ctl_obj[CTL_SEN_ID_MAX] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	CTL_SEN_GET_MODE_BASIC_PARAM mode_param;
	UINT32 sen_id;
	ER rt;

	if (sen_id_mapping_enable) {
		sen_id = sen_id_mapping_table[id];
	} else {
		sen_id = id;
	}

	if ((UINT32)sen_id >= CTL_SEN_ID_MAX) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "sen id out of range (%d) \r\n", sen_id);
		return E_NOEXS;
	}

	p_sen_ctl_obj[sen_id] = ctl_sen_get_object(sen_id);
	if (p_sen_ctl_obj[sen_id] == NULL) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object NULL, id: %d \r\n", sen_id);
		return E_NOEXS;
	}
	if (p_sen_ctl_obj[sen_id]->get_cfg == NULL) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object->get_cfg NULL, id: %d \r\n", sen_id);
		return E_NOEXS;
	}
	if (p_sen_ctl_obj[sen_id]->is_opened() == FALSE) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object->is_opened() FALSE, id: %d \r\n", sen_id);
		return E_NOEXS;
	}

	mode_param.mode = CTL_SEN_MODE_CUR;
	rt = p_sen_ctl_obj[sen_id]->get_cfg(CTL_SEN_CFGID_GET_MODE_BASIC, &mode_param);
	if (rt != 0) {
		return rt;
	}

	*frame_num = mode_param.frame_num;

	return rt;
}

ER isp_api_get_chg_fps(ISP_ID id, UINT32 *chg_fps)
{
	PCTL_SEN_OBJ p_sen_ctl_obj[CTL_SEN_ID_MAX] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	CTL_SEN_GET_FPS_PARAM fps_param;
	UINT32 sen_id;
	ER rt;

	if (sen_id_mapping_enable) {
		sen_id = sen_id_mapping_table[id];
	} else {
		sen_id = id;
	}

	p_sen_ctl_obj[sen_id] = ctl_sen_get_object(sen_id);
	if (p_sen_ctl_obj[sen_id] == NULL) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object NULL, id: %d \r\n", sen_id);
		return E_NOEXS;
	}
	if (p_sen_ctl_obj[sen_id]->get_cfg == NULL) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object->get_cfg NULL, id: %d \r\n", sen_id);
		return E_NOEXS;
	}
	if (p_sen_ctl_obj[sen_id]->is_opened() == FALSE) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object->is_opened() FALSE, id: %d \r\n", sen_id);
		return E_NOEXS;
	}

	rt = p_sen_ctl_obj[sen_id]->get_cfg(CTL_SEN_CFGID_GET_FPS, &fps_param);
	if (rt != 0) {
		return rt;
	}

	*chg_fps = fps_param.chg_fps;

	return rt;
}

static ER isp_api_get_sensor_data_fmt(ISP_ID id, UINT32 *data_fmt)
{
	PCTL_SEN_OBJ p_sen_ctl_obj[CTL_SEN_ID_MAX] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	CTL_SEN_GET_MODE_BASIC_PARAM mode_basic_param;
	UINT32 sen_id;
	ER rt;

	if (sen_id_mapping_enable) {
		sen_id = sen_id_mapping_table[id];
	} else {
		sen_id = id;
	}

	p_sen_ctl_obj[sen_id] = ctl_sen_get_object(sen_id);
	if (p_sen_ctl_obj[sen_id] == NULL) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object NULL, id: %d \r\n", sen_id);
		return E_NOEXS;
	}
	if (p_sen_ctl_obj[sen_id]->get_cfg == NULL) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object->get_cfg NULL, id: %d \r\n", sen_id);
		return E_NOEXS;
	}
	if (p_sen_ctl_obj[sen_id]->is_opened() == FALSE) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object->is_opened() FALSE, id: %d \r\n", sen_id);
		return E_NOEXS;
	}

	mode_basic_param.mode = CTL_SEN_MODE_CUR;
	rt = p_sen_ctl_obj[sen_id]->get_cfg(CTL_SEN_CFGID_GET_MODE_BASIC, &mode_basic_param);
	if (rt != 0) {
		return rt;
	}

	if (mode_basic_param.data_fmt == CTL_SEN_DATA_FMT_RGBIR) {
		*data_fmt = ISP_SEN_DATA_FMT_RGBIR;
	} else if (mode_basic_param.data_fmt == CTL_SEN_DATA_FMT_RCCB) {
		*data_fmt = ISP_SEN_DATA_FMT_RCCB;
	}else if (mode_basic_param.data_fmt == CTL_SEN_DATA_FMT_YUV) {
		*data_fmt = ISP_SEN_DATA_FMT_YUV;
	} else if (mode_basic_param.data_fmt == CTL_SEN_DATA_FMT_Y_ONLY) {
		*data_fmt = ISP_SEN_DATA_FMT_Y_ONLY;
	} else {
		*data_fmt = ISP_SEN_DATA_FMT_RGB;
	}

	return rt;
}

static ER isp_api_get_sensor_mode_type(ISP_ID id, UINT32 *mode_type)
{
	PCTL_SEN_OBJ p_sen_ctl_obj[CTL_SEN_ID_MAX] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	CTL_SEN_GET_MODE_BASIC_PARAM mode_basic_param;
	UINT32 sen_id;
	ER rt;

	if (sen_id_mapping_enable) {
		sen_id = sen_id_mapping_table[id];
	} else {
		sen_id = id;
	}

	p_sen_ctl_obj[sen_id] = ctl_sen_get_object(sen_id);
	if (p_sen_ctl_obj[sen_id] == NULL) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object NULL, id: %d \r\n", sen_id);
		return E_NOEXS;
	}
	if (p_sen_ctl_obj[sen_id]->get_cfg == NULL) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object->get_cfg NULL, id: %d \r\n", sen_id);
		return E_NOEXS;
	}
	if (p_sen_ctl_obj[sen_id]->is_opened() == FALSE) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object->is_opened() FALSE, id: %d \r\n", sen_id);
		return E_NOEXS;
	}

	mode_basic_param.mode = CTL_SEN_MODE_CUR;
	rt = p_sen_ctl_obj[sen_id]->get_cfg(CTL_SEN_CFGID_GET_MODE_BASIC, &mode_basic_param);
	if (rt != 0) {
		return rt;
	}

	if (mode_basic_param.mode_type == CTL_SEN_MODE_BUILTIN_HDR) {
		*mode_type = ISP_SEN_MODE_BUILTIN_HDR;
	} else if (mode_basic_param.mode_type == CTL_SEN_MODE_CCIR) {
		*mode_type = ISP_SEN_MODE_CCIR;
	} else if (mode_basic_param.mode_type == CTL_SEN_MODE_CCIR_INTERLACE) {
		*mode_type = ISP_SEN_MODE_CCIR_INTERLACE;
	} else if (mode_basic_param.mode_type == CTL_SEN_MODE_STAGGER_HDR) {
		*mode_type = ISP_SEN_MODE_STAGGER_HDR;
	} else if (mode_basic_param.mode_type == CTL_SEN_MODE_PDAF) {
		*mode_type = ISP_SEN_MODE_PDAF;
	} else {
		*mode_type = ISP_SEN_MODE_LINEAR;
	}

	return rt;
}

ER isp_api_get_sensor_mode_info(ISP_ID id, ISP_SENSOR_MODE_INFO *mode_param)
{
	PCTL_SEN_OBJ p_sen_ctl_obj[CTL_SEN_ID_MAX] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	CTL_SEN_GET_MODE_BASIC_PARAM mode_basic_param;
	UINT32 min_expt = 0;
	UINT32 sen_id;
	ER rt;

	if (sen_id_mapping_enable) {
		sen_id = sen_id_mapping_table[id];
	} else {
		sen_id = id;
	}

	p_sen_ctl_obj[sen_id] = ctl_sen_get_object(sen_id);
	if (p_sen_ctl_obj[sen_id] == NULL) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object NULL, id: %d \r\n", sen_id);
		return E_NOEXS;
	}
	if (p_sen_ctl_obj[sen_id]->get_cfg == NULL) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object->get_cfg NULL, id: %d \r\n", sen_id);
		return E_NOEXS;
	}
	if (p_sen_ctl_obj[sen_id]->is_opened() == FALSE) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object->is_opened() FALSE, id: %d \r\n", sen_id);
		return E_NOEXS;
	}

	mode_basic_param.mode = CTL_SEN_MODE_CUR;
	rt = p_sen_ctl_obj[sen_id]->get_cfg(CTL_SEN_CFGID_GET_MODE_BASIC, &mode_basic_param);
	if (rt != 0) {
		return rt;
	}

	if (mode_basic_param.data_fmt == CTL_SEN_DATA_FMT_RGBIR) {
		mode_param->data_fmt = ISP_SEN_DATA_FMT_RGBIR;
	} else {
		mode_param->data_fmt = ISP_SEN_DATA_FMT_RGB;
	}

	if (mode_basic_param.mode_type == CTL_SEN_MODE_BUILTIN_HDR) {
		mode_param->mode_type = ISP_SEN_MODE_BUILTIN_HDR;
	} else if (mode_basic_param.mode_type == CTL_SEN_MODE_STAGGER_HDR) {
		mode_param->mode_type = ISP_SEN_MODE_STAGGER_HDR;
	} else if (mode_basic_param.mode_type == CTL_SEN_MODE_CCIR){
		mode_param->mode_type = ISP_SEN_MODE_CCIR;
	} else if (mode_basic_param.mode_type == CTL_SEN_MODE_CCIR_INTERLACE){
		mode_param->mode_type = ISP_SEN_MODE_CCIR_INTERLACE;
	} else {
		mode_param->mode_type = ISP_SEN_MODE_LINEAR;
	}

	mode_param->max_gain = mode_basic_param.gain.max;
	mode_param->min_gain = mode_basic_param.gain.min;
	rt |= p_sen_ctl_obj[sen_id]->get_cfg(CTL_SEN_CFGID_USER_DEFINE2, &min_expt);
	mode_param->min_expt = min_expt;
	memcpy(&mode_param->act_size, &mode_basic_param.act_size, sizeof(URECT)*ISP_SEN_MFRAME_MAX_NUM);
	memcpy(&mode_param->crp_size, &mode_basic_param.crp_size, sizeof(USIZE));
	mode_param->row_time = mode_basic_param.row_time;
	mode_param->row_time_step = mode_basic_param.row_time_step;
	return rt;
}

ER isp_api_get_func(ISP_ID id, ISP_FUNC_INFO *isp_func)
{
	ISP_DEV_INFO *pdev_info = isp_get_dev_info();
	ISP_FUNC_EN sie_func_en = 0, ipp_func_en = 0;
	CTL_SIE_ISP_STATUS sie_status = CTL_SIE_ISP_STS_CLOSE;
	CTL_IPP_ISP_STATUS_INFO ipp_status_info = {0};
	CTL_IPP_ISP_IOSIZE ipp_isp_iosize = {0};
	ER rt = E_OK;

	if (pdev_info == NULL) {
		return 0;
	}
	if (id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", id);
		return 0;
	}

	rt |= ctl_sie_isp_get(sie_id_mapping_table[id], CTL_SIE_ISP_ITEM_FUNC_EN, &sie_func_en);
	rt |= ctl_sie_isp_get(sie_id_mapping_table[id], CTL_SIE_ISP_ITEM_STATUS, &sie_status);
	rt |= ctl_ipp_isp_get(ipp_id_mapping_table[id], CTL_IPP_ISP_ITEM_FUNC_EN, &ipp_func_en);
	rt |= ctl_ipp_isp_get(ipp_id_mapping_table[id], CTL_IPP_ISP_ITEM_STATUS_INFO, &ipp_status_info);
	rt |= ctl_ipp_isp_get(ipp_id_mapping_table[id], CTL_IPP_ISP_ITEM_IOSIZE, &ipp_isp_iosize);

	isp_func->sie_valid = (sie_status == CTL_SIE_ISP_STS_RUN)  ? TRUE : FALSE;
	isp_func->ipp_valid = ((ipp_status_info.sts == CTL_IPP_ISP_STS_READY) || (ipp_status_info.sts == CTL_IPP_ISP_STS_RUN)) ? TRUE : FALSE;
	isp_func->ae_valid = FALSE;
	isp_func->af_valid = FALSE;
	isp_func->awb_valid = FALSE;
	if (pdev_info->isp_ae != NULL) {
		isp_func->ae_valid = ((sie_func_en & ISP_FUNC_EN_AE) && !(strcmp(pdev_info->isp_ae->name, "NVT_AE"))) ? TRUE : FALSE;
	}
	if (pdev_info->isp_af != NULL) {
		isp_func->af_valid = ((ipp_func_en & ISP_FUNC_EN_AF) && !(strcmp(pdev_info->isp_af->name, "NVT_AF"))) ? TRUE : FALSE;
	}
	if (pdev_info->isp_awb != NULL) {
		isp_func->awb_valid = ((sie_func_en & ISP_FUNC_EN_AWB) && !(strcmp(pdev_info->isp_awb->name, "NVT_AWB"))) ? TRUE : FALSE;
	}
	isp_func->defog_valid = (ipp_func_en & ISP_FUNC_EN_DEFOG) ? TRUE : FALSE;
	isp_func->shdr_valid = ((sie_func_en & ISP_FUNC_EN_SHDR) && (ipp_func_en & ISP_FUNC_EN_SHDR)) ? TRUE : FALSE;
	isp_func->wdr_valid = (ipp_func_en & ISP_FUNC_EN_WDR) ? TRUE : FALSE;
	memcpy(&isp_func->yuv_out_ch, &ipp_isp_iosize.out_ch, sizeof(ipp_isp_iosize.out_ch));

	return rt;
}

ER isp_api_get_sensor_reg(ISP_ID id, UINT32 *addr, UINT32 *data)
{
	PCTL_SEN_OBJ p_sen_ctl_obj[CTL_SEN_ID_MAX] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	CTL_SEN_CMD cmd = {0};
	UINT32 sen_id;
	ER rt;

	if (sen_id_mapping_enable) {
		sen_id = sen_id_mapping_table[id];
	} else {
		sen_id = id;
	}

	p_sen_ctl_obj[sen_id] = ctl_sen_get_object(sen_id);
	if (p_sen_ctl_obj[sen_id] == NULL) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object NULL, id: %d \r\n", sen_id);
		return E_NOEXS;
	}
	if (p_sen_ctl_obj[sen_id]->read_reg == NULL) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object->read_reg NULL, id: %d \r\n", sen_id);
		return E_NOEXS;
	}
	if (p_sen_ctl_obj[sen_id]->is_opened() == FALSE) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object->is_opened() FALSE, id: %d \r\n", sen_id);
		return E_NOEXS;
	}

	cmd.addr = *addr;
	rt = p_sen_ctl_obj[sen_id]->read_reg(&cmd);
	if (rt != 0) {
		return rt;
	}

	*data = ((cmd.data[1] << 8) & 0xFF00) | (cmd.data[0] & 0xFF);
	PRINT_ISP(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_SENSOR_REG, "get sensor reg., id (%d), addr (0x%X), data (0x%X) \r\n", (int)sen_id, *(int *)addr, *(int *)data);

	return rt;
}

ER isp_api_get_sensor_info(ISP_SENSOR_INFO *sensor_info)
{
	PCTL_SEN_OBJ p_sen_ctl_obj[CTL_SEN_ID_MAX] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	CTL_SEN_GET_ATTR_PARAM attr_param;
	UINT32 id, sen_id;
	ER rt = E_OK;

	for (id = 0; id < ISP_ID_MAX_NUM; id++) {
		if (sen_id_mapping_enable) {
			sen_id = sen_id_mapping_table[id];
		} else {
			sen_id = id;
		}

		p_sen_ctl_obj[sen_id] = ctl_sen_get_object(sen_id);

		if ((p_sen_ctl_obj[sen_id] == NULL) || (p_sen_ctl_obj[sen_id]->get_cfg == NULL) || (p_sen_ctl_obj[sen_id]->is_opened() == FALSE)) {
			sprintf(sensor_info->name[id], "NULL");
		} else {
			rt = p_sen_ctl_obj[sen_id]->get_cfg(CTL_SEN_CFGID_GET_ATTR, &attr_param);
			strcpy(sensor_info->name[id], attr_param.name);
		}

		sensor_info->src_id_mask[id] = isp_src_id_mask[id];
	}

	return rt;
}

ER isp_api_get_yuv(ISP_ID id, ISP_YUV_INFO *yuv_info)
{
	UINT32 i;
	UINT32 isp_id = id & 0xFFFF;
	ER rt = E_OK;

	if (isp_id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(isp_id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", id);
		return 0;
	}
	if (yout_lock[isp_id] == FALSE) {
		yout_lock[isp_id] = TRUE;

		ctl_ipp_isp_yuv_out.pid = yuv_info->pid;
		rt = ctl_ipp_isp_get((ipp_id_mapping_table[isp_id] & 0xFF000000) | id, CTL_IPP_ISP_ITEM_YUV_OUT, &ctl_ipp_isp_yuv_out);

		if (rt == E_OK) {
			yuv_info->info_vaild = TRUE;
			yuv_info->pxlfmt = (VDO_PXLFMT)ctl_ipp_isp_yuv_out.vdo_frm.pxlfmt;
			yuv_info->size.w = ctl_ipp_isp_yuv_out.vdo_frm.size.w;
			yuv_info->size.h = ctl_ipp_isp_yuv_out.vdo_frm.size.h;
			for (i = 0; i < VDO_MAX_PLANE; i++) {
				yuv_info->pw[i] = ctl_ipp_isp_yuv_out.vdo_frm.pw[i];
				yuv_info->ph[i] = ctl_ipp_isp_yuv_out.vdo_frm.ph[i];
				yuv_info->loff[i] = ctl_ipp_isp_yuv_out.vdo_frm.loff[i];
				yuv_info->addr[i] = ctl_ipp_isp_yuv_out.vdo_frm.addr[i];
				#if defined(__KERNEL__)
				if (ctl_ipp_isp_yuv_out.vdo_frm.addr[i] != 0) {
					yuv_info->psy_addr[i] = vos_cpu_get_phy_addr(ctl_ipp_isp_yuv_out.vdo_frm.addr[i]);
				} else {
					 yuv_info->psy_addr[i] = 0x0;
				}
				#else
				yuv_info->psy_addr[i] = ctl_ipp_isp_yuv_out.vdo_frm.addr[i];
				#endif
			}
		} else {
			PRINT_ISP_WRN(isp_dbg_get_dbg_mode(isp_id) & ISP_DBG_WRN_MSG, "get yuv timeout (%d, %d) (%d) \r\n", id, yuv_info->pid, rt);
			yuv_info->info_vaild = FALSE;
			rt = E_SYS;
		}
	} else {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(isp_id) & ISP_DBG_WRN_MSG, "id = %d, yout_lock: TRUE \r\n", id);
		yuv_info->info_vaild = FALSE;
		rt = E_SYS;
	}

	return rt;
}

ER isp_api_get_raw(ISP_ID id, ISP_RAW_INFO *raw_info)
{
	CTL_IPP_ISP_STATUS_INFO ipp_isp_status_info = {0};
	UINT32 isp_id = id & 0xFFFF;
	ER rt = E_OK;

	if (id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", id);
		return 0;
	}

	if (raw_lock[id] == FALSE) {
		raw_lock[id] = TRUE;

		rt = ctl_sie_isp_get(sie_id_mapping_table[id], CTL_SIE_ISP_ITEM_IMG_OUT, &ctl_isp_sie_header_info);
		if (rt == E_OK) {
			raw_info->info_vaild = TRUE;
			raw_info->pxlfmt = (VDO_PXLFMT)ctl_isp_sie_header_info.vdo_frm.pxlfmt;
			raw_info->size.w = ctl_isp_sie_header_info.vdo_frm.size.w;
			raw_info->size.h = ctl_isp_sie_header_info.vdo_frm.size.h;
			raw_info->pw = ctl_isp_sie_header_info.vdo_frm.pw[0];
			raw_info->ph = ctl_isp_sie_header_info.vdo_frm.ph[0];
			raw_info->loff = ctl_isp_sie_header_info.vdo_frm.loff[0];
			raw_info->addr = ctl_isp_sie_header_info.vdo_frm.addr[0];
			#if defined(__KERNEL__)
			raw_info->psy_addr = vos_cpu_get_phy_addr(ctl_isp_sie_header_info.vdo_frm.addr[0]);
			#else
			raw_info->psy_addr = ctl_isp_sie_header_info.vdo_frm.addr[0];
			#endif
		} else {
			ctl_ipp_isp_get((ipp_id_mapping_table[isp_id] & 0xFF000000) | id, CTL_IPP_ISP_ITEM_STATUS_INFO, &ipp_isp_status_info);
			if (ipp_isp_status_info.flow == CTL_IPP_ISP_FLOW_DIRECT_RAW) {
				DBG_WRN("cannot get raw at direct mode (%d) \r\n", id);
			} else {
				PRINT_ISP_WRN(isp_dbg_get_dbg_mode(id) & ISP_DBG_WRN_MSG, "get raw timeout (%d) (%d) \r\n", id, rt);
			}
			raw_info->info_vaild = FALSE;
			rt = E_SYS;
		}

	} else {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(id) & ISP_DBG_WRN_MSG, "id = %d, raw_lock: TRUE \r\n", id);
		raw_info->info_vaild = FALSE;
		rt = E_SYS;
	}

	return rt;
}

ER isp_api_get_3dnr_sta(ISP_ID id, ISP_3DNR_STA_INFO *_3dnr_sta_info)
{
	UINT32 isp_id = id & 0xFFFF;
	ER rt = E_OK;

	if (isp_id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(isp_id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", id);
		return 0;
	}
	rt = ctl_ipp_isp_get((ipp_id_mapping_table[isp_id] & 0xFF000000) | id, CTL_IPP_ISP_ITEM_3DNR_STA, &ctl_ipp_isp_3dnr_sta);

	if (rt == E_OK) {
		_3dnr_sta_info->info_vaild = TRUE;
		_3dnr_sta_info->enable = ctl_ipp_isp_3dnr_sta.enable;
		_3dnr_sta_info->buf_addr = ctl_ipp_isp_3dnr_sta.buf_addr;
		_3dnr_sta_info->max_sample_num = ctl_ipp_isp_3dnr_sta.max_sample_num;
		_3dnr_sta_info->lofs = ctl_ipp_isp_3dnr_sta.lofs;
		_3dnr_sta_info->sample_step.w = ctl_ipp_isp_3dnr_sta.sample_step.w;
		_3dnr_sta_info->sample_step.h = ctl_ipp_isp_3dnr_sta.sample_step.h;
		_3dnr_sta_info->sample_num.w = ctl_ipp_isp_3dnr_sta.sample_num.w;
		_3dnr_sta_info->sample_num.h = ctl_ipp_isp_3dnr_sta.sample_num.h;
		_3dnr_sta_info->sample_st.x = ctl_ipp_isp_3dnr_sta.sample_st.x;
		_3dnr_sta_info->sample_st.y = ctl_ipp_isp_3dnr_sta.sample_st.y;
	} else {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(isp_id) & ISP_DBG_WRN_MSG, "get _3dnr_sta fail (%d)  \r\n", id);
		_3dnr_sta_info->info_vaild = FALSE;
		rt = E_SYS;
	}

	return rt;
}

ER isp_api_get_md_sta(ISP_ID id, ISP_MD_STA_INFO *md_sta)
{
	CTL_IPP_ISP_MD_SUBOUT ipp_isp_md_subout = {0};
	UINT32 base_addr, w_div = 0, h_div = 0, read_value, max_cnt;
	UINT32 i, j, i_start, i_range, j_start, j_end, j_range, line_cnt = 0, bit_cnt = 0, bit_cnt_start;
	ER rt = E_OK;

	rt = ctl_ipp_isp_get(id, CTL_IPP_ISP_ITEM_MD_SUBOUT, &ipp_isp_md_subout);

	if (rt != E_OK) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(id) & ISP_DBG_WRN_MSG, "get md_sta fail (%d)  \r\n", id);
		md_sta->vaild = FALSE;
		return rt;
	}

	memset(md_sta, 0, sizeof(ISP_MD_STA_INFO));
	md_sta->vaild = TRUE;

	w_div = ipp_isp_md_subout.md_size.w / ISP_VA_W_WINNUM;
	h_div = ipp_isp_md_subout.md_size.h / ISP_VA_H_WINNUM;
	if (w_div == 0) {
		w_div = 1;
	}
	if (h_div == 0) {
		h_div = 1;
	}
	j_range = h_div * ISP_VA_H_WINNUM;
	j_start = (ipp_isp_md_subout.md_size.h - j_range) >> 1;
	j_end = j_start + j_range;
	i_range = w_div * ISP_VA_W_WINNUM;
	i_start = (ipp_isp_md_subout.md_size.w - i_range) >> 1;
	max_cnt = w_div * h_div;
	base_addr = ipp_isp_md_subout.addr;
	//printk("size %d %d, %d %d, lofs %d, addr 0x%X, div %d %d, i: %d %d, j: %d %d \r\n ", ipp_isp_md_subout.src_img_size.w, ipp_isp_md_subout.src_img_size.h, ipp_isp_md_subout.md_size.w, ipp_isp_md_subout.md_size.h, ipp_isp_md_subout.md_lofs, ipp_isp_md_subout.addr, w_div, h_div, i_start, i_range, j_start, j_end);

	for (j = j_start; j < j_end; j++) {
		for (i = 0; i < ipp_isp_md_subout.md_lofs; i+=4) {
			#if defined(__FREERTOS)
			read_value = *(UINT32 volatile *)(base_addr + (j * ipp_isp_md_subout.md_lofs + i));
			#else
			read_value = ioread32((void *)(base_addr + (j * ipp_isp_md_subout.md_lofs + i)));
			#endif

			if (i == 0) {
				bit_cnt_start = i_start;
			} else {
				bit_cnt_start = 0;
			}

			for (bit_cnt = bit_cnt_start; bit_cnt < 32; bit_cnt++) {
				if (line_cnt < i_range) {
					md_sta->data[((j - j_start) / h_div * ISP_VA_W_WINNUM) + (line_cnt / w_div)] += (read_value >> bit_cnt) & 0x1;
					line_cnt++;
				}
			}

			//if (j == j_start) {
			//	printk("i: %2d, j: %2d, line_cnt: %3d, idx: %2d, addr: %8X, data: %8X \r\n", i, j, line_cnt, ((j - j_start) / h_div * ISP_VA_W_WINNUM) + ((line_cnt - 1) / w_div), base_addr + (j * ipp_isp_md_subout.md_lofs + i), read_value);
			//}
		}

		//printk("i: %2d, j: %2d, line_cnt: %3d, idx: %2d, addr: %8X, data: %8X \r\n", i, j, line_cnt, ((j - j_start) / h_div * ISP_VA_W_WINNUM) + ((line_cnt - 1) / w_div), base_addr + (j * ipp_isp_md_subout.md_lofs + i - 4), read_value);
		line_cnt = 0;
	}

	for (i = 0; i < ISP_VA_MAX_WINNUM; i++) {
		md_sta->data[i] = md_sta->data[i] * 100 / max_cnt;
	}

	return E_OK;
}

ER isp_api_get_ir_info(ISP_ID id, ISP_RGBIR_INFO *ir_info)
{
	CTL_SIE_ISP_KDRV_PARAM sie_isp_kdrv_param;
	KDRV_SIE_RGBIR_INFO kdrv_ir_info;
	ER rt = E_OK;

	if (id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", id);
		return 0;
	}

	sie_isp_kdrv_param.param_id = KDRV_SIE_ITEM_IR_INFO;
	sie_isp_kdrv_param.data = (void *)&kdrv_ir_info;
	rt |= ctl_sie_isp_get(sie_id_mapping_table[id], CTL_SIE_ISP_ITEM_KDRV_PARAM, &sie_isp_kdrv_param);
	if (rt == E_OK) {
		ir_info->ir_level = kdrv_ir_info.kdrv_sie_ir_level;
		ir_info->saturation = kdrv_ir_info.kdrv_sie_ir_sat;
	}
	return rt;
}

ER isp_api_get_enc_isp_ratio(ISP_ID id, ISP_ENC_ISP_RATIO *enc_isp_ratio)
{
	ER rt = E_OK;

	if (id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", id);
		return 0;
	}

	memcpy(enc_isp_ratio, &(isp_enc_ratio[id]), sizeof(ISP_ENC_ISP_RATIO));
	return rt;
}

ISP_SENSOR_INIT_INFO *isp_api_get_fastboot_sensor_info(void)
{
	return &sensor_init_info;
}

BOOL isp_api_get_fastboot_valid(void)
{
	#if defined(__KERNEL__)
	return kdrv_builtin_is_fastboot();
	#else
	return FALSE;
	#endif
}

BOOL isp_api_get_fastboot_sensor_valid(UINT32 id)
{
	#if defined(__KERNEL__)
	return isp_builtin_get_sensor_valid(id);
	#else
	return FALSE;
	#endif
}

static void isp_api_get_fastboot_sync_info(UINT32 id)
{
	#if defined(__KERNEL__)
	ISP_AE_INIT_INFO ae_init_info;
	ISP_AWB_INIT_INFO awb_init_info;
	ISP_IQ_INIT_INFO iq_init_info;
	ISP_BUILTIN_CGAIN *builtin_cgain;
	ISP_BUILTIN_SENSOR_CTRL *p_sensor_ctrl;
	INT32 i;
	UINT32 id_mask;

	if (isp_builtin_get_sensor_valid(id)) {
		ae_init_info.lv = isp_builtin_get_lv(id);
		ae_init_info.total_gain = isp_builtin_get_total_gain(id);
		ae_init_info.d_gain = isp_builtin_get_dgain(id);
		ae_init_info.expt_max = isp_builtin_get_sensor_expt_max(id);
		ae_init_info.overexposure_offset = isp_builtin_get_overexposure_offset(id);
		isp_builtin_get_ae_param(id, &ae_init_info.param_addr);
		isp_dev_set_sync_item(id, ISP_SYNC_SEL_CURRENT, ISP_SYNC_AE_INIT, &ae_init_info);
		if (isp_builtin_get_shdr_enable(id)) {
			id_mask = isp_builtin_get_shdr_id_mask(id);
			for (i = ISP_ID_MAX_NUM-1; i >= 0; i--) {
				if ((0x1 << i) & id_mask) {
					isp_dev_set_sync_item(i, ISP_SYNC_SEL_CURRENT, ISP_SYNC_AE_INIT, &ae_init_info);
					break;
				}
			}
		}
		awb_init_info.ct = isp_builtin_get_ct(id);
		builtin_cgain = isp_builtin_get_cgain(id);
		awb_init_info.r_gain = builtin_cgain->r;
		awb_init_info.g_gain = builtin_cgain->g;
		awb_init_info.b_gain = builtin_cgain->b;
		isp_builtin_get_awb_param(id, &awb_init_info.param_addr);
		isp_dev_set_sync_item(id, ISP_SYNC_SEL_CURRENT, ISP_SYNC_AWB_INIT, &awb_init_info);
		if (isp_builtin_get_shdr_enable(id)) {
			isp_dev_set_sync_item(i, ISP_SYNC_SEL_CURRENT, ISP_SYNC_AWB_INIT, &awb_init_info);
		}
		iq_init_info.nr_lv = isp_builtin_get_nr_lv(id);
		iq_init_info._3dnr_lv = isp_builtin_get_3dnr_lv(id);
		iq_init_info.sharpness_lv = isp_builtin_get_sharpness_lv(id);
		iq_init_info.saturation_lv = isp_builtin_get_saturation_lv(id);
		iq_init_info.contrast_lv = isp_builtin_get_contrast_lv(id);
		iq_init_info.brightness_lv = isp_builtin_get_brightness_lv(id);
		iq_init_info.night_mode = isp_builtin_get_night_mode(id);
		isp_builtin_get_iq_param(id, &iq_init_info.param_addr);
		isp_dev_set_sync_item(id, ISP_SYNC_SEL_CURRENT, ISP_SYNC_IQ_INIT, &iq_init_info);

		p_sensor_ctrl = isp_builtin_get_sensor_expt(id);
		sensor_init_info.expt = p_sensor_ctrl->exp_time[id];
		p_sensor_ctrl = isp_builtin_get_sensor_gain(id);
		sensor_init_info.gain = p_sensor_ctrl->gain_ratio[id];
	}
	#endif
}

BOOL isp_api_get_emu_enable(void)
{
	return emu_enable;
}

ER isp_get_chip_info(ISP_CHIP_INFO *info)
{
	ER rt = E_OK;

	switch(nvt_get_chip_id()) {
		case CHIP_NA51055:
		default:
			info->id = ISP_CHIP_ID_NT9852X;
			break;

		case CHIP_NA51084:
			info->id = ISP_CHIP_ID_NT98528;
			break;

		case CHIP_NA51089:
			info->id = ISP_CHIP_ID_NT98560;
			break;
	}

	return rt;
}

ER isp_get_sie_func(ISP_ID id, ISP_FUNC_EN *fun_en)
{
	ER rt = E_OK;

	rt = ctl_sie_isp_get(sie_id_mapping_table[id], CTL_SIE_ISP_ITEM_FUNC_EN, fun_en);

	return rt;
}

ER isp_get_ipp_func(ISP_ID id, ISP_FUNC_EN *fun_en)
{
	ER rt = E_OK;

	rt = ctl_ipp_isp_get(ipp_id_mapping_table[id], CTL_IPP_ISP_ITEM_FUNC_EN, fun_en);

	return rt;
}


ER isp_api_set_expt(ISP_ID id, ISP_SENSOR_CTRL *sensor_ctrl)
{
	PCTL_SEN_OBJ p_sen_ctl_obj[CTL_SEN_ID_MAX] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	UINT32 sen_id;
	ER rt;

	if (sen_id_mapping_enable) {
		sen_id = sen_id_mapping_table[id];
	} else {
		sen_id = id;
	}

	if (isp_dbg_get_bypass_eng(sen_id) & ISP_BYPASS_SEN_EXPT) {
		return E_OK;
	}

	p_sen_ctl_obj[sen_id] = ctl_sen_get_object(sen_id);
	if (p_sen_ctl_obj[sen_id] == NULL) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object NULL, id: %d \r\n", sen_id);
		return E_NOEXS;
	}
	if (p_sen_ctl_obj[sen_id]->set_cfg == NULL) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object->set_cfg NULL, id: %d \r\n", sen_id);
		return E_NOEXS;
	}
	if (p_sen_ctl_obj[sen_id]->is_opened() == FALSE) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object->is_opened() FALSE, id: %d \r\n", sen_id);
		return E_NOEXS;
	}

	PRINT_ISP(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_SENSOR_EXPT, "id (%d), Expt (%7d, %7d) \r\n", (int)sen_id, (int)sensor_ctrl->exp_time[0], (int)sensor_ctrl->exp_time[1]);
	rt = p_sen_ctl_obj[sen_id]->set_cfg(CTL_SEN_CFGID_SET_EXPT, sensor_ctrl);

	return rt;
}

ER isp_api_set_gain(ISP_ID id, ISP_SENSOR_CTRL *sensor_ctrl)
{
	PCTL_SEN_OBJ p_sen_ctl_obj[CTL_SEN_ID_MAX] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	UINT32 sen_id;
	ER rt;

	if (sen_id_mapping_enable) {
		sen_id = sen_id_mapping_table[id];
	} else {
		sen_id = id;
	}

	if (isp_dbg_get_bypass_eng(sen_id) & ISP_BYPASS_SEN_GAIN) {
		return E_OK;
	}

	p_sen_ctl_obj[sen_id] = ctl_sen_get_object(sen_id);
	if (p_sen_ctl_obj[sen_id] == NULL) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object NULL, id: %d \r\n", sen_id);
		return E_NOEXS;
	}
	if (p_sen_ctl_obj[sen_id]->set_cfg == NULL) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object->set_cfg NULL, id: %d \r\n", sen_id);
		return E_NOEXS;
	}
	if (p_sen_ctl_obj[sen_id]->is_opened() == FALSE) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object->is_opened() FALSE, id: %d \r\n", sen_id);
		return E_NOEXS;
	}

	PRINT_ISP(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_SENSOR_GAIN, "id (%d), Gain (%7d, %7d) \r\n", (int)sen_id, (int)sensor_ctrl->gain_ratio[0], (int)sensor_ctrl->gain_ratio[1]);
	rt = p_sen_ctl_obj[sen_id]->set_cfg(CTL_SEN_CFGID_SET_GAIN, sensor_ctrl);

	return rt;
}

ER isp_api_set_preset(ISP_ID id, ISP_SENSOR_PRESET_CTRL *preset_ctrl)
{
	PCTL_SEN_OBJ p_sen_ctl_obj[CTL_SEN_ID_MAX] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	UINT32 sen_id;
	ER rt;

	if (sen_id_mapping_enable) {
		sen_id = sen_id_mapping_table[id];
	} else {
		sen_id = id;
	}

	p_sen_ctl_obj[sen_id] = ctl_sen_get_object(sen_id);
	if (p_sen_ctl_obj[sen_id] == NULL) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object NULL, id: %d \r\n", id);
		return E_NOEXS;
	}
	if (p_sen_ctl_obj[sen_id]->set_cfg == NULL) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object->set_cfg NULL, id: %d \r\n", id);
		return E_NOEXS;
	}
	if (p_sen_ctl_obj[sen_id]->is_opened() == FALSE) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object->is_opened() FALSE, id: %d \r\n", id);
		return E_NOEXS;
	}

	rt = p_sen_ctl_obj[sen_id]->set_cfg(CTL_SEN_CFGID_USER_DEFINE1, preset_ctrl);

	return rt;
}

ER isp_api_set_direction(ISP_ID id, ISP_SENSOR_DIRECTION *sensor_direction)
{
	UINT32 sen_flip = CTL_SEN_FLIP_NONE;
	PCTL_SEN_OBJ p_sen_ctl_obj[CTL_SEN_ID_MAX] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	UINT32 sen_id;
	ER rt;

	if (sen_id_mapping_enable) {
		sen_id = sen_id_mapping_table[id];
	} else {
		sen_id = id;
	}

	if (isp_dbg_get_bypass_eng(sen_id) & ISP_BYPASS_SEN_DIR) {
		return E_OK;
	}

	p_sen_ctl_obj[sen_id] = ctl_sen_get_object(sen_id);
	if (p_sen_ctl_obj[sen_id] == NULL) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object NULL, id: %d \r\n", id);
		return E_NOEXS;
	}
	if (p_sen_ctl_obj[sen_id]->set_cfg == NULL) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object->set_cfg NULL, id: %d \r\n", id);
		return E_NOEXS;
	}
	if (p_sen_ctl_obj[sen_id]->is_opened() == FALSE) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object->is_opened() FALSE, id: %d \r\n", id);
		return E_NOEXS;
	}

	if (sensor_direction->mirror) {
		sen_flip |= CTL_SEN_FLIP_H;
	}
	if (sensor_direction->flip) {
		sen_flip |= CTL_SEN_FLIP_V;
	}

	rt = p_sen_ctl_obj[sen_id]->set_cfg(CTL_SEN_CFGID_FLIP_TYPE, &sen_flip);

	return rt;
}

ER isp_api_set_sensor_reg(ISP_ID id, UINT32 *addr, UINT32 *data)
{
	PCTL_SEN_OBJ p_sen_ctl_obj[CTL_SEN_ID_MAX] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	CTL_SEN_CMD cmd = {0};
	UINT32 sen_id;
	ER rt;

	if (sen_id_mapping_enable) {
		sen_id = sen_id_mapping_table[id];
	} else {
		sen_id = id;
	}

	if (isp_dbg_get_bypass_eng(sen_id) & ISP_BYPASS_SEN_REG) {
		return E_OK;
	}

	p_sen_ctl_obj[sen_id] = ctl_sen_get_object(sen_id);
	if (p_sen_ctl_obj[sen_id] == NULL) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object NULL, id: %d \r\n", sen_id);
		return E_NOEXS;
	}
	if (p_sen_ctl_obj[sen_id]->write_reg == NULL) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object->write_reg NULL, id: %d \r\n", sen_id);
		return E_NOEXS;
	}
	if (p_sen_ctl_obj[sen_id]->is_opened() == FALSE) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object->is_opened() FALSE, id: %d \r\n", sen_id);
		return E_NOEXS;
	}

	cmd.addr = *addr;
	cmd.data[1] = (*data >> 8) & 0xFF;  // MSB
	cmd.data[0] = *data & 0xFF;         // LSB

	PRINT_ISP(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_SENSOR_REG, "set sensor reg., id (%d), addr (0x%X), data (0x%X) \r\n", (int)sen_id, *(int *)addr, *(int *)data);
	rt = p_sen_ctl_obj[sen_id]->write_reg(&cmd);

	return rt;
}

ER isp_api_set_sensor_sleep(ISP_ID id)
{
	PCTL_SEN_OBJ p_sen_ctl_obj[CTL_SEN_ID_MAX] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	UINT32 sen_id;
	ER rt;

	if (sen_id_mapping_enable) {
		sen_id = sen_id_mapping_table[id];
	} else {
		sen_id = id;
	}

	if (isp_dbg_get_bypass_eng(sen_id) & ISP_BYPASS_SEN_SLEEP) {
		return E_OK;
	}

	p_sen_ctl_obj[sen_id] = ctl_sen_get_object(sen_id);
	if (p_sen_ctl_obj[sen_id] == NULL) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object NULL, id: %d \r\n", sen_id);
		return E_NOEXS;
	}
	if (p_sen_ctl_obj[sen_id]->sleep == NULL) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object->sleep NULL, id: %d \r\n", sen_id);
		return E_NOEXS;
	}
	if (p_sen_ctl_obj[sen_id]->is_opened() == FALSE) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object->is_opened() FALSE, id: %d \r\n", sen_id);
		return E_NOEXS;
	}

	rt = p_sen_ctl_obj[sen_id]->sleep();

	return rt;
}

ER isp_api_set_sensor_wakeup(ISP_ID id)
{
	PCTL_SEN_OBJ p_sen_ctl_obj[CTL_SEN_ID_MAX] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	UINT32 sen_id;
	ER rt;

	if (sen_id_mapping_enable) {
		sen_id = sen_id_mapping_table[id];
	} else {
		sen_id = id;
	}

	if (isp_dbg_get_bypass_eng(sen_id) & ISP_BYPASS_SEN_WAKEUP) {
		return E_OK;
	}

	p_sen_ctl_obj[sen_id] = ctl_sen_get_object(sen_id);
	if (p_sen_ctl_obj[sen_id] == NULL) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object NULL, id: %d \r\n", sen_id);
		return E_NOEXS;
	}
	if (p_sen_ctl_obj[sen_id]->wakeup == NULL) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object->sleep NULL, id: %d \r\n", sen_id);
		return E_NOEXS;
	}
	if (p_sen_ctl_obj[sen_id]->is_opened() == FALSE) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(sen_id) & ISP_DBG_WRN_MSG, "ctl_sen_get_object->is_opened() FALSE, id: %d \r\n", sen_id);
		return E_NOEXS;
	}

	rt = p_sen_ctl_obj[sen_id]->wakeup();

	return rt;
}

ER isp_api_set_yuv(ISP_ID id)
{
	UINT32 isp_id = id & 0xFFFF;
	ER rt = E_OK;

	if (isp_id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(isp_id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", id);
		return 0;
	}
	if (yout_lock[isp_id] == TRUE) {
		yout_lock[isp_id] = FALSE;

		rt = ctl_ipp_isp_set((ipp_id_mapping_table[isp_id] & 0xFF000000) | id, CTL_IPP_ISP_ITEM_YUV_OUT, &ctl_ipp_isp_yuv_out);
	} else {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(isp_id) & ISP_DBG_WRN_MSG, "id = %d, yout_lock: FALSE \r\n", id);
		rt = E_SYS;
	}

	return rt;
}

ER isp_api_set_raw(ISP_ID id)
{
	ER rt = E_OK;

	if (id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", id);
		return 0;
	}
	if (raw_lock[id] == TRUE) {
		raw_lock[id] = FALSE;

		rt = ctl_sie_isp_set(sie_id_mapping_table[id], CTL_SIE_ISP_ITEM_IMG_OUT, &ctl_isp_sie_header_info);
	} else {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(id) & ISP_DBG_WRN_MSG, "id = %d, raw_lock: FALSE \r\n", id);
		rt = E_SYS;
	}
	return rt;
}

ER isp_api_set_emu_new_buf(ISP_ID id, CTL_SIE_ISP_SIM_BUF_NEW *param)
{
	ER rt = E_OK;

	rt = ctl_sie_isp_set(sie_id_mapping_table[id], CTL_SIE_ISP_ITEM_SIM_BUF_NEW, param);

	return rt;
}

ER isp_api_set_emu_run(ISP_ID id, CTL_SIE_ISP_SIM_BUF_PUSH *param)
{
	ER rt = E_OK;

	if (!param->sim_end) {
		emu_enable = TRUE;
	} else {
		emu_enable = FALSE;
	}

	rt = ctl_sie_isp_set(sie_id_mapping_table[id], CTL_SIE_ISP_ITEM_SIM_BUF_PUSH, param);

	return rt;
}

ER isp_api_set_md_param(ISP_ID id, CTL_SIE_ISP_SIM_BUF_PUSH *param)
{
	ER rt = E_OK;

	if (!param->sim_end) {
		emu_enable = TRUE;
	} else {
		emu_enable = FALSE;
	}

	rt = ctl_sie_isp_set(sie_id_mapping_table[id], CTL_SIE_ISP_ITEM_SIM_BUF_PUSH, param);

	return rt;
}

void isp_api_set_iq_param(ISP_ID id, ISP_IQ_ITEM isp_iq_item, void *param)
{
	UINT32 isp_id = id & 0xFFFF;
	//ISP_DEV_INFO *pdev_info = isp_get_dev_info();

	if (isp_id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(isp_id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", id);
		return;
	}

	//down(&pdev_info->api_mutex);

	switch (isp_iq_item) {
	case ISP_IQ_ITEM_SIE_ROI:
		if (!(isp_dbg_get_bypass_eng(isp_id) & ISP_BYPASS_SIE_ROI)) {
			ctl_sie_isp_set(sie_id_mapping_table[id], CTL_SIE_ISP_ITEM_STA_ROI_RATIO, (CTL_SIE_ISP_ROI_RATIO *)param);
		}
	break;

	case ISP_IQ_ITEM_SIE_PARAM:
		if (!(isp_dbg_get_bypass_eng(isp_id) & ISP_BYPASS_SIE_PARAM)) {
			ctl_sie_isp_set(sie_id_mapping_table[id], CTL_SIE_ISP_ITEM_IQ_PARAM, (CTL_SIE_IQ_PARAM *)param);
			PRINT_ISP(isp_dbg_get_dbg_mode(isp_id) & ISP_DBG_SIE_SET, "ctl_sie_isp_set (%X) SIE_IQ_PARAM \r\n", sie_id_mapping_table[id]);
		}
	break;

	case ISP_IQ_ITEM_IFE_PARAM:
		if (!(isp_dbg_get_bypass_eng(isp_id) & ISP_BYPASS_IFE_PARAM)) {
			ctl_ipp_isp_set((ipp_id_mapping_table[isp_id] & 0xFF000000) | id, CTL_IPP_ISP_ITEM_IFE_IQ_PARAM, (CTL_IFE_ISP_IQ_ALL *)param);
			PRINT_ISP(isp_dbg_get_dbg_mode(isp_id) & ISP_DBG_IPP_SET, "ctl_ipp_isp_set (%X) IFE_IQ_PARAM \r\n", ipp_id_mapping_table[id]);
		}
	break;

	case ISP_IQ_ITEM_DCE_PARAM:
		if (!(isp_dbg_get_bypass_eng(isp_id) & ISP_BYPASS_DCE_PARAM)) {
			ctl_ipp_isp_set((ipp_id_mapping_table[isp_id] & 0xFF000000) | id, CTL_IPP_ISP_ITEM_DCE_IQ_PARAM, (CTL_DCE_ISP_IQ_ALL *)param);
		}
	break;

	case ISP_IQ_ITEM_IPE_PARAM:
		if (!(isp_dbg_get_bypass_eng(isp_id) & ISP_BYPASS_IPE_PARAM)) {
			ctl_ipp_isp_set((ipp_id_mapping_table[isp_id] & 0xFF000000) | id, CTL_IPP_ISP_ITEM_IPE_IQ_PARAM, (CTL_IPE_ISP_IQ_ALL *)param);
		}
	break;

	case ISP_IQ_ITEM_IME_PARAM:
		if (!(isp_dbg_get_bypass_eng(isp_id) & ISP_BYPASS_IME_PARAM)) {
			ctl_ipp_isp_set((ipp_id_mapping_table[isp_id] & 0xFF000000) | id, CTL_IPP_ISP_ITEM_IME_IQ_PARAM, (CTL_IME_ISP_IQ_ALL *)param);
		}
	break;

	case ISP_IQ_ITEM_IFE_VIG_CENT:
		if (!(isp_dbg_get_bypass_eng(isp_id) & ISP_BYPASS_IFE_VIG_CENT)) {
			ctl_ipp_isp_set((ipp_id_mapping_table[isp_id] & 0xFF000000) | id, CTL_IPP_ISP_ITEM_IFE_VIG_CENT, (CTL_IPP_ISP_IFE_VIG_CENT_RATIO *)param);
		}
	break;

	case ISP_IQ_ITEM_DCE_DC_CENT:
		if (!(isp_dbg_get_bypass_eng(isp_id) & ISP_BYPASS_DCE_DC_CENT)) {
			ctl_ipp_isp_set((ipp_id_mapping_table[isp_id] & 0xFF000000) | id, CTL_IPP_ISP_ITEM_DCE_DC_CENT, (CTL_IPP_ISP_DCE_DC_CENT_RATIO *)param);
		}
	break;

	case ISP_IQ_ITEM_IFE2_FILT_TIME:
		if (!(isp_dbg_get_bypass_eng(isp_id) & ISP_BYPASS_IFE2_FILT_TIME)) {
			ctl_ipp_isp_set((ipp_id_mapping_table[isp_id] & 0xFF000000) | id, CTL_IPP_ISP_ITEM_IFE2_FILT_TIME, (CTL_IPP_ISP_IFE2_FILTER_TIME *)param);
		}
	break;

	case ISP_IQ_ITEM_IME_LCA_SIZE:
		if (!(isp_dbg_get_bypass_eng(isp_id) & ISP_BYPASS_IME_LCA_SIZE)) {
			ctl_ipp_isp_set((ipp_id_mapping_table[isp_id] & 0xFF000000) | id, CTL_IPP_ISP_ITEM_IME_LCA_SIZE, (CTL_IPP_ISP_IME_LCA_SIZE_RATIO *)param);
		}
	break;

	case ISP_IQ_ITEM_IPE_VA_WIN_SIZE:
		if (!(isp_dbg_get_bypass_eng(isp_id) & ISP_BYPASS_IPE_VA_WIN_SIZE)) {
			ctl_ipp_isp_set((ipp_id_mapping_table[isp_id] & 0xFF000000) | id, CTL_IPP_ISP_ITEM_VA_WIN_SIZE, (CTL_IPP_ISP_VA_WIN_SIZE_RATIO *)param);
		}
	break;

	case ISP_IQ_ITEM_ENC_3DNR_PARAM:
		if (!(isp_dbg_get_bypass_eng(isp_id) & ISP_BYPASS_ENC_3DNR_PARAM)) {
			kflow_videoenc_set(id, KFLOW_VIDEOENC_ISP_ITEM_TNR, (KDRV_VDOENC_3DNR *)param);
			PRINT_ISP(isp_dbg_get_dbg_mode(isp_id) & ISP_DBG_ENC_SET, "kflow_videoenc_set (%d) ENC_3DNR_IQ_PARAM \r\n", id);
		}
	break;

	case ISP_IQ_ITEM_ENC_SHARPEN_PARAM:
		if (!(isp_dbg_get_bypass_eng(isp_id) & ISP_BYPASS_ENC_SHARPEN_PARAM)) {
			kflow_videoenc_set(id, KFLOW_VIDEOENC_ISP_ITEM_SPN, (KDRV_H26XENC_SPN *)param);
			PRINT_ISP(isp_dbg_get_dbg_mode(isp_id) & ISP_DBG_ENC_SET, "kflow_videoenc_set (%d) ENC_SHARPEN_IQ_PARAM \r\n", id);
		}
	break;

	default:
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(isp_id) & ISP_DBG_WRN_MSG, "ISP_IQ_ITEM out of renage (%d) \r\n", isp_iq_item);
	break;
	}

	//up(&pdev_info->api_mutex);

	return;
}

#if defined(__KERNEL__)
EXPORT_SYMBOL(isp_api_get_expt);
EXPORT_SYMBOL(isp_api_get_gain);
EXPORT_SYMBOL(isp_api_get_sync_time);
EXPORT_SYMBOL(isp_api_get_frame_num);
EXPORT_SYMBOL(isp_api_get_chg_fps);
EXPORT_SYMBOL(isp_api_get_sensor_mode_info);
EXPORT_SYMBOL(isp_api_get_md_sta);
EXPORT_SYMBOL(isp_api_get_ir_info);
EXPORT_SYMBOL(isp_api_get_enc_isp_ratio);
EXPORT_SYMBOL(isp_api_get_fastboot_sensor_info);
EXPORT_SYMBOL(isp_api_get_fastboot_valid);
EXPORT_SYMBOL(isp_api_get_fastboot_sensor_valid);
EXPORT_SYMBOL(isp_get_chip_info);
EXPORT_SYMBOL(isp_api_set_expt);
EXPORT_SYMBOL(isp_api_set_gain);
EXPORT_SYMBOL(isp_api_set_preset);
EXPORT_SYMBOL(isp_api_set_iq_param);
#endif

