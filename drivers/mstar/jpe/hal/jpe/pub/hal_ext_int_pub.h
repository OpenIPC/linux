////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2009 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// (¡§MStar Confidential Information¡¨) by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////

/**
* @file    hal_ext_int-pub.h
* @version
* @brief   External interrupt controller header file
*
*/

#ifndef __HAL_EXT_INT_PUB_H__
#define __HAL_EXT_INT_PUB_H__

/*=============================================================*/
// Include files
/*=============================================================*/
//#include "vm_types.ht"
#include <asm/types.h>
#include <linux/interrupt.h>

/*=============================================================*/
// Extern definition
/*=============================================================*/

/*=============================================================*/
// Macro definition
/*=============================================================*/

/*=============================================================*/
// Data type definition
/*=============================================================*/
typedef enum
{
    EXT_INT_NUM_0=0,
    EXT_INT_NUM_1,
    EXT_INT_NUM_2,
    EXT_INT_NUM_3,
    EXT_INT_NUM_4,
    EXT_INT_NUM_5,
    EXT_INT_NUM_6,
    EXT_INT_NUM_7,
    EXT_INT_NUM_8,
    EXT_INT_NUM_9,
    EXT_INT_NUM_10,
    EXT_INT_NUM_11,
    EXT_INT_NUM_12,
    EXT_INT_NUM_13,
    EXT_INT_NUM_14,
    EXT_INT_NUM_15,
    EXT_INT_NUM_MAX
} ExtIntNumber_e;

/**
 * clock division factor type for debounce
 */
typedef enum
{
    EXT_INT_CLK_DIV_2=0,
    EXT_INT_CLK_DIV_4,
    EXT_INT_CLK_DIV_8,
    EXT_INT_CLK_DIV_16,
    EXT_INT_CLK_DIV_32,
    EXT_INT_CLK_DIV_64,
    EXT_INT_CLK_DIV_128,
    EXT_INT_CLK_DIV_256,
    EXT_INT_CLK_DIV_MAX
} ExtIntClkDiv_e;

/**
 * Mapping of internal interrupt controller
 */
typedef enum
{
    EXT_INT_MAP_1=0,
    EXT_INT_MAP_2,
    EXT_INT_MAP_3,
    EXT_INT_MAP_4,
    EXT_INT_MAP_5,
    EXT_INT_MAP_6,
    EXT_INT_MAP_7,
    EXT_INT_MAP_8,
    EXT_INT_MAP_MAX
} ExtIntMap_e;

/**
 * Polarity of the interrupt line
 */
typedef enum
{
    EXT_INT_POLARITY_NORMAL=0, /// normal polarity (rising edge/high level)
    EXT_INT_POLARITY_REVERSE,  /// reverse polarity (falling edge/low level)
    EXT_INT_POLARITY_MAX
} ExtIntPolarity_e;

/**
 * Mode type
 */
typedef enum
{
    EXT_INT_MODE_BYPASS=0,           /// bypass mode
    EXT_INT_MODE_SHORT_PULSE,        /// short pulse mode
    EXT_INT_MODE_DEBOUNCE,           /// debounce mode
    EXT_INT_MODE_DEBOUNCE_BOTH_EDGE, /// debounce + both edge detector mode
    EXT_INT_MODE_MAX
} ExtIntMode_e;

/**
 * Ack type
 */
typedef enum
{
    EXT_INT_CLEAR_PRE=0,  /// auto-clear before isr invocation
    EXT_INT_CLEAR_POST,   /// auto-clear after isr invocation
    EXT_INT_CLEAR_MANUAL, /// manual clear
    EXT_INT_CLEAR_MAX
} ExtIntClear_e;

/**
 * GPIO pull type
 */
typedef enum
{
    EXT_INT_PULL_DISABLE=0, /// disable pull functionaility
    EXT_INT_PULL_DOWN,      /// pull-down this GPIO
    EXT_INT_PULL_UP,        /// pull-up this GPIO
    EXT_INT_PULL_DEFAULT,   /// don't care
    EXT_INT_PULL_MAX
} ExtIntPull_e;

/**
 * Debounce type
 */
typedef enum
{
    EXT_INT_DEBOUNCE_0=0, /// disable debounce functionaility
    EXT_INT_DEBOUNCE_1,   /// debounce 1 clock
    EXT_INT_DEBOUNCE_2,   /// debounce 2 clocks
    EXT_INT_DEBOUNCE_3,   /// debounce 3 clocks
    EXT_INT_DEBOUNCE_4,   /// debounce 4 clocks
    EXT_INT_DEBOUNCE_5,   /// debounce 5 clocks
    EXT_INT_DEBOUNCE_6,   /// debounce 6 clocks
    EXT_INT_DEBOUNCE_7,   /// debounce 7 clocks
    EXT_INT_DEBOUNCE_MAX
} ExtIntDebounce_e;

/**
 * Interrupt Service Routine type
 */
//typedef void (*PfnExtIntISR)(void);
typedef irqreturn_t (*PfnExtIntISR)(int eIntNum, void* dev_id);


/**
 * Interrupt control parameters
 */
typedef struct
{
    ExtIntMap_e      eMap;
    ExtIntPolarity_e ePolarity;
    ExtIntMode_e     eMode;
    ExtIntClear_e    eClear;
    ExtIntPull_e     ePull;
    ExtIntDebounce_e eDebounce;
    PfnExtIntISR     pfnIsr;
} ExtIntParam_t;

/*=============================================================*/
// Variable definition
/*=============================================================*/

/*=============================================================*/
// Global function definition
/*=============================================================*/

#endif // __HAL_EXT_INT_PUB_H__

