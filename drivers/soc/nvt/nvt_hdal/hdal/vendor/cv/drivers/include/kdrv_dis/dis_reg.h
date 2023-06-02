/*
    DIS register related header file.

    @file       dis_reg.h
    @ingroup    mIDrvIPP_DIS

    Copyright   Novatek Microelectronics Corp. 2010.  All rights reserved.
*/

#ifndef _DIS_NT98560_H_
#define _DIS_NT98560_H_

#include "dis_platform.h"


/*
    DIS_SW_RST    :    [0x0, 0x1],			bits : 0
    DIS_START     :    [0x0, 0x1],			bits : 1
    DIS_LOAD_START:    [0x0, 0x1],			bits : 2
    DIS_LOAD_FD   :    [0x0, 0x1],			bits : 3
    DMA_DISABLE   :    [0x0, 0x1],			bits : 4
    DMA_IDLE      :    [0x0, 0x1],			bits : 5
    LL_FIRE       :    [0x0, 0x1],			bits : 28
*/
#define DIS_CONTROL_REGISTER_OFS 0x0000
REGDEF_BEGIN(DIS_CONTROL_REGISTER)
    REGDEF_BIT(DIS_SW_RST    ,        1)
    REGDEF_BIT(DIS_START     ,        1)
    REGDEF_BIT(DIS_LOAD_START,        1)
    REGDEF_BIT(DIS_LOAD_FD   ,        1)
    REGDEF_BIT(DMA_DISABLE   ,        1)	// for engine suspending
    REGDEF_BIT(DMA_IDLE   ,           1)	// for engine suspending
    REGDEF_BIT(              ,        22)
    REGDEF_BIT(LL_FIRE       ,        1)
REGDEF_END(DIS_CONTROL_REGISTER)



/*
    INTE_FRM       :    [0x0, 0x1],			bits : 0
    INTE_MDS       :    [0x0, 0x1],			bits : 1
    INTE_BLK       :    [0x0, 0x1],			bits : 2
    INTE_MDS_HBLK  :    [0x0, 0x1],			bits : 3
    INTE_LL_DONE   :    [0x0, 0x1],			bits : 8
    INTE_LL_JOB_END:    [0x0, 0x1],			bits : 9
    INTE_LL_ERROR  :    [0x0, 0x1],			bits : 10
*/
#define DIS_INTERRUPT_ENABLE_REGISTER_OFS 0x0004
REGDEF_BEGIN(DIS_INTERRUPT_ENABLE_REGISTER)
    REGDEF_BIT(INTE_FRM       ,        1)
    REGDEF_BIT(INTE_MDS       ,        1)
    REGDEF_BIT(INTE_BLK       ,        1)
    REGDEF_BIT(INTE_MDS_HBLK  ,        1)
    REGDEF_BIT(               ,        4)
    REGDEF_BIT(INTE_LL_DONE   ,        1)
    REGDEF_BIT(INTE_LL_JOB_END,        1)
    REGDEF_BIT(INTE_LL_ERROR  ,        1)
REGDEF_END(DIS_INTERRUPT_ENABLE_REGISTER)



/*
    INTS_FRM     :    [0x0, 0x1],			bits : 0
    INTS_MDS     :    [0x0, 0x1],			bits : 1
    INTS_BLK     :    [0x0, 0x1],			bits : 2
    INTS_MDS_HBLK:    [0x0, 0x1],			bits : 3
    INTS_LL_DONE :    [0x0, 0x1],			bits : 8
	INTS_LL_JOB_END:	[0x0, 0x1],			bits : 9
    INTS_LL_ERROR:    [0x0, 0x1],			bits : 10
*/
#define DIS_INTERRUPT_STATUS_REGISTER_OFS 0x0008
REGDEF_BEGIN(DIS_INTERRUPT_STATUS_REGISTER)
    REGDEF_BIT(INTS_FRM     ,        1)
    REGDEF_BIT(INTS_MDS     ,        1)
    REGDEF_BIT(INTS_BLK     ,        1)
    REGDEF_BIT(INTS_MDS_HBLK,        1)
    REGDEF_BIT(             ,        4)
    REGDEF_BIT(INTS_LL_DONE,		 1)
    REGDEF_BIT(INTS_LL_JOB_END,		 1)
    REGDEF_BIT(INTS_LL_ERROR,        1)
REGDEF_END(DIS_INTERRUPT_STATUS_REGISTER)



/*
    DRAM_LL_SAI:    [0x0, 0x3fffffff],			bits : 31_2
*/
#define DIS_INPUT_CHANNEL_LINKED_LIST_OFS 0x000c
REGDEF_BEGIN(DIS_INPUT_CHANNEL_LINKED_LIST)
    REGDEF_BIT(           ,        2)
    REGDEF_BIT(DRAM_LL_SAI,        30)
REGDEF_END(DIS_INPUT_CHANNEL_LINKED_LIST)


/*
    DRAM_E_SAI0:    [0x0, 0x3fffffff],			bits : 31_2
*/
#define DIS_INPUT_CHANNEL_REGISTER_0_OFS 0x0010
REGDEF_BEGIN(DIS_INPUT_CHANNEL_REGISTER_0)
    REGDEF_BIT(           ,        2)
    REGDEF_BIT(DRAM_E_SAI0,        30)
REGDEF_END(DIS_INPUT_CHANNEL_REGISTER_0)


/*
    DRAM_E_SAI1:    [0x0, 0x3fffffff],			bits : 31_2
*/
#define DIS_INPUT_CHANNEL_REGISTER_1_OFS 0x0014
REGDEF_BEGIN(DIS_INPUT_CHANNEL_REGISTER_1)
    REGDEF_BIT(           ,        2)
    REGDEF_BIT(DRAM_E_SAI1,        30)
REGDEF_END(DIS_INPUT_CHANNEL_REGISTER_1)


/*
    DRAM_E_OFSI:    [0x0, 0xfff],			bits : 13_2
*/
#define DIS_INPUT_CHANNEL_REGISTER_2_OFS 0x0018
REGDEF_BEGIN(DIS_INPUT_CHANNEL_REGISTER_2)
    REGDEF_BIT(           ,        2)
    REGDEF_BIT(DRAM_E_OFSI,        12)
REGDEF_END(DIS_INPUT_CHANNEL_REGISTER_2)


/*
    DRAM_M_SAO:    [0x0, 0x3fffffff],			bits : 31_2
*/
#define DIS_OUTPUT_CHANNEL_REGISTER_0_OFS 0x001c
REGDEF_BEGIN(DIS_OUTPUT_CHANNEL_REGISTER_0)
    REGDEF_BIT(          ,        2)
    REGDEF_BIT(DRAM_M_SAO,        30)
REGDEF_END(DIS_OUTPUT_CHANNEL_REGISTER_0)


/*
    DIS_SR       :    [0x0, 0x1],			bits : 0
    DIS_LUT      :    [0x0, 0x1],			bits : 2
    BLK_HORI     :    [0x0, 0x3],			bits : 5_4
    BLK_VERT     :    [0x0, 0x3f],			bits : 13_8
    NUMMDS2DO    :    [0x0, 0x1f],			bits : 20_16
    ETH_SUBIN	    :    [0x0, 0x3],			bits : 25_24
    ETH_SUBIN_SEL:    [0x0, 0x3],			bits : 27_26
*/
#define DIS_ME_REGISTER0_OFS 0x0020
REGDEF_BEGIN(DIS_ME_REGISTER0)
    REGDEF_BIT(DIS_SR       ,        1)
    REGDEF_BIT(             ,        1)		//REGDEF_BIT(DIS_BLOCKSIZE,        1)	// only support 32x32 in 560
    REGDEF_BIT(DIS_LUT      ,        1)
    REGDEF_BIT(             ,        1)
    REGDEF_BIT(BLK_HORI     ,        2)
    REGDEF_BIT(             ,        2)
    REGDEF_BIT(BLK_VERT     ,        6)
    REGDEF_BIT(             ,        2)
    REGDEF_BIT(NUMMDS2DO    ,        5)
	REGDEF_BIT( 			,		 3)
    REGDEF_BIT(ETH_SUBIN    ,        2)
    REGDEF_BIT(ETH_SUBIN_SEL,		 2)
REGDEF_END(DIS_ME_REGISTER0)


/*
    DIS_SCROFFS  :    [0x0, 0xff],			bits : 7_0
    DIS_SCRTHR   :    [0x0, 0xff],			bits : 15_8
    CENTER_SCRTHR:    [0x0, 0xff],			bits : 23_16
    FTCNT_THR    :    [0x0, 0xff],			bits : 31_24
*/
#define DIS_ME_REGISTER1_OFS 0x0024
REGDEF_BEGIN(DIS_ME_REGISTER1)
    REGDEF_BIT(DIS_SCROFFS  ,        8)
    REGDEF_BIT(DIS_SCRTHR   ,        8)
    REGDEF_BIT(CENTER_SCRTHR,        8)
    REGDEF_BIT(FTCNT_THR    ,        8)
REGDEF_END(DIS_ME_REGISTER1)


/*
    DIS_R00_ADDRX:    [0x0, 0x1fff],			bits : 12_0
    DIS_R00_ADDRY:    [0x0, 0xfff],			bits : 27_16
*/
#define DIS_START_ADDRESS_REGISTER_0_OFS 0x0028
REGDEF_BEGIN(DIS_START_ADDRESS_REGISTER_0)
    REGDEF_BIT(DIS_R00_ADDRX,        13)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(DIS_R00_ADDRY,        12)
REGDEF_END(DIS_START_ADDRESS_REGISTER_0)


/*
    DIS_R01_ADDRX:    [0x0, 0x1fff],			bits : 12_0
    DIS_R01_ADDRY:    [0x0, 0xfff],			bits : 27_16
*/
#define DIS_START_ADDRESS_REGISTER_1_OFS 0x002c
REGDEF_BEGIN(DIS_START_ADDRESS_REGISTER_1)
    REGDEF_BIT(DIS_R01_ADDRX,        13)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(DIS_R01_ADDRY,        12)
REGDEF_END(DIS_START_ADDRESS_REGISTER_1)


/*
    DIS_R02_ADDRX:    [0x0, 0x1fff],			bits : 12_0
    DIS_R02_ADDRY:    [0x0, 0xfff],			bits : 27_16
*/
#define DIS_START_ADDRESS_REGISTER_2_OFS 0x0030
REGDEF_BEGIN(DIS_START_ADDRESS_REGISTER_2)
    REGDEF_BIT(DIS_R02_ADDRX,        13)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(DIS_R02_ADDRY,        12)
REGDEF_END(DIS_START_ADDRESS_REGISTER_2)


/*
    DIS_R03_ADDRX:    [0x0, 0x1fff],			bits : 12_0
    DIS_R03_ADDRY:    [0x0, 0xfff],			bits : 27_16
*/
#define DIS_START_ADDRESS_REGISTER_3_OFS 0x0034
REGDEF_BEGIN(DIS_START_ADDRESS_REGISTER_3)
    REGDEF_BIT(DIS_R03_ADDRX,        13)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(DIS_R03_ADDRY,        12)
REGDEF_END(DIS_START_ADDRESS_REGISTER_3)


/*
    DIS_R04_ADDRX:    [0x0, 0x1fff],			bits : 12_0
    DIS_R04_ADDRY:    [0x0, 0xfff],			bits : 27_16
*/
#define DIS_START_ADDRESS_REGISTER_4_OFS 0x0038
REGDEF_BEGIN(DIS_START_ADDRESS_REGISTER_4)
    REGDEF_BIT(DIS_R04_ADDRX,        13)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(DIS_R04_ADDRY,        12)
REGDEF_END(DIS_START_ADDRESS_REGISTER_4)


/*
    DIS_R05_ADDRX:    [0x0, 0x1fff],			bits : 12_0
    DIS_R05_ADDRY:    [0x0, 0xfff],			bits : 27_16
*/
#define DIS_START_ADDRESS_REGISTER_5_OFS 0x003c
REGDEF_BEGIN(DIS_START_ADDRESS_REGISTER_5)
    REGDEF_BIT(DIS_R05_ADDRX,        13)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(DIS_R05_ADDRY,        12)
REGDEF_END(DIS_START_ADDRESS_REGISTER_5)


/*
    DIS_R06_ADDRX:    [0x0, 0x1fff],			bits : 12_0
    DIS_R06_ADDRY:    [0x0, 0xfff],			bits : 27_16
*/
#define DIS_START_ADDRESS_REGISTER_6_OFS 0x0040
REGDEF_BEGIN(DIS_START_ADDRESS_REGISTER_6)
    REGDEF_BIT(DIS_R06_ADDRX,        13)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(DIS_R06_ADDRY,        12)
REGDEF_END(DIS_START_ADDRESS_REGISTER_6)


/*
    DIS_R07_ADDRX:    [0x0, 0x1fff],			bits : 12_0
    DIS_R07_ADDRY:    [0x0, 0xfff],			bits : 27_16
*/
#define DIS_START_ADDRESS_REGISTER_7_OFS 0x0044
REGDEF_BEGIN(DIS_START_ADDRESS_REGISTER_7)
    REGDEF_BIT(DIS_R07_ADDRX,        13)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(DIS_R07_ADDRY,        12)
REGDEF_END(DIS_START_ADDRESS_REGISTER_7)


/*
    DIS_R08_ADDRX:    [0x0, 0x1fff],			bits : 12_0
    DIS_R08_ADDRY:    [0x0, 0xfff],			bits : 27_16
*/
#define DIS_START_ADDRESS_REGISTER_8_OFS 0x0048
REGDEF_BEGIN(DIS_START_ADDRESS_REGISTER_8)
    REGDEF_BIT(DIS_R08_ADDRX,        13)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(DIS_R08_ADDRY,        12)
REGDEF_END(DIS_START_ADDRESS_REGISTER_8)


/*
    DIS_R09_ADDRX:    [0x0, 0x1fff],			bits : 12_0
    DIS_R09_ADDRY:    [0x0, 0xfff],			bits : 27_16
*/
#define DIS_START_ADDRESS_REGISTER_9_OFS 0x004c
REGDEF_BEGIN(DIS_START_ADDRESS_REGISTER_9)
    REGDEF_BIT(DIS_R09_ADDRX,        13)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(DIS_R09_ADDRY,        12)
REGDEF_END(DIS_START_ADDRESS_REGISTER_9)


/*
    DIS_R10_ADDRX:    [0x0, 0x1fff],			bits : 12_0
    DIS_R10_ADDRY:    [0x0, 0xfff],			bits : 27_16
*/
#define DIS_START_ADDRESS_REGISTER_10_OFS 0x0050
REGDEF_BEGIN(DIS_START_ADDRESS_REGISTER_10)
    REGDEF_BIT(DIS_R10_ADDRX,        13)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(DIS_R10_ADDRY,        12)
REGDEF_END(DIS_START_ADDRESS_REGISTER_10)


/*
    DIS_R11_ADDRX:    [0x0, 0x1fff],			bits : 12_0
    DIS_R11_ADDRY:    [0x0, 0xfff],			bits : 27_16
*/
#define DIS_START_ADDRESS_REGISTER_11_OFS 0x0054
REGDEF_BEGIN(DIS_START_ADDRESS_REGISTER_11)
    REGDEF_BIT(DIS_R11_ADDRX,        13)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(DIS_R11_ADDRY,        12)
REGDEF_END(DIS_START_ADDRESS_REGISTER_11)


/*
    DIS_R12_ADDRX:    [0x0, 0x1fff],			bits : 12_0
    DIS_R12_ADDRY:    [0x0, 0xfff],			bits : 27_16
*/
#define DIS_START_ADDRESS_REGISTER_12_OFS 0x0058
REGDEF_BEGIN(DIS_START_ADDRESS_REGISTER_12)
    REGDEF_BIT(DIS_R12_ADDRX,        13)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(DIS_R12_ADDRY,        12)
REGDEF_END(DIS_START_ADDRESS_REGISTER_12)


/*
    DIS_R13_ADDRX:    [0x0, 0x1fff],			bits : 12_0
    DIS_R13_ADDRY:    [0x0, 0xfff],			bits : 27_16
*/
#define DIS_START_ADDRESS_REGISTER_13_OFS 0x005c
REGDEF_BEGIN(DIS_START_ADDRESS_REGISTER_13)
    REGDEF_BIT(DIS_R13_ADDRX,        13)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(DIS_R13_ADDRY,        12)
REGDEF_END(DIS_START_ADDRESS_REGISTER_13)


/*
    DIS_R14_ADDRX:    [0x0, 0x1fff],			bits : 12_0
    DIS_R14_ADDRY:    [0x0, 0xfff],			bits : 27_16
*/
#define DIS_START_ADDRESS_REGISTER_14_OFS 0x0060
REGDEF_BEGIN(DIS_START_ADDRESS_REGISTER_14)
    REGDEF_BIT(DIS_R14_ADDRX,        13)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(DIS_R14_ADDRY,        12)
REGDEF_END(DIS_START_ADDRESS_REGISTER_14)


/*
    DIS_R15_ADDRX:    [0x0, 0x1fff],			bits : 12_0
    DIS_R15_ADDRY:    [0x0, 0xfff],			bits : 27_16
*/
#define DIS_START_ADDRESS_REGISTER_15_OFS 0x0064
REGDEF_BEGIN(DIS_START_ADDRESS_REGISTER_15)
    REGDEF_BIT(DIS_R15_ADDRX,        13)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(DIS_R15_ADDRY,        12)
REGDEF_END(DIS_START_ADDRESS_REGISTER_15)


/*
    DIS_R16_ADDRX:    [0x0, 0x1fff],			bits : 12_0
    DIS_R16_ADDRY:    [0x0, 0xfff],			bits : 27_16
*/
#define DIS_START_ADDRESS_REGISTER_16_OFS 0x0068
REGDEF_BEGIN(DIS_START_ADDRESS_REGISTER_16)
    REGDEF_BIT(DIS_R16_ADDRX,        13)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(DIS_R16_ADDRY,        12)
REGDEF_END(DIS_START_ADDRESS_REGISTER_16)


/*
    DIS_R17_ADDRX:    [0x0, 0x1fff],			bits : 12_0
    DIS_R17_ADDRY:    [0x0, 0xfff],			bits : 27_16
*/
#define DIS_START_ADDRESS_REGISTER_17_OFS 0x006c
REGDEF_BEGIN(DIS_START_ADDRESS_REGISTER_17)
    REGDEF_BIT(DIS_R17_ADDRX,        13)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(DIS_R17_ADDRY,        12)
REGDEF_END(DIS_START_ADDRESS_REGISTER_17)


/*
    DIS_R18_ADDRX:    [0x0, 0x1fff],			bits : 12_0
    DIS_R18_ADDRY:    [0x0, 0xfff],			bits : 27_16
*/
#define DIS_START_ADDRESS_REGISTER_18_OFS 0x0070
REGDEF_BEGIN(DIS_START_ADDRESS_REGISTER_18)
    REGDEF_BIT(DIS_R18_ADDRX,        13)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(DIS_R18_ADDRY,        12)
REGDEF_END(DIS_START_ADDRESS_REGISTER_18)


/*
    DIS_R19_ADDRX:    [0x0, 0x1fff],			bits : 12_0
    DIS_R19_ADDRY:    [0x0, 0xfff],			bits : 27_16
*/
#define DIS_START_ADDRESS_REGISTER_19_OFS 0x0074
REGDEF_BEGIN(DIS_START_ADDRESS_REGISTER_19)
    REGDEF_BIT(DIS_R19_ADDRX,        13)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(DIS_R19_ADDRY,        12)
REGDEF_END(DIS_START_ADDRESS_REGISTER_19)


/*
    DIS_R20_ADDRX:    [0x0, 0x1fff],			bits : 12_0
    DIS_R20_ADDRY:    [0x0, 0xfff],			bits : 27_16
*/
#define DIS_START_ADDRESS_REGISTER_20_OFS 0x0078
REGDEF_BEGIN(DIS_START_ADDRESS_REGISTER_20)
    REGDEF_BIT(DIS_R20_ADDRX,        13)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(DIS_R20_ADDRY,        12)
REGDEF_END(DIS_START_ADDRESS_REGISTER_20)


/*
    DIS_R21_ADDRX:    [0x0, 0x1fff],			bits : 12_0
    DIS_R21_ADDRY:    [0x0, 0xfff],			bits : 27_16
*/
#define DIS_START_ADDRESS_REGISTER_21_OFS 0x007c
REGDEF_BEGIN(DIS_START_ADDRESS_REGISTER_21)
    REGDEF_BIT(DIS_R21_ADDRX,        13)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(DIS_R21_ADDRY,        12)
REGDEF_END(DIS_START_ADDRESS_REGISTER_21)


/*
    DIS_R22_ADDRX:    [0x0, 0x1fff],			bits : 12_0
    DIS_R22_ADDRY:    [0x0, 0xfff],			bits : 27_16
*/
#define DIS_START_ADDRESS_REGISTER_22_OFS 0x0080
REGDEF_BEGIN(DIS_START_ADDRESS_REGISTER_22)
    REGDEF_BIT(DIS_R22_ADDRX,        13)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(DIS_R22_ADDRY,        12)
REGDEF_END(DIS_START_ADDRESS_REGISTER_22)


/*
    DIS_R23_ADDRX:    [0x0, 0x1fff],			bits : 12_0
    DIS_R23_ADDRY:    [0x0, 0xfff],			bits : 27_16
*/
#define DIS_START_ADDRESS_REGISTER_23_OFS 0x0084
REGDEF_BEGIN(DIS_START_ADDRESS_REGISTER_23)
    REGDEF_BIT(DIS_R23_ADDRX,        13)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(DIS_R23_ADDRY,        12)
REGDEF_END(DIS_START_ADDRESS_REGISTER_23)


/*
    DIS_R24_ADDRX:    [0x0, 0x1fff],			bits : 12_0
    DIS_R24_ADDRY:    [0x0, 0xfff],			bits : 27_16
*/
#define DIS_START_ADDRESS_REGISTER_24_OFS 0x0088
REGDEF_BEGIN(DIS_START_ADDRESS_REGISTER_24)
    REGDEF_BIT(DIS_R24_ADDRX,        13)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(DIS_R24_ADDRY,        12)
REGDEF_END(DIS_START_ADDRESS_REGISTER_24)


/*
    DIS_R25_ADDRX:    [0x0, 0x1fff],			bits : 12_0
    DIS_R25_ADDRY:    [0x0, 0xfff],			bits : 27_16
*/
#define DIS_START_ADDRESS_REGISTER_25_OFS 0x008c
REGDEF_BEGIN(DIS_START_ADDRESS_REGISTER_25)
    REGDEF_BIT(DIS_R25_ADDRX,        13)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(DIS_R25_ADDRY,        12)
REGDEF_END(DIS_START_ADDRESS_REGISTER_25)


/*
    DIS_R26_ADDRX:    [0x0, 0x1fff],			bits : 12_0
    DIS_R26_ADDRY:    [0x0, 0xfff],			bits : 27_16
*/
#define DIS_START_ADDRESS_REGISTER_26_OFS 0x0090
REGDEF_BEGIN(DIS_START_ADDRESS_REGISTER_26)
    REGDEF_BIT(DIS_R26_ADDRX,        13)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(DIS_R26_ADDRY,        12)
REGDEF_END(DIS_START_ADDRESS_REGISTER_26)


/*
    DIS_R27_ADDRX:    [0x0, 0x1fff],			bits : 12_0
    DIS_R27_ADDRY:    [0x0, 0xfff],			bits : 27_16
*/
#define DIS_START_ADDRESS_REGISTER_27_OFS 0x0094
REGDEF_BEGIN(DIS_START_ADDRESS_REGISTER_27)
    REGDEF_BIT(DIS_R27_ADDRX,        13)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(DIS_R27_ADDRY,        12)
REGDEF_END(DIS_START_ADDRESS_REGISTER_27)


/*
    DIS_R28_ADDRX:    [0x0, 0x1fff],			bits : 12_0
    DIS_R28_ADDRY:    [0x0, 0xfff],			bits : 27_16
*/
#define DIS_START_ADDRESS_REGISTER_28_OFS 0x0098
REGDEF_BEGIN(DIS_START_ADDRESS_REGISTER_28)
    REGDEF_BIT(DIS_R28_ADDRX,        13)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(DIS_R28_ADDRY,        12)
REGDEF_END(DIS_START_ADDRESS_REGISTER_28)


/*
    DIS_R29_ADDRX:    [0x0, 0x1fff],			bits : 12_0
    DIS_R29_ADDRY:    [0x0, 0xfff],			bits : 27_16
*/
#define DIS_START_ADDRESS_REGISTER_29_OFS 0x009c
REGDEF_BEGIN(DIS_START_ADDRESS_REGISTER_29)
    REGDEF_BIT(DIS_R29_ADDRX,        13)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(DIS_R29_ADDRY,        12)
REGDEF_END(DIS_START_ADDRESS_REGISTER_29)


/*
    DIS_R30_ADDRX:    [0x0, 0x1fff],			bits : 12_0
    DIS_R30_ADDRY:    [0x0, 0xfff],			bits : 27_16
*/
#define DIS_START_ADDRESS_REGISTER_30_OFS 0x00a0
REGDEF_BEGIN(DIS_START_ADDRESS_REGISTER_30)
    REGDEF_BIT(DIS_R30_ADDRX,        13)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(DIS_R30_ADDRY,        12)
REGDEF_END(DIS_START_ADDRESS_REGISTER_30)


/*
    DIS_R31_ADDRX:    [0x0, 0x1fff],			bits : 12_0
    DIS_R31_ADDRY:    [0x0, 0xfff],			bits : 27_16
*/
#define DIS_START_ADDRESS_REGISTER_31_OFS 0x00a4
REGDEF_BEGIN(DIS_START_ADDRESS_REGISTER_31)
    REGDEF_BIT(DIS_R31_ADDRX,        13)
    REGDEF_BIT(             ,        3)
    REGDEF_BIT(DIS_R31_ADDRY,        12)
REGDEF_END(DIS_START_ADDRESS_REGISTER_31)


/*
    MDSOFS_X00:    [0x0, 0xff],			bits : 7_0
    MDSOFS_Y00:    [0x0, 0xff],			bits : 15_8
    MDSOFS_X01:    [0x0, 0xff],			bits : 23_16
    MDSOFS_Y01:    [0x0, 0xff],			bits : 31_24
*/
#define DIS_OFFSET_REGISTER_00_OFS 0x00a8
REGDEF_BEGIN(DIS_OFFSET_REGISTER_00)
    REGDEF_BIT(MDSOFS_X00,        8)
    REGDEF_BIT(MDSOFS_Y00,        8)
    REGDEF_BIT(MDSOFS_X01,        8)
    REGDEF_BIT(MDSOFS_Y01,        8)
REGDEF_END(DIS_OFFSET_REGISTER_00)


/*
    MDSOFS_X02:    [0x0, 0xff],			bits : 7_0
    MDSOFS_Y02:    [0x0, 0xff],			bits : 15_8
    MDSOFS_X03:    [0x0, 0xff],			bits : 23_16
    MDSOFS_Y03:    [0x0, 0xff],			bits : 31_24
*/
#define DIS_OFFSET_REGISTER_01_OFS 0x00ac
REGDEF_BEGIN(DIS_OFFSET_REGISTER_01)
    REGDEF_BIT(MDSOFS_X02,        8)
    REGDEF_BIT(MDSOFS_Y02,        8)
    REGDEF_BIT(MDSOFS_X03,        8)
    REGDEF_BIT(MDSOFS_Y03,        8)
REGDEF_END(DIS_OFFSET_REGISTER_01)


/*
    MDSOFS_X04:    [0x0, 0xff],			bits : 7_0
    MDSOFS_Y04:    [0x0, 0xff],			bits : 15_8
    MDSOFS_X05:    [0x0, 0xff],			bits : 23_16
    MDSOFS_Y05:    [0x0, 0xff],			bits : 31_24
*/
#define DIS_OFFSET_REGISTER_02_OFS 0x00b0
REGDEF_BEGIN(DIS_OFFSET_REGISTER_02)
    REGDEF_BIT(MDSOFS_X04,        8)
    REGDEF_BIT(MDSOFS_Y04,        8)
    REGDEF_BIT(MDSOFS_X05,        8)
    REGDEF_BIT(MDSOFS_Y05,        8)
REGDEF_END(DIS_OFFSET_REGISTER_02)


/*
    MDSOFS_X06:    [0x0, 0xff],			bits : 7_0
    MDSOFS_Y06:    [0x0, 0xff],			bits : 15_8
    MDSOFS_X07:    [0x0, 0xff],			bits : 23_16
    MDSOFS_Y07:    [0x0, 0xff],			bits : 31_24
*/
#define DIS_OFFSET_REGISTER_03_OFS 0x00b4
REGDEF_BEGIN(DIS_OFFSET_REGISTER_03)
    REGDEF_BIT(MDSOFS_X06,        8)
    REGDEF_BIT(MDSOFS_Y06,        8)
    REGDEF_BIT(MDSOFS_X07,        8)
    REGDEF_BIT(MDSOFS_Y07,        8)
REGDEF_END(DIS_OFFSET_REGISTER_03)


/*
    MDSOFS_X08:    [0x0, 0xff],			bits : 7_0
    MDSOFS_Y08:    [0x0, 0xff],			bits : 15_8
    MDSOFS_X09:    [0x0, 0xff],			bits : 23_16
    MDSOFS_Y09:    [0x0, 0xff],			bits : 31_24
*/
#define DIS_OFFSET_REGISTER_04_OFS 0x00b8
REGDEF_BEGIN(DIS_OFFSET_REGISTER_04)
    REGDEF_BIT(MDSOFS_X08,        8)
    REGDEF_BIT(MDSOFS_Y08,        8)
    REGDEF_BIT(MDSOFS_X09,        8)
    REGDEF_BIT(MDSOFS_Y09,        8)
REGDEF_END(DIS_OFFSET_REGISTER_04)


/*
    MDSOFS_X10:    [0x0, 0xff],			bits : 7_0
    MDSOFS_Y10:    [0x0, 0xff],			bits : 15_8
    MDSOFS_X11:    [0x0, 0xff],			bits : 23_16
    MDSOFS_Y11:    [0x0, 0xff],			bits : 31_24
*/
#define DIS_OFFSET_REGISTER_05_OFS 0x00bc
REGDEF_BEGIN(DIS_OFFSET_REGISTER_05)
    REGDEF_BIT(MDSOFS_X10,        8)
    REGDEF_BIT(MDSOFS_Y10,        8)
    REGDEF_BIT(MDSOFS_X11,        8)
    REGDEF_BIT(MDSOFS_Y11,        8)
REGDEF_END(DIS_OFFSET_REGISTER_05)


/*
    MDSOFS_X12:    [0x0, 0xff],			bits : 7_0
    MDSOFS_Y12:    [0x0, 0xff],			bits : 15_8
    MDSOFS_X13:    [0x0, 0xff],			bits : 23_16
    MDSOFS_Y13:    [0x0, 0xff],			bits : 31_24
*/
#define DIS_OFFSET_REGISTER_06_OFS 0x00c0
REGDEF_BEGIN(DIS_OFFSET_REGISTER_06)
    REGDEF_BIT(MDSOFS_X12,        8)
    REGDEF_BIT(MDSOFS_Y12,        8)
    REGDEF_BIT(MDSOFS_X13,        8)
    REGDEF_BIT(MDSOFS_Y13,        8)
REGDEF_END(DIS_OFFSET_REGISTER_06)


/*
    MDSOFS_X14:    [0x0, 0xff],			bits : 7_0
    MDSOFS_Y14:    [0x0, 0xff],			bits : 15_8
    MDSOFS_X15:    [0x0, 0xff],			bits : 23_16
    MDSOFS_Y15:    [0x0, 0xff],			bits : 31_24
*/
#define DIS_OFFSET_REGISTER_07_OFS 0x00c4
REGDEF_BEGIN(DIS_OFFSET_REGISTER_07)
    REGDEF_BIT(MDSOFS_X14,        8)
    REGDEF_BIT(MDSOFS_Y14,        8)
    REGDEF_BIT(MDSOFS_X15,        8)
    REGDEF_BIT(MDSOFS_Y15,        8)
REGDEF_END(DIS_OFFSET_REGISTER_07)


/*
    MDSOFS_X16:    [0x0, 0xff],			bits : 7_0
    MDSOFS_Y16:    [0x0, 0xff],			bits : 15_8
    MDSOFS_X17:    [0x0, 0xff],			bits : 23_16
    MDSOFS_Y17:    [0x0, 0xff],			bits : 31_24
*/
#define DIS_OFFSET_REGISTER_08_OFS 0x00c8
REGDEF_BEGIN(DIS_OFFSET_REGISTER_08)
    REGDEF_BIT(MDSOFS_X16,        8)
    REGDEF_BIT(MDSOFS_Y16,        8)
    REGDEF_BIT(MDSOFS_X17,        8)
    REGDEF_BIT(MDSOFS_Y17,        8)
REGDEF_END(DIS_OFFSET_REGISTER_08)


/*
    MDSOFS_X18:    [0x0, 0xff],			bits : 7_0
    MDSOFS_Y18:    [0x0, 0xff],			bits : 15_8
    MDSOFS_X19:    [0x0, 0xff],			bits : 23_16
    MDSOFS_Y19:    [0x0, 0xff],			bits : 31_24
*/
#define DIS_OFFSET_REGISTER_09_OFS 0x00cc
REGDEF_BEGIN(DIS_OFFSET_REGISTER_09)
    REGDEF_BIT(MDSOFS_X18,        8)
    REGDEF_BIT(MDSOFS_Y18,        8)
    REGDEF_BIT(MDSOFS_X19,        8)
    REGDEF_BIT(MDSOFS_Y19,        8)
REGDEF_END(DIS_OFFSET_REGISTER_09)


/*
    MDSOFS_X20:    [0x0, 0xff],			bits : 7_0
    MDSOFS_Y20:    [0x0, 0xff],			bits : 15_8
    MDSOFS_X21:    [0x0, 0xff],			bits : 23_16
    MDSOFS_Y21:    [0x0, 0xff],			bits : 31_24
*/
#define DIS_OFFSET_REGISTER_10_OFS 0x00d0
REGDEF_BEGIN(DIS_OFFSET_REGISTER_10)
    REGDEF_BIT(MDSOFS_X20,        8)
    REGDEF_BIT(MDSOFS_Y20,        8)
    REGDEF_BIT(MDSOFS_X21,        8)
    REGDEF_BIT(MDSOFS_Y21,        8)
REGDEF_END(DIS_OFFSET_REGISTER_10)


/*
    MDSOFS_X22:    [0x0, 0xff],			bits : 7_0
    MDSOFS_Y22:    [0x0, 0xff],			bits : 15_8
    MDSOFS_X23:    [0x0, 0xff],			bits : 23_16
    MDSOFS_Y23:    [0x0, 0xff],			bits : 31_24
*/
#define DIS_OFFSET_REGISTER_11_OFS 0x00d4
REGDEF_BEGIN(DIS_OFFSET_REGISTER_11)
    REGDEF_BIT(MDSOFS_X22,        8)
    REGDEF_BIT(MDSOFS_Y22,        8)
    REGDEF_BIT(MDSOFS_X23,        8)
    REGDEF_BIT(MDSOFS_Y23,        8)
REGDEF_END(DIS_OFFSET_REGISTER_11)


/*
    MDSOFS_X24:    [0x0, 0xff],			bits : 7_0
    MDSOFS_Y24:    [0x0, 0xff],			bits : 15_8
    MDSOFS_X25:    [0x0, 0xff],			bits : 23_16
    MDSOFS_Y25:    [0x0, 0xff],			bits : 31_24
*/
#define DIS_OFFSET_REGISTER_12_OFS 0x00d8
REGDEF_BEGIN(DIS_OFFSET_REGISTER_12)
    REGDEF_BIT(MDSOFS_X24,        8)
    REGDEF_BIT(MDSOFS_Y24,        8)
    REGDEF_BIT(MDSOFS_X25,        8)
    REGDEF_BIT(MDSOFS_Y25,        8)
REGDEF_END(DIS_OFFSET_REGISTER_12)


/*
    MDSOFS_X26:    [0x0, 0xff],			bits : 7_0
    MDSOFS_Y26:    [0x0, 0xff],			bits : 15_8
    MDSOFS_X27:    [0x0, 0xff],			bits : 23_16
    MDSOFS_Y27:    [0x0, 0xff],			bits : 31_24
*/
#define DIS_OFFSET_REGISTER_13_OFS 0x00dc
REGDEF_BEGIN(DIS_OFFSET_REGISTER_13)
    REGDEF_BIT(MDSOFS_X26,        8)
    REGDEF_BIT(MDSOFS_Y26,        8)
    REGDEF_BIT(MDSOFS_X27,        8)
    REGDEF_BIT(MDSOFS_Y27,        8)
REGDEF_END(DIS_OFFSET_REGISTER_13)


/*
    MDSOFS_X28:    [0x0, 0xff],			bits : 7_0
    MDSOFS_Y28:    [0x0, 0xff],			bits : 15_8
    MDSOFS_X29:    [0x0, 0xff],			bits : 23_16
    MDSOFS_Y29:    [0x0, 0xff],			bits : 31_24
*/
#define DIS_OFFSET_REGISTER_14_OFS 0x00e0
REGDEF_BEGIN(DIS_OFFSET_REGISTER_14)
    REGDEF_BIT(MDSOFS_X28,        8)
    REGDEF_BIT(MDSOFS_Y28,        8)
    REGDEF_BIT(MDSOFS_X29,        8)
    REGDEF_BIT(MDSOFS_Y29,        8)
REGDEF_END(DIS_OFFSET_REGISTER_14)


/*
    MDSOFS_X30:    [0x0, 0xff],			bits : 7_0
    MDSOFS_Y30:    [0x0, 0xff],			bits : 15_8
    MDSOFS_X31:    [0x0, 0xff],			bits : 23_16
    MDSOFS_Y31:    [0x0, 0xff],			bits : 31_24
*/
#define DIS_OFFSET_REGISTER_15_OFS 0x00e4
REGDEF_BEGIN(DIS_OFFSET_REGISTER_15)
    REGDEF_BIT(MDSOFS_X30,        8)
    REGDEF_BIT(MDSOFS_Y30,        8)
    REGDEF_BIT(MDSOFS_X31,        8)
    REGDEF_BIT(MDSOFS_Y31,        8)
REGDEF_END(DIS_OFFSET_REGISTER_15)


/*
    DIS_CREDIT0:    [0x0, 0xff],			bits : 7_0
    DIS_CREDIT1:    [0x0, 0xff],			bits : 15_8
    DIS_CREDIT2:    [0x0, 0xff],			bits : 23_16
*/
#define DIS_CREDITS_OFS 0x00e8
REGDEF_BEGIN(DIS_CREDITS)
    REGDEF_BIT(DIS_CREDIT0,        8)
    REGDEF_BIT(DIS_CREDIT1,        8)
    REGDEF_BIT(DIS_CREDIT2,        8)
REGDEF_END(DIS_CREDITS)


/*
    LL_TABLE_IDX_0:    [0x0, 0xff],			bits : 7_0
    LL_TABLE_IDX_1:    [0x0, 0xff],			bits : 15_8
    LL_TABLE_IDX_2:    [0x0, 0xff],			bits : 23_16
    LL_TABLE_IDX_3:    [0x0, 0xff],			bits : 31_24
*/
#define LL_FRAME_REGISTER_0_OFS 0x00ec
REGDEF_BEGIN(LL_FRAME_REGISTER_0)
    REGDEF_BIT(LL_TABLE_IDX_0,        8)
    REGDEF_BIT(LL_TABLE_IDX_1,        8)
    REGDEF_BIT(LL_TABLE_IDX_2,        8)
    REGDEF_BIT(LL_TABLE_IDX_3,        8)
REGDEF_END(LL_FRAME_REGISTER_0)


/*
    LL_TABLE_IDX_0:    [0x0, 0xff],			bits : 7_0
    LL_TABLE_IDX_1:    [0x0, 0xff],			bits : 15_8
    LL_TABLE_IDX_2:    [0x0, 0xff],			bits : 23_16
    LL_TABLE_IDX_3:    [0x0, 0xff],			bits : 31_24
*/
#define LL_FRAME_REGISTER_1_OFS 0x00f0
REGDEF_BEGIN(LL_FRAME_REGISTER_1)
    REGDEF_BIT(LL_TABLE_IDX_0,        8)
    REGDEF_BIT(LL_TABLE_IDX_1,        8)
    REGDEF_BIT(LL_TABLE_IDX_2,        8)
    REGDEF_BIT(LL_TABLE_IDX_3,        8)
REGDEF_END(LL_FRAME_REGISTER_1)


/*
    LL_TABLE_IDX_0:    [0x0, 0xff],			bits : 7_0
    LL_TABLE_IDX_1:    [0x0, 0xff],			bits : 15_8
    LL_TABLE_IDX_2:    [0x0, 0xff],			bits : 23_16
    LL_TABLE_IDX_3:    [0x0, 0xff],			bits : 31_24
*/
#define LL_FRAME_REGISTER_2_OFS 0x00f4
REGDEF_BEGIN(LL_FRAME_REGISTER_2)
    REGDEF_BIT(LL_TABLE_IDX_0,        8)
    REGDEF_BIT(LL_TABLE_IDX_1,        8)
    REGDEF_BIT(LL_TABLE_IDX_2,        8)
    REGDEF_BIT(LL_TABLE_IDX_3,        8)
REGDEF_END(LL_FRAME_REGISTER_2)


/*
    LL_TABLE_IDX_0:    [0x0, 0xff],			bits : 7_0
    LL_TABLE_IDX_1:    [0x0, 0xff],			bits : 15_8
    LL_TABLE_IDX_2:    [0x0, 0xff],			bits : 23_16
    LL_TABLE_IDX_3:    [0x0, 0xff],			bits : 31_24
*/
#define LL_FRAME_REGISTER_3_OFS 0x00f8
REGDEF_BEGIN(LL_FRAME_REGISTER_3)
    REGDEF_BIT(LL_TABLE_IDX_0,        8)
    REGDEF_BIT(LL_TABLE_IDX_1,        8)
    REGDEF_BIT(LL_TABLE_IDX_2,        8)
    REGDEF_BIT(LL_TABLE_IDX_3,        8)
REGDEF_END(LL_FRAME_REGISTER_3)


/*
    LL_TERMINATE:    [0x0, 0x1],			bits : 0
*/
#define LL_REGISTER_OFS 0x00fc
REGDEF_BEGIN(LL_REGISTER)
    REGDEF_BIT(LL_TERMINATE,        1)
REGDEF_END(LL_REGISTER)


/*
    DIS_COUNT0:    [0x0, 0xffffff],			bits : 23_0
*/
#define DIS_COUNT_REGISTER0_OFS 0x0100
REGDEF_BEGIN(DIS_COUNT_REGISTER0)
    REGDEF_BIT(DIS_COUNT0,        24)
REGDEF_END(DIS_COUNT_REGISTER0)


/*
    DIS_COUNT1:    [0x0, 0xffffff],			bits : 23_0
*/
#define DIS_COUNT_REGISTER1_OFS 0x0104
REGDEF_BEGIN(DIS_COUNT_REGISTER1)
    REGDEF_BIT(DIS_COUNT1,        24)
REGDEF_END(DIS_COUNT_REGISTER1)


/*
    DIS_COUNT2:    [0x0, 0xffffff],			bits : 23_0
*/
#define DIS_COUNT_REGISTER2_OFS 0x0108
REGDEF_BEGIN(DIS_COUNT_REGISTER2)
    REGDEF_BIT(DIS_COUNT2,        24)
REGDEF_END(DIS_COUNT_REGISTER2)


/*
    DIS_COUNT3:    [0x0, 0xffffff],			bits : 23_0
*/
#define DIS_COUNT_REGISTER3_OFS 0x010c
REGDEF_BEGIN(DIS_COUNT_REGISTER3)
    REGDEF_BIT(DIS_COUNT3,        24)
REGDEF_END(DIS_COUNT_REGISTER3)


/*
    BLK_VCNT:    [0x0, 0x3f],			bits : 5_0
    BLK_HCNT:    [0x0, 0x3],			bits : 8_7
    MDS_CNT :    [0x0, 0x1f],			bits : 13_9
*/
#define DIS_DEBUG_MESSAGES_OFS 0x0110
REGDEF_BEGIN(DIS_DEBUG_MESSAGES)
    REGDEF_BIT(BLK_VCNT,        6)
    REGDEF_BIT(        ,        1)
    REGDEF_BIT(BLK_HCNT,        2)
    REGDEF_BIT(MDS_CNT ,        5)
REGDEF_END(DIS_DEBUG_MESSAGES)


/*
    DIS_CYCLE_CNT:    [0x0, 0xffffffff],			bits : 31_0
*/
#define DIS_CYCLE_COUNTER_REGISTER_OFS 0x0114
REGDEF_BEGIN(DIS_CYCLE_COUNTER_REGISTER)
    REGDEF_BIT(DIS_CYCLE_CNT,        32)
REGDEF_END(DIS_CYCLE_COUNTER_REGISTER)

/*
    LL_CMD_START_ADDR_INFO:    [0x0, 0x0],			bits : 31_0
*/
#define LL_CMD_START_ADDR_INFO_REGISTER_OFS 0x0118
REGDEF_BEGIN(LL_CMD_START_ADDR_INFO_REGISTER)
    REGDEF_BIT(LL_CMD_START_ADDR_INFO,        32)
REGDEF_END(LL_CMD_START_ADDR_INFO_REGISTER)

/*
    LL_CMDPRS_CNT:    [0x0, 0x0],			bits : 19_0
*/
#define LL_CMDPRS_CNT_REGISTER_OFS 0x011C
REGDEF_BEGIN(LL_CMDPRS_CNT_REGISTER)
    REGDEF_BIT(LL_CMDPRS_CNT,        20)
REGDEF_END(LL_CMDPRS_CNT_REGISTER)

#if 0
typedef struct
{
    //0x0000
    T_DIS_CONTROL_REGISTER 
    DIS_Register_0000;

    //0x0004
    T_DIS_INTERRUPT_ENABLE_REGISTER 
    DIS_Register_0004;

    //0x0008
    T_DIS_INTERRUPT_STATUS_REGISTER 
    DIS_Register_0008;

    //0x000c
    T_DIS_INPUT_CHANNEL_LINKED_LIST 
    DIS_Register_000c;

    //0x0010
    T_DIS_INPUT_CHANNEL_REGISTER_0 
    DIS_Register_0010;

    //0x0014
    T_DIS_INPUT_CHANNEL_REGISTER_1 
    DIS_Register_0014;

    //0x0018
    T_DIS_INPUT_CHANNEL_REGISTER_2 
    DIS_Register_0018;

    //0x001c
    T_DIS_OUTPUT_CHANNEL_REGISTER_0 
    DIS_Register_001c;

    //0x0020
    T_DIS_ME_REGISTER0 
    DIS_Register_0020;

    //0x0024
    T_DIS_ME_REGISTER1 
    DIS_Register_0024;

    //0x0028
    T_DIS_START_ADDRESS_REGISTER_0 
    DIS_Register_0028;

    //0x002c
    T_DIS_START_ADDRESS_REGISTER_1 
    DIS_Register_002c;

    //0x0030
    T_DIS_START_ADDRESS_REGISTER_2 
    DIS_Register_0030;

    //0x0034
    T_DIS_START_ADDRESS_REGISTER_3 
    DIS_Register_0034;

    //0x0038
    T_DIS_START_ADDRESS_REGISTER_4 
    DIS_Register_0038;

    //0x003c
    T_DIS_START_ADDRESS_REGISTER_5 
    DIS_Register_003c;

    //0x0040
    T_DIS_START_ADDRESS_REGISTER_6 
    DIS_Register_0040;

    //0x0044
    T_DIS_START_ADDRESS_REGISTER_7 
    DIS_Register_0044;

    //0x0048
    T_DIS_START_ADDRESS_REGISTER_8 
    DIS_Register_0048;

    //0x004c
    T_DIS_START_ADDRESS_REGISTER_9 
    DIS_Register_004c;

    //0x0050
    T_DIS_START_ADDRESS_REGISTER_10 
    DIS_Register_0050;

    //0x0054
    T_DIS_START_ADDRESS_REGISTER_11 
    DIS_Register_0054;

    //0x0058
    T_DIS_START_ADDRESS_REGISTER_12 
    DIS_Register_0058;

    //0x005c
    T_DIS_START_ADDRESS_REGISTER_13 
    DIS_Register_005c;

    //0x0060
    T_DIS_START_ADDRESS_REGISTER_14 
    DIS_Register_0060;

    //0x0064
    T_DIS_START_ADDRESS_REGISTER_15 
    DIS_Register_0064;

    //0x0068
    T_DIS_START_ADDRESS_REGISTER_16 
    DIS_Register_0068;

    //0x006c
    T_DIS_START_ADDRESS_REGISTER_17 
    DIS_Register_006c;

    //0x0070
    T_DIS_START_ADDRESS_REGISTER_18 
    DIS_Register_0070;

    //0x0074
    T_DIS_START_ADDRESS_REGISTER_19 
    DIS_Register_0074;

    //0x0078
    T_DIS_START_ADDRESS_REGISTER_20 
    DIS_Register_0078;

    //0x007c
    T_DIS_START_ADDRESS_REGISTER_21 
    DIS_Register_007c;

    //0x0080
    T_DIS_START_ADDRESS_REGISTER_22 
    DIS_Register_0080;

    //0x0084
    T_DIS_START_ADDRESS_REGISTER_23 
    DIS_Register_0084;

    //0x0088
    T_DIS_START_ADDRESS_REGISTER_24 
    DIS_Register_0088;

    //0x008c
    T_DIS_START_ADDRESS_REGISTER_25 
    DIS_Register_008c;

    //0x0090
    T_DIS_START_ADDRESS_REGISTER_26 
    DIS_Register_0090;

    //0x0094
    T_DIS_START_ADDRESS_REGISTER_27 
    DIS_Register_0094;

    //0x0098
    T_DIS_START_ADDRESS_REGISTER_28 
    DIS_Register_0098;

    //0x009c
    T_DIS_START_ADDRESS_REGISTER_29 
    DIS_Register_009c;

    //0x00a0
    T_DIS_START_ADDRESS_REGISTER_30 
    DIS_Register_00a0;

    //0x00a4
    T_DIS_START_ADDRESS_REGISTER_31 
    DIS_Register_00a4;

    //0x00a8
    T_DIS_OFFSET_REGISTER_00 
    DIS_Register_00a8;

    //0x00ac
    T_DIS_OFFSET_REGISTER_01 
    DIS_Register_00ac;

    //0x00b0
    T_DIS_OFFSET_REGISTER_02 
    DIS_Register_00b0;

    //0x00b4
    T_DIS_OFFSET_REGISTER_03 
    DIS_Register_00b4;

    //0x00b8
    T_DIS_OFFSET_REGISTER_04 
    DIS_Register_00b8;

    //0x00bc
    T_DIS_OFFSET_REGISTER_05 
    DIS_Register_00bc;

    //0x00c0
    T_DIS_OFFSET_REGISTER_06 
    DIS_Register_00c0;

    //0x00c4
    T_DIS_OFFSET_REGISTER_07 
    DIS_Register_00c4;

    //0x00c8
    T_DIS_OFFSET_REGISTER_08 
    DIS_Register_00c8;

    //0x00cc
    T_DIS_OFFSET_REGISTER_09 
    DIS_Register_00cc;

    //0x00d0
    T_DIS_OFFSET_REGISTER_10 
    DIS_Register_00d0;

    //0x00d4
    T_DIS_OFFSET_REGISTER_11 
    DIS_Register_00d4;

    //0x00d8
    T_DIS_OFFSET_REGISTER_12 
    DIS_Register_00d8;

    //0x00dc
    T_DIS_OFFSET_REGISTER_13 
    DIS_Register_00dc;

    //0x00e0
    T_DIS_OFFSET_REGISTER_14 
    DIS_Register_00e0;

    //0x00e4
    T_DIS_OFFSET_REGISTER_15 
    DIS_Register_00e4;

    //0x00e8
    T_DIS_CREDITS 
    DIS_Register_00e8;

    //0x00ec
    T_LL_FRAME_REGISTER_0 
    DIS_Register_00ec;

    //0x00f0
    T_LL_FRAME_REGISTER_1 
    DIS_Register_00f0;

    //0x00f4
    T_LL_FRAME_REGISTER_2 
    DIS_Register_00f4;

    //0x00f8
    T_LL_FRAME_REGISTER_3 
    DIS_Register_00f8;

    //0x00fc
    T_LL_REGISTER 
    DIS_Register_00fc;

    //0x0100
    T_DIS_COUNT_REGISTER0 
    DIS_Register_0100;

    //0x0104
    T_DIS_COUNT_REGISTER1 
    DIS_Register_0104;

    //0x0108
    T_DIS_COUNT_REGISTER2 
    DIS_Register_0108;

    //0x010c
    T_DIS_COUNT_REGISTER3 
    DIS_Register_010c;

    //0x0110
    T_DIS_DEBUG_MESSAGES 
    DIS_Register_0110;

    //0x0114
    T_DIS_CYCLE_COUNTER_REGISTER 
    DIS_Register_0114;

	//0x0118
//	T_DIS_LL_CMD_START_ADDR_INFO_REGISTER
//	DIS_Register_0118;

	//0x011c
//	T_DIS_LL_CMDPRS_CNT_REGISTER
//	DIS_Register_011c;
} NT98560_DIS_REG_STRUCT;
#endif



#endif
