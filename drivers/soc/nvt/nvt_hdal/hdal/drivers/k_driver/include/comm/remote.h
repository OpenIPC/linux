/**
    Header file for remote controller module.

    This file is the header file that define the APIs and data type for remote
    controller module.
    The user can reference this section for the detail description of the each driver API usage
    and also the parameter descriptions and its limitations.
    The overall combinational usage flow is introduced in the application note document,
    and the user must reference to the application note for the driver usage flow.

    @file       remote.h
    @ingroup    mIDrvIO_Remote
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _REMOTE_H_
#define _REMOTE_H_

#if defined __UITRON || defined __ECOS
#include "Type.h"
#include "Driver.h"
#else
#include "kwrap/type.h"
#include "comm/driver.h"
#endif

/**
    @addtogroup mIDrvIO_Remote
*/
//@{

/**
    Remote controller returned data command structure
*/
typedef struct {
	UINT32  uiLow;      ///<  Low word of received data pattern (bit0 ~ bit31)
	UINT32  uiHigh;     ///< High word of received data pattern (bit32 ~ bit63)
} REMOTE_PATTERN, *PREMOTE_PATTERN;


/**
    Remote controller interrupt identification

    This definition is used in the interrupt enable/disable APIs and the ISR callback identification usage.

    @note Used in remote_setInterruptEnable() / remote_setInterruptDisable() and the RMISRCallbackFunc's uiCallID.
*/
typedef enum {
	REMOTE_INT_RD       = 0x01,  ///< Data ready status ID
	REMOTE_INT_ERR      = 0x02,  ///< Timeout error status ID
	REMOTE_INT_MATCH    = 0x04,  ///< Match interrupt status ID
	REMOTE_INT_REPEAT   = 0x08,  ///< RepeatCode interrupt status ID
	REMOTE_INT_OVERRUN  = 0x10,  ///< Overrun interrupt status ID

	REMOTE_INT_ALL      = 0x1F,
	ENUM_DUMMY4WORD(REMOTE_INTERRUPT)
} REMOTE_INTERRUPT;


/**
    Remote controller threshold select

    @note Used in remote_setConfig(REMOTE_CONFIG_ID_THRESHOLD_SEL);
*/
typedef enum {
	REMOTE_SPACE_DET,   ///< Detect the space cycle count as threshold
	REMOTE_PULSE_DET,   ///< Detect the pulse cycle count as threshold

	ENUM_DUMMY4WORD(REMOTE_TH_SEL)
} REMOTE_TH_SEL;


/**
    Remote controller data order

    @note Used in remote_setConfig(REMOTE_CONFIG_ID_DATA_ORDER);
*/
typedef enum {
	REMOTE_DATA_LSB,    ///< Data bit order is LSB first
	REMOTE_DATA_MSB,    ///< Data bit order is MSB first

	ENUM_DUMMY4WORD(REMOTE_DATA_ORDER)
} REMOTE_DATA_ORDER;


/**
    Remote controller Header Detect Method

    @note Used in remote_setConfig(REMOTE_CONFIG_ID_HEADER_DETECT);
*/
typedef enum {
	REMOTE_NO_DETECT_HEADER,        ///< None of detect header bit. This can be used in the DENON/SHARP/Mitsubishi like IR protocol.
	REMOTE_FORCE_DETECT_HEADER,     ///< Force detect header bit before each command. This can be used in the NEC/JVC like IR protocol.
	REMOTE_OPTIONAL_HEADER_DETECT,  ///< Optinal detect header bit before each command. This can be used in the JVC like IR protocol.

	ENUM_DUMMY4WORD(REMOTE_HEADER_DETECT)
} REMOTE_HEADER_DETECT;


/**
    Remote controller clock source selection

    @note Used in remote_setConfig(REMOTE_CONFIG_ID_CLK_SRC_SEL);
*/
typedef enum {
	REMOTE_CLK_SRC_RTC,   ///< Remote clock source on real chip is 32.768KHz, and 31.25KHz on FPGA.
	REMOTE_CLK_SRC_OSC,   ///< Remote clock source on real chip is 32KHz, and 32KHz on FPGA.
	REMOTE_CLK_SRC_EXT,   ///< Remote clock source from external clock.
	REMOTE_CLK_SRC_3M,    ///< Remote clock source on real chip is 3MHz, and 300KHz on FPGA.

	ENUM_DUMMY4WORD(REMOTE_CLK_SRC_SEL)
} REMOTE_CLK_SRC_SEL;


/**
    Remote controller bi-phase header detect method

    The hardware will filter the headers according to these bits, leaving address and command.

    @note Used in remote_setConfig(REMOTE_CONFIG_ID_BI_PHASE_HW_DETECT_HEADER);
*/
typedef enum {
	REMOTE_BI_PHASE_HW_DETECT_HEADER_USER,      ///< User defined by "REMOTE_CONFIG_ID_BI_PHASE_HEADER_LENGTH"
	                                            ///                  "REMOTE_CONFIG_ID_BI_PHASE_HEADER_MATCH_LOW"
	                                            ///                  "REMOTE_CONFIG_ID_BI_PHASE_HEADER_MATCH_HIGH"
	                                            ///                  "REMOTE_CONFIG_ID_BI_PHASE_DETECT_HEADER_TH"(optional)
	REMOTE_BI_PHASE_HW_DETECT_HEADER_RC5,       ///< RC5
	REMOTE_BI_PHASE_HW_DETECT_HEADER_RC6,       ///< RC6
	REMOTE_BI_PHASE_HW_DETECT_HEADER_EM4095,    ///< EM4095

	ENUM_DUMMY4WORD(REMOTE_BI_PHASE_HW_DETECT_HEADER_SEL)
} REMOTE_BI_PHASE_HW_DETECT_HEADER_SEL;


/**
    Set Remote control configurations.

    These definitions are used in remote_setConfig() to configure specified settings of the remote functionalities.

    @note Used in remote_setConfig()
*/
typedef enum {
	REMOTE_CONFIG_ID_LOGIC_TH,      ///< Configure the Logic-0/1 threshold to judge the data bit. Unit in Us.
	REMOTE_CONFIG_ID_GSR_TH,        ///< Configure the glitch suppress threshold. Unit in Us. This value must be larger than or equal to 31.
	REMOTE_CONFIG_ID_REPEAT_TH,     ///< Configure the repeat code judgement threshold. Unit in Us.
	REMOTE_CONFIG_ID_HEADER_TH,     ///< Configure the header bit judgement threshold. Unit in Us.
	REMOTE_CONFIG_ID_ERROR_TH,      ///< Configure the Error condition threshold. Unit in Us.

	REMOTE_CONFIG_ID_LOGIC_TH_HW,   ///< Configure the Logic-0/1 threshold to judge the data bit. Unit is 1T depends on clk src.
	REMOTE_CONFIG_ID_GSR_TH_HW,     ///< Configure the glitch suppress threshold. Unit is 1T depnds on clk src.
	REMOTE_CONFIG_ID_HEADER_TH_HW,  ///< Configure the header bit judgement threshold Unit is 1T depends on clk src.
	REMOTE_CONFIG_ID_ERROR_TH_HW,   ///< Configure the Error condition threshold. Unit is 1T depends on clk src.

	REMOTE_CONFIG_ID_THRESHOLD_SEL, ///< Configure the Data it is judged by the Space/Pulse period. Use REMOTE_SPACE_DET or REMOTE_PULSE_DET.
	REMOTE_CONFIG_ID_DATA_LENGTH,   ///< Configure the Data Command Bit Length. Valid value from 1 to 64.
	REMOTE_CONFIG_ID_DATA_ORDER,    ///< Configure the Received Data Bit is MSb ot LSb first. Use REMOTE_DATA_LSB or REMOTE_DATA_MSB.
	REMOTE_CONFIG_ID_HEADER_DETECT, ///< Configure the Command Receive forcing detect header or not.
	///< Use REMOTE_NO_DETECT_HEADER / REMOTE_FORCE_DETECT_HEADER / REMOTE_OPTIONAL_HEADER_DETECT.
	REMOTE_CONFIG_ID_INPUT_INVERSE, ///< Configure the input signal inversion enable. Use ENABLE or DISABLE.
	REMOTE_CONFIG_ID_REPEAT_DET_EN, ///< Configure the NEC protocol repeat code detection enable. Use ENABLE or DISABLE.
	REMOTE_CONFIG_ID_MATCH_LOW,     ///< Configure command comparision match LSB 32bits.
	REMOTE_CONFIG_ID_MATCH_HIGH,    ///< Configure command comparision match MSB 32bits.
	REMOTE_CONFIG_ID_WAKEUP_ENABLE, ///< Enable the remote wakeup CPU condition after CPU power down mode. Please use "REMOTE_INTERRUPT" as input parameter.
	REMOTE_CONFIG_ID_WAKEUP_DISABLE,///< Disable the remote wakeup CPU condition after CPU power down mode. Please use "REMOTE_INTERRUPT" as input parameter.
	REMOTE_CONFIG_ID_CLK_SRC_SEL,   ///< Configure remote input clock source. Please use "REMOTE_CLK_SRC_SEL" as input parameter. (680 Unsupported)

	REMOTE_CONFIG_ID_BI_PHASE_EN,                ///< Select bi-phase mode enable/disable.
	REMOTE_CONFIG_ID_BI_PHASE_HW_DETECT_HEADER,  ///< Set bi-phase header detect method. Use "REMOTE_BI_PHASE_HW_DETECT_HEADER_SEL".
	REMOTE_CONFIG_ID_BI_PHASE_DETECT_HEADER_TH,  ///< Set bi-phase mode to detect the long header pulse, such as RC6 protocol.
	REMOTE_CONFIG_ID_BI_PHASE_HEADER_LENGTH,     ///< Set bi-phase header length, the unit is half cycle.
	REMOTE_CONFIG_ID_BI_PHASE_HEADER_MATCH_LOW,  ///< Set bi-phase header match data low, each bit represents a level of half cycle, and the order is LSb first.
	REMOTE_CONFIG_ID_BI_PHASE_HEADER_MATCH_HIGH, ///< Set bi-phase header match data high, each bit represents a level of half cycle, and the order is LSb first.
	// EX: A 2T header with the following pattern should be described with match_length = 4 and match_data_l = 0xA (1010)
	//       |------|      |------|
	//       |      |      |      |
	// ------|      |------|      |
	//  1/2T   1/2T   1/2T   1/2T

	REMOTE_CONFIG_ID_REMOTE_DEBUG,  ///< Enable/Disable remote debug messages

	ENUM_DUMMY4WORD(REMOTE_CONFIG_ID)
} REMOTE_CONFIG_ID;




//
// Export remote APIs
//

extern ER   remote_platform_create_resource(void *pmodule_info); // REMOTE_MODULE_INFO* on linux, NULL on other platform
extern void remote_platform_release_resource(void);
extern ER   remote_open(DRV_CB pEventHdl);
extern ER   remote_close(void);
extern ER   remote_setEnable(BOOL bEn);

extern ER   remote_setInterruptEnable(REMOTE_INTERRUPT IntEn);
extern ER   remote_setInterruptDisable(REMOTE_INTERRUPT IntDis);
extern ER   remote_setConfig(REMOTE_CONFIG_ID CfgID, UINT32 uiCfgValue);
extern void remote_getDataCommand(PREMOTE_PATTERN pDataCmd);
extern void remote_getRawDataCommand(PREMOTE_PATTERN pDataCmd);
//@}

#endif
