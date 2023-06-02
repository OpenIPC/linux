/**
    GPIO header file
    This file will setup gpio related base address
    @file       nvt-gpio.h
    @ingroup
    @note
    Copyright   Novatek Microelectronics Corp. 2014.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/
#ifndef __ASM_ARCH_NA51055_NVT_GPIO_H
#define __ASM_ARCH_NA51055_NVT_GPIO_H
#include <kwrap/nvt_type.h>
#include "../comm/driver.h"
/**
    @addtogroup mIDrvIO_GPIO
*/
//@{

/**
    GPIO direction

    GPIO direction definition for gpio_setDir() and gpio_getDir()
*/
typedef enum {
	GPIO_DIR_INPUT =            0,      ///< GPIO is input direction
	GPIO_DIR_OUTPUT =           1,      ///< GPIO is output direction

	ENUM_DUMMY4WORD(GPIO_DIR)
} GPIO_DIR;

/**
    GPIO interrupt type

    GPIO interrupt type definition for type argument of gpio_setIntTypePol()
*/
typedef enum {
	GPIO_INTTYPE_EDGE =         0,      ///< GPIO interrupt is edge trigger
	GPIO_INTTYPE_LEVEL =        1,      ///< GPIO interrupt is level trigger

	ENUM_DUMMY4WORD(GPIO_INTTYPE)
} GPIO_INTTYPE;

/**
    GPIO interrupt polarity

    GPIO interrupt polarity definition for pol argument of gpio_setIntTypePol()
*/
typedef enum {
	GPIO_INTPOL_POSHIGH =       0,      ///< GPIO interrupt polarity is \n
	///< - @b positvie edge for edge trigger
	///< - @b high level for level trigger
	GPIO_INTPOL_NEGLOW =        1,      ///< GPIO interrupt polarity is \n
	///< - @b negative edge for edge trigger
	///< - @b low level for level trigger
	GPIO_INTPOL_BOTHEDGE =      2,      ///< GPIO interrupt polarity is \n
	///< - @b both edge for edge trigger
	ENUM_DUMMY4WORD(GPIO_INTPOL)
} GPIO_INTPOL;



/**
    @name   GPIO pins ID

    GPIO pins ID definition

    For detail GPIO pin out, please refer to NT96650 data sheet.
*/
//@{
/*Storage GPIO - CGPIO*/
/**
    @name   GPIO pins ID

    GPIO pins ID definition

    For detail GPIO pin out, please refer to NT96520 data sheet.
*/
//@{
/*Storage GPIO - CGPIO*/
#define C_GPIO_0    0                       ///< C_GPIO[0]
#define C_GPIO_1    1                       ///< C_GPIO[1]
#define C_GPIO_2    2                       ///< C_GPIO[2]
#define C_GPIO_3    3                       ///< C_GPIO[3]
#define C_GPIO_4    4                       ///< C_GPIO[4]
#define C_GPIO_5    5                       ///< C_GPIO[5]
#define C_GPIO_6    6                       ///< C_GPIO[6]
#define C_GPIO_7    7                       ///< C_GPIO[7]
#define C_GPIO_8    8                       ///< C_GPIO[8]
#define C_GPIO_9    9                       ///< C_GPIO[9]
#define C_GPIO_10   10                      ///< C_GPIO[10]
#define C_GPIO_11   11                      ///< C_GPIO[11]
#define C_GPIO_12   12                      ///< C_GPIO[12]
#define C_GPIO_13   13                      ///< C_GPIO[13]
#define C_GPIO_14   14                      ///< C_GPIO[14]
#define C_GPIO_15   15                      ///< C_GPIO[15]
#define C_GPIO_16   16                      ///< C_GPIO[16]
#define C_GPIO_17   17                      ///< C_GPIO[17]
#define C_GPIO_18   18                      ///< C_GPIO[18]
#define C_GPIO_19   19                      ///< C_GPIO[19]
#define C_GPIO_20   20                      ///< C_GPIO[20]
#define C_GPIO_21   21                      ///< C_GPIO[21]
#define C_GPIO_22   22                      ///< C_GPIO[22]

/*Peripheral GPIO - PGPIO*/
#define P_GPIO_SHIFT_BASE 32
#define P_GPIO_0    (0 +P_GPIO_SHIFT_BASE)  ///< P_GPIO[0]
#define P_GPIO_1    (1 +P_GPIO_SHIFT_BASE)  ///< P_GPIO[1]
#define P_GPIO_2    (2 +P_GPIO_SHIFT_BASE)  ///< P_GPIO[2]
#define P_GPIO_3    (3 +P_GPIO_SHIFT_BASE)  ///< P_GPIO[3]
#define P_GPIO_4    (4 +P_GPIO_SHIFT_BASE)  ///< P_GPIO[4]
#define P_GPIO_5    (5 +P_GPIO_SHIFT_BASE)  ///< P_GPIO[5]
#define P_GPIO_6    (6 +P_GPIO_SHIFT_BASE)  ///< P_GPIO[6]
#define P_GPIO_7    (7 +P_GPIO_SHIFT_BASE)  ///< P_GPIO[7]
#define P_GPIO_8    (8 +P_GPIO_SHIFT_BASE)  ///< P_GPIO[8]
#define P_GPIO_9    (9 +P_GPIO_SHIFT_BASE)  ///< P_GPIO[9]
#define P_GPIO_10   (10+P_GPIO_SHIFT_BASE)  ///< P_GPIO[10]
#define P_GPIO_11   (11+P_GPIO_SHIFT_BASE)  ///< P_GPIO[11]
#define P_GPIO_12   (12+P_GPIO_SHIFT_BASE)  ///< P_GPIO[12]
#define P_GPIO_13   (13+P_GPIO_SHIFT_BASE)  ///< P_GPIO[13]
#define P_GPIO_14   (14+P_GPIO_SHIFT_BASE)  ///< P_GPIO[14]
#define P_GPIO_15   (15+P_GPIO_SHIFT_BASE)  ///< P_GPIO[15]
#define P_GPIO_16   (16+P_GPIO_SHIFT_BASE)  ///< P_GPIO[16]
#define P_GPIO_17   (17+P_GPIO_SHIFT_BASE)  ///< P_GPIO[17]
#define P_GPIO_18   (18+P_GPIO_SHIFT_BASE)  ///< P_GPIO[18]
#define P_GPIO_19   (19+P_GPIO_SHIFT_BASE)  ///< P_GPIO[19]
#define P_GPIO_20   (20+P_GPIO_SHIFT_BASE)  ///< P_GPIO[20]
#define P_GPIO_21   (21+P_GPIO_SHIFT_BASE)  ///< P_GPIO[21]
#define P_GPIO_22   (22+P_GPIO_SHIFT_BASE)  ///< P_GPIO[22]
#define P_GPIO_23   (23+P_GPIO_SHIFT_BASE)  ///< P_GPIO[23]
#define P_GPIO_24   (24+P_GPIO_SHIFT_BASE)  ///< P_GPIO[24]
#define P_GPIO_25   (25+P_GPIO_SHIFT_BASE)  ///< P_GPIO[25]

/*Sensor GPIO - SGPIO*/
#define S_GPIO_SHIFT_BASE 64
#define S_GPIO_0    (0 +S_GPIO_SHIFT_BASE)  ///< S_GPIO[0]
#define S_GPIO_1    (1 +S_GPIO_SHIFT_BASE)  ///< S_GPIO[1]
#define S_GPIO_2    (2 +S_GPIO_SHIFT_BASE)  ///< S_GPIO[2]
#define S_GPIO_3    (3 +S_GPIO_SHIFT_BASE)  ///< S_GPIO[3]
#define S_GPIO_4    (4 +S_GPIO_SHIFT_BASE)  ///< S_GPIO[4]
#define S_GPIO_5    (5 +S_GPIO_SHIFT_BASE)  ///< S_GPIO[5]
#define S_GPIO_6    (6 +S_GPIO_SHIFT_BASE)  ///< S_GPIO[6]
#define S_GPIO_7    (7 +S_GPIO_SHIFT_BASE)  ///< S_GPIO[7]
#define S_GPIO_8    (8 +S_GPIO_SHIFT_BASE)  ///< S_GPIO[8]
#define S_GPIO_9    (9 +S_GPIO_SHIFT_BASE)  ///< S_GPIO[9]
#define S_GPIO_10   (10+S_GPIO_SHIFT_BASE)  ///< S_GPIO[10]
#define S_GPIO_11   (11+S_GPIO_SHIFT_BASE)  ///< S_GPIO[11]
#define S_GPIO_12   (12+S_GPIO_SHIFT_BASE)  ///< S_GPIO[11]


/*LCD GPIO - LGPIO*/
#define L_GPIO_SHIFT_BASE 96
#define L_GPIO_0    (0 +L_GPIO_SHIFT_BASE)  ///< L_GPIO[0]
#define L_GPIO_1    (1 +L_GPIO_SHIFT_BASE)  ///< L_GPIO[1]
#define L_GPIO_2    (2 +L_GPIO_SHIFT_BASE)  ///< L_GPIO[2]
#define L_GPIO_3    (3 +L_GPIO_SHIFT_BASE)  ///< L_GPIO[3]
#define L_GPIO_4    (4 +L_GPIO_SHIFT_BASE)  ///< L_GPIO[4]
#define L_GPIO_5    (5 +L_GPIO_SHIFT_BASE)  ///< L_GPIO[5]
#define L_GPIO_6    (6 +L_GPIO_SHIFT_BASE)  ///< L_GPIO[6]
#define L_GPIO_7    (7 +L_GPIO_SHIFT_BASE)  ///< L_GPIO[7]
#define L_GPIO_8    (8 +L_GPIO_SHIFT_BASE)  ///< L_GPIO[8]
#define L_GPIO_9    (9 +L_GPIO_SHIFT_BASE)  ///< L_GPIO[9]
#define L_GPIO_10   (10+L_GPIO_SHIFT_BASE)  ///< L_GPIO[10]
#define L_GPIO_11   (11+L_GPIO_SHIFT_BASE)  ///< L_GPIO[11]
#define L_GPIO_12   (12+L_GPIO_SHIFT_BASE)  ///< L_GPIO[12]
#define L_GPIO_13   (13+L_GPIO_SHIFT_BASE)  ///< L_GPIO[13]
#define L_GPIO_14   (14+L_GPIO_SHIFT_BASE)  ///< L_GPIO[14]
#define L_GPIO_15   (15+L_GPIO_SHIFT_BASE)  ///< L_GPIO[15]
#define L_GPIO_16   (16+L_GPIO_SHIFT_BASE)  ///< L_GPIO[16]
#define L_GPIO_17   (17+L_GPIO_SHIFT_BASE)  ///< L_GPIO[17]
#define L_GPIO_18   (18+L_GPIO_SHIFT_BASE)  ///< L_GPIO[18]
#define L_GPIO_19   (19+L_GPIO_SHIFT_BASE)  ///< L_GPIO[19]
#define L_GPIO_20   (20+L_GPIO_SHIFT_BASE)  ///< L_GPIO[20]
#define L_GPIO_21   (21+L_GPIO_SHIFT_BASE)  ///< L_GPIO[21]
#define L_GPIO_22   (22+L_GPIO_SHIFT_BASE)  ///< L_GPIO[22]
#define L_GPIO_23   (23+L_GPIO_SHIFT_BASE)  ///< L_GPIO[23]
#define L_GPIO_24   (24+L_GPIO_SHIFT_BASE)  ///< L_GPIO[24]

/*Dedicated GPIO - DGPIO*/
// In order to backward comaptible, DGPIO is used as " GPIO_IS_DGPIO | D_GPIO_* "
#define D_GPIO_SHIFT_BASE 128
#define D_GPIO_0    (0 +D_GPIO_SHIFT_BASE)  ///< DGPIO[0]
#define D_GPIO_1    (1 +D_GPIO_SHIFT_BASE)  ///< DGPIO[1]
#define D_GPIO_2    (2 +D_GPIO_SHIFT_BASE)  ///< DGPIO[2]
#define D_GPIO_3    (3 +D_GPIO_SHIFT_BASE)  ///< DGPIO[3]
#define D_GPIO_4    (4 +D_GPIO_SHIFT_BASE)  ///< DGPIO[4]
#define D_GPIO_5    (5 +D_GPIO_SHIFT_BASE)  ///< DGPIO[5]
#define D_GPIO_6    (6 +D_GPIO_SHIFT_BASE)  ///< DGPIO[6]
#define D_GPIO_7    (7 +D_GPIO_SHIFT_BASE)  ///< DGPIO[7]
#define D_GPIO_8    (8 +D_GPIO_SHIFT_BASE)  ///< DGPIO[8]
#define D_GPIO_9    (9 +D_GPIO_SHIFT_BASE)  ///< DGPIO[9]
#define D_GPIO_10   (10+D_GPIO_SHIFT_BASE)  ///< DGPIO[10]
#define D_GPIO_11   (11+D_GPIO_SHIFT_BASE)  ///< DGPIO[11]
#define D_GPIO_12   (12+D_GPIO_SHIFT_BASE)  ///< DGPIO[12]
#define D_GPIO_13   (13+D_GPIO_SHIFT_BASE)  ///< DGPIO[13]
#define D_GPIO_14   (14+D_GPIO_SHIFT_BASE)  ///< DGPIO[14]
#define D_GPIO_15   (15+D_GPIO_SHIFT_BASE)  ///< DGPIO[15]
#define D_GPIO_16   (16 +D_GPIO_SHIFT_BASE) ///< D_GPIO[16] (USB Wakeup) (No pad instance)
#define D_GPIO_17   (17+D_GPIO_SHIFT_BASE)  ///< DGPIO[17]
#define D_GPIO_18   (18+D_GPIO_SHIFT_BASE)  ///< DGPIO[18]
#define D_GPIO_19   (19+D_GPIO_SHIFT_BASE)  ///< DGPIO[19]
#define D_GPIO_20   (20+D_GPIO_SHIFT_BASE)  ///< DGPIO[20]
#define D_GPIO_21   (21+D_GPIO_SHIFT_BASE)  ///< DGPIO[21]

/*GPIO HSI Data register(High speed interface)*/
#define H_GPIO_SHIFT_BASE 160
#define H_GPIO_0    (0 +H_GPIO_SHIFT_BASE)  ///< HSI_GPIO[0]
#define H_GPIO_1    (1 +H_GPIO_SHIFT_BASE)  ///< HSI_GPIO[1]
#define H_GPIO_2    (2 +H_GPIO_SHIFT_BASE)  ///< HSI_GPIO[2]
#define H_GPIO_3    (3 +H_GPIO_SHIFT_BASE)  ///< HSI_GPIO[3]
#define H_GPIO_4    (4 +H_GPIO_SHIFT_BASE)  ///< HSI_GPIO[4]
#define H_GPIO_5    (5 +H_GPIO_SHIFT_BASE)  ///< HSI_GPIO[5]
#define H_GPIO_6    (6 +H_GPIO_SHIFT_BASE)  ///< HSI_GPIO[6]
#define H_GPIO_7    (7 +H_GPIO_SHIFT_BASE)  ///< HSI_GPIO[7]
#define H_GPIO_8    (8 +H_GPIO_SHIFT_BASE)  ///< HSI_GPIO[8]
#define H_GPIO_9    (9 +H_GPIO_SHIFT_BASE)  ///< HSI_GPIO[9]
#define H_GPIO_10   (10+H_GPIO_SHIFT_BASE)  ///< HSI_GPIO[10]
#define H_GPIO_11   (11+H_GPIO_SHIFT_BASE)  ///< HSI_GPIO[11]

/*GPIO ADC Data register*/
#define A_GPIO_SHIFT_BASE 192
#define A_GPIO_0    (0 + A_GPIO_SHIFT_BASE)  ///< A_GPIO[0]
#define A_GPIO_1    (1 + A_GPIO_SHIFT_BASE)  ///< A_GPIO[1]
#define A_GPIO_2    (2 + A_GPIO_SHIFT_BASE)  ///< A_GPIO[2]

/*GPIO DSI Data register*/
#define DSI_GPIO_SHIFT_BASE 224
#define DSI_GPIO_0  (0 + DSI_GPIO_SHIFT_BASE)  ///< DSI_GPIO[0]
#define DSI_GPIO_1  (1 + DSI_GPIO_SHIFT_BASE)  ///< DSI_GPIO[1]
#define DSI_GPIO_2  (2 + DSI_GPIO_SHIFT_BASE)  ///< DSI_GPIO[2]
#define DSI_GPIO_3  (3 + DSI_GPIO_SHIFT_BASE)  ///< DSI_GPIO[3]
#define DSI_GPIO_4  (4 + DSI_GPIO_SHIFT_BASE)  ///< DSI_GPIO[4]
#define DSI_GPIO_5  (5 + DSI_GPIO_SHIFT_BASE)  ///< DSI_GPIO[5]
#define DSI_GPIO_6  (6 + DSI_GPIO_SHIFT_BASE)  ///< DSI_GPIO[6]
#define DSI_GPIO_7  (7 + DSI_GPIO_SHIFT_BASE)  ///< DSI_GPIO[7]
#define DSI_GPIO_8  (8 + DSI_GPIO_SHIFT_BASE)  ///< DSI_GPIO[8]
#define DSI_GPIO_9  (9 + DSI_GPIO_SHIFT_BASE)  ///< DSI_GPIO[9]
#define DSI_GPIO_10 (10+ DSI_GPIO_SHIFT_BASE)  ///< DSI_GPIO[10]
//@}






/**
    @name   GPIO Interrupt ID

    GPIO interrupt ID definition

    GPIO interrupt ID for interrupt related APIs.
*/
//@{
#define GPIO_INT_00                 0       ///< GPIO INT[0]:   C_GPIO[3]
#define GPIO_INT_01                 1       ///< GPIO INT[1]:   C_GPIO[5]
#define GPIO_INT_02                 2       ///< GPIO INT[2]:   C_GPIO[7]
#define GPIO_INT_03                 3       ///< GPIO INT[3]:   C_GPIO[9]
#define GPIO_INT_04                 4       ///< GPIO INT[4]:   C_GPIO[12]
#define GPIO_INT_05                 5       ///< GPIO INT[5]:   C_GPIO[14]
#define GPIO_INT_06                 6       ///< GPIO INT[6]:   C_GPIO[16]
#define GPIO_INT_07                 7       ///< GPIO INT[7]:   C_GPIO[18]
#define GPIO_INT_08                 8       ///< GPIO INT[8]:   C_GPIO[20]
#define GPIO_INT_09                 9       ///< GPIO INT[9]:   C_GPIO[22]
#define GPIO_INT_10                 10      ///< GPIO INT[10]:  H_GPIO[0]
#define GPIO_INT_11                 11      ///< GPIO INT[11]:  H_GPIO[11]
#define GPIO_INT_12                 12      ///< GPIO INT[12]:  S_GPIO[1]
#define GPIO_INT_13                 13      ///< GPIO INT[13]:  S_GPIO[4]
#define GPIO_INT_14                 14      ///< GPIO INT[14]:  S_GPIO[6]
#define GPIO_INT_15                 15      ///< GPIO INT[15]:  S_GPIO[8]
#define GPIO_INT_16                 16      ///< GPIO INT[16]:  S_GPIO[2]
#define GPIO_INT_17                 17      ///< GPIO INT[17]:  P_GPIO[3]
#define GPIO_INT_18                 18      ///< GPIO INT[18]:  P_GPIO[7]
#define GPIO_INT_19                 19      ///< GPIO INT[19]:  P_GPIO[8]
#define GPIO_INT_20                 20      ///< GPIO INT[20]:  P_GPIO[9]
#define GPIO_INT_21                 21      ///< GPIO INT[21]:  P_GPIO[11]
#define GPIO_INT_22                 22      ///< GPIO INT[22]:  P_GPIO[20]
#define GPIO_INT_23                 23      ///< GPIO INT[23]:  P_GPIO[17]
#define GPIO_INT_24                 24      ///< GPIO INT[24]:  P_GPIO[18]
#define GPIO_INT_25                 25      ///< GPIO INT[25]:  P_GPIO[24]
#define GPIO_INT_26                 26      ///< GPIO INT[26]:  DSI_GPIO[1]
#define GPIO_INT_27                 27      ///< GPIO INT[27]:  DSI_GPIO[6]
#define GPIO_INT_28                 28      ///< GPIO INT[28]:  DSI_GPIO[1]
#define GPIO_INT_29                 29      ///< GPIO INT[29]:  L_GPIO[4]
#define GPIO_INT_30                 30      ///< GPIO INT[30]:  L_GPIO[8]
#define GPIO_INT_31                 31      ///< GPIO INT[31]:  L_GPIO[0]
#define GPIO_INT_32                 32      ///< DGPIO INT[0]:  D_GPIO[0]
#define GPIO_INT_33                 33      ///< DGPIO INT[1]:  D_GPIO[1]
#define GPIO_INT_34                 34      ///< DGPIO INT[2]:  D_GPIO[2]
#define GPIO_INT_35                 35      ///< DGPIO INT[3]:  D_GPIO[3]
#define GPIO_INT_36                 36      ///< DGPIO INT[4]:  D_GPIO[4]
#define GPIO_INT_37                 37      ///< DGPIO INT[5]:  D_GPIO[5]
#define GPIO_INT_38                 38      ///< DGPIO INT[6]:  D_GPIO[6]
#define GPIO_INT_39                 39      ///< DGPIO INT[7]:  D_GPIO[7]
#define GPIO_INT_40                 40      ///< DGPIO INT[8]:  D_GPIO[8]
#define GPIO_INT_41                 41      ///< DGPIO INT[9]:  D_GPIO[9]
#define GPIO_INT_42                 42      ///< DGPIO INT[10]: D_GPIO[10]
#define GPIO_INT_43                 43      ///< DGPIO INT[11]: D_GPIO[11]
#define GPIO_INT_44                 44      ///< DGPIO INT[12]: D_GPIO[12]
#define GPIO_INT_45                 45      ///< DGPIO INT[13]: D_GPIO[13]
#define GPIO_INT_46                 46      ///< DGPIO INT[14]: D_GPIO[14]
#define GPIO_INT_47                 47      ///< DGPIO INT[15]: D_GPIO[15]
#define GPIO_INT_48                 48      ///< DGPIO INT[16]: USB wakeup
#define GPIO_INT_49                 49      ///< DGPIO INT[17]: D_GPIO[17]
#define GPIO_INT_50                 50      ///< DGPIO INT[18]: D_GPIO[18]
#define GPIO_INT_51                 51      ///< DGPIO INT[19]: D_GPIO[19]
#define GPIO_INT_52                 52      ///< DGPIO INT[20]: D_GPIO[20]
#define GPIO_INT_53                 53      ///< DGPIO INT[21]: D_GPIO[21]
#define GPIO_INT_54                 54      ///< DGPIO INT[22]: A_GPIO[0]
#define GPIO_INT_55                 55      ///< DGPIO INT[23]: A_GPIO[1]
#define GPIO_INT_56                 56      ///< DGPIO INT[24]: A_GPIO[2]
#define GPIO_INT_57                 57      ///< DGPIO INT[25]: L_GPIO[23]
#define GPIO_INT_58                 58      ///< DGPIO INT[26]: P_GPIO[0]
#define GPIO_INT_59                 59      ///< DGPIO INT[27]: P_GPIO[12]
#define GPIO_INT_60                 60      ///< DGPIO INT[28]: P_GPIO[19]
#define GPIO_INT_61                 61      ///< DGPIO INT[29]: P_GPIO[20]
#define GPIO_INT_62                 62      ///< DGPIO INT[30]: P_GPIO[22]
#define GPIO_INT_63                 63      ///< DGPIO INT[31]: S_GPIO[9]

#define GPIO_INT_USBPLUGIN          (GPIO_INT_39)   ///< DGPIO INT[11]: USB VBUSI
#define GPIO_INT_USBWAKEUP          (GPIO_INT_48)   ///< DGPIO INT[16]: USB wakeup
//@}

// In order to backward comaptible, DGPIO is used as " GPIO_IS_DGPIO | D_GPIO_* " or "D_GPIO_*"
#define GPIO_IS_DGPIO               (128)


//
// The general api for the GPIO device driver
//
extern ER       gpio_open(void);
extern ER       gpio_close(void);
extern BOOL     gpio_isOpened(void);

extern ER       gpio_waitIntFlag(UINT32 gpioIntID);

// ---- GPIO ----
extern void     gpio_setDir(UINT32 pin, GPIO_DIR dir);
extern GPIO_DIR gpio_getDir(UINT32 pin);
extern void     gpio_setPin(UINT32 pin);
extern void     gpio_clearPin(UINT32 pin);
extern UINT32   gpio_getPin(UINT32 pin);
extern void     gpio_pullSet(UINT32 pin);
extern void     gpio_pullClear(UINT32 pin);
extern UINT32   gpio_readData(UINT32 dataidx);
extern void     gpio_writeData(UINT32 dataidx, UINT32 value);

// ---- DGPIO ----
extern UINT32   dgpio_readData(void);
extern void     dgpio_writeData(UINT32 value);

// ---- Interrupt Control ----
extern void     gpio_enableInt(UINT32 ipin);
extern void     gpio_disableInt(UINT32 ipin);
extern UINT32   gpio_getIntEnable(UINT32 ipin);
extern void     gpio_clearIntStatus(UINT32 ipin);
extern UINT32   gpio_getIntStatus(UINT32 ipin);
extern void     gpio_setIntTypePol(UINT32 ipin, GPIO_INTTYPE type, GPIO_INTPOL pol);
extern void     gpio_setIntIsr(UINT32 ipin, DRV_CB pHdl);
extern void     gpio_enableDestination(UINT32 ipin);
extern void     gpio_disableDestination(UINT32 ipin);

extern void     gpio_platform_init(void);
extern void     gpio_platform_uninit(void);

#define NVT_GPIO_STG_DATA_0  0x00
#define NVT_GPIO_STG_DIR_0   0x20
#define NVT_GPIO_STG_SET_0   0x40
#define NVT_GPIO_STG_CLR_0   0x60
#define NVT_GPIO_STS_CPU     0x80
#define NVT_GPIO_INTEN_CPU   0x90
#define NVT_GPIO_INT_TYPE    0xA0
#define NVT_GPIO_INT_POL     0xA4
#define NVT_GPIO_EDGE_TYPE   0xA8
#define NVT_DGPIO_STS_CPU    0xC0
#define NVT_DGPIO_INTEN_CPU  0xD0
#define NVT_DGPIO_INT_TYPE   0xE0
#define NVT_DGPIO_INT_POL    0xE4
#define NVT_DGPIO_EDGE_TYPE  0xE8

/* GPIO pin number translation  */
#define C_GPIO(pin)			(pin)
#define P_GPIO(pin)			(pin + 0x20)
#define S_GPIO(pin)			(pin + 0x40)
#define L_GPIO(pin)			(pin + 0x60)
#define D_GPIO(pin)			(pin + 0x80)
#define H_GPIO(pin)			(pin + 0xA0)
#define A_GPIO(pin)			(pin + 0xC0)
#define DSI_GPIO(pin)			(pin + 0xE0)

#define C_GPIO_NUM			23
#define P_GPIO_NUM			26
#define S_GPIO_NUM			13
#define L_GPIO_NUM			10
#define H_GPIO_NUM			12
#define D_GPIO_NUM			11
#define A_GPIO_NUM			3
#define DSI_GPIO_NUM			11


/*linux compatible*/
int gpio_get_value(UINT32 pin);
void gpio_set_value(UINT32 pin, int value);
void gpio_direction_input(UINT32 pin);
void gpio_direction_output(UINT32 pin, int value);
#endif /* __ASM_ARCH_NA51000_NVT_GPIO_H */
