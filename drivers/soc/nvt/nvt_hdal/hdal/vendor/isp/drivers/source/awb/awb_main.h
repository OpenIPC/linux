#ifndef _AWB_MAIN_H_
#define _AWB_MAIN_H_

// From awb_flow.h
extern AWB_PARAM *awb_param[AWB_ID_MAX_NUM];
extern BOOL awb_flow_get_id_valid(UINT32 id);
extern UINT32 awb_flow_get_info_map(UINT32 id);
extern void awb_flow_show_info(struct seq_file *sfile);
extern void awb_flow_get_wb(AWB_ID id, VOID *wb);
extern void awb_flow_get_expand_ct_en(AWB_ID id, UINT32 *en);
extern void awb_flow_get_weight_mode(AWB_ID id, UINT32 *en, UINT32 *ct_idx);
extern void awb_flow_get_gray_world_en(AWB_ID id, UINT32 *en);
extern void awb_flow_get_ca_th(AWB_ID id, VOID *ca_th);
extern void awb_flow_set_mwb(AWB_ID id, VOID *mwb);
extern void awb_flow_set_expand_ct_en(AWB_ID id, UINT32 en);
extern void awb_flow_set_weight_mode(AWB_ID id, UINT32 en);
extern void awb_flow_set_gray_world_en(AWB_ID id, UINT32 en);
extern void awb_flow_set_ca_th(AWB_ID id, VOID *ca_th);
extern void awb_flow_get_ca_msg(struct seq_file *sfile, AWB_ID id);
extern void awb_flow_get_flag(struct seq_file *sfile, AWB_ID id);
extern ISP_MODULE *awb_get_module(void);
extern INT32 awb_flow_init(UINT32 id);
extern INT32 awb_flow_uninit(UINT32 id);
extern INT32 awb_flow_process(UINT32 id, void *arg);
extern ER awb_init_module(UINT32 id_list);
extern ER awb_uninit_module(void);

// From awbt_api_int.h
extern UINT32 awbt_api_get_item_size(AWBT_ITEM item);
extern ER awbt_api_get_cmd(AWBT_ITEM item, UINT32 addr);
extern ER awbt_api_set_cmd(AWBT_ITEM item, UINT32 addr);

// From awb_ui_int.h
extern ER awb_ui_set_info(AWB_ID id, AWB_UI_INFO index, UINT32 value);
extern UINT32 awb_ui_get_info(AWB_ID id, AWB_UI_INFO index);

#endif

