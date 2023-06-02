#ifndef _AE_MAIN_H_
#define _AE_MAIN_H_

// From ae_flow.h
extern AE_PARAM *ae_param[AE_ID_MAX_NUM];
extern ISP_MODULE *ae_get_module(void);
extern INT32 ae_flow_init(UINT32 id);
extern INT32 ae_flow_uninit(UINT32 id);
extern INT32 ae_flow_process(UINT32 id, void *arg);
extern ER ae_init_module(UINT32 id_list);
extern ER ae_uninit_module(void);

// From aet_api_int.h
extern UINT32 aet_api_get_item_size(AET_ITEM item);
extern ER aet_api_get_cmd(AET_ITEM item, UINT32 addr);
extern ER aet_api_set_cmd(AET_ITEM item, UINT32 addr);
extern BOOL ae_flow_get_id_valid(UINT32 id);
extern UINT32 ae_flow_get_info_map(UINT32 id);

#endif

