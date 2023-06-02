#ifndef _ISP_RTOS_INC_H_
#define _ISP_RTOS_INC_H_

extern ER ae_init_module(UINT32 id_list);
extern ER ae_uninit_module(void);
extern ER awb_init_module(UINT32 id_list);
extern ER awb_uninit_module(void);
extern ER iq_init_module(UINT32 id_list, UINT32 dpc_en, UINT32 ecs_en, UINT32 _2dlut_en);
extern ER iq_uninit_module(void);

#endif

