#ifndef __GPIO_TABLE_H__
#define __GPIO_TABLE_H__

#include "mhal_gpio.h"

//==============================================================================
//
//                              STRUCTURE
//
//==============================================================================

struct gpio_setting {
    U32 r_oen;
    U8  m_oen;
    U32 r_out;
    U8  m_out;
    U32 r_in;
    U8  m_in;
};

extern const struct gpio_setting gpio_table[];

#endif /* __GPIO_TABLE_H__ */
