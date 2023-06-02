/**
    MIPI Camera Serial Interface(CSI) Controller global header

    MIPI CSI Controller global header. This header file can be used for CSI and CSI2 modules.
    The user can reference this section for the detail description of the each driver API usage
    and also the parameter descriptions and its limitations.
    The overall combinational usage flow is introduced in the application note document,
    and the user must reference to the application note for the driver usage flow.

    @file       csi.h
    @ingroup    mIDrvIO_CSI
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#ifndef _CSI_H
#define _CSI_H

//#ifdef __KERNEL__
#include "kwrap/type.h"
//#endif

/**
    @addtogroup mIDrvIO_CSI
*/
//@{


/**
    CSI Functional Configuration Selection

    This definition is used in csi_set_config()/csi2_set_config() to specify which of the csi function is selected
    to assign new configurations.
*/
typedef enum {
	CSI_CONFIG_ID_DIS_METHOD,       ///< Configure the CSI disable mehtod. TRUE: Force disable, FALSE: disable when frame end. (Not Support)
	CSI_CONFIG_ID_LPKT_AUTO,        ///< Configure the CSI Pixel packet receiving as Auto mode enable.TRUE: Auto Mode enable. . FALSE: Auto Mode disable
	CSI_CONFIG_ID_LPKT_MANUAL,      ///< Configure the CSI Pixel packet receiving as Manual mode1 enable. TRUE: Manual Mode1 enable. FALSE: Manual Mode1 disable.
	CSI_CONFIG_ID_LPKT_MANUAL2,     ///< Configure the CSI Pixel packet receiving as Manual mode2 enable. TRUE: Manual Mode2 enable. FALSE: Manual Mode2 disable.
	CSI_CONFIG_ID_LPKT_MANUAL3,     ///< Configure the CSI Pixel packet receiving as Manual mode3 enable. TRUE: Manual Mode3 enable. FALSE: Manual Mode3 disable.

	CSI_CONFIG_ID_PHECC_BITORDER,   ///< Configure packet header bit order for ECC generation. Use CSI_PH_ECC_BIT_ORD_LSB or CSI_PH_ECC_BIT_ORD_MSB as input parameter.
	///< The normal user need not change this setting value. The default value is valid for the MIPI-CSI standard and needs not be changed.
	CSI_CONFIG_ID_PHECC_BYTEORDER,  ///< Configure packet header byte order for ECC generation. Use CSI_PH_ECC_BYTE_ORD_LSB or CSI_PH_ECC_BYTE_ORD_MSB as input parameter.
	///< The normal user need not change this setting value. The default value is valid for the MIPI-CSI standard and needs not be changed.
	CSI_CONFIG_ID_PHECC_FIELDORDER, ///< Configure packet header field order as DI or WC first. Use CSI_PH_ECC_FIED_ORD_DIWC or CSI_PH_ECC_FIED_ORD_WCDI as input parameter.
	///< The normal user need not change this setting value. The default value is valid for the MIPI-CSI standard and needs not be changed.
	CSI_CONFIG_ID_HD_GEN_METHOD,    ///< Configure Hsync generation method from packet header or Hsync short packet. Use CSI_LINESYNC_SPKT or CSI_LINESYNC_PKTHEAD as input parameter.
	///< In normal usage, we suggest that the user always uses the CSI_LINESYNC_PKTHEAD. This is valid for every kinds of the CSI protocol.
	CSI_CONFIG_ID_ROUND_ENABLE,     ///< If the sensor output format is 14bits per pixel, enabling this function can round the bit-13 to bit-12 because the IC internal bus is 12bits per pixel.
	///< This setting is valid only if the sensor output format is RAW14.
	CSI_CONFIG_ID_HD_GATING_EN,     ///< Configure HD gating method. The value must be ENABLE / DISABLE
	CSI_CONFIG_ID_DLANE_NUMBER,     ///< Configure data lane number for pixel receiving. Use CSI_DATLANE_NUM_1 / CSI_DATLANE_NUM_2 / CSI_DATLANE_NUM_4 as input parameter.
	CSI_CONFIG_ID_MANUAL_FORMAT,    ///< If the CSI_CONFIG_ID_LPKT_MANUAL is set to manual mode, this is used to select the manaul depack format. Use CSI_MANDEPACK_* as input parameter.
	CSI_CONFIG_ID_MANUAL_DATA_ID,   ///< If the CSI_CONFIG_ID_LPKT_MANUAL is set to manual mode, this is used to specify the receiving data id. Valid value from 0x00 to 0xFF.
	CSI_CONFIG_ID_MANUAL2_FORMAT,   ///< If the CSI_CONFIG_ID_LPKT_MANUAL2 is set to manual mode, this is used to select the manaul depack format. Use CSI_MANDEPACK_* as input parameter.
	CSI_CONFIG_ID_MANUAL2_DATA_ID,  ///< If the CSI_CONFIG_ID_LPKT_MANUAL2 is set to manual mode, this is used to specify the receiving data id. Valid value from 0x00 to 0xFF.
	CSI_CONFIG_ID_MANUAL3_FORMAT,   ///< If the CSI_CONFIG_ID_LPKT_MANUAL3 is set to manual mode, this is used to select the manaul depack format. Use CSI_MANDEPACK_* as input parameter.
	CSI_CONFIG_ID_MANUAL3_DATA_ID,  ///< If the CSI_CONFIG_ID_LPKT_MANUAL3 is set to manual mode, this is used to specify the receiving data id. Valid value from 0x00 to 0xFF.

	CSI_CONFIG_ID_CHID_MODE,        ///< Configure HDR channel id is from virtual channel id or SONY DOL id. (CSI_CHID_MIPIDI or CSI_CHID_SONYLI)
	CSI_CONFIG_ID_VIRTUAL_ID,       ///< Configure virtual id value.
	CSI_CONFIG_ID_VIRTUAL_ID2,      ///< Configure virtual id2 value.
	CSI_CONFIG_ID_VIRTUAL_ID3,      ///< Configure virtual id3 value.
	CSI_CONFIG_ID_VIRTUAL_ID4,      ///< Configure virtual id4 value.
	CSI_CONFIG_ID_LI_CHID_BIT0,     ///< Configure SONY DOL virtual id Bit[0] position. Valid value is 0 ~ 15.(It is valid when CSI_CHID_SONYLI)
	CSI_CONFIG_ID_LI_CHID_BIT1,     ///< Configure SONY DOL virtual id Bit[1] position. Valid value is 0 ~ 15.(It is valid when CSI_CHID_SONYLI)
	CSI_CONFIG_ID_LI_CHID_BIT2,     ///< Configure SONY DOL virtual id Bit[2] position. Valid value is 0 ~ 15.(It is valid when CSI_CHID_SONYLI)
	CSI_CONFIG_ID_LI_CHID_BIT3,     ///< Configure SONY DOL virtual id Bit[3] position. Valid value is 0 ~ 15.(It is valid when CSI_CHID_SONYLI)

	CSI_CONFIG_ID_VALID_HEIGHT,     ///< Configure pixel image receiving valid height.
	///< This value would be the line numbers transmitted to SIEx engine and including the sensor vertical blanking lines.
	///< When CSI, transmitted line numbers to SIE
	///< When CSI2, transmitted line numbers to SIE2
	CSI_CONFIG_ID_VD_ISSUE_TWOSIE,  ///< Configure CSI issue VD to two sie. (for sony DOL)

	CSI_CONFIG_ID_VALID_HEIGHT2,    ///< Configure pixel image receiving valid height for SIEx path.
	///< This value would be the line numbers transmitted to SIEx engine and including the sensor vertical blanking lines.
	///< When CSI, transmitted line numbers to SIE2
	///< When CSI2, transmitted line numbers to SIE4
	CSI_CONFIG_ID_VALID_HEIGHT3,    ///< Configure pixel image receiving valid height for SIEx path.
	///<This value would be the line numbers transmitted to SIEx engine and including the sensor vertical blanking lines.
	///< When 520, not support.
	CSI_CONFIG_ID_VALID_HEIGHT4,    ///< Configure pixel image receiving valid height for SIEx path.
	///<This value would be the line numbers transmitted to SIEx engine and including the sensor vertical blanking lines.
	///< When 520, not support.
	CSI_CONFIG_ID_EOT_TIMEOUT,      ///< Configure the bus monitoring EOT timeout period.

	CSI_CONFIG_ID_DEPACK_BYTE_ORDER,///< Configure the pxiel depack byte order. Use CSI_DEPACK_ORD_LSB or CSI_DEPACK_ORD_MSB as input parameter.
	CSI_CONFIG_ID_DEPACK_BIT_ORDER, ///< Configure the pxiel depack bit order. Use CSI_DEPACK_ORD_LSB or CSI_DEPACK_ORD_MSB as input parameter.

	CSI_CONFIG_ID_DATALANE0_PIN,    ///< Configure the internal data lane 0 is from which of the input pad pin. Use CSI_INPUT_PIN_* as input parameter.
	///< After setting new configuration of pin location, the new setting would be activated after csi_set_enable()/csi2_set_enable().
	///< The user must notice that the csi_get_config(CSI_CONFIG_ID_DATALANE0_PIN) would be the old setting before controller enabled.
	CSI_CONFIG_ID_DATALANE1_PIN,    ///< Configure the internal data lane 1 is from which of the input pad pin. Use CSI_INPUT_PIN_* as input parameter.
	///< After setting new configuration of pin location, the new setting would be activated after csi_set_enable()/csi2_set_enable().
	///< The user must notice that the csi_get_config(CSI_CONFIG_ID_DATALANE1_PIN) would be the old setting before controller enabled.
	CSI_CONFIG_ID_DATALANE2_PIN,    ///< Configure the internal data lane 2 is from which of the input pad pin. Use CSI_INPUT_PIN_* as input parameter.
	///< After setting new configuration of pin location, the new setting would be activated after csi_set_enable()/csi2_set_enable().
	///< The user must notice that the csi_get_config(CSI_CONFIG_ID_DATALANE2_PIN) would be the old setting before controller enabled.
	CSI_CONFIG_ID_DATALANE3_PIN,    ///< Configure the internal data lane 3 is from which of the input pad pin. Use CSI_INPUT_PIN_* as input parameter.
	///< After setting new configuration of pin location, the new setting would be activated after csi_set_enable()/csi2_set_enable().
	///< The user must notice that the csi_get_config(CSI_CONFIG_ID_DATALANE3_PIN) would be the old setting before controller enabled.
	CSI_CONFIG_ID_IMGDUPLICATE_EN,  ///< Set Enable/Disable SIE1 Image Duplicate to SIE2.

	CSI_CONFIG_ID_DIS_SRC,          ///< Set csi disable source. Use CSI_DIS_SRC_FRAME_END/CSI_DIS_SRC_FRAME_END2/CSI_DIS_SRC_FRAME_END3/CSI_DIS_SRC_FRAME_END4
	CSI_CONFIG_ID_PATGEN_EN,        ///< Set PatternGen Enable/Disable
	CSI_CONFIG_ID_PATGEN_MODE,      ///< Set PatternGen Mode, the value must be CSI_PATGEN_MODE_XXX
	CSI_CONFIG_ID_PATGEN_VAL,       ///< Set PatternGen Value

	/*
	    Analog Block Control
	*/
	CSI_CONFIG_ID_HSDATAO_DELAY,    ///< Configure the analog High Speed Data output delay. Unit in 60/80MHz clock cycles.
	///< The Data Lane high speed terminator would be opened after LP-00 is detected and with 3~4T( 60/80MHz) delay latency.
	///< This HSDATAO delay unit is the delay time after terminator enabled to enable the digital controller monitoring the analog output.
	///< Because after the terminator enabled, it may have some unstable period on the high speed signaling and we must skip this period.
	CSI_CONFIG_ID_HSDATAO_EDGE,     ///< Configure the analog High Speed Data clock valid edge according to 60/80MHz clock source. Set 0 is positive and 1 is negative edge.
	CSI_CONFIG_ID_HSDONE_DLY_CNT,   ///< Configure the analog signal HS_DONE to RX_STOP delay. Valid value range from 0 to 15. Default value is 0.
	CSI_CONFIG_ID_A_FORCE_CLK_HS,   ///< Enable/Disable CSI Clock Lane forcing at High Speed State.
	CSI_CONFIG_ID_A_FORCE_CLK_FSM,	///< Enable/Disable CSI CLock Lane State machine to ignore LP.
	CSI_CONFIG_ID_A_DLY_ENABLE,     ///< Set CSI Analog Clock/Data Lanes Delay function enable/disable.
	CSI_CONFIG_ID_A_DLY_CLK,        ///< Clock Lane analog Delay Unit select. Valid value range from 0x0 to 0x7.
	///< This setting is mapping to pad pin CK-0 for CSI, pad pin CK-1 for CSI2.
	CSI_CONFIG_ID_A_DLY_DAT0,       ///< Pad pin D0-P/N analog Delay Unit select for CSI, or Pad pin D5-P/N analog Delay Unit select for CSI2.
	///< Valid value range from 0x0 to 0x7.
	CSI_CONFIG_ID_A_DLY_DAT1,       ///< Pad pin D1-P/N analog Delay Unit select for CSI, or Pad pin D6-P/N analog Delay Unit select for CSI2.
	///< Valid value range from 0x0 to 0x7.
	CSI_CONFIG_ID_A_DLY_DAT2,       ///< Pad pin D2-P/N analog Delay Unit select for CSI, or Pad pin D7-P/N analog Delay Unit select for CSI2.
	///< Valid value range from 0x0 to 0x7.
	CSI_CONFIG_ID_A_DLY_DAT3,       ///< Pad pin D3-P/N analog Delay Unit select for CSI, or Pad pin D8-P/N analog Delay Unit select for CSI2.
	///< Valid value range from 0x0 to 0x7.
	CSI_CONFIG_ID_A_IADJ,           ///< Analog Current Source adjustment. Set 0x0 is 20uA (Default value). Set 0x1 is 40uA.
	///< The user must notice that if one of CSI or CSI2 is set this value to 0x1, both the CSI and CSI2 analog block current would be 40uA.
	CSI_CONFIG_ID_A_SYNC_RST_EN,    ///< Analog block clock lanes synchronous reset control. Default is disabled.

	CSI_CONFIG_ID_CLANE_SWITCH,     ///< Switch Clock Lane Selection from C0/C4 to C2/C6.(This not valid for CSI3/4/5/6/7/8), FALSE: C0 (CSI), C4 (CSI2), TRUE: C2 (CSI), C6 (CSI2)

	CSI_CONFIG_ID_TIMEOUT_PERIOD,   ///< wait_interrupt timeout period. Unit in ms. Default is 0 means no timeout.
	CSI_CONFIG_ID_SENSOR_TARGET_MCLK,///< Set the Sensor operating mode targeting MCLK speed. Unit in Hertz.
	CSI_CONFIG_ID_SENSOR_REAL_MCLK, ///< Set the Sensor REAL MCLK speed provided. Unit in Hertz.

	CSI_CONFIG_ID_FS_CNT,                    ///< Get the Sensor FS count.

	ENUM_DUMMY4WORD(CSI_CONFIG_ID)
} CSI_CONFIG_ID;
#define CSI_CONFIG_ID_DISABLE_SOURCE CSI_CONFIG_ID_DIS_SRC   ///< for NA51023 compatible.

/**
    Line Sync (Hsync) Generation Method Selection

    This is used in csi_set_config(CSI_CONFIG_ID_HD_GEN_METHOD)/csi2_set_config(CSI_CONFIG_ID_HD_GEN_METHOD) to specify the Hsync pulse generation method is detecting the
    HSYNC short packet or from the pixel packet's packet header.
*/
typedef enum {
	CSI_LINESYNC_SPKT,              ///< Line Sync (Hsync) is from Line Sync Short Packet.
	CSI_LINESYNC_PKTHEAD,           ///< Line Sync (Hsync) is from Pixel Packet's packet header.

	ENUM_DUMMY4WORD(CSI_LINESYNC)
} CSI_LINESYNC;


/**
    Channel ID Method Selection

    This is used in csi_set_config(CSI_CONFIG_ID_CHID_MODE) to specify the HDR channel is from virtual channel id or sony dol id.
*/
typedef enum {
	CSI_CHID_MIPIDI,               ///< channel id is from mipi DI bits
	CSI_CHID_SONYLI,               ///< channel id is from sony LI bits

	ENUM_DUMMY4WORD(CSI_CHID_MODE)
} CSI_CHID_MODE;

/**
    CSI Pixel Receiving Data Lane Number Select

    This is used in csi_set_config(CSI_CONFIG_ID_DLANE_NUMBER)/csi2_set_config(CSI_CONFIG_ID_DLANE_NUMBER) to specify the data lane number in the pixel receiving.
*/
typedef enum {
	CSI_DATLANE_NUM_1,              ///< Pixel Receiving uses 1 data lane.
	CSI_DATLANE_NUM_2,              ///< Pixel Receiving uses 2 data lanes.
	CSI_DATLANE_NUM_4,              ///< Pixel Receiving uses 4 data lanes.

	ENUM_DUMMY4WORD(CSI_DATLANE_NUM)
} CSI_DATLANE_NUM;

/**
    Manual Mode Pixel Depack Method

    If the "CSI_CONFIG_ID_LPKT_MANUAL" is set to TRUE, the controller would operate at the MANUAL mode.
    This definition is used in csi_set_config(CSI_CONFIG_ID_MANUAL_FORMAT)/csi2_set_config(CSI_CONFIG_ID_MANUAL_FORMAT) to specify the depacking method for pixel packet.
*/
typedef enum {
	CSI_MANDEPACK_8,                ///< Select the depacking method as RAW8.
	CSI_MANDEPACK_10,               ///< Select the depacking method as RAW10.
	CSI_MANDEPACK_12,               ///< Select the depacking method as RAW12.
	CSI_MANDEPACK_14,               ///< Select the depacking method as RAW14.

	ENUM_DUMMY4WORD(CSI_MANDEPACK)
} CSI_MANDEPACK;

/**
    Interrupt Event Identifications

    This definition is used in csi_wait_interrupt()/csi2_wait_interrupt() to specify which of the interrupt event that the user waited.
*/
typedef enum {
	CSI_INTERRUPT_FS_GOT           = 0x00000040,    ///< Wait Frame Start Short Packet.
	CSI_INTERRUPT_FE_GOT           = 0x00000080,    ///< Wait Frame End Short Packet.
	CSI_INTERRUPT_LS_GOT           = 0x00000400,    ///< Wait Line Start Short Packet.
	CSI_INTERRUPT_FRAME_END        = 0x00010000,    ///< Wait CSI engine Frame End Status.
	CSI_INTERRUPT_FRAME_END2       = 0x00020000,    ///< Wait CSI engine Frame End2 Status.
	CSI_INTERRUPT_FRAME_END3       = 0x00040000,    ///< Wait CSI engine Frame End3 Status.
	CSI_INTERRUPT_FRAME_END4       = 0x00080000,    ///< Wait CSI engine Frame End4 Status.
	CSI_INTERRUPT_ABORT			   = 0x80000000,	///< Wait interrupt timeout abort.

	CSI_INTERRUP_ALL               = 0x000F04C0,
	ENUM_DUMMY4WORD(CSI_INTERRUPT)
} CSI_INTERRUPT;

/**
    CSI Input Pad Pin definition

    This definition is used in csi_set_config(CSI_CONFIG_ID_DATALANE*_PIN)/csi2_set_config(CSI_CONFIG_ID_DATALANE*_PIN)
    to specify the CSI internal data lane is from which of the IC pad pins.
    The CSI should use the definitions of CSI_INPUT_PIN_D0PN/CSI_INPUT_PIN_D1PN/CSI_INPUT_PIN_D2PN/CSI_INPUT_PIN_D3PN.
    The CSI2 should use the definitions of CSI_INPUT_PIN_D4PN/CSI_INPUT_PIN_D5PN/CSI_INPUT_PIN_D6PN/CSI_INPUT_PIN_D7PN.
*/
typedef enum {
	CSI_INPUT_PIN_D0PN = 0,         ///< From input Pad pin HSI_D0P/HSI_D0N. For CSI usage.
	CSI_INPUT_PIN_D1PN,             ///< From input Pad pin HSI_D1P/HSI_D1N. For CSI usage.
	CSI_INPUT_PIN_D2PN,             ///< From input Pad pin HSI_D2P/HSI_D2N. For CSI usage.
	CSI_INPUT_PIN_D3PN,             ///< From input Pad pin HSI_D3P/HSI_D3N. For CSI usage.
	CSI_INPUT_PIN_D4PN,             ///< From input Pad pin HSI_D4P/HSI_D3N. For CSI2 usage.
	CSI_INPUT_PIN_D5PN,             ///< From input Pad pin HSI_D5P/HSI_D5N. For CSI2 usage.
	CSI_INPUT_PIN_D6PN,             ///< From input Pad pin HSI_D6P/HSI_D6N. For CSI2 usage.
	CSI_INPUT_PIN_D7PN,             ///< From input Pad pin HSI_D7P/HSI_D7N. For CSI2 usage.
	CSI_INPUT_PIN_D8PN,             ///< Backward compatible

	ENUM_DUMMY4WORD(CSI_INPUT_PIN)
} CSI_INPUT_PIN;

/**
    CSI Pixel Depack Order

    This definition is used in csi_set_config(CSI_CONFIG_ID_DEPACK_BYTE_ORDER) / csi_set_config(CSI_CONFIG_ID_DEPACK_BIT_ORDER)
    to specify the pixel depack byte/bit order when pixel format is RAW10/RAW12/RAW14.
*/
typedef enum {
	CSI_DEPACK_ORD_LSB,             ///< The Order is Least Significant Bit/Byte First.
	CSI_DEPACK_ORD_MSB,             ///< The Order is Most Significant Bit/Byte First.

	ENUM_DUMMY4WORD(CSI_DEPACK_ORD)
} CSI_DEPACK_ORD;

/**
    CSI Packet Header Bit Order

    This definition is used in csi_set_config(CSI_CONFIG_ID_PHECC_BITORDER) / csi2_set_config(CSI_CONFIG_ID_PHECC_BITORDER)
    to specify the packet header bit order.
*/
typedef enum {
	CSI_PH_ECC_BIT_ORD_LSB,         ///< The Order is Least Significant Bit/Byte First.
	CSI_PH_ECC_BIT_ORD_MSB,         ///< The Order is Most Significant Bit/Byte First.

	ENUM_DUMMY4WORD(CSI_PH_ECC_BIT_ORD)
} CSI_PH_ECC_BIT_ORD;

/**
    CSI Packet Header WC Byte Order

    This definition is used in csi_set_config(CSI_CONFIG_ID_PHECC_BYTEORDER) / csi2_set_config(CSI_CONFIG_ID_PHECC_BYTEORDER)
    to specify the packet header Word-Count's byte order.
*/
typedef enum {
	CSI_PH_ECC_BYTE_ORD_LSB,        ///< The Order is Least Significant Bit/Byte First.
	CSI_PH_ECC_BYTE_ORD_MSB,        ///< The Order is Most Significant Bit/Byte First.

	ENUM_DUMMY4WORD(CSI_PH_ECC_BYTE_ORD)
} CSI_PH_ECC_BYTE_ORD;

/**
    CSI Packet Header Field Order

    This definition is used in csi_set_config(CSI_CONFIG_ID_PHECC_FIELDORDER) / csi2_set_config(CSI_CONFIG_ID_PHECC_FIELDORDER)
    to specify the packet header Field order is DI or WC first.
*/
typedef enum {
	CSI_PH_ECC_FIED_ORD_DIWC,       ///< Field Order is DI first and then WC.
	CSI_PH_ECC_FIED_ORD_WCDI,       ///< Field Order is WC first and then DI.

	ENUM_DUMMY4WORD(CSI_PH_ECC_FIED_ORD)
} CSI_PH_ECC_FIED_ORD;

/**
    CSI Pattern Gen mode

    This definition is used in csi_set_config(CSI_CONFIG_ID_PATGEN_MODE) / csi2_set_config(CSI_CONFIG_ID_PATGEN_MODE)
    to specify the Pattern Gen mode
*/
typedef enum {
	CSI_PATGEN_MODE_COLORBAR = 1,   ///< PG is color bar
	CSI_PATGEN_MODE_RANDOM,         ///< PG is pseudo random(Frame base)
	CSI_PATGEN_MODE_FIXED,          ///< PG is fixed value
	CSI_PATGEN_MODE_1DINC,          ///< PG is incremental and reset to base pixel value when every line
	CSI_PATGEN_MODE_2DINC,          ///< PG is incremental. The vase pixel value of ever line  st to th line number

	ENUM_DUMMY4WORD(CSI_PATGEN_MODE)
} CSI_PATGEN_MODE;

/**
	CSI Disable source

	This definition is used in csi_set_config(CSI_CONFIG_ID_DIS_SRC) / csi2_set_config(CSI_CONFIG_ID_DIS_SRC)

	When CSI, only support CSI_DIS_SRC_FRAME_END or CSI_DIS_SRC_FRAME_END2
	When CSI2 SDR, only support CSI_DIS_SRC_FRAME_END
	When CSI2 HDR, only support CSI_DIS_SRC_FRAME_END or CSI_DIS_SRC_FRAME_END2
*/
typedef enum {
	CSI_DIS_SRC_FRAME_END = 0,		///< disable source frame end
	CSI_DIS_SRC_FRAME_END2,			///< disable source frame end 2
	CSI_DIS_SRC_FRAME_END3,			///< disable source frame end 3
	CSI_DIS_SRC_FRAME_END4,			///< disable source frame end 4
	ENUM_DUMMY4WORD(CSI_DIS_SRC)
} CSI_DIS_SRC;

/**
    CSI Module ID

    This is used by the CSI Get Object API csi_get_drv_object() to get the specific CSI module control object.
*/
typedef enum {
	CSI_ID_CSI,   ///< CSI  Controller ID
	CSI_ID_CSI2,  ///< CSI2 Controller ID
#if 0
	CSI_ID_CSI3,  ///< CSI3 Controller ID
	CSI_ID_CSI4,  ///< CSI4 Controller ID
	CSI_ID_CSI5,  ///< CSI5 Controller ID
	CSI_ID_CSI6,  ///< CSI6 Controller ID
	CSI_ID_CSI7,  ///< CSI7 Controller ID
	CSI_ID_CSI8,  ///< CSI8 Controller ID
#endif

	CSI_ID_MAX,
	ENUM_DUMMY4WORD(CSI_ID)
} CSI_ID;


/**
    CSI Driver Object Definotions

    This is used by the CSI Get Object API csi_get_drv_object() to get the specific CSI module control object.

	When CSI, only support CSI_DIS_SRC_FRAME_END or CSI_DIS_SRC_FRAME_END2
	When CSI2 SDR, only support CSI_DIS_SRC_FRAME_END
	When CSI2 HDR, only support CSI_DIS_SRC_FRAME_END or CSI_DIS_SRC_FRAME_END2
*/
typedef struct {
	CSI_ID          csi_id;

	ER(*open)(void);                                                            ///< csi module driver open

	ER(*close)(void);                                                           ///< csi module driver close

	BOOL (*is_opened)(void);                                                     ///< check csi module driver is opened

	ER(*set_enable)(BOOL ben);                                                   ///< set csi module enable/disable

	BOOL (*get_enable)(void);                                                    ///< get csi module enable/disable

	void (*set_config)(CSI_CONFIG_ID config_id, UINT32 cfg_value);              ///< set csi module configurations

	UINT32(*get_config)(CSI_CONFIG_ID config_id);                                ///< get csi module configurations

	CSI_INTERRUPT(*wait_interrupt)(CSI_INTERRUPT waited_flag);                   ///< wait csi interrupt event

} CSIOBJ, *PCSIOBJ;


extern PCSIOBJ csi_get_drv_object(CSI_ID csi_id);

//
//  CSI Exporting APIs
//
#ifdef __KERNEL__
extern void				csi_create_resource(void);
extern void				csi_release_resource(void);
extern UINT32			csi_isr_check(void);
extern void				csi_isr(void);
extern void				csi_set_base_addr(UINT32 addr);
extern void				siecsi_set_base_addr(UINT32 addr);
extern void             csi_set_fastboot(void);
#endif
extern ER				csi_open(void);
extern ER				csi_close(void);
extern BOOL				csi_is_opened(void);

extern ER				csi_set_enable(BOOL ben);
extern BOOL				csi_get_enable(void);

extern void				csi_set_config(CSI_CONFIG_ID config_id, UINT32 cfg_value);
extern UINT32			csi_get_config(CSI_CONFIG_ID config_id);
extern CSI_INTERRUPT	csi_wait_interrupt(CSI_INTERRUPT waited_flag);


//
//  CSI2 Exporting APIs
//
#ifdef __KERNEL__
extern void				csi2_create_resource(void);
extern void				csi2_release_resource(void);
extern UINT32			csi2_isr_check(void);
extern void				csi2_isr(void);
extern void				csi2_set_base_addr(UINT32 addr);
extern void             csi2_set_fastboot(void);
#endif
extern ER				csi2_open(void);
extern ER				csi2_close(void);
extern BOOL				csi2_is_opened(void);

extern ER				csi2_set_enable(BOOL ben);
extern BOOL				csi2_get_enable(void);

extern void				csi2_set_config(CSI_CONFIG_ID config_id, UINT32 cfg_value);
extern UINT32			csi2_get_config(CSI_CONFIG_ID config_id);
extern CSI_INTERRUPT	csi2_wait_interrupt(CSI_INTERRUPT waited_flag);

//
//  CSI3 Exporting APIs
//
#ifdef __KERNEL__
extern void				csi3_create_resource(void);
extern void				csi3_release_resource(void);
extern UINT32			csi3_isr_check(void);
extern void				csi3_isr(void);
extern void				csi3_set_base_addr(UINT32 addr);
#endif
extern ER				csi3_open(void);
extern ER				csi3_close(void);
extern BOOL				csi3_is_opened(void);

extern ER				csi3_set_enable(BOOL ben);
extern BOOL				csi3_get_enable(void);

extern void				csi3_set_config(CSI_CONFIG_ID config_id, UINT32 cfg_value);
extern UINT32			csi3_get_config(CSI_CONFIG_ID config_id);
extern CSI_INTERRUPT	csi3_wait_interrupt(CSI_INTERRUPT waited_flag);

//
//  CSI4 Exporting APIs
//
#ifdef __KERNEL__
extern void				csi4_create_resource(void);
extern void				csi4_release_resource(void);
extern UINT32			csi4_isr_check(void);
extern void				csi4_isr(void);
extern void				csi4_set_base_addr(UINT32 addr);
#endif
extern ER				csi4_open(void);
extern ER				csi4_close(void);
extern BOOL				csi4_is_opened(void);

extern ER				csi4_set_enable(BOOL ben);
extern BOOL				csi4_get_enable(void);

extern void				csi4_set_config(CSI_CONFIG_ID config_id, UINT32 cfg_value);
extern UINT32			csi4_get_config(CSI_CONFIG_ID config_id);
extern CSI_INTERRUPT	csi4_wait_interrupt(CSI_INTERRUPT waited_flag);

//
//  CSI5 Exporting APIs
//
#ifdef __KERNEL__
extern void				csi5_create_resource(void);
extern void				csi5_release_resource(void);
extern UINT32			csi5_isr_check(void);
extern void				csi5_isr(void);
extern void				csi5_set_base_addr(UINT32 addr);
#endif
extern ER				csi5_open(void);
extern ER				csi5_close(void);
extern BOOL				csi5_is_opened(void);

extern ER				csi5_set_enable(BOOL ben);
extern BOOL				csi5_get_enable(void);

extern void				csi5_set_config(CSI_CONFIG_ID config_id, UINT32 cfg_value);
extern UINT32			csi5_get_config(CSI_CONFIG_ID config_id);
extern CSI_INTERRUPT	csi5_wait_interrupt(CSI_INTERRUPT waited_flag);

//
//  CSI6 Exporting APIs
//
#ifdef __KERNEL__
extern void				csi6_create_resource(void);
extern void				csi6_release_resource(void);
extern UINT32			csi6_isr_check(void);
extern void				csi6_isr(void);
extern void				csi6_set_base_addr(UINT32 addr);
#endif
extern ER               csi6_open(void);
extern ER               csi6_close(void);
extern BOOL             csi6_is_opened(void);

extern ER               csi6_set_enable(BOOL ben);
extern BOOL             csi6_get_enable(void);

extern void             csi6_set_config(CSI_CONFIG_ID config_id, UINT32 cfg_value);
extern UINT32           csi6_get_config(CSI_CONFIG_ID config_id);
extern CSI_INTERRUPT    csi6_wait_interrupt(CSI_INTERRUPT waited_flag);

//
//  CSI7 Exporting APIs
//
#ifdef __KERNEL__
extern void				csi7_create_resource(void);
extern void				csi7_release_resource(void);
extern UINT32           csi7_isr_check(void);
extern void				csi7_isr(void);
extern void				csi7_set_base_addr(UINT32 addr);
#endif
extern ER               csi7_open(void);
extern ER               csi7_close(void);
extern BOOL             csi7_is_opened(void);

extern ER               csi7_set_enable(BOOL ben);
extern BOOL             csi7_get_enable(void);

extern void             csi7_set_config(CSI_CONFIG_ID config_id, UINT32 cfg_value);
extern UINT32           csi7_get_config(CSI_CONFIG_ID config_id);
extern CSI_INTERRUPT    csi7_wait_interrupt(CSI_INTERRUPT waited_flag);

//
//  CSI8 Exporting APIs
//
#ifdef __KERNEL__
extern void				csi8_create_resource(void);
extern void				csi8_release_resource(void);
extern UINT32           csi8_isr_check(void);
extern void				csi8_isr(void);
extern void				csi8_set_base_addr(UINT32 addr);
#endif
extern ER               csi8_open(void);
extern ER               csi8_close(void);
extern BOOL             csi8_is_opened(void);

extern ER               csi8_set_enable(BOOL ben);
extern BOOL             csi8_get_enable(void);

extern void             csi8_set_config(CSI_CONFIG_ID config_id, UINT32 cfg_value);
extern UINT32           csi8_get_config(CSI_CONFIG_ID config_id);
extern CSI_INTERRUPT    csi8_wait_interrupt(CSI_INTERRUPT waited_flag);

#ifndef __KERNEL__
extern void csi_init(void);
extern void csi2_init(void);
extern void csi_uninit(void);
extern void csi2_uninit(void);
#endif

//@}
#endif
