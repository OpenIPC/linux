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

#ifndef _NA51055_NVT_SRAMCTL_H
#define _NA51055_NVT_SRAMCTL_H
#include <mach/rcw_macro.h>
#include <mach/nvt_type.h>
//#define ENUM_DUMMY4WORD(m)
#define NVT_SRAM_PHY_BASE 0xF0011000
/*
    SRAM ShutDown ID

    This is for nvt_disable_sram_shutdown() and nvt_enable_sram_shutdown().
*/
typedef enum {
	NUE_SD = 0,       //< Shut Down NUE SRAM
	NUE2_SD,          //< Shut Down NUE2 SRAM
	CNN_SD,           //< Shut Down CNN SRAM
	DSI_SD,           //< Shut Down DSI SRAM
	SIE_SD = 4,       //< Shut Down SIE SRAM
	SIE2_SD,          //< Shut Down SIE2 SRAM
	//5
	SIE3_SD,          //< Shut Down SIE3 SRAM
	IPE_SD = 7,       //< Shut Down IPE SRAM
	DIS_SD,           //< Shut Down DIS SRAM
	IME_SD,           //< Shut Down IME SRAM
	MDBC_SD,          //< Shut Down MDBC SRAM
	//10

	ISE_SD,           //< Shut Down ISE SRAM
	SDP_SD,           //< Shut Down SDP SRAM
	IFE_SD,           //< Shut Down IFE SRAM
	DCE_SD,           //< Shut Down DCE SRAM
	IFE2_SD,          //< Shut Down IFE2 SRAM
	//15

	IDE_SD = 16,      //< Shut Down IDE SRAM
	NAND_SD,          //< Shut Down xD/NAND SRAM
	ETH_SD,           //< Shut Down ETH SRAM
	USB_SD,           //< Shut Down USB SRAM
	TSE_SD,           //< Shut Down TSMUX SRAM
	//20
	CPU_BRG_SD,       //< Shut Down CPU BRIDGE SRAM
	CNN2_SD,          //< Shut Down CNN2 SRAM
    IVE_SD,
	H264_SD = 24,     //< Shut Down H264 SRAM
	RSA_SD,           //< Shut Down RSA SRAM
	//25
	JPG_SD,           //< Shut Down JPG SRAM
	GRAPH_SD,         //< Shut Down Graphic SRAM
	GRAPH2_SD,        //< Shut Down Graphic2 SRAM
	SIE4_SD,          //< Shut Down SIE4 SRAM
	SIE5_SD,          //< Shut Down SIE5 SRAM
	//30
	SDIO3_SD = 31,    //< Shut Down SDIO3 SRAM
	VPE_SD = 32,      //< Shut Down VPE SRAM
	SDE_SD = 33,      //< Shut Down SDE SRAM

	ENUM_DUMMY4WORD(SRAM_SD)
} SRAM_SD;

extern void nvt_disable_sram_shutdown(SRAM_SD id);
extern void nvt_enable_sram_shutdown(SRAM_SD id);


#endif /* _NT96680_NVT_SRAMCTL_H */
