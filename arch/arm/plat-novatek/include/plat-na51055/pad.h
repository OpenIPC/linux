/**
	PAD controller header

	PAD controller header

	@file       pad.h
	@ingroup    mIDrvSys_PAD
	@note       Nothing

	Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 2 as
	published by the Free Software Foundation.
*/

#ifndef _PAD_H
#define _PAD_H

#include <mach/nvt_type.h>

/**
    @addtogroup mIDrvSys_PAD
*/
//@{

#define	PAD_C_GPIO_BASE         0
#define	PAD_S_GPIO_BASE         96
#define	PAD_P_GPIO_BASE         128
#define	PAD_L_GPIO_BASE         224
#define	PAD_D_GPIO_BASE         288
#define	PAD_H_GPIO_BASE         320
#define	PAD_A_GPIO_BASE         352
#define PAD_DS_GROUP_10         0x00000000
#define PAD_DS_GROUP_16         0x10000000
#define PAD_DS_GROUP_40         0x80000000

/**
    @name Pad type pin ID.

    Pad type pin ID.

    Pad ID of pad_set_pull_updown (), pad_get_pull_updown ().
*/
#define PAD_PIN_NOT_EXIST       (64) // For backward compatible
//@{
typedef enum {

	//C_GPIO group
	PAD_PIN_CGPIO0     =     (PAD_C_GPIO_BASE + 0),     ///< C_GPIO_0
	PAD_PIN_CGPIO1     =     (PAD_C_GPIO_BASE + 2),     ///< C_GPIO_1
	PAD_PIN_CGPIO2     =     (PAD_C_GPIO_BASE + 4),     ///< C_GPIO_2
	PAD_PIN_CGPIO3     =     (PAD_C_GPIO_BASE + 6),     ///< C_GPIO_3
	PAD_PIN_CGPIO4     =     (PAD_C_GPIO_BASE + 8),     ///< C_GPIO_4
	PAD_PIN_CGPIO5     =     (PAD_C_GPIO_BASE + 10),    ///< C_GPIO_5
	PAD_PIN_CGPIO6     =     (PAD_C_GPIO_BASE + 12),    ///< C_GPIO_6
	PAD_PIN_CGPIO7     =     (PAD_C_GPIO_BASE + 14),    ///< C_GPIO_7
	PAD_PIN_CGPIO8     =     (PAD_C_GPIO_BASE + 16),    ///< C_GPIO_8
	PAD_PIN_CGPIO9     =     (PAD_C_GPIO_BASE + 18),    ///< C_GPIO_9
	PAD_PIN_CGPIO10    =     (PAD_C_GPIO_BASE + 20),    ///< C_GPIO_10
	PAD_PIN_CGPIO11    =     (PAD_C_GPIO_BASE + 22),    ///< C_GPIO_11
	PAD_PIN_CGPIO12    =     (PAD_C_GPIO_BASE + 24),    ///< C_GPIO_12
	PAD_PIN_CGPIO13    =     (PAD_C_GPIO_BASE + 26),    ///< C_GPIO_13
	PAD_PIN_CGPIO14    =     (PAD_C_GPIO_BASE + 28),    ///< C_GPIO_14
	PAD_PIN_CGPIO15    =     (PAD_C_GPIO_BASE + 30),    ///< C_GPIO_15
	PAD_PIN_CGPIO16    =     (PAD_C_GPIO_BASE + 32),    ///< C_GPIO_16
	PAD_PIN_CGPIO17    =     (PAD_C_GPIO_BASE + 34),    ///< C_GPIO_17
	PAD_PIN_CGPIO18    =     (PAD_C_GPIO_BASE + 36),    ///< C_GPIO_18
	PAD_PIN_CGPIO19    =     (PAD_C_GPIO_BASE + 38),    ///< C_GPIO_19
	PAD_PIN_CGPIO20    =     (PAD_C_GPIO_BASE + 40),    ///< C_GPIO_20
	PAD_PIN_CGPIO21    =     (PAD_C_GPIO_BASE + 42),    ///< C_GPIO_21
	PAD_PIN_CGPIO22    =     (PAD_C_GPIO_BASE + 44),    ///< C_GPIO_22

	//S_GPIO group
	PAD_PIN_SGPIO0      =    (PAD_S_GPIO_BASE + 0),     ///< S_GPIO_0
	PAD_PIN_SGPIO1      =    (PAD_S_GPIO_BASE + 2),     ///< S_GPIO_1
	PAD_PIN_SGPIO2      =    (PAD_S_GPIO_BASE + 4),     ///< S_GPIO_2
	PAD_PIN_SGPIO3      =    (PAD_S_GPIO_BASE + 6),     ///< S_GPIO_3
	PAD_PIN_SGPIO4      =    (PAD_S_GPIO_BASE + 8),     ///< S_GPIO_4
	PAD_PIN_SGPIO5      =    (PAD_S_GPIO_BASE + 10),    ///< S_GPIO_5
	PAD_PIN_SGPIO6      =    (PAD_S_GPIO_BASE + 12),    ///< S_GPIO_6
	PAD_PIN_SGPIO7      =    (PAD_S_GPIO_BASE + 14),    ///< S_GPIO_7
	PAD_PIN_SGPIO8      =    (PAD_S_GPIO_BASE + 16),    ///< S_GPIO_8
	PAD_PIN_SGPIO9      =    (PAD_S_GPIO_BASE + 18),    ///< S_GPIO_9
	PAD_PIN_SGPIO10     =    (PAD_S_GPIO_BASE + 20),    ///< S_GPIO_10
	PAD_PIN_SGPIO11     =    (PAD_S_GPIO_BASE + 22),    ///< S_GPIO_11
	PAD_PIN_SGPIO12     =    (PAD_S_GPIO_BASE + 24),    ///< S_GPIO_12

	//P_GPIO group
	PAD_PIN_PGPIO0      =    (PAD_P_GPIO_BASE + 0),     ///< P_GPIO_0
	PAD_PIN_PGPIO1      =    (PAD_P_GPIO_BASE + 2),     ///< P_GPIO_1
	PAD_PIN_PGPIO2      =    (PAD_P_GPIO_BASE + 4),     ///< P_GPIO_2
	PAD_PIN_PGPIO3      =    (PAD_P_GPIO_BASE + 6),     ///< P_GPIO_3
	PAD_PIN_PGPIO4      =    (PAD_P_GPIO_BASE + 8),     ///< P_GPIO_4
	PAD_PIN_PGPIO5      =    (PAD_P_GPIO_BASE + 10),    ///< P_GPIO_5
	PAD_PIN_PGPIO6      =    (PAD_P_GPIO_BASE + 12),    ///< P_GPIO_6
	PAD_PIN_PGPIO7      =    (PAD_P_GPIO_BASE + 14),    ///< P_GPIO_7
	PAD_PIN_PGPIO8      =    (PAD_P_GPIO_BASE + 16),    ///< P_GPIO_8
	PAD_PIN_PGPIO9      =    (PAD_P_GPIO_BASE + 18),    ///< P_GPIO_9
	PAD_PIN_PGPIO10     =    (PAD_P_GPIO_BASE + 20),    ///< P_GPIO_10
	PAD_PIN_PGPIO11     =    (PAD_P_GPIO_BASE + 22),    ///< P_GPIO_11
	PAD_PIN_PGPIO12     =    (PAD_P_GPIO_BASE + 24),    ///< P_GPIO_12
	PAD_PIN_PGPIO13     =    (PAD_P_GPIO_BASE + 26),    ///< P_GPIO_13
	PAD_PIN_PGPIO14     =    (PAD_P_GPIO_BASE + 28),    ///< P_GPIO_14
	PAD_PIN_PGPIO15     =    (PAD_P_GPIO_BASE + 30),    ///< P_GPIO_15
	PAD_PIN_PGPIO16     =    (PAD_P_GPIO_BASE + 32),    ///< P_GPIO_16
	PAD_PIN_PGPIO17     =    (PAD_P_GPIO_BASE + 34),    ///< P_GPIO_17
	PAD_PIN_PGPIO18     =    (PAD_P_GPIO_BASE + 36),    ///< P_GPIO_18
	PAD_PIN_PGPIO19     =    (PAD_P_GPIO_BASE + 38),    ///< P_GPIO_19
	PAD_PIN_PGPIO20     =    (PAD_P_GPIO_BASE + 40),    ///< P_GPIO_20
	PAD_PIN_PGPIO21     =    (PAD_P_GPIO_BASE + 42),    ///< P_GPIO_21
	PAD_PIN_PGPIO22     =    (PAD_P_GPIO_BASE + 44),    ///< P_GPIO_22
	PAD_PIN_PGPIO23     =    (PAD_P_GPIO_BASE + 46),    ///< P_GPIO_23
	PAD_PIN_PGPIO24     =    (PAD_P_GPIO_BASE + 48),    ///< P_GPIO_24

	//L_GPIO group
	PAD_PIN_LGPIO0      =    (PAD_L_GPIO_BASE + 0),      ///< L_GPIO_0
	PAD_PIN_LGPIO1      =    (PAD_L_GPIO_BASE + 2),      ///< L_GPIO_1
	PAD_PIN_LGPIO2      =    (PAD_L_GPIO_BASE + 4),      ///< L_GPIO_2
	PAD_PIN_LGPIO3      =    (PAD_L_GPIO_BASE + 6),      ///< L_GPIO_3
	PAD_PIN_LGPIO4      =    (PAD_L_GPIO_BASE + 8),      ///< L_GPIO_4
	PAD_PIN_LGPIO5      =    (PAD_L_GPIO_BASE + 10),     ///< L_GPIO_5
	PAD_PIN_LGPIO6      =    (PAD_L_GPIO_BASE + 12),     ///< L_GPIO_6
	PAD_PIN_LGPIO7      =    (PAD_L_GPIO_BASE + 14),     ///< L_GPIO_7
	PAD_PIN_LGPIO8      =    (PAD_L_GPIO_BASE + 16),     ///< L_GPIO_8
	PAD_PIN_LGPIO9      =    (PAD_L_GPIO_BASE + 18),     ///< L_GPIO_9
	PAD_PIN_LGPIO10     =    (PAD_L_GPIO_BASE + 20),     ///< L_GPIO_10
	PAD_PIN_LGPIO11     =    (PAD_L_GPIO_BASE + 22),     ///< L_GPIO_11
	PAD_PIN_LGPIO12     =    (PAD_L_GPIO_BASE + 24),     ///< L_GPIO_12
	PAD_PIN_LGPIO13     =    (PAD_L_GPIO_BASE + 26),     ///< L_GPIO_13
	PAD_PIN_LGPIO14     =    (PAD_L_GPIO_BASE + 28),     ///< L_GPIO_14
	PAD_PIN_LGPIO15     =    (PAD_L_GPIO_BASE + 30),     ///< L_GPIO_15
	PAD_PIN_LGPIO16     =    (PAD_L_GPIO_BASE + 32),     ///< L_GPIO_16
	PAD_PIN_LGPIO17     =    (PAD_L_GPIO_BASE + 34),     ///< L_GPIO_17
	PAD_PIN_LGPIO18     =    (PAD_L_GPIO_BASE + 36),     ///< L_GPIO_18
	PAD_PIN_LGPIO19     =    (PAD_L_GPIO_BASE + 38),     ///< L_GPIO_19
	PAD_PIN_LGPIO20     =    (PAD_L_GPIO_BASE + 40),     ///< L_GPIO_20
	PAD_PIN_LGPIO21     =    (PAD_L_GPIO_BASE + 42),     ///< L_GPIO_21
	PAD_PIN_LGPIO22     =    (PAD_L_GPIO_BASE + 44),     ///< L_GPIO_22
	PAD_PIN_LGPIO23     =    (PAD_L_GPIO_BASE + 46),     ///< L_GPIO_23
	PAD_PIN_LGPIO24     =    (PAD_L_GPIO_BASE + 48),     ///< L_GPIO_23

	//D_GPIO group
	PAD_PIN_DGPIO0      =    (PAD_D_GPIO_BASE + 0),      ///< D_GPIO_0
	PAD_PIN_DGPIO1      =    (PAD_D_GPIO_BASE + 2),      ///< D_GPIO_1
	PAD_PIN_DGPIO2      =    (PAD_D_GPIO_BASE + 4),      ///< D_GPIO_2
	PAD_PIN_DGPIO3      =    (PAD_D_GPIO_BASE + 6),      ///< D_GPIO_3
	PAD_PIN_DGPIO4      =    (PAD_D_GPIO_BASE + 8),      ///< D_GPIO_4
	PAD_PIN_DGPIO5      =    (PAD_D_GPIO_BASE + 10),     ///< D_GPIO_5
	PAD_PIN_DGPIO6      =    (PAD_D_GPIO_BASE + 12),     ///< D_GPIO_6
	PAD_PIN_DGPIO7      =    (PAD_D_GPIO_BASE + 14),     ///< D_GPIO_7

	//HSI_GPIO group
	PAD_PIN_HSIGPIO0    =    (PAD_H_GPIO_BASE + 0),      ///< HSI_GPIO_0
	PAD_PIN_HSIGPIO1    =    (PAD_H_GPIO_BASE + 2),      ///< HSI_GPIO_1
	PAD_PIN_HSIGPIO2    =    (PAD_H_GPIO_BASE + 4),      ///< HSI_GPIO_2
	PAD_PIN_HSIGPIO3    =    (PAD_H_GPIO_BASE + 6),      ///< HSI_GPIO_3
	PAD_PIN_HSIGPIO4    =    (PAD_H_GPIO_BASE + 8),      ///< HSI_GPIO_4
	PAD_PIN_HSIGPIO5    =    (PAD_H_GPIO_BASE + 10),     ///< HSI_GPIO_5
	PAD_PIN_HSIGPIO6    =    (PAD_H_GPIO_BASE + 12),     ///< HSI_GPIO_6
	PAD_PIN_HSIGPIO7    =    (PAD_H_GPIO_BASE + 14),     ///< HSI_GPIO_7
	PAD_PIN_HSIGPIO8    =    (PAD_H_GPIO_BASE + 16),     ///< HSI_GPIO_8
	PAD_PIN_HSIGPIO9    =    (PAD_H_GPIO_BASE + 18),     ///< HSI_GPIO_9
	PAD_PIN_HSIGPIO10   =    (PAD_H_GPIO_BASE + 20),     ///< HSI_GPIO_10
	PAD_PIN_HSIGPIO11   =    (PAD_H_GPIO_BASE + 22),     ///< HSI_GPIO_11

	//A_GPIO group
	PAD_PIN_AGPIO0      =    (PAD_A_GPIO_BASE + 0),      ///< A_GPIO_0
	PAD_PIN_AGPIO1      =    (PAD_A_GPIO_BASE + 2),      ///< A_GPIO_1
	PAD_PIN_AGPIO2      =    (PAD_A_GPIO_BASE + 4),      ///< A_GPIO_2

	PAD_PIN_MAX         =     PAD_PIN_AGPIO2,
	ENUM_DUMMY4WORD(PAD_PIN)
} PAD_PIN;

//@}

/**
    Pad type select

    Pad type select

    Pad type value for pad_set_pull_updown(), pad_get_pull_updown().
*/
typedef enum {
	PAD_NONE         =          0x00,      ///< none of pull up/down
	PAD_PULLDOWN     =          0x01,      ///< pull down
	PAD_PULLUP       =          0x02,      ///< pull up
	PAD_KEEPER       =          0x03,      ///< keeper

	ENUM_DUMMY4WORD(PAD_PULL)
} PAD_PULL;

/**
    Pad driving select

    Pad driving select

    Pad driving value for pad_set_drivingsink(), pad_get_drivingsink().
*/
typedef enum {
	PAD_DRIVINGSINK_4MA     =    0x0001,     ///< Pad driver/sink 4mA
	PAD_DRIVINGSINK_10MA    =    0x0202,     ///< Pad driver/sink 10mA
	PAD_DRIVINGSINK_6MA     =    0x0010,     ///< Pad driver/sink 6mA
	PAD_DRIVINGSINK_16MA    =    0x0020,     ///< Pad driver/sink 16mA
	PAD_DRIVINGSINK_5MA     =    0x0100,     ///< Pad driver/sink 5mA
	PAD_DRIVINGSINK_15MA    =    0x0400,     ///< Pad driver/sink 15mA
	PAD_DRIVINGSINK_20MA    =    0x0800,     ///< Pad driver/sink 20mA
	PAD_DRIVINGSINK_25MA    =    0x1000,     ///< Pad driver/sink 25mA
	PAD_DRIVINGSINK_30MA    =    0x2000,     ///< Pad driver/sink 30mA
	PAD_DRIVINGSINK_35MA    =    0x4000,     ///< Pad driver/sink 35mA
	PAD_DRIVINGSINK_40MA    =    0x8000,     ///< Pad driver/sink 40mA
	PAD_DRIVINGSINK_8MA     =    0x10000,    ///< Pad driver/sink 8mA
	PAD_DRIVINGSINK_12MA    =    0x20000,    ///< Pad driver/sink 12mA

	ENUM_DUMMY4WORD(PAD_DRIVINGSINK)
} PAD_DRIVINGSINK;

typedef enum {
	//C_GPIO group
	PAD_DS_CGPIO0     =    ((PAD_C_GPIO_BASE +  0)    |    PAD_DS_GROUP_10), ///< C_GPIO_0
	PAD_DS_CGPIO1     =    ((PAD_C_GPIO_BASE +  2)    |    PAD_DS_GROUP_10), ///< C_GPIO_1
	PAD_DS_CGPIO2     =    ((PAD_C_GPIO_BASE +  4)    |    PAD_DS_GROUP_10), ///< C_GPIO_2
	PAD_DS_CGPIO3     =    ((PAD_C_GPIO_BASE +  6)    |    PAD_DS_GROUP_10), ///< C_GPIO_3
	PAD_DS_CGPIO4     =    ((PAD_C_GPIO_BASE +  8)    |    PAD_DS_GROUP_10), ///< C_GPIO_4
	PAD_DS_CGPIO5     =    ((PAD_C_GPIO_BASE + 10)    |    PAD_DS_GROUP_10), ///< C_GPIO_5
	PAD_DS_CGPIO6     =    ((PAD_C_GPIO_BASE + 12)    |    PAD_DS_GROUP_10), ///< C_GPIO_6
	PAD_DS_CGPIO7     =    ((PAD_C_GPIO_BASE + 14)    |    PAD_DS_GROUP_10), ///< C_GPIO_7
	PAD_DS_CGPIO8     =    ((PAD_C_GPIO_BASE + 16)    |    PAD_DS_GROUP_16), ///< C_GPIO_8
	PAD_DS_CGPIO9     =    ((PAD_C_GPIO_BASE + 18)    |    PAD_DS_GROUP_10), ///< C_GPIO_9
	PAD_DS_CGPIO10    =    ((PAD_C_GPIO_BASE + 20)    |    PAD_DS_GROUP_10), ///< C_GPIO_10
	PAD_DS_CGPIO11    =    ((PAD_C_GPIO_BASE + 0)     |    PAD_DS_GROUP_40), ///< C_GPIO_11
	PAD_DS_CGPIO12    =    ((PAD_C_GPIO_BASE + 4)     |    PAD_DS_GROUP_40), ///< C_GPIO_12
	PAD_DS_CGPIO13    =    ((PAD_C_GPIO_BASE + 8)     |    PAD_DS_GROUP_40), ///< C_GPIO_13
	PAD_DS_CGPIO14    =    ((PAD_C_GPIO_BASE + 12)    |    PAD_DS_GROUP_40), ///< C_GPIO_14
	PAD_DS_CGPIO15    =    ((PAD_C_GPIO_BASE + 16)    |    PAD_DS_GROUP_40), ///< C_GPIO_15
	PAD_DS_CGPIO16    =    ((PAD_C_GPIO_BASE + 20)    |    PAD_DS_GROUP_40), ///< C_GPIO_16
	PAD_DS_CGPIO17    =    ((PAD_C_GPIO_BASE + 24)    |    PAD_DS_GROUP_40), ///< C_GPIO_17
	PAD_DS_CGPIO18    =    ((PAD_C_GPIO_BASE + 36)    |    PAD_DS_GROUP_16), ///< C_GPIO_18
	PAD_DS_CGPIO19    =    ((PAD_C_GPIO_BASE + 38)    |    PAD_DS_GROUP_16), ///< C_GPIO_19
	PAD_DS_CGPIO20    =    ((PAD_C_GPIO_BASE + 40)    |    PAD_DS_GROUP_16), ///< C_GPIO_20
	PAD_DS_CGPIO21    =    ((PAD_C_GPIO_BASE + 42)    |    PAD_DS_GROUP_16), ///< C_GPIO_21
	PAD_DS_CGPIO22    =    ((PAD_C_GPIO_BASE + 44)    |    PAD_DS_GROUP_16), ///< C_GPIO_22

	//S_GPIO group
	PAD_DS_SGPIO0     =    ((PAD_S_GPIO_BASE + 0)     |    PAD_DS_GROUP_16), ///< S_GPIO_0
	PAD_DS_SGPIO1     =    ((PAD_S_GPIO_BASE + 2)     |    PAD_DS_GROUP_16), ///< S_GPIO_1
	PAD_DS_SGPIO2     =    ((PAD_S_GPIO_BASE + 4)     |    PAD_DS_GROUP_10), ///< S_GPIO_2
	PAD_DS_SGPIO3     =    ((PAD_S_GPIO_BASE + 6)     |    PAD_DS_GROUP_10), ///< S_GPIO_3
	PAD_DS_SGPIO4     =    ((PAD_S_GPIO_BASE + 8)     |    PAD_DS_GROUP_10), ///< S_GPIO_4
	PAD_DS_SGPIO5     =    ((PAD_S_GPIO_BASE + 10)    |    PAD_DS_GROUP_16), ///< S_GPIO_5
	PAD_DS_SGPIO6     =    ((PAD_S_GPIO_BASE + 12)    |    PAD_DS_GROUP_10), ///< S_GPIO_6
	PAD_DS_SGPIO7     =    ((PAD_S_GPIO_BASE + 14)    |    PAD_DS_GROUP_10), ///< S_GPIO_7
	PAD_DS_SGPIO8     =    ((PAD_S_GPIO_BASE + 16)    |    PAD_DS_GROUP_10), ///< S_GPIO_8
	PAD_DS_SGPIO9     =    ((PAD_S_GPIO_BASE + 18)    |    PAD_DS_GROUP_10), ///< S_GPIO_9
	PAD_DS_SGPIO10    =    ((PAD_S_GPIO_BASE + 20)    |    PAD_DS_GROUP_10), ///< S_GPIO_10
	PAD_DS_SGPIO11    =    ((PAD_S_GPIO_BASE + 22)    |    PAD_DS_GROUP_10), ///< S_GPIO_11
	PAD_DS_SGPIO12    =    ((PAD_S_GPIO_BASE + 24)    |    PAD_DS_GROUP_10), ///< S_GPIO_12

	//P_GPIO group
	PAD_DS_PGPIO0     =    ((PAD_P_GPIO_BASE + 0)     |    PAD_DS_GROUP_10), ///< P_GPIO_0
	PAD_DS_PGPIO1     =    ((PAD_P_GPIO_BASE + 2)     |    PAD_DS_GROUP_10), ///< P_GPIO_1
	PAD_DS_PGPIO2     =    ((PAD_P_GPIO_BASE + 4)     |    PAD_DS_GROUP_10), ///< P_GPIO_2
	PAD_DS_PGPIO3     =    ((PAD_P_GPIO_BASE + 6)     |    PAD_DS_GROUP_10), ///< P_GPIO_3
	PAD_DS_PGPIO4     =    ((PAD_P_GPIO_BASE + 8)     |    PAD_DS_GROUP_10), ///< P_GPIO_4
	PAD_DS_PGPIO5     =    ((PAD_P_GPIO_BASE + 10)    |    PAD_DS_GROUP_10), ///< P_GPIO_5
	PAD_DS_PGPIO6     =    ((PAD_P_GPIO_BASE + 12)    |    PAD_DS_GROUP_10), ///< P_GPIO_6
	PAD_DS_PGPIO7     =    ((PAD_P_GPIO_BASE + 14)    |    PAD_DS_GROUP_10), ///< P_GPIO_7
	PAD_DS_PGPIO8     =    ((PAD_P_GPIO_BASE + 16)    |    PAD_DS_GROUP_16), ///< P_GPIO_8
	PAD_DS_PGPIO9     =    ((PAD_P_GPIO_BASE + 18)    |    PAD_DS_GROUP_10), ///< P_GPIO_9
	PAD_DS_PGPIO10    =    ((PAD_P_GPIO_BASE + 20)    |    PAD_DS_GROUP_10), ///< P_GPIO_10
	PAD_DS_PGPIO11    =    ((PAD_P_GPIO_BASE + 22)    |    PAD_DS_GROUP_10), ///< P_GPIO_11
	PAD_DS_PGPIO12    =    ((PAD_P_GPIO_BASE + 24)    |    PAD_DS_GROUP_10), ///< P_GPIO_12
	PAD_DS_PGPIO13    =    ((PAD_P_GPIO_BASE + 26)    |    PAD_DS_GROUP_10), ///< P_GPIO_13
	PAD_DS_PGPIO14    =    ((PAD_P_GPIO_BASE + 28)    |    PAD_DS_GROUP_10), ///< P_GPIO_14
	PAD_DS_PGPIO15    =    ((PAD_P_GPIO_BASE + 30)    |    PAD_DS_GROUP_10), ///< P_GPIO_15
	PAD_DS_PGPIO16    =    ((PAD_P_GPIO_BASE + 32)    |    PAD_DS_GROUP_10), ///< P_GPIO_16
	PAD_DS_PGPIO17    =    ((PAD_P_GPIO_BASE + 34)    |    PAD_DS_GROUP_10), ///< P_GPIO_17
	PAD_DS_PGPIO18    =    ((PAD_P_GPIO_BASE + 36)    |    PAD_DS_GROUP_10), ///< P_GPIO_18
	PAD_DS_PGPIO19    =    ((PAD_P_GPIO_BASE + 38)    |    PAD_DS_GROUP_10), ///< P_GPIO_19
	PAD_DS_PGPIO20    =    ((PAD_P_GPIO_BASE + 40)    |    PAD_DS_GROUP_10), ///< P_GPIO_20
	PAD_DS_PGPIO21    =    ((PAD_P_GPIO_BASE + 42)    |    PAD_DS_GROUP_10), ///< P_GPIO_21
	PAD_DS_PGPIO22    =    ((PAD_P_GPIO_BASE + 44)    |    PAD_DS_GROUP_10), ///< P_GPIO_22
	PAD_DS_PGPIO23    =    ((PAD_P_GPIO_BASE + 46)    |    PAD_DS_GROUP_10), ///< P_GPIO_23
	PAD_DS_PGPIO24    =    ((PAD_P_GPIO_BASE + 48)    |    PAD_DS_GROUP_10), ///< P_GPIO_24

	//L_GPIO group
	PAD_DS_LGPIO0     =    ((PAD_L_GPIO_BASE + 0)     |    PAD_DS_GROUP_10), ///< L_GPIO_0
	PAD_DS_LGPIO1     =    ((PAD_L_GPIO_BASE + 2)     |    PAD_DS_GROUP_10), ///< L_GPIO_1
	PAD_DS_LGPIO2     =    ((PAD_L_GPIO_BASE + 4)     |    PAD_DS_GROUP_10), ///< L_GPIO_2
	PAD_DS_LGPIO3     =    ((PAD_L_GPIO_BASE + 6)     |    PAD_DS_GROUP_10), ///< L_GPIO_3
	PAD_DS_LGPIO4     =    ((PAD_L_GPIO_BASE + 8)     |    PAD_DS_GROUP_10), ///< L_GPIO_4
	PAD_DS_LGPIO5     =    ((PAD_L_GPIO_BASE + 10)    |    PAD_DS_GROUP_10), ///< L_GPIO_5
	PAD_DS_LGPIO6     =    ((PAD_L_GPIO_BASE + 12)    |    PAD_DS_GROUP_10), ///< L_GPIO_6
	PAD_DS_LGPIO7     =    ((PAD_L_GPIO_BASE + 14)    |    PAD_DS_GROUP_10), ///< L_GPIO_7
	PAD_DS_LGPIO8     =    ((PAD_L_GPIO_BASE + 16)    |    PAD_DS_GROUP_16), ///< L_GPIO_8
	PAD_DS_LGPIO9     =    ((PAD_L_GPIO_BASE + 18)    |    PAD_DS_GROUP_10), ///< L_GPIO_9
	PAD_DS_LGPIO10    =    ((PAD_L_GPIO_BASE + 20)    |    PAD_DS_GROUP_10), ///< L_GPIO_10
	PAD_DS_LGPIO11    =    ((PAD_L_GPIO_BASE + 22)    |    PAD_DS_GROUP_10), ///< L_GPIO_11
	PAD_DS_LGPIO12    =    ((PAD_L_GPIO_BASE + 24)    |    PAD_DS_GROUP_10), ///< L_GPIO_12
	PAD_DS_LGPIO13    =    ((PAD_L_GPIO_BASE + 26)    |    PAD_DS_GROUP_10), ///< L_GPIO_13
	PAD_DS_LGPIO14    =    ((PAD_L_GPIO_BASE + 28)    |    PAD_DS_GROUP_10), ///< L_GPIO_14
	PAD_DS_LGPIO15    =    ((PAD_L_GPIO_BASE + 30)    |    PAD_DS_GROUP_10), ///< L_GPIO_15
	PAD_DS_LGPIO16    =    ((PAD_L_GPIO_BASE + 32)    |    PAD_DS_GROUP_10), ///< L_GPIO_16
	PAD_DS_LGPIO17    =    ((PAD_L_GPIO_BASE + 34)    |    PAD_DS_GROUP_10), ///< L_GPIO_17
	PAD_DS_LGPIO18    =    ((PAD_L_GPIO_BASE + 36)    |    PAD_DS_GROUP_10), ///< L_GPIO_18
	PAD_DS_LGPIO19    =    ((PAD_L_GPIO_BASE + 38)    |    PAD_DS_GROUP_10), ///< L_GPIO_19
	PAD_DS_LGPIO20    =    ((PAD_L_GPIO_BASE + 40)    |    PAD_DS_GROUP_10), ///< L_GPIO_20
	PAD_DS_LGPIO21    =    ((PAD_L_GPIO_BASE + 42)    |    PAD_DS_GROUP_10), ///< L_GPIO_21
	PAD_DS_LGPIO22    =    ((PAD_L_GPIO_BASE + 44)    |    PAD_DS_GROUP_10), ///< L_GPIO_22
	PAD_DS_LGPIO23    =    ((PAD_L_GPIO_BASE + 46)    |    PAD_DS_GROUP_10), ///< L_GPIO_23
	PAD_DS_LGPIO24    =    ((PAD_L_GPIO_BASE + 48)    |    PAD_DS_GROUP_10), ///< L_GPIO_23

	//D_GPIO group
	PAD_DS_DGPIO0     =    ((PAD_D_GPIO_BASE + 0)     |    PAD_DS_GROUP_16), ///< D_GPIO_0
	PAD_DS_DGPIO1     =    ((PAD_D_GPIO_BASE + 2)     |    PAD_DS_GROUP_16), ///< D_GPIO_1
	PAD_DS_DGPIO2     =    ((PAD_D_GPIO_BASE + 4)     |    PAD_DS_GROUP_16), ///< D_GPIO_2
	PAD_DS_DGPIO3     =    ((PAD_D_GPIO_BASE + 6)     |    PAD_DS_GROUP_16), ///< D_GPIO_3
	PAD_DS_DGPIO4     =    ((PAD_D_GPIO_BASE + 8)     |    PAD_DS_GROUP_16), ///< D_GPIO_4
	PAD_DS_DGPIO5     =    ((PAD_D_GPIO_BASE + 10)    |    PAD_DS_GROUP_16), ///< D_GPIO_5
	PAD_DS_DGPIO6     =    ((PAD_D_GPIO_BASE + 12)    |    PAD_DS_GROUP_16), ///< D_GPIO_6
	PAD_DS_DGPIO7     =    ((PAD_D_GPIO_BASE + 14)    |    PAD_DS_GROUP_10), ///< D_GPIO_7

	//HSI_GPIO group
	PAD_DS_HSIGPIO0   =    ((PAD_H_GPIO_BASE + 0)     |    PAD_DS_GROUP_10), ///< HSI_GPIO_0
	PAD_DS_HSIGPIO1   =    ((PAD_H_GPIO_BASE + 2)     |    PAD_DS_GROUP_10), ///< HSI_GPIO_1
	PAD_DS_HSIGPIO2   =    ((PAD_H_GPIO_BASE + 4)     |    PAD_DS_GROUP_10), ///< HSI_GPIO_2
	PAD_DS_HSIGPIO3   =    ((PAD_H_GPIO_BASE + 6)     |    PAD_DS_GROUP_10), ///< HSI_GPIO_3
	PAD_DS_HSIGPIO4   =    ((PAD_H_GPIO_BASE + 8)     |    PAD_DS_GROUP_10), ///< HSI_GPIO_4
	PAD_DS_HSIGPIO5   =    ((PAD_H_GPIO_BASE + 10)    |    PAD_DS_GROUP_10), ///< HSI_GPIO_5
	PAD_DS_HSIGPIO6   =    ((PAD_H_GPIO_BASE + 12)    |    PAD_DS_GROUP_10), ///< HSI_GPIO_6
	PAD_DS_HSIGPIO7   =    ((PAD_H_GPIO_BASE + 14)    |    PAD_DS_GROUP_10), ///< HSI_GPIO_7
	PAD_DS_HSIGPIO8   =    ((PAD_H_GPIO_BASE + 16)    |    PAD_DS_GROUP_10), ///< HSI_GPIO_8
	PAD_DS_HSIGPIO9   =    ((PAD_H_GPIO_BASE + 18)    |    PAD_DS_GROUP_10), ///< HSI_GPIO_9
	PAD_DS_HSIGPIO10  =    ((PAD_H_GPIO_BASE + 20)    |    PAD_DS_GROUP_10), ///< HSI_GPIO_10
	PAD_DS_HSIGPIO11  =    ((PAD_H_GPIO_BASE + 22)    |    PAD_DS_GROUP_10), ///< HSI_GPIO_11

	//A_GPIO group
	PAD_DS_AGPIO0     =    ((PAD_A_GPIO_BASE + 0)     |    PAD_DS_GROUP_10), ///< A_GPIO_0
	PAD_DS_AGPIO1     =    ((PAD_A_GPIO_BASE + 2)     |    PAD_DS_GROUP_10), ///< A_GPIO_1
	PAD_DS_AGPIO2     =    ((PAD_A_GPIO_BASE + 4)     |    PAD_DS_GROUP_10), ///< A_GPIO_2

	PAD_DS_MAX = PAD_DS_AGPIO2,
	ENUM_DUMMY4WORD(PAD_DS)
} PAD_DS;

/**
    Pad power ID select

    Pad power ID for PAD_POWER_STRUCT.
*/
typedef enum {
	PAD_POWERID_MC0  =          0x00,      ///< Pad power id for MC0
	PAD_POWERID_MC1  =          0x01,      ///< Pad power id for MC1
	PAD_POWERID_ADC  =          0x02,      ///< Pad power id for ADC
	PAD_POWERID_CSI  =          0x04,      ///< Pad power id for CSI

	ENUM_DUMMY4WORD(PAD_POWERID)
} PAD_POWERID;

/**
    Pad power select

    Pad power value for PAD_POWER_STRUCT.
*/
typedef enum {
	PAD_3P3V         =          0x00,      ///< Pad power is 3.3V
	PAD_AVDD         =          0x00,      ///< Pad power is AVDD ( for PAD_POWERID_ADC use)
	PAD_1P8V         =          0x01,      ///< Pad power is 1.8V
	PAD_PAD_VAD      =          0x01,      ///< Pad power is PAD_ADC_VAD ( for PAD_POWERID_ADC use)

	ENUM_DUMMY4WORD(PAD_POWER)
} PAD_POWER;

/**
    Pad power VAD for  PAD_POWERID_ADC

    Pad power VAD value for PAD_POWER_STRUCT.
*/
typedef enum {
	PAD_VAD_2P9V     =          0x00,      ///< Pad power VAD = 2.9V <ADC>
	PAD_VAD_3P0V     =          0x01,      ///< Pad power VAD = 3.0V <ADC>
	PAD_VAD_3P1V     =          0x03,      ///< Pad power VAD = 3.1V <ADC>

	PAD_VAD_2P4V     =          0x100,     ///< Pad power VAD = 2.4V <CSI>
	PAD_VAD_2P5V     =          0x101,     ///< Pad power VAD = 2.5V <CSI>
	PAD_VAD_2P6V     =          0x103,     ///< Pad power VAD = 2.6V <CSI>


	ENUM_DUMMY4WORD(PAD_VAD)
} PAD_VAD;


/**
    PAD power structure

    PAD power setting for pad_setPower()
*/
typedef struct {
	PAD_POWERID         pad_power_id;         ///< Pad power id
	PAD_POWER           pad_power;           ///< Pad power
	BOOL                bias_current;       ///< Regulator bias current selection
	///< - @b FALSE: disable
	///< - @b TRUE: enable
	BOOL                opa_gain;           ///< Regulator OPA gain/phase selection
	///< - @b FALSE: disable
	///< - @b TRUE: enable
	BOOL                pull_down;          ///< Regulator output pull down control
	///< - @b FALSE: none
	///< - @b TRUE: pull down enable
	BOOL                enable;            ///< Regulator enable
	///< - @b FALSE: disable
	///< - @b TRUE: enable
	PAD_VAD             pad_vad;             ///< Pad VAD of PAD_POWERID_ADC when PAD_PAD_VAD = 1
} PAD_POWER_STRUCT;

extern ER pad_set_pull_updown(PAD_PIN pin, PAD_PULL pulltype);
extern ER pad_get_pull_updown(PAD_PIN pin, PAD_PULL *pulltype);
extern ER pad_set_drivingsink(PAD_DS name, PAD_DRIVINGSINK driving);
extern ER pad_get_drivingsink(PAD_DS name, PAD_DRIVINGSINK *driving);
extern ER pad_set_power(PAD_POWER_STRUCT *pad_power);
extern void pad_get_power(PAD_POWER_STRUCT *pad_power);


//@}

#endif
