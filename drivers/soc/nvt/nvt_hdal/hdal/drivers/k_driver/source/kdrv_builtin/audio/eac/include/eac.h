/**
    Embedded Audio Codec (EAC) Driver Public Header File

    This file is the public header file for Embedded Audio Codec(EAC).
    The user can reference this section for the detail description of the each driver API usage
    and also the parameter descriptions and its limitations.
    The overall combinational usage flow is introduced in the application note document,
    and the user must reference to the application note for the driver usage flow.

    @file       eac.h
    @ingroup    mIDrvAud_EAC
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.
*/

#ifndef _EAC_H
#define _EAC_H

#ifdef __KERNEL__
#include "kwrap/type.h"
#elif defined(__FREERTOS)
#include "kwrap/type.h"
#endif

/**
    @addtogroup mIDrvAud_EAC
*/
//@{

/**
    Embedded Audio Codec(EAC) Analog to Digital (Record) Configuration Selection

    This definition is used in eac_setAdConfig() to select which of the recording function is selected to assign new configuration.
    When configuring the PGA Gain (EAC_CONFIG_AD_PGAGAIN_L and EAC_CONFIG_AD_PGAGAIN_R), the setting is valid only
    if the ALC is disabled becuase the PGA gain would under ALC control if ALC is enabled.
    The user should also notice that the Digital Gain 1 is under ALC control, so the adjustment of the Digital Gain 1
    may be restored back from the PGA gain if the ALC target is unchanged.
    Besides, after changing the settings of EAC_CONFIG_AD_DGAIN1_L / EAC_CONFIG_AD_DGAIN1_R /EAC_CONFIG_AD_DGAIN2_L / EAC_CONFIG_AD_DGAIN2_R
    / EAC_CONFIG_AD_ALC_TARGET / EAC_CONFIG_AD_PGAGAIN_L / EAC_CONFIG_AD_PGAGAIN_R, the user must also call eac_setLoad() to activate the new settings.
    The user can reference to the eac application note for the basic functional blocks of the embedded audio codec functional blocks.
*/
typedef enum {
	EAC_CONFIG_AD_DMIC_EN,          ///< Configure AD(Record) Digital Microphone Enable/Disable.
	EAC_CONFIG_AD_DMIC_CLK_EN,      ///< Configure AD(Record) Digital Microphone Clock Output Enable/Disable.
	EAC_CONFIG_AD_DMIC_LRSWAP,      ///< Configure AD(Record) Digital Microphone Left/Right Channel Swap.
	EAC_CONFIG_AD_DMIC2_LRSWAP,     ///< Configure AD(Record) Digital Microphone2 Left/Right Channel Swap.
	EAC_CONFIG_AD_ALC_MODE_DGAIN,   ///< Configure AD(Record) Auto Level Control Using Ditial Gain only.
	EAC_CONFIG_AD_DMIC_PATH_MUX,	///< Configure AD(Record) Digital Microphone decimation filter mux.
	EAC_CONFIG_AD_DMIC_CHANNEL,     ///< Configure AD(Record) Digital Microphone channel number 0x0/0x1 for 2/4ch.
	///< This is used for the Digital Microphone.

	EAC_CONFIG_AD_DCCAN_EN,         ///< Configure AD(Record) DC Cancellation Enable/Disable.
	EAC_CONFIG_AD_DCCAN_TYPE,       ///< Configure AD(Record) DC Cancellation Selection Options. Please use EAC_DCCAN_TYPE as input parameter.
	EAC_CONFIG_AD_DCCAN_RESO,       ///< Configure AD(Record) DC Cancellation Resolution. Please use EAC_DCCAN_RESO as input parameter.
	EAC_CONFIG_AD_DCINIT_L,         ///< Configure AD(Record) DC Cancellation Left  Channel Inital Value. This value is used by EAC_CONFIG_AD_DCCAN_TYPE.
	EAC_CONFIG_AD_DCINIT_R,         ///< Configure AD(Record) DC Cancellation Right Channel Inital Value. This value is used by EAC_CONFIG_AD_DCCAN_TYPE.
	EAC_CONFIG_AD_DMIC2_DCINIT_L,   ///< Configure AD(Record) DC Cancellation Left  Channel Inital Value. This value is used by EAC_CONFIG_AD_DCCAN_TYPE.(dmic2)
	EAC_CONFIG_AD_DMIC2_DCINIT_R,   ///< Configure AD(Record) DC Cancellation Right Channel Inital Value. This value is used by EAC_CONFIG_AD_DCCAN_TYPE.(dmic2)
	EAC_CONFIG_AD_DGAIN1_L,         ///< Configure AD(Record) Digital Gain 1 for Left channel. Value range 0x1~0xFF which mapping to -57.0dB ~ +70.0dB(+0.5dB each step). Value 0x0 is mute.
	EAC_CONFIG_AD_DGAIN1_R,         ///< Configure AD(Record) Digital Gain 1 for Right channel. Value range 0x1~0xFF which mapping to -57.0dB ~ +70.0dB(+0.5dB each step). Value 0x0 is mute.
	EAC_CONFIG_AD_DGAIN2_L,         ///< Configure AD(Record) Digital Gain 2 for Left channel. Value range 0x1~0xFF which mapping to -97.0dB ~ +30.0dB(+0.5dB each step). Value 0x0 is mute.
	EAC_CONFIG_AD_DGAIN2_R,         ///< Configure AD(Record) Digital Gain 2 for Right channel. Value range 0x1~0xFF which mapping to -97.0dB ~ +30.0dB(+0.5dB each step). Value 0x0 is mute.
	EAC_CONFIG_AD_DMIC2_DGAIN_L,    ///< Configure AD(Record) Digital Gain 1 for dmic2 Left channel. Value range 0x1~0xFF which mapping to -57.0dB ~ +70.0dB(+0.5dB each step). Value 0x0 is mute.
	EAC_CONFIG_AD_DMIC2_DGAIN_R,    ///< Configure AD(Record) Digital Gain 1 for dmic2 Right channel. Value range 0x1~0xFF which mapping to -57.0dB ~ +70.0dB(+0.5dB each step). Value 0x0 is mute.


	EAC_CONFIG_AD_ALC_EN,           ///< Configure AD(Record) Auto Level Control (ALC) Enable/Disable.
	EAC_CONFIG_AD_ALC_TARGET_L,     ///< Configure AD(Record) Auto Level Control (ALC) Target Level for  Left Channel. Use the enumeration "EAC_ALC_TARGET" as input parameter.
	EAC_CONFIG_AD_ALC_TARGET_R,     ///< Configure AD(Record) Auto Level Control (ALC) Target Level for Right Channel. Use the enumeration "EAC_ALC_TARGET" as input parameter.

	EAC_CONFIG_AD_ALC_MAXGAIN_L,    ///< Configure AD(Record) Auto Level Control (ALC) PGA Max Gain for  Left Channel. Use the enumeration "EAC_ALC_MAXGAIN" as input parameter.
	EAC_CONFIG_AD_ALC_MAXGAIN_R,    ///< Configure AD(Record) Auto Level Control (ALC) PGA Max Gain for Right Channel. Use the enumeration "EAC_ALC_MAXGAIN" as input parameter.

	EAC_CONFIG_AD_ALC_MINGAIN_L,    ///< Configure AD(Record) Auto Level Control (ALC) PGA Max Gain for  Left Channel. Use the enumeration "EAC_ALC_MINGAIN" as input parameter.
	EAC_CONFIG_AD_ALC_MINGAIN_R,    ///< Configure AD(Record) Auto Level Control (ALC) PGA Max Gain for Right Channel. Use the enumeration "EAC_ALC_MINGAIN" as input parameter.

	EAC_CONFIG_AD_ALC_ATTACK_TIME,  ///< Configure AD(Record) Auto Level Control (ALC) Attack Time. Time length would be "(2 ^ uiCfgValue) x ALC_Time_Resolution_Basis". Default value is 0x2.
									///< This setting is also valid in the noise gate state. If the user want to keep the same attack time above noise gate threshold but larger the attack time below noise gate threshold,
									///< the user must increase time resolution of NoiseGate and shorten the time resolution of ALC.
	EAC_CONFIG_AD_ALC_DECAY_TIME,   ///< Configure AD(Record) Auto Level Control (ALC) Decay Time. Time length would be "(2 ^ uiCfgValue) x ALC_Time_Resolution_Basis". Default value is 0x3.
									///< This setting is also valid in the noise gate state. If the user want to keep the same decay time above noise gate threshold but larger the decay time below noise gate threshold,
									///< the user must increase time resolution of NoiseGate and shorten the time resolution of ALC.
	EAC_CONFIG_AD_ALC_HOLD_TIME,    ///< Configure AD(Record) Auto Level Control (ALC) HOLD Time. Time length would be "(2 ^ (uiCfgValue-1)) x 117" samples. uiCfgValue 0x0 is 1 sample. Default value is 0x0.
									///< This setting is also valid in the noise gate state. If the user want to keep the same hold time above noise gate threshold but larger the hold time below noise gate threshold,
									///< the user must increase time resolution of NoiseGate and shorten the time resolution of ALC.

	EAC_CONFIG_AD_NG_ATTACK_TIME,   ///< Configure AD(Record) Auto Level Control (ALC) Attack Time. Time length would be "(2 ^ uiCfgValue) x ALC_Time_Resolution_Basis". Default value is 0x2.
									///< This setting is also valid in the noise gate state. If the user want to keep the same attack time above noise gate threshold but larger the attack time below noise gate threshold,
									///< the user must increase time resolution of NoiseGate and shorten the time resolution of ALC.
	EAC_CONFIG_AD_NG_DECAY_TIME,    ///< Configure AD(Record) Auto Level Control (ALC) Decay Time. Time length would be "(2 ^ uiCfgValue) x ALC_Time_Resolution_Basis". Default value is 0x3.
									///< This setting is also valid in the noise gate state. If the user want to keep the same decay time above noise gate threshold but larger the decay time below noise gate threshold,
									///< the user must increase time resolution of NoiseGate and shorten the time resolution of ALC.
	EAC_CONFIG_AD_NG_HOLD_TIME,     ///< Configure AD(Record) Auto Level Control (ALC) HOLD Time. Time length would be "(2 ^ (uiCfgValue-1)) x 117" samples. uiCfgValue 0x0 is 1 sample. Default value is 0x0.
									///< This setting is also valid in the noise gate state. If the user want to keep the same hold time above noise gate threshold but larger the hold time below noise gate threshold,
									///< the user must increase time resolution of NoiseGate and shorten the time resolution of ALC.

	EAC_CONFIG_AD_ALC_STEP,         ///< Configure AD(Record) Auto Level Control (ALC) Step value. This value would constraint the ALC controllable PGA gain changement max value in eack zero crossing. value is "0.5+(1.5 x uiCfgValue)" dB.
	EAC_CONFIG_AD_ALC_TRESO,        ///< Configure AD(Record) Auto Level Control (ALC) Time Resolution Basis during Attack/Decay time calculation.
									///< Please use "EAC_ALC_TRESO_BASIS" as input parameter. The default value is 15ms.
	EAC_CONFIG_AD_ALC_NG_EN,        ///< Configure AD(Record) Auto Level Control (ALC) Noise Gate Enable/Disable.

	EAC_CONFIG_AD_ALC_NG_THD_L,     ///< Configure AD(Record) Auto Level Control (ALC) Noise Gate Threshold value for  left channel. Please use the enumeration "EAC_NG_THRESHOLD" as input parameter.
	EAC_CONFIG_AD_ALC_NG_THD_R,     ///< Configure AD(Record) Auto Level Control (ALC) Noise Gate Threshold value for right channel. Please use the enumeration "EAC_NG_THRESHOLD" as input parameter.

	EAC_CONFIG_AD_ALC_NG_TARGET_L,  ///< Configure AD(Record) Auto Level Control (ALC) Noise Gate Target Level for  left channel. Value range 0~15 which the Noise Gate target dB would be (ALC_TARGET - (NG_THD - INPUT_SIGNAL_LEVEL)*uiCfgValue).
	EAC_CONFIG_AD_ALC_NG_TARGET_R,  ///< Configure AD(Record) Auto Level Control (ALC) Noise Gate Target Level for right channel. Value range 0~15 which the Noise Gate target dB would be (ALC_TARGET - (NG_THD - INPUT_SIGNAL_LEVEL)*uiCfgValue).

	EAC_CONFIG_AD_ALC_NG_TRESO,     ///< Configure AD(Record) Auto Level Control (ALC) Noise Gate Time Resolution Basis during Attack/Decay time calculation inside Noise Gate state.
									///< Please use "EAC_ALC_TRESO_BASIS" as input parameter. The default value is 15ms.
	EAC_CONFIG_AD_BOOST_COMPEN_L,   ///< Configure AD(Record) Boost Gain Digital Compensation for left channel in ALC loop. please use EAC_ALC_BOOST_COMPEN as input parameter.
	EAC_CONFIG_AD_BOOST_COMPEN_R,   ///< Configure AD(Record) Boost Gain Digital Compensation for right channel in ALC loop. please use EAC_ALC_BOOST_COMPEN as input parameter.
	EAC_CONFIG_AD_IIR_ALC_L,        ///< Configure AD(Record) Enable/Disable the IIR function to the Auto Level Control (ALC)  Left Channel Input.
	EAC_CONFIG_AD_IIR_ALC_R,        ///< Configure AD(Record) Enable/Disable the IIR function to the Auto Level Control (ALC) Right Channel Input.
	EAC_CONFIG_AD_IIR_OUT_L,        ///< Configure AD(Record) Enable/Disable the IIR function to the Recorded  Left Channel output Samples.
	EAC_CONFIG_AD_IIR_OUT_R,        ///< Configure AD(Record) Enable/Disable the IIR function to the Recorded Right Channel output Samples.
	EAC_CONFIG_AD_IIR2_OUT_L,       ///< Configure AD(Record) Enable/Disable the IIR function to the Recorded  Left Channel output Samples.(dmic2)
	EAC_CONFIG_AD_IIR2_OUT_R,       ///< Configure AD(Record) Enable/Disable the IIR function to the Recorded Right Channel output Samples.(dmic2)

	EAC_CONFIG_AD_PGAGAIN_L,        ///< Configure AD(Record) Analog PGA Gain for Left channel if ALC disabled. Please use the enumeration "EAC_AD_PGAGAIN" as input parameter.
	EAC_CONFIG_AD_PGAGAIN_R,        ///< Configure AD(Record) Analog PGA Gain for Right channel if ALC disabled. Please use the enumeration "EAC_AD_PGAGAIN" as input parameter.

	EAC_CONFIG_AD_PGABOOST_L,       ///< Configure AD(Record) Analog Boost Gain for  left channel. Please use EAC_PGABOOST_SEL as input parameter.
	EAC_CONFIG_AD_PGABOOST_R,       ///< Configure AD(Record) Analog Boost Gain for right channel. Please use EAC_PGABOOST_SEL as input parameter.

	EAC_CONFIG_AD_POWER_EN_L,       ///< Configure AD(Record) Analog Power Enable of the Left channel. Use TRUE/FALSE as input parameter to set Enable/Disable power.
	EAC_CONFIG_AD_POWER_EN_R,       ///< Configure AD(Record) Analog Power Enable of the Right channel. Use TRUE/FALSE as input parameter to set Enable/Disable power.
	EAC_CONFIG_AD_RESET,            ///< Configure AD(Record) Analog Block Reset. Set TRUE/FALSE to Enable/Disable the analog block reset operation.
	EAC_CONFIG_AD_PDREF_BUF,        ///< Configure AD(Record) Analog Block Reference Buffer Power Down.
	EAC_CONFIG_AD_PDREF_BIAS,       ///< Configure AD(Record) Analog Block Reference Bias Power Down.
	EAC_CONFIG_AD_PD_VCMBIAS,       ///< Configure AD(Record) Analog Block Vcm Power Down.

	EAC_CONFIG_AD_MICBIAS_EN,       ///< Configure AD(Record) Analog Block MIC Bias Enable/Disable.
    EAC_CONFIG_AD_MICBIAS_LVL,      ///< Configure AD(Record) Analog Block MIC Bias Level 2V/2.4V.
	EAC_CONFIG_AD_DCOFS_L,          ///< Get AD(Record) DC Cancellation Left  Channel Offset Value.
	EAC_CONFIG_AD_DCOFS_R,          ///< Get AD(Record) DC Cancellation Right Channel Offset Value.
	EAC_CONFIG_AD_DMIC2_DCOFS_L,    ///< Get AD(Record) DC Cancellation Left  Channel Offset Value.(dmic2)
	EAC_CONFIG_AD_DMIC2_DCOFS_R,    ///< Get AD(Record) DC Cancellation Right Channel Offset Value.(dmic2)
	EAC_CONFIG_AD_ZC_EN,            ///< Get AD(Record) Zero Crossing Function Enable/Disable.
	EAC_CONFIG_AD_ZC_TIMEOUT,       ///< Get AD(Record) Zero Crossing Function Timeout Value.

	ENUM_DUMMY4WORD(EAC_CONFIG_AD)
} EAC_CONFIG_AD;

/**
    Embedded Audio Codec(EAC) Digital to Analog (Playback) Configuration Selection

    This definition is used in eac_set_da_config() to select which of the playback function is selected to assign new configuration.
    Besides, after changing the settings of EAC_CONFIG_DA_DGAIN_L / EAC_CONFIG_DA_DGAIN_R / EAC_CONFIG_DA_LINEOUT_GAIN / EAC_CONFIG_DA_SPEAKER_GAIN
    / EAC_CONFIG_DA_HEADPHONE_GAIN, the user must also call eac_set_load() to activate the new settings.
    The user can reference to the eac application note for the basic functional blocks of the embedded audio codec functional blocks.
*/
typedef enum {
	EAC_CONFIG_DA_DGAIN_L   = 0x80, ///< Configure DA(Playback) Digital Gain for Left channel. Value range 0x4B~0xFF which mapping to -90.0dB ~ 0dB(+0.5dB each step). Value Below 0x4B is mute.
	EAC_CONFIG_DA_DGAIN_R,          ///< Configure DA(Playback) Digital Gain for Right channel. Value range 0x4B~0xFF which mapping to -90.0dB ~ 0dB(+0.5dB each step). Value Below 0x4B is mute.
	EAC_CONFIG_DA_DATAMIXER,        ///< Configure DA(Playback) Digital Data Mixer. Please use EAC_DA_MIXER_LR_INDEPENDENT or EAC_DA_MIXER_LR_AVERAGE as input parameter.
	EAC_CONFIG_DA_DCCAN_EN,         ///< Configure DA(Playback) DC Cancellation Enable/Disable.
	EAC_CONFIG_DA_DCCAN_RESO,       ///< Configure DA(Playback) DC Cancellation Resolution. Please use EAC_DCCAN_RESO as input parameter.

	EAC_CONFIG_DA_LINEOUT_GAIN,     ///< Configure DA(Playback) Analog Gain for Lineout path. NT96660 does not has lineout, so this would map to headphone path.
	EAC_CONFIG_DA_SPEAKER_GAIN,     ///< Configure DA(Playback) Analog Gain for Speaker path. Please use the enumeration "EAC_DA_PGAGAIN" as input parameter.
	EAC_CONFIG_DA_HEADPHONE_GAIN,   ///< Configure DA(Playback) Analog Gain for HeadPhone path. Please use the enumeration "EAC_DA_PGAGAIN" as input parameter.
	EAC_CONFIG_DA_POWER_EN_L,       ///< Configure DA(Playback) Analog Power Enable of the Left channel. Use TRUE/FALSE as input parameter to set Enable/Disable power.
	EAC_CONFIG_DA_POWER_EN_R,       ///< Configure DA(Playback) Analog Power Enable of the Right channel. Use TRUE/FALSE as input parameter to set Enable/Disable power.
	EAC_CONFIG_DA_RESET,            ///< Configure DA(Playback) Analog Block Reset. Set TRUE/FALSE to Enable/Disable the analog block reset operation.

	EAC_CONFIG_DA_ZC_EN,            ///< Get DA(Playback) Zero Crossing Function Enable/Disable.
	EAC_CONFIG_DA_SPKR_MONO_EN,     ///< Get DA(Playback) Speaker Mono function Enable/Disable

	EAC_CONFIG_DA_OSR_SEL,			///< Configure DA(Playback) Select the over sampling ration of DAC, please use EAC_OSR_SEL as input parameter.

	EAC_CONFIG_DA_DEPOP_EN,         ///< Configure DA(Playback) De-Pop function Enable/Disable
	EAC_CONFIG_DA_DEPOP_PERIOD_H,   ///< Configure DA(Playback) De-Pop function High Level Period
	EAC_CONFIG_DA_DEPOP_PERIOD_L,   ///< Configure DA(Playback) De-Pop function Low Level Period
	EAC_CONFIG_DA_DEPOP_PERIOD_CYC, ///< Configure DA(Playback) De-Pop function Cycle Count

	EAC_CONFIG_DA_DWA_FORCE_ON,     ///< Configure Enable/Disable DA(Playback) Data Weighted Average Force ON to freerun.

	EAC_CONFIG_DA_TEST_EN,          ///< Configure DA(Playback) Test Enable Bit. This would also pull down the DA output.

	ENUM_DUMMY4WORD(EAC_CONFIG_DA)
} EAC_CONFIG_DA;

/**
    DAC output path selection

    DAC output path selection, this is used in eac_set_dac_output() to enable/disable the selected output path.
*/
typedef enum {
	EAC_OUTPUT_SPK,                 ///< DAC output to Speaker
	EAC_OUTPUT_CLSD_SPK,            ///< DAC output to CLASS-D Speaker. No Support for NT96680.
	EAC_OUTPUT_LINE,                ///< DAC output to LINEOUT Stereo
	EAC_OUTPUT_LINE_L,              ///< DAC output to LINEOUT Left Channel
	EAC_OUTPUT_LINE_R,              ///< DAC output to LINEOUT Right Channel

	EAC_OUTPUT_NONE,                ///< None of DAC output
	EAC_OUTPUT_ALL,                 ///< DAC output to Speaker&LINEOUT Stereo Channel

	ENUM_DUMMY4WORD(EAC_OUTPUT)
} EAC_OUTPUT;

/**
    DC Cancellation Type definition

    This field is used in eac_set_ad_config(EAC_CONFIG_AD_DCCAN_TYPE) to specify the DC offset initialization behavior.
    The DC cancellation function is default enabled and the default behavior is EAC_DCCAN_TYPE_KEEP_OPTIMAL.
    The user can use eac_set_ad_config(EAC_CONFIG_AD_DCCAN_EN) to enable/disable the DC cancellation function.
*/
typedef enum {
	EAC_DCCAN_TYPE_AUTO_RESET,      ///< DCCAN Type0: DCCAN would reset the current value after seting eac_set_ad_enable(FALSE).
	EAC_DCCAN_TYPE_KEEP,            ///< DCCAN Type1: DCCAN would not reset the current value.
	EAC_DCCAN_TYPE_KEEP_OPTIMAL,    ///< DCCAN Type2: DCCAN would not reset the current value, and assign 96660 optimal DC initial value to controller.
	EAC_DCCAN_TYPE_RESET_OPTIMAL,   ///< DCCAN Type3: Assign 96660 optimal DC initial value to controller and DCCAN would reset the current value after seting eac_set_ad_enable(FALSE)

	ENUM_DUMMY4WORD(EAC_DCCAN_TYPE)
} EAC_DCCAN_TYPE;

/**
    DC Cancellation (DCCAN) Resolution definition

    The DC Cancellation function would take the average of specified number input samples as DC value and then compensates it.
    The more samples is chosen, the larger of the convergence time of the DC Cancellation.
    But the larger average samples have good performance during very low input frequency such as 10 Hertz.
    This is one of the performance trade-off for DCCAN.
*/
typedef enum {
	EAC_DCCAN_RESO_1024_SAMPLES,    ///< DC Cancellation would take 1024 samples average as DC value.
	EAC_DCCAN_RESO_2048_SAMPLES,    ///< DC Cancellation would take 2048 samples average as DC value. (Default value for AD)
	EAC_DCCAN_RESO_4096_SAMPLES,    ///< DC Cancellation would take 4096 samples average as DC value.
	EAC_DCCAN_RESO_8192_SAMPLES,    ///< DC Cancellation would take 8192 samples average as DC value. (Default value for DA)
	EAC_DCCAN_RESO_16384_SAMPLES,   ///< DC Cancellation would take 16384 samples average as DC value.
	EAC_DCCAN_RESO_32768_SAMPLES,   ///< DC Cancellation would take 32768 samples average as DC value.
	EAC_DCCAN_RESO_65536_SAMPLES,   ///< DC Cancellation would take 65536 samples average as DC value.
	EAC_DCCAN_RESO_131072_SAMPLES,  ///< DC Cancellation would take 131072 samples average as DC value.

	EAC_DCCAN_RESO_MAX_SAMPLES = 10,
	ENUM_DUMMY4WORD(EAC_DCCAN_RESO)
} EAC_DCCAN_RESO;


/**
    Embedded Audio Codec AD analog Boost Select

    This definition is used in eac_set_ad_config(EAC_CONFIG_AD_PGABOOST) to select the AD analog boost gain during audio recording.
*/
typedef enum {
	EAC_PGABOOST_SEL_0DB,           ///< Disable AD analog boost.
	EAC_PGABOOST_SEL_10DB,          ///< Enable  AD analog 10dB boost.
	EAC_PGABOOST_SEL_20DB,          ///< Enable  AD analog 20dB boost.
	EAC_PGABOOST_SEL_30DB,          ///< Enable  AD analog 30dB boost.

	ENUM_DUMMY4WORD(EAC_PGABOOST_SEL)
} EAC_PGABOOST_SEL;

/**
    EAC Analog to Digital PGA Gain Selection

    This definition is used in eac_set_ad_config(EAC_CONFIG_AD_PGAGAIN_L) or eac_set_ad_config(EAC_CONFIG_AD_PGAGAIN_R)
    to specify the AD analog PGA Gain.
*/
typedef enum {
	EAC_AD_PGAGAIN_N21P0_DB,        ///< EAC Analog to Digital PGA Gain as -21.0dB.
	EAC_AD_PGAGAIN_N19P5_DB,        ///< EAC Analog to Digital PGA Gain as -19.5dB.
	EAC_AD_PGAGAIN_N18P0_DB,        ///< EAC Analog to Digital PGA Gain as -18.0dB.
	EAC_AD_PGAGAIN_N16P5_DB,        ///< EAC Analog to Digital PGA Gain as -16.5dB.
	EAC_AD_PGAGAIN_N15P0_DB,        ///< EAC Analog to Digital PGA Gain as -15.0dB.
	EAC_AD_PGAGAIN_N13P5_DB,        ///< EAC Analog to Digital PGA Gain as -13.5dB.
	EAC_AD_PGAGAIN_N12P0_DB,        ///< EAC Analog to Digital PGA Gain as -12.0dB.
	EAC_AD_PGAGAIN_N10P5_DB,        ///< EAC Analog to Digital PGA Gain as -10.5dB.
	EAC_AD_PGAGAIN_N9P0_DB,         ///< EAC Analog to Digital PGA Gain as  -9.0dB.
	EAC_AD_PGAGAIN_N7P5_DB,         ///< EAC Analog to Digital PGA Gain as  -7.5dB.
	EAC_AD_PGAGAIN_N6P0_DB,         ///< EAC Analog to Digital PGA Gain as  -6.0dB.
	EAC_AD_PGAGAIN_N4P5_DB,         ///< EAC Analog to Digital PGA Gain as  -4.5dB.
	EAC_AD_PGAGAIN_N3P0_DB,         ///< EAC Analog to Digital PGA Gain as  -3.0dB.
	EAC_AD_PGAGAIN_N1P5_DB,         ///< EAC Analog to Digital PGA Gain as  -1.5dB.
	EAC_AD_PGAGAIN_0_DB,            ///< EAC Analog to Digital PGA Gain as     0dB.
	EAC_AD_PGAGAIN_P1P5_DB,         ///< EAC Analog to Digital PGA Gain as  +1.5dB.
	EAC_AD_PGAGAIN_P3P0_DB,         ///< EAC Analog to Digital PGA Gain as  +3.0dB.
	EAC_AD_PGAGAIN_P4P5_DB,         ///< EAC Analog to Digital PGA Gain as  +4.5dB.
	EAC_AD_PGAGAIN_P6P0_DB,         ///< EAC Analog to Digital PGA Gain as  +6.0dB.
	EAC_AD_PGAGAIN_P7P5_DB,         ///< EAC Analog to Digital PGA Gain as  +7.5dB.
	EAC_AD_PGAGAIN_P9P0_DB,         ///< EAC Analog to Digital PGA Gain as  +9.0dB.
	EAC_AD_PGAGAIN_P10P5_DB,        ///< EAC Analog to Digital PGA Gain as +10.5dB.
	EAC_AD_PGAGAIN_P12P0_DB,        ///< EAC Analog to Digital PGA Gain as +12.0dB.
	EAC_AD_PGAGAIN_P13P5_DB,        ///< EAC Analog to Digital PGA Gain as +13.5dB.
	EAC_AD_PGAGAIN_P15P0_DB,        ///< EAC Analog to Digital PGA Gain as +15.0dB.
	EAC_AD_PGAGAIN_P16P5_DB,        ///< EAC Analog to Digital PGA Gain as +16.5dB.
	EAC_AD_PGAGAIN_P18P0_DB,        ///< EAC Analog to Digital PGA Gain as +18.0dB.
	EAC_AD_PGAGAIN_P19P5_DB,        ///< EAC Analog to Digital PGA Gain as +19.5dB.
	EAC_AD_PGAGAIN_P21P0_DB,        ///< EAC Analog to Digital PGA Gain as +21.0dB.
	EAC_AD_PGAGAIN_P22P5_DB,        ///< EAC Analog to Digital PGA Gain as +22.5dB.
	EAC_AD_PGAGAIN_P24P0_DB,        ///< EAC Analog to Digital PGA Gain as +24.0dB.
	EAC_AD_PGAGAIN_P25P5_DB,        ///< EAC Analog to Digital PGA Gain as +25.5dB.

	EAC_AD_PGAGAIN_MAX = 0x1F,
	ENUM_DUMMY4WORD(EAC_AD_PGAGAIN)
} EAC_AD_PGAGAIN;

/**
    Auto Level Control (ALC) Peak Value Source Select

    This is used in eac_set_ad_config(EAC_CONFIG_AD_ALC_PEAK_SEL) to specify the peak value source during ALC state machine.
*/
typedef enum {
	EAC_ALC_PEAK_SRC_BOTH_LR,       ///< Peak value from Max(L,R).
	EAC_ALC_PEAK_SRC_LEFT,          ///< Peak value from Left channel.
	EAC_ALC_PEAK_SRC_RIGHT,         ///< Peak value from Right channel.

	EAC_ALC_PEAK_SRC_MAX = 0x2,
	ENUM_DUMMY4WORD(EAC_ALC_PEAK_SRC)
} EAC_ALC_PEAK_SRC;

/**
    ALC Target Level Selection

    This definition is used in eac_set_ad_config(EAC_CONFIG_AD_ALC_TARGET) to specify the ALC target level.
*/
typedef enum {
	EAC_ALC_TARGET_N28P5_DB,        ///< ALC Target Level as -28.5dB.
	EAC_ALC_TARGET_N27P0_DB,        ///< ALC Target Level as -27.0dB.
	EAC_ALC_TARGET_N25P5_DB,        ///< ALC Target Level as -25.5dB.
	EAC_ALC_TARGET_N24P0_DB,        ///< ALC Target Level as -24.0dB.
	EAC_ALC_TARGET_N22P5_DB,        ///< ALC Target Level as -22.5dB.
	EAC_ALC_TARGET_N21P0_DB,        ///< ALC Target Level as -21.0dB.
	EAC_ALC_TARGET_N19P5_DB,        ///< ALC Target Level as -19.5dB.
	EAC_ALC_TARGET_N18P0_DB,        ///< ALC Target Level as -18.0dB.
	EAC_ALC_TARGET_N16P5_DB,        ///< ALC Target Level as -16.5dB.
	EAC_ALC_TARGET_N15P0_DB,        ///< ALC Target Level as -15.0dB.
	EAC_ALC_TARGET_N13P5_DB,        ///< ALC Target Level as -13.5dB.
	EAC_ALC_TARGET_N12P0_DB,        ///< ALC Target Level as -12.0dB.
	EAC_ALC_TARGET_N10P5_DB,        ///< ALC Target Level as -10.5dB.
	EAC_ALC_TARGET_N9P0_DB,         ///< ALC Target Level as  -9.0dB.
	EAC_ALC_TARGET_N7P5_DB,         ///< ALC Target Level as  -7.5dB.
	EAC_ALC_TARGET_N6P0_DB,         ///< ALC Target Level as  -6.0dB.
	EAC_ALC_TARGET_N4P5_DB,         ///< ALC Target Level as  -4.5dB.
	EAC_ALC_TARGET_N3P0_DB,         ///< ALC Target Level as  -3.0dB.
	EAC_ALC_TARGET_N1P5_DB,         ///< ALC Target Level as  -1.5dB.

	EAC_ALC_TARGET_MAX = 0x12,
	ENUM_DUMMY4WORD(EAC_ALC_TARGET)
} EAC_ALC_TARGET;

/**
    ALC Maximum Gain Selection

    This definition is used in eac_set_ad_config(EAC_CONFIG_AD_ALC_MAXGAIN_L / EAC_CONFIG_AD_ALC_MAXGAIN_R) to specify the ALC controllable Maximum PGA Gain.
*/
typedef enum {
	EAC_ALC_MAXGAIN_N21P0_DB,       ///< ALC controllable Maximum Gain as -21.0dB.
	EAC_ALC_MAXGAIN_N19P5_DB,       ///< ALC controllable Maximum Gain as -19.5dB.
	EAC_ALC_MAXGAIN_N18P0_DB,       ///< ALC controllable Maximum Gain as -18.0dB.
	EAC_ALC_MAXGAIN_N16P5_DB,       ///< ALC controllable Maximum Gain as -16.5dB.
	EAC_ALC_MAXGAIN_N15P0_DB,       ///< ALC controllable Maximum Gain as -15.0dB.
	EAC_ALC_MAXGAIN_N13P5_DB,       ///< ALC controllable Maximum Gain as -13.5dB.
	EAC_ALC_MAXGAIN_N12P0_DB,       ///< ALC controllable Maximum Gain as -12.0dB.
	EAC_ALC_MAXGAIN_N10P5_DB,       ///< ALC controllable Maximum Gain as -10.5dB.
	EAC_ALC_MAXGAIN_N9P0_DB,        ///< ALC controllable Maximum Gain as  -9.0dB.
	EAC_ALC_MAXGAIN_N7P5_DB,        ///< ALC controllable Maximum Gain as  -7.5dB.
	EAC_ALC_MAXGAIN_N6P0_DB,        ///< ALC controllable Maximum Gain as  -6.0dB.
	EAC_ALC_MAXGAIN_N4P5_DB,        ///< ALC controllable Maximum Gain as  -4.5dB.
	EAC_ALC_MAXGAIN_N3P0_DB,        ///< ALC controllable Maximum Gain as  -3.0dB.
	EAC_ALC_MAXGAIN_N1P5_DB,        ///< ALC controllable Maximum Gain as  -1.5dB.
	EAC_ALC_MAXGAIN_0_DB,           ///< ALC controllable Maximum Gain as     0dB.
	EAC_ALC_MAXGAIN_P1P5_DB,        ///< ALC controllable Maximum Gain as  +1.5dB.
	EAC_ALC_MAXGAIN_P3P0_DB,        ///< ALC controllable Maximum Gain as  +3.0dB.
	EAC_ALC_MAXGAIN_P4P5_DB,        ///< ALC controllable Maximum Gain as  +4.5dB.
	EAC_ALC_MAXGAIN_P6P0_DB,        ///< ALC controllable Maximum Gain as  +6.0dB.
	EAC_ALC_MAXGAIN_P7P5_DB,        ///< ALC controllable Maximum Gain as  +7.5dB.
	EAC_ALC_MAXGAIN_P9P0_DB,        ///< ALC controllable Maximum Gain as  +9.0dB.
	EAC_ALC_MAXGAIN_P10P5_DB,       ///< ALC controllable Maximum Gain as +10.5dB.
	EAC_ALC_MAXGAIN_P12P0_DB,       ///< ALC controllable Maximum Gain as +12.0dB.
	EAC_ALC_MAXGAIN_P13P5_DB,       ///< ALC controllable Maximum Gain as +13.5dB.
	EAC_ALC_MAXGAIN_P15P0_DB,       ///< ALC controllable Maximum Gain as +15.0dB.
	EAC_ALC_MAXGAIN_P16P5_DB,       ///< ALC controllable Maximum Gain as +16.5dB.
	EAC_ALC_MAXGAIN_P18P0_DB,       ///< ALC controllable Maximum Gain as +18.0dB.
	EAC_ALC_MAXGAIN_P19P5_DB,       ///< ALC controllable Maximum Gain as +19.5dB.
	EAC_ALC_MAXGAIN_P21P0_DB,       ///< ALC controllable Maximum Gain as +21.0dB.
	EAC_ALC_MAXGAIN_P22P5_DB,       ///< ALC controllable Maximum Gain as +22.5dB.
	EAC_ALC_MAXGAIN_P24P0_DB,       ///< ALC controllable Maximum Gain as +24.0dB.
	EAC_ALC_MAXGAIN_P25P5_DB,       ///< ALC controllable Maximum Gain as +25.5dB.

	EAC_ALC_MAXGAIN_MAX = 0x1F,
	ENUM_DUMMY4WORD(EAC_ALC_MAXGAIN)
} EAC_ALC_MAXGAIN;

/**
    ALC Minimum Gain Selection

    This definition is used in eac_set_ad_config(EAC_CONFIG_AD_ALC_MINGAIN_L / EAC_CONFIG_AD_ALC_MINGAIN_R) to specify the ALC controllable Minimum PGA Gain.
*/
typedef enum {
	EAC_ALC_MINGAIN_N21P0_DB,       ///< ALC controllable Maximum Gain as -21.0dB.
	EAC_ALC_MINGAIN_N19P5_DB,       ///< ALC controllable Maximum Gain as -19.5dB.
	EAC_ALC_MINGAIN_N18P0_DB,       ///< ALC controllable Maximum Gain as -18.0dB.
	EAC_ALC_MINGAIN_N16P5_DB,       ///< ALC controllable Maximum Gain as -16.5dB.
	EAC_ALC_MINGAIN_N15P0_DB,       ///< ALC controllable Maximum Gain as -15.0dB.
	EAC_ALC_MINGAIN_N13P5_DB,       ///< ALC controllable Maximum Gain as -13.5dB.
	EAC_ALC_MINGAIN_N12P0_DB,       ///< ALC controllable Maximum Gain as -12.0dB.
	EAC_ALC_MINGAIN_N10P5_DB,       ///< ALC controllable Maximum Gain as -10.5dB.
	EAC_ALC_MINGAIN_N9P0_DB,        ///< ALC controllable Maximum Gain as  -9.0dB.
	EAC_ALC_MINGAIN_N7P5_DB,        ///< ALC controllable Maximum Gain as  -7.5dB.
	EAC_ALC_MINGAIN_N6P0_DB,        ///< ALC controllable Maximum Gain as  -6.0dB.
	EAC_ALC_MINGAIN_N4P5_DB,        ///< ALC controllable Maximum Gain as  -4.5dB.
	EAC_ALC_MINGAIN_N3P0_DB,        ///< ALC controllable Maximum Gain as  -3.0dB.
	EAC_ALC_MINGAIN_N1P5_DB,        ///< ALC controllable Maximum Gain as  -1.5dB.
	EAC_ALC_MINGAIN_0_DB,           ///< ALC controllable Maximum Gain as     0dB.
	EAC_ALC_MINGAIN_P1P5_DB,        ///< ALC controllable Maximum Gain as  +1.5dB.
	EAC_ALC_MINGAIN_P3P0_DB,        ///< ALC controllable Maximum Gain as  +3.0dB.
	EAC_ALC_MINGAIN_P4P5_DB,        ///< ALC controllable Maximum Gain as  +4.5dB.
	EAC_ALC_MINGAIN_P6P0_DB,        ///< ALC controllable Maximum Gain as  +6.0dB.
	EAC_ALC_MINGAIN_P7P5_DB,        ///< ALC controllable Maximum Gain as  +7.5dB.
	EAC_ALC_MINGAIN_P9P0_DB,        ///< ALC controllable Maximum Gain as  +9.0dB.
	EAC_ALC_MINGAIN_P10P5_DB,       ///< ALC controllable Maximum Gain as +10.5dB.
	EAC_ALC_MINGAIN_P12P0_DB,       ///< ALC controllable Maximum Gain as +12.0dB.
	EAC_ALC_MINGAIN_P13P5_DB,       ///< ALC controllable Maximum Gain as +13.5dB.
	EAC_ALC_MINGAIN_P15P0_DB,       ///< ALC controllable Maximum Gain as +15.0dB.
	EAC_ALC_MINGAIN_P16P5_DB,       ///< ALC controllable Maximum Gain as +16.5dB.
	EAC_ALC_MINGAIN_P18P0_DB,       ///< ALC controllable Maximum Gain as +18.0dB.
	EAC_ALC_MINGAIN_P19P5_DB,       ///< ALC controllable Maximum Gain as +19.5dB.
	EAC_ALC_MINGAIN_P21P0_DB,       ///< ALC controllable Maximum Gain as +21.0dB.
	EAC_ALC_MINGAIN_P22P5_DB,       ///< ALC controllable Maximum Gain as +22.5dB.
	EAC_ALC_MINGAIN_P24P0_DB,       ///< ALC controllable Maximum Gain as +24.0dB.
	EAC_ALC_MINGAIN_P25P5_DB,       ///< ALC controllable Maximum Gain as +25.5dB.

	EAC_ALC_MINGAIN_MAX = 0x1F,
	ENUM_DUMMY4WORD(EAC_ALC_MINGAIN)
} EAC_ALC_MINGAIN;

/**
    EAC Digital Gain value

    This is pre-defined Digital Gain value for adjusting Digital Gain 1 usage.
    This is designed for interpolating the ALC Max/Min Gain step.
*/
typedef enum {
	EAC_DGAIN_N3P0_DB   = 0xBD,     ///< EAC AD Digital Gain -3.0dB.
	EAC_DGAIN_N2P5_DB   = 0xBE,     ///< EAC AD Digital Gain -2.5dB.
	EAC_DGAIN_N2P0_DB   = 0xBF,     ///< EAC AD Digital Gain -2.0dB.
	EAC_DGAIN_N1P5_DB   = 0xC0,     ///< EAC AD Digital Gain -1.5dB.
	EAC_DGAIN_N1P0_DB   = 0xC1,     ///< EAC AD Digital Gain -1.0dB.
	EAC_DGAIN_N0P5_DB   = 0xC2,     ///< EAC AD Digital Gain -0.5dB.
	EAC_DGAIN_0_DB      = 0xC3,     ///< EAC AD Digital Gain 0dB. This is default value for AD Digital Gain 1.
	EAC_DGAIN_P0P5_DB   = 0xC4,     ///< EAC AD Digital Gain +0.5dB.
	EAC_DGAIN_P1P0_DB   = 0xC5,     ///< EAC AD Digital Gain +1.0dB.
	EAC_DGAIN_P1P5_DB   = 0xC6,     ///< EAC AD Digital Gain +1.5dB.
	EAC_DGAIN_P2P0_DB   = 0xC7,     ///< EAC AD Digital Gain +2.0dB.
	EAC_DGAIN_P2P5_DB   = 0xC8,     ///< EAC AD Digital Gain +2.5dB.

	ENUM_DUMMY4WORD(EAC_DGAIN)
} EAC_DGAIN;


/**
    ALC Noise Gate Threshold Level

    This definition is used in eac_set_ad_config(EAC_CONFIG_AD_ALC_NG_THD) to specify the ALC Noise Gate Threshold level.
*/
typedef enum {
	EAC_NG_THRESHOLD_N76P5_DB,      /// ALC Noise Gate Threshold as -76.5dB.
	EAC_NG_THRESHOLD_N75P0_DB,      /// ALC Noise Gate Threshold as -75.0dB.
	EAC_NG_THRESHOLD_N73P5_DB,      /// ALC Noise Gate Threshold as -73.5dB.
	EAC_NG_THRESHOLD_N72P0_DB,      /// ALC Noise Gate Threshold as -72.0dB.
	EAC_NG_THRESHOLD_N70P5_DB,      /// ALC Noise Gate Threshold as -70.5dB.
	EAC_NG_THRESHOLD_N69P0_DB,      /// ALC Noise Gate Threshold as -69.0dB.
	EAC_NG_THRESHOLD_N67P5_DB,      /// ALC Noise Gate Threshold as -67.5dB.
	EAC_NG_THRESHOLD_N66P0_DB,      /// ALC Noise Gate Threshold as -66.0dB.
	EAC_NG_THRESHOLD_N64P5_DB,      /// ALC Noise Gate Threshold as -64.5dB.
	EAC_NG_THRESHOLD_N63P0_DB,      /// ALC Noise Gate Threshold as -63.0dB.
	EAC_NG_THRESHOLD_N61P5_DB,      /// ALC Noise Gate Threshold as -61.5dB.
	EAC_NG_THRESHOLD_N60P0_DB,      /// ALC Noise Gate Threshold as -60.0dB.
	EAC_NG_THRESHOLD_N58P5_DB,      /// ALC Noise Gate Threshold as -58.5dB.
	EAC_NG_THRESHOLD_N57P0_DB,      /// ALC Noise Gate Threshold as -57.0dB.
	EAC_NG_THRESHOLD_N55P5_DB,      /// ALC Noise Gate Threshold as -55.5dB.
	EAC_NG_THRESHOLD_N54P0_DB,      /// ALC Noise Gate Threshold as -54.0dB.
	EAC_NG_THRESHOLD_N52P5_DB,      /// ALC Noise Gate Threshold as -52.5dB.
	EAC_NG_THRESHOLD_N51P0_DB,      /// ALC Noise Gate Threshold as -51.0dB.
	EAC_NG_THRESHOLD_N49P5_DB,      /// ALC Noise Gate Threshold as -49.5dB.
	EAC_NG_THRESHOLD_N48P0_DB,      /// ALC Noise Gate Threshold as -48.0dB.
	EAC_NG_THRESHOLD_N46P5_DB,      /// ALC Noise Gate Threshold as -46.5dB.
	EAC_NG_THRESHOLD_N45P0_DB,      /// ALC Noise Gate Threshold as -45.0dB.
	EAC_NG_THRESHOLD_N43P5_DB,      /// ALC Noise Gate Threshold as -43.5dB.
	EAC_NG_THRESHOLD_N42P0_DB,      /// ALC Noise Gate Threshold as -42.0dB.
	EAC_NG_THRESHOLD_N40P5_DB,      /// ALC Noise Gate Threshold as -40.5dB.
	EAC_NG_THRESHOLD_N39P0_DB,      /// ALC Noise Gate Threshold as -39.0dB.
	EAC_NG_THRESHOLD_N37P5_DB,      /// ALC Noise Gate Threshold as -37.5dB.
	EAC_NG_THRESHOLD_N36P0_DB,      /// ALC Noise Gate Threshold as -36.0dB.
	EAC_NG_THRESHOLD_N34P5_DB,      /// ALC Noise Gate Threshold as -34.5dB.
	EAC_NG_THRESHOLD_N33P0_DB,      /// ALC Noise Gate Threshold as -33.0dB.
	EAC_NG_THRESHOLD_N31P5_DB,      /// ALC Noise Gate Threshold as -31.5dB.
	EAC_NG_THRESHOLD_N30P0_DB,      /// ALC Noise Gate Threshold as -30.0dB.

	EAC_NG_THRESHOLD_MAX = 0x1F,
	ENUM_DUMMY4WORD(EAC_NG_THRESHOLD)
} EAC_NG_THRESHOLD;


/**
    EAC Digital to Analog PGA Gain Selection

    This definition is used in eac_set_da_config(EAC_CONFIG_DA_SPEAKER_GAIN) or eac_set_da_config(EAC_CONFIG_DA_HEADPHONE_GAIN)
    to specify the DA analog PGA Gain for specified output path.
*/
typedef enum {
	EAC_DA_PGAGAIN_N30P0_DB,        ///< EAC Analog to Digital PGA Gain as -30.0dB.
	EAC_DA_PGAGAIN_N28P8_DB,        ///< EAC Analog to Digital PGA Gain as -28.8dB.
	EAC_DA_PGAGAIN_N27P6_DB,        ///< EAC Analog to Digital PGA Gain as -27.6dB.
	EAC_DA_PGAGAIN_N26P5_DB,        ///< EAC Analog to Digital PGA Gain as -26.5dB.
	EAC_DA_PGAGAIN_N25P3_DB,        ///< EAC Analog to Digital PGA Gain as -25.3dB.
	EAC_DA_PGAGAIN_N24P1_DB,        ///< EAC Analog to Digital PGA Gain as -24.1dB.
	EAC_DA_PGAGAIN_N23P0_DB,        ///< EAC Analog to Digital PGA Gain as -23.0dB.
	EAC_DA_PGAGAIN_N21P8_DB,        ///< EAC Analog to Digital PGA Gain as -21.8dB.
	EAC_DA_PGAGAIN_N20P7_DB,        ///< EAC Analog to Digital PGA Gain as -20.7dB.
	EAC_DA_PGAGAIN_N19P5_DB,        ///< EAC Analog to Digital PGA Gain as -19.5dB.
	EAC_DA_PGAGAIN_N18P3_DB,        ///< EAC Analog to Digital PGA Gain as -18.3dB.
	EAC_DA_PGAGAIN_N17P2_DB,        ///< EAC Analog to Digital PGA Gain as -17.2dB.
	EAC_DA_PGAGAIN_N16P0_DB,        ///< EAC Analog to Digital PGA Gain as -16.0dB.
	EAC_DA_PGAGAIN_N14P9_DB,        ///< EAC Analog to Digital PGA Gain as -14.9dB.
	EAC_DA_PGAGAIN_N13P7_DB,        ///< EAC Analog to Digital PGA Gain as -13.7dB.
	EAC_DA_PGAGAIN_N12P5_DB,        ///< EAC Analog to Digital PGA Gain as -12.5dB.
	EAC_DA_PGAGAIN_N11P4_DB,        ///< EAC Analog to Digital PGA Gain as -11.4dB.
	EAC_DA_PGAGAIN_N10P2_DB,        ///< EAC Analog to Digital PGA Gain as -10.2dB.
	EAC_DA_PGAGAIN_N9P0_DB,         ///< EAC Analog to Digital PGA Gain as  -9.0dB.
	EAC_DA_PGAGAIN_N7P9_DB,         ///< EAC Analog to Digital PGA Gain as  -7.9dB.
	EAC_DA_PGAGAIN_N6P7_DB,         ///< EAC Analog to Digital PGA Gain as  -6.7dB.
	EAC_DA_PGAGAIN_N5P6_DB,         ///< EAC Analog to Digital PGA Gain as  -5.6dB.
	EAC_DA_PGAGAIN_N4P4_DB,         ///< EAC Analog to Digital PGA Gain as  -4.4dB.
	EAC_DA_PGAGAIN_N3P2_DB,         ///< EAC Analog to Digital PGA Gain as  -3.2dB.
	EAC_DA_PGAGAIN_N2P1_DB,         ///< EAC Analog to Digital PGA Gain as  -2.1dB.
	EAC_DA_PGAGAIN_N0P9_DB,         ///< EAC Analog to Digital PGA Gain as  -0.9dB.
	EAC_DA_PGAGAIN_P0P1_DB,         ///< EAC Analog to Digital PGA Gain as  +0.1dB.
	EAC_DA_PGAGAIN_P1P3_DB,         ///< EAC Analog to Digital PGA Gain as  +1.3dB.
	EAC_DA_PGAGAIN_P2P5_DB,         ///< EAC Analog to Digital PGA Gain as  +2.5dB.
	EAC_DA_PGAGAIN_P3P6_DB,         ///< EAC Analog to Digital PGA Gain as  +3.6dB.
	EAC_DA_PGAGAIN_P4P8_DB,         ///< EAC Analog to Digital PGA Gain as  +4.8dB.
	EAC_DA_PGAGAIN_P6P0_DB,         ///< EAC Analog to Digital PGA Gain as  +6.0dB.

	EAC_DA_PGAGAIN_MAX = 0x1F,
	ENUM_DUMMY4WORD(EAC_DA_PGAGAIN)
} EAC_DA_PGAGAIN;

/**
    Auto Level Control and Noise Gate Time Resolution basis

    This definition is used in eac_set_ad_config(EAC_CONFIG_AD_ALC_TRESO) / eac_set_ad_config(EAC_CONFIG_AD_ALC_NG_TRESO)
    to specify the ALC attack and decay time resolution basis.
    This basis time is used to calcualte the Decay/Attack time = "(2 ^ uiCfgValue) x Basis_Time".
    The NT96660 Lowest Decay time is 800us and lowest Attack time is 1.6ms.
    If the Basis is set to 800us, the EAC_CONFIG_AD_ALC_ATTACK_TIME's uiCfgValue must be equal to or larger than 0x1.
    The CfgValue value is set by the eac_set_ad_config(EAC_CONFIG_AD_ALC_ATTACK_TIME) and eac_set_ad_config(EAC_CONFIG_AD_ALC_DECAY_TIME).
    These configurations are the simulation results of the audio codec c-model and are the roughly value.
    The calculation method is compute the PGA gain change period from 0dB to +25.5dB for decay time period and +25.5dB to 0dB for attack time period.
*/
typedef enum {
	EAC_ALC_TRESO_BASIS_800US   = 0x40457,  ///< Time Basis as  0.8ms for ALC Attack/Decay Time adjustment.
	EAC_ALC_TRESO_BASIS_1000US  = 0x5056C,  ///< Time Basis as  1.0ms for ALC Attack/Decay Time adjustment.
	EAC_ALC_TRESO_BASIS_2000US  = 0xA0000,  ///< Time Basis as  2.0ms for ALC Attack/Decay Time adjustment.
	EAC_ALC_TRESO_BASIS_5000US  = 0x120000, ///< Time Basis as  5.0ms for ALC Attack/Decay Time adjustment.
	EAC_ALC_TRESO_BASIS_10000US = 0x1FFFFF, ///< Time Basis as 10.0ms for ALC Attack/Decay Time adjustment.(This is noise gate time resolution basis default value)
	EAC_ALC_TRESO_BASIS_15000US = 0x2FFFFF, ///< Time Basis as 15.0ms for ALC Attack/Decay Time adjustment.(This is ALC time resolution basis default value)
	EAC_ALC_TRESO_BASIS_45000US = 0x7FFFFF, ///< Time Basis as 45.0ms for ALC Attack/Decay Time adjustment.

	ENUM_DUMMY4WORD(EAC_ALC_TRESO_BASIS)
} EAC_ALC_TRESO_BASIS;


/**
    EAC Digital to Analog Data Mixer Selection

    This is used in eac_set_da_config(EAC_CONFIG_DA_DATAMIXER) to select the Left/Right Input Channels Data Mixer Method.
*/
typedef enum {
	EAC_DA_MIXER_LR_INDEPENDENT,    ///< Stereo L/R Input to L/R output independently. L(out)=L(in) and R(out)=R(in).
	EAC_DA_MIXER_LR_AVERAGE,        ///< Stereo L/R Input to L and R output with averaged value. L(out)=R(out) = ( L(in) + R(in) )/2.

	ENUM_DUMMY4WORD(EAC_DA_MIXER)
} EAC_DA_MIXER;


/**
    EAC Embebbed IIR Channel select

    This is used in eac_set_iir_coef() to select which audio IIR filter channel is selected to assign filter
    parameters.
*/
typedef enum {
	EAC_IIRCH_LEFT      = 0x1,  ///< IIR filter left channel
	EAC_IIRCH_RIGHT     = 0x2,  ///< IIR filter right channel
	EAC_IIRCH_STEREO    = 0x3,  ///< IIR filter left/right channels

	ENUM_DUMMY4WORD(EAC_IIRCH)
} EAC_IIRCH;

/**
    DAC over sapmaling ratio select

    This is used in eac_setDaConfig(EAC_CONFIG_DA_OSR_SEL) to select the over sampling ratio of DAC.
    parameters.
*/
typedef enum {
	EAC_OSR_SEL_x128      ,  ///< over sampling ratio is x128
	EAC_OSR_SEL_x256      ,  ///< over sampling ratio is x256
	EAC_OSR_SEL_x512      ,  ///< over sampling ratio is x512
	EAC_OSR_SEL_x1024     ,  ///< over sampling ratio is x1024

	ENUM_DUMMY4WORD(EAC_OSR_SEL)
} EAC_OSR_SEL;

/**
    ADC ALC boost compensate valure

    This is used in eac_setDaConfig(EAC_CONFIG_AD_BOOST_COMPEN_L/EAC_CONFIG_AD_BOOST_COMPEN_R) to select the compensate boost value for alc.
    parameters.
*/
typedef enum {
	EAC_ALC_BOOST_COMPEN_0DB     ,  ///< alc compensation gain is 0dB
	EAC_ALC_BOOST_COMPEN_10DB    ,  ///< alc compensation gain is 10dB
	EAC_ALC_BOOST_COMPEN_20DB    ,  ///< alc compensation gain is 20dB
	EAC_ALC_BOOST_COMPEN_30DB    ,  ///< alc compensation gain is 30dB

	ENUM_DUMMY4WORD(EAC_ALC_BOOST_COMPEN)
} EAC_ALC_BOOST_COMPEN;



/**
    Convert floating point IIR filter coefficient to EAC used fixed point format
*/
//@{
#define EAC_IIRCOEF(x)     ((INT32)((FLOAT)((FLOAT)(x))*32768.0))
//@}


//
// Embedded Audio Codec exporting APIs
//

/**
    Initialize Embedded Audio Codec

    This function is the initialize the default settings of the Embedded Audio Codec module.
    For AD, the ALC is default as disabled. And the DCCAN/NG/Mic_Bias/ZC are default enabled.
    For DA, the ZC is default as enabled.

    @return void
*/
extern void     eac_init(void);

/**
    Set Embedded Audio Codec AD(Record) function Enable/Disable

    This function is used to enable/disable the embedded audio codec recording.

    @param [in] b_en
     - @b TRUE:  Enable AD.
     - @b FALSE: Disable AD.

    @return void
*/
extern void     eac_set_ad_enable(BOOL b_en);

/**
    Set Embedded Audio Codec DA(Playback) function Enable/Disable

    This function is used to enable/disable the embedded audio codec playback.

    @param [in] b_en
     - @b TRUE:  Enable DA.
     - @b FALSE: Disable DA.

    @return void
*/
extern void     eac_set_da_enable(BOOL b_en);

/**
    Get Embedded Audio Codec AD function Enable/Disable

    This function is used to get the enable/disable of the embedded audio codec AD function.

    @return
     - @b TRUE:  Enabled.
     - @b FALSE: Disabled.
*/
extern BOOL     eac_get_ad_enable(void);

/**
    Get Embedded Audio Codec DA function Enable/Disable

    This function is used to get the enable/disable of the embedded audio codec DA function.

    @return
     - @b TRUE:  Enabled.
     - @b FALSE: Disabled.
*/
extern BOOL     eac_get_da_enable(void);

/**
    Load Embedded Audio Codec Dynamic adjustment registers

    Some of the EAC settings have the ability to load new settings dynamically while the EAC is enabled.
    After changing these settings, the user must use this API to activate the new settings.
    These configurations are: EAC_CONFIG_DA_HEADPHONE_GAIN / EAC_CONFIG_DA_LINEOUT_GAIN / EAC_CONFIG_DA_SPEAKER_GAIN / EAC_CONFIG_DA_DGAIN_L /
    EAC_CONFIG_DA_DGAIN_R / EAC_CONFIG_AD_DGAIN1_L / EAC_CONFIG_AD_DGAIN1_R / EAC_CONFIG_AD_DGAIN2_L / EAC_CONFIG_AD_DGAIN2_R / EAC_CONFIG_AD_ALC_TARGET /
    EAC_CONFIG_AD_PGAGAIN_L / EAC_CONFIG_AD_PGAGAIN_R.

    @return void
*/
extern void     eac_set_load(void);

/**
    Set configuring to EAC AD(Record) Functions

    This api is used to configure the analog to digital(Record) functions of embedded audio codec,
    such as ALC-enable/disable, Recording-target-gain, Noise-Gate, DC-Cancellation, ...etc.

    @param[in] ConfigID     The configuration selection ID. Please refer to EAC_CONFIG_AD for details.
    @param[in] uiCfgValue   The configuration parameter according to the ConfigID.

    @return void
*/
extern void		eac_set_ad_config(EAC_CONFIG_AD config_id, UINT32 config_value);

/**
    Get configuration of EAC AD(Record) Functions

    This api is used to get the analog to digital(Record) configurations of embedded audio codec,
    such as ALC-enable/disable, Recording-target-gain, Noise-Gate, DC-Cancellation, ...etc.

    @param[in] ConfigID     The configuration selection ID. Please refer to EAC_CONFIG_AD for details.
     - @b EAC_CONFIG_AD_ALC_EN: Get the ALC Enable/Disable.
     - @b EAC_CONFIG_AD_RESET:  Get the AD Analog Block is set reset or not.
     - @b Others:               Have not implemented. Always return 0x0.

    @return The configuration parameter according to the ConfigID.
*/
extern UINT32   eac_get_ad_config(EAC_CONFIG_AD config_id);

/**
    Configure IIR filter coefficients (7-entries)

    EAC embeds a second order IIR filter. This API is used to configure the filter coefficients.
    The IIR can assign different filter coefficients for left/right channel and the selection is controlled
    by input parameter iirCH. The input coefficients pCoef have 7 entries. They are:
    pCoef[0]=B0,pCoef[1]=B1,pCoef[2]=B2,pCoef[3]=A0,pCoef[4]=A1,pCoef[5]=A2,pCoef[6]=TotalGain.
    The second order IIR equation is H(z) =  ((B0 + B1*Z1 + B2*Z2) / (A0 - A1*Z1 - A2*Z2)) x (Total-Gain).
    The user can get the floating point parameters from MATLAB simulation and feeds the floating point coefficient
    to EAC_IIRCOEF() to convert to EAC used fixed point INT32 coefficients.
    The floating point coefficient supported range is from -2.0 ~ +1.999969482421875.

    @param[in] iirCH    IIR channel selection. Please refer to EAC_IIRCH definition.
    @param[in] pCoef    The filter coefficients pCoef have 7 entries. They are:

	pCoef[0]=B0,pCoef[1]=B1,pCoef[2]=B2,pCoef[3]=A0,pCoef[4]=A1,pCoef[5]=A2,pCoef[6]=TotalGain.
	The second order IIR equation is H(z) =  ((B0 + B1*Z1 + B2*Z2) / (A0 - A1*Z1 - A2*Z2)) x (Total-Gain).

    @return void
*/
extern void		eac_set_iir_coef(EAC_IIRCH iir_ch, INT32 *p_coef);

/**
    Set configuring to EAC DA(Playback) Functions

    This api is used to configure the digital to analog(Playback) functions of embedded audio codec,
    such as Digital Gain, Analog Gain, Power-Enable, ...etc.

    @param[in] ConfigID     The configuration selection ID. Please refer to EAC_CONFIG_DA for details.
    @param[in] uiCfgValue   The configuration parameter according to the ConfigID.

    @return void
*/
extern void     eac_set_da_config(EAC_CONFIG_DA config_id, UINT32 config_value);

/**
    Get configuring of EAC DA(Playback) Functions

    This api is used to get the digital to analog(Playback) configurations of embedded audio codec,
    such as Digital Gain, Analog Gain, Power-Enable, ...etc.

    @param[in] ConfigID     The configuration selection ID. Please refer to EAC_CONFIG_DA for details.
     - @b EAC_CONFIG_DA_RESET:  Get the DA Analog Block is set reset or not.
     - @b Others:               Have not implemented. Always return 0x0.

    @return The configuration parameter according to the ConfigID
*/
extern UINT32   eac_get_da_config(EAC_CONFIG_DA config_id);

/**
    Check whether the DA path is enabled or not

    This function is used to check whether the DA path is enable or not in embedded audio codec.

    @return
     - @b TRUE:  Digital to Analog is enabled.
     - @b FALSE: Digital to Analog is disabled.
*/
extern BOOL     eac_is_dac_enable(void);

/**
    Set Enable/Disable the DA output path for embedded audio codec

    This function is used to select the DA output path  for embedded audio codec.

    @param [in] EacOutput       DAC Output path selection
     - @b EAC_OUTPUT_SPK:       Select codec output to Speaker channel. Set TRUE to enable. Set FALSE to disable.
     - @b EAC_OUTPUT_LINE:      Select codec output to Line Out. This would map to EAC_OUTPUT_HP in NT96660. Set TURE/FALSE to enable/disable both the HP left and right channels.
     - @b EAC_OUTPUT_LINE_L:    Select codec output to Line Out  Left Channel. Set TURE/FALSE to enable/disable the Lineout left channel.
     - @b EAC_OUTPUT_LINE_R:    Select codec output to Line Out Right Channel. Set TURE/FALSE to enable/disable the Lineout right channel.
     - @b EAC_OUTPUT_NONE:      Disable all of the codec output path. Set TRUE or FALSE would both disable all of the output path.

    @param [in] bEn             If selected path is EAC_OUTPUT_NONE, this api always disable all of the codec output path.
     - @b TRUE:  Enable the selected output path
     - @b FALSE: Disable the selected output path

    @return void
*/
extern void		eac_set_dac_output(EAC_OUTPUT eac_output, BOOL b_en);

/**
    Check the DA output path is Enabled or Disabled

    This function is used to check the DA output path is Enabled or Disabled.

    @param [in] EacOutput       DAC Output path selection. Please use EAC_OUTPUT_* as parameter.

    @return If selected path is EAC_OUTPUT_NONE, this api would always return FALSE.
     - @b TRUE:  The selected output path is ENABLE.
     - @b FALSE: The selected output path is DISABLE.
*/
extern BOOL     eac_get_dac_output(EAC_OUTPUT eac_output);

/**
    Get Current Record PGA Gain

    Get Current Record PGA Gain
*/
extern BOOL		eac_get_cur_pgagain(UINT16 *p_left, UINT16 *p_right);

/**
	EAC Set PHY Power ENABLE/DISABLE

	@param [in] bEn ENABLE/DISABLE of EAC PHY Power

	@return void
*/
extern void		eac_set_phypower(BOOL b_en);

/**
	EAC Set PHY DAC Power ENABLE/DISABLE

	@param [in] bEn ENABLE/DISABLE of EAC PHY DAC Power

	@return void
*/
extern void		eac_set_phydacpower(BOOL b_en);

/**
	EAC CLK releated API
*/
extern void		eac_enableclk(BOOL b_en);
extern void		eac_enableadcclk(BOOL b_en);
extern void		eac_enabledacclk(BOOL b_en);
extern void     eac_setdacclkrate(unsigned long clkrate);

//@}


#endif
