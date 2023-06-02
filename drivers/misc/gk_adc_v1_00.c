/*
 * drivers/misc/gk_adc_v1_00.c
 *
 * Author: Steven Yu, <yulindeng@gokemicro.com>
 * Copyright (C) 2012-2015, goke, Inc.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#include <mach/hardware.h>
#include <mach/io.h>
#include CONFIG_GK_CHIP_INCLUDE_FILE


#include <linux/clk.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/miscdevice.h>


#define DEVICE_NAME "adc"

/*----------------------------------------------------------------------------*/
/* registers                                                                  */
/*----------------------------------------------------------------------------*/
#define ADC_VA_BASE             (GK_VA_ADC2)
#define ADC_REG(x)              (ADC_VA_BASE + (x))

#define ADC_ANALOG_VA_BASE      (GK_VA_ADC)
#define ADC_ANALOG_REG(x)       (ADC_ANALOG_VA_BASE + (x))

#define REG_ADC_AUX_ATOP        ADC_ANALOG_REG(0x00) /* read/write */
#define REG_ADC_GPIO            ADC_ANALOG_REG(0x04) /* read/write */
#define REG_ADC_CONTROL         ADC_REG(0x000) /* read/write */
#define REG_ADC_READDATA        ADC_REG(0x004) /* read */
#define REG_ADC_ENABLE          ADC_REG(0x018) /* read/write */
#define REG_ADC_INTCONTROL      ADC_REG(0x044) /* read/write */

#define GK_ADC_IOC_MAGIC        'A'
#define IOC_ADC_SET_CHANNEL     _IOW(GK_ADC_IOC_MAGIC, 0, unsigned int)
#define IOC_ADC_GET_CHANNEL     _IOR(GK_ADC_IOC_MAGIC, 1, unsigned int)
#define IOC_ADC_GET_DATA        _IOR(GK_ADC_IOC_MAGIC, 2, unsigned int)
/*---------------------------------------------------------------------------*/
/* types, enums and structures                                               */
/*---------------------------------------------------------------------------*/
/*!
*******************************************************************************
**
** \brief ADC channel number.
**
** \sa    ADC_OPEN_PARAMS_S
**
******************************************************************************/
typedef enum
{
    ADC_CHANNEL_ONE = 0,    //!< ADC channel 1.
    ADC_CHANNEL_TWO,        //!< ADC channel 2.
    ADC_CHANNEL_COUNT,      //!< ADC channel count
    ADC_CHANNEL_NONE = -1,
}ADC_CHANNEL_E;

/*----------------------------------------------------------------------------*/
/* bit group structures                                                       */
/*----------------------------------------------------------------------------*/
typedef union { /* ADC_AUX_ATOP */
    uint32_t all;
    struct {
        uint32_t sar_maxsel                  : 4;
        uint32_t sar_maxnsel                 : 3;
        uint32_t sar_pd                      : 1;
        uint32_t sar_oneshot                 : 1;
        uint32_t sar_freerun                 : 1;
        uint32_t sar_refnsel                 : 2;
        uint32_t sar_refsel                  : 3;
        uint32_t sar_pd_tsi                  : 1;
        uint32_t                             : 16;
    } bitc;
} ADC_AUX_ATOP_S;

typedef union { /* ADC_GPIO */
    uint32_t all;
    struct {
        uint32_t sar_test                    : 4;
        uint32_t sar_key_pge                 : 4;
        uint32_t sar_key_aie                 : 4;
        uint32_t oen_sar_key                 : 4;
        uint32_t                             : 16;
    } bitc;
} ADC_GPIO_S;

typedef union { /* ADC_Control */
    uint32_t all;
    struct {
        uint32_t status                      : 1;
        uint32_t start                       : 1;
        uint32_t                             : 1;
        uint32_t channel                     : 3;
        uint32_t                             : 26;
    } bitc;
} ADC_CONTROL_S;


/*---------------------------------------------------------------------------*/
/* ADC state structure                                                             */
/*---------------------------------------------------------------------------*/
typedef struct
{
    ADC_CHANNEL_E channel;
    uint32_t data;
} ADC_STATE_MACHINE_S;

static struct mutex adc_mutex;
static uint32_t in_use;
static ADC_CHANNEL_E cur_ch;

void ADC_set_Control(uint32_t data)
{
    gk_adc_writel(REG_ADC_CONTROL, data);
}

uint32_t ADC_get_Control_status(void)
{
    ADC_CONTROL_S d;
    d.all = gk_adc_readl(REG_ADC_CONTROL);
    return d.bitc.status;
}

void ADC_set_Control_status(uint8_t data)
{
    ADC_CONTROL_S d;
    d.all = gk_adc_readl(REG_ADC_CONTROL);
    d.bitc.status = data;
    gk_adc_writel(REG_ADC_CONTROL, d.all);
}

void ADC_set_Control_start(uint8_t data)
{
    ADC_CONTROL_S d;
    d.all = gk_adc_readl(REG_ADC_CONTROL);
    d.bitc.start = data;
    gk_adc_writel(REG_ADC_CONTROL, d.all);
}

void ADC_set_Control_channel(uint8_t data)
{
    ADC_CONTROL_S d;
    d.all = gk_adc_readl(REG_ADC_CONTROL);
    d.bitc.channel = data;
    gk_adc_writel(REG_ADC_CONTROL, d.all);
}

static uint32_t  ADC_get_ReadData(uint8_t index)
{
    uint32_t value;
    value = gk_adc_readl(REG_ADC_READDATA + (index * 0x4));
    return value;
}

static void ADC_set_Enable(uint32_t data)
{
    gk_adc_writel(REG_ADC_ENABLE, data);
}

static void ADC_set_AUX_ATOP(uint32_t data)
{
    gk_adc_writel(REG_ADC_AUX_ATOP, data);
}

static void ADC_set_AUX_ATOP_sar_maxsel(uint8_t data)
{
    ADC_AUX_ATOP_S d;
    d.all = gk_adc_readl(REG_ADC_AUX_ATOP);
    d.bitc.sar_maxsel = data;
    gk_adc_writel(REG_ADC_AUX_ATOP, d.all);
}

static void ADC_select_channel(ADC_CHANNEL_E channel)
{
    if(channel == ADC_CHANNEL_ONE){
        ADC_set_AUX_ATOP_sar_maxsel(0x05);
        ADC_set_Control_channel(0x01);
    }else if(channel == ADC_CHANNEL_TWO){
        ADC_set_AUX_ATOP_sar_maxsel(0x06);
        ADC_set_Control_channel(0x02);
    }else{
        printk(KERN_ERR "Invalid channel number.\n");
    }
}

static int gk_adc_open(struct inode *inode, struct file *filp)
{
    mutex_lock(&adc_mutex);
    if(1 == in_use){
        return -EBUSY;
    } else {
        in_use = 1;
    }
    cur_ch = ADC_CHANNEL_NONE;
    mutex_unlock(&adc_mutex);

    return 0;
}

static int gk_adc_close(struct inode *inode, struct file *filp)
{
    mutex_lock(&adc_mutex);
    in_use = 0;
    mutex_unlock(&adc_mutex);
    return 0;
}

static long gk_adc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    uint32_t cnt = 0;
    ADC_STATE_MACHINE_S adc_state;

    switch (cmd)
    {
        case IOC_ADC_SET_CHANNEL: //set ADC channel
        {
            if (arg == 0)
                return -EINVAL;

            if (copy_from_user(&adc_state, (void __user *)arg, sizeof(ADC_STATE_MACHINE_S)))
                return -EFAULT;

            if((adc_state.channel == ADC_CHANNEL_ONE)
                || (adc_state.channel == ADC_CHANNEL_TWO)){
                mutex_lock(&adc_mutex);
                cur_ch = adc_state.channel;
                mutex_unlock(&adc_mutex);
                ADC_select_channel(cur_ch);
            }else {
                printk(KERN_ERR "Invalid param, ADC only has two channel.\n");
                return -EINVAL;
            }
            break;
        }
        case IOC_ADC_GET_CHANNEL:
        {
            if(copy_to_user((void __user *)arg, &adc_state, sizeof(ADC_STATE_MACHINE_S)))
                return -EFAULT;
            break;
        }
        case IOC_ADC_GET_DATA:
        {
            if (copy_from_user(&adc_state, (void __user *)arg, sizeof(ADC_STATE_MACHINE_S)))
                return -EFAULT;
            if((adc_state.channel == ADC_CHANNEL_ONE)
                || (adc_state.channel == ADC_CHANNEL_TWO)){
                mutex_lock(&adc_mutex);
                if(adc_state.channel != cur_ch){
                    cur_ch = adc_state.channel;
                    ADC_select_channel(cur_ch);
                    ADC_set_Control_status(0);
                    while(0 == ADC_get_Control_status()){
                        cnt++;
                        if (cnt > 10){
                            break;
                        }
                        mdelay(1);
                    }
                     adc_state.data = ADC_get_ReadData((u32)cur_ch);
                }
                else {
                     adc_state.data = ADC_get_ReadData((u32)cur_ch);
                }
                mutex_unlock(&adc_mutex);
            }else {
                printk(KERN_ERR "Invalid param, ADC only has two channel.\n");
                return -EINVAL;
            }
             if(copy_to_user((void __user *)arg, &adc_state, sizeof(ADC_STATE_MACHINE_S)))
                return -EFAULT;
            break;
        }
        default:
            return -EINVAL;
    }
    return 0; //³É¹¦·µ»Ø
}


/*initalize structure file operations for adc*/
static struct file_operations dev_fops = {
    .owner      = THIS_MODULE,
    .open       = gk_adc_open,
    .release    = gk_adc_close,
//    .read       = gk_adc_read,
    .unlocked_ioctl      = gk_adc_ioctl,
};

static struct miscdevice misc = {
    .minor  = 11, /*MISC_DYNAMIC_MINOR,*/
    .name   = DEVICE_NAME,
    .fops   = &dev_fops,
};

static int __init dev_init(void)
{
    int ret;
    ADC_AUX_ATOP_S  data;
    mutex_init(&adc_mutex);
    in_use = 0;
    ADC_set_Enable(1);
    data.all = 0;
    data.bitc.sar_maxnsel = 0x05;
    data.bitc.sar_maxsel  = 0x05;
    data.bitc.sar_freerun = 0x01;
    ADC_set_AUX_ATOP(data.all);
    // 001 0 1 0
    ADC_set_Control(0x000A);
    ret = misc_register(&misc); //register a misc device
    printk (DEVICE_NAME" initialized (10:11)\n");
    return ret;
}

static void __exit dev_exit(void)
{
    misc_deregister(&misc); //unregister a misc device
    mutex_destroy(&adc_mutex);
}

module_init(dev_init);
module_exit(dev_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Goke Microelectronics Inc.");
MODULE_DESCRIPTION("GK ADC Driver");
