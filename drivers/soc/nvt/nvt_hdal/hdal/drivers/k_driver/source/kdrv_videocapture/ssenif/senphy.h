/*
    LVDS/MIPI-CSI/HiSPi Sensor PHY Configuration Driver Global header

    LVDS/MIPI-CSI/HiSPi Sensor PHY Configuration Driver Global header

    @file       senphy.h
    @ingroup
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _SENPHY_H
#define _SENPHY_H


#include "kwrap/type.h"


/**
    Sensor PHY configuration Selection

    This is used in senphy_set_config() to specify which of the function is selected to assign new configuration.
*/
typedef enum {
	SENPHY_CONFIG_ID_ENO_DLY,       ///< (Clock) Lane Enable to Valid Output Delay Count in (60/80/120)MHz.

	SENPHY_CONFIG_ID_DLY_EN,        ///< Set new PHY Delay/DeSkew Function Enable/Disable.
	SENPHY_CONFIG_ID_DLY_CLK0,      ///< Clock Lane 0 analog Delay Unit select. Valid value range from 0x0 to 0x7.
	SENPHY_CONFIG_ID_DLY_CLK1,      ///< Clock Lane 1 analog Delay Unit select. Valid value range from 0x0 to 0x7.
	SENPHY_CONFIG_ID_DLY_CLK2,      ///< Clock Lane 2 analog Delay Unit select. Valid value range from 0x0 to 0x7.
	SENPHY_CONFIG_ID_DLY_CLK3,      ///< Clock Lane 3 analog Delay Unit select. Valid value range from 0x0 to 0x7.
	SENPHY_CONFIG_ID_DLY_CLK4,      ///< Clock Lane 4 analog Delay Unit select. Valid value range from 0x0 to 0x7.
	SENPHY_CONFIG_ID_DLY_CLK5,      ///< Clock Lane 5 analog Delay Unit select. Valid value range from 0x0 to 0x7.
	SENPHY_CONFIG_ID_DLY_CLK6,      ///< Clock Lane 6 analog Delay Unit select. Valid value range from 0x0 to 0x7.
	SENPHY_CONFIG_ID_DLY_CLK7,      ///< Clock Lane 7 analog Delay Unit select. Valid value range from 0x0 to 0x7.

	SENPHY_CONFIG_ID_DLY_DAT0,      ///< Data Lane 0 analog Delay Unit select.  Valid value range from 0x0 to 0x7.
	SENPHY_CONFIG_ID_DLY_DAT1,      ///< Data Lane 1 analog Delay Unit select.  Valid value range from 0x0 to 0x7.
	SENPHY_CONFIG_ID_DLY_DAT2,      ///< Data Lane 2 analog Delay Unit select.  Valid value range from 0x0 to 0x7.
	SENPHY_CONFIG_ID_DLY_DAT3,      ///< Data Lane 3 analog Delay Unit select.  Valid value range from 0x0 to 0x7.
	SENPHY_CONFIG_ID_DLY_DAT4,      ///< Data Lane 4 analog Delay Unit select.  Valid value range from 0x0 to 0x7.
	SENPHY_CONFIG_ID_DLY_DAT5,      ///< Data Lane 5 analog Delay Unit select.  Valid value range from 0x0 to 0x7.
	SENPHY_CONFIG_ID_DLY_DAT6,      ///< Data Lane 6 analog Delay Unit select.  Valid value range from 0x0 to 0x7.
	SENPHY_CONFIG_ID_DLY_DAT7,      ///< Data Lane 7 analog Delay Unit select.  Valid value range from 0x0 to 0x7.

	SENPHY_CONFIG_ID_IADJ,          ///< Analog Block Current Source adjustment.
									///< Set 0x0 is 20uA (Default value). Set 0x1 is 40uA. 0x2 is 60uA. 0x3 is 80uA.
	SENPHY_CONFIG_ID_CURRDIV2,      ///< Current Divide by 2 function ENABLE/DISABLE.

	SENPHY_CONFIG_ID_INV_DAT0,      ///< Data Lane 0 pad pin P/N Swap Enable/Disable.
	SENPHY_CONFIG_ID_INV_DAT1,      ///< Data Lane 1 pad pin P/N Swap Enable/Disable.
	SENPHY_CONFIG_ID_INV_DAT2,      ///< Data Lane 2 pad pin P/N Swap Enable/Disable.
	SENPHY_CONFIG_ID_INV_DAT3,      ///< Data Lane 3 pad pin P/N Swap Enable/Disable.
	SENPHY_CONFIG_ID_INV_DAT4,      ///< Data Lane 4 pad pin P/N Swap Enable/Disable.
	SENPHY_CONFIG_ID_INV_DAT5,      ///< Data Lane 5 pad pin P/N Swap Enable/Disable.
	SENPHY_CONFIG_ID_INV_DAT6,      ///< Data Lane 6 pad pin P/N Swap Enable/Disable.
	SENPHY_CONFIG_ID_INV_DAT7,      ///< Data Lane 7 pad pin P/N Swap Enable/Disable.

	SENPHY_CONFIG_CK1_EN,			///< Clock lane 1 enable/disable. This shall be enabled during dual sensor application.

	SENPHY_CONFIG_CSI_MODE,			///< Senphy Mode CSI
	SENPHY_CONFIG_CSI2_MODE,		///< Senphy Mode CSI2
	SENPHY_CONFIG_LVDS_MODE,		///< Senphy Mode LVDS
	SENPHY_CONFIG_LVDS2_MODE,		///< Senphy Mode LVDS2

	SENPHY_CONFIG_ID_DBGMSG,        //   Sensor PHY Module Debug Message Enable/Disable. Default is disabled.
	ENUM_DUMMY4WORD(SENPHY_CONFIG_ID)
} SENPHY_CONFIG_ID;



extern ER       senphy_set_config(SENPHY_CONFIG_ID config_id, UINT32 config_value);



/*
    Shall be used by csi/lvds only
*/
typedef enum {
	SENPHY_SEL_MIPILVDS,
	SENPHY_SEL_MIPILVDS2,
	SENPHY_SEL_MIPILVDS3,
	SENPHY_SEL_MIPILVDS4,
	SENPHY_SEL_MIPILVDS5,
	SENPHY_SEL_MIPILVDS6,
	SENPHY_SEL_MIPILVDS7,
	SENPHY_SEL_MIPILVDS8,

	ENUM_DUMMY4WORD(SENPHY_SEL)
} SENPHY_SEL;


typedef enum {
	SENPHY_DATASEL_D0       = 0x0001,
	SENPHY_DATASEL_D1       = 0x0002,
	SENPHY_DATASEL_D2       = 0x0004,
	SENPHY_DATASEL_D3       = 0x0008,
	SENPHY_DATASEL_D4       = 0x0010,
	SENPHY_DATASEL_D5       = 0x0020,
	SENPHY_DATASEL_D6       = 0x0040,
	SENPHY_DATASEL_D7       = 0x0080,

	ENUM_DUMMY4WORD(SENPHY_DATASEL)
} SENPHY_DATASEL;

typedef enum {
	SENPHY_CLKMAP_CK0,
	SENPHY_CLKMAP_CK1,
	SENPHY_CLKMAP_CK2,
	SENPHY_CLKMAP_CK3,
	SENPHY_CLKMAP_CK4,
	SENPHY_CLKMAP_CK5,
	SENPHY_CLKMAP_CK6,
	SENPHY_CLKMAP_CK7,

	SENPHY_CLKMAP_MAX,
	SENPHY_CLKMAP_OFF = SENPHY_CLKMAP_MAX,
	ENUM_DUMMY4WORD(SENPHY_CLKMAP)
} SENPHY_CLKMAP;

extern void     senphy_init(void);

extern ER       senphy_set_power(SENPHY_SEL phy_select, BOOL b_enable);
extern ER       senphy_enable(SENPHY_SEL phy_select, BOOL b_enable);
extern ER       senphy_set_valid_lanes(SENPHY_SEL phy_select, SENPHY_DATASEL data_select);
extern ER       senphy_set_clock_map(SENPHY_SEL phy_select, SENPHY_CLKMAP clk_select);
extern ER       senphy_set_clock2_map(SENPHY_SEL phy_select, SENPHY_CLKMAP clk_select);

#endif
