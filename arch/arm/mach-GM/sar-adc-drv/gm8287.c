#include "platform.h"                                                                                                                                                     
#include "sar_adc_drv.h"                                                                                                                                                  
#include "sar_adc_dev.h"                                                                                                                                                  
#include <mach/ftpmu010.h>                                               
                                                                                                                                         
#include <linux/delay.h>                                                                                                                                                  
#include <linux/timer.h>

#if (HZ==1000)                                                                                                                                                  
#define POLLING_DELAY 200
#elif (HZ==100)
#define POLLING_DELAY 20
#else
#define POLLING_DELAY 200
#endif

struct timer_list xain_change_timer;                                                                                                                                                             
                                                                                                                                                                          
#define PRINT_E(args...) do { printk(args); }while(0)                                                                                                                     
#define PRINT_I(args...) do { printk(args); }while(0)                                                                                                                     
                                                                                                                                                                          
#define PLL_CLK          0xB8   /* ADC0 clock */                                                                                                                          
#define PLL_CLK_BIT     (0x1<<22)                                                                                                                                         
                                                                                                                                                                          
#define GATE_CLK	 0x70  /* ADC0 clk select and pvalue */                                                                                                   
#define GATE_CLK_BIT	(0xff << 24)                                                                                                                           

                                                                               
                                                                               
                                                                               
static int scu_fd;                                                                                                                                                        
                                                                                                                                                                          
/* *INDENT-OFF* */                                                                                                                                                        
static pmuReg_t pmu_reg[] = {                                                                                                                                             
/*  {reg_off,   bits_mask,     lock_bits,    init_val,    init_mask } */                                                                                                  
    {PLL_CLK,   PLL_CLK_BIT,   PLL_CLK_BIT,      0x0<<22,         0x1 << 22 }, // ADC PLL clock selection                                                                 
    {GATE_CLK,  GATE_CLK_BIT,  GATE_CLK_BIT, 	 (0x0f<<24|0x2<<30),0xff<<24}, /* pvalue is 16(0x0f+1) 12M/16=768K , clk sel is 0x2 for dividing  by cntyp */             
};                                                                                        
                                                                                                                                                                          
static pmuRegInfo_t pmu_reg_info = {                                                                                                                                      
    DEV_NAME,                                                                                                                                                             
    ARRAY_SIZE(pmu_reg),                                                                                                                                                  
    ATTR_TYPE_NONE,                                                                                                                                                       
    pmu_reg,                                                                                                                                                              
};                                                                                                                                                                        
/* *INDENT-ON* */                                                                                                                                                         
                                                                                                                                                                          
int register_scu(void)                                                                                                                                                    
{                                                                                                                                                                         
    scu_fd = ftpmu010_register_reg(&pmu_reg_info);                                                                                                                        
    if (scu_fd < 0) {                                                                                                                                                     
        printk("Failed to register %s scu\n", DEV_NAME);                                                                                                                  
        return -1;                                                                                                                                                        
    }                                                                                                                                                                     
    return 0;                                                                                                                                                             
}                                                                                                                                                                         
                                                                                                                                                                          
int deregister_scu(void)                                                                                                                                                  
{                                                                                                                                                                         
    if (scu_fd >= 0) {                                                                                                                                                    
        if (ftpmu010_deregister_reg(scu_fd)) {                                                                                                                            
            printk("Failed to deregister %s scu\n", DEV_NAME);                                                                                                            
            scu_fd = -1;                                                                                                                                                  
            return -1;                                                                                                                                                    
        }                                                                                                                                                                 
    }                                                                                                                                                                     
    return 0;                                                                                                                                                             
}                                                                                                                                                                         
                                                                                                                                                                          
int scu_probe(struct scu_t *p_scu)                                                                                                                                        
{                                                                                                                                                                         
    if (unlikely(register_scu() < 0))                                                                                                                                     
    {                                                                                                                                                                     
        printk("register_scu errr\n");                                                                                                                                    
        return -1;                                                                                                                                                        
    }                                                                                                                                                                     
    return 0;                                                                                                                                                             
}                                                                                                                                                                         
                                                                                                                                                                          
int scu_remove(struct scu_t *p_scu)                                                                                                                                       
{                                                                                                                                                                         
    int ret;                                                                                                                                                              
                                                                                                                                                                          
    ret = deregister_scu();                                                                                                                                               
    return ret;                                                                                                                                                           
}                                                                                                                                                                         
                                                                                                                                                                          
int set_pinmux(void)                                                                                                                                                      
{                                                                                                                                                                         
    unsigned int pmu_mfps = ftpmu010_read_reg(PLL_CLK);                                                                                                               
    int ret = 0;                                                                                                                                                      
                                                                                                                                                                          
    pmu_mfps &= ~(PLL_CLK_BIT);                                                                                                                                       
                                                                                                                                                                          
    if (unlikely((ret = ftpmu010_write_reg(scu_fd, PLL_CLK , 0, PLL_CLK_BIT))!= 0))                                                                                   
    {                                                                                                                                                                 
	PRINT_E("%s set as 0x%08X\n", __func__, pmu_mfps);                                                                                                        
    }                                                                                                                                                                 
    pmu_mfps = ftpmu010_read_reg(PLL_CLK);                                                                                                                            
    PRINT_I("%s set as 0x%02X 0x%08X OK\n", __func__, PLL_CLK, pmu_mfps);                                                                                             
                                                                                                                                                                          
    return ret;                                                                                                                                                           
}                                                                                                                                                                         
                                                                                                                                                                          
static void change_detect_xain(unsigned long data)                                                                                                                        
{                                                                                                                                                                         
    struct dev_data *p_dev_data = (struct dev_data*)data;                                                                                                             
    unsigned int base = (unsigned int)p_dev_data->io_vadr;                                                                                                            
	                                                                                                                                                                  
    static int xain_num=0;                                                                                                                                                
    int val;                                                                                                                                                              
                                                                                                                                                                          
    if(!xain_num){                                                                                                                                                        
        val = 0x000B2021;                                                                                                                                                 
        SAR_ADC_RAW_REG_WR( base, FTSAR_ADC_CR0, val);                                                                                                                    
        xain_num = 1;                                                                                                                                                     
    }else{                                                                                                                                                                
        val = 0x000B2022;                                                                                                                                                 
        SAR_ADC_RAW_REG_WR( base, FTSAR_ADC_CR0, val);                                                                                                                    
        xain_num = 0;                                                                                                                                                     
    }                                                                                                                                                                     
    xain_change_timer.expires = jiffies + POLLING_DELAY;                                                                                                                  
    add_timer(&xain_change_timer);                                                                                                                           
}                                                                                                                                                                         
                                                                                                                                                                          
void Platform_Set_Init_Reg(struct dev_data *p_dev_data)                                                                                                                   
{                                                                                                                                                                         
    unsigned int val;                                                                                                                                                 
    unsigned int base = (unsigned int)p_dev_data->io_vadr;                                                                                                            
	                                                                                                                                                                  
    val = 0x00100000;/* set high threshold as 0x10 */                                                                                                                 
    SAR_ADC_RAW_REG_WR( base, FTSAR_ADC_THD0, val);                                                                                                                       
    val = 0x000E0000;/* set high threshold as 0x0E */                                                                                                                     
    SAR_ADC_RAW_REG_WR( base, FTSAR_ADC_THD1, val);                                                                                                                       
    val = 0x000B2021;                                                                                                                                                     
    SAR_ADC_RAW_REG_WR( base, FTSAR_ADC_CR0, val);                                                                                                                        
    udelay(50);                                                                                                                                                           
}                                                                                                                                                                         
                                                                                                                                                                          
int Platform_Init_Pollign_Timer(struct dev_data *p_dev_data)                                                                                                             
{                                                                                                                                                                         
/* Because xian 0 and xain 1 can't be selected at the same time, so we need to polling it */                                                                              
   init_timer(&xain_change_timer);                                                                                                                                        
   xain_change_timer.function = change_detect_xain;                                                                                                                       
   xain_change_timer.data = ((unsigned long) p_dev_data);                                                                                                                 
   xain_change_timer.expires = jiffies + POLLING_DELAY;                                                                                                                   
   add_timer(&xain_change_timer);
   return 0;
}                                                                                                                                                                         

int Platform_Get_Polling_Timer_State(void)
{
    return 1;
}

void Platform_Del_Pollign_Timer(void)                                                                                                                                     
{                                                                                                                                                                         
    del_timer(&xain_change_timer);                                                                                                                                    
}                                                                                                                                                                         
                                                                                                                                                                          
void Platform_Process_Volt_Threshold(struct dev_data *p_dev_data)                                                                                    
{                                                                                                                                                                         
    /*8287 do nothing*/

}  
int Platform_SAR_ADC_GetData(struct dev_data *p_dev_data , sar_adc_pub_data* new_data)
{    
    unsigned int base = (unsigned int)p_dev_data->io_vadr;
    unsigned int value;
    
    value=SAR_ADC_RAW_REG_RD(base,FTSAR_ADC_DAT0);
    if(value > 0x0) {
        new_data->adc_val = value;
        new_data->status = KEY_XAIN_0;
        return 0;
    }

    value=SAR_ADC_RAW_REG_RD(base,FTSAR_ADC_DAT1);
    if(value > 0x0) {
        new_data->adc_val = value;
        new_data->status = KEY_XAIN_1;
        return 0;
    }

    new_data->adc_val = 0;
    new_data->status = 0;
    return -1;
}
void Platform_Lock_Polling_Mutex(void)
{
    /*8287 do nothing*/    
}

void Platform_UnLock_Polling_Mutex(void)
{
    /*8287 do nothing*/  
}

int Platform_Check_Support_XGain(int num)
{
    if(num >=0 && num <= 1)
        return 0;
    return -1;    
}
unsigned int Platform_Direct_Get_XGain_Value(unsigned int base , int num)
{
    unsigned int value = 0;
    
    if(num == 0){
        value=SAR_ADC_RAW_REG_RD(base,FTSAR_ADC_DAT0);
        
    }

    if(num == 1){
        value=SAR_ADC_RAW_REG_RD(base,FTSAR_ADC_DAT1);

    }
    return value;
}

int Platform_Get_SARADC_Debug(void)
{
    return 0;
}


