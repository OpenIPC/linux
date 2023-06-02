/*
    ADC internal header

    ADC internal header file

    @file       adc_int.h
    @ingroup    mIDrvIO_ADC
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/


#ifndef _ADC_INT_H
#define _ADC_INT_H

//#include "DrvCommon.h"

/**
    @addtogroup mIDrvIO_ADC
*/
//@{

#define ADC_INTERRUPT_READY_MSK     0x000010FF
#define ADC_INTERRUPT_VALTRIG_MSK   0x46000000

#define ADC_TP_FIFO_ENTRY           8

#define ADC_TP_STATUS_BITS          0x0050D000

#define ADC_OCLK_MAX                1000000     //1MHz for Analog Design

// Control register
#define AIN0_CONT_MODE              0x00000001  // Channel 0 continuous mode

// one shot start register
#define AIN0_ONESHOT_START          0x00000001  // Channel 0 one-shot start

// interrupt control register
#define AIN0_INT_EN                 0x0000001   // Channel 0 interrupt enable
#define TP_INT_EN                   0x0000400   // TP interrupt enable

// Status register
#define AIN0_DATAREADY              0x0000001   // Channel 0 data ready
#define TP_DATAREADY                0x0000400   // TP data ready
#define TP_PDSTS                    0x0000100   // TP pen down status

// data register
#define ADC_DATA_MASK               0x000003FF  // ADC data register only have 12 bits valid data

#define ADC_XDATA_MASK              0x000003FF  // Position x data register only have 12 bits valid data
#define ADC_XDATA_SHT               0           // Position x data shiter

#define ADC_YDATA_MASK              0x03FF0000  // Position y data register only have 12 bits valid data
#define ADC_YDATA_SHT               16          // Position y data shiter

#define ADC_P1DATA_MASK             0x000003FF  // Pressurement 1 data register only have 12 bits valid data
#define ADC_P1DATA_SHT              0           // Pressurement 1 data shiter

#define ADC_P2DATA_MASK             0x03FF0000  // Pressurement 2 data register only have 12 bits valid data
#define ADC_P2DATA_SHT              16          // Pressurement 2 data shiter


//  Definition for ADC channel sampling divider range
#define ADC_CHSAMPDIV_MIN           0x0         // Minimum channel sampling divider value
#define ADC_CHSAMPDIV_MAX           0xFF        // Maximum channel sampling divider value

//  Definition for ADC channel external sampling counts range
#define ADC_EXTCNT_MIN              0x0         // Minimum channel external sampling counts value
#define ADC_EXTCNT_MAX              0xF         // Maximum channel external sampling counts value

//  Definition for thermal channel average count
#define ADC_THERMAL_AVG_MIN         0x0         // Minimum average sampling counts value 
#define ADC_THERMAL_AVG_MAX         0x7         // Maximum average sampling counts value


//  Definition for internal resistor range
#define TP_INTRES_MIN               0x0         // Minimum internal resistor value
#define TP_INTRES_MAX               0x3F        // Maximum internal resistor value

//    Definition for TP sampling cycle range
#define TP_SAMPCYC_MIN              0x0         // Minimum sampling cycle value
#define TP_SAMPCYC_MAX              0xFF        // Maximum sampling cycle value

//  Definition for TP pen-down gsr range
#define TP_PDGSR_MIN                0x0         // Minimum pen-down gsr value
#define TP_PDGSR_MAX                0xFF        // Maximum pen-down gsr value


// TP switch register
#define PXP_PXG_MOD_MASK        0x00000003
#define PXP_OFF_PXG_OFF         0x00000000
#define PXP_OFF_PXG_ON          0x00000001
#define PXP_ON_PXG_OFF          0x00000002
#define MXP_MXG_MOD_MASK        0x0000000C
#define MXP_OFF_MXG_OFF         0x00000000
#define MXP_OFF_MXG_ON          0x00000004
#define MXP_ON_MXG_OFF          0x00000008
#define PYP_PYG_MOD_MASK        0x00000030
#define PYP_OFF_PYG_OFF         0x00000000
#define PYP_OFF_PYG_ON          0x00000010
#define PYP_ON_PYG_OFF          0x00000020
#define MYP_MYG_MOD_MASK        0x000000C0
#define MYP_OFF_MYG_OFF         0x00000000
#define MYP_OFF_MYG_ON          0x00000040
#define MYP_ON_MYG_OFF          0x00000080
#define S_MASK                  0x00001F00
#define S1_ON                   0x00000100
#define S2_ON                   0x00000200
#define S3_ON                   0x00000400
#define S4_ON                   0x00000800
#define S5_ON                   0x00001000

#define ADC_TPSW_ALL_SW_MASK    (PXP_PXG_MOD_MASK|MXP_MXG_MOD_MASK|PYP_PYG_MOD_MASK|MYP_MYG_MOD_MASK)
#define ADC_TPSW_PD0_SW         (PXP_ON_PXG_OFF|MXP_ON_MXG_OFF|PYP_OFF_PYG_ON|MYP_OFF_MYG_ON|S2_ON|S4_ON|S5_ON)
#define ADC_TPSW_XP_SW          (PXP_ON_PXG_OFF|MXP_OFF_MXG_ON|PYP_OFF_PYG_OFF|MYP_OFF_MYG_OFF|S1_ON|S3_ON)
#define ADC_TPSW_YP_SW          (PXP_OFF_PXG_OFF|MXP_OFF_MXG_OFF|PYP_ON_PYG_OFF|MYP_OFF_MYG_ON|S2_ON|S4_ON)
#define ADC_TPSW_P1_SW          (PXP_OFF_PXG_OFF|MXP_OFF_MXG_ON|PYP_ON_PYG_OFF|MYP_OFF_MYG_OFF|S4_ON)
#define ADC_TPSW_P2_SW          (PXP_OFF_PXG_OFF|MXP_OFF_MXG_ON|PYP_ON_PYG_OFF|MYP_OFF_MYG_OFF|S1_ON)
#define ADC_TPSW_XRes_SW        (PXP_ON_PXG_OFF|MXP_OFF_MXG_ON|PYP_OFF_PYG_OFF|MYP_OFF_MYG_OFF|S4_ON)
#define ADC_TPSW_YRes_SW        (PXP_OFF_PXG_OFF|MXP_OFF_MXG_OFF|PYP_ON_PYG_OFF|MYP_OFF_MYG_ON|S3_ON)
#define ADC_TPSW_Gen_SW         (PXP_OFF_PXG_OFF|MXP_OFF_MXG_OFF|PYP_OFF_PYG_OFF|MYP_OFF_MYG_OFF|S1_ON|S4_ON)
#define ADC_TPSW_XRight_Cal_SW  (PXP_ON_PXG_OFF|MXP_OFF_MXG_ON|PYP_OFF_PYG_OFF|MYP_OFF_MYG_OFF|S4_ON)
#define ADC_TPSW_XLeft_Cal_SW   (PXP_ON_PXG_OFF|MXP_OFF_MXG_ON|PYP_OFF_PYG_OFF|MYP_OFF_MYG_OFF|S2_ON)
#define ADC_TPSW_YTop_Cal_SW    (PXP_OFF_PXG_OFF|MXP_OFF_MXG_OFF|PYP_ON_PYG_OFF|MYP_OFF_MYG_ON|S3_ON)
#define ADC_TPSW_YDown_Cal_SW   (PXP_OFF_PXG_OFF|MXP_OFF_MXG_OFF|PYP_ON_PYG_OFF|MYP_OFF_MYG_ON|S1_ON)

#define ADC_TPSW_ADC6           (PXP_OFF_PXG_OFF|MXP_OFF_MXG_OFF|PYP_OFF_PYG_OFF|MYP_OFF_MYG_OFF|S4_ON)
#define ADC_TPSW_ADC7           (PXP_OFF_PXG_OFF|MXP_OFF_MXG_OFF|PYP_OFF_PYG_OFF|MYP_OFF_MYG_OFF|S1_ON)
#define ADC_TPSW_YP             (PXP_OFF_PXG_OFF|MXP_OFF_MXG_OFF|PYP_OFF_PYG_OFF|MYP_OFF_MYG_OFF|S3_ON)
#define ADC_TPSW_XM             (PXP_OFF_PXG_OFF|MXP_OFF_MXG_OFF|PYP_OFF_PYG_OFF|MYP_OFF_MYG_OFF|S2_ON)


//#define S6_ON              0x00002000
//#define ADC_TPSW_PD1_SW       (PXP_OFF_PXG_ON|MXP_OFF_MXG_ON|PYP_ON_PYG_OFF|MYP_ON_MYG_OFF|S1_ON|S3_ON|S6_ON)


#define ADC_RES_BITS            9
#define ADC_MAXVALUE            ((0x01<<ADC_RES_BITS)-1)

#define ADC_STS_VALTRIG_OFS     4
#define ADC_VALTRIG_RANGE_OFS   4
#define ADC_VALTRIG_MODE_OFS    8
#define ADC_VALTRIG_CTRL_MASK1  0x02020000
#define ADC_VALTRIG_CTRL_MASK2  0x04040000
#define ADC_VALTRIG_MASK        0xFFFF
#define ADC_VALTRIG_MAX         0x1FF

/*
    Definition for ADC value trigger channel ID

    Used to specify which of the value trigger ADC channel.
*/
typedef enum {
	ADC_VALTRIG_CH1,            // ADC value trigger channel 1
	ADC_VALTRIG_CH2,            // ADC value trigger channel 2
	ADC_VALTRIG_CH6,            // ADC value trigger channel 6

	ADC_VALTRIG_TOTAL_CH,

	ADC_VALTRIG_CH0,            // ADC value trigger channel 0

	ENUM_DUMMY4WORD(ADC_VALTRIG)
} ADC_VALTRIG;

/*
    For ADC Wakeup CPU Original Parameter stroe
*/
typedef enum {
	ADCWU_SOURCE,
	ADCWU_ORIGIN_INTEN,
	ADCWU_ORIGIN_CTRL_REG,
	ADCWU_ORIGIN_DIVIDER,
	ADCWU_ORIGIN_GSR_PRECHG,

	ADCWU_PARAM_MAX,
	ENUM_DUMMY4WORD(ADCWU_PARAM)
} ADCWU_PARAM;



/*
    This Debug Function is designed for ADC/TP verification usage
*/
#define ADC_DEBUG                   DISABLE



extern ER               adc_openTp(void);
extern ER               adc_closeTp(void);
extern BOOL             adc_isTpOpened(void);

extern ER               adc_setTpEnable(BOOL bEn);
extern BOOL             adc_getTpEnable(void);

extern void             adc_setTpConfig(ADC_TP_CONFIG_ID CfgID, UINT32 uiCfgValue);
extern UINT32           adc_getTpConfig(ADC_TP_CONFIG_ID CfgID);

extern void             adc_readTPPressure(UINT16 *puiP1, UINT16 *puiP2);
extern void             adc_readTPXY(UINT16 *puiX, UINT16 *puiY);
extern void             adc_readTpFifo(PADC_TP_FIFO pTpFifo);

extern BOOL             adc_isTPPD(void);

extern BOOL             adc_isTPDataReady(void);
extern void             adc_clearTPDataReady(void);

#endif

//@}

