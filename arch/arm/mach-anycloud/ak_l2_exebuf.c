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


//#define PM_DEBUG
#define L2_BUFFER0_SIZE     512

ktime_t l2_wakeup_time;

void (*jumpto_L2)(unsigned long param1,unsigned long param2,
        unsigned long param3,unsigned long param4);

#ifdef CONFIG_PM
enum pm_wakeup_mode{
    AK_PM_WAKEUP_GPIO_MODE = 0x1,
    AK_PM_WAKEUP_RTC_MODE,
    AK_PM_WAKEUP_AIN0_MODE,
    AK_PM_WAKEUP_RTC_TIMER_MODE,
    AK_PM_WAKEUP_USB_DP_MODE,
};

static int wakeup_all_mode[]={
    AK_PM_GPIO_WAKEUP,
    AK_PM_RTC_WAKEUP,
    AK_PM_AIN0_WAKEUP,
    AK_PM_RTC_TIMER_WAKEUP,
    AK_PM_USB_DP_WAKEUP,
};

static int wakeup_all_gpio[]={
    AK_PM_WAKEUP_GPIO2,
    AK_PM_WAKEUP_GPIO8,
    AK_PM_WAKEUP_GPIO9,
    AK_PM_WAKEUP_GPIO16,
    AK_PM_WAKEUP_GPIO20,
    AK_PM_WAKEUP_GPIO22,
    AK_PM_WAKEUP_GPIO24,
    AK_PM_WAKEUP_GPIO28,
    AK_PM_WAKEUP_GPIO31,
    AK_PM_WAKEUP_GPIO46,
    AK_PM_WAKEUP_GPIO49,
    AK_PM_WAKEUP_GPIO51,
    AK_PM_WAKEUP_GPIO53,
    AK_PM_WAKEUP_GPIO55,
    AK_PM_WAKEUP_GPIO56,
    AK_PM_WAKEUP_GPIO61,
    AK_PM_WAKEUP_GPIO67,
    AK_PM_WAKEUP_GPIO68,
    AK_PM_WAKEUP_GPIO70,
    AK_PM_WAKEUP_GPIO72,
    AK_PM_WAKEUP_GPIO75,
    AK_PM_WAKEUP_GPIO78,
    AK_PM_WAKEUP_GPIO84,
    AK_PM_WAKEUP_GPIO85,
    AK_PM_WAKEUP_GPIO86,
};

extern struct pm_wakeup_info ak_pm_info;


static int ak_pm_gpio_wakeup_set(int *pm_wakeup_gpio,
    int *pm_wakeup_gpio_edge, int pm_wakeup_gpio_cnt)
{
    int i = 0;
    int gpio_regbit = 0;
    unsigned long value;
    int ret = 0;
    int gpio_index = 0;

    /* check gpio range */
    for (gpio_index = 0; gpio_index < pm_wakeup_gpio_cnt; gpio_index++){
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

    }

    return ret;
}

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

    return 0;
}

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

    return 0;
}

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

    return 0;
}

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
    //    __func__, __LINE__);

    return ret;
}
#endif

#ifdef PM_DEBUG
static void pm_print_info(const char *start, int len)
{

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

}
#endif

/*
 * copy from ddr2 to l2 memory to run, and exit standby
 */
int l2_exec_buf(const char *vaddr, int len, unsigned long param1,
unsigned long param2,unsigned long param3,unsigned long param4)
{
    unsigned long i, flags ;

    /* disable ARM interrupt */
    local_irq_save(flags);

    memset((void *)AK_VA_L2MEM, 0, L2_BUFFER0_SIZE);

    /* copy from ddr2 to l2 memory */
    for (i = 0; i < len; i += 4) {
        *(volatile unsigned long *)(AK_VA_L2MEM + i) =
            *(unsigned long *)(vaddr + i);
    }
#ifdef CONFIG_PM
    pm_print_info(vaddr, len);

    if (ak_pm_wakeup_src_cfg() < 0){
        printk(KERN_ERR"%s , line:%d , pm wakeup mode set ERR!\n",
            __func__, __LINE__);
        return -1;
    }
#endif
    /* disable ahb_flag_en */
    *(volatile unsigned long *)(AK_VA_L2CTRL + 0x84) &= ~(1 << 29);

    /* jumpto_L2 run */
    jumpto_L2 = (void *)(AK_VA_L2MEM);

    jumpto_L2(param1,param2,param3,param4);

    /* enable ahb_flag_en */
    *(volatile unsigned long *)(AK_VA_L2CTRL + 0x84) |= (1 << 29);

    /* enable ARM interrupt */
    local_irq_restore(flags);

    return 0;
}
/*end of file*/
