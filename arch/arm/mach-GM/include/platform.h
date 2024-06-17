#ifndef _IRDET_PLATFORM_H
#define _IRDET_PLATFORM_H
#include "sar_adc_drv.h"
#include "sar_adc_api.h"

#define DEV_IRQ_START 	ADC_WRAP_0_IRQ
#define DEV_IRQ_END 	ADC_WRAP_0_IRQ
#define DEV_PA_START 	ADC_WRAP_0_PA_BASE
#define DEV_PA_END 	ADC_WRAP_0_PA_LIMIT

#if defined(CONFIG_PLATFORM_GM8287)
#define ADC_WRAP_CLEAR_INT             0x01
#define ADDA_GET_IO_ADDR(io_add)  
#define ADDA_UMMAP_IO_ADDR(io_addr)

#elif defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
#define ADDA_PA_START                  ADDA_WRAP_0_PA_BASE
#define ADDA_PA_END                    ADDA_WRAP_0_PA_LIMIT
#define TVE100_PA_START                ADDA_WRAP_0_PA_BASE
#define TVE100_PA_END                  ADDA_WRAP_0_PA_LIMIT

#define ADDA_DAC_REG                   0x14
#define ADDA_DAC_DET_EN                0x10
#define ADC_WRAP_CLEAR_INT             0x3F/* xlian2 high/low interrupt flag */
#define TVE_POWER_DOWN_REG             0x0C
#endif


extern int scu_probe(struct scu_t *p_scu);
extern int scu_remove(struct scu_t *p_scu);
extern int set_pinmux(void);

int Platform_Init_Pollign_Timer(struct dev_data *p_dev_data);
void Platform_Del_Pollign_Timer(void);
int Platform_Get_Polling_Timer_State(void);
void Platform_Set_Init_Reg(struct dev_data *p_dev_data);
void Platform_Process_Volt_Threshold(struct dev_data *p_dev_data); 
int Platform_SAR_ADC_GetData(struct dev_data *p_dev_data , sar_adc_pub_data* new_data);
void Platform_Lock_Polling_Mutex(void);
void Platform_UnLock_Polling_Mutex(void);
int Platform_Check_Support_XGain(int num);
unsigned int Platform_Direct_Get_XGain_Value(unsigned int base , int num);

#if defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
int Platform_Get_CVBS_Info(void);
int Platform_Get_SARADC_Debug(void);
void Platform_Set_SARADC_Debug(int value);
int Platform_Force_Set_ADDA_TVBS(int force);
int Platform_Force_Get_ADDA_TVBS(void);
int Platform_Set_CVBS_THR_Value(unsigned int on_vlaue,unsigned int off_value);
int Platform_Get_CVBS_THR_Value(unsigned int *on_vlaue,unsigned int *off_value);
int Platform_Set_Polling_Time(unsigned int time_val);
u32 Platform_Get_Polling_Time(void);
#endif

#if defined(CONFIG_PLATFORM_GM8287)
int Platform_Get_SARADC_Debug(void);
#endif

#endif
