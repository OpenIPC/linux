/*
 * rtc.h
 *
 *  Created on: Aug 18, 2016
 *      Author: fullhan
 */

#ifndef ARCH_ARM_MACH_FH_INCLUDE_MACH_RTC_H_
#define ARCH_ARM_MACH_FH_INCLUDE_MACH_RTC_H_
#include "fh_predefined.h"

/*
 * Registers offset
 */
#define  FH_RTC_COUNTER            0x0
#define  FH_RTC_OFFSET             0x4
#define  FH_RTC_POWER_FAIL         0x8
#define  FH_RTC_ALARM_COUNTER      0xC
#define  FH_RTC_INT_STAT           0x10
#define  FH_RTC_INT_EN             0x14
#define  FH_RTC_SYNC               0x18
#define  FH_RTC_DEBUG              0x1C
#define  FH_RTC_USER_REG           0x20

#define SEC_BIT_START		0
#define SEC_VAL_MASK		0x3f

#define MIN_BIT_START		6
#define MIN_VAL_MASK		0xfc0

#define HOUR_BIT_START		12
#define HOUR_VAL_MASK		0x1f000

#define DAY_BIT_START		17
#define DAY_VAL_MASK		0xfffe0000

#define FH_RTC_ISR_SEC_POS  	1<<0
#define FH_RTC_ISR_MIN_POS  	1<<1
#define	FH_RTC_ISR_HOUR_POS  	1<<2
#define	FH_RTC_ISR_DAY_POS  	1<<3
#define	FH_RTC_ISR_ALARM_POS    1<<4
#define	FH_RTC_ISR_SEC_MASK 	1<<27
#define	FH_RTC_ISR_MIN_MASK 	1<<28
#define	FH_RTC_ISR_HOUR_MASK 	1<<29
#define	FH_RTC_ISR_DAY_MASK 	1<<30
#define	FH_RTC_ISR_ALARM_MASK 	1<<31

// input: val=fh_rtc_get_time(base_addr)
#define FH_GET_RTC_SEC(val)		 ((val & SEC_VAL_MASK) >> SEC_BIT_START)
#define FH_GET_RTC_MIN(val)		 ((val & MIN_VAL_MASK) >> MIN_BIT_START)
#define FH_GET_RTC_HOUR(val)	 ((val & HOUR_VAL_MASK) >> HOUR_BIT_START)
#define FH_GET_RTC_DAY(val)	 	 ((val & DAY_VAL_MASK) >> DAY_BIT_START)

#define ELAPSED_LEAP_YEARS(y) (((y -1)/4)-((y-1)/100)+((y+299)/400)-17)

#define FH_RTC_PROC_FILE    "driver/fh_rtc"

struct fh_rtc_platform_data
{
	u32 clock_in;
	char *clk_name;
	char *dev_name;
	u32 base_year;
	u32 base_month;
	u32 base_day;
	int sadc_channel;
};
enum
{
	init_done=1,
	initing=0

};

/*******************************************************************************
* Function Name  : fh_rtc_interrupt_disabel
* Description    : disabale rtc interrupt
* Input          : rtc base addr
* Output         : None
* Return         : None
*
 *******************************************************************************/
#define  fh_rtc_interrupt_disabel(base_addr)                   SET_REG(base_addr+REG_RTC_INT_EN,DISABLE)

/*******************************************************************************
* Function Name  : fh_rtc_get_time
* Description    : get rtc current time
* Input          : rtc base addr
* Output         : None
* Return         : None
*
 *******************************************************************************/
#define  fh_rtc_get_time(base_addr)                  			GET_REG(base_addr+FH_RTC_COUNTER)

/*******************************************************************************
* Function Name  : fh_rtc_set_time
* Description    : set rtc current time
* Input          : rtc base addr
* Output         : None
* Return         : None
*
 *******************************************************************************/
#define  fh_rtc_set_time(base_addr,value)                  		SET_REG(base_addr+FH_RTC_COUNTER,value)

/*******************************************************************************
* Function Name  : fh_rtc_set_alarm_time
* Description    : set rtc alarm
* Input          : rtc base addr
* Output         : None
* Return         : None
*
 *******************************************************************************/
#define  fh_rtc_set_alarm_time(base_addr,value)                  SET_REG(base_addr+FH_RTC_ALARM_COUNTER,value)

/*******************************************************************************
* Function Name  : fh_rtc_get_alarm_time
* Description    : get alarm register
* Input          : rtc base addr
* Output         : None
* Return         : None
*
 *******************************************************************************/
#define  fh_rtc_get_alarm_time(base_addr)                 		 GET_REG(base_addr+FH_RTC_ALARM_COUNTER)

/*******************************************************************************
* Function Name  : fh_rtc_get_int_status
* Description    : get rtc current interrupt status
* Input          : rtc base addr
* Output         : None
* Return         : None
*
 *******************************************************************************/
#define  fh_rtc_get_int_status(base_addr)                  		  GET_REG(base_addr+FH_RTC_INT_STAT)
/*******************************************************************************
* Function Name  : fh_rtc_enable_interrupt
* Description    : enable rtc interrupt
* Input          : rtc base addr
* Output         : None
* Return         : None
*
 *******************************************************************************/
#define  fh_rtc_enable_interrupt(base_addr,value)                 SET_REG(base_addr+FH_RTC_INT_EN,value|GET_REG(base_addr+FH_RTC_INT_EN))
/*******************************************************************************
* Function Name  : fh_rtc_disenable_interrupt
* Description    : disable interrupt
* Input          : rtc base addr
* Output         : None
* Return         : None
*
 *******************************************************************************/
#define  fh_rtc_disenable_interrupt(base_addr,value)             SET_REG(base_addr+FH_RTC_INT_EN,(~value)&GET_REG(base_addr+FH_RTC_INT_EN))

/*******************************************************************************
* Function Name  : fh_rtc_get_enabled_interrupt
* Description    : get rtc current interrupt enabled
* Input          : rtc base addr
* Output         : None
* Return         : None
*
 *******************************************************************************/
#define  fh_rtc_get_enabled_interrupt(base_addr)                 GET_REG(base_addr+FH_RTC_INT_EN)
/*******************************************************************************
* Function Name  : fh_rtc_set_mask_interrupt
* Description    : set rtc interrupt mask
* Input          : rtc base addr
* Output         : None
* Return         : None
*
 *******************************************************************************/
#define  fh_rtc_set_mask_interrupt(base_addr,value)             SET_REG(base_addr+FH_RTC_INT_EN,value|GET_REG(base_addr+FH_RTC_INT_EN))
/*******************************************************************************
* Function Name  : fh_rtc_clear_interrupt_status
* Description    : clear rtc interrupt status
* Input          : rtc base addr
* Output         : None
* Return         : None
*
 *******************************************************************************/
#define  fh_rtc_clear_interrupt_status(base_addr,value)         SET_REG(base_addr+FH_RTC_INT_STAT,(~value)&GET_REG(base_addr+FH_RTC_INT_STAT))
/*******************************************************************************
* Function Name  : fh_rtc_get_offset
* Description    : get rtc offset
* Input          : rtc base addr
* Output         : None
* Return         : None
*
 *******************************************************************************/
#define  fh_rtc_get_offset(base_addr)                  			GET_REG(base_addr+FH_RTC_OFFSET)
/*******************************************************************************
* Function Name  : fh_rtc_get_power_fail
* Description    : get rtc power fail register
* Input          : rtc base addr
* Output         : None
* Return         : None
*
 *******************************************************************************/
#define  fh_rtc_get_power_fail(base_addr)                  		GET_REG(base_addr+FH_RTC_POWER_FAIL)

/*******************************************************************************
* Function Name  : fh_rtc_get_sync
* Description    : get rtc sync register value
* Input          : rtc base addr
* Output         : None
* Return         : None
*
 *******************************************************************************/
#define  fh_rtc_get_sync(base_addr)                  			GET_REG(base_addr+FH_RTC_SYNC)

/*******************************************************************************
* Function Name  : fh_rtc_set_sync
* Description    : set rtc sync register value
* Input          : rtc base addr,init_done/initing
* Output         : None
* Return         : None
*
 *******************************************************************************/
#define  fh_rtc_set_sync(base_addr,value)                  		SET_REG(base_addr+FH_RTC_SYNC,value)

/*******************************************************************************
* Function Name  : fh_rtc_get_debug
* Description    : get rtc debug register value
* Input          : rtc base addr
* Output         : None
* Return         : None
*
 *******************************************************************************/
#define  fh_rtc_get_debug(base_addr)                  			GET_REG(base_addr+FH_RTC_DEBUG)

/*******************************************************************************
* Function Name  : fh_rtc_set_debug
* Description    : set rtc debug register value
* Input          : rtc base addr,x pclk
* Output         : None
* Return         : None
*
 *******************************************************************************/
#define  fh_rtc_set_debug(base_addr,value)     					SET_REG(base_addr+FH_RTC_DEBUG,value)
#endif /* ARCH_ARM_MACH_FH_INCLUDE_MACH_RTC_H_ */
