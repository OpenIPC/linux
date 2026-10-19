/*
 * arch/arm/mach-anycloud/l2_exebuf.c
 *
 * Copyright (C) 2020 Anyka(Guangzhou) Microelectronics Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
*/

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/irqflags.h>
#include <mach/ak_l2_exebuf.h>
#include <mach/map.h>
#include <linux/io.h>
#include <asm-generic/gpio.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/of_platform.h>
#include <linux/suspend.h>
#include "ak_pm.h"
#include <asm/idmap.h>
#include <linux/delay.h>


//#define AK_PM_X2

//#define PM_DEBUG
#define L2_BUFFER0_SIZE     512

ktime_t l2_wakeup_time;

void (*jumpto_L2)(int status,unsigned long param1,unsigned long param2,
        unsigned long param3,unsigned long param4);

#if defined(CONFIG_MACH_KM01A) || defined(CONFIG_MACH_AK3918AV130)
    extern void ak_pm_save_data_in_mem(unsigned long param);
    extern unsigned int *ak_pm_resume_data_base;

    enum pm_mem_wakeup_mode{
        AK_PM_PWR_WAKEUP_MODE = 0x1,
        AK_PM_RTC_INT_WAKEUP_MODE,
        AK_PM_BUTTON_WAKEUP_MODE,
    };

    static int mem_wakeup_all_mode[]={
        AK_PM_PWR_WAKEUP,
        AK_PM_RTC_INT_WAKEUP,
        AK_PM_BUTTON_WAKEUP,
    };
#endif

/*
 * pm_wakeup_mode
 */
enum pm_wakeup_mode{
    AK_PM_WAKEUP_GPIO_MODE = 0x1,
    AK_PM_WAKEUP_RTC_MODE,
    AK_PM_WAKEUP_AIN0_MODE,
    AK_PM_WAKEUP_RTC_TIMER_MODE,
    AK_PM_WAKEUP_USB_DP_MODE,
};

/*
 * wakeup_all_mode
 */
static int wakeup_all_mode[]={
    AK_PM_GPIO_WAKEUP,
    AK_PM_RTC_WAKEUP,
    AK_PM_AIN0_WAKEUP,
    AK_PM_RTC_TIMER_WAKEUP,
    AK_PM_USB_DP_WAKEUP,
};

#if defined(CONFIG_MACH_KM01A) || defined(CONFIG_MACH_AK3918AV130)
/*
 * wakeup_all_gpio
 *
 */
static int wakeup_all_gpio[]={
    AK_PM_WAKEUP_GPIO71,
    AK_PM_WAKEUP_GPIO70,
     AK_PM_WAKEUP_GPIO2,
     AK_PM_WAKEUP_GPIO3,
    AK_PM_WAKEUP_GPIO10,
    AK_PM_WAKEUP_GPIO11,
    0,
    0,
    0,
    0,
    AK_PM_WAKEUP_GPIO24,
    AK_PM_WAKEUP_GPIO25,
    AK_PM_WAKEUP_GPIO26,
    AK_PM_WAKEUP_GPIO27,
    0,
    0,
    0,
    AK_PM_WAKEUP_GPIO37,
    0,
    0,
    0,
    0,
    AK_PM_WAKEUP_GPIO42,
    AK_PM_WAKEUP_GPIO44,
    AK_PM_WAKEUP_GPIO45,
    AK_PM_WAKEUP_GPIO48,
    AK_PM_WAKEUP_GPIO49,
    AK_PM_WAKEUP_GPIO50,
    AK_PM_WAKEUP_GPIO51,
    AK_PM_WAKEUP_GPIO52,

};

/*
 * wakeup_all_io
 *
 */
static int wakeup_all_io[]={
    AK_PM_WAKEUP_GPIO55,
    AK_PM_WAKEUP_GPIO56,
    0,
    AK_PM_WAKEUP_GPIO58,
    0,
    0,
    0,
    0,
    0,
    0,
    AK_PM_WAKEUP_GPIO65,
    AK_PM_WAKEUP_GPIO66,
    AK_PM_WAKEUP_GPIO67,
    AK_PM_WAKEUP_GPIO68,
    AK_PM_WAKEUP_GPIO69,
};
#else
/*
 * wakeup_all_gpio
 *
 */
static int wakeup_all_gpio[]={
    AK_PM_WAKEUP_GPIO71,
    AK_PM_WAKEUP_GPIO70,
     AK_PM_WAKEUP_GPIO2,
     AK_PM_WAKEUP_GPIO3,
    AK_PM_WAKEUP_GPIO10,
    AK_PM_WAKEUP_GPIO11,
    0,
    0,
    0,
    0,
    AK_PM_WAKEUP_GPIO24,
    AK_PM_WAKEUP_GPIO25,
    AK_PM_WAKEUP_GPIO26,
    AK_PM_WAKEUP_GPIO27,
    0,
    0,
    0,
    AK_PM_WAKEUP_GPIO37,
    0,
    0,
    0,
    0,
    AK_PM_WAKEUP_GPIO42,
    AK_PM_WAKEUP_GPIO44,
    AK_PM_WAKEUP_GPIO45,
    AK_PM_WAKEUP_GPIO48,
    AK_PM_WAKEUP_GPIO49,
    AK_PM_WAKEUP_GPIO50,
    AK_PM_WAKEUP_GPIO51,
    AK_PM_WAKEUP_GPIO52,

};

/*
 * wakeup_all_io
 *
 */
static int wakeup_all_io[]={
    AK_PM_WAKEUP_GPIO55,
    AK_PM_WAKEUP_GPIO56,
    0,
    AK_PM_WAKEUP_GPIO58,
    0,
    0,
    0,
    0,
    0,
    0,
    AK_PM_WAKEUP_GPIO65,
    AK_PM_WAKEUP_GPIO66,
    AK_PM_WAKEUP_GPIO67,
    AK_PM_WAKEUP_GPIO68,
    AK_PM_WAKEUP_GPIO69,
};

#endif


extern struct pm_wakeup_info ak_pm_info;

#define REG32(_register_) (*(volatile unsigned long*)(_register_))

/*
 * vcm23_reg_value
 *
 */
static unsigned int vcm23_reg_value=0;

/*
 * ak_wakeup_gpio_reg_set
 *
 */
static void ak_wakeup_io_reg_set(int gpio_regbit,int *pm_wakeup_gpio_edge,int gpio_index)
{
    unsigned long value;
    // AK_BUG("[%s line:%d] \n",__func__,__LINE__);
    /* clear wake-up status registers */
    value = __raw_readl(WAKE_UP_IO_TRIGGER_STA_CLR_REG);
    value |= (0x1 << gpio_regbit);
    __raw_writel(value, WAKE_UP_IO_TRIGGER_STA_CLR_REG);

    /* clear wake-up status registers */
    value = __raw_readl(WAKE_UP_IO_TRIGGER_STA_CLR_REG);
    value &= ~(0x1 << gpio_regbit);
    __raw_writel(value, WAKE_UP_IO_TRIGGER_STA_CLR_REG);

    /* set gpio is 1:falling-edge triggered , 0:rising 1:falling*/
    value = __raw_readl(WAKE_UP_IO_POLARITY_SEL_REG);
    if (pm_wakeup_gpio_edge[gpio_index]){
        value |= (0x1 << gpio_regbit); /* 1:falling edge */
    }else{
        value &= ~(0x1 << gpio_regbit);/* 0:rising edge */
    }
    __raw_writel(value, WAKE_UP_IO_POLARITY_SEL_REG);

    /* set gpio wake-up function enable, 1:enable, 0:disable */
    value = __raw_readl(WAKE_UP_IO_FUNC_ENA_REG);
    value |= (0x1 << gpio_regbit);
    __raw_writel(value, WAKE_UP_IO_FUNC_ENA_REG);


}

/*
 * ak_wakeup_io_reg_set
 *
 *
 */
static void ak_wakeup_gpio_reg_set(int gpio_regbit,int *pm_wakeup_gpio_edge,int gpio_index)
{
    unsigned long value;
    /* clear wake-up status registers */
    value = __raw_readl(WAKE_UP_GPIO_TRIGGER_STA_CLR_REG);
    value |= (0x1 << gpio_regbit);
    __raw_writel(value, WAKE_UP_GPIO_TRIGGER_STA_CLR_REG);
    
    /* clear wake-up status registers */
    value = __raw_readl(WAKE_UP_GPIO_TRIGGER_STA_CLR_REG);
    value &= ~(0x1 << gpio_regbit);
    __raw_writel(value, WAKE_UP_GPIO_TRIGGER_STA_CLR_REG);
    
    /* set gpio is 1:falling-edge triggered , 0:rising 1:falling*/
    value = __raw_readl(WAKE_UP_GPIO_POLARITY_SEL_REG);
    if (pm_wakeup_gpio_edge[gpio_index]){
        value |= (0x1 << gpio_regbit); /* 1:falling edge */
    }else{
        value &= ~(0x1 << gpio_regbit);/* 0:rising edge */
    }
    __raw_writel(value, WAKE_UP_GPIO_POLARITY_SEL_REG);
    
    /* set gpio wake-up function enable, 1:enable, 0:disable */
    value = __raw_readl(WAKE_UP_GPIO_FUNC_ENA_REG);
    value |= (0x1 << gpio_regbit);
    __raw_writel(value, WAKE_UP_GPIO_FUNC_ENA_REG);

    value = __raw_readl(WAKE_UP_GPIO_FUNC_ENA_REG); // flush 0x08000000

}

/*
 * ak_pm_gpio_wakeup_set
 *
 *
 */
static int ak_pm_gpio_wakeup_set(int *pm_wakeup_gpio,
    int *pm_wakeup_gpio_edge, int pm_wakeup_gpio_cnt)
{
    int i = 0;
    int gpio_regbit = 0;
    int ret = 0;
    int gpio_index = 0;

    /* check gpio range */
    for (gpio_index = 0; gpio_index < pm_wakeup_gpio_cnt; gpio_index++){
      if(ak_pm_info.wakeup_gpio_index[gpio_index] >= 55 && ak_pm_info.wakeup_gpio_index[gpio_index] <= 69){
        
          for (i=0; i<sizeof(wakeup_all_io)/sizeof((wakeup_all_io)[0]); i++){
            if(pm_wakeup_gpio[gpio_index] == (wakeup_all_io)[i]){
                gpio_regbit = i;
              //  AK_BUG("[%s line:%d]pm_wakeup_gpio[gpio_index]:%d \n",__func__,__LINE__,pm_wakeup_gpio[gpio_index]);
                break;
            }
          }

          if (i == sizeof(wakeup_all_io)){
              printk(KERN_ERR"%s , line:%d , pm wakeup gpio %d "
                  "set unknow err!\n",
                  __func__, __LINE__, pm_wakeup_gpio[gpio_index]);
              continue;
          }

            /* set GPIO sharepin as gpio ,enable pu, and input direction */
            if (gpio_direction_input(pm_wakeup_gpio[gpio_index])){
                printk(KERN_ERR"%s , line:%d , pm wakeup gpio %d "
                    "input direction set err!\n",
                    __func__, __LINE__, pm_wakeup_gpio[gpio_index]);
                continue;
            }

           /*wake_up io reg set*/
          ak_wakeup_io_reg_set(gpio_regbit,pm_wakeup_gpio_edge,gpio_index);

      }else{
        for (i=0; i<sizeof(wakeup_all_gpio)/sizeof(wakeup_all_gpio[0]); i++){
            if(pm_wakeup_gpio[gpio_index] == wakeup_all_gpio[i]){
                gpio_regbit = i;
                break;
            }
        }

        if (i == sizeof(wakeup_all_gpio)){
            printk(KERN_ERR"%s , line:%d , pm wakeup gpio %d "
                "set unknow err!\n",
                __func__, __LINE__, pm_wakeup_gpio[gpio_index]);
            continue;
        }

        /* set GPIO sharepin as gpio ,enable pu, and input direction */
        if (gpio_direction_input(pm_wakeup_gpio[gpio_index])){
            printk(KERN_ERR"%s , line:%d , pm wakeup gpio %d "
                "input direction set err!\n",
                __func__, __LINE__, pm_wakeup_gpio[gpio_index]);
            continue;
        }

          /*wake_up gpio reg set*/
         ak_wakeup_gpio_reg_set(gpio_regbit,pm_wakeup_gpio_edge,gpio_index);

      }

    }

    return ret;
}

/*
 * ak_pm_ain0_wakeup_set
 *
 */
static int ak_pm_ain0_wakeup_set(int wakeup_ain0_edge)
{
    /*
     * 2021-07-29 yeguohong
     *
     * ain0 wakeup voltage from EVB & SVT board:
     * falling-edge: from 3.3V down to 1.74V then wakeup, so range [0, 1.74];
     * rising-edge: from 0V up to 1.85V then wakeup, so range [1.85, MAX],
     *              but the lowest static voltage is 1.74V.
     */
    unsigned long value;

    pr_debug("%s\n",__func__);

    //disable the ain0 wakeup digital function
    value = __raw_readl(OTHER_WAKE_UP_SRC_CTRL_REG);
    value &= (~(1<<11));
    __raw_writel(value, OTHER_WAKE_UP_SRC_CTRL_REG);

    //disable the ain0 wakeup analog
    value = __raw_readl(AUDIO_CODEC_CFG1_REG);
    value &= (~(1<<8));
    __raw_writel(value, AUDIO_CODEC_CFG1_REG);

    //clear AIN0 wakeup status
    value = __raw_readl(OTHER_WAKE_UP_SRC_CTRL_REG);
    value |= (1<<6);
    __raw_writel(value, OTHER_WAKE_UP_SRC_CTRL_REG);
    value = __raw_readl(OTHER_WAKE_UP_SRC_CTRL_REG);
    value &= (~(1<<6));
    __raw_writel(value, OTHER_WAKE_UP_SRC_CTRL_REG);

    //config the AIN0 wakeup trigger
    value = __raw_readl(OTHER_WAKE_UP_SRC_CTRL_REG);
    value &= (~(1<<1));
    value |= ((wakeup_ain0_edge ? 1:0)<<1);
    __raw_writel(value, OTHER_WAKE_UP_SRC_CTRL_REG);

    //enable the ain0 wakeup analog
    value = __raw_readl(AUDIO_CODEC_CFG1_REG);
    value |= (1<<8);
    __raw_writel(value, AUDIO_CODEC_CFG1_REG);

    //enable the ain0 wakeup digital funciton
    value = __raw_readl(OTHER_WAKE_UP_SRC_CTRL_REG);
    value |= (1<<11);
    __raw_writel(value, OTHER_WAKE_UP_SRC_CTRL_REG);

    value = __raw_readl(OTHER_WAKE_UP_SRC_CTRL_REG); // flush 0x08000000
    return 0;
}

/*
 * ak_pm_rtc_timer_wakeup_set
 *
 */
static int ak_pm_rtc_alarm_wakeup_set(void)
{
    unsigned long timeout = 10000;
    unsigned long val;

    /* 1st: set bit[7] of Other Source Wake-up Control Register to 0X1 */
    val = __raw_readl(OTHER_WAKE_UP_SRC_CTRL_REG);
    val |= (0x1<<7);
    __raw_writel(val, OTHER_WAKE_UP_SRC_CTRL_REG);

    /* 2nd: make sure that Other Source Wake-up Status Register is 0 */
    while (__raw_readl(OTHER_WAKE_UP_SRC_TRIGGER_STA_REG) & 0x4) {
        timeout--;
        udelay(10);
        if(0 == timeout ) return -1;
    }

    /* 3rd: clear bit[7] of Other Source Wake-up Control Register to 0x0 */
    val = __raw_readl(OTHER_WAKE_UP_SRC_CTRL_REG);
    val &= ~(1<<7);
    __raw_writel(val, OTHER_WAKE_UP_SRC_CTRL_REG);

    /* 4rd: unmask bit[12] of Other Source Wake-up Control Register to 0x0 */
    val = __raw_readl(OTHER_WAKE_UP_SRC_CTRL_REG);
    val |= (0x1 << 12);
    __raw_writel(val, OTHER_WAKE_UP_SRC_CTRL_REG);

    val = __raw_readl(OTHER_WAKE_UP_SRC_CTRL_REG); // flush 0x08000000
    return 0;
}

/*
 * ak_pm_rtc_timer_wakeup_set
 *
 */
static int ak_pm_rtc_timer_wakeup_set(void)
{
    unsigned long timeout = 10000;
    unsigned long val;

    /* 1st: set bit[8] of Other Source Wake-up Control Register to 0X1 */
    val = __raw_readl(OTHER_WAKE_UP_SRC_CTRL_REG);
    val |= (0x1<<8);
    __raw_writel(val, OTHER_WAKE_UP_SRC_CTRL_REG);

    /* 2nd: make sure that Other Source Wake-up Status Register is 0 */
    while (__raw_readl(OTHER_WAKE_UP_SRC_TRIGGER_STA_REG) & 0x8) {
        timeout--;
        udelay(10);
        if(0 == timeout ) return -1;
    }

    /* 3rd: clear bit[8] of Other Source Wake-up Control Register to 0x0 */
    val = __raw_readl(OTHER_WAKE_UP_SRC_CTRL_REG);
    val &= ~(1<<8);
    __raw_writel(val, OTHER_WAKE_UP_SRC_CTRL_REG);

    /* 4rd: unmask bit[13] of Other Source Wake-up Control Register to 0x0 */
    val = __raw_readl(OTHER_WAKE_UP_SRC_CTRL_REG);
    val |= (0x1 << 13);
    __raw_writel(val, OTHER_WAKE_UP_SRC_CTRL_REG);

    val = __raw_readl(OTHER_WAKE_UP_SRC_CTRL_REG); // flush 0x08000000
    return 0;
}

/*
 * dts_io_wake_up
 *
 */
void dts_io_wake_up(void)
{
    int i;
    int ret = 0;
    struct device_node *soc_np;
    struct device_node *child_np;
    int wakeup_io;
    int edge;
    const char *status;
    struct platform_device *pdev;
    int exist =0;

    soc_np = of_find_node_by_path("/soc");
    for_each_child_of_node(soc_np, child_np)
    {
        status = of_get_property(child_np, "status",NULL);
        if(!status || strcmp(status,"okay") != 0)
            continue;

        if(strncmp(child_np->name, "mmc",3) != 0){
          pdev = of_find_device_by_node(child_np);
          if(pdev){
            //AK_BUG("[%s line:%d]pdev->name:%s \n",__func__,__LINE__,pdev->name);
            if(!pdev->dev.driver)
               continue;
          }
        }

        if(of_property_read_bool(child_np, "wakeup_flag"))
        {
            wakeup_io = of_get_named_gpio(child_np , "wakeup-gpio", 0);
            ret=of_property_read_u32(child_np, "wakeup-gpio-edge",&edge);

            for(i=0;i<sizeof(ak_pm_info.wakeup_gpio_index)/sizeof(ak_pm_info.wakeup_gpio_index[0]);i++){
                 if(wakeup_io == ak_pm_info.wakeup_gpio_index[i]){
                     exist =1;
                     break;
                 }
            }

            if(!exist){
                 ak_pm_info.wakeup_gpio_index[ak_pm_info.wakeup_gpio_cnt] = wakeup_io;
                 ak_pm_info.wakeup_gpio_edge[ak_pm_info.wakeup_gpio_cnt] =edge;
                 ak_pm_info.wakeup_gpio_cnt++;
            }

            wakeup_io = 0;
            edge =0;
        }
    }

#ifdef AK_PM_BUG
        for(i=0;i<ak_pm_info.wakeup_gpio_cnt;i++)
        {
            AK_BUG("[%s line:%d] ak_pm_info.wakeup_gpio_index[%d]:%d  "\
                   "ak_pm_info.wakeup_gpio_edge[%d]: %d \n",__func__,__LINE__,
                   i,ak_pm_info.wakeup_gpio_index[i],
                   i,ak_pm_info.wakeup_gpio_edge[i]);
        }
#endif
}

/*
 * ak_pm_wakeup_src_cfg
 *
 */
static int ak_pm_wakeup_src_cfg(void)
{
    int index = 0;
    int cnt = 0;
    int ret = 0;

    /* 1.check wakup mode range */
    for (cnt=0; cnt<ak_pm_info.wakeup_mode_cnt; cnt++){
            for (index=0;
                index<sizeof(wakeup_all_mode)/sizeof(wakeup_all_mode[0]);
                index++){
                if(ak_pm_info.wakeup_mode[cnt]== wakeup_all_mode[index]){
                    break;
                }
            }

            if (index == sizeof(wakeup_all_mode)){
                printk(KERN_ERR"%s , line:%d , pm wakeup mode err:%d!\n",
                    __func__, __LINE__, ak_pm_info.wakeup_mode[cnt]);
                ret = -1;
                return ret;
            }
    }

    /* 2.cfg every wakeup triger */
    for (index=0; index<ak_pm_info.wakeup_mode_cnt; index++){
        switch(ak_pm_info.wakeup_mode[index]){
            case AK_PM_WAKEUP_GPIO_MODE:

                dts_io_wake_up();

                ret = ak_pm_gpio_wakeup_set(&ak_pm_info.wakeup_gpio_index[0],
                            &ak_pm_info.wakeup_gpio_edge[0],
                            ak_pm_info.wakeup_gpio_cnt);

                if (ret < 0){
                    printk(KERN_ERR"%s , line:%d , pm wakeup mode %d "
                        "set err!\n",
                        __func__, __LINE__, ak_pm_info.wakeup_mode[index]);
                }
                break;
            case AK_PM_WAKEUP_AIN0_MODE:
                ret = ak_pm_ain0_wakeup_set(ak_pm_info.wakeup_ain0_edge);
                if (ret < 0){
                    printk(KERN_ERR"%s , line:%d , pm wakeup mode %d "
                        "set err!\n", __func__, __LINE__,
                        ak_pm_info.wakeup_mode[index]);
                }
                break;
            case AK_PM_WAKEUP_RTC_MODE:
                ret = ak_pm_rtc_alarm_wakeup_set();
                if (ret < 0){
                    printk(KERN_ERR"%s , line:%d , pm wakeup mode %d "
                        "set err!\n",
                        __func__, __LINE__, ak_pm_info.wakeup_mode[index]);
                }
                break;
            case AK_PM_WAKEUP_RTC_TIMER_MODE:
                ret = ak_pm_rtc_timer_wakeup_set();
                if (ret < 0){
                    printk(KERN_ERR"%s , line:%d , pm wakeup mode %d "
                        "set err!\n",
                        __func__, __LINE__, ak_pm_info.wakeup_mode[index]);
                }
                break;
            default:
                printk(KERN_ERR"%s , line:%d , pm wakeup mode %d"
                    "set err!\n",
                    __func__, __LINE__, ak_pm_info.wakeup_mode[index]);
                ret = -1;
        }

        if (ret < 0){
            printk(KERN_ERR"%s , line:%d , pm wakeup mode %d "
                "set err!\n",
                __func__, __LINE__, ak_pm_info.wakeup_mode[index]);
            break;
        }
    }

    //printk(KERN_ERR"%s , line:%d , pm wakeup mode set ok!\n",

    return ret;
}

/*
 * ak_pm_pmc_wakeup_src_cfg
 *
 */
static __maybe_unused void ak_pm_pmc_wakeup_src_cfg(void)
{
    int index = 0;
    int cnt = 0;
    //int ret = 0;

     //pwr_td0设置
    if(pmc_cfg_pwr_td0(ak_pm_info.mem_pwr_td0) < 0){
        pr_err("Err: pmc_cfg_pwr_td0 config fail.\n");
    }

    //回读确认是否pwr_td0正常设置
    if(pmc_read_pwr_td0() != ak_pm_info.mem_pwr_td0){
        pr_err("Err: pwr_td0 config fail.\n");
    }

    /* 1.check wakup mode range */
    for (cnt=0; cnt<ak_pm_info.mem_wakeup_mode_cnt; cnt++){
            for (index=0;
                index<sizeof(mem_wakeup_all_mode)/sizeof(mem_wakeup_all_mode[0]);
                index++){
                if(ak_pm_info.mem_wakeup_mode[cnt]== mem_wakeup_all_mode[index]){
                  //printk("wakeup_mode[%d]=%d\n",cnt,ak_pm_info.wakeup_mode[cnt]);
                    break;
                }
            }

            if (index == sizeof(mem_wakeup_all_mode)){
                printk(KERN_ERR"%s , line:%d , pm wakeup mode err:%d!\n",
                    __func__, __LINE__, ak_pm_info.mem_wakeup_mode[cnt]);

            }
    }

    /* 2.cfg every wakeup triger */
    for (index=0; index<ak_pm_info.mem_wakeup_mode_cnt; index++){
        // printk("ak_pm_info.wakeup_mode[%d]:%d\n",index,ak_pm_info.wakeup_mode[index]);
        switch(ak_pm_info.mem_wakeup_mode[index]){

            case AK_PM_PWR_WAKEUP_MODE:

               if(ak_pm_info.mem_pwr_wakeup_edge){
                  pmc_power_wakeup_cfg(PMC_PWR_WAKEUP_HIGH_LEVEL_TRIG);
               }else{
                   //pmc_power_wakeup_cfg(PMC_PWR_WAKEUP_LOW_LEVEL_TRIG);
                   pmc_power_wakeup_cfg(PMC_PWR_WAKEUP_FALLING_EDGE_TRIG);
               }
               pmc_power_wakeup_enable();
               //pmc_read_all_reg_val();
            break;

            case AK_PM_RTC_INT_WAKEUP_MODE:
                pmc_rtc_int_wakeup_enable();
                //pmc_read_all_reg_val();

            break;

            case AK_PM_BUTTON_WAKEUP_MODE:
                   pr_info("Input poweron button press time(1~16383) is %d, unit: ms \n",ak_pm_info.mem_pwr_button_time);
                   pmc_poweron_button_press_time_cfg(ak_pm_info.mem_pwr_button_time);
                   if( pmc_read_poweron_button_press_time() != ak_pm_info.mem_pwr_button_time){
                       pr_err("Err: poweron_button_time config fail.\n");
                   }
            break;
        }
    }
}

/**
 * __disable_all_wakeup - disabled all wakeup source
 */
static void __disable_all_wakeup(void)
{
    /* disable all wakeup source */
    REG32(OTHER_WAKE_UP_SRC_CTRL_REG) &= ~(0x1f << 10);
    REG32(WAKE_UP_GPIO_FUNC_ENA_REG) = 0;
#if 0
    //sensor first poweron
    if(ak_pm_info.wakeup_gpio_index[1]>0)
        gpio_direction_output(ak_pm_info.wakeup_gpio_index[1], 1);//jiucloud
    if(ak_pm_info.wakeup_gpio_index[2]>0)
        gpio_direction_output(ak_pm_info.wakeup_gpio_index[2], 1);
#endif
    //gpio_direction_output(1, 1);//jiucloud
    //gpio_direction_output(48, 1);//km01d_evb
    //gpio_direction_output(49, 1);
}

/**
 * __pm_get_wake_up_source - get wakeup source
 */
#if 0
static int __pm_get_wake_up_source(void)
{
    unsigned int reg;
    int ret;
    ret = -1;
    reg = REG32(OTHER_WAKE_UP_SRC_TRIGGER_STA_REG);
    if (reg) {
        if (reg & 0x1 << 0)
            ret = 0;//WAKEUP_AIN1; // ain1 wakeup
        if (reg & 0x1 << 1)
            ret = 1;//WAKEUP_AIN0; // ain0 wakeup
        if (reg & 0x1 << 2)
            ret = 2;//WAKEUP_RTC_ALARM; // rtc alarm wakeup
        if (reg & 0x1 << 3)
            ret = 3;//WAKEUP_RTC_TIMER; // rtc timer wakeup
        if (reg & 0x1 << 4)
            ret = 4;//WAKEUP_USB_DP; // usb wakeup
    }
    printk("s:%d\n",ret);
    return ret;
}
#endif

/*
 * __pm_resume
 *
 */
static void __pm_resume(void)
{
    unsigned int tmp;

    //standby out flag
    printk("s:0\n");

   // __pm_get_wake_up_source();
    tmp = (vcm23_reg_value >> 0) & (0x6);
    REG32(AK_VA_SYSCTRL + 0x009c) &= ~(0x6 << 0);
    REG32(AK_VA_SYSCTRL + 0x009c) |= (tmp << 0);
}

/*
 * ak_pm_resume
 *
 */
static int ak_pm_resume(int pm_state)
{
    if (pm_state == PM_SUSPEND_STANDBY){
        __pm_resume();
        __disable_all_wakeup();
    }else if(pm_state == PM_SUSPEND_MEM) {
#if defined(CONFIG_MACH_KM01A)
        pmc_rtc_int_wakeup_disable();
#endif

    }

    return 0;
}

/*
 * pm_print_info
 *
 */
static void pm_print_info(const char *start, int len)
{
#ifdef PM_DEBUG
    int i;
    unsigned char local_l2mem[L2_BUFFER0_SIZE] = {0} ;

    printk("start = %p, len = %d\n", start, len);
    for (i = 0; i < len; i += 4) {
        *(unsigned long *)(local_l2mem + i) =
            *(volatile unsigned long *)(AK_VA_L2MEM + i);
    }
    for (i = 0; i < len; i++) {
        printk(" 0x%02x", local_l2mem[i]);
        if (i % 16 == 15) printk("\n");
    }
#endif
}


/* L2 buf0 */
#define L2_BUF0_COMBUF_REQ_MAP      (0xfful << 24)
#define L2_BUF0_UARTBUF_REQ_MAP     (0x3ful << 16)
#define L2_BUF0_FRACDMA_REQ_MAP     (0x1ul << 9)

static int check_l2_running_status(void)
{
    unsigned int reg32, map32;
    int retval = 0;
    int i;

    /* check L2 buffer0 */
    reg32 = __raw_readl(AK_VA_L2CTRL + 0x80);
    map32 = L2_BUF0_COMBUF_REQ_MAP | L2_BUF0_UARTBUF_REQ_MAP | \
        L2_BUF0_FRACDMA_REQ_MAP;
    if(reg32 & map32){
        retval = -1;
         printk(KERN_EMERG"l2 buf0 is running(reg %08x, map %08x, check %08x)\n",
            reg32, map32, reg32 & map32);
        for(i = 0; i < 32; i++){
            if(!((0x1ul << i) & map32))
                continue;
            switch(i){
                /* common buffer */
                case 24:
                case 25:
                case 26:
                case 27:
                case 28:
                case 29:
                case 30:
                case 31:
                     printk(KERN_EMERG"buff%d\t %s\n", i-24,
                        ((reg32 >> i) & 0x1)? "running":"stop");
                    break;
                /* uart buffer */
                case 16:
                case 17:
                case 18:
                case 19:
                case 20:
                case 21:
                     printk(KERN_EMERG"uart%d\t %s\t %s\n", (i-16)/2,
                        ((reg32 >> i) & 0x1)? "running":"stop",
                        ((i-16)%2)? "rx":"tx");
                    break;
                /* frac transportion */
                case 9:
                     printk(KERN_EMERG"frac \t %s\n", ((reg32 >> i) & 0x1)? "running":"stop");
                    break;
            }
        }
        /* display buffer asssignment */
         reg32 = REG32(AK_VA_L2CTRL + 0x90);
         printk(KERN_EMERG"L2 buf0 assignment\n");
         printk(KERN_EMERG"dac   bufnum: %d\n", (reg32 >> 27) & 0x7);
         printk(KERN_EMERG"spi0r bufnum: %d\n", (reg32 >> 24) & 0x7);
         printk(KERN_EMERG"spi0w bufnum: %d\n", (reg32 >> 21) & 0x7);
         printk(KERN_EMERG"mci2  bufnum: %d\n", (reg32 >> 18) & 0x7);
         printk(KERN_EMERG"mci1  bufnum: %d\n", (reg32 >> 15) & 0x7);
         printk(KERN_EMERG"mci0  bufnum: %d\n", (reg32 >> 12) & 0x7);
         reg32 = REG32(AK_VA_L2CTRL + 0x94);
         printk(KERN_EMERG"adc   bufnum: %d\n", (reg32 >> 12) & 0x7);
    }

    return retval;
}
/* end of func */

/*
 * copy from ddr2 to l2 memory to run, and exit standby
 */

#if defined(CONFIG_MACH_KM01A) || defined(CONFIG_MACH_AK3918AV130)

typedef void (*ak_pm_suspend_in_mem_t)(unsigned long param1,
        unsigned long param2, unsigned long param3, unsigned long param4);
extern void ak_pm_suspend_in_mem(unsigned long param1,
        unsigned long param2, unsigned long param3, unsigned long param4);
extern phys_addr_t __pm_resume_data_pbase;
#endif

/*
 * l2_exec_buf
 *
 */
int l2_exec_buf(const char *vaddr, int len,int pm_state, unsigned long param1,
unsigned long param2,unsigned long param3,unsigned long param4)
{
    unsigned long i, flags ;
    int sum1 = 0, sum2 = 0, value;
    char *ptr;

#if defined(CONFIG_MACH_KM01A)
    //u32 reg_val;
    ak_pm_suspend_in_mem_t __suspend;
#endif

    if(check_l2_running_status() != 0){
        printk(KERN_EMERG"l2 dma is running now, entry suspend failed\n");
    }

    /* disable ARM interrupt */
    local_irq_save(flags);

    /*disable all clock expect DDR2 & L2BUF0 & uart0*/
    value = (~((0x1<<7) | (0x1<<9) | (0x3<<24)));
    __raw_writel(value, AK_VA_SYSCTRL + 0x1C);
    /*disable clock of enc/npu/isp*/
    value = __raw_readl(AK_VA_SYSCTRL + 0x11C);
    value &= ~((0x1<<22) | (0x1<<14) | (0x1<<6) );
    __raw_writel(value, AK_VA_SYSCTRL + 0x11C);
    /*HOLD all reset except DDR2 & L2BUF0 & uart0*/
    value = (~((0x1<<7) | (0x1<<9) | (0x3<<24)));
    __raw_writel(value, AK_VA_SYSCTRL + 0x20);
    /*HOLD reset of enc/npu/isp*/
    value = __raw_readl(AK_VA_SYSCTRL + 0x11C);
    value |= ((0x1<<23) | (0x1<<15) | (0x1<<7));
    __raw_writel(value, AK_VA_SYSCTRL + 0x11C);
    __raw_readl(AK_VA_SYSCTRL);

    memset((void *)AK_VA_L2MEM, 0, L2_BUFFER0_SIZE);

    /* copy from ddr2 to l2 memory */
    for (i = 0; i < len; i += 4) {
        *(volatile unsigned long *)(AK_VA_L2MEM + i) =
            *(unsigned long *)(vaddr + i);
    }

    if(!param1){ //reboot时param1置1
        pm_print_info(vaddr, len);
        if (pm_state == PM_SUSPEND_STANDBY){
            if (ak_pm_wakeup_src_cfg() < 0){
                printk(KERN_ERR"%s , line:%d , pm wakeup mode set ERR!\n",
                    __func__, __LINE__);
                return -1;
            }
        }else if (pm_state == PM_SUSPEND_MEM){
            #if  defined(CONFIG_MACH_KM01A)
                    ak_pm_pmc_wakeup_src_cfg();
                 pr_info("now start linux enter mem ( ddr selfrefresh and pmc power down) .......\n");
                 #if 0
                 /*
                 在DDR2上直接做以下实验： 按Button 上电启动-> 
                 调用powersavebootcfg -> 软件设置掉电 
                 -> 按button 
                 */
                 PowerSaveBootCfg();
                 pr_err("%s , line:%d n",__func__, __LINE__);

                 pmc_software_poweroff();
                 
                 while(1);
                 #endif
            #endif
        }
    }
    __raw_readl(AK_VA_SYSCTRL);
    /* disable ahb_flag_en */
    *(volatile unsigned long *)(AK_VA_L2CTRL + 0x84) &= ~(1 << 29);

    /*
     *  Perform checksum on data in DDR and L2 to ensure data consistency.
     */
    sum1 = 0;
    sum2 = 0;
    value = 0;
    ptr = (char *)vaddr;
    for (i = 0; i < len; i++) {
        sum1 += ptr[i];
    }
    ptr = AK_VA_L2MEM;
    for (i = 0; i < len; i++) {
        sum2 += ptr[i];
    }

    do {
        value = __raw_readl(AK_VA_SUBCTRL + 0x30018);
    } while (value & (0x1 << 2));

    if (sum1 == sum2) {
        // printk(KERN_ERR"%s sum1[0x%x]\n", __func__, sum1);
        /*
         *  data consistency, print 'Y'
         */
        __raw_writel(0x59 << 16, AK_VA_SUBCTRL + 0x3000c);
    } else {
        /*
         *  data inconsistency, print 'N'
         */
        __raw_writel(0x4E << 16, AK_VA_SUBCTRL + 0x3000c);
    }

    /* jumpto_L2 run */
    jumpto_L2 = (void *)(AK_VA_L2MEM);
#ifndef AK_PM_X2
     //param1被置为1是表示reboot
    if (pm_state == PM_SUSPEND_STANDBY || param1){
        jumpto_L2(pm_state,param1,param2,param3,param4); 
        // gpio_on_off_on(1);
    }else  if (pm_state == PM_SUSPEND_MEM){
        #if  defined(CONFIG_MACH_KM01A)
            __raw_readl(AK_VA_SYSCTRL);
            ak_pm_save_data_in_mem((unsigned long)ak_pm_resume_data_base);
            setup_mm_for_reboot(); 
            __suspend = (ak_pm_suspend_in_mem_t)(unsigned long)virt_to_idmap(ak_pm_suspend_in_mem);
            //汇编里运行L2 执行掉电操作
            __suspend((unsigned long)__pm_resume_data_pbase, 0, 0, 0);
            //cpu初始化
             cpu_init();
            //唤醒后返回这里
            pmc_clear_poweroff_bit();
        #endif
     }
#endif
    /* enable ahb_flag_en */
    *(volatile unsigned long *)(AK_VA_L2CTRL + 0x84) |= (1 << 29);

#ifndef AK_PM_X2
    ak_pm_resume(pm_state);
#endif

    /* enable ARM interrupt */
    local_irq_restore(flags);

    return 0;
}
/*end of file*/
