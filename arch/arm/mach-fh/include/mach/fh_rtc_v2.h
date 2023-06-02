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
#define  FH_RTC_INT_STATUS			   0x0
#define  FH_RTC_INT_EN				   0x4
#define  FH_RTC_DEBUG0				   0x8
#define  FH_RTC_DEBUG1				   0xC
#define  FH_RTC_DEBUG2				   0x10
#define  FH_RTC_CMD					   0x14
#define  FH_RTC_RD_DATA				   0x18
#define  FH_RTC_WR_DATA				   0x1C

#define  FH_RTC_CMD_COUNTER            (0<<4)
#define  FH_RTC_CMD_OFFSET             (1<<4)
#define  FH_RTC_CMD_ALARM_CFG          (2<<4)
#define  FH_RTC_CMD_TEMP_INFO          (0x3<<4)
#define  FH_RTC_CMD_TEMP_CFG           (0x4<<4)
#define  FH_RTC_CMD_ANA_CFG            (0x5<<4)
#define  FH_RTC_CMD_INT_STATUS         (0x6<<4)
#define  FH_RTC_CMD_INT_EN             (0x7<<4)
#define  FH_RTC_CMD_DEBUG              (0x8<<4)
#define  FH_RTC_CMD_OFFSET_LUT         (0x9<<4)

#define  OFFSET_EN                     (1<<0)
#define  OFFSET_ATUTO                  (1<<1)
#define  OFFSET_IDX                    (1<<2)
#define  OFFSET_BK_EN                  (1<<8)
#define  OFFSET_BK_AUTO                (1<<9)
#define  OFFSET_BK_IDX                 (1<<10)
#define  OFFSET_CURRENT                (1<<16)
#define  LP_MODE                       (1<<31)

#define  RTC_READ 1
#define  RTC_WRITE 2
#define  RTC_TEMP 3
#define  FH_RTC_INT_STATUS_RX_CRC_ERR   (1<<0)
#define  FH_RTC_INT_STATUS_RX_COM_ERR   (1<<1)
#define  FH_RTC_INT_STATUS_RX_LEN_ERR   (1<<2)
#define  FH_RTC_INT_STATUS_CNT_THL	    (1<<3)
#define  FH_RTC_INT_STATUS_CNT_THH	    (1<<4)
#define  FH_RTC_INT_STATUS_CORE_IDLE	(1<<5)
#define  FH_RTC_INT_STATUS_CORE		    (1<<6)
#define  FH_RTC_INT_STATUS_WRAPPER_BUSY (1<<8)
#define  FH_RTC_INT_STATUS_CORE_BUSY    (1<<16)

#define  FH_RTC_INT_RX_CRC_ERR_EN        (1<<0)
#define  FH_RTC_INT_RX_COM_ERR_EN        (1<<1)
#define  FH_RTC_INT_RX_LEN_ERR_EN        (1<<2)
#define  FH_RTC_INT_CNT_THL_ERR_EN       (1<<3)
#define  FH_RTC_INT_CNT_THH_ERR_EN       (1<<4)
#define  FH_RTC_INT_CORE_IDLE_ERR_EN     (1<<5)
#define  FH_RTC_INT_CORE_INT_ERR_EN      (1<<6)
#define  FH_RTC_INT_RX_CRC_ERR_MASK      (1<<16)
#define  FH_RTC_INT_RX_COM_ERR_MASK      (1<<17)
#define  FH_RTC_INT_RX_LEN_ERR_MASK      (1<<18)
#define  FH_RTC_INT_CNT_THL_ERR_MASK     (1<<19)
#define  FH_RTC_INT_CNT_THH_ERR_MASK     (1<<20)
#define  FH_RTC_INT_CORE_IDLE_ERR_MASK   (1<<21)
#define  FH_RTC_INT_CORE_INT_ERR_MASK    (1<<22)
#define  FH_RTC_INT_CORE_INT_ERR_MASK_COV    0xffbfffff
#define  FH_RTC_INT_CORE_INT_STATUS_COV    0xffffff3f
#define  FH_RTC_INT_CORE_INT_ALL_COV    0xffffffff

#define FH_RTC_CORE_INT_EN_SEC_INT       (0x1<<0)
#define FH_RTC_CORE_INT_EN_MIN_INT       (0x1<<1)
#define FH_RTC_CORE_INT_EN_HOU_INT       (0x1<<2)
#define FH_RTC_CORE_INT_EN_DAY_INT       (0x1<<3)
#define FH_RTC_CORE_INT_EN_ALM_INT       (0x1<<4)
#define FH_RTC_CORE_INT_EN_POW_INT       (0x1<<5)


#define FH_RTC_CORE_INT_EN_SEC_MAS       (0x1<<16)
#define FH_RTC_CORE_INT_EN_MIN_MAS       (0x1<<17)
#define FH_RTC_CORE_INT_EN_HOU_MAS       (0x1<<18)
#define FH_RTC_CORE_INT_EN_DAY_MAS       (0x1<<19)
#define FH_RTC_CORE_INT_EN_ALM_MAS       (0x1<<20)
#define FH_RTC_CORE_INT_EN_POE_MAS       (0x1<<21)

#define	FH_RTC_IOCTL_MEM_BASE	'A'
#define GET_TSENSOR_DATA _IOWR(FH_RTC_IOCTL_MEM_BASE, 0, int)
#define GET_CURRENT_OFFSET_DATA _IOWR(FH_RTC_IOCTL_MEM_BASE, 1, int)
#define GET_CURRENT_OFFSET_IDX _IOWR(FH_RTC_IOCTL_MEM_BASE, 2, int)
#define RTC_GET_LUT _IOWR(FH_RTC_IOCTL_MEM_BASE, 3, int)
#define RTC_SET_LUT _IOWR(FH_RTC_IOCTL_MEM_BASE, 4, int)
#define GET_REG_VALUE _IOWR(FH_RTC_IOCTL_MEM_BASE, 5, int)
#define SET_REG_VALUE _IOWR(FH_RTC_IOCTL_MEM_BASE, 6, int)
#define GET_TEMP_VALUE _IOWR(FH_RTC_IOCTL_MEM_BASE, 7, int)
#define GET_WRAPPER_REG _IOWR(FH_RTC_IOCTL_MEM_BASE, 8, int)
#define SET_WRAPPER_REG _IOWR(FH_RTC_IOCTL_MEM_BASE, 9, int)
#define SET_REG_VALUE_SINGLE _IOWR(FH_RTC_IOCTL_MEM_BASE, 10, int)

#define SEC_BIT_START		0
#define SEC_VAL_MASK		0x3f

#define MIN_BIT_START		6
#define MIN_VAL_MASK		0xfc0

#define HOUR_BIT_START		12
#define HOUR_VAL_MASK		0x1f000

#define DAY_BIT_START		17
#define DAY_VAL_MASK		0xfffe0000

#define FH_RTC_ISR_SEC_POS	              (1<<0)
#define FH_RTC_ISR_MIN_POS	              (1<<1)
#define	FH_RTC_ISR_HOUR_POS	              (1<<2)
#define	FH_RTC_ISR_DAY_POS	              (1<<3)
#define	FH_RTC_ISR_ALARM_POS              (1<<4)
#define	FH_RTC_ISR_POWERFAIL_POS          (1<<5)
#define FH_RTC_ISR_RX_CRC_ERR_INT         (1<<6)
#define FH_RTC_ISR_RX_COM_ERR_INT         (1<<7)
#define FH_RTC_LEN_ERR_INT                (1<<8)

/* input: val=fh_rtc_get_time(base_addr)*/
#define FH_GET_RTC_SEC(val) ((val & SEC_VAL_MASK) >> SEC_BIT_START)
#define FH_GET_RTC_MIN(val) ((val & MIN_VAL_MASK) >> MIN_BIT_START)
#define FH_GET_RTC_HOUR(val) ((val & HOUR_VAL_MASK) >> HOUR_BIT_START)
#define FH_GET_RTC_DAY(val) ((val & DAY_VAL_MASK) >> DAY_BIT_START)

#define ELAPSED_LEAP_YEARS(y) (((y-1)/4)-((y-1)/100)+((y+299)/400)-17)
#define OUT_TEMP(y) (y/4096*235-73)
#define FH_RTC_PROC_FILE    "fh_zy_rtc"

struct fh_rtc_platform_data {
	u32 clock_in;
	char *clk_name;
	char *dev_name;
	u32 base_year;
	u32 base_month;
	u32 base_day;
	int sadc_channel;
};
enum {
	init_done = 1,
	initing = 0

};

/****************************************************
* Function Name  : fh_rtc_interrupt_disabel
* Description    : disabale rtc interrupt
* Input          : rtc base addr
* Output         : None
* Return         : None
*
 ****************************************************/
#define  fh_rtc_interrupt_disabel(base_addr) \
SET_REG(base_addr+REG_RTC_INT_EN, DISABLE)

/******************************************************
* Function Name  : fh_rtc_get_int_status
* Description    : get rtc current interrupt status
* Input          : rtc base addr
* Output         : None
* Return         : None
*
 ******************************************************/
#define  fh_rtc_get_int_status(base_addr) \
GET_REG(base_addr+FH_RTC_INT_STAT)
/******************************************************
* Function Name  : fh_rtc_enable_interrupt
* Description    : enable rtc interrupt
* Input          : rtc base addr
* Output         : None
* Return         : None
*
 ******************************************************/
#define  fh_rtc_enable_interrupt(base_addr, value) \
SET_REG(base_addr+FH_RTC_INT_EN, \
value|GET_REG(base_addr+FH_RTC_INT_EN))


/*******************************************************
* Function Name  : fh_rtc_get_enabled_interrupt
* Description    : get rtc current interrupt enabled
* Input          : rtc base addr
* Output         : None
* Return         : None
*
 ******************************************************/
#define  fh_rtc_get_enabled_interrupt(base_addr) \
GET_REG(base_addr+FH_RTC_INT_EN)
/********************************************************
* Function Name  : fh_rtc_set_mask_interrupt
* Description    : set rtc interrupt mask
* Input          : rtc base addr
* Output         : None
* Return         : None
*
 *******************************************************/
#define  fh_rtc_set_mask_interrupt(base_addr, value) \
SET_REG(base_addr+FH_RTC_INT_EN, value|GET_REG(base_addr+FH_RTC_INT_EN))
/********************************************************
* Function Name  : fh_rtc_get_offset
* Description    : get rtc offset
* Input          : rtc base addr
* Output         : None
* Return         : None
*
 *******************************************************/
#define  fh_rtc_get_offset(base_addr) \
GET_REG(base_addr+FH_RTC_OFFSET)
/********************************************************
* Function Name  : fh_rtc_get_power_fail
* Description    : get rtc power fail register
* Input          : rtc base addr
* Output         : None
* Return         : None
*
 ******************************************************/
#define  fh_rtc_get_power_fail(base_addr) \
GET_REG(base_addr+FH_RTC_POWER_FAIL)

/********************************************************
* Function Name  : fh_rtc_get_sync
* Description    : get rtc sync register value
* Input          : rtc base addr
* Output         : None
* Return         : None
*
 *********************************************************/
#define  fh_rtc_get_sync(base_addr) \
GET_REG(base_addr+FH_RTC_SYNC)

/**********************************************************
* Function Name  : fh_rtc_set_sync
* Description    : set rtc sync register value
* Input          : rtc base addr,init_done/initing
* Output         : None
* Return         : None
*
************************************************************/
#define  fh_rtc_set_sync(base_addr, value) \
SET_REG(base_addr+FH_RTC_SYNC, value)

/***********************************************************
* Function Name  : fh_rtc_get_debug
* Description    : get rtc debug register value
* Input          : rtc base addr
* Output         : None
* Return         : None
*
 **********************************************************/
#define  fh_rtc_get_debug(base_addr)  \
GET_REG(base_addr+FH_RTC_DEBUG)

/************************************************************
* Function Name  : fh_rtc_set_debug
* Description    : set rtc debug register value
* Input          : rtc base addr,x pclk
* Output         : None
* Return         : None
*
 ***********************************************************/
#define  fh_rtc_set_debug(base_addr, value) \
SET_REG(base_addr+FH_RTC_DEBUG, value)
#endif /* ARCH_ARM_MACH_FH_INCLUDE_MACH_RTC_H_ */
