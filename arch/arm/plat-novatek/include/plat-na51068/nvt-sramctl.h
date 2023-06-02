/**
	SRAM Control header file
	This file will handle core communications.
	@file       nvt-sramctl.h
	@ingroup
	@note
	Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 2 as
	published by the Free Software Foundation.
*/

#ifndef _NA51068_NVT_SRAMCTL_H
#define _NA51068_NVT_SRAMCTL_H
#include <mach/rcw_macro.h>
#include <mach/nvt_type.h>
//#define ENUM_DUMMY4WORD(m)
#define NVT_SRAM_PHY_BASE 0xFE030108
/*
    SRAM ShutDown ID

    This is for nvt_disable_sram_shutdown() and nvt_enable_sram_shutdown().
*/
typedef enum {
	DSP_SD = 0,       //< Shut Down NUE SRAM
	LCD210_SD,        //< Shut Down NUE2 SRAM
	LCD310_SD,        //< Shut Down CNN SRAM
	VCAP316_SD,       //< Shut Down DSI SRAM
	CNN_SD = 4,       //< Shut Down SIE SRAM
	NUE_SD,           //< Shut Down SIE2 SRAM
	//5
	NUE2_SD,          //< Shut Down SIE3 SRAM
	VPE536_SD = 7,    //< Shut Down IPE SRAM
	SATA0_SD,         //< Shut Down DIS SRAM
	SATA1_SD,         //< Shut Down IME SRAM
	VDEC_SD,          //< Shut Down MDBC SRAM
	//10

	VPE316_SD,        //< Shut Down ISE SRAM
	VENC_SD,          //< Shut Down SDP SRAM
	OSG_SD,           //< Shut Down IFE SRAM
	USB0_SD,          //< Shut Down DCE SRAM
	USB1_SD,          //< Shut Down IFE2 SRAM
	//15

	MAU_SD = 16,      //< Shut Down IDE SRAM
	ETH0_SD,          //< Shut Down xD/NAND SRAM
	ETH1_SD,          //< Shut Down ETH SRAM
	JPEG_SD,          //< Shut Down USB SRAM
	SDC_SD,           //< Shut Down TSMUX SRAM
	//20
	SSCA_SD,          //< Shut Down CPU BRIDGE SRAM
	RSA_SD,          //< Shut Down CNN2 SRAM

	ENUM_DUMMY4WORD(SRAM_SD)
} SRAM_SD;

extern void nvt_disable_sram_shutdown(SRAM_SD id);
extern void nvt_enable_sram_shutdown(SRAM_SD id);


#endif /* _NA51068_NVT_SRAMCTL_H */
