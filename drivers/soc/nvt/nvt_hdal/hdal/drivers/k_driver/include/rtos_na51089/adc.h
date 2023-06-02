/**
    Header file for ADC module.

    This file is the header file that define the APIs and data types for ADC module.
    The user can reference this section for the detail description of the each driver API usage
    and also the parameter descriptions and its limitations.
    The overall combinational usage flow is introduced in the application note document,
    and the user must reference to the application note for the driver usage flow.

    @file       adc.h

    @ingroup    mIDrvIO_ADC
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2014.  All rights reserved.
*/

#ifndef _ADC_H
#define _ADC_H

#include <kwrap/nvt_type.h>
#include "../comm/driver.h"

/**
    @addtogroup mIDrvIO_ADC
*/
//@{

/**
    Definitions for ADC configuration ID.

    Definitions for ADC configuration ID.
    These definitions are used to configure the general settings such as adc analog block operating frequency and sample periods.

    @note Used in adc_setConfig() / adc_getConfig().
*/
typedef enum {
	ADC_CONFIG_ID_OCLK_FREQ,        ///< Set the ADC analog block operating frequency. This value allowable range is from 250000 to 2000000 in current design.
	ADC_CONFIG_ID_SAMPLE_AVERAGE,   ///< Set ADC Sample average times. Use "ADC_SAMPLEAVG" as input paramters.
	ADC_CONFIG_ID_EXT_SAMPLE_CNT,   ///< Set ADC channel external HOLD time. Valid range from 0x0~0xF. This setting would enlarge the hold time of adc each channel.
	ADC_CONFIG_ID_THERMAL_AVG,      ///< Set ADC thermal channel average counts, range from 2 ~ 256 as 2's order.
	ENUM_DUMMY4WORD(ADC_CONFIG_ID)
} ADC_CONFIG_ID;


/**
    ADC sampling average times every sequence

    Definition ADC sampling average times every output sample. By setting this value to ADC_SAMPAVG_2 or ADC_SAMPAVG_4, the
    ADC output rate would be slower because the controller would sample each ADC channel 2 or 4 times and output the average value automatically.

    @note Used in adc_setConfig(ADC_CONFIG_ID_SAMPLE_AVERAGE, ADC_SAMPAVG_1/2/4).
*/
typedef enum {
	ADC_SAMPAVG_1,  ///< Sample one time and output the value.
	ADC_SAMPAVG_2,  ///< Sample two times and output the average value.
	ADC_SAMPAVG_4   ///< Sample four times and output the average value.
} ADC_SAMPLEAVG;


/**
    ADC channel ID

    Definition for ADC channel identification. These definitions are used to specify the dedicated channel to access.

    @note Used in adc_open() / adc_close() / adc_readVoltage() / adc_setChConfig() / ...
*/
typedef enum {
	ADC_CHANNEL_0,          ///< ADC channel 0
	ADC_CHANNEL_1,          ///< ADC channel 1
	ADC_CHANNEL_2,          ///< ADC channel 2
	ADC_CHANNEL_3,          ///< ADC channel 3

	ADC_TOTAL_CH,   // Total ADC channel number
	ENUM_DUMMY4WORD(ADC_CHANNEL)
} ADC_CHANNEL;


/**
    ADC Channel Configuration ID

    These definitions are used to configure each ADC channel operating settings such as ISR callback handler, sample frequency, interrupt enanle,
    sample mode, and also the value trigger settings. Please refer to the application note for the functional introductions.

    @note Used in adc_setChConfig(ADC_CHANNEL_X, ADC_CH_CONFIG_ID_SAMPLE_MODE) / adc_getChConfig(ADC_CHANNEL_X, ADC_CH_CONFIG_ID_SAMPLE_MODE).
*/
typedef enum {
	ADC_CH_CONFIG_ID_SAMPLE_MODE,           ///< Set ADC Channel sample mode. Use ADC_CH_SAMPLEMODE_ONESHOT or ADC_CH_SAMPLEMODE_CONTINUOUS.
	ADC_CH_CONFIG_ID_SAMPLE_FREQ,           ///< Set ADC Channel sample frequency. Unit in Hertz.
	ADC_CH_CONFIG_ID_INTEN,                 ///< Set ADC Channel interrupt enable/disable.

	ADC_CH_CONFIG_ID_VALUETRIG_VOL_LOW,     ///< Set ADC Channel value trigger LOW voltage in mV. Valid Range from 0 to 3300(mV).
	///< The user must notice that the NT96650's ADC is 10bits design and the valid resolution is about 8bits,
	///< so the value trigger function is designed for 256 levels only(8bits) and aligned to MSB 8bits of the ADC value.
	///< So this assigned trigger voltage would be slightly different about 0~10mV if this value is read out form adc_getChConfig().
	///< This is because the ADC trigger value LSB 2 bits is forced to 0x0 in the ADC design.
	ADC_CH_CONFIG_ID_VALUETRIG_VOL_HIGH,    ///< Set ADC Channel value trigger HIGH voltage in mV. Valid Range from 0 to 3300.
	///< The user must notice that the NT96650's ADC is 10bits design and the valid resolution is about 8bits,
	///< so the value trigger function is designed for 256 levels only(8bits) and aligned to MSB 8bits of the ADC value.
	///< So this assigned trigger voltage would be slightly different about 0~10mV if this value is read out form adc_getChConfig().
	///< This is because the ADC trigger value LSB 2 bits is forced to 0x0 in the ADC design.
	ADC_CH_CONFIG_ID_VALUETRIG_THD_LOW,     ///< Set ADC Channel value trigger LOW threshold value. Valid Range from 0 to 1023.
	///< The user must notice that the NT96650's ADC is 10bits design and the valid resolution is about 8bits,
	///< so the value trigger function is designed for 256 levels only(8bits) and aligned to MSB 8bits of the ADC value.
	///< So this assigned trigger voltage would be slightly different about 0~2 if this value is read out form adc_getChConfig().
	///< This is because the ADC trigger value LSB 2 bits is forced to 0x0 in the ADC design.
	ADC_CH_CONFIG_ID_VALUETRIG_THD_HIGH,    ///< Set ADC Channel value trigger HIGH threshold value. Valid Range from 0 to 1023.
	///< The user must notice that the NT96650's ADC is 10bits design and the valid resolution is about 8bits,
	///< so the value trigger function is designed for 256 levels only(8bits) and aligned to MSB 8bits of the ADC value.
	///< So this assigned trigger voltage would be slightly different about 0~2 if this value is read out form adc_getChConfig().
	///< This is because the ADC trigger value LSB 2 bits is forced to 0x0 in the ADC design.
	ADC_CH_CONFIG_ID_VALUETRIG_RANGE,       ///< Set ADC Channel value trigger range condition. Please use ADC_VALTRIG_RANGE as input parameter.
	ADC_CH_CONFIG_ID_VALUETRIG_MODE,        ///< Set ADC Channel value trigger mode condition. Please use ADC_VALTRIG_MODE as input parameter.
	ADC_CH_CONFIG_ID_VALUETRIG_EN,          ///< Set ADC Channel value trigger function enable/ disable.
	ADC_CH_CONFIG_ID_VALUETRIG_INTEN,       ///< Set ADC Channel value trigger interrupt enable / disable.

	ENUM_DUMMY4WORD(ADC_CH_CONFIG_ID)
} ADC_CH_CONFIG_ID;


/**
    ADC Channel Sample Mode

    Specify the specified ADC Channel is in OneShot or Continuously Sample Mode.

    @note Used in adc_setChConfig(ADC_CHANNEL_X, ADC_CH_CONFIG_ID_SAMPLE_MODE)
*/
typedef enum {
	ADC_CH_SAMPLEMODE_ONESHOT,      ///< Set ADC Channel to OneShot trigger mode.
	ADC_CH_SAMPLEMODE_CONTINUOUS,   ///< Set ADC Channel to Continuously trigger mode.

	ENUM_DUMMY4WORD(ADC_CH_SAMPLEMODE)
} ADC_CH_SAMPLEMODE;


/**
    Definitions for setup value trigger interrupt's value trigger range.

    ADC controller's value trigger interrupt can select to issue interrupt when the sample value is within specified range.

    @note Used in adc_setChConfig(ADC_CHANNEL_X, ADC_CH_CONFIG_ID_VALUETRIG_RANGE).
*/
typedef enum {
	ADC_VALTRIG_RANGE_IN,   ///< The value trigger interrupt is issued when sample value is inside the specified range.
	ADC_VALTRIG_RANGE_OUT,  ///< The value trigger interrupt is issued when sample value is outside the specified range.

	ENUM_DUMMY4WORD(ADC_VALTRIG_RANGE)
} ADC_VALTRIG_RANGE;


/**
    Definition for setup the value trigger interrupt is edge trigger or level trigger interrupt.

    @note Used in adc_setChConfig(ADC_CHANNEL_X, ADC_CH_CONFIG_ID_VALUETRIG_MODE).
*/
typedef enum {
	ADC_VALTRIG_MODE_LVL,   ///< Set the value trigger interrupt issue condition to level trigger.
	ADC_VALTRIG_MODE_EDGE,  ///< Set the value trigger interrupt issue condition to edge trigger.

	ENUM_DUMMY4WORD(ADC_VALTRIG_MODE)
} ADC_VALTRIG_MODE;

/*
    Touch Panel Configuration ID

    (OBSOLETE)

    Definitions for Touch Panel configurations.

    @note Used in adc_setTpConfig() / adc_getTpConfig().
*/
typedef enum {
	ADC_TP_CONFIG_ID_GSR,               //< De-Glitch function of the touch panel. Unit is 4us per step. Value range 0x00 to 0xFF.
	ADC_TP_CONFIG_ID_RESISTOR,          //< Configure the Touch Panel PenDown Detect Pull Up Resistor. Valid Range form 0 to 63 which means 1K~64K ohm.
	ADC_TP_CONFIG_ID_SAMPLE_FREQ,       //< Configure the Touch Panel Sample Rate per seconds. The allowable range depends on the ADC OCLK_FREQ.
	ADC_TP_CONFIG_ID_OPERATE_MODE,      //< Configure the Touch Panel operating mode. Please use "ADC_TP_OPMODE" and the default value is ADC_TP_OPMODE_AUTO_NORMAL.
	ADC_TP_CONFIG_ID_INTEN,             //< Configure the Touch Panel interrupt enable. Please uses "ENABLE" or "DISABLE" as input parameter.
	ADC_TP_CONFIG_ID_ISR_CALLBACK,      //< Configure the Touch Panel interrupt ISR callback handler.
	ADC_TP_CONFIG_ID_CURRENT_SEL,       //< Configure the Touch Panel pressure measure current source. Please uses ADC_TP_CURRENT_200UA or ADC_TP_CURRENT_400UA as input parameter.

	ADC_TP_CONFIG_ID_FIFO_TH,           //< When TP operate mode as ADC_TP_OPMODE_AUTO_THRESHOLD, this is used to configure the FIFO threshold. Valid range form 0x1 to 0x4.
	ADC_TP_CONFIG_ID_PRESSURE_FILT_EN,  //< Configure the Touch Panel pressure filter functionality enable/disable.
	ADC_TP_CONFIG_ID_PRESSURE_TH,       //< Configure the Touch Panel pressure filter functionality threshold value

	ADC_TP_CONFIG_ID_SYNC_EN,           //< Configure the Touch Panel Sync to IDE functionality Enable/Disable.
	ADC_TP_CONFIG_ID_SYNC_SRC,          //< Configure the Touch Panel Sync to IDE function source from IDE or IDE2. Please use ADC_TP_SYNC as input parameter.

	ADC_TP_CONFIG_ID_PRECHARGE_EN,      //< Enable/Disable Touch Panel PreChare period.
	//< The PreChare period is that the TP state machine would connect to VCC first before PenDown Detect Pull Up Resistor,
	//< this period is used to charge the panel's parasitic capacitance first before using the pull up resistor.
	ADC_TP_CONFIG_ID_PRECHARGE_TIME,    //< If the PreCharge Period is enabled, this is the PreCharge Time length. Unit is 4us per step. Value range 0x00 to 0xFF.

#if defined(_NVT_FPGA_)
	ADC_TP_CONFIG_ID_MS_MODE,           //< Reserved for NVT internal usage.
	ADC_TP_CONFIG_ID_MANUAL_MUX,        //< Reserved for NVT internal usage.
#endif
	ADC_TP_CONFIG_ID_TOTAL,
	ENUM_DUMMY4WORD(ADC_TP_CONFIG_ID)
} ADC_TP_CONFIG_ID;


/*
    TP operation mode

    (OBSOLETE)

    Definition for TP operation.

    @note Used in adc_setTpConfig(ADC_TP_CONFIG_ID_OPERATE_MODE).
*/
typedef enum {
	ADC_TP_OPMODE_MANUAL,               //< Reserved for NVT internal usage only. Not allow setting to Manual mode.
	ADC_TP_OPMODE_AUTO_NORMAL,          //< Select ADC to Auto Mode. Driver Default Value.
	ADC_TP_OPMODE_AUTO_THRESHOLD,       //< Select ADC to Auto Mode with FIFO Threshold.

	ENUM_DUMMY4WORD(ADC_TP_OPMODE)
} ADC_TP_OPMODE;


/**
    The ADC interrupt identification IDs.

    These definitions can be used in the adc_waitInterrupt() to specify the waited interrupt events.
*/
typedef enum {
	ADC_INTERRUPT_READY_CH0 = 0x00000001,       ///< ADC Channel 0 Data Ready
	ADC_INTERRUPT_READY_CH1 = 0x00000002,       ///< ADC Channel 1 Data Ready
	ADC_INTERRUPT_READY_CH2 = 0x00000004,       ///< ADC Channel 2 Data Ready
	ADC_INTERRUPT_READY_CH3 = 0x00000008,       ///< ADC Channel 3 Data Ready
	ADC_INTERRUPT_VALTRIG_0 = 0x00000010,       ///< ADC Channel 0 Level trigger ready
	ADC_INTERRUPT_VALTRIG_1 = 0x00000020,       ///< ADC Channel 1 Level trigger ready
	ADC_INTERRUPT_VALTRIG_2 = 0x00000040,       ///< ADC Channel 2 Level trigger ready

	ENUM_DUMMY4WORD(ADC_INTERRUPT)
} ADC_INTERRUPT;

/**
    ADC thermal average counts

    Definition ADC thermal average counts every output sample. 

    @note Used in adc_setConfig(ADC_CONFIG_ID_THERMAL_AVG, ADC_THERMAL_AVG_2 ~ 128).
*/
typedef enum {
	ADC_THERMAL_AVG_2 = 0,    ///<   2 times average.
	ADC_THERMAL_AVG_4,        ///<   4 times average.
	ADC_THERMAL_AVG_8,        ///<   8 times average.
	ADC_THERMAL_AVG_16,       ///<  16 times average.
	ADC_THERMAL_AVG_32,       ///<  32 times average.
	ADC_THERMAL_AVG_64,       ///<  64 times average.
	ADC_THERMAL_AVG_128,      ///< 128 times average.
	ADC_THERMAL_AVG_256,      ///< 256 times average.
} ADC_THERMAL_AVG;

/*
    The Touch Panel interrupt identification IDs.

    (OBSOLETE)

    These definitions can be used in the callback function to identify which touch panel's interrupt is issued.
*/
typedef enum {
	ADC_TP_INTERRUPT_PENDOWN = 0x00004000,      //< Touch Panel Pen-Down interrupt
	ADC_TP_INTERRUPT_PENUP   = 0x00008000,      //< Touch Panel Pen-Up interrupt
	ADC_TP_INTERRUPT_READY   = 0x00001000,      //< Touch Panel Data-Ready interrupt
	ADC_TP_INTERRUPT_TH_HIT  = 0x00100000,      //< Touch Panel Data-Ready Fifo Threshold Hit interrupt
	ADC_TP_INTERRUPT_FIFO_OVR = 0x00400000,     //< Touch Panel Fifo OverRun interrupt

	ENUM_DUMMY4WORD(ADC_TP_INTERRUPT)
} ADC_TP_INTERRUPT;


/*
    Select the current source in presure measurement.

    (OBSOLETE)

    @note Used in adc_setTpConfig(ADC_TP_CONFIG_ID_CURRENT_SEL)
*/
typedef enum {
	ADC_TP_CURRENT_200UA,   //< Set presure measure current source as 200uA.
	ADC_TP_CURRENT_400UA,   //< Set presure measure current source as 400uA.

	ENUM_DUMMY4WORD(ADC_TP_CURRENT)
} ADC_TP_CURRENT;


/*
    The structure for touch panel fifo data.

    (OBSOLETE)

    This is used in the touch panel fifo threshold mode.

    @note Used i the adc_readTpFifo().
*/
typedef struct {
	UINT32 uiEntry;     //< Number of Valid Entries

	UINT32 uiPosX[4];   //< Position X FIFO values
	UINT32 uiPosY[4];   //< Position Y FIFO values
	UINT32 uiPre1[4];   //< Pressure 1 FIFO values
	UINT32 uiPre2[4];   //< Pressure 2 FIFO values
} ADC_TP_FIFO, *PADC_TP_FIFO;


/*
    Select the TP Synchronization source from IDE or IDE2

    (OBSOLETE)

    @note Used in adc_setTpConfig(ADC_TP_CONFIG_ID_SYNC_SRC)
*/
typedef enum {
	ADC_TP_SYNC_IDE,    //< TP SYNC with IDE
	ADC_TP_SYNC_IDE2,   //< TP SYNC with IDE2

	ENUM_DUMMY4WORD(ADC_TP_SYNC)
} ADC_TP_SYNC;


/**
    Configuration ID for ADC wakeup CPU event condition.

    After CPU entering power down mode, the ADC can wakeup the CPU by the specified conditions.

    @note Used in the adc_setWakeupConfig().
*/
typedef enum {
	ADC_WAKEUP_CONFIG_ENABLE,   ///< Set ADC Wakeup CPU source enable. Parameter uses "Bit-Wise OR" of the ADC_WAKEUP_SRC.
	ADC_WAKEUP_CONFIG_DISABLE,  ///< Set ADC Wakeup CPU source disable. Parameter uses "Bit-Wise OR" of the ADC_WAKEUP_SRC.


	ENUM_DUMMY4WORD(ADC_WAKEUP_CONFIG)
} ADC_WAKEUP_CONFIG;


/**
    Wakeup Source Options of the ADC module

    @note Used in the adc_setWakeupConfig(ADC_WAKEUP_CONFIG_ENABLE / ADC_WAKEUP_CONFIG_DISABLE).
*/
typedef enum {
	ADC_WAKEUP_SRC_ADCCH_0      = 0x00000010,   ///< Set ADC Wakeup CPU condition as ADC_CHANNEL_0 Value Trigger Interrupt
	ADC_WAKEUP_SRC_ADCCH_1      = 0x00000020,   ///< Set ADC Wakeup CPU condition as ADC_CHANNEL_1 Value Trigger Interrupt
	ADC_WAKEUP_SRC_ADCCH_2      = 0x00000040,   ///< Set ADC Wakeup CPU condition as ADC_CHANNEL_2 Value Trigger Interrupt

	ADC_WAKEUP_SRC_MASK         = 0x00000070,
	ENUM_DUMMY4WORD(ADC_WAKEUP_SRC)
} ADC_WAKEUP_SRC;




//
// Export ADC APIs
//
extern ER               adc_open(ADC_CHANNEL Channel);
extern ER               adc_close(ADC_CHANNEL Channel);
extern BOOL             adc_isOpened(ADC_CHANNEL Channel);

extern ER               adc_setEnable(BOOL bEn);
extern BOOL             adc_getEnable(void);

extern void             adc_setConfig(ADC_CONFIG_ID CfgID, UINT32 uiCfgValue);
extern UINT32           adc_getConfig(ADC_CONFIG_ID CfgID);

extern void             adc_setChConfig(ADC_CHANNEL Channel, ADC_CH_CONFIG_ID CfgID, UINT32 uiCfgValue);
extern UINT32           adc_getChConfig(ADC_CHANNEL Channel, ADC_CH_CONFIG_ID CfgID);

extern void             adc_triggerOneShot(ADC_CHANNEL Channel);

extern UINT32           adc_readData(ADC_CHANNEL Channel);
extern BOOL             adc_isDataReady(ADC_CHANNEL Channel);
extern void             adc_clearDataReady(ADC_CHANNEL Channel);
extern UINT32           adc_readVoltage(ADC_CHANNEL Channel);

extern UINT32           adc_thermal_read_data(void);
extern UINT32           adc_thermal_read_voltage(void);

extern void             adc_setCaliOffset(UINT32 Offset, UINT32 CalVDDADC);
extern ER               adc_setWakeupConfig(ADC_WAKEUP_CONFIG CfgID, UINT32 uiParam);

extern ADC_INTERRUPT    adc_waitInterrupt(ADC_INTERRUPT WaitedFlag);

extern void             thermal_channel_init(void);
extern double           adc_thermal_temp(void);

#if defined(_NVT_FPGA_)
#define ADC_SRCCLOCK                1600000
#else
#define ADC_SRCCLOCK               16000000
#endif

#endif
//@}
