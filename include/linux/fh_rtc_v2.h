#ifndef FH_RTC_V2_H_
#define FH_RTC_V2_H_

#define	FH_RTC_IOCTL_MEM_BASE	'A'
#define GET_TSENSOR_DATA _IOWR(FH_RTC_IOCTL_MEM_BASE, 0, int)
#define GET_CURRENT_OFFSET_DATA _IOWR(FH_RTC_IOCTL_MEM_BASE, 1, int)
#define GET_CURRENT_OFFSET_IDX _IOWR(FH_RTC_IOCTL_MEM_BASE, 2, int)
#define RTC_GET_LUT _IOWR(FH_RTC_IOCTL_MEM_BASE, 3, int)
#define RTC_SET_LUT _IOWR(FH_RTC_IOCTL_MEM_BASE, 4, int)
#define GET_REG_VALUE _IOWR(FH_RTC_IOCTL_MEM_BASE, 5, int)
#define SET_REG_VALUE _IOWR(FH_RTC_IOCTL_MEM_BASE, 6, int)
#define GET_TEMP_VALUE _IOWR(FH_RTC_IOCTL_MEM_BASE, 7, int)

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

#define FH_RTC_MISC_DEVICE_NAME			"fh_rtc_misc"

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

int fh_rtc_get_tsensor_data(int *data, int *temp);

#endif /* FH_RTC_V2_H_ */
