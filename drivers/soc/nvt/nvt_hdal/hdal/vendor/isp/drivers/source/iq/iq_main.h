#ifndef _IQ_MAIN_H_
#define _IQ_MAIN_H_

// From iq_flow.h
extern IQ_PARAM_PTR *iq_param[IQ_ID_MAX_NUM];
extern BOOL test_mode_en[IQ_ID_MAX_NUM];
extern IQ_OB_MODE test_mode_ob[IQ_ID_MAX_NUM];
extern IQ_DG_MODE test_mode_dg[IQ_ID_MAX_NUM];
extern IQ_CG_MODE test_mode_cg[IQ_ID_MAX_NUM];
extern BOOL companding_en[IQ_ID_MAX_NUM];
extern UINT32 nr_r_ratio[IQ_ID_MAX_NUM];
extern UINT32 nr_b_ratio[IQ_ID_MAX_NUM];
extern UINT32 nr_ir_ratio[IQ_ID_MAX_NUM];
extern UINT32 residue_reset_num[IQ_ID_MAX_NUM];
extern BOOL tmnr_ae_still_en[IQ_ID_MAX_NUM];
extern void iq_flow_ext_set_cfa_en(IQ_ID id, BOOL en);
extern void iq_flow_ext_set_wdr_subimg_lpf(IQ_ID id, UINT32 coef_0, UINT32 coef_1, UINT32 coef_2);
extern void iq_flow_ext_set_wdr_blend_w(IQ_ID id, UINT32 blend_w);
extern void iq_flow_ext_set_ipe_subimg_size(IQ_ID id, UINT32 width, UINT32 height);
extern void iq_flow_ext_set_edge_gamma_sel(IQ_ID id, UINT32 sel);
extern void iq_flow_ext_set_edge_region_str(IQ_ID id, UINT32 enable, UINT32 enh_thin, UINT32 enh_robust);
extern void iq_flow_ext_set_dbcs_y_wt(IQ_ID id, UINT32 wt0, UINT32 wt1, UINT32 wt2, UINT32 wt3, UINT32 wt4, UINT32 wt5  \
												, UINT32 wt6, UINT32 wt7, UINT32 wt8, UINT32 wt9, UINT32 wt10          \
												, UINT32 wt11, UINT32 wt12, UINT32 wt13, UINT32 wt14, UINT32 wt15);
extern void iq_flow_ext_set_dbcs_c_wt(IQ_ID id, UINT32 wt0, UINT32 wt1, UINT32 wt2, UINT32 wt3, UINT32 wt4, UINT32 wt5  \
												, UINT32 wt6, UINT32 wt7, UINT32 wt8, UINT32 wt9, UINT32 wt10          \
												, UINT32 wt11, UINT32 wt12, UINT32 wt13, UINT32 wt14, UINT32 wt15);
extern void iq_flow_ext_set_post_sharpen_con_eng(IQ_ID id, UINT32 con_eng);
extern void iq_flow_ext_set_post_sharpen_weight(IQ_ID id, UINT32 low_w, UINT32 high_w);
extern void iq_flow_show_info(struct seq_file *sfile);
extern BOOL iq_flow_get_id_valid(UINT32 id);
extern UINT32 iq_flow_get_info_map(UINT32 id);
extern ISP_MODULE *iq_get_module(void);
extern ER iq_init_module(UINT32 id_list, UINT32 dpc_en, UINT32 ecs_en, UINT32 _2dlut_en);
extern ER iq_uninit_module(void);

// From iqt_api_int.h
extern UINT32 iqt_api_get_item_size(IQT_ITEM item);
extern ER iqt_api_get_cmd(IQT_ITEM item, UINT32 addr);
extern ER iqt_api_set_cmd(IQT_ITEM item, UINT32 addr);

// From iq_lib.h
extern UINT32 iq_auto_wdr_dark_th;
extern UINT32 iq_auto_wdr_dr_th;
extern UINT32 iq_auto_wdr_dark_compensate;

// From iq_ui_int.h
extern ER iq_ui_set_info(IQ_ID id, IQ_UI_ITEM index, UINT32 value);
extern UINT32 iq_ui_get_info(IQ_ID id, IQ_UI_ITEM index);

#endif

