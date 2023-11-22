/*
 *  ip6303_battery.h
 *  fuel-gauge systems for lithium-ion (Li+) batteries
 *
 *  Copyright (C) 2019 SStar
 *  lei.qin <lei.qin@sigmastar.com.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef __IP6303_BATTERY_H_
#define __IP6303_BATTERY_H_

#define GPIO_HIGH 1
#define GPIO_LOW  0
#define PMU_IRQ_ACT_LEVEL		(1)

#define BIT0	0x01
#define BIT1	0x02
#define BIT2	0x04
#define BIT3	0x08
#define BIT4	0x10
#define BIT5	0x20
#define BIT6	0x40
#define BIT7	0x80

typedef unsigned int    MMP_ULONG;

struct ip6303_platform_data {
	int (*battery_online)(void);
	int (*charger_online)(void);
	int (*charger_enable)(void);
};

enum _IP_PMU_INTR_MASK
{
	IP_PMU_INTR_UNMASK = 0,
	IP_PMU_INTR_MASK
};

enum _IP6303_REG_E
{
    PSTATE_CTL0 = 0x00,
    PSTATE_CTL1 = 0x01,
    PSTATE_CTL2 = 0x02,
    PSTATE_CTL3 = 0x03,
    PSTATE_SET = 0X04,
    PROTECT_CTL4 = 0X0A,
    PWROFF_REC0 = 0X10,
    PWROFF_REC1 = 0X11,
    PWROFF_REC2 = 0X12,
    WDOG_CTL	= 0x1A,
    DC1_VSET = 0X21,
    DC2_VSET = 0X26,
    DC3_VSET = 0X2B,
    LDO_EN = 0X40,
    LDO2_VSET = 0X42,
    LDO3_VSET = 0X43,
    LDO4_VSET = 0X44,
    LDO5_VSET = 0X45,
    CHG_ANA_CTL0 = 0x50,
    CHG_ANA_CTL1 = 0x51,
    CHG_DIG_CTL0 = 0x53,
    CHG_DIG_CTL1 = 0x54,
    CHG_DIG_CTL2 = 0x55,
    CHG_DIG_CTL3 = 0x58,
    ADC_ANA_CTL0 = 0x60,
    ADC_DATA_VBAT = 0x64,
    ADC_DATA_IBAT = 0x65,
    ADC_DATA_ICHG = 0x66,
    ADC_DATA_GP1 = 0x67,
    ADC_DATA_GP2 = 0x68,
    INTS_CTL = 0x70,
    INTR_FLAG_0 = 0x71,
    INTR_FLAG_1 = 0x72,
    INTR_MASK_0 = 0x73,
    INTR_MASK_1 = 0x74,
    MFP_CTL0 = 0x75,
    MFP_CTL1 = 0x76,
};

typedef struct _IpWDogCtl_T
{
	unsigned int	uTimerType : 2;     //00:0.5s   01:2s   10:8s   11:16s
	unsigned int	bTimerClr : 1;      //写1  清除
	unsigned int	bIsWDogEn : 1;      // 1:使能   0:禁止
	unsigned int	bRsvd : 28;
}IpWDogCtl_T, *PIpWDogCtl_T;

typedef union _IP6303_WDogCtl_U
{
	unsigned int	uData;
	IpWDogCtl_T	tFlag;
}IP6303WDogCtl_U, *pIP6303WDogCtl_U;

//REG_0x00
typedef struct _IPPStateCtl0_T
{
	MMP_ULONG	bPwrOffEn : 1;//1:进入S2/S3
	MMP_ULONG	bInstPowerDown : 1;//写0 :顺序掉电，1:同时掉电
	MMP_ULONG	bPorOffEn : 1;//1:使能POR拉低关机使能
	MMP_ULONG	bVBusWkEn : 1;//1:使能VBUS唤醒
	MMP_ULONG	bSOnoffWkEn : 1;//1:使能短按ONOFF唤醒
	MMP_ULONG	bLOnoffWkEn : 1;//1:使能长按ONOFF唤醒
	MMP_ULONG	bIrqWkEn : 1;//1:使能外部中断唤醒
	MMP_ULONG	bAlarmWkEn : 1;//1:使能Alarm唤醒
	MMP_ULONG	bRsvd : 24;
}IPPStateCtl0_T, *PIPPStateCtl0_T;
typedef union _IPPStateCtl0_U
{
	MMP_ULONG		uData;
	IPPStateCtl0_T	tFlag;
}IPPStateCtl0_U, *PIPPStateCtl0_U;

//REG_0x58
typedef struct _IpChgDigCtl3_T
{
	MMP_ULONG	bRsvd0 : 1;
	MMP_ULONG	bEnChg : 1;//1:使能充电
	MMP_ULONG	uChgLedMode : 1;//0:充电亮，充满灭，放电灭，低电慢闪;1:充电快闪，充满常亮，放电常亮，低电慢闪
	MMP_ULONG	bRsvd : 29;
}IpChgDigCtl3_T, *PIpChgDigCtl3_T;
typedef union _IpChgDigCtl3_U
{
	MMP_ULONG		uData;
	IpChgDigCtl3_T	tFlag;
}IpChgDigCtl3_U, *PIpChgDigCtl3_U;

//REG_0x60
typedef struct _IpAdcAnaCtl0_T
{
	MMP_ULONG	bEnVBatAdc : 1;//1:使能电池电压ADC 检测
	MMP_ULONG	bEnIBatAdc : 1;//使能电池电流ADC 检测
	MMP_ULONG	bEnIChgAdc : 1;//1:使能充电电流ADC 检测
	MMP_ULONG	bEnGp1Adc : 1;//1:使能GP1  ADC 检测
	MMP_ULONG	bEnGp2Adc : 1;//1:使能GP2  ADC 检测
	MMP_ULONG	bRsvd : 27;
}IpAdcAnaCtl0_T, *PIpAdcAnaCtl0_T;
typedef union _IpAdcAnaCtl0_U
{
	MMP_ULONG		uData;
	IpAdcAnaCtl0_T	tFlag;
}IpAdcAnaCtl0_U, *PIpAdcAnaCtl0_U;

//REG_0x70
typedef struct _IpIntsCtl_T
{
	MMP_ULONG	bIrqPolHighValid : 1;//IRQ极性1:High Valid,0:Low Valid
	MMP_ULONG	bIrqClr : 1;//写1  后IRQ输出无效电瓶，delay 32 us后再输出有效电瓶
	MMP_ULONG	bRsvd : 30;
}IpIntsCtl_T, *PIpIntsCtl_T;
typedef union _IpIntsCtl_U
{
	MMP_ULONG		uData;
	IpIntsCtl_T	tFlag;
}IpIntsCtl_U, *PIpIntsCtl_U;


//REG_0x71
typedef struct _IpIntrFlag0_T
{
	MMP_ULONG	bShortPressOnOff : 1;//短按键
	MMP_ULONG	bLongPressOnOff : 1;//长按键
	MMP_ULONG	bUShortPressOnOff : 1;//超短按按键
	MMP_ULONG	bVBusIn : 1;//VBUS插入
	MMP_ULONG	bVBusOut : 1;//VBUS拔出
	MMP_ULONG	bLowBat : 1;//电池低电
	MMP_ULONG	bRsvd : 1;
	MMP_ULONG	bAlarm : 1;//ALARM
	MMP_ULONG	bRsvd1 : 24;
}IpIntrFlag0_T, *PIpIntrFlag0_T;
typedef union _IpIntrFlag0_U
{
	MMP_ULONG		uData;
	IpIntrFlag0_T	tFlag;
}IpIntrFlag0_U, *PIpIntrFlag0_U;


//REG_0x73
typedef struct _IpIntrMask0_T
{
	MMP_ULONG	bShortPressOnOff : 1;//短按键
	MMP_ULONG	bLongPressOnOff : 1;//长按键
	MMP_ULONG	bUShortPressOnOff : 1;//超短按按键
	MMP_ULONG	bVBusIn : 1;//VBUS插入
	MMP_ULONG	bVBusOut : 1;//VBUS拔出
	MMP_ULONG	bLowBat : 1;//电池低电
	MMP_ULONG	bRsvd : 1;
	MMP_ULONG	bAlarm : 1;//ALARM
	MMP_ULONG	bRsvd1 : 24;
}IpIntrMask0_T, *PIpIntrMask0_T;
typedef union _IpIntrMask0_U
{
	MMP_ULONG		uData;
	IpIntrMask0_T	tFlag;
}IpIntrMask0_U, *PIpIntrMask0_U;

#endif
